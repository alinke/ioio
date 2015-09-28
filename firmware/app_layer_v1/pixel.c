#include "Compiler.h"
#include "libconn/connection.h"
#include "features.h"
#include "protocol.h"
#include "logging.h"
#include "rgb_led_matrix.h"

#include "sdcard/FSIO.h"
#include "sdcard/SD-SPI.h"
#include "timer.h"
#include "pixel.h"
#include <stdio.h>

#define ANIMATION_FILENAME "artdata.bin"
#define METADATA_FILENAME "fps.bin"
#define SHIFTER_LENGTH_FILENAME "matrixtp.bin"


#include "log.h"

#include "uart.h"



//
// Pixel
//

typedef enum {
  STATE_NONE,
  STATE_PLAY_FILE,
  STATE_WRITE_FILE,
  STATE_INTERACTIVE
} STATE;

static STATE state = STATE_NONE;
static int frame_delay; // static int frame_delay;
static int shifter_len_32;
static int num_rows;

static FSFILE *animation_file;
static FSFILE *metadata_file;
static FSFILE *shifter_length_file;

// TODO: this can be optimized - we already have a buffer of this size for this
// purpose in RgbLedMatrix.
// BYTE frame[1536 * 4] __attribute__((far));


void PixelStartTimer(int period) {
  // Stop the timer
  T1CON = 0x0000;

  // Period is (PR1 + 1) / 62500 (seconds)
  PR1 = period;
  TMR1 = 0x0000;
  _T1IF = 0;

  // Enable interrupt
  _T1IE = 1;

  // Start the timer @ 62.5[kHz]
  T1CON = 0x8030;
}

void PixelStopTimer() {
  T1CON = 0x0000;
}



static unsigned timer_tick = 0;

void __attribute__((__interrupt__, auto_psv)) _T1Interrupt() {
  if ( state == STATE_PLAY_FILE )
    timer_tick = 1;
  else
    timer_tick = 0;

  _T1IF = 0;  // clear interrupt
}


unsigned PixelHasTimerInterrupt(void) {
  return timer_tick;
}
void PixelClearTimerInterrupt(void) {
  timer_tick = 0;
}



//--------------------------------------------------------------------------------
//
// PlayFile stuff
//
//

// Start playing an animation file.
//
// load the animation file metadata
// open the animation frame file
// Initialize the LED display.
// setup and start timer 1
//
static void StartPlayFile() {
  LogPixel("StartPlayFile");  

  if (!FSInit()) {
    // Failed to initialize FAT16 ? do something?
    return;
  }
  LogPixel("  FSInit Ok");

  // Read metadata file.
  //Open the shifter length file.
  BYTE shiftbuff[sizeof(int)];
  shifter_length_file = FSfopen(SHIFTER_LENGTH_FILENAME, "r");
  if (!shifter_length_file) return; //move on if the matrix type file is not there
  //if (!fsize(shifter_length_file) > 0) return; //move on if the file is 0 bytes
  FSfread(shiftbuff, sizeof( int ), 1, shifter_length_file);
  FSfclose(shifter_length_file);

  //Store the shifter length
  shifter_len_32 = shiftbuff[0] & 0x0F;
  int rows = (shiftbuff[0] >> 4) & 0x03;

  //let's make sure we got good data and abort if not
  if (shifter_len_32 != 1 && shifter_len_32 != 2 && shifter_len_32 != 4 && shifter_len_32 != 8 ) {
     return;
  }
  if (rows != 0 && rows != 1) {
    return;
  }
  num_rows = rows == 0 ? 8 : 16;

  LogPixel("  play  sl: %d  rows: %d", shifter_len_32, num_rows);

  //Open the metadata File
  BYTE buff[sizeof(int)];
  metadata_file = FSfopen(METADATA_FILENAME, "r");
  if (!metadata_file) return;
 // if (!fsize(metadata_file) > 0) return; //move on if the file is 0 bytes

  FSfread(buff, sizeof( int ), 1, metadata_file);  //changed to float FSfread(buff, sizeof( int ), 1, metadata_file);
  FSfclose(metadata_file);

  //Store the framerate into frame_delay
  frame_delay = buff[0] | (buff[1]  << 8);


  LogPixel("  open Animation file");

  // Open the animation file.
  animation_file = FSfopen(ANIMATION_FILENAME, "r");
  if (!animation_file) return;
 // if (!fsize(animation_file) > 0) return; //move on if the file is 0 bytes

  // check that the file is not empty
 // fseek(animation_file, 0, SEEK_END); // seek to end of file
 // int size = ftell(animation_file); // get current file pointer
 // if (size > 0) {  //file is not 0 bytles so we are good
  //  fseek(animation_file, 0, SEEK_SET); // seek back to beginning of file
 // }
 // else return; //empty file so return

  // Initialize the matrix.
  RgbLedMatrixEnable(shifter_len_32, rows);

  // Initialize the timer.
  PixelStartTimer(frame_delay);
  /*
  // Stop the timer
  T1CON = 0x0000;
  // Period is (PR1 + 1) / 62500 (seconds)
  PR1 = frame_delay;
  TMR1 = 0x0000;
  _T1IF = 0;
  // Start the timer @ 62.5[kHz]
  T1CON = 0x8030;
  */
  state = STATE_PLAY_FILE;
}


