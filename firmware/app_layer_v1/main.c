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

#include "Compiler.h"
#include "libconn/connection.h"
#include "features.h"
#include "protocol.h"
#include "logging.h"
#include "pixel.h"

#include "uart.h"

#include "log.h"

#include <stdint.h>


//
// Trap handlers
//
#include "Compiler.h"
#include "p24fxxxx.h"


void* getTopOfStack()
{
  volatile int markerValue;
  void* p = (void*)&markerValue;
  return p;
}

// 
// W15 is the stack pointer
// W14 is the frame pointer
//
// ;1. PC[15:0]            <--- Trap Address
// ;2. SR[7:0]:IPL3:PC[22:16]
// ;3. RCOUNT
// ;4. W0
// ;5. W1
// ;6. W2
// ;7. W3
// ;8. W4
// ;9. W5
// ;10. W6
// ;11. W7
// ;12. OLD FRAME POINTER [W14]	<---- Save the upper return address
// ;13. PC[15:0]           <---- W14
// ;14. 0:PC[22:16]
// ;15.                    <---- W15
// 
/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This routine is not available for [CASE e] 
;; because push instruction will confuse Frame Pointer
;; and Program Counter. 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_getErrLoc:
        mov    w14,w2
        sub    w2,#24,w2
        mov    [w2++],w0
        mov    [w2++],w1 
        mov    #0x7f,w3     ; Mask off non-address bits
        and    w1,w3,w1
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
	;; The instructions above will return to the next 
	;; instruction of the error location. 
	;; If you want to return to the error location, 
	;; you should decomment the following instructions.
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;  
    ;	mov    #2,w2        ; Decrement the address by 2
    ;	sub    w0,w2,w0
    ;	clr    w2
    ;	subb   w1,w2,w1
        return
