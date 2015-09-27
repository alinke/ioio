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

// Implementation of btstack hci transport layer over IOIO's bluetooth dongle
// driver within the MCHPUSB host framework.

#include <assert.h>

#include "btstack-config.h"
//#include "config.h"

#include "usb_host_bluetooth.h"
#include "logging.h"

#include "debug.h"
#include "hci.h"
#include "hci_transport.h"
#include "hci_dump.h"


#include "bt_log.h"

/*
static int log_usb_open = 1;
static int log_usb_close = 1;
static int log_usb_can_send_packet = 0;
static int log_usb_send_packet = 1;
static int log_usb_send_cmd_packet = 1;
static int log_usb_send_acl_packet = 0;
static int log_bluetooth_callback = 1;
*/
/*
static int log_usb_open = 1;
static int log_usb_close = 0;
static int log_usb_can_send_packet = 0;
static int log_usb_send_packet = 0;
static int log_usb_send_cmd_packet = 0;
static int log_usb_send_acl_packet = 0;
static int log_bluetooth_callback = 0;
*/

/*
#include <stdio.h>


const char* packetTypeName(uint8_t type) {
  switch ( type ) {
  case HCI_COMMAND_DATA_PACKET:
    return "HCI_COMMAND_DATA_PACKET";
  case HCI_ACL_DATA_PACKET:
    return "HCI_ACL_DATA_PACKET";
  }
  return "PACKET_TYPE-unknown";
}


//const char* usbBluetoothEventName(BLUETOOTH_EVENT event) {
const char* usbBluetoothEventName(uint16_t event) {
  //  LogTransport("usbBluetoothEventName  event 0x%04x   %u", event, event );

  switch ( event ) {
  case BLUETOOTH_EVENT_ATTACHED:
    return "BLUETOOTH_EVENT_ATTACHED";
  case BLUETOOTH_EVENT_DETACHED:
    return "BLUETOOTH_EVENT_DETACHED";
  case BLUETOOTH_EVENT_READ_BULK_DONE:
    return "BLUETOOTH_EVENT_READ_BULK_DONE";
  case BLUETOOTH_EVENT_READ_INTERRUPT_DONE:
    return "BLUETOOTH_EVENT_READ_INTERRUPT_DONE";
  case BLUETOOTH_EVENT_WRITE_BULK_DONE:
    return "BLUETOOTH_EVENT_WRITE_BULK_DONE";
  case BLUETOOTH_EVENT_WRITE_CONTROL_DONE:
    return "BLUETOOTH_EVENT_WRITE_CONTROL_DONE";
  }

  return "BLUETOOTH_EVENT-unknown";
}
*/
/*
static char _usbEventBuf[32];

//char* usbEventName(USB_EVENT event) {
const char* usbEventName(uint16_t event) {
  //  LogTransport("usbEventName  event 0x%04x   %u", event, event );

  switch ( event ) {
  case USB_SUCCESS:
    return "USB_SUCCESS";
  case USB_INVALID_STATE:
    return "USB_INVALID_STATE";
  case USB_BUSY:
    return "USB_BUSY";
  case USB_ILLEGAL_REQUEST:
    return "USB_ILLEGAL_REQUEST";
  case USB_INVALID_CONFIGURATION:
    return "USB_INVALID_CONFIGURATION";
  case USB_MEMORY_ALLOCATION_ERROR:
    return "USB_MEMORY_ALLOCATION_ERROR";
  case USB_UNKNOWN_DEVICE:
    return "USB_UNKNOWN_DEVICE";
  case USB_CANNOT_ENUMERATE:
    return "USB_CANNOT_ENUMERATE";
  case USB_EVENT_QUEUE_FULL:
    return "USB_EVENT_QUEUE_FULL";
  case USB_ENDPOINT_BUSY:
    return "USB_ENDPOINT_BUSY";
  case USB_ENDPOINT_STALLED:
    return "USB_ENDPOINT_STALLED";
  case USB_ENDPOINT_ERROR:
    return "USB_ENDPOINT_ERROR";
  case USB_ENDPOINT_ERROR_ILLEGAL_PID:
    return "USB_ENDPOINT_ERROR_ILLEGAL_PID";
  case USB_ENDPOINT_NOT_FOUND:
    return "USB_ENDPOINT_NOT_FOUND";
  case USB_ENDPOINT_ILLEGAL_DIRECTION:
    return "USB_ENDPOINT_ILLEGAL_DIRECTION";
  case USB_ENDPOINT_NAK_TIMEOUT:
    return "USB_ENDPOINT_NAK_TIMEOUT";
  case USB_ENDPOINT_ILLEGAL_TYPE:
    return "USB_ENDPOINT_ILLEGAL_TYPE";
  case USB_ENDPOINT_UNRESOLVED_STATE:
    return "USB_ENDPOINT_UNRESOLVED_STATE";
  case USB_ENDPOINT_ERROR_BIT_STUFF:
    return "USB_ENDPOINT_ERROR_BIT_STUFF";
  case USB_ENDPOINT_ERROR_DMA:
    return "USB_ENDPOINT_ERROR_DMA";
  case USB_ENDPOINT_ERROR_TIMEOUT:
    return "USB_ENDPOINT_ERROR_TIMEOUT";
  case USB_ENDPOINT_ERROR_DATA_FIELD:
    return "USB_ENDPOINT_ERROR_DATA_FIELD";
  case USB_ENDPOINT_ERROR_CRC16:
    return "USB_ENDPOINT_ERROR_CRC16";
  case USB_ENDPOINT_ERROR_END_OF_FRAME:
    return "USB_ENDPOINT_ERROR_END_OF_FRAME";
  case USB_ENDPOINT_ERROR_PID_CHECK:
    return "USB_ENDPOINT_ERROR_PID_CHECK";
  case USB_ENDPOINT_ERROR_BMX:
    return "USB_ENDPOINT_ERROR_BMX";
  case USB_ERROR_INSUFFICIENT_POWER:
    return "USB_ERROR_INSUFFICIENT_POWER";
  }

  sprintf(_usbEventBuf, "[USB_EVENT 0x%04x]", event);
  return _usbEventBuf;
}
*/

