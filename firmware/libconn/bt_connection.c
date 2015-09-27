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

// Application-layer Bluetooth logic.

#include "bt_connection.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "logging.h"
#define USB_SUPPORT_HOST
#include "usb_host_bluetooth.h"
#include "hci.h"
#include "l2cap.h"
#include "rfcomm.h"
#include "sdp.h"

#include "btstack_memory.h"
#include "hci_transport.h"
#include "btstack/sdp_util.h"


//
//#include "spp_and_le_counter.h"
#include "le_conn.h"


#include "att.h"
#include "att_server.h"
#include "le_device_db.h"
#include "gap_le.h"
#include "sm.h"
//

#include "uart.h"


/*
 * @section Advertisements 
 *
 * @text The Flags attribute in the Advertisement Data indicates if a device is in dual-mode or not.
 * Flag 0x02 indicates LE General Discoverable, Dual-Mode device. See Listing advertisements.
 */
/* LISTING_START(advertisements): Advertisement data: Flag 0x02 indicates a dual mode device */
const uint8_t adv_data[] = {
    // Flags: General Discoverable
    0x02, 0x01, 0x02, 
    // Service
    0x11, 0x06, 0xF5, 0x1E, 0x6B, 0xD5, 0x2D, 0x04, 0x39, 0x89, 0x2A, 0x42, 0x61, 0x6D, 0xD0, 0xFB, 0x30, 0x11,
    // Name
    0x06, 0x09, 'C', '.', 'A', '.', 'T',
};
/* LISTING_END */
uint8_t adv_data_len = sizeof(adv_data);

//  1c 
//  02 01 02     
//  11 06 f5 1e 6b d5 2d 04 39 89 2a 42 61 6d d0 fb 30 11 
//  06 09 43 2e 41 2e 54 
//  0a 00 02


#include "log.h"
#include "bt_log.h"

// logging state
//static int log_event_packets = 1;
//static int log_data_packets = 1;
//static int log_send_packets = 1;



//--------------------------------------------------------------------------------
//
// BT Connection state
//

typedef enum {
  STATE_DETACHED,
  STATE_ATTACHED
} STATE;

static void DummyCallback(const void *data, UINT32 size, int_or_ptr_t arg) {
}


//
// HCI buffer  set by BTInit
//
static void *bt_buf;
static int bt_buf_size;

//
// BR/EDR connections
//
static uint8_t    rfcomm_channel_nr = 1;
static uint16_t   rfcomm_channel_id;

uint8_t set_rfcomm_channel_nr(uint8_t new_nr) {
  if ( rfcomm_channel_nr != new_nr )
    LogConn("CHANGED rfcomm_channel_nr  to: 0x%02x  from: 0x%02x", new_nr, rfcomm_channel_nr);
  rfcomm_channel_nr = new_nr;
  return rfcomm_channel_nr;
}
uint16_t set_rfcomm_channel_id(uint16_t new_id) {
  if ( rfcomm_channel_id != new_id )
    LogConn("CHANGED rfcomm_channel_id  to: 0x%04x  from: 0x%04x", new_id, rfcomm_channel_id);
  rfcomm_channel_id = new_id;
  return rfcomm_channel_id;
}

static uint8_t    spp_service_buffer[128] __attribute__((aligned(__alignof(service_record_item_t))));
static uint8_t    rfcomm_send_credit = 0;

//static char       local_name[] = "PIXEL (00:00)";  // the digits will be replaced by the MSB of the BD-ADDR
static char       local_name[] = "C.A.T (00:00)";  // the digits will be replaced by the MSB of the BD-ADDR

//
// Channel
//
static ChannelCallback client_callback;
static int_or_ptr_t client_callback_arg;

static STATE state;

STATE set_state(STATE new_state) {
  if ( state != new_state ) {
    if ( new_state )
      LogConn("state = STATE_ATTACHED");
    else
      LogConn("state = STATE_DETACHED");
  }
  state = new_state;
  return state;
}



//
// L2CAP and RFCOMM packet handlers
//

static uint8_t hexChar(uint8_t n) {
  if ( n < 10 )
    return ( n + '0' );
  else
    return ( n + 'A' );
}

