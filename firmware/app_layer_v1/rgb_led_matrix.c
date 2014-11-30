/*
 * Copyright 2012 Ytai Ben-Tsvi. All rights reserved.
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

#include "rgb_led_matrix.h"

#include <string.h>
#include <p24Fxxxx.h>

// Connections:
//
// (r1 g1 b1 r2 g2 b2) (24 .. 19)
// clk                 25
// (a b c)             (7 10 11)
// lat                 27
// oe                  28

// Ordering:
//
// Pixels sent from left to right.
// MSB is upper half.

#define DATA_CLK_PORT LATE
#define CLK_PIN _LATE6
#define ADDR_PORT LATD
#define LAT_PIN _LATG6
#define OE_PIN _LATG7

#define SUB_FRAMES_PER_FRAME 3
#define ROWS_PER_SUB_FRAME 8

typedef uint8_t bin_row_t[256];
typedef bin_row_t bin_frame_t[ROWS_PER_SUB_FRAME];
typedef bin_frame_t frame_t[SUB_FRAMES_PER_FRAME];

static frame_t frames[2] __attribute__((eds, page));
static uint8_t const *data;
static int sub_frame;
static int displayed_frame;
static int back_frame_ready;
static int shifter_repeat;
static uint8_t address;

//static const frame_t DEFAULT_FRAME = {
//#include "default_frame.inl"
//};

void RgbLedMatrixEnable(int shifter_len_32) {
  _T4IE = 0;

  if (shifter_len_32) {
    LATE = 0;
    address = 0;
    LATG = (1 << 7);

    ODCE &= ~0x7F;
    ODCD &= ~0x7;
    ODCG &= ~0xC0;

    TRISE &= ~0x7F;
    TRISD &= ~0x7;
    TRISG &= ~0xC0;

    data = (const uint8_t *) (__builtin_edsoffset(frames));
    DSWPAG = __builtin_edspage(frames);
    memset(data, 0, sizeof(frame_t));
    sub_frame = 0;
    displayed_frame = 0;
    back_frame_ready = 0;
    shifter_repeat = shifter_len_32;

    // timer 4 is sysclk / 64 = 250KHz
    PR4 = 1;
    TMR4 = 0x0000;
    _T4IP = 7;
    _T4IE = 1;
  } else {
    TRISE |= 0x7F;
    TRISD |= 0x7;
    TRISG |= 0xC0;
  }
}

void RgbLedMatrixFrame(const uint8_t frame[]) {
  back_frame_ready = 0;
  DSWPAG = __builtin_edspage(frames);
  void * target = (void *) (__builtin_edsoffset(frames)
    + (displayed_frame ^ 1) * sizeof(frame_t));
  memcpy(target, frame, 768 * shifter_repeat);
  back_frame_ready = 1;
}

void RgbLedMatrixSwapFrame() {
  back_frame_ready = 1;
}

void RgbLedMatrixGetBackBuffer(void ** buffer, unsigned int * page) {
  if (back_frame_ready) {
    *buffer = NULL;
    return;
  }
  *page = __builtin_edspage(frames);
  *buffer = (void *) (__builtin_edsoffset(frames)
    + (displayed_frame ^ 1) * sizeof(frame_t));
}

static void draw_row() {
  int i;

  if (sub_frame == SUB_FRAMES_PER_FRAME) {
    OE_PIN = 1; // black
    if (++address == ROWS_PER_SUB_FRAME) {
      // sub-frame done
      address = 0;
      // frame done
      sub_frame = 0;

      if (back_frame_ready) {
        displayed_frame = displayed_frame ^ 1;
        back_frame_ready = 0;
      }
      data = (const uint8_t *) (__builtin_edsoffset(frames)
          + displayed_frame * sizeof(frame_t));
    }
    return;
  }

  DSRPAG = __builtin_edspage(frames);
  for (i = shifter_repeat; i > 0; --i) {
    // push 32 bytes
    #define DUMP1 "mov.b [%0++], [%1]\nbset [%1], #6\n"  //the assembly language calls Ytai added
    #define DUMP4 DUMP1 DUMP1 DUMP1 DUMP1
    #define DUMP16 DUMP4 DUMP4 DUMP4 DUMP4

    asm(DUMP16 DUMP16 : "+r"(data) : "r"(&DATA_CLK_PORT));
  }

  OE_PIN = 1; // black
  // latch
  LAT_PIN = 1;
  LAT_PIN = 0;
  *((uint8_t *) &ADDR_PORT) = address;

  OE_PIN = 0; // enable output
  
  // Advance the row address, sub-frame if needed, frame if needed.

  if (++address == ROWS_PER_SUB_FRAME) {
    // sub-frame done
    address = 0;
    ++sub_frame;
  }
}

int RgbLedMatrixFrameSize() {
  return shifter_repeat * 32 * ROWS_PER_SUB_FRAME * SUB_FRAMES_PER_FRAME;
}

static unsigned int times[] = { //15, 30, 60, 150   133 to 122.5 refresh rate 37.5%, this is the normal one
  15, 30, 60, 150  //this has something to do with brightness levels  original 20, 40, 80, 100 , if you make the last number bigger, you'll have a longer black gap between every two frames so you'd both reduce the average brightness and the frame rate (frequency). 33% brightness
};                // for original pixel  8, 16, 32, 250 this one was not a good refresh rate/ bad
                  // 15,30,60,150 //150 is the black frame time, increase this number to make it
                  // for lower power, reduce the first three by a factor of x and multiple the fourth by x
                  // low power 4, 8, 15, 600
                  // lowest power 3, 6, 12, 750

void __attribute__((__interrupt__, auto_psv)) _T4Interrupt() {
  // Schedule the next interrupt.
  PR4 = times[sub_frame] - 1;
  draw_row();
  _T4IF = 0; // clear
}

