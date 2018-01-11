/*
 * Copyright 2011 Ytai Ben-Tsvi. All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ARSHAN POURSOHI OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied.
 */

// Bootloader main

#include <string.h>
#include <stdint.h>

#include "HardwareProfile.h"
#include <libpic30.h>
#include "GenericTypeDefs.h"
#include "board.h"
#include "boot_protocol.h"
#include "bootloader_defs.h"
#include "flash.h"
#include "logging.h"
#include "ioio_file.h"
#include "USB/usb_common.h"
#include "USB/usb_function_cdc.h"
#include "USB/usb_device.h"
#include "boot_features.h"

#include "sdcard/FSIO.h"
#include "sdcard/SD-SPI.h"

#include "blapi/version.h"


//#define LogBootloader(f, ...) printf("[bootloader.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogBootloaderRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogBootloader(f, ...)
#define LogBootloaderRaw(f, ...)


void bl_hexdump(const void *data, int size)
{
    if (size <= 0) return;
    int i;
    for (i=0; i<size;i++){
      LogBootloaderRaw("%02X ", ((uint8_t *)data)[i]);
    }
}




// Desired behavior:
// 1. Read the status of the "boot" pin (LED). If high, skip the bootloader
//    and run the app.
// 2. Wait for the "boot" pin to go high.
// 3. Initialize USB device.
// 4. Wait for an incoming connection and run the bootloader protocol. Repeat
//    forever (although one of the protocol commands forces a reset, which will
//    kick us out).


////////////////////////////////////////////////////////////////////////////////
// Configuration Bits
#if defined(__PIC24FJ256GB206__)
  _CONFIG1(FWDTEN_OFF & ICS_PGx2 & GWRP_OFF & GCP_OFF & JTAGEN_OFF)
  _CONFIG2(POSCMOD_NONE & IOL1WAY_ON & OSCIOFNC_ON & FCKSM_CSDCMD & FNOSC_FRCPLL & PLL96MHZ_ON & PLLDIV_NODIV & IESO_OFF)
  _CONFIG3(WPDIS_WPEN & WPFP_WPFP19 & WPCFG_WPCFGEN & WPEND_WPSTARTMEM & SOSCSEL_EC)
#else
  #error Unsupported target
#endif


// When IOIO gets reset for an unexpected reason, its default behavior is to
// restart normally. This is desired in most cases.
// For debug purposes, however, we want to know what went wrong. With defining
// SIGANAL_AFTER_BAD_RESET, on any reset other than power-on, IOIO will blink
// a 16-bit code (long-1 short-0) to designtae the error.
// One very useful case is catching failed asserts. Their blink code will start
// with short-long-...
#ifdef SIGNAL_AFTER_BAD_RESET
static void SignalBit(int bit) {
  led_on();
  __delay_ms(bit ? 900 : 100);
  led_off();
  __delay_ms(bit ? 100 : 900);
}

static void SignalWord(unsigned int word) {
  int i;
  for (i = 0; i < 16; ++i) {
    SignalBit(word & 0x8000);
    word <<= 1;
  }
}

static void SignalRcon() {
  log_printf("RCON = 0x%x", RCON);
  while (1) {
   SignalWord(RCON);
   Delay(8000000UL);
  }
}
#endif

static void Blink(int times) {
  while (times-- > 0) {
    led_on();
    __delay_ms(100);
    led_off();
    __delay_ms(200);
  }
}

// Measures the period of one USB frame, based on SOF interrupts.
// Depends on timer2/3 running in 32-bit configuration.
uint32_t MeasureSOF() {
    USBSOFIF = 1;  // Clear.
    while (!USBSOFIF);
    const uint32_t start = TMR2 | (((uint32_t) TMR3HLD) << 16);
    USBSOFIF = 1;  // Clear.
    while (!USBSOFIF);
    const uint32_t end = TMR2 | (((uint32_t) TMR3HLD) << 16);
    return end - start;
}