static void PacketHandler(int l2cap_event, void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  if ( l2cap_event ) {
    printf("L2CAP   %d  ", size);
  } else {
    printf("RFCOMM  %d  ", size);
  }

  /*
  if ( l2cap_event ) {
    //LogConnPacket("L2CAP PacketHandler", packet, (int)size);
    printf("[conn] - L2CAP-handler  ");
    printf("channel: %04x  size: %d  type: %02x  packet: %02x %02x\n", channel, (int)size, packet_type, packet[0], packet[1]);
  } else {
    //LogConnPacket("RFCOMM PacketHandler", packet, (int)size);
    printf("[conn] - RFCOMM-handler ");
    printf("channel: %04x  size: %d  type: %02x  packet: %02x %02x\n", channel, (int)size, packet_type, packet[0], packet[1]);
  }
  */
  uint8_t rfcomm_channel_nr;
  uint16_t mtu;


  uint8_t event_code;
  uint8_t event_size;

  uint8_t event_status;
  uint16_t event_handle;
  bd_addr_t event_addr;
  uint8_t event_link_type;
  uint8_t event_encryption_enabled;

  switch (packet_type) {

  case HCI_EVENT_PACKET:
    event_code = packet[0];
    event_size = packet[1];

    switch ( event_code ) {

    case HCI_EVENT_INQUIRY_COMPLETE:                                // 0x01
      LogConn("PacketHandler - HCI connection complete  status: %d", (int)packet[2]);
      break;

    case HCI_EVENT_CONNECTION_COMPLETE:                             // 0x03
      // status(1)  connection_handle(2)  bd_addr(6)  link_type(1)  encryption_enabled(1)
      event_status = packet[2];                  // status: 1 octet
      event_handle = READ_BT_16(packet, 3);      // connection_handle: 2 octets
      bt_flip_addr(event_addr, &packet[5]);
      event_link_type = packet[12];
      event_encryption_enabled = packet[13];

      LogConn("PacketHandler - HCI connection complete  status: %d  handle: %u  addr: %s  link_type: %d  encryption_enabled: %d",
              event_status, event_handle, bd_addr_to_str(event_addr), event_link_type, event_encryption_enabled );
      break;
    case HCI_EVENT_CONNECTION_REQUEST:                              // 0x04
      LogConn("PacketHandler - HCI connection rquest");
      break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:                          // 0x05
      LogConn("PacketHandler - HCI disconnection complete");
      break;


    case BTSTACK_EVENT_STATE:
      // bt stack activated, get started - set local name
      if (packet[2] == HCI_STATE_WORKING) {
        LogConn("PacketHandler - HCI Working  write_local_name: %s", local_name);
        hci_send_cmd(&hci_write_local_name, local_name);
      }
      break;

    case HCI_EVENT_COMMAND_COMPLETE:
      // num_HCI_command_packets   1 octet
      // command_opcode            2 octets

      // return_parameters         depends on command_opcode

      //   command_opcode          2 octets
      //   param_num               1 octet
      //   parameters              param_num octets

      if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
        bt_flip_addr(event_addr, &packet[6]);
        //log_printf("BD-ADDR: %s", bd_addr_to_str(event_addr));
        LogConn("PacketHandler - BD-ADDR: %s", bd_addr_to_str(event_addr));

        //sprintf(local_name, "PIXEL (%02X:%02X)", event_addr[4], event_addr[5]);
        sprintf(local_name, "C.A.T (%02X:%02X)", event_addr[4], event_addr[5]);

        // modify LE database
        LogConn( "Update LE database   %02x %02x     %s", event_addr[4], event_addr[5], bd_addr_to_str(event_addr) );
        profile_data[38] = hexChar( ( event_addr[4] & 0xf0 ) >> 4 );
        profile_data[39] = hexChar( event_addr[4] & 0x0f );
        profile_data[41] = hexChar( ( event_addr[5] & 0xf0 ) >> 4 );
        profile_data[42] = hexChar( event_addr[5] & 0x0f );

        break;
      }
      if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
        LogConn("PacketHandler - Command Complete WRITE_LOCAL_NAME");

        hci_discoverable_control(1);
        break;
      }
      break;

    case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
      break;

    case HCI_EVENT_LINK_KEY_REQUEST:
      // deny link key request
      //log_printf("Link key request - deny");
      LogConn("PacketHandler - Link key request - deny");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_link_key_request_negative_reply, &event_addr);
      break;

    case HCI_EVENT_PIN_CODE_REQUEST:
      // inform about pin code request
      //log_printf("Pin code request - using '0000'");
      LogConn("PacketHandler - Pin code request - using '0000'");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
      break;

      //
      // L2CAP
      //
    case L2CAP_EVENT_SERVICE_REGISTERED:                            // 0x75
      LogConn("PacketHandler - L2CAP service registered");
      break;

      //
      // RFCOMM
      //
    case RFCOMM_EVENT_SERVICE_REGISTERED:                           // 0x85
      LogConn("PacketHandler - RFCOMM service registered");
      break;

    case RFCOMM_EVENT_INCOMING_CONNECTION:
      // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
      bt_flip_addr(event_addr, &packet[2]);
      //set_rfcomm_channel_nr( packet[8] );
      rfcomm_channel_nr = packet[8];
      set_rfcomm_channel_id( READ_BT_16(packet, 9) );
      //log_printf("RFCOMM channel %u requested for %s", rfcomm_channel_nr, bd_addr_to_str(event_addr));
      LogConn("PacketHandler - RFCOMM id: %u  channel %u requested for %s", rfcomm_channel_id, rfcomm_channel_nr, bd_addr_to_str(event_addr));
      rfcomm_accept_connection_internal(rfcomm_channel_id);
      break;

    case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
      // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
      if (packet[2]) {
        //log_printf("RFCOMM channel open failed, status %u", packet[2]);
        LogConn("PacketHandler - RFCOMM channel open failed, status %u", packet[2]);
      } else {
        set_rfcomm_channel_id( READ_BT_16(packet, 12) );
        rfcomm_send_credit = 1;
        mtu = READ_BT_16(packet, 14);
        //log_printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u", rfcomm_channel_id, mtu);
        LogConn("PacketHandler - RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u", rfcomm_channel_id, mtu);
      }
      break;

    case RFCOMM_EVENT_CHANNEL_CLOSED:
      //log_printf("RFCOMM channel closed.");
      LogConn("PacketHandler - RFCOMM channel close");

      // notify the main.c AppCallback that the connection has closed
      client_callback(NULL, 0, client_callback_arg);
      client_callback = DummyCallback;
      set_rfcomm_channel_id( 0 );
      break;


      //
      // DAEMON
      //
    case DAEMON_EVENT_HCI_PACKET_SENT:                              // 0x6C
      break;


    default:
      LogConn("PacketHandler - PacketHandler  event: %02X", packet[0]);
      break;
    }
    break;

  case RFCOMM_DATA_PACKET:
    // BR/EDR connection  pass incoming data to AppProtocol handler
    client_callback(packet, size, client_callback_arg);
    rfcomm_send_credit = 1;

  default:
    break;
  }
}

