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

#ifndef NO_ANIMATION 
#define ANIMATION_FILENAME "artdata.bin"
#define METADATA_FILENAME "fps.bin"
#define SHIFTER_LENGTH_FILENAME "matrixtp.bin"
#endif // NO_ANIMATION


#include "log.h"


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

#ifndef NO_ANIMATION 
static FSFILE *animation_file;
static FSFILE *metadata_file;
static FSFILE *shifter_length_file;
#endif // NO_ANIMATION

// TODO: this can be optimized - we already have a buffer of this size for this
// purpose in RgbLedMatrix.
// BYTE frame[1536 * 4] __attribute__((far));

////////////////////////////////////////////////////////////////////////////////
// PlayFile stuff

static void StartPlayFile() {
  
#ifndef NO_ANIMATION
  if (!FSInit()) {
    // Failed to initialize FAT16 ? do something?
    return;
  }
#endif // NO_ANIMATION


  // Read metadata file.
  //Open the shifter length file.
  BYTE shiftbuff[sizeof(int)];
#ifndef NO_ANIMATION
  shifter_length_file = FSfopen(SHIFTER_LENGTH_FILENAME, "r");
  if (!shifter_length_file) return; //move on if the matrix type file is not there
  //if (!fsize(shifter_length_file) > 0) return; //move on if the file is 0 bytes
  FSfread(shiftbuff, sizeof( int ), 1, shifter_length_file);
  FSfclose(shifter_length_file);
#else // NO_ANIMATION
  shiftbuff[0] = 0x01;
  shiftbuff[1] = 0x00;
#endif // NO_ANIMATION
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


#ifndef NO_ANIMATION
  //Open the metadata File
  BYTE buff[sizeof(int)];
  metadata_file = FSfopen(METADATA_FILENAME, "r");
  if (!metadata_file) return;
 // if (!fsize(metadata_file) > 0) return; //move on if the file is 0 bytes

  FSfread(buff, sizeof( int ), 1, metadata_file);  //changed to float FSfread(buff, sizeof( int ), 1, metadata_file);
  FSfclose(metadata_file);

  //Store the framerate into frame_delay
  frame_delay = buff[0] | (buff[1]  << 8);
#else // NO_ANIMATION
  frame_delay = 6249;
#endif // NO_ANIMATION


#ifndef NO_ANIMATION
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
#endif // NO_ANIMATION

  // Initialize the matrix.
  RgbLedMatrixEnable(shifter_len_32, rows);

  // Initialize the timer.
  // Stop the timer
  T1CON = 0x0000;
  // Period is (PR1 + 1) / 62500 (seconds)
  PR1 = frame_delay;
  TMR1 = 0x0000;
  _T1IF = 0;
  // Start the timer @ 62.5[kHz]
  T1CON = 0x8030;

  state = STATE_PLAY_FILE;
}

//int fsize(FILE *fp){
//    int prev=ftell(fp);
//    fseek(fp, 0L, SEEK_END);
//    int sz=ftell(fp);
//    fseek(fp,prev,SEEK_SET); //go back to where we were
//    return sz;
//}

void hal_tick_call_handler(void);


static void MaybeFrameFromFile() {
  // If our timer elapsed, push a frame to the display.
  if (_T1IF) {
    void * frame;
    unsigned int dswpag;
    RgbLedMatrixGetBackBuffer(&frame, &dswpag);
    // There's already a frame in the back-buffer, we're not keeping up...
    if (!frame) return;

    _T1IF = 0;

    DSWPAG = dswpag;
    

#ifndef NO_ANIMATION
    FSfread(frame, RgbLedMatrixFrameSize(), 1, animation_file);
#endif // NO_ANIMATION

    hal_tick_call_handler();

#ifdef LED_DEBUG
    LedDebugFrame(frame, RgbLedMatrixFrameSize());
#endif // LED_DEBUG

    RgbLedMatrixSwapFrame();

#ifndef NO_ANIMATION
    if (FSfeof(animation_file)) {
      // Rewind
      FSfseek(animation_file, 0, SEEK_SET);
    }
#endif // NO_ANIMATION
  }
}