// Calibrate the oscillar according to the SOF clock coming from a USB host.
// This function will first block until a USB connection has been established.
// Then it will wait for 32 SOF's for the tuning process (will hang if they do
// not arrive!). Then, if will disconnect from the USB bus and wait 2 seconds.
// Upon exit, the _TUN register will hold the correct value.
static void OscCalibrate() {
  int i;

  led_init();
  USBInitialize();

  // Start timer2/3 as 32-bit, system clock (16MHz).
  T2CON = 0x0008;
  PR2 = PR3 = 0xFFFF;
  T2CON = 0x8008;

  // Wait for a USB connection. Blink meanwhile.
  int led_counter = 0;
  while (USBGetDeviceState() != POWERED_STATE) {
    USBTasks();
    if ((led_counter++ & 0x3FFF) == 0) led_toggle();
  }
  led_off();

  log_printf("Connected to host. Starting calibration.");
  LogBootloader("Connected to host. Starting calibration.\n");

  // Start from 0 (redundant, but better be explicit).
  _TUN = 0;

  for (i = 0; i < 32; ++i) {
    uint32_t duration;
    // Repeat for sanity: if for some reason the SOF is too long or too short,
    // ignore it.
    do {
      duration = MeasureSOF();
    } while (duration < 15000 || duration > 17000);

    if (duration < 16000 && _TUN != 31) {
      // We're running too slow.
      _TUN++;
    } else if (duration > 16000 && _TUN != 32) {
      // We're running too fast.
      _TUN--;
    }
  }
  log_printf("Duration: %lu, TUN=%d", MeasureSOF(), _TUN);
  LogBootloader("Duration: %lu, TUN=%d\n", MeasureSOF(), _TUN);

  // Undo side-effects.
  T2CON = 0x0000;

  // Detach, wait.
  USBShutdown();
  __delay_ms(2000);
}

// Tune the oscillator by reading the value from configuration, or otherwise
// start the tuning process and store the result.
void OscCalibrateCached() {
  BYTE tun = ReadOscTun();
  LogBootloader("Read tune value 0x%X.\n", tun);
  log_printf("Read tune value 0x%X.", tun);
  if (tun > 0x3F) {
    LogBootloader("Entering oscillator calibration process.\n");
    log_printf("Entering oscillator calibration process.");
    OscCalibrate();
    WriteOscTun(_TUN);  // Write result to flash.
  } else {
    _TUN = tun;
  }
}

static bool IsPin1Grounded() {
  bool result;
  pin1_pullup = 1;
  result = !pin1_read();
  pin1_pullup = 0;
  return result;
}

static bool ShouldEnterBootloader() {
#ifdef BOOTLOADER_ON_ILLEGAL_OPCODE
  bool const result = !led_read() || _IOPUWR;
  _IOPUWR = 0;
  return result;
#else
  return !led_read();
#endif
}




void UART1Init(void)
{
  // UART 1 on pins 3 and 4
  RPINR18 = 0x3f04;   // RX on RP4   PIN4
  RPOR1 = ( 0x0300 | ( RPOR1 & 0x00ff ) );    // TX on RP3    U1TX output function 3   PIN5
  // Setup
  //  UARTConfig(0, 34, 1, 0, 0);   // 115k  114285.7
  //  AssignUxRXIE(0, 0);  // disable RX int.
  //  AssignUxTXIE(0, 0);  // disable TX int.
  U1MODE = 0;          // disable UART
  U1BRG = 34;

  //  AssignUxRXIF(0, 0);  // clear RX int.
  //  AssignUxTXIF(0, 1);  // set TX int, since the hardware FIFO is empty.
  //  AssignUxRXIE(0, 1);  // enable RX int.
  U1MODEbits.BRGH = BRGH2;
  U1STA = 0;
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;
  IFS0bits.U1RXIF = 0;
}




//--------------------------------------------------------------------------------
//
// Firmware update
//