static void L2CAP_PacketHandler(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  PacketHandler( 1, connection, packet_type, channel, packet, size );
}

static void RFCOMM_PacketHandler(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  PacketHandler( 0, connection, packet_type, channel, packet, size );
}



static void BTInit(void *buf, int size) {
  LogConn("BTInit  size: %d", size);

  set_state(STATE_DETACHED);
  bt_buf = buf;
  bt_buf_size = size;
}


// hci_transport_mchpusb.c
hci_transport_t * hci_transport_mchpusb_instance(void *buf, int size);
void hci_transport_mchpusb_tasks();


//--------------------------------------------------------------------------------
//
// Packet Handling
//
static int ble_notification_enabled = 0;


#define HEARTBEAT_PERIOD_MS 1000

static timer_source_t heartbeat;
static int  counter = 0;
//static char counter_string[30];
//static int  counter_string_len;

static void  heartbeat_handler(struct timer *ts) {
  LogConn("heartbeat %d", counter);
  counter++;

  run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
  run_loop_add_timer(ts);
}

static void StartHeartbeat(void) {
  // set one-shot timer
  heartbeat.process = &heartbeat_handler;
  run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
  run_loop_add_timer(&heartbeat);
}


// 
// ATT Server packet handler
//
//   Disconnect / Connect / MTU
//
static int ble_connected = 0;
static int ble_mtu = 0;

static uint16_t ble_conn_handle;

static void ble_connect(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  ble_conn_handle = READ_BT_16(packet, 4);
  LogConn("BLE connected %u", ble_conn_handle);
  ble_connected = 1;
}


static void ble_disconnect(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  if ( ble_connected ) {
    LogConn("BLE disconect");
    ble_connected = 0;
    ble_notification_enabled = 0;

    // notify the main.c AppCallback that the connection has closed
    client_callback(NULL, 0, client_callback_arg);
    client_callback = DummyCallback;
  }
}


// Events
#define HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE         0x22
#define HCI_EVENT_FLOW_SPECIFICATION_COMPLETE              0x22
#define HCI_EVENT_SUPERVISION_TIMEOUT_CHANGED              0x38

