
#ifndef __BTSTACK_CONFIG_H__
#define __BTSTACK_CONFIG_H__

#include "log.h"

#define EMBEDDED

//#define NO_RUN_LOOP


#ifdef ENABLE_LOG

#define ENABLE_LOG_DEBUG
#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR

#endif // ENABLE_LOG


#define HAVE_BLE

#define HAVE_TICK


#define HCI_ACL_PAYLOAD_SIZE 252  // will make total packet 256

// 
#define MAX_SPP_CONNECTIONS 1

#define MAX_NO_HCI_CONNECTIONS MAX_SPP_CONNECTIONS
#define MAX_NO_L2CAP_SERVICES  2
#define MAX_NO_L2CAP_CHANNELS  (1+MAX_SPP_CONNECTIONS)
#define MAX_NO_RFCOMM_MULTIPLEXERS MAX_SPP_CONNECTIONS
#define MAX_NO_RFCOMM_SERVICES 1
#define MAX_NO_RFCOMM_CHANNELS MAX_SPP_CONNECTIONS
#define MAX_NO_DB_MEM_DEVICE_LINK_KEYS  2
#define MAX_NO_DB_MEM_DEVICE_NAMES 0
#define MAX_NO_DB_MEM_SERVICES 1

// LE options ?
#define MAX_NO_GATT_CLIENTS 0
#define MAX_NO_GATT_SUBCLIENTS 0
#define MAX_NO_BNEP_SERVICES 0
#define MAX_NO_BNEP_CHANNELS 0
#define MAX_NO_HFP_CONNECTIONS 0


// LED logging
#include <stdint.h>

#define OFF       0x00
#define RED       0x04
#define GREEN     0x02
#define BLUE      0x01
#define MAGENTA   (RED | BLUE)
#define YELLOW    (RED | GREEN)
#define CYAN      (GREEN | BLUE)
#define WHITE     (RED | GREEN | BLUE)

void set_app_log_flag(int row, int col, uint8_t color);
uint8_t get_app_log_flag(int row, int col);

void clear_app_log_flag_row(int row);

// LED logging


#endif // __BTSTACK_CONFIG_H__