/*
char* usbEventName(USB_EVENT event) {
  switch ( event ) {
    // No event occured (NULL event)
  case EVENT_NONE:
    return "EVENT_NONE";

  case EVENT_DEVICE_STACK_BASE:
    return "EVENT_DEVICE_STACK_BASE";
  case EVENT_HOST_STACK_BASE:               // 100
    return "EVENT_HOST_STACK_BASE";
  case EVENT_HUB_ATTACH:
    return "EVENT_HUB_ATTACH";

  case EVENT_STALL:
    return "EVENT_STALL";
  case EVENT_VBUS_SES_REQUEST:
    return "EVENT_VBUS_SES_REQUEST";
  case EVENT_VBUS_OVERCURRENT:
    return "EVENT_VBUS_OVERCURRENT";
  case EVENT_VBUS_REQUEST_POWER:
    return "EVENT_VBUS_REQUEST_POWER";
  case EVENT_VBUS_RELEASE_POWER:
    return "EVENT_VBUS_RELEASE_POWER";
  case EVENT_VBUS_POWER_AVAILABLE:
    return "EVENT_VBUS_POWER_AVAILABLE";
  case EVENT_UNSUPPORTED_DEVICE:
    return "EVENT_UNSUPPORTED_DEVICE";
  case EVENT_CANNOT_ENUMERATE:
    return "EVENT_CANNOT_ENUMERATE";
  case EVENT_CLIENT_INIT_ERROR:
    return "EVENT_CLIENT_INIT_ERROR";
  case EVENT_OUT_OF_MEMORY:
    return "EVENT_OUT_OF_MEMORY";
  case EVENT_UNSPECIFIED_ERROR:
    return "EVENT_UNSPECIFIED_ERROR";
  case EVENT_DETACH:
    return "EVENT_DETACH";
  case EVENT_TRANSFER:
    return "EVENT_TRANSFER";
  case EVENT_SOF:
    return "EVENT_SOF";
  case EVENT_RESUME:
    return "EVENT_RESUME";
  case EVENT_SUSPEND:
    return "EVENT_SUSPEND";
  case EVENT_RESET:
    return "EVENT_RESET";
  case EVENT_DATA_ISOC_READ:
    return "EVENT_DATA_ISOC_READ";
  case EVENT_DATA_ISOC_WRITE:
    return "EVENT_DATA_ISOC_WRITE";
  case EVENT_OVERRIDE_CLIENT_DRIVER_SELECTION:
    return "EVENT_OVERRIDE_CLIENT_DRIVER_SELECTION";
  case EVENT_1MS:
    return "EVENT_1MS,";

//    EVENT_GENERIC_BASE  = 400,      // Offset for Generic class events
//    EVENT_MSD_BASE      = 500,      // Offset for Mass Storage Device class events
//    EVENT_HID_BASE      = 600,      // Offset for Human Interface Device class events
//    EVENT_PRINTER_BASE  = 700,      // Offset for Printer class events
//    EVENT_CDC_BASE      = 800,      // Offset for CDC class events
//    EVENT_CHARGER_BASE  = 900,      // Offset for Charger client driver events.
//    EVENT_AUDIO_BASE    = 1000,      // Offset for Audio client driver events.
//    EVENT_USER_BASE     = 10000,    // Add integral values to this event number
//                                    // to create user-defined events.

  case EVENT_BUS_ERROR:
    return "EVENT_BUS_ERROR";
  }

  sprintf(_usbEventBuf, "[USB_EVENT 0x%04x]", event);
  return _usbEventBuf;
}
*/




