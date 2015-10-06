
#include "button.h"

#ifdef ENABLE_BUTTON


#include "Compiler.h"
#include <stdint.h>

#include "timer2.h"
#include "log.h"


static uint16_t button_on_time;
static uint16_t button_off_time;

static uint16_t button_state;

int ButtonPressed() {
  return (int)button_state;
}

//void PixelPlayNext();

static uint16_t input_state;

void ButtonService() {
  // read port D pin 9 state
  uint16_t input = ( ( PORTD & 0x0200 ) >> 9 );
  // shift the input state and add the next input value
  input_state = ( ( input_state << 1 ) | input );

  uint16_t last_button_state = button_state;

  // look at the last 8 inputs
  if ( ( input_state & 0x00ff ) == 0x00ff ) {
    button_state = 1;
  } else {
    button_state = 0;
  }

  if ( !last_button_state && button_state ) {
    button_on_time = 0;
    LogMain("Button Pressed  on: %d  off: %d", button_on_time, button_off_time);
  }

  if ( last_button_state && !button_state ) {
    button_off_time = 0;
    LogMain("Button Released  on: %d  off: %d", button_on_time, button_off_time);

    //    // If the press is a quick press, less than 30 microseconds then play the next animation
    //    if ( button_on_time < 30 ) {
    //      PixelPlayNext();
    //    }
  }

  // track how long the button has been on
  if ( button_state )
    button_on_time++;
  else
    button_off_time++;
}

    
void ButtonInit() {
  // Initialize the I/O port
  TRISD |= 0x0200;  // set RD9 as an input

  button_state = 0;
  button_on_time = 0;
  button_off_time = 0;

  input_state = 0x0000;

  // Register the button scanner with timer 2 at 250 hz
  Timer2Register("button", 4, ButtonService);
  Timer2Start("button");
}

#endif // ENABLE_BUTTON
