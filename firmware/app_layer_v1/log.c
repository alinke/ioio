#include "build_number.h"

#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "HardwareProfile.h"


#include "sdcard/FSIO.h"
#include "sdcard/SD-SPI.h"

#include "uart.h"

#include "log.h"


//
// PIN6 toggle
//
void p6init(void) {
  // PIN6   RP12 / GD7 / CN56 / RD11   pin 45
  ODCD &= ~0x0800;
  TRISD &= ~0x0800;

  for ( int i = 0; i < 2; i++ ) {
    LATD |= 0x0800;
    for ( int j = 0; j < 63; j++ )
      asm("nop\n");
    LATD &= ~0x0800;
    for ( int j = 0; j < 63; j++ )
      asm("nop\n");
  }
}

void p6(int state) {
  if (state)
    LATD |= 0x0800;
  else
    LATD &= ~0x0800;
}


//--------------------------------------------------------------------------------
//
// STDIO UART 1 Logging
//

void UART1Init(void) {
  // UART 1
  RPINR18 = 0x3f04;   // RX on RP4
  RPOR1 = ( 0x0300 | ( RPOR1 & 0x00ff ) );    // TX on RP3    U1TX output function 3
  // Setup
  //  UARTConfig(0, 34, 1, 0, 0);   // 115k  114285.7

  U1BRG = 34;
  U1MODE = 0;
  U1MODEbits.BRGH = BRGH2;
  U1STA = 0;
  U1MODEbits.UARTEN = 1;
  U1STAbits.UTXEN = 1;
  IFS0bits.U1RXIF = 0;
}



//--------------------------------------------------------------------------------
//
// UART Logging
//

static int enable_uart_logging = 0;

void LogUARTInit(void) {
  /*
  _uartMsg[0] = 0x01;
  _uartMsg[1] = 0x03;
  _uartMsg[2] = 0x07;
  _uartMsg[3] = 0x0f;
  _uartMsg[4] = 0xc0;
  _uartMsg[6] = 0x00;
  */

  //    UARTConfig(1, 115200, 0, 0, 0);
  //  UARTConfig(1, 8, 0, 0, 0);   // 115k  111111.1
  UARTConfig(1, 34, 1, 0, 0);   // 115k  114285.7
  //  UARTConfig(1, 15, 1, 0, 0);   // 250k  250000.0
  //  UARTConfig(1, 12, 1, 0, 0);   // 300k  307692.3
  // UARTConfig(1, 7, 1, 0, 0);    // 500k  500000.0
  //  UARTConfig(1, 3, 1, 0, 0);    // 1M   1000000.0
  //  UARTConfig(1, 1, 1, 0, 0);    // 2M   2000000.0
  //  UARTConfig(1, 0, 1, 0, 0);    // 4M   4000000.0

  RPINR19 = 0x3f0a;    // RX on R10
  RPOR8 = 0x0500;      // TX on R17
}


void LogInit(void) {
  int i;
  if ( enable_uart_logging ) {
    for ( i = 0; i < 128; i++ )
      UARTTransmit(1, "-", 1);

    UARTTransmit(1, "\n", 1);
    UARTTransmit(1, "\n", 1);
  }
}


static int log_num = 0;
//static char p_log_buf[250];
static char p_log_buf[4];

int LogWriteModule(const char *module)
{
  log_num++;
  int len = sprintf( p_log_buf, "%04d [%s] ", log_num, module );
  return len;
}

void LogUARTWrite(int len0, int len1)
{
  int len = ( len0 + len1 );
  if ( len > 250 )
    len = 250;
  p_log_buf[len] = '\n';
  len++;
  p_log_buf[len] = 0;

  UARTTransmit(1, p_log_buf, len);
}



void LogBytes( uint8_t *data, int size ) {
  if ( enable_uart_logging ) {
    int len = LogWriteModule( "main" );
    UARTTransmit(1, p_log_buf, len);

    int num = size;
    for ( int i = 0; i < num; i++ ) {
      int len1 = sprintf( p_log_buf, " %02x", data[i] );
      UARTTransmit(1, p_log_buf, len1);
    }

    int len2 = sprintf( p_log_buf, "\n" );
    UARTTransmit(1, p_log_buf, len2);
  }
}


/*
void LogMain( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "main" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
*/

void LogFeature( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "feature" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
void LogPixel( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "pixel" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
void LogRgb( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "rgb" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

/*
void LogProtocol( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "protocol" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
*/
/*
void LogConn( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "conn" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
*/
void LogUSB( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "usb" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}


/*
void LogLine( const char * format, ...)
{
  if ( enable_uart_logging ) {
    log_num++;
    int len0 = sprintf( p_log_buf, "%04d ", log_num );

    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    
    LogUARTWrite( len0, len1 );
  }
}


void LogHCI( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "hci" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
void LogHCI_error( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "hci ERROR" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

void LogL2CAP( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "l2cap" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
void LogRFCOMM( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "rfcomm" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}


void LogRunLoop( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "run_loop" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
void LogTransport( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "hci_transport" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

static char *tick_str = ".";
void LogHALTick(void)
{
  if ( enable_uart_logging ) {
    UARTTransmit(1, tick_str, 1);
  }
}
static char *tick_str2 = "#";
void LogHALTick2(void)
{
  if ( enable_uart_logging ) {
    UARTTransmit(1, tick_str2, 1);
  }
}

void LogHAL( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "hal" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

void LogSDP( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "sdp" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

void LogSM( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "sm" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}

void LogBLE( const char * format, ...)
{
  if ( enable_uart_logging ) {
    int len0 = LogWriteModule( "ble" );
    va_list argptr;
    va_start(argptr, format);
    int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
    va_end(argptr);
    LogUARTWrite( len0, len1 );
  }
}
*/