*/
// 
// 8.2.2.3 ADDRESS ERROR TRAP (HARD TRAP, LEVEL 13)
// 
// The following paragraphs describe operating scenarios that would cause
// an address error trap to be generated:
// 
// 1. A misaligned data word fetch is attempted. This condition occurs
// when an instruction performs a word access with the LSb of the
// effective address set to ‘1’. The PIC24F CPU requires all word
// accesses to be aligned to an even address boundary.
// 
// 2. A bit manipulation instruction using the Indirect Addressing mode
// with the LSb of the effective address set to ‘1’.
// 
// 3. A data fetch from unimplemented data address space is attempted.
// 
// 4. Execution of a “BRA #literal” instruction or a “GOTO #literal”
// instruction, where literal is an unimplemented program memory address.
// 
// 5. Executing instructions after modifying the PC to point to
// unimplemented program memory addresses. The PC may be modified by
// loading a value into the stack and executing a RETURN instruction.
// 
// Data space writes will be inhibited whenever an address error trap
// occurs, so that data is not destroyed.
// 
// An address error can be detected in software by polling the ADDRERR
// status bit (INTCON1<3>). To avoid re-entering the Trap Service
// Routine, the ADDRERR status flag must be cleared in software prior to
// returning from the trap with a RETFIE instruction.
// 
void __attribute__((interrupt, no_auto_psv)) _AddressError(void)
{
  for ( int i = 0; i < 2; i++ ) {
    LATD |= 0x0800;  for ( int j = 0; j < 31; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");

  while (1) {
    LATD |= 0x0800;  for ( int j = 0; j < 7; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 7; j++ ) asm("nop\n");
  }

  // Clear the trap flag
  INTCON1bits.ADDRERR = 0;
}

//
// 8.2.2.2 OSCILLATOR FAILURE TRAP (HARD TRAP, LEVEL 14)
// 
// An oscillator failure trap event will be generated if the Fail-Safe
// Clock Monitor (FSCM) is enabled and has detected a loss of the system
// clock source.
// 
// An oscillator failure trap event can be detected in software by
// polling the OSCFAIL status bit (INTCON1<1>) or the CF status bit
// (OSCCON<3>). To avoid re-entering the Trap Service Routine, the
// OSCFAIL status flag must be cleared in software prior to returning
// from the trap with a RETFIE instruction.
// 
// Refer to Section 6. “Oscillator” and Section 32. “Device
// Configuration” for more information about the FSCM.
// 
void __attribute__((interrupt, no_auto_psv)) _OscillatorFail(void)
{
  for ( int i = 0; i < 4; i++ ) {
    LATD |= 0x0800;  for ( int j = 0; j < 31; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");

  while (1) {
    LATD |= 0x0800;  for ( int j = 0; j < 7; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 7; j++ ) asm("nop\n");
  }

  INTCON1bits.OSCFAIL = 0;        //Clear the trap flag
}

// 
// 8.2.1.1 STACK ERROR TRAP (SOFT TRAP, LEVEL 12)
// 
// The stack is initialized to 0x0800 during Reset. A stack error trap
// will be generated should the Stack Pointer address ever be less than
// 0x0800.
// 
// There is a Stack Limit register (SPLIM) associated with the Stack
// Pointer that is uninitialized at Reset. The stack overflow check is
// not enabled until a word write to SPLIM occurs.
// 
// All Effective Addresses (EA) generated using W15 as a source or
// destination pointer are compared against the value in SPLIM. Should
// the EA be greater than the contents of the SPLIM register, then a
// stack error trap is generated. In addition, a stack error trap will be
// generated should the EA calculation wrap over the end of data space
// (0xFFFF).
// 
// A stack error can be detected in software by polling the STKERR status
// bit (INTCON1<2>). To avoid re-entering the Trap Service Routine, the
// STKERR status flag must be cleared in software prior to returning from
// the trap with a RETFIE instruction.
// 
void __attribute__((interrupt, no_auto_psv)) _StackError(void)
{
  // WREG15;
  //  main_sp = (uint16_t)getTopOfStack();
  //  main_splim = SPLIM;

  for ( int i = 0; i < 6; i++ ) {
    LATD |= 0x0800;  for ( int j = 0; j < 31; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");


  //  printf("sp: %04x\n", main_sp);
  //  printf("splim: %04x\n", main_splim);

  /*
  LATD |= 0x0800;  for ( int j = 0; j < 63; j++ ) asm("nop\n");

  tmpw = main_sp;
  for ( int i = 0; i < 8; i++ ) {
    if ( ( tmpw & 0x8000 ) == 0x8000 ) {
      LATD |= 0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    } else {
      LATD &= ~0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    }
    tmpw = ( tmpw << 1 );
  }
  LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  LATD |= 0x0800;  for ( int j = 0; j < 63; j++ ) asm("nop\n");


  for ( int i = 0; i < 8; i++ ) {
    if ( ( tmpw & 0x8000 ) == 0x8000 ) {
      LATD |= 0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    } else {
      LATD &= ~0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    }
    tmpw = ( tmpw << 1 );
  }
  LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  LATD |= 0x0800;  for ( int j = 0; j < 127; j++ ) asm("nop\n");
  */

  /*
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 5; j++ ) asm("nop\n");

  tmpw = main_sp;
  for ( int i = 0; i < 16; i++ ) {
    if ( ( tmpw & 0x8000 ) == 0x8000 ) {
      LATD |= 0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    } else {
      LATD |= 0x0800; 
      for ( int j = 0; j < 5; j++ ) asm("nop\n");
      LATD &= ~0x0800; 
      for ( int j = 0; j < 28; j++ ) asm("nop\n");
    }
    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
    tmpw = ( tmpw << 1 );
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");


  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 5; j++ ) asm("nop\n");

  tmpw = main_splim;
  for ( int i = 0; i < 16; i++ ) {
    if ( ( tmpw & 0x8000 ) == 0x8000 ) {
      LATD |= 0x0800; 
      for ( int j = 0; j < 31; j++ ) asm("nop\n");
    } else {
      LATD |= 0x0800; 
      for ( int j = 0; j < 5; j++ ) asm("nop\n");
      LATD &= ~0x0800; 
      for ( int j = 0; j < 28; j++ ) asm("nop\n");
    }
    for ( int j = 0; j < 31; j++ ) asm("nop\n");

    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
    tmpw = ( tmpw << 1 );
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");
  */

  while (1) {
    LATD |= 0x0800;  for ( int j = 0; j < 7; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 7; j++ ) asm("nop\n");
  }

  INTCON1bits.STKERR = 0; //Clear the trap flag
}

// 
// 8.2.1.2 MATH ERROR TRAP (LEVEL 11)
// 
// The Math Error trap will execute should an attempt be made to divide
// by zero. The math error trap can be detected in software by polling
// the MATHERR status bit (INTCON1<4>). To avoid re-entering the Trap
// Service Routine, the MATHERR status flag must be cleared in software
// prior to returning from the trap with a RETFIE instruction.
// 
void __attribute__((interrupt, no_auto_psv)) _MathError(void)
{
  for ( int i = 0; i < 8; i++ ) {
    LATD |= 0x0800;  for ( int j = 0; j < 31; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 31; j++ ) asm("nop\n");
  }
  LATD |= 0x0800;  for ( int j = 0; j < 3; j++ ) asm("nop\n");
  LATD &= ~0x0800; for ( int j = 0; j < 63; j++ ) asm("nop\n");

  while (1) {
    LATD |= 0x0800;  for ( int j = 0; j < 7; j++ ) asm("nop\n");
    LATD &= ~0x0800; for ( int j = 0; j < 7; j++ ) asm("nop\n");
  }

  INTCON1bits.MATHERR = 0;        //Clear the trap flag
}




//--------------------------------------------------------------------------------
//
// Referenced from   firmware/microchip/usb/usb_host.c
//

// define in non-const arrays to ensure data space
static char descManufacturer[] = "IOIO Open Source Project";
static char descModel[] = "IOIO";
static char descDesc[] = "IOIO Standard Application";
static char descVersion[] = FW_IMPL_VER;
static char descUri[] = "https://github.com/ytai/ioio/wiki/ADK";
static char descSerial[] = "N/A";

const char* accessoryDescs[6] = {
  descManufacturer,
  descModel,
  descDesc,
  descVersion,
  descUri,
  descSerial
};


//--------------------------------------------------------------------------------
//
// main.c  state
//

typedef enum {
  STATE_INIT,
  STATE_OPEN_CHANNEL,
  STATE_WAIT_CHANNEL_OPEN,
  STATE_CONNECTED,
  STATE_ERROR
} STATE;

const char *mainStateName(STATE state) {
  switch ( state ) {
  case STATE_INIT:
    return "STATE_INIT";
  case STATE_OPEN_CHANNEL:
    return "STATE_OPEN_CHANNEL";
  case STATE_WAIT_CHANNEL_OPEN:
    return "STATE_WAIT_CHANNEL_OPEN";
  case STATE_CONNECTED:
    return "STATE_CONNECTED";
  case STATE_ERROR:
    return "STATE_ERROR";
  }
  return "STATE-unknown";
}


// main Channel connection state
static STATE state = STATE_INIT;

STATE mainSetState(STATE new_state) {
  if ( state != new_state ) {
    LogMain("state change  %s -> %s", mainStateName(state), mainStateName(new_state));
  }
  state = new_state;
  return state;
}

// main Channel handle state
static CHANNEL_HANDLE handle;

CHANNEL_HANDLE mainSetHandle(CHANNEL_HANDLE new_handle) {
  if ( handle != new_handle ) {
    LogMain("handle change  0x%04x -> 0x%04x", handle, new_handle);
  }
  handle = new_handle;
  return handle;
}


//--------------------------------------------------------------------------------
//
// AppCallback - called by usb_host
//

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg);

static inline CHANNEL_HANDLE OpenAvailableChannel() {
  int_or_ptr_t arg = { .i = 0 };
  if (ConnectionTypeSupported(CHANNEL_TYPE_ADB)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ADB)) {
      return ConnectionOpenChannelAdb("tcp:4545", &AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_ACC)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ACC)) {
      return ConnectionOpenChannelAccessory(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_BT)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_BT)) {
      return ConnectionOpenChannelBtServer(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_CDC_DEVICE)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_CDC_DEVICE)) {
      return ConnectionOpenChannelCdc(&AppCallback, arg);
    }
  }
  return INVALID_CHANNEL_HANDLE;
}

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg) {
  if (data) {
    if (!AppProtocolHandleIncoming(data, data_len)) {
      // got corrupt input. need to close the connection and soft reset.
      log_printf("Protocol error");
      mainSetState(STATE_ERROR);
    }
  } else {
    // data == NULL

    // connection closed, soft reset and re-establish
    if (state == STATE_CONNECTED) {
      log_printf("Channel closed");
      SoftReset();
    } else {
      log_printf("Channel failed to open");
    }
    mainSetState(STATE_OPEN_CHANNEL);
  }
}


