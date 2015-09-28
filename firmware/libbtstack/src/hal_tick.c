
#include <stdlib.h>

#include "btstack-config.h"

#include "btstack/hal_tick.h"

#include "bt_log.h"


static void dummy_handler(void){};

static void (*tick_handler)(void) = &dummy_handler;

// 1/10 of second interrupt rate 100 milliseconds

int hal_tick_get_tick_period_in_ms(void) {
  // 100 ms
  return 100;
}


// pic24f SFRs
#include "Compiler.h"

void hal_tick_init(void) {
  // timer 2 runs at 62.5 kHz  - set timer 2 period for 10 hz timer rate
  PR2 = 6249;
  // Enable interrupt
  _T2IE = 1;
}


void hal_tick_set_handler(void (*handler)(void)) {
    if (handler == NULL){
        tick_handler = &dummy_handler;
        return;
    }
    tick_handler = handler;
}

void hal_tick_call_handler(void) {
  LogHALTick();

  // call handler
  (*tick_handler)();
}


//
// Timer 2 interrupt
//
void __attribute__((__interrupt__, auto_psv)) _T2Interrupt() {
  _T2IF = 0;  // clear interrupt

  hal_tick_call_handler();
}