// Firmware Fingerprint
typedef struct __attribute__ ((packed))
{
  BYTE data[16];
} FirmwareFingerprint;

// Firmware Header
typedef struct __attribute__ ((packed))
{
  WORD rel_version;
  WORD dev_version;
  WORD num_blocks;
  WORD crc;
  FirmwareFingerprint fingerprint;
  BYTE imgHeader[8];
} FirmwareHeader;

static const BYTE ioio_img_header[8] = { 'I', 'O', 'I', 'O', '\1', '\0', '\0', '\0' };
static FirmwareHeader fwHeader;

int FirmwareUpdateImage()
{
  //
  FSFILE *file = FSfopen("FW.IMG", "r");
  if ( file != NULL ) {
    LogBootloader("-- image file FOUND\n");
    
    BYTE img_header[8];
    int num = FSfread(img_header, 1, 8, file);
    if ( num != 8 ) {
      LogBootloader("-- image file ERROR reading image header\n");
    } else {
      //      if ( memcmp(img_header, ioio_img_header, 8) != 0 ) {
      //	LogBootloader("-- image file INVALID HEADER\n");
      //      } else {
	LogBootloader("-- image file header = '");
	bl_hexdump(img_header, 8);
	LogBootloaderRaw("'\n");
	
	// write image
	static DWORD img_last_page = BOOTLOADER_INVALID_ADDRESS;
	DWORD address, page_address;
	int block_num = 0;
	BYTE block_buffer[196];
	num = FSfread(block_buffer, 1, 196, file);
	while ( num == 196 ) {
	  //LogBootloader("-- read block % 5d  num: %d\n", block_num, num);
	  
	  address = *((const DWORD *) block_buffer);
	  if (address & 0x7F) {
	    LogBootloader("-- Misaligned block: 0x%lx\n", address);
	    // bail out
	    num = 0;
	  }
	  if (address < BOOTLOADER_MIN_APP_ADDRESS || address >= BOOTLOADER_MAX_APP_ADDRESS) {
	    LogBootloader("-- Adderess outside of permitted range: 0x%lx\n", address);
	    // bail out
	    num = 0;
	  }
	  if (img_last_page != BOOTLOADER_INVALID_ADDRESS && address < img_last_page) {
	    LogBootloader("Out-of-order address: 0x%lx\n", address);
	    // bail out
	    num = 0;
	  }
	  
	  if ( num != 0 ) {
	    page_address = address & 0xFFFFFFC00ull;
	    if (page_address != img_last_page) {
		LogBootloader("    -- Erasing Flash page: 0x%lx  page: %lx\n", address, page_address);
	      if (!FlashErasePage(page_address)) {
		  LogBootloader("    -- Erasing Flash page: 0x%lx -- ERROR\n", address);		      
		num = 0;
	      }
	      img_last_page = page_address;
	    }
	    LogBootloader("    --   Writing Flash block: 0x%lx  page: %lx\n", address, page_address);
	    if (!FlashWriteBlock(address, (block_buffer + 4))) {
		LogBootloader("    --   Writing Flash block: 0x%lx -- ERROR\n", address);
	      num = 0;
	    }
	    
	    if ( num != 0 ) {
	      block_num++;
	      num = FSfread(block_buffer, 1, 196, file);
	    }
	  }
	}
	LogBootloader("-- wrote final block: %d   num: %d\n", block_num, num);
	//      }
    }
    // close image file
    FSfclose(file);
  } else {
    LogBootloader("-- image file NOT FOUND\n");	  
    return 1;
  }

  return 0;
}


