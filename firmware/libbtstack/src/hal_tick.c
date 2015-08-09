
#include <stdlib.h>

#include "btstack-config.h"

#include "btstack/hal_tick.h"

#include "bt_log.h"


static void dummy_handler(void){};

static void (*tick_handler)(void) = &dummy_handler;


static int tick_number = 0;

static int tick_toggle = 0;


// 1/4 of second interrupt rate  250 milliseconds

void hal_tick_init(void) {
  LedSetFlag(0, 10, GREEN);

  //    TA1CCTL0 = CCIE;                   // CCR0 interrupt enabled
  //    TA1CTL = TASSEL_1 | MC_2 | TACLR;  // use ACLK (32768), contmode, clear TAR
  //    TA1CCR0 = TIMER_COUNTDOWN;    // -> 1/4 s
}

void hal_tick_set_handler(void (*handler)(void)) {
  LedSetFlag(0, 11, GREEN);

    if (handler == NULL){
        tick_handler = &dummy_handler;
        return;
    }
    tick_handler = handler;
}

int hal_tick_get_tick_period_in_ms(void) {
  LedSetFlag(0, 12, GREEN);

  //    return 250;
    return 100;
}


const char *hciStackStateName();

void hal_tick_call_handler(void) {
  if ( tick_handler == &dummy_handler ) {
    LedSetFlag(0, 13, GREEN);
  }

  tick_number++;
  //  LogHAL("hal_tick  handler  %d   HCI stack state %s", tick_number, hciStackStateName() );
  //LogHAL("-------- hal_tick: %d - %s", tick_number, hciStackStateName() );
  LogHAL("-tick- % 4d %s", tick_number, hciStackStateName() );

  (*tick_handler)();

  tick_toggle = !tick_toggle;
  if ( tick_toggle )
    LedSetFlag(0, 14, BLUE);
  else
    LedSetFlag(0, 14, OFF);
}