static void StopPlayFile() {
  // Stop the timer.
  T1CON = 0x0000;

#ifndef NO_ANIMATION
  // Close the file.
  FSfclose(animation_file);
#endif // NO_ANIMATION

  // Close the matrix.
  RgbLedMatrixEnable(0, 0);

  state = STATE_NONE;
}





////////////////////////////////////////////////////////////////////////////////
// WriteFile stuff

static void StartWriteFile(int fd, int sl32, int rows) {  //was int fd

#ifndef NO_ANIMATION 
  // Initialize
  if (!FSInit()) {
    // Failed to initialize FAT16 ? do something?
    return;
  }
#endif // NO_ANIMATION 
  
  // Write the shifterlength file.
  BYTE shiftbuff[sizeof( int ) ];
  shiftbuff[0] = (BYTE)((sl32 & 0x0F) | ((rows & 0x3) << 4));
  shiftbuff[1] = 0;

#ifndef NO_ANIMATION 
  shifter_length_file = FSfopen(SHIFTER_LENGTH_FILENAME, "w");
  if (!shifter_length_file) return;
  FSfwrite(shiftbuff, sizeof shiftbuff, 1, shifter_length_file);
  FSfclose(shifter_length_file);
  // write the arguments into the meta file.
#endif // NO_ANIMATION 

  // Write the metadata file.
  BYTE buff[sizeof( int ) ]; //was int
  buff[0] = (BYTE)((fd & 0x00FF));
  buff[1] = (BYTE)((fd & 0xFF00) >> 8);

#ifndef NO_ANIMATION 
  metadata_file = FSfopen(METADATA_FILENAME, "w");
  if (!metadata_file) return;
  FSfwrite(buff, sizeof buff, 1, metadata_file);
  FSfclose(metadata_file);
  // write the arguments into the meta file.
#endif // NO_ANIMATION 
  
#ifndef NO_ANIMATION 
  // Open the animation file for writing.
  animation_file = FSfopen(ANIMATION_FILENAME, "w");
  if (!animation_file) {
    // Either file is not present or card is not present
    return;
  }
#endif // NO_ANIMATION 

  frame_delay = fd;
  shifter_len_32 = sl32;
  num_rows = rows == 0 ? 8 : 16;
  state = STATE_WRITE_FILE;
}

static void WriteFrameToFile(const BYTE f[]) {
#ifndef NO_ANIMATION 
  // Write the frame to the animation file.
  FSfwrite(f, 3 * 32 * shifter_len_32 * num_rows, 1, animation_file);
#endif // NO_ANIMATION 
}

static void StopWriteFile() {
#ifndef NO_ANIMATION 
  // Close the animation file.
  FSfclose(animation_file);
  state = STATE_NONE;
#endif // NO_ANIMATION 
}



////////////////////////////////////////////////////////////////////////////////
// Interactive stuff

static void StartInteractive(int shifter_len_32, int num_rows) {
  // Intialize the matrix.
  RgbLedMatrixEnable(shifter_len_32, num_rows);

  state = STATE_INTERACTIVE;
}

static void StopInteractive() {
  // Close the matrix.
  RgbLedMatrixEnable(0, 0);

  state = STATE_NONE;
}



////////////////////////////////////////////////////////////////////////////////
// External API

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


void PixelInteractive(int shifter_len_32, int num_rows) {
  ExitCurrentState();
  StartInteractive(shifter_len_32, num_rows);
}

void PixelWriteFile(int frame_delay, int shifter_len_32, int num_rows) {   //void PixelWriteFile(int frame_delay, int shifter_len_32) {
  ExitCurrentState();
  StartWriteFile(frame_delay, shifter_len_32, num_rows);
}

void PixelPlayFile() {
  ExitCurrentState();
  StartPlayFile();
}