static void MaybeFrameFromFile() {
  // If our timer elapsed, push a frame to the display.
  if ( PixelHasTimerInterrupt() ) {
    void * frame;
    unsigned int dswpag;
    RgbLedMatrixGetBackBuffer(&frame, &dswpag);
    // There's already a frame in the back-buffer, we're not keeping up...
    if (!frame) return;

    PixelClearTimerInterrupt();

    DSWPAG = dswpag;
    
    //LogPixel("  read frame  sl: %d  rows: %d  size: %d", shifter_len_32, num_rows, RgbLedMatrixFrameSize());

    FSfread(frame, RgbLedMatrixFrameSize(), 1, animation_file);

    RgbLedMatrixSwapFrame();

    if (FSfeof(animation_file)) {
      // Rewind
      FSfseek(animation_file, 0, SEEK_SET);
    }
  }
}

static void StopPlayFile() {
  LogPixel("StopPlayFile");

  // Stop the timer.
  PixelStopTimer();
  // Close the file.
  FSfclose(animation_file);
  // Close the matrix.
  RgbLedMatrixEnable(0, 0);

  state = STATE_NONE;
}



//--------------------------------------------------------------------------------
//
// WriteFile functions
//

static FSFILE *write_animation_file;
static FSFILE *write_metadata_file;
static FSFILE *write_shifter_length_file;


// Setup the file to write frame data to.
static void StartWriteFile(int fd, int sl32, int rows) {  //was int fd
  LogPixel("StartWriteFile  fd: %d  sl32: %d  rows: %d", fd, sl32, rows);

  // Initialize
  if (!FSInit()) {
    // Failed to initialize FAT16 ? do something?
    return;
  }
  LogPixel("  FSInit ok");
  
  // Write the shifterlength file.
  BYTE shiftbuff[sizeof( int ) ];
  shiftbuff[0] = (BYTE)((sl32 & 0x0F) | ((rows & 0x3) << 4));
  shiftbuff[1] = 0;

  write_shifter_length_file = FSfopen(SHIFTER_LENGTH_FILENAME, "w");
  if (!shifter_length_file) return;
  FSfwrite(shiftbuff, sizeof shiftbuff, 1, write_shifter_length_file);
  FSfclose(write_shifter_length_file);
  // write the arguments into the meta file.

  // Write the metadata file.
  BYTE buff[sizeof( int ) ]; //was int
  buff[0] = (BYTE)((fd & 0x00FF));
  buff[1] = (BYTE)((fd & 0xFF00) >> 8);
  write_metadata_file = FSfopen(METADATA_FILENAME, "w");
  if (!metadata_file) return;
  FSfwrite(buff, sizeof buff, 1, write_metadata_file);
  FSfclose(write_metadata_file);
  // write the arguments into the meta file.


  // Open the animation file for writing.
  write_animation_file = FSfopen(ANIMATION_FILENAME, "w");

  LogPixel("  open Animation file: 0x%04x", (unsigned int)write_animation_file);

  if (!write_animation_file) {
    // Either file is not present or card is not present
    return;
  }

  frame_delay = fd;
  shifter_len_32 = sl32;
  num_rows = rows == 0 ? 8 : 16;
  state = STATE_WRITE_FILE;
}

