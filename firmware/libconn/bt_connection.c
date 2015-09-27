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

  bd_addr_t event_addr;
  uint8_t rfcomm_channel_nr;
  uint16_t mtu;

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    switch (packet[0]) {
    case BTSTACK_EVENT_STATE:
      // bt stack activated, get started - set local name
      if (packet[2] == HCI_STATE_WORKING) {
        LogConn("HCI Working  write_local_name: %s", local_name);
        hci_send_cmd(&hci_write_local_name, local_name);
      }
      break;

    case HCI_EVENT_COMMAND_COMPLETE:
      if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
        bt_flip_addr(event_addr, &packet[6]);
        log_printf("BD-ADDR: %s", bd_addr_to_str(event_addr));

        // LogConn("BD-ADDR: %s", bd_addr_to_str(event_addr));

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
        LogConn("Command Complete WRITE_LOCAL_NAME");

        hci_discoverable_control(1);
        break;
      }
      break;

    case HCI_EVENT_LINK_KEY_REQUEST:
      // deny link key request
      log_printf("Link key request - deny");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_link_key_request_negative_reply, &event_addr);
      break;

    case HCI_EVENT_PIN_CODE_REQUEST:
      // inform about pin code request
      log_printf("Pin code request - using '0000'");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
      break;

    case RFCOMM_EVENT_INCOMING_CONNECTION:
      // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
      bt_flip_addr(event_addr, &packet[2]);
      //set_rfcomm_channel_nr( packet[8] );
      rfcomm_channel_nr = packet[8];
      set_rfcomm_channel_id( READ_BT_16(packet, 9) );
      log_printf("RFCOMM channel %u requested for %s", rfcomm_channel_nr, bd_addr_to_str(event_addr));
      //LogConn("RFCOMM channel_nr: 0x%02x  channel_id: 0x%04x  requested for %s", rfcomm_channel_nr, rfcomm_channel_id, bd_addr_to_str(event_addr));
      rfcomm_accept_connection_internal(rfcomm_channel_id);
      break;

    case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
      // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
      if (packet[2]) {
        log_printf("RFCOMM channel open failed, status %u", packet[2]);
      } else {
        set_rfcomm_channel_id( READ_BT_16(packet, 12) );
        rfcomm_send_credit = 1;
        mtu = READ_BT_16(packet, 14);
        log_printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u", rfcomm_channel_id, mtu);
      }
      break;

    case RFCOMM_EVENT_CHANNEL_CLOSED:
      log_printf("RFCOMM channel closed.");
      client_callback(NULL, 0, client_callback_arg);
      client_callback = DummyCallback;
      set_rfcomm_channel_id( 0 );
      break;

    default:
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
  LogConn("BTInit  size: %d\n", size);

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
  LogConn("BLE connected %04x", ble_conn_handle);
  ble_connected = 1;
}

void AppProtocolDisconnect();

static void ble_disconnect(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  LogConn("BLE disconect");
  ble_connected = 0;
  ble_notification_enabled = 0;

  // Tell the channel the device has disconnected
  AppProtocolDisconnect();
}


static void att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

  switch (packet_type) {
  case HCI_EVENT_PACKET:
    //    LogConn("ATT_packet_handler  type: %02x  channel: %04x  packet[0]: %02x  size: %d", packet_type, channel, packet[0], (int)size);

    switch (packet[0]) {
      // #define HCI_EVENT_DISCONNECTION_COMPLETE                   0x05
    case HCI_EVENT_DISCONNECTION_COMPLETE:
      // BLE disconenct
      ble_disconnect(packet_type, channel, packet, size);
      break;

      // #define HCI_EVENT_LE_META                                  0x3E
    case HCI_EVENT_LE_META:
      // #define HCI_SUBEVENT_LE_CONNECTION_COMPLETE                0x01
      // #define HCI_SUBEVENT_LE_ADVERTISING_REPORT                 0x02
      // #define HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE         0x03
      // #define HCI_SUBEVENT_LE_READ_REMOTE_USED_FEATURES_COMPLETE 0x04
      // #define HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST              0x05
      LogConn("  LE_META   packet[2]: %02x", packet[2]);

      switch (packet[2]) {
      case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
        // BLE conenct
        ble_connect(packet_type, channel, packet, size);
        break;
      }
      break;  

      // #define ATT_MTU_EXCHANGE_COMPLETE                          0xB5
    case ATT_MTU_EXCHANGE_COMPLETE:
      ble_mtu = READ_BT_16(packet, 4) - 3;
      LogConn("BLE att mtu = %d", ble_mtu);
      break;
    }
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
      LogConn("Send to AppProtocolHandleIncoming   size: %d", (int)buffer_size);

      // Send the packet data to the App protocol handler
      client_callback(&buffer[2], ( buffer_size - 2 ), client_callback_arg);
    }
  }

  // handle notification enable / disable
  if ( att_handle == ATT_CHARACTERISTIC_1130FBD1_6D61_422A_8939_042DD56B1EF5_01_CLIENT_CONFIGURATION_HANDLE ) {
    int le_not = READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    LogConn("  ble_notification_enabled  %d", le_not);

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
          LogConn("BTTasks  grant credits");
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
      LogConn("BTCanSend  RFCOMM handle: 0x%04x", h);
      return rfcomm_can_send_packet_now(rfcomm_channel_id);
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
