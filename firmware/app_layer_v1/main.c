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
#include "Compiler.h"
#include "libconn/connection.h"
#include "features.h"
#include "protocol.h"
#include "logging.h"
#include "pixel.h"

#include "uart.h"
#include "traps.h"

#include "log.h"

#include <stdint.h>


//--------------------------------------------------------------------------------
//
// Referenced from   firmware/microchip/usb/usb_host.c
//

// define in non-const arrays to ensure data space
static char descManufacturer[] = "IOIO Open Source Project";
static char descModel[] = "IOIO";
static char descDesc[] = "IOIO Standard Application";
static char descVersion[] = FW_IMPL_VER;
static char descUri[] = "https://github.com/ytai/ioio/wiki/ADK";
static char descSerial[] = "N/A";

const char* accessoryDescs[6] = {
  descManufacturer,
  descModel,
  descDesc,
  descVersion,
  descUri,
  descSerial
};


//--------------------------------------------------------------------------------
//
// main.c  state
//

typedef enum {
  STATE_INIT,
  STATE_OPEN_CHANNEL,
  STATE_WAIT_CHANNEL_OPEN,
  STATE_CONNECTED,
  STATE_ERROR
} STATE;

const char *mainStateName(STATE state) {
  switch ( state ) {
  case STATE_INIT:
    return "STATE_INIT";
  case STATE_OPEN_CHANNEL:
    return "STATE_OPEN_CHANNEL";
  case STATE_WAIT_CHANNEL_OPEN:
    return "STATE_WAIT_CHANNEL_OPEN";
  case STATE_CONNECTED:
    return "STATE_CONNECTED";
  case STATE_ERROR:
    return "STATE_ERROR";
  }
  return "STATE-unknown";
}


// main Channel connection state
static STATE state = STATE_INIT;

STATE mainSetState(STATE new_state) {
  if ( state != new_state ) {
    LogMain("state change  %s -> %s", mainStateName(state), mainStateName(new_state));
  }
  state = new_state;
  return state;
}

// main Channel handle state
static CHANNEL_HANDLE handle;

CHANNEL_HANDLE mainSetHandle(CHANNEL_HANDLE new_handle) {
  if ( handle != new_handle ) {
    LogMain("handle change  0x%04x -> 0x%04x", handle, new_handle);
  }
  handle = new_handle;
  return handle;
}


//--------------------------------------------------------------------------------
//
// AppCallback - called by usb_host
//

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg);

static inline CHANNEL_HANDLE OpenAvailableChannel() {
  int_or_ptr_t arg = { .i = 0 };
  if (ConnectionTypeSupported(CHANNEL_TYPE_ADB)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ADB)) {
      return ConnectionOpenChannelAdb("tcp:4545", &AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_ACC)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_ACC)) {
      return ConnectionOpenChannelAccessory(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_BT)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_BT)) {
      return ConnectionOpenChannelBtServer(&AppCallback, arg);
    }
  } else if (ConnectionTypeSupported(CHANNEL_TYPE_CDC_DEVICE)) {
    if (ConnectionCanOpenChannel(CHANNEL_TYPE_CDC_DEVICE)) {
      return ConnectionOpenChannelCdc(&AppCallback, arg);
    }
  }
  return INVALID_CHANNEL_HANDLE;
}

void AppCallback(const void* data, UINT32 data_len, int_or_ptr_t arg) {
  if (data) {
    if (!AppProtocolHandleIncoming(data, data_len)) {
      // got corrupt input. need to close the connection and soft reset.
      log_printf("Protocol error");
      mainSetState(STATE_ERROR);
    }
  } else {
    // data == NULL

    // connection closed, soft reset and re-establish
    if (state == STATE_CONNECTED) {
      log_printf("Channel closed");
      SoftReset();
    } else {
      log_printf("Channel failed to open");
    }
    mainSetState(STATE_OPEN_CHANNEL);
  }
}


// reset RCON value
static uint16_t reset_rcon;

static uint16_t reset_sp;
static uint16_t reset_splim;

int main() {
  reset_sp = (uint16_t)getTopOfStack();
  reset_splim = SPLIM;
  reset_rcon = RCON;

  p6init();
  UART1Init();

  LogMain();
  LogMain("start  RCON: 0x%04x", reset_rcon);
  LogMain("     reset_SP: 0x%04x", reset_sp);
  LogMain("  reset_SPLIM: 0x%04x", reset_splim);
  LogMain("           SP: 0x%04x", (unsigned int)getTopOfStack());


  // IOIO logging init
  log_init();
  log_printf("***** Hello from app-layer! *******");


  SoftReset();    // PixelInit gets called by this


  LogMain("main()  state = %s", mainStateName(state) );

  LogMain("connection init");
  LogMain("SP: 0x%04x", (unsigned int)getTopOfStack());


  ConnectionInit();

  while (1) {
    // LogMain("  loop TOP state = %s", mainStateName(state) );

    PixelTasks();
    ConnectionTasks();

    switch (state) {
      case STATE_INIT:
        mainSetHandle(INVALID_CHANNEL_HANDLE);
        LogMain("INIT  handle %04x", handle);
        mainSetState(STATE_OPEN_CHANNEL);
        break;

      case STATE_OPEN_CHANNEL:
        if (mainSetHandle(OpenAvailableChannel()) != INVALID_CHANNEL_HANDLE) {
          LogMain("OPEN_CHANNEL  handle %04x", handle);
          log_printf("Connected");
          mainSetState(STATE_WAIT_CHANNEL_OPEN);
        }
        break;

      case STATE_WAIT_CHANNEL_OPEN:
        LogMain("handle = 0x%04x", handle);

       if (ConnectionCanSend(handle)) {
         LogMain("CHANNEL OPEN  handle %04x   AppProtocolInit", handle);
          log_printf("Channel open");
          AppProtocolInit(handle);
          mainSetState(STATE_CONNECTED);
        }
        break;

      case STATE_CONNECTED:
        // LogMain("AppProtocolTasks  0x%04x", handle);
        AppProtocolTasks(handle);
        break;

      case STATE_ERROR:
        LogMain("ERROR  connection close  handle %04x", handle);
        ConnectionCloseChannel(handle);
        SoftReset();
        mainSetState(STATE_INIT);
        break;
    }
  }
  return 0;
}
