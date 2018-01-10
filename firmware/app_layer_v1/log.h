
#ifndef __LOG_H___
#define __LOG_H___

#include <stdio.h>
#include "GenericTypeDefs.h"


#define SAVE_PIN_FOR_STDIO_LOG(pin) if ((pin == 4) || (pin == 5)) return
#define SAVE_UART_FOR_STDIO_LOG(uart) if (uart == 0) return


void P34init(void);
void P34(int state);
#define TRIGGER_ON    (LATB |= 0x0004)
#define TRIGGER_OFF   (LATB &= ~0x0004)

void T2Setup();
#define T2_ON    (LATD |= 0x0800)
#define T2_OFF   (LATD &= ~0x0800)


void UART1Init(void);
void UART1Tasks(void);


//--------------------------------------------------------------------------------
//
// Logging
//

#define LogAppLed(f, ...)
#define LogAppLedRaw(f, ...)

//#define LogLight(f, ...) printf("[light.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogLightRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogLight(f, ...)
#define LogLightRaw(f, ...)

//#define LogFirmware(f, ...) printf("[firmware.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogFirmwareRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogFirmware(f, ...)
#define LogFirmwareRaw(f, ...)

//#define LogDB(f, ...) printf("[remote_device_db_cat.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogDBRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogDB(f, ...)
#define LogDBRaw(f, ...)

#define LogHCI(f, ...)

//#define LogConn(f, ...) printf("[bt_connection.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogConnRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogConn(f, ...)
#define LogConnRaw(f, ...)

//#define LogANCS(f, ...) printf("__ANCS__ [ancs_client.c:%d] " f "\n", __LINE__, ##__VA_ARGS__)
//#define LogANCSRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogANCS(f, ...)
#define LogANCSRaw(f, ...)

//#define LogBLE(f, ...) printf("[ble_connection.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogBLERaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogBLE(f, ...)
#define LogBLERaw(f, ...)

#define LogFeatures(f, ...)
#define LogFeaturesRaw(f, ...)

#define LogButton(f, ...)
//#define LogButton(f, ...) printf("[button.c:%d] " f "", __LINE__, ##__VA_ARGS__)

//#define LogProtocol(f, ...) printf("[protocol.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogProtocolRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogProtocol(f, ...)
#define LogProtocolRaw(f, ...)

#define LogProtocol2(f, ...) printf("[protocol.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogProtocolRaw2(f, ...) printf(f, ##__VA_ARGS__)
//#define LogProtocol2(f, ...)
//#define LogProtocolRaw2(f, ...)

//#define LogFile(f, ...) printf("[file.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogFile(f, ...)


#define LogMain(f, ...)


//#define LogDevice(f, ...) printf("[device.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogDeviceRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogDevice(f, ...)
#define LogDeviceRaw(f, ...)
    
//#define LogMatrix(f, ...) printf("[rgb.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogMatrixRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogMatrix(f, ...)
#define LogMatrixRaw(f, ...)
    

#define LogContext(f, ...)
#define LogContextRaw(f, ...)



#define LogBoot(f, ...) printf("[BOOT main.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogBootRaw(f, ...) printf(f, ##__VA_ARGS__)
//#define LogBoot(f, ...)
//#define LogBootRaw(f, ...)

//#define LogMedia(f, ...) printf("[media.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogMediaRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogMedia(f, ...)
#define LogMediaRaw(f, ...)

//#define LogMediaLibrary(f, ...) printf("[media_library.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogMediaLibraryRaw(f, ...) printf(f, ##__VA_ARGS__)
#define LogMediaLibrary(f, ...)
#define LogMediaLibraryRaw(f, ...)


//#define LogPixel2(f, ...) printf("[pixel.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogPixel2(f, ...)

//#define LogPixelApp(f, ...) printf("[pixel_app.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogPixelApp(f, ...)

#define LogPixel(f, ...) printf("[pixel.c:%d] " f "", __LINE__, ##__VA_ARGS__)
//#define LogPixel(f, ...)

//#define LogGIF(f, ...) printf("[gif.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogGIF(f, ...)

//#define LogPlayRaw(f, ...) printf("[app_raw.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogPlayRaw(f, ...)

//#define LogPlayGif(f, ...) printf("[app_gif.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogPlayGif(f, ...)

//#define LogPlayText(f, ...) printf("[app_text.c:%d] " f "", __LINE__, ##__VA_ARGS__)
#define LogPlayText(f, ...)

#define LogFS(f, ...)

#define LogSD(f, ...)


    
//sudo screen -L /dev/tty.usbserial-FTZ2ABOT 115200

#endif // __LOG_H___