// Write a frame
static void WriteFrameToFile( const BYTE frame[] ) {
  // Write the frame to the animation file.
  int size = (3 * 32 * shifter_len_32 * num_rows);
  LogPixel("WriteFrameToFile  %d  sl: %d   rows: %d", size, shifter_len_32, num_rows);

  /*
  // dump frame
  int len = 0;
  char buf[32];
  for ( int subframe = 0; subframe < 3; subframe++ ) {
    for ( int row = 0; row < num_rows; row++ ) {
      len = sprintf(buf, "[%02d : %02d]: ", subframe, row);
      UARTTransmit(1, buf, len);

      for ( int col = 0; col < 32; col++ ) {
        BYTE b = f[(subframe * 256) + (row * 32) + col];
        len = sprintf(buf, " %02x", b);
        UARTTransmit(1, buf, len);
      }

      UARTTransmit(1, "\n", 1);
    }
    UARTTransmit(1, "\n", 1);
  }
  */

  FSfwrite(frame, 3 * 32 * shifter_len_32 * num_rows, 1, write_animation_file);
}

// Close the file. Exit the WRITE_FILE state.
static void StopWriteFile() {
  // Close the animation file.
  LogPixel("StopWritefile");
  FSfclose(write_animation_file);
  state = STATE_NONE;
}



//--------------------------------------------------------------------------------
//
// Interactive stuff
//

static void StartInteractive(int shifter_len_32, int num_rows) {
  // Intialize the matrix.
  RgbLedMatrixEnable(shifter_len_32, num_rows);

  LogPixel("StartInteractive");
  state = STATE_INTERACTIVE;
}

static void StopInteractive() {
  // Close the matrix.
  RgbLedMatrixEnable(0, 0);

  LogPixel("StopInteractive");
  state = STATE_NONE;
}





//--------------------------------------------------------------------------------
//
// External API
//

void PixelInit() {
  StartPlayFile();
}

void PixelTasks() {
  switch (state) {
    case STATE_PLAY_FILE:
      MaybeFrameFromFile();
      break;

    case STATE_NONE:
    case STATE_WRITE_FILE:
    case STATE_INTERACTIVE:
      // Nothing
      break;
  }
}


// protocol
void PixelFrame(const BYTE frame[]) {
  switch (state) {
    case STATE_NONE:
    case STATE_PLAY_FILE:
      // Ignore.
      break;

    case STATE_WRITE_FILE:
      WriteFrameToFile(frame);
      break;

    case STATE_INTERACTIVE:
      RgbLedMatrixFrame(frame);
      break;
  }
}


static void ExitCurrentState() {
  switch (state) {
    case STATE_NONE:
      // Nothing
      break;

    case STATE_PLAY_FILE:
      StopPlayFile();
      break;

    case STATE_WRITE_FILE:
      StopWriteFile();
      break;

    case STATE_INTERACTIVE:
      StopInteractive();
      break;
  }
}

// protocol
void PixelInteractive(int shifter_len_32, int num_rows) {
  ExitCurrentState();
  StartInteractive(shifter_len_32, num_rows);
}

// protocol
void PixelWriteFile(int frame_delay, int shifter_len_32, int num_rows) {
  ExitCurrentState();
  StartWriteFile(frame_delay, shifter_len_32, num_rows);
}

// protocol
void PixelPlayFile() {
  ExitCurrentState();
  StartPlayFile();
}