/*
const char *event_names[256] = {
  NULL,                                                    // 0x00
  "HCI_EVENT_INQUIRY_COMPLETE",                            // 0x01
  "HCI_EVENT_INQUIRY_RESULT",                              // 0x02
  "HCI_EVENT_CONNECTION_COMPLETE",                         // 0x03
  "HCI_EVENT_CONNECTION_REQUEST",                          // 0x04
  "HCI_EVENT_DISCONNECTION_COMPLETE",                      // 0x05
  "HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT",               // 0x06
  "HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE",                // 0x07
  "HCI_EVENT_ENCRYPTION_CHANGE",                           // 0x08
  "HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE",         // 0x09
  "HCI_EVENT_MASTER_LINK_KEY_COMPLETE",                    // 0x0A
  "HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE",     // 0x0B
  "HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE",    // 0x0C
  "HCI_EVENT_QOS_SETUP_COMPLETE",                          // 0x0D
  "HCI_EVENT_COMMAND_COMPLETE",                            // 0x0E
  "HCI_EVENT_COMMAND_STATUS",                              // 0x0F
  "HCI_EVENT_HARDWARE_ERROR",                              // 0x10
  "HCI_EVENT_FLUSH_OCCURED",                               // 0x11
  "HCI_EVENT_ROLE_CHANGE",                                 // 0x12
  "HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS",                 // 0x13
  "HCI_EVENT_MODE_CHANGE_EVENT",                           // 0x14
  "HCI_EVENT_RETURN_LINK_KEYS",                            // 0x15
  "HCI_EVENT_PIN_CODE_REQUEST",                            // 0x16
  "HCI_EVENT_LINK_KEY_REQUEST",                            // 0x17
  "HCI_EVENT_LINK_KEY_NOTIFICATION",                       // 0x18
  NULL,                                                    // 0x19
  "HCI_EVENT_DATA_BUFFER_OVERFLOW",                        // 0x1A
  "HCI_EVENT_MAX_SLOTS_CHANGED",                           // 0x1B
  "HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE",                  // 0x1C
  "HCI_EVENT_PACKET_TYPE_CHANGED",                         // 0x1D
  NULL,                                                    // 0x1E  QoS Violation
  NULL,                                                    // 0x1F
  "HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE",            // 0x20
  "HCI_EVENT_FLOW_SPECIFICATION_COMPLETE",                 // 0x21
  "HCI_EVENT_INQUIRY_RESULT_WITH_RSSI",                    // 0x22
  NULL,  // 0x23  "HCI_EVENT_INQUIRY_READ_REMOTE_EXTENDED_FEATURES_COMPLETE",
  NULL,  // 0x24
  NULL,  // 0x25
  NULL,  // 0x26
  NULL,  // 0x27
  NULL,  // 0x28
  NULL,  // 0x29
  NULL,  // 0x2A
  NULL,  // 0x2B
  "HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE",
  NULL,  // 0x2D
  NULL,  // 0x2E
  "HCI_EVENT_EXTENDED_INQUIRY_RESPONSE",
  NULL,  // 0x30
  "HCI_EVENT_IO_CAPABILITY_REQUEST",
  "HCI_EVENT_IO_CAPABILITY_RESPONSE",
  "HCI_EVENT_USER_CONFIRMATION_REQUEST",
  "HCI_EVENT_USER_PASSKEY_REQUEST",
  "HCI_EVENT_REMOTE_OOB_DATA_REQUEST",
  "HCI_EVENT_SIMPLE_PAIRING_COMPLETE",
  NULL,  // 0x37
  "HCI_EVENT_SUPERVISION_TIMEOUT_CHANGED",
  NULL,  // 0x39
  NULL,  // 0x3A
  NULL,  // 0x3B
  NULL,  // 0x3C
  NULL,  // 0x3D
  "HCI_EVENT_LE_META",
  NULL,  // 0x3F
  NULL,  // 0x40
  NULL,  // 0x41
  NULL,  // 0x42
  NULL,  // 0x43
  NULL,  // 0x44
  NULL,  // 0x45
  NULL,  // 0x46
  NULL,  // 0x47
  NULL,  // 0x48
  NULL,  // 0x49
  NULL,  // 0x4A
  NULL,  // 0x4B
  NULL,  // 0x4C
  NULL,  // 0x4D
  NULL,  // 0x4E
  NULL,  // 0x4F
  NULL,  // 0x50
  NULL,  // 0x51
  NULL,  // 0x52
  NULL,  // 0x53
  NULL,  // 0x54
  NULL,  // 0x55
  NULL,  // 0x56
  NULL,  // 0x57
  NULL,  // 0x58
  NULL,  // 0x59
  NULL,  // 0x5A
  NULL,  // 0x5B
  NULL,  // 0x5C
  NULL,  // 0x5D
  NULL,  // 0x5E
  NULL,  // 0x5F
  "BTSTACK_EVENT_STATE",
  "BTSTACK_EVENT_NR_CONNECTIONS_CHANGED",
  "BTSTACK_EVENT_POWERON_FAILED",
  "BTSTACK_EVENT_VERSION",
  "BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED",
  "BTSTACK_EVENT_REMOTE_NAME_CACHED",
  "BTSTACK_EVENT_DISCOVERABLE_ENABLED",
  NULL,  // 0x67
  "DAEMON_EVENT_CONNECTION_OPENED",
  "DAEMON_EVENT_CONNECTION_CLOSED",
  "DAEMON_NR_CONNECTIONS_CHANGED",
  "DAEMON_EVENT_NEW_RFCOMM_CREDITS",
  "DAEMON_EVENT_HCI_PACKET_SENT",
  NULL,  // 0x6D
  NULL,  // 0x6E
  NULL,  // 0x6F
  "L2CAP_EVENT_CHANNEL_OPENED",
  "L2CAP_EVENT_CHANNEL_CLOSED",
  "L2CAP_EVENT_INCOMING_CONNECTION",
  "L2CAP_EVENT_TIMEOUT_CHECK",
  "L2CAP_EVENT_CREDITS",
  "L2CAP_EVENT_SERVICE_REGISTERED",
  "L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST",
  "L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE",
  NULL,  // 0x78
  NULL,  // 0x79
  NULL,  // 0x7A
  NULL,  // 0x7B
  NULL,  // 0x7C
  NULL,  // 0x7D
  NULL,  // 0x7E
  NULL,  // 0x7F
  "RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE",
  "RFCOMM_EVENT_CHANNEL_CLOSED",
  "RFCOMM_EVENT_INCOMING_CONNECTION",
  "RFCOMM_EVENT_REMOTE_LINE_STATUS",
  "RFCOMM_EVENT_CREDITS",
  "RFCOMM_EVENT_SERVICE_REGISTERED",
  "RFCOMM_EVENT_PERSISTENT_CHANNEL",
  "RFCOMM_EVENT_REMOTE_MODEM_STATUS",
  "RFCOMM_EVENT_PORT_CONFIGURATION",
  NULL,  // 0x89
  NULL,  // 0x8A
  NULL,  // 0x8B
  NULL,  // 0x8C
  NULL,  // 0x8D
  NULL,  // 0x8E
  NULL,  // 0x8F
  "SDP_SERVICE_REGISTERED",
  "SDP_QUERY_COMPLETE",
  "SDP_QUERY_RFCOMM_SERVICE",
  "SDP_QUERY_ATTRIBUTE_VALUE",
  "SDP_QUERY_SERVICE_RECORD_HANDLE",
  NULL,  // 0x95
  NULL,  // 0x96
  NULL,  // 0x97
  NULL,  // 0x98
  NULL,  // 0x99
  NULL,  // 0x9A
  NULL,  // 0x9B
  NULL,  // 0x9C
  NULL,  // 0x9D
  NULL,  // 0x9E
  NULL,  // 0x9F
  "GATT_QUERY_COMPLETE",
  "GATT_SERVICE_QUERY_RESULT",
  "GATT_CHARACTERISTIC_QUERY_RESULT",
  "GATT_INCLUDED_SERVICE_QUERY_RESULT",
  "GATT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT",
  "GATT_CHARACTERISTIC_VALUE_QUERY_RESULT",
  "GATT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT",
  "GATT_NOTIFICATION",
  "GATT_INDICATION",
  "GATT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT",
  "GATT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT",
  "GATT_MTU",
  NULL,  // 0xAC
  NULL,  // 0xAD
  NULL,  // 0xAE
  NULL,  // 0xAF
  NULL,  // 0xB0
  NULL,  // 0xB1
  NULL,  // 0xB2
  NULL,  // 0xB3
  NULL,  // 0xB4
  "ATT_MTU_EXCHANGE_COMPLETE",
  "ATT_HANDLE_VALUE_INDICATION_COMPLETE",
  NULL,  // 0xB7
  NULL,  // 0xB8
  NULL,  // 0xB9
  NULL,  // 0xBA
  NULL,  // 0xBB
  NULL,  // 0xBC
  NULL,  // 0xBD
  NULL,  // 0xBE
  NULL,  // 0xBF
  "BNEP_EVENT_SERVICE_REGISTERED",
  "BNEP_EVENT_OPEN_CHANNEL_COMPLETE",
  "BNEP_EVENT_INCOMING_CONNECTION",
  "BNEP_EVENT_CHANNEL_CLOSED",
  "BNEP_EVENT_CHANNEL_TIMEOUT",
  "BNEP_EVENT_READY_TO_SEND",
  NULL,  // 0xC6
  NULL,  // 0xC7
  NULL,  // 0xC8
  NULL,  // 0xC9
  NULL,  // 0xCA
  NULL,  // 0xCB
  NULL,  // 0xCC
  NULL,  // 0xCD
  NULL,  // 0xCE
  NULL,  // 0xCF
  "SM_JUST_WORKS_REQUEST",
  "SM_JUST_WORKS_CANCEL",
  "SM_PASSKEY_DISPLAY_NUMBER",
  "SM_PASSKEY_DISPLAY_CANCEL",
  "SM_PASSKEY_INPUT_NUMBER",
  "SM_PASSKEY_INPUT_CANCEL",
  "SM_IDENTITY_RESOLVING_STARTED",
  "SM_IDENTITY_RESOLVING_FAILED",
  "SM_IDENTITY_RESOLVING_SUCCEEDED",
  "SM_AUTHORIZATION_REQUEST",
  "SM_AUTHORIZATION_RESULT",
  NULL,  // 0xDB
  NULL,  // 0xDC
  NULL,  // 0xDD
  NULL,  // 0xDE
  NULL,  // 0xDF
  "GAP_SECURITY_LEVEL",
  "GAP_DEDICATED_BONDING_COMPLETED",
  "GAP_LE_ADVERTISING_REPORT",
  NULL,  // 0xE3
  NULL,  // 0xE4
  NULL,  // 0xE5
  NULL,  // 0xE6
  NULL,  // 0xE7
  "HCI_EVENT_HSP_META",
  "HCI_EVENT_HFP_META",
  NULL,  // 0xEA
  NULL,  // 0xEB
  NULL,  // 0xEC
  NULL,  // 0xED
  NULL,  // 0xEE
  NULL,  // 0xEF
  "ANCS_CLIENT_CONNECTED",
  "ANCS_CLIENT_NOTIFICATION",
  "ANCS_CLIENT_DISCONNECTED",
  NULL,  // 0xF3
  NULL,  // 0xF4
  NULL,  // 0xF5
  NULL,  // 0xF6
  NULL,  // 0xF7
  NULL,  // 0xF8
  NULL,  // 0xF9
  NULL,  // 0xFA
  NULL,  // 0xFB
  NULL,  // 0xFC
  NULL,  // 0xFD
  NULL,  // 0xFE
  "HCI_EVENT_VENDOR_SPECIFIC",
};
*/

