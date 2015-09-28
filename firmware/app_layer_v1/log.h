#ifndef __LOG_H___
#define __LOG_H___


//--------------------------------------------------------------------------------
//
// Module logging
//

#define ENABLE_LOG

#ifndef ENABLE_LOG

#define LogMain(f, ...)
#define LogConn(f, ...)
#define LogProtocol(f, ...)
#define LogFeature(f, ...)
#define LogPixel(f, ...)
#define LogRgb(f, ...)
#define LogUSB(f, ...)

#else // ENABLE_LOG

#define LogMain(f, ...) printf("[main] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LogConn(f, ...) printf("[conn] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LogProtocol(f, ...) printf("[protocol] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LogFeature(f, ...) printf("[feature] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LogPixel(f, ...) printf("[pixel] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LogRgb(f, ...) printf("[rgb] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LogUSB(f, ...) printf("[usb] - [%s:%d] " f "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif // ENABLE_LOG


//--------------------------------------------------------------------------------
//
// UART 1 setup
//
void UART1Init(void);


//--------------------------------------------------------------------------------
//
// Pin 6 output
//
#define SAVE_PIN_FOR_STDIO_LOG(pin) if ((pin == 5) || (pin == 6)) return
#define SAVE_UART_FOR_STDIO_LOG(uart) if (uart == 0) return

void p6init(void);
void p6(int state);


#endif // __LOG_H___
