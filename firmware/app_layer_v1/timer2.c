
#include "timer2.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <libpic30.h>

#include "Compiler.h"
#include "logging.h"
#include "protocol.h"
#include "pins.h"
#include "sync.h"

#include "log.h"

typedef struct {
  const char *name;
  int period_ms;
  Timer2Callback callback;
  int period_ticks;
  int count_ticks;
  int enabled;
} TIMER2_STATE;

#define NUM_TIMER2_TIMERS  4

static TIMER2_STATE timer2_timers[NUM_TIMER2_TIMERS];

int Timer2FindTimer(const char *name) {
  for ( int i = 0; i < NUM_TIMER2_TIMERS; i++ ) {
    if ( strcmp( timer2_timers[i].name, name ) == 0 )
      return i;
  }
  return -1;
}

int Timer2GetFreeTimer() {
  for ( int i = 0; i < NUM_TIMER2_TIMERS; i++ ) {
    if ( timer2_timers[i].name == NULL )
      return i;
  }
  return -1;
}

void Timer2Tasks() {
  for ( int i = 0; i < NUM_TIMER2_TIMERS; i++ ) {
    if ( timer2_timers[i].enabled ) {
      if ( timer2_timers[i].count_ticks == timer2_timers[i].period_ticks ) {
        // call callback
        (*timer2_timers[i].callback)();
        // reset counter
        timer2_timers[i].count_ticks = 0;
      } else {
        // inc counter
        timer2_timers[i].count_ticks++;
      }
    }
  }
}


//
// Timer 2 interrupt
//
void __attribute__((__interrupt__, auto_psv)) _T2Interrupt() {
  // clear interrupt
  _T2IF = 0;

  // Service the timers
  Timer2Tasks();
}



//--------------------------------------------------------------------------------
//
// Public Interface
//
void Timer2Init() {
  // timer 2 runs at 62.5 kHz  - set timer 2 period for 250 hz timer rate
  PR2 = 249;
  // Enable interrupt
  _T2IE = 1;
}

int Timer2Register(const char *name, int period_ms, Timer2Callback callback) {
  int timer_index = Timer2GetFreeTimer();
  if ( timer_index == -1 )
    return -1;

  int period_ticks = ( period_ms / 4 );

  LogMain("timer2 register %s  ms: %d  idx: %d  ticks: %d  callback: 0x%04x", name, period_ms, timer_index, period_ticks, (unsigned int)callback);

  timer2_timers[timer_index].name = strdup(name);
  timer2_timers[timer_index].period_ms = period_ms;
  timer2_timers[timer_index].callback = callback;
  timer2_timers[timer_index].period_ticks = period_ticks;
  timer2_timers[timer_index].count_ticks = 0;
  timer2_timers[timer_index].enabled = 0;

  return timer_index;
}

int Timer2Start(const char *name) {
  int timer_index = Timer2FindTimer(name);
  if ( timer_index == -1 )
    return -1;

  timer2_timers[timer_index].enabled = 1;
  return timer_index;
}

int Timer2Stop(const char *name) {
  int timer_index = Timer2FindTimer(name);
  if ( timer_index == -1 )
    return -1;

  timer2_timers[timer_index].enabled = 0;
  return timer_index;
}