static void att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  uint8_t event_code = 0;
  uint8_t event_size = 0;
  //  const char *name = NULL;
  uint8_t le_code;

  switch (packet_type) {

  case HCI_EVENT_PACKET:
    event_code = packet[0];
    event_size = packet[1];
    //    name = event_names[event_code];

    switch (packet[0]) {

    case HCI_EVENT_CONNECTION_COMPLETE:                             // 0x03
      LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_CONNECTION_COMPLETE", channel, size);
      break;

    case HCI_EVENT_CONNECTION_REQUEST:                              // 0x04
      LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_CONNECTION_REQUEST", channel, size);
      break;

    case HCI_EVENT_DISCONNECTION_COMPLETE:                          // 0x05
      LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_DISCONNECTION_COMPLETE", channel, size);
      // BLE disconenct
      ble_disconnect(packet_type, channel, packet, size);
      break;

    case HCI_EVENT_COMMAND_COMPLETE:                                // 0x0E
      //      LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_COMMAND_COMPLETE  %02x %02x", channel, size, packet[1], packet[2]);
      break;

    case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
      break;

      // LE Meta
    case HCI_EVENT_LE_META:                                         // 0x3E
      le_code = packet[2];

      switch ( le_code ) {
      case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
        LogConn("att_packet_handler  channel: %u  size: %d  BLE connect", channel, size);
        // BLE conenct
        ble_connect(packet_type, channel, packet, size);
        break;

        //      case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:                     // 0x01
        //      case HCI_SUBEVENT_LE_ADVERTISING_REPORT:                      // 0x02
        //      case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:              // 0x03
        //      case HCI_SUBEVENT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:      // 0x04
        //      case HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST:                   // 0x05
        //        break;

      default:
        LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_LE_META  le_code: %d", channel, size, (int)le_code);
        break;
      }
      break;  

      // ATT
    case ATT_MTU_EXCHANGE_COMPLETE:                                 // 0xB5
      ble_mtu = READ_BT_16(packet, 4) - 3;
      LogConn("att_packet_handler  channel: %u  size: %d  ATT_MTU_EXCHANGE_COMPLETE  mtu: %d", channel, size, ble_mtu);
      break;



    case BTSTACK_EVENT_STATE:                                       // 0x60
      LogConn("att_packet_handler  channel: %u  size: %d  BTSTACK_EVENT_STATE  state: %d", channel, size, (int)packet[2]);
      break;  
    case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:                      // 0x61
      LogConn("att_packet_handler  channel: %u  size: %d  HCI_EVENT_NR_CONNECTIONS_CHANGED  number: %d", channel, size, (int)packet[2]);
      break;
    case BTSTACK_EVENT_DISCOVERABLE_ENABLED:                        // 0x66
      LogConn("att_packet_handler  channel: %u  size: %d  BTSTACK_EVENT_DISCOVERABLE_ENABLED  enabled: %d", channel, size, (int)packet[2]);
      break;  

    case DAEMON_EVENT_HCI_PACKET_SENT:                              // 0x6C
      //      LogConn("att_packet_handler  channel: %u  size: %d  DAEMON_EVENT_HCI_PACKET_SENT  %02x", channel, size, packet[1]);
      break;  



      // 
    default:
      //      if ( name != NULL )
      //        LogConn("att_packet_handler  channel: %u  size: %d  %s", channel, size, name);
      //      else
      LogConn("att_packet_handler  channel: %u  size: %d  event: %02X", channel, size, event_code);
      break;
    }
    break;

    // other packet types
  default:
    LogConn("att_packet_handler  channel: %u  packet_type: %d  size: %d", channel, (int)packet_type, size);    
  }
}


