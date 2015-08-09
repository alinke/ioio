
#ifndef __BT_LOG_H__
#define __BT_LOG_H__

#import "log.h"
#import "btstack/hci_cmds.h"

void LogTransportPacket(const char *tag, const char *usbStatus, const char *bluetoothEvent, uint8_t *packet, int size);

void LogConnPacket(const char *tag, uint8_t *packet, int size);

void LogHCIPacket(const char *tag, uint8_t *packet, int size);
void LogEvent(const char *tag, uint8_t *packet, int size);

/*
char *get_packet_string(uint8_t *packet, uint16_t size);
const char *substateName(uint8_t state);
char *hciEventName(uint8_t code);
*/

const char *stackStateName(HCI_STATE state);


const char *ogfName(uint16_t opcode);
const char *ocfName(uint16_t opcode);

#endif // __BT_LOG_H__
