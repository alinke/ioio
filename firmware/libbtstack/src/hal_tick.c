
#include <stdlib.h>

#include "btstack-config.h"

#include "btstack/hal_tick.h"

#include "timer2.h"
#include "bt_log.h"


static void dummy_handler(void){};

static void (*tick_handler)(void) = &dummy_handler;

// 1/10 of second interrupt rate 100 milliseconds

int hal_tick_get_tick_period_in_ms(void) {
  // 100 ms
  return 100;
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


// pic24f SFRs
#include "Compiler.h"

void hal_tick_init(void) {
  // get tick period
  int ms = hal_tick_get_tick_period_in_ms();

  // Register the HAL tick
  Timer2Register("hal-tick", ms, hal_tick_call_handler);
  Timer2Start("hal-tick");
}