static uint8_t *bulk_in;
static int bulk_in_size;
static uint8_t *int_in;
#define INT_IN_SIZE 64

void hci_transport_mchpusb_tasks() {
  if (!USBHostBlueToothIntInBusy()) {
    USBHostBluetoothReadInt(int_in, INT_IN_SIZE);
  }
  if (!USBHostBlueToothBulkInBusy()) {
    USBHostBluetoothReadBulk(bulk_in, bulk_in_size);
  }
}

// single instance
static hci_transport_t hci_transport_mchpusb;
static void (*packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size) = NULL;

static int usb_open(void *transport_config) {
  //  if ( log_usb_open )
  //    LogTransport("usb_open()");
  return 0;
}

static int usb_close() {
  //  if ( log_usb_close )
  //    LogTransport("usb_close()");
  return 0;
}


static int usb_send_cmd_packet(uint8_t *packet, int size) {
  //  if ( log_usb_send_cmd_packet )
  //    LogTransport("usb_send_cmd_packet   size(%d)", size);
    //LogPacket("usb_send_cmd_packet", packet, size);

  return USB_SUCCESS == USBHostBluetoothWriteControl(packet, size) ? 0 : -1;
}

static int usb_send_acl_packet(uint8_t *packet, int size) {
  //  if ( log_usb_send_acl_packet )
  //    LogTransport("usb_send_acl_packet   size(%d)", size);
  return USB_SUCCESS == USBHostBluetoothWriteBulk(packet, size) ? 0 : -1;
}


static int usb_send_packet(uint8_t packet_type, uint8_t * packet, int size) {
  int res;
      //    LogTransport("usb_send_packet  type(%s)  size(%d)  %s", packetTypeName(packet_type), size, );

  switch (packet_type) {
    case HCI_COMMAND_DATA_PACKET:
      //      if ( log_usb_send_packet )
      //        LogHCIPacket("usb_send_packet COMMAND ", packet, size);
      res = usb_send_cmd_packet(packet, size);
      break;

    case HCI_ACL_DATA_PACKET:
      //      if ( log_usb_send_packet )
      //        LogHCIPacket("usb_send_packet ACL ", packet, size);
      res = usb_send_acl_packet(packet, size);
      break;
    default:
      return -1;
  }

  //  //  if ( log_usb_send_packet || ( res != 0 ) )
  //  //    LogTransport("usb_send_packet  -> %d", res );
  return res;
}

