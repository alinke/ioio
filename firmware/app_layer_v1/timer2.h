
#ifndef __TIMER2_H__
#define __TIMER2_H__

//
// Timer2 is used to drive the HAL tick and button debounce.
//

typedef void (*Timer2Callback)();

void Timer2Init();

// Register a callback that will get called evey period_ms. 
//   @return the actual scheculed period.
int Timer2Register(const char *name, int period_ms, Timer2Callback callback);

int Timer2Start(const char *name);

int Timer2Stop(const char *name);


#endif // __TIMER2_H__
