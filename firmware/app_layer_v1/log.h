
#ifndef __LOG_H___
#define __LOG_H___

#include <stdarg.h>

#define NO_ANIMATION


//--------------------------------------------------------------------------------
//
// General logging
//
void LogInit(void);
void LogFlush(void);


//--------------------------------------------------------------------------------
//
// Module logging
//
void LogLine( const char * format, ...);

void LogMain( const char * format, ...);

void LogFeature( const char * format, ...);
void LogPixel( const char * format, ...);
void LogRgb( const char * format, ...);

void LogConn( const char * format, ...);
void LogUSB( const char * format, ...);

void LogHCI( const char * format, ...);
void LogHCI_error( const char * format, ...);

void LogL2CAP( const char * format, ...);
void LogRFCOMM( const char * format, ...);

void LogRunLoop( const char * format, ...);
void LogTransport( const char * format, ...);

void LogHAL( const char * format, ...);

void LogSDP( const char * format, ...);

void LogSM( const char * format, ...);

void LogBLE( const char * format, ...);


//--------------------------------------------------------------------------------
//
// LED Debugging
//

#define LED_DEBUG


#ifdef LED_DEBUG

// from  <stdint.h>
#ifndef uint8_t
typedef unsigned char uint8_t;
#define uint8_t uint8_t
#define UINT8_MAX (255)
#endif
//

#define OFF       0x00
#define RED       0x04
#define GREEN     0x02
#define BLUE      0x01
#define MAGENTA   (RED | BLUE)
#define YELLOW    (RED | GREEN)
#define CYAN      (GREEN | BLUE)
#define WHITE     (RED | GREEN | BLUE)

void LedDebugInit(void);

void LedDebugFrame(void * frame, int size);


void LedSetFlag(int row, int col, uint8_t color);

#else // LED_DEBUG


// from  <stdint.h>
#ifndef uint8_t
typedef unsigned char uint8_t;
#define uint8_t uint8_t
#define UINT8_MAX (255)
#endif
//

void LedSetFlag(int row, int col, uint8_t color);


#endif // LED_DEBUG


#endif // __LOG_H___
