
#include "build_number.h"

#include "sdcard/FSIO.h"
#include "sdcard/SD-SPI.h"

#include "log.h"



#define FLUSH_EACH_WRITE

static FSFILE *p_log_file = NULL;
static char p_log_buf[250];
//static char lines[128][64];

static char file_name[16];



//
// Add code to read configuration bits from a file
//

static int log_line_enabled = 1;
static int log_main_enabled = 1;

static int log_feature_enabled = 1;
static int log_pixel_enabled = 1;
static int log_rgb_enabled = 1;

static int log_conn_enabled = 1;
static int log_bt_enabled = 1;
static int log_usb_enabled = 1;
static int log_hal_enabled = 1;



static int log_init_done = 0;

void LogInit(void)
{

  //  if ( p_log_file != NULL )
  //    FSfclose( p_log_file );

  // create the file name
  sprintf( file_name, "log-%d.txt", BUILD_NUMBER );

  // init the filesystem
  if ( !FSInit() ) {
    return;
  }
  
  p_log_file = FSfopen( file_name, "a");

  if ( p_log_file != NULL ) {
    // Log build_number
    int len = sprintf( p_log_buf, "# Build_number: %d\n", BUILD_NUMBER );
    FSfwrite( p_log_buf, len, 1, p_log_file );
  }

#ifdef FLUSH_EACH_WRITE
  // close
  FSfclose( p_log_file );
  p_log_file = NULL;
#endif // FLUSH_EACH_WRITE

  log_init_done = 1;

}


void LogFlush(void)
{
  if ( log_init_done == 0 )
    return;

#ifndef FLUSH_EACH_WRITE
  if ( p_log_file != NULL ) {
    // close
    FSfclose( p_log_file );
    p_log_file = FSfopen( file_name, "a");
  }
#endif // FLUSH_EACH_WRITE
}


static int log_num = 0;

void LogStartWrite(void)
{
#ifdef FLUSH_EACH_WRITE
  if ( p_log_file == NULL )
    p_log_file = FSfopen( file_name, "a");
#endif // FLUSH_EACH_WRITE
}
void LogEndWrite(int len0, int len1)
{
  int len = ( len0 + len1 );
  if ( len > 250 )
    len = 250;
  p_log_buf[len] = '\n';
  len++;
  p_log_buf[len] = 0;

  FSfwrite( p_log_buf, len, 1, p_log_file );

#ifdef FLUSH_EACH_WRITE
  FSfclose( p_log_file );
  p_log_file = NULL;
#endif // FLUSH_EACH_WRITE
}

int LogWriteModule(const char *module)
{
  log_num++;
  int len = sprintf( p_log_buf, "%04d [%s] ", log_num, module );
  return len;
}


//--------------------------------------------------------------------------------
//
// Module logging
//

void LogLine( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_line_enabled )
    return;

  LogStartWrite();

  log_num++;
  int len0 = sprintf( p_log_buf, "%04d ", log_num );

  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogMain( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_main_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "main" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogFeature( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_feature_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "feature" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogPixel( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_pixel_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "pixel" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogRgb( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_rgb_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "rgb" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogConn( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_conn_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "conn" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogUSB( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_usb_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "usb" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}


void LogHCI( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "hci" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogHCI_error( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "hci ERROR" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogL2CAP( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "l2cap" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogRFCOMM( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "rfcomm" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}


void LogRunLoop( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "run_loop" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}
void LogTransport( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "hci_transport" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogHAL( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_hal_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "hal" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogSDP( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "sdp" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogSM( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "sm" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}

void LogBLE( const char * format, ...)
{
  if ( log_init_done == 0 )
    return;

  if ( !log_bt_enabled )
    return;

  LogStartWrite();

  int len0 = LogWriteModule( "ble" );
  va_list argptr;
  va_start(argptr, format);
  int len1 = vsnprintf( &p_log_buf[len0], ( sizeof(p_log_buf) - len0 ), format, argptr);
  va_end(argptr);

  LogEndWrite( len0, len1 );
}




//--------------------------------------------------------------------------------
//
// LED Debugging
//
#ifdef LED_DEBUG


static uint8_t led_flags[256];


void LedDebugClearFlags(void)
{
  for ( int row = 0; row < 8; row++ ) {
    for ( int col = 0; col < 32; col++ ) {
      led_flags[ ( row * 32 ) + col ] = 0x00;
    }
  }
}

static int color_idx = 0;
static uint8_t color = 0x24;

void nextColor()
{
      color_idx++;
      if ( color_idx == 3 )
        color_idx = 0;
      
      switch ( color_idx ) {
      case 0:
        color = RED;
        break;
      case 1:
        color = GREEN;
        break;
      case 2:
        color = BLUE;
        break;
      default:
        color = WHITE;
        break;
      }
}

void LedDebugTestPattern(void)
{
  for ( int row = 0; row < 16; row++ ) {
    for ( int col = 0; col < 32; col++ ) {
      LedSetFlag( row, col, color );
      nextColor();
    }
  }
}


void LedDebugInit(void)
{
  LedDebugClearFlags();
  //  LedDebugTestPattern();
}


static int flags_toggle2 = 0;

void LedUpdateFrame(void)
{
  // LedDebugInit();

  flags_toggle2 = !flags_toggle2;
  if ( flags_toggle2 )
    led_flags[0] = 0x10;
  else
    led_flags[0] = 0x00;
}


void LedDebugFrame(void * frame, int size)
{
  //  LedDebugTestPattern();

  LedUpdateFrame();

  // copy the frame
  memcpy(frame, led_flags, 256);
  //  memcpy((frame+2048), led_flags, 256);
  //  memcpy((frame+4096), led_flags, 256);
  memcpy((frame+256), led_flags, 256);
  memcpy((frame+512), led_flags, 256);
}


void LedSetFlag(int row, int col, uint8_t color)
{

  if ( row < 8 ) {
    // upper nibble
    int index = ( ( row * 32 ) + col );
    led_flags[index] = ( ( led_flags[index] & 0xc7 ) + ( ( color & 0x07 ) << 3 ) );
  } else {
    // lower nibble
    int index = ( ( ( row - 8 ) * 32 ) + col );
    led_flags[index] = ( ( led_flags[index] & 0xf8 ) + ( color & 0x07 ) );
  }
}


#else // LED_DEBUG

void LedSetFlag(int row, int col, uint8_t color)
{
}



#endif // LED_DEBUG