// Send data to the central
static uint16_t att_send(uint8_t *buffer, uint16_t buffer_size) {
  LogConn("Send AppProtocol packet to phone   size: %d", (int)buffer_size);

  if ( ble_notification_enabled ) {
    att_server_notify(ATT_CHARACTERISTIC_1130FBD1_6D61_422A_8939_042DD56B1EF5_01_VALUE_HANDLE, buffer, buffer_size);
  }

  return buffer_size;
}


// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(uint16_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
  LogConn("att_read_callback  size: %d", (int)buffer_size);

  /*
  // handle read value
  if (att_handle == ATT_CHARACTERISTIC_1130FBD1_6D61_422A_8939_042DD56B1EF5_01_VALUE_HANDLE) {
    //LogConn("att_read   offset: %u  size: %u", (unsigned int)offset, (unsigned int)buffer_size );
    if ( buffer ) {
      //memcpy(buffer, &counter_string[offset], counter_string_len - offset);
    }

    // return number of bytes
    return counter_string_len - offset;
  }
  */
  return 0;
}

//
// ATT Client Write Callback for Dynamic Data
//
static int att_write_callback(uint16_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){

  // handle write value
  if ( att_handle == ATT_CHARACTERISTIC_1130FBD1_6D61_422A_8939_042DD56B1EF5_01_VALUE_HANDLE ) {
    //LogConn("att_write  mode: %u  offset: %u  size: %u", (unsigned int)transaction_mode, (unsigned int)offset, (unsigned int)buffer_size );

    if ( buffer_size > 0 ) {
      //LogConn("Send to AppProtocolHandleIncoming   size: %d", (int)buffer_size);

      // Send the packet data to the App protocol handler
      client_callback(&buffer[2], ( buffer_size - 2 ), client_callback_arg);
    }
  }

  // handle notification enable / disable
  if ( att_handle == ATT_CHARACTERISTIC_1130FBD1_6D61_422A_8939_042DD56B1EF5_01_CLIENT_CONFIGURATION_HANDLE ) {
    LogConn("  ble_notification_enabled  %d", ( READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION ) );

    ble_notification_enabled = READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
  }

  return 0;
}



