/*
 * Copyright 2011 Ytai Ben-Tsvi. All rights reserved.
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

#ifndef __UART_H__
#define __UART_H__

// The uart module uses 2144 bytes of memory
//
// Without ENABLE_UART
//   heap                        0x6f8c                               0x600  (1536)
//   stack                       0x758c                               0xa74  (2676)
//   Maximum dynamic memory (bytes):         0x1074  (4212)
//
// With ENABLE_UART
//   heap                        0x77ee                               0x600  (1536)
//   stack                       0x7dee                               0x212  (532)
//   Maximum dynamic memory (bytes):          0x812  (2068)
//

//#define ENABLE_UART

#ifndef ENABLE_UART

#define UARTInit()
#define UARTConfig(...)
#define UARTTransmit(...)
#define UARTTasks()

#else // ENABLE_UART

void UARTInit();
void UARTConfig(int uart_num, int rate, int speed4x, int two_stop_bits,
                int parity);
void UARTTransmit(int uart_num, const void* data, int size);
void UARTTasks();

#endif // ENABLE_UART


#endif  // __UART_H__
