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