// reset RCON value
static uint16_t reset_rcon;

static uint16_t reset_sp;
static uint16_t reset_splim;

int main() {
  reset_sp = (uint16_t)getTopOfStack();
  reset_splim = SPLIM;
  reset_rcon = RCON;

  p6init();
  UART1Init();

  LogMain();
  LogMain("start  RCON: 0x%04x", reset_rcon);
  LogMain("     reset_SP: 0x%04x", reset_sp);
  LogMain("  reset_SPLIM: 0x%04x", reset_splim);
  LogMain("           SP: 0x%04x", (unsigned int)getTopOfStack());


  // IOIO logging init
  log_init();
  log_printf("***** Hello from app-layer! *******");


  SoftReset();    // PixelInit gets called by this

  // Init UART and Log
  //  LogUARTInit();
  //  LogInit();


  LogMain("main()  state = %s", mainStateName(state) );

  LogMain("connection init");
  LogMain("SP: 0x%04x", (unsigned int)getTopOfStack());

  ConnectionInit();
  while (1) {
    //    LogMain("  loop TOP state = %s", mainStateName(state) );

    PixelTasks();
    ConnectionTasks();

    switch (state) {
      case STATE_INIT:
        mainSetHandle(INVALID_CHANNEL_HANDLE);
        LogMain("INIT  handle %04x", handle);
        mainSetState(STATE_OPEN_CHANNEL);
        break;

      case STATE_OPEN_CHANNEL:
        if (mainSetHandle(OpenAvailableChannel()) != INVALID_CHANNEL_HANDLE) {
          LogMain("OPEN_CHANNEL  handle %04x", handle);
          log_printf("Connected");
          mainSetState(STATE_WAIT_CHANNEL_OPEN);
        }
        break;

      case STATE_WAIT_CHANNEL_OPEN:
        LogMain("handle = 0x%04x", handle);

       if (ConnectionCanSend(handle)) {
         LogMain("CHANNEL OPEN  handle %04x   AppProtocolInit", handle);
          log_printf("Channel open");
          AppProtocolInit(handle);
          mainSetState(STATE_CONNECTED);
        }
        break;

      case STATE_CONNECTED:
        // LogMain("AppProtocolTasks  0x%04x", handle);
        AppProtocolTasks(handle);
        break;

      case STATE_ERROR:
        LogMain("ERROR  connection close  handle %04x", handle);
        ConnectionCloseChannel(handle);
        SoftReset();
        mainSetState(STATE_INIT);
        break;
    }
  }
  return 0;
}
