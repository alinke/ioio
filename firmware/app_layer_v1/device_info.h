
#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__


#define ENABLE_DEVICE_INFO


#ifndef ENABLE_DEVICE_INFO

#define DeviceInfoInit()

#define DeviceSetBrightnessLevel(level)
#define DeviceGetBrightnessLevel()
#define DeviceGetBatteryLevel()
#define DeviceSendInfo()

#else // ENABLE_DEVICE_INFO

//#include <stdint.h>

// Initialize this module.
void DeviceInfoInit();

void DeviceSetBrightnessLevel(int level);
int DeviceGetBrightnessLevel();

int DeviceGetBatteryLevel();

void DeviceSendInfo();

#endif // ENABLE_DEVICE_INFO


#endif // __DEVICE_INFO_H__
