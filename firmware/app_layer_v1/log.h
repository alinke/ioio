
#ifndef __LOG_H___
#define __LOG_H___

#include <stdarg.h>


void p6init(void);
void p6(int state);

#define SAVE_PIN_FOR_STDIO_LOG(pin) if ((pin == 5) || (pin == 6)) return
#define SAVE_UART_FOR_STDIO_LOG(uart) if (uart == 0) return

void UART1Init(void);


// from  <stdint.h>
#ifndef uint8_t
typedef unsigned char uint8_t;
#define uint8_t uint8_t
#define UINT8_MAX (255)
#endif
//
void LogUARTInit(void);

void LogBytes( uint8_t *data, int size );
void LogHALTick(void);
void LogHALTick2(void);



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

//#define LogMain(f, ...)
#define LogMain(f, ...) printf("[main] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)

//#define LogConn(f, ...)
#define LogConn(f, ...) printf("[conn] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)

//#define LogProtocol(f, ...)
#define LogProtocol(f, ...) printf("[protocol] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)


//void LogMain( const char * format, ...);
//void LogConn( const char * format, ...);
//void LogProtocol( const char * format, ...);

void LogPixel( const char * format, ...);
void LogRgb( const char * format, ...);

void LogFeature( const char * format, ...);

void LogUSB( const char * format, ...);





// libbtstack log helpers
/*
void LogLine( const char * format, ...);

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
*/


#endif // __LOG_H___
