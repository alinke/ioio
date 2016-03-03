
#include "device_info.h"

#ifdef ENABLE_DEVICE_INFO

#include "Compiler.h"
#include <stdint.h>

#include "protocol_defs.h"
#include "protocol.h"

#include "rgb_led_matrix.h"

#include "log.h"


typedef struct {
  char display_name[16];

  int brightness_level;
  int battery_level;
} DEVICE_INFO_STATE;

static DEVICE_INFO_STATE device_info;


void DeviceInfoInit() {
  sprintf(device_info.display_name, "CAT (00:00)");
  device_info.brightness_level = 7;
  device_info.battery_level = 85;
}

// brightness
//   level  0 - 15
void DeviceSetBrightnessLevel(int level) {
  int new_level = level;
  if ( new_level < 0 )
    new_level = 0;
  if ( new_level > 15 )
    new_level = 15;

  device_info.brightness_level = new_level;
  RgbLedSetBrightness( device_info.brightness_level );
}

int DeviceGetBrightnessLevel() {
  return device_info.brightness_level;
}

int DeviceGetBatteryLevel() {
  return device_info.battery_level;
}


// Send a device info message
void DeviceSendInfo() {
  OUTGOING_MESSAGE msg;
  msg.type = API2_OUT_GET_DEVICE_INFO;
  msg.args.api2_out_get_device_info.brightness_level = DeviceGetBrightnessLevel();
  msg.args.api2_out_get_device_info.battery_level = DeviceGetBatteryLevel();

  LogMain("DeviceSendInfo  type: 0x%02x  brightness: %d  battery: %d",
          msg.type,
          msg.args.api2_out_get_device_info.brightness_level,
          msg.args.api2_out_get_device_info.battery_level);

  AppProtocolSendMessage(&msg);
}

#endif // ENABLE_DEVICE_INFO