void sm_init2(void (*handler)(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size));


static void BTAttached() {
  LogConn("BTAttached()");

  btstack_memory_init();

  // KAL -
  run_loop_init(RUN_LOOP_EMBEDDED);

  // init HCI
  hci_transport_t * transport = hci_transport_mchpusb_instance(bt_buf, bt_buf_size);
  bt_control_t * control = NULL;
  hci_uart_config_t * config = NULL;
  const remote_device_db_t * remote_db = &remote_device_db_memory;
  hci_init(transport, config, control, remote_db);
  //pinless bt setting is here, set to 1 for pinless but problem is doesn't keep pairing on Android, only good for demo mode
  hci_ssp_set_enable(0);  //issue with this is that android device will not lose bt pairing code on device reboot, didn't test Rasp Pi

  // init L2CAP
  l2cap_init();
  //  l2cap_register_packet_handler(PacketHandler);
  l2cap_register_packet_handler(L2CAP_PacketHandler);

  // init RFCOMM
  rfcomm_init();
  //  rfcomm_register_packet_handler(PacketHandler);
  rfcomm_register_packet_handler(RFCOMM_PacketHandler);
  rfcomm_register_service_internal(NULL, rfcomm_channel_nr, 100); // reserved channel, mtu=100

  // init SDP, create record for SPP and register with SDP
  sdp_init();

  memset(spp_service_buffer, 0, sizeof (spp_service_buffer));
  service_record_item_t * service_record_item = (service_record_item_t *) spp_service_buffer;
  sdp_create_spp_service((uint8_t*) & service_record_item->service_record, 1, "IOIO-App");
  log_printf("SDP service buffer size: %u\n\r", (uint16_t) (sizeof (service_record_item_t) + de_get_len((uint8_t*) & service_record_item->service_record)));
  sdp_register_service_internal(NULL, service_record_item);


  //  LogConn("  setup heartbeat");
  StartHeartbeat();



  // BLE - setup
  //    hci_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);

  //  LogConn("  CALL le_device_db_init()");

  // setup le device db
  le_device_db_init();

  // setup SM: Display only
  sm_init2(L2CAP_PacketHandler);

  //LogConn("  CALL att init 0x%04x", profile_data);
  //  LogConn("  CALL att init\n");

  // setup ATT server
  att_server_init(profile_data, att_read_callback, att_write_callback);    
  att_server_register_packet_handler(att_packet_handler);


  //  LogConn("  POST att init");
  //  LogConn("");


  //  LogConn("  setup adv");
  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  
  //  LogConn("  setup adv params");
  
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
  
  //  LogConn("  enable adv");
  gap_advertisements_enable(1);

  // BLE - setup  DONE


  //  LogConn("  TURN ON");
  hci_power_control(HCI_POWER_ON);

  client_callback = DummyCallback;

  LogConn("BTAttached() - DONE");
}

