
#ifndef __BT_LOG_H__
#define __BT_LOG_H__


//#import "log.h"

#define LogPrintf(format, ...)  PRINTF("    " format "\n",  ## __VA_ARGS__)


#define LogBLE(...)
#define LogHCI(...)
#define LogHCI_error(...)
#define LogTransport(...)
#define LogLine(...)
#define LogL2CAP(...)
#define LogRunLoop(...)
#define LogHALTick(...)
#define LogSDP(...)

#define LedSetFlag(...)


//#import "btstack/hci_cmds.h"

#define LogTransportPacket(...)
//void LogTransportPacket(const char *tag, const char *usbStatus, const char *bluetoothEvent, uint8_t *packet, int size);

#define LogConnPacket(...)
//void LogConnPacket(const char *tag, uint8_t *packet, int size);

#define LogHCIPacket(...)
//void LogHCIPacket(const char *tag, uint8_t *packet, int size);

#define LogEvent(...)
//void LogEvent(const char *tag, uint8_t *packet, int size);



//const char *stackStateName(HCI_STATE state);

//const char *ogfName(uint16_t opcode);
//const char *ocfName(uint16_t opcode);


#endif // __BT_LOG_H__
