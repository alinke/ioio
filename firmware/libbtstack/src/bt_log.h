
#ifndef __BT_LOG_H__
#define __BT_LOG_H__

#import "log.h"


#ifndef ENABLE_LOG


#define LogPrintf(...)
#define LogBLE(...)
#define LogHCI(...)
#define LogHCI_error(...)
#define LogTransport(...)
#define LogLine(...)
#define LogL2CAP(...)
#define LogRunLoop(...)
#define LogHALTick(...)
#define LogSDP(...)

//#import "btstack/hci_cmds.h"
#define LogTransportPacket(...)
#define LogConnPacket(...)
#define LogHCIPacket(...)
#define LogEvent(...)


#else // ENABLE_LOG


#define LogPrintf(...)
#define LogBLE(...)
#define LogHCI(...)
#define LogHCI_error(...)
#define LogTransport(...)
#define LogLine(...)
#define LogL2CAP(...)
#define LogRunLoop(...)
#define LogHALTick(...)
#define LogSDP(...)

//#import "btstack/hci_cmds.h"
#define LogTransportPacket(...)
#define LogConnPacket(...)
#define LogHCIPacket(...)
#define LogEvent(...)


#endif // ENABLE_LOG


#endif // __BT_LOG_H__

