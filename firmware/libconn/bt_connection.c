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
    0x11, 0x06, 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x10, 0xFF, 0x00, 0x00, 
    // Name
    0x04, 0x09, 'C', 'A', 'T',
};
/* LISTING_END */
uint8_t adv_data_len = sizeof(adv_data);




#include "log.h"
#include "bt_log.h"

// logging state
//static int log_event_packets = 1;
//static int log_data_packets = 1;
//static int log_send_packets = 1;



typedef enum {
  STATE_DETACHED,
  STATE_ATTACHED
} STATE;

static void DummyCallback(const void *data, UINT32 size, int_or_ptr_t arg) {
}

static uint8_t    rfcomm_channel_nr = 1;
static uint16_t   rfcomm_channel_id;
static uint8_t    spp_service_buffer[128] __attribute__((aligned(__alignof(service_record_item_t))));
static uint8_t    rfcomm_send_credit = 0;
static ChannelCallback client_callback;
static int_or_ptr_t client_callback_arg;
static char       local_name[] = "PIXEL (00:00)";  // the digits will be replaced by the MSB of the BD-ADDR
static STATE state;
static void *bt_buf;
static int bt_buf_size;

static void PacketHandler(int l2cap_event, void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
  /*
  if ( l2cap_event ) {
    LogConnPacket("L2CAP PacketHandler", packet, (int)size);
  } else {
    LogConnPacket("RFCOMM PacketHandler", packet, (int)size);
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
            hci_send_cmd(&hci_write_local_name, local_name);
          }
          break;

        case HCI_EVENT_COMMAND_COMPLETE:
          if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
            bt_flip_addr(event_addr, &packet[6]);
            log_printf("BD-ADDR: %s", bd_addr_to_str(event_addr));
            sprintf(local_name, "PIXEL (%02X:%02X)", event_addr[4], event_addr[5]);
            break;
          }
          if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
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
          rfcomm_channel_nr = packet[8];
          rfcomm_channel_id = READ_BT_16(packet, 9);
          log_printf("RFCOMM channel %u requested for %s", rfcomm_channel_nr, bd_addr_to_str(event_addr));
          rfcomm_accept_connection_internal(rfcomm_channel_id);
          break;

        case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
          // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
          if (packet[2]) {
            log_printf("RFCOMM channel open failed, status %u", packet[2]);
          } else {
            rfcomm_channel_id = READ_BT_16(packet, 12);
            rfcomm_send_credit = 1;
            mtu = READ_BT_16(packet, 14);
            log_printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u", rfcomm_channel_id, mtu);
          }
          break;

        case RFCOMM_EVENT_CHANNEL_CLOSED:
          log_printf("RFCOMM channel closed.");
          client_callback(NULL, 0, client_callback_arg);
          client_callback = DummyCallback;
          rfcomm_channel_id = 0;
          break;

        default:
          break;
      }
      break;

    case RFCOMM_DATA_PACKET:
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
  LogConn("BTInit( %p, %d )", buf, size);

  state = STATE_DETACHED;
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
static int le_notification_enabled = 0;


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



// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(uint16_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
  //LedSetFlag(0, 18, GREEN);
  //  LogConn("att_read_callback");
  /*
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE){
        if (buffer){
            memcpy(buffer, &counter_string[offset], counter_string_len - offset);
        }
        return counter_string_len - offset;
    }
  */
    return 0;
}

// write requests
static int att_write_callback(uint16_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
  //LedSetFlag(0, 19, GREEN);


  if ( att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE ) {
    //LogConn("att_write  mode: %u  offset: %u  size: %u", (unsigned int)transaction_mode, (unsigned int)offset, (unsigned int)buffer_size );
    //    LogConn("att_write  mode: %u  offset: %u  size: %u", (unsigned int)transaction_mode, (unsigned int)offset, (unsigned int)buffer_size );

    if ( buffer_size > 0 ) {
      //if ( packet_received(offset, buffer, buffer_size) ) {
        // Send the packet data to the App protocol handler
      //      LogConn("  forward packet to protocol");

      client_callback(&buffer[2], ( buffer_size - 2 ), client_callback_arg);
    }

  /*
    // printf("WRITE Callback, handle %04x, mode %u, offset %u, data: ", handle, transaction_mode, offset);
    // printf_hexdump(buffer, buffer_size);
    if (att_handle != ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE) return 0;
    le_notification_enabled = READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
  */
  }

  // handle notification enable / disable
  if ( att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE ) {
    int le_not = READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    LogConn("  le_notification_enabled  %d", le_not);

    le_notification_enabled = READ_BT_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
  }
  return 0;
}



void sm_init2(void (*handler)(void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size));


static void BTAttached() {
  LogConn("");
  LogConn("BTAttach()");

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


  LogConn("  setup heartbeat");
  StartHeartbeat();



  // BLE - setup
  //    hci_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);

  LogConn("  CALL le_device_db_init()");

  // setup le device db
  le_device_db_init();

  // setup SM: Display only
  sm_init2(L2CAP_PacketHandler);

  LogConn("  CALL att init 0x%04x", profile_data);

  // setup ATT server
  att_server_init(profile_data, att_read_callback, att_write_callback);    

  LogConn("  POST att init");
  LogConn("");


  LogConn("  setup adv");
  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  
  LogConn("  setup adv params");
  
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
  
  LogConn("  enable adv");
  gap_advertisements_enable(1);

  // BLE - setup  DONE


  LogConn("  TURN ON");
  hci_power_control(HCI_POWER_ON);

  client_callback = DummyCallback;

  LogConn("BTAttach() - DONE");
  LogConn("");
}


static void BTTasks() {
  switch (state) {
    case STATE_DETACHED:
      if (USBHostBluetoothIsDeviceAttached()) {
        BTAttached();
        state = STATE_ATTACHED;
      }
      break;

    case STATE_ATTACHED:
      if (USBHostBluetoothIsDeviceAttached()) {
        hci_transport_mchpusb_tasks();

        if (rfcomm_channel_id && rfcomm_send_credit) {
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
        rfcomm_channel_id = 0;
        state = STATE_DETACHED;
       }
  }
}

static int BTIsReadyToOpen() {
  LogConn("BTIsReadyToOpen()  rfcid 0x%04x  %d", rfcomm_channel_id, ( client_callback == DummyCallback ) );

  // return rfcomm_channel_id != 0 && client_callback == DummyCallback;
  return ( client_callback == DummyCallback );
}

static int BTOpen(ChannelCallback cb, int_or_ptr_t open_arg, int_or_ptr_t cb_args) {
  log_printf("BTOpen()");
  client_callback = cb;
  client_callback_arg = cb_args;
  return 0;
}

static void BTSend(int h, const void *data, int size) {
  assert(!(size >> 16));
  assert(h == 0);
  rfcomm_send_internal(rfcomm_channel_id, (uint8_t *) data, size & 0xFFFF);
}

static int BTCanSend(int h) {
  assert(h == 0);
  // return rfcomm_can_send(rfcomm_channel_id);
  return rfcomm_can_send_packet_now(rfcomm_channel_id);
}

static void BTClose(int h) {
  assert(h == 0);
  rfcomm_disconnect_internal(rfcomm_channel_id);
}

static int BTIsAvailable() {
  //  return USBHostBluetoothIsDeviceAttached();
  int res = USBHostBluetoothIsDeviceAttached();
  if ( res )
    LogConn("BTIsAvailable()");

  return res;
}

static int BTMaxPacketSize(int h) {
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