static int usb_can_send_packet(uint8_t packet_type) {
  //  if ( log_usb_can_send_packet )
  //    LogTransport("usb_can_send_packet( 0x%02x )", packet_type );

  int res;
  switch (packet_type) {
    case HCI_COMMAND_DATA_PACKET:
      res = !USBHostBlueToothControlOutBusy();
      break;
    case HCI_ACL_DATA_PACKET:
      res = !USBHostBlueToothBulkOutBusy();
      break;
    default:
      res = -1;
  }
  if ( res != 1 )
    LogTransport("usb_can_send_packet  type(%s)  -> %d", packetTypeName(packet_type), res );
  return res;
}

static void usb_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)) {
  //  LogTransport("usb_register_packet_handler( %p )", handler);
  //  log_info("registering packet handler\n");
  packet_handler = handler;
}

static const char * usb_get_transport_name() {
  return "USB";
}

// get usb singleton

hci_transport_t * hci_transport_mchpusb_instance(void *buf, int size) {
  assert(size > INT_IN_SIZE + 256);
  int_in = buf;
  bulk_in = int_in + INT_IN_SIZE;
  bulk_in_size = size - INT_IN_SIZE;
  hci_transport_mchpusb.open = usb_open;
  hci_transport_mchpusb.close = usb_close;
  hci_transport_mchpusb.send_packet = usb_send_packet;
  hci_transport_mchpusb.register_packet_handler = usb_register_packet_handler;
  hci_transport_mchpusb.get_transport_name = usb_get_transport_name;
  hci_transport_mchpusb.set_baudrate = NULL;
  hci_transport_mchpusb.can_send_packet_now = usb_can_send_packet;
  return &hci_transport_mchpusb;
}


BOOL USBHostBluetoothCallback(BLUETOOTH_EVENT event, USB_EVENT status, void *data, DWORD size) {
  //  int size2 = (int)size;
  //  uint16_t bt_event = (uint16_t)event;
  //  uint16_t usb_status = (uint16_t)status;

  //  uint8_t *packet = (uint8_t *)data;
  
  //  if ( event == BLUETOOTH_EVENT_WRITE_CONTROL_DONE ) {

  //  if ( log_bluetooth_callback ) {
  //    LogLine("");
  //    LogTransportPacket("USBHostBluetoothCallback", usbEventName(usb_status), usbBluetoothEventName(bt_event), packet, size2);
  //  }


  //
  // Handle bluetooth event
  //
  switch (event) {
    case BLUETOOTH_EVENT_WRITE_BULK_DONE:
    case BLUETOOTH_EVENT_WRITE_CONTROL_DONE:
    {
      uint8_t e[] = { DAEMON_EVENT_HCI_PACKET_SENT, 0 };
      packet_handler(HCI_EVENT_PACKET, &e[0], sizeof(e));
      //      if ( log_bluetooth_callback )
      //        LogTransport("1 USBHostBluetoothCallback -> TRUE");
      return TRUE;
    }

    case BLUETOOTH_EVENT_DETACHED:
      hci_close();
    case BLUETOOTH_EVENT_ATTACHED:
      //      if ( log_bluetooth_callback )
      //        LogTransport("2 USBHostBluetoothCallback -> TRUE");
      return TRUE;

    case BLUETOOTH_EVENT_READ_BULK_DONE:
      if (status == USB_SUCCESS) {
        if (size) {
          if (packet_handler) {
            packet_handler(HCI_ACL_DATA_PACKET, data, size);
          }
        }
      } else {
        LogTransport("  ## Read bulk failure");
        // log_printf("Read bulk failure");
      }
      //      if ( log_bluetooth_callback )
      //        LogTransport("3 USBHostBluetoothCallback -> TRUE");
      return TRUE;

    case BLUETOOTH_EVENT_READ_INTERRUPT_DONE:
      if (status == USB_SUCCESS) {
        if (size) {
          if (packet_handler) {
            packet_handler(HCI_EVENT_PACKET, data, size);
          }
        }
      } else {
        LogTransport("  ## Read bulk failure");
        // log_printf("Read bulk failure");
      }
      //      if ( log_bluetooth_callback )
      //        LogTransport("4 USBHostBluetoothCallback -> TRUE");
      return TRUE;

    default:
      //      if ( log_bluetooth_callback )
      //        LogTransport("4 USBHostBluetoothCallback -> FALSE");
      return FALSE;
  }
}
