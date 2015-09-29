
#ifndef __BUTTON_H__
#define __BUTTON_H__


#define ENABLE_BUTTON


#ifndef ENABLE_BUTTON

#define ButtonInit()
#define ButtonPressed()

#else // ENABLE_BUTTON

#include <stdint.h>

// Initialize this module.
void ButtonInit();
uint8_t ButtonPressed();


#endif // ENABLE_BUTTON


#endif // __BUTTON_H__
