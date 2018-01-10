
#include "build_number.h"

#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "HardwareProfile.h"

//#include "sensors/uart.h"
#include "field_accessors.h"

#include "log.h"

#include "pixel.h"
//#include "timer1.h"
//#include "file.h"

// J3
// RB3  pin33  yellow
// RB2  pin34  white


//#include "sdcard/FSIO.h"
#include "sdcard/SD-SPI.h"

void hci_set_embedded_dump(int value);


// black - TRIGGER
void P34init(void) {
  // PIN34  AN2 / C2INB / VMIO / RP13 / CN4 / RB2
  ODCB &= ~0x0004;
  TRISB &= ~0x0004;
}
void P34(int state) {
  if (state)
    LATB |= 0x0004;
  else
    LATB &= ~0x0004;
}


void T2Setup()
{
    // T2  - PIN6  RP12 / RD11
    ODCD &= ~0x0800;
    TRISD &= ~0x0800;
}


//--------------------------------------------------------------------------------
//
// STDIO UART 1 Logging
//

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

  /*
  // UART 1 on pins 1 (rp10)  and 2 (rp17)
  RPINR18 = 0x3f0a;   // U1RX on RP10
  RPOR8 = ( 0x0300 | ( RPOR8 & 0x00ff ) );    // U1TX on RP17    U1TX output function 3

  U1BRG = 34;
  U1MODE = 0;
  U1MODEbits.BRGH = BRGH2;
  U1STA = 0;
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;
  IFS0bits.U1RXIF = 0;
  */
}



void UART1Tasks(void)
{
  /*
  DWORD start;
  BYTE res;
  DWORD end;
  MEDIA_INFORMATION *mediaInformation;
  */

  char c = '.';
  if ( U1STAbits.URXDA ) {
    // read character and transmit
    c = U1RXREG;
    //    printf("=> %c\n", c);

    switch ( c ) {

      /*
    case 's':
      printf("StopPlayFile\n");
      PixelStopFile();
      break;

    case 'p':
      printf("StartPlayFile\n");
      PixelPlayFile();
      break;

    case 't':
      printf("SysTicks: %lu\n", Timer1Ticks() );
      break;

    case 'd':
      start = Timer1Ticks();
      res = MDD_SDSPI_MediaDetect();
      end = Timer1Ticks();
      printf("MediaPresent %d    dt: %lu\n", res, ( end - start ) );
      break;

    case 'r':
      start = Timer1Ticks();
      mediaInformation = MDD_SDSPI_MediaInitialize();
      if ( mediaInformation->errorCode != MEDIA_NO_ERROR ) {
	end = Timer1Ticks();
	printf("SectorRead ERROR  code: %d   dt: %lu  ", mediaInformation->errorCode, ( end - start ) );
      } else {
	res = MDD_SDSPI_SectorRead( 0L, sector_buffer );
	end = Timer1Ticks();
	printf("SectorRead %d    dt: %lu\n", res, ( end - start ) );
	for ( int row = 0; row < 16; row++ ) {
	  printf("  [%03d]:", row);
	  for ( int col = 0; col < 32; col++ )
	    printf(" %02x", sector_buffer[(row*32)+col] );
	  printf("\n");
	}
      }
      break;
      */

      /*
    case 's':
      printf("FSInit -> %d\n", file_init());
      break;

    case 'a':
      printf("open1 -> %p\n", file_open1());
      break;
    case 'b':
      printf("open2 -> %p\n", file_open2());
      break;
    case 'c':
      printf("open3 -> %p\n", file_open3());
      break;

    case 'd':
      printf("write1 -> %d\n", file_write1());
      break;
    case 'e':
      file_read1();
      break;

    case 'f':
      printf("write2 -> %d\n", file_write2());
      break;
    case 'g':
      file_read2();
      break;

    case 'h':
      printf("write3 -> %d\n", file_write3());
      break;
    case 'i':
      file_read3();
      break;

    case 'j':
      printf("close1 -> %d\n", file_close1());
      break;
    case 'k':
      printf("close2 -> %d\n", file_close2());
      break;
    case 'l':
      printf("close3 -> %d\n", file_close3());
      break;
      */

    default:
      printf("=> %c\n", c);
      break;
    }
  }
}


/*
void __attribute__((__interrupt__, auto_psv)) _U1RXInterrupt()
{

  int uart_num = 0;

  volatile UART* reg = uart_reg[uart_num];
  BYTE_QUEUE* q = &uarts[uart_num].rx_queue;
  while (reg->uxsta & 0x0001) {
    BYTE b = reg->uxrxreg;
    if ((reg->uxsta & 0x000C) == 0) {
      // there is no frame/parity err
      ByteQueuePushByte(q, b);
    } // Otherwise, discard
    // It is OK to clear the interrupt now, since we're just about to poll for
    // any remaining characters in the FIFO, so we'll never miss an interrupt.
    AssignUxRXIF(uart_num, 0);
  }


  //AssignUxRXIF(uart_num, 0);
  IFS0bits.U1RXIF = 0;
}
*/