void DumpConfigPage2()
{
    LogBootloaderRaw("[bootloader.c:%d] Config Page\n", __LINE__);

    int row, i;
    DWORD addr = BOOTLOADER_CONFIG_PAGE;
    for ( row = 0; row < 5; row++ ) {
        LogBootloaderRaw("[bootloader.c:%d]   row[%d]  addr: 0x%lx", __LINE__, row, addr);
        
        for (i = 0; i < DEVICE_UUID_SIZE / 2; ++i) {
            DWORD_VAL dw = {FlashReadDWORD(addr)};
//            *buffer++ = dw.byte.LB;
//            *buffer++ = dw.byte.HB;
            LogBootloaderRaw(" %02X %02X", dw.byte.LB, dw.byte.HB);
            
            addr += 2;
        }
        LogBootloaderRaw("\n");
    }
    LogBootloaderRaw("[bootloader.c:%d]\n", __LINE__);
}

void MaybeUpdateFirmware()
{
    LogBootloader("---- MaybeUpdateFirmware\n");
    
{
    LogBootloaderRaw("[bootloader.c:%d] Config Page\n", __LINE__);

    int row, i;
    DWORD addr = BOOTLOADER_CONFIG_PAGE;
    for ( row = 0; row < 5; row++ ) {
        LogBootloaderRaw("[bootloader.c:%d]   row[%d]  addr: 0x%lx", __LINE__, row, addr);
        
        for (i = 0; i < DEVICE_UUID_SIZE / 2; ++i) {
            DWORD_VAL dw = {FlashReadDWORD(addr)};
//            *buffer++ = dw.byte.LB;
//            *buffer++ = dw.byte.HB;
            LogBootloaderRaw(" %02X %02X", dw.byte.LB, dw.byte.HB);
            
            addr += 2;
        }
        LogBootloaderRaw("\n");
    }
    LogBootloaderRaw("[bootloader.c:%d]\n", __LINE__);
}
//    DumpConfigPage2();
    LogBootloader("\n");
    
    BYTE fp[FINGERPRINT_SIZE];
    ReadFingerprintToBuffer(fp);
    LogBootloader("-- current fingerprint: '");
    bl_hexdump(fp, FINGERPRINT_SIZE);
    LogBootloaderRaw("'\n");

    // wait for SD card to power up
    __delay_ms(200);

    int res = FSInit();
    LogBootloader("-- FSinit -> %d\n", res);
    if ( res ) {
        FSFILE *file = FSfopen("FW.HDR","r");
        LogBootloader("-- file = %p\n", file);
        if ( file != NULL ) {
            // install image
            LogBootloader("-- fingerprint file found\n");
            int num = FSfread(&fwHeader, 1, sizeof(FirmwareHeader), file);
            FSfclose(file);
            if ( num != sizeof(FirmwareHeader) ) {
                LogBootloader("-- error reading FirmwareHeader num: %d\n", num);
                return;
            }

            LogBootloader("FW Header   rel: %d  dev: %d  num: %d  crc: 0x%04x  fp: '",
                          fwHeader.rel_version,
                          fwHeader.dev_version,
                          fwHeader.num_blocks,
                          fwHeader.crc);
            bl_hexdump(fwHeader.fingerprint.data, FINGERPRINT_SIZE);
            LogBootloaderRaw("'\n");

            if ( fwHeader.rel_version == 0 ) {
                LogBootloader("-- error rel_version == 0   rel: %d  dev: %d\n:", fwHeader.rel_version, fwHeader.dev_version);
                return;
            }
	
/*
            LogBootloader("FW Header   rel: %d  dev: %d  num: %d  crc: 0x%04x  fp: '",
                          fwHeader.rel_version,
                          fwHeader.dev_version,
                          fwHeader.num_blocks,
                          fwHeader.crc);
            bl_hexdump(fwHeader.fingerprint.data, FINGERPRINT_SIZE);
            LogBootloaderRaw("'\n");
*/
            
            if ( memcmp(fp, fwHeader.fingerprint.data, FINGERPRINT_SIZE) != 0 ) {
                LogBootloader("-- fingerprint mismatch\n");
	
                // update from image file
                int err = FirmwareUpdateImage();
                if ( !err ) {
                    LogBootloader("-- UPDATE fingerprint: ");
                    bl_hexdump(fwHeader.fingerprint.data, FINGERPRINT_SIZE);
                    LogBootloaderRaw("'\n");

                    LogBootloader("-- FP addr: %lx\n", BOOTLOADER_FINGERPRINT_ADDRESS );
                    bool res = WriteFingerprint(fwHeader.fingerprint.data);

                    BYTE fp2[FINGERPRINT_SIZE];
                    ReadFingerprintToBuffer(fp2);
                    LogBootloader("-- new fingerprint: '");
                    bl_hexdump(fp2, FINGERPRINT_SIZE);
                    LogBootloaderRaw("'\n");
                }
            }

        } else {
            LogBootloader("-- fingerprint file not found\n");
        }
    }

{
    LogBootloaderRaw("[bootloader.c:%d] Config Page\n", __LINE__);

    int row, i;
    DWORD addr = BOOTLOADER_CONFIG_PAGE;
    for ( row = 0; row < 5; row++ ) {
        LogBootloaderRaw("[bootloader.c:%d]   row[%d]  addr: 0x%lx", __LINE__, row, addr);
        
        for (i = 0; i < DEVICE_UUID_SIZE / 2; ++i) {
            DWORD_VAL dw = {FlashReadDWORD(addr)};
//            *buffer++ = dw.byte.LB;
//            *buffer++ = dw.byte.HB;
            LogBootloaderRaw(" %02X %02X", dw.byte.LB, dw.byte.HB);
            
            addr += 2;
        }
        LogBootloaderRaw("\n");
    }
    LogBootloaderRaw("[bootloader.c:%d]\n", __LINE__);
}
//    DumpConfigPage2();
    LogBootloader("\n");
}