static BOOL last_device_attached;

static void BTTasks() {
  BOOL device_attached = USBHostBluetoothIsDeviceAttached();
  if ( last_device_attached != device_attached )
    LogConn("BTTasks  state: %d    device attached: %d", state, device_attached);
  last_device_attached = device_attached;

  switch (state) {
    case STATE_DETACHED:
      // if (USBHostBluetoothIsDeviceAttached()) {
      if (device_attached) {
        BTAttached();
        set_state(STATE_ATTACHED);
      }
      break;

    case STATE_ATTACHED:
      // if (USBHostBluetoothIsDeviceAttached()) {
      if (device_attached) {
        hci_transport_mchpusb_tasks();

        if (rfcomm_channel_id && rfcomm_send_credit) {
          //LogConn("BTTasks  grant credits");
          rfcomm_grant_credits(rfcomm_channel_id, 1);
          rfcomm_send_credit = 0;
        }
      } else {
        // Detached. We don't care about the state of btstack, since we're not
        // going to give it any context, and we'll reset it the next time a
        // dongle is attached. Just close the channel if it is open.
        log_printf("Bluetooth detached.");
        client_callback(NULL, 1, client_callback_arg);
        client_callback = DummyCallback;
        set_rfcomm_channel_id(0);
        set_state(STATE_DETACHED);
       }
  }
}

static int last_ready = 0;

static int BTIsReadyToOpen() {
  int br_ready = ( rfcomm_channel_id != 0 ) && ( client_callback == DummyCallback );
  int le_ready = ( ble_connected && ble_notification_enabled );

  int ready = ( br_ready | le_ready );
  if ( last_ready != ready ) {
    LogConn("BTIsReadyTopOpen()    rfcomm_channel_id: 0x%04x  dummy_callback: %d  BLE connected: %d  Notification Enabled: %d",
            rfcomm_channel_id, ( client_callback == DummyCallback ),
            ble_connected, ble_notification_enabled );
    LogConn("  ready changed  from: %d  to: %d", last_ready, ready );
  }
  last_ready = ready;

  return ready;

  // return rfcomm_channel_id != 0 && client_callback == DummyCallback;
  //  return ( client_callback == DummyCallback );
}

static int BTOpen(ChannelCallback cb, int_or_ptr_t open_arg, int_or_ptr_t cb_args) {
  log_printf("BTOpen()");
  LogConn("BTOpen()");

  client_callback = cb;
  client_callback_arg = cb_args;
  return 0;
}

static void BTSend(int h, const void *data, int size) {
  LogConn("BTSend  handle: 0x%04x  size: %d", h, size);

  assert(!(size >> 16));
  assert(h == 0);

  // if we have a BLE connection then call att_send
  if ( ble_notification_enabled ) {
    att_send( (uint8_t *)data, (size & 0xFFFF) );
  } else {
    // send to the BR/EDR connection
    rfcomm_send_internal(rfcomm_channel_id, (uint8_t *) data, size & 0xFFFF);
  }
}

static int last_rfcomm_can_send = 0;

static int BTCanSend(int h) {
  //  LogConn("BTCanSend  handle: 0x%04x", h);

  assert(h == 0);
  // return rfcomm_can_send(rfcomm_channel_id);

  if ( ble_connected ) {
    if ( ble_notification_enabled ) {
      return 1;
    }
  } else {
    if ( rfcomm_channel_id != 0 ) {
      int res = rfcomm_can_send_packet_now(rfcomm_channel_id);
      if ( res != last_rfcomm_can_send )
        LogConn("BTCanSend  changed  from: %d  to: %d   RFCOMM handle: 0x%04x", last_rfcomm_can_send, res, h);
      last_rfcomm_can_send = res;
      return res;
    }
  }
  return 0;
}

static void BTClose(int h) {
  LogConn("BTClose  handle: 0x%04x", h);
  assert(h == 0);
  rfcomm_disconnect_internal(rfcomm_channel_id);
}

static int usb_attached = 0;

static int BTIsAvailable() {
  //  LogConn("BTIsAvailable()");

  int attached = USBHostBluetoothIsDeviceAttached();
  if ( usb_attached != attached )
    LogConn("BTIsAvailable()  device attached: %d", attached);
  usb_attached = attached;
  return usb_attached;
}

static int BTMaxPacketSize(int h) {
  LogConn("BTMaxPacketSize  handle: 0x%04x", h);

  //  if ( ble_connected )
  //    return ble_mtu;

  return 242;  // TODO: 244?
}

const CONNECTION_FACTORY bt_connection_factory = {
  BTInit,
  BTTasks,
  BTIsAvailable,
  BTIsReadyToOpen,
  BTOpen,
  BTClose,
  BTSend,
  BTCanSend,
  BTMaxPacketSize
};