int main() {
  log_init();
  UART1Init();
  LogBootloader("BOOTLOADER   3   ***  RCON: %04x\n", RCON);

  // If bootloader mode not requested, go immediately to app.
  if (!ShouldEnterBootloader()) {
    OscCalibrateCached();
    // check for firmware update
    MaybeUpdateFirmware();

    char str[9];
    _prog_addressT p;
    _init_prog_address(p, hardware_version);
    _memcpy_p2d16(str, p, 8);
    str[8] = 0;
    LogBootloader("  HW: '%s'\n", str);    

    _init_prog_address(p, bootloader_version);
    _memcpy_p2d16(str, p, 8);
    str[8] = 0;
    LogBootloader("  BL: '%s'\n", str);    

/*
    memcpy(str, FW_IMPL_VER, 8);
    str[8] = 0;
    LogBootloader("  FW: '%s'\n", str);        
*/    
    LogBootloader("\n");
    LogBootloader("Running app\n");

    log_printf("Running app...");
    __asm__("goto __APP_RESET");
  }

  // We need to enter bootloader mode, wait for the boot pin to be released.
  while (!led_read());

  // Now we can start!
  led_init();
#ifdef SIGNAL_AFTER_BAD_RESET
  if (RCON & 0b1100001001000000) {
    SignalRcon();
  }
#endif

  log_printf("Hello from Bootloader!!!");
  if (IsPin1Grounded()) {
    log_printf("Erasing config.");
    EraseConfig();
  }
  OscCalibrateCached();
  Blink(5);
  USBInitialize();

  while (1) {
    // Wait for connection
    while (!(USBGetDeviceState() == CONFIGURED_STATE
      && CDCIsDtePresent())) USBTasks();

    log_printf("Connected!");
    BootProtocolInit();

    while (USBGetDeviceState() == CONFIGURED_STATE && CDCIsDtePresent()) {
      static char in_buf[64];
      USBTasks();

      BYTE size = getsUSBUSART(in_buf, sizeof(in_buf));
      if (!BootProtocolProcess(in_buf, size)) {
        log_printf("Protocol error. Will detach / re-attach.");
        USBSoftDetach();
        __delay_ms(2000);
        USBDeviceAttach();
        break;
      }
      BootProtocolTasks();
    }
    log_printf("Disconnected!");
  }
  return 0;
}
