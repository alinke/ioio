#include <stdio.h>

#include "btstack-config.h"
//#include "config.h"

#include "hci.h"
//#include "gap.h"

#ifdef HAVE_BLE
#include "gap_le.h"
#endif

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifndef EMBEDDED
#ifdef _WIN32
#include "Winsock2.h"
#else
#include <unistd.h> // gethostbyname
#endif
#include <btstack/version.h>
#endif

#include "btstack_memory.h"
#include "debug.h"
#include "hci_dump.h"

#include <btstack/linked_list.h>
#include <btstack/hci_cmds.h>

#define HCI_CONNECTION_TIMEOUT_MS 10000

#ifdef USE_BLUETOOL
#include "../platforms/ios/src/bt_control_iphone.h"
#endif



#include <btstack/hci_cmds.h>


#include "bt_log.h"


// OGF_CONTROLLER_BASEBAND
#define HCI_SET_EVENT_MASK                   0x01
#define HCI_RESET                            0x03
#define HCI_SET_EVENT_FILTER                 0x05
#define HCI_FLUSH                            0x08
#define HCI_READ_PIN_TYPE                    0x09
#define HCI_WRITE_PIN_TYPE                   0x0A
#define HCI_CREATE_NEW_UNIT_KEY              0x0B
#define HCI_READ_STORED_LINK_KEY             0x0D
#define HCI_WRITE_STORED_LINK_KEY            0x11
#define HCI_DELETE_STORED_LINK_KEY           0x12
#define HCI_WRITE_LOCAL_NAME                 0x13
#define HCI_READ_LOCAL_NAME                  0x14
#define HCI_READ_CONNECTION_ACCEPT_TIMEOUT   0x15
#define HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT  0x16
#define HCI_READ_PAGE_TIMEOUT                0x17
#define HCI_WRITE_PAGE_TIMEOUT               0x18
#define HCI_READ_SCAN_ENABLE                 0x19
#define HCI_WRITE_SCAN_ENABLE                0x1A
#define HCI_READ_PAGE_SCAN_ACTIVITY          0x1B
#define HCI_WRITE_PAGE_SCAN_ACTIVITY         0x1C
#define HCI_READ_INQUIRY_SCAN_ACTIVITY       0x1D
#define HCI_WRITE_INQUIRY_SCAN_ACTIVITY      0x1E
#define HCI_READ_AUTHENTICATION_ENABLE       0x1F
#define HCI_WRITE_AUTHENTICATION_ENABLE      0x20
#define HCI_READ_CLASS_OF_DEVICE             0x23
#define HCI_WRITE_CLASS_OF_DEVICE            0x24
#define HCI_READ_VOICE_SETTING               0x25
#define HCI_WRITE_VOICE_SETTING              0x26
 
#define HCI_READ_SIMPLE_PAIRING_MODE         0x55
#define HCI_WRITE_SIMPLE_PAIRING_MODE        0x56


//  OGF_INFORMATIONAL_PARAMETERS

#define HCI_READ_LOCAL_VERSION_INFORMATION   0x01
#define HCI_READ_LOCAL_SUPPORTED_COMMANDS    0x02
#define HCI_READ_LOCAL_SUPPORTED_FEATURES    0x03
#define HCI_READ_LOCAL_EXTENDED_FEATURES     0x04
#define HCI_READ_BUFFER_SIZE                 0x05
#define HCI_READ_BD_ADDR                     0x09
#define HCI_READ_DATA_BLOCK_SIZE             0x0a



//--------------------
//
// Helpers
//

const char *stackStateName(HCI_STATE state) {
  switch ( state ) {
  case HCI_STATE_OFF:
    return "OFF";
  case HCI_STATE_INITIALIZING:
    return "INITIALIZING";
  case HCI_STATE_WORKING:
    return "WORKING";
  case HCI_STATE_HALTING:
    return "HALTING";
  case HCI_STATE_SLEEPING:
    return "SLEEPING";
  case HCI_STATE_FALLING_ASLEEP:
    return "FALLING_ASLEEP";
  }

  return "<unknown>";
}


/*
static char p_buf[250];
char *get_packet_string(uint8_t *packet, uint16_t size)
{
  int i = 0;
  int pos = 0;
  int len = 0;
  char *buf = p_buf;

  int n = size;
  if ( n > 8 )
    n = 8;

  for ( i = 0; i < n; i++ ) {
    len = sprintf( buf, "%02x ", packet[i] );
    pos += len;
    buf = &p_buf[pos];
  }

  return p_buf;
}
*/


//
// OGF
//
static char _ogfBuf[6];

const char *ogfName(uint16_t opcode) {
  uint16_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  //  uint16_t ocf = ( opcode & 0x03ff );

  switch ( ogf ) {

  case 0:
    return "OGF_NOP";
  case OGF_LINK_CONTROL:
    return "OGF_LINK_CONTROL";
  case OGF_LINK_POLICY:
    return "OGF_LINK_POLICY";
  case OGF_CONTROLLER_BASEBAND:
    return "OGF_CONTROLLER_BASEBAND";
  case OGF_INFORMATIONAL_PARAMETERS:
    return "OGF_INFORMATIONAL_PARAMETERS";
  case OGF_STATUS_PARAMETERS:
    return "OGF_STATUS_PARAMETERS";
  case OGF_LE_CONTROLLER:
    return "OGF_LE_CONTROLLER";
  case OGF_BTSTACK:
    return "OGF_BTSTACK";
  case OGF_VENDOR:
    return "OGF_VENDOR";
  }

  sprintf(_ogfBuf, "0x%02x", ogf);
  return _ogfBuf;
  //  return "OGF ?";
}



//
// Events
//

#define HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE    0x20
#define HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED    0x38

static char _hciEventBuf[6];

char *hciEventName(uint8_t code) {
  switch ( code ) {

  case HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE:
    return "HCI_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE";
  case HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED:
    return "HCI_EVENT_LINK_SUPERVISION_TIMEOUT_CHANGED";


  case DAEMON_EVENT_HCI_PACKET_SENT:
    return "DAEMON_EVENT_HCI_PACKET_SENT";

    //  case HCI_EVENT_COMMAND_COMPLETE:
    //    return "HCI_EVENT_COMMAND_COMPLETE";

  case HCI_EVENT_INQUIRY_COMPLETE:
    return "HCI_EVENT_INQUIRY_COMPLETE";
  case HCI_EVENT_INQUIRY_RESULT:
    return "HCI_EVENT_INQUIRY_RESULT";
  case HCI_EVENT_CONNECTION_COMPLETE:
    return "HCI_EVENT_CONNECTION_COMPLETE";
  case HCI_EVENT_CONNECTION_REQUEST:
    return "HCI_EVENT_CONNECTION_REQUEST";
  case HCI_EVENT_DISCONNECTION_COMPLETE:
    return "HCI_EVENT_DISCONNECTION_COMPLETE";
  case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
    return "HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT";
    /*
  case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
    return "HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE";
  case HCI_EVENT_ENCRYPTION_CHANGE:
    return "HCI_EVENT_ENCRYPTION_CHANGE";
  case HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE:
    return "HCI_EVENT_CHANGE_CONNECTION_LINK_KEY_COMPLETE";
  case HCI_EVENT_MASTER_LINK_KEY_COMPLETE:
    return "HCI_EVENT_MASTER_LINK_KEY_COMPLETE";
    */
  case HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE:
    return "HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE";
    /*
  case HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
    return "HCI_EVENT_READ_REMOTE_VERSION_INFORMATION_COMPLETE";
  case HCI_EVENT_QOS_SETUP_COMPLETE:
    return "HCI_EVENT_QOS_SETUP_COMPLETE";
  case HCI_EVENT_COMMAND_COMPLETE:
    return "HCI_EVENT_COMMAND_COMPLETE";
    */
  case HCI_EVENT_COMMAND_STATUS:
    return "HCI_EVENT_COMMAND_STATUS";
    /*
  case HCI_EVENT_HARDWARE_ERROR:
    return "HCI_EVENT_HARDWARE_ERROR";
  case HCI_EVENT_FLUSH_OCCURED:
    return "HCI_EVENT_FLUSH_OCCURED";
  case HCI_EVENT_ROLE_CHANGE:
    return "HCI_EVENT_ROLE_CHANGE";
    */
  case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
    return "HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS";
    /*
  case HCI_EVENT_MODE_CHANGE_EVENT:
    return "HCI_EVENT_MODE_CHANGE_EVENT";
  case HCI_EVENT_RETURN_LINK_KEYS:
    return "HCI_EVENT_RETURN_LINK_KEYS";
    */
  case HCI_EVENT_PIN_CODE_REQUEST:
    return "HCI_EVENT_PIN_CODE_REQUEST";
  case HCI_EVENT_LINK_KEY_REQUEST:
    return "HCI_EVENT_LINK_KEY_REQUEST";
  case HCI_EVENT_LINK_KEY_NOTIFICATION:
    return "HCI_EVENT_LINK_KEY_NOTIFICATION";
  case HCI_EVENT_DATA_BUFFER_OVERFLOW:
    return "HCI_EVENT_DATA_BUFFER_OVERFLOW";
  case HCI_EVENT_MAX_SLOTS_CHANGED:
    return "HCI_EVENT_MAX_SLOTS_CHANGED";
    /*
  case HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE:
    return "HCI_EVENT_READ_CLOCK_OFFSET_COMPLETE";
  case HCI_EVENT_PACKET_TYPE_CHANGED:
    return "HCI_EVENT_PACKET_TYPE_CHANGED";
  case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
    return "HCI_EVENT_INQUIRY_RESULT_WITH_RSSI";
    //  case HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE:
    //    return "HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE";
  case HCI_EVENT_EXTENDED_INQUIRY_RESPONSE:
    return "HCI_EVENT_EXTENDED_INQUIRY_RESPONSE";
  case HCI_EVENT_IO_CAPABILITY_REQUEST:
    return "HCI_EVENT_IO_CAPABILITY_REQUEST";
  case HCI_EVENT_IO_CAPABILITY_RESPONSE:
    return "HCI_EVENT_IO_CAPABILITY_RESPONSE";
  case HCI_EVENT_USER_CONFIRMATION_REQUEST:
    return "HCI_EVENT_USER_CONFIRMATION_REQUEST";
  case HCI_EVENT_USER_PASSKEY_REQUEST:
    return "HCI_EVENT_USER_PASSKEY_REQUEST";
  case HCI_EVENT_REMOTE_OOB_DATA_REQUEST:
    return "HCI_EVENT_REMOTE_OOB_DATA_REQUEST";
  case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
    return "HCI_EVENT_SIMPLE_PAIRING_COMPLETE";
    */
  case HCI_EVENT_LE_META:
    return "HCI_EVENT_LE_META";
    /*
    //  case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
    //    return "HCI_SUBEVENT_LE_CONNECTION_COMPLETE";
    //  case HCI_SUBEVENT_LE_ADVERTISING_REPORT:
    //    return "HCI_SUBEVENT_LE_ADVERTISING_REPORT";
    //  case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
    //    return "HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE";
    //  case HCI_SUBEVENT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
    //    return "HCI_SUBEVENT_LE_READ_REMOTE_USED_FEATURES_COMPLETE";
    //  case HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST:
    //    return "HCI_SUBEVENT_LE_LONG_TERM_KEY_REQUEST";
  case BTSTACK_EVENT_STATE:
    return "BTSTACK_EVENT_STATE";
  case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
    return "BTSTACK_EVENT_NR_CONNECTIONS_CHANGED";
  case BTSTACK_EVENT_POWERON_FAILED:
    return "BTSTACK_EVENT_POWERON_FAILED";
  case BTSTACK_EVENT_VERSION:
    return "BTSTACK_EVENT_VERSION";
  case BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED:
    return "BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED";
  case BTSTACK_EVENT_REMOTE_NAME_CACHED:
    return "BTSTACK_EVENT_REMOTE_NAME_CACHED";
  case BTSTACK_EVENT_DISCOVERABLE_ENABLED:
    return "BTSTACK_EVENT_DISCOVERABLE_ENABLED";
  case DAEMON_EVENT_CONNECTION_OPENED:
    return "DAEMON_EVENT_CONNECTION_OPENED";
  case DAEMON_EVENT_CONNECTION_CLOSED:
    return "DAEMON_EVENT_CONNECTION_CLOSED";
  case DAEMON_NR_CONNECTIONS_CHANGED:
    return "DAEMON_NR_CONNECTIONS_CHANGED";
  case DAEMON_EVENT_NEW_RFCOMM_CREDITS:
    return "DAEMON_EVENT_NEW_RFCOMM_CREDITS";
  case DAEMON_EVENT_HCI_PACKET_SENT:
    return "DAEMON_EVENT_HCI_PACKET_SENT";
  case L2CAP_EVENT_CHANNEL_OPENED:
    return "L2CAP_EVENT_CHANNEL_OPENED";
  case L2CAP_EVENT_CHANNEL_CLOSED:
    return "L2CAP_EVENT_CHANNEL_CLOSED";
  case L2CAP_EVENT_INCOMING_CONNECTION:
    return "L2CAP_EVENT_INCOMING_CONNECTION";
  case L2CAP_EVENT_TIMEOUT_CHECK:
    return "L2CAP_EVENT_TIMEOUT_CHECK";
  case L2CAP_EVENT_CREDITS:
    return "L2CAP_EVENT_CREDITS";
  case L2CAP_EVENT_SERVICE_REGISTERED:
    return "L2CAP_EVENT_SERVICE_REGISTERED";
    //  case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST:
    //    return "L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_REQUEST";
    //  case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
    //    return "L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE";
  case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
    return "RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE";
  case RFCOMM_EVENT_CHANNEL_CLOSED:
    return "RFCOMM_EVENT_CHANNEL_CLOSED";
  case RFCOMM_EVENT_INCOMING_CONNECTION:
    return "RFCOMM_EVENT_INCOMING_CONNECTION";
  case RFCOMM_EVENT_REMOTE_LINE_STATUS:
    return "RFCOMM_EVENT_REMOTE_LINE_STATUS";
  case RFCOMM_EVENT_CREDITS:
    return "RFCOMM_EVENT_CREDITS";
  case RFCOMM_EVENT_SERVICE_REGISTERED:
    return "RFCOMM_EVENT_SERVICE_REGISTERED";
  case RFCOMM_EVENT_PERSISTENT_CHANNEL:
    return "RFCOMM_EVENT_PERSISTENT_CHANNEL";
    //  case RFCOMM_EVENT_REMOTE_MODEM_STATUS:
    //    return "RFCOMM_EVENT_REMOTE_MODEM_STATUS";
    //  case RFCOMM_EVENT_PORT_CONFIGURATION:
    //    return "RFCOMM_EVENT_PORT_CONFIGURATION";
  case SDP_SERVICE_REGISTERED:
    return "SDP_SERVICE_REGISTERED";
  case SDP_QUERY_COMPLETE:
    return "SDP_QUERY_COMPLETE";
  case SDP_QUERY_RFCOMM_SERVICE:
    return "SDP_QUERY_RFCOMM_SERVICE";
  case SDP_QUERY_ATTRIBUTE_VALUE:
    return "SDP_QUERY_ATTRIBUTE_VALUE";
  case SDP_QUERY_SERVICE_RECORD_HANDLE:
    return "SDP_QUERY_SERVICE_RECORD_HANDLE";
    //  case GATT_QUERY_COMPLETE:
    //    return "GATT_QUERY_COMPLETE";
  case GATT_SERVICE_QUERY_RESULT:
    return "GATT_SERVICE_QUERY_RESULT";
  case GATT_CHARACTERISTIC_QUERY_RESULT:
    return "GATT_CHARACTERISTIC_QUERY_RESULT";
    //  case GATT_INCLUDED_SERVICE_QUERY_RESULT:
    //    return "GATT_INCLUDED_SERVICE_QUERY_RESULT";
    //  case GATT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
    //    return "GATT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT";
    //  case GATT_CHARACTERISTIC_VALUE_QUERY_RESULT:
    //    return "GATT_CHARACTERISTIC_VALUE_QUERY_RESULT";
    //  case GATT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT:
    //    return "GATT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT";
    //  case GATT_NOTIFICATION:
    //    return "GATT_NOTIFICATION";
    //  case GATT_INDICATION:
    //    return "GATT_INDICATION";
    //  case GATT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
    //    return "GATT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT";
    //  case GATT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
    //    return "GATT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT";
    //  case GATT_MTU:
    //    return "GATT_MTU";
    //  case ATT_MTU_EXCHANGE_COMPLETE:
    //    return "ATT_MTU_EXCHANGE_COMPLETE";
    //  case ATT_HANDLE_VALUE_INDICATION_COMPLETE:
    //    return "ATT_HANDLE_VALUE_INDICATION_COMPLETE";
    //  case BNEP_EVENT_SERVICE_REGISTERED:
    //    return "BNEP_EVENT_SERVICE_REGISTERED";
    //  case BNEP_EVENT_OPEN_CHANNEL_COMPLETE:
    //    return "BNEP_EVENT_OPEN_CHANNEL_COMPLETE";
    //  case BNEP_EVENT_INCOMING_CONNECTION:
    //    return "BNEP_EVENT_INCOMING_CONNECTION";
    //  case BNEP_EVENT_CHANNEL_CLOSED:
    //    return "BNEP_EVENT_CHANNEL_CLOSED";
    //  case BNEP_EVENT_CHANNEL_TIMEOUT:
    //    return "BNEP_EVENT_CHANNEL_TIMEOUT";
    //  case BNEP_EVENT_READY_TO_SEND:
    //    return "BNEP_EVENT_READY_TO_SEND";
  case SM_JUST_WORKS_REQUEST:
    return "SM_JUST_WORKS_REQUEST";
  case SM_JUST_WORKS_CANCEL:
    return "SM_JUST_WORKS_CANCEL";
  case SM_PASSKEY_DISPLAY_NUMBER:
    return "SM_PASSKEY_DISPLAY_NUMBER";
  case SM_PASSKEY_DISPLAY_CANCEL:
    return "SM_PASSKEY_DISPLAY_CANCEL";
  case SM_PASSKEY_INPUT_NUMBER:
    return "SM_PASSKEY_INPUT_NUMBER";
  case SM_PASSKEY_INPUT_CANCEL:
    return "SM_PASSKEY_INPUT_CANCEL";
    //  case SM_IDENTITY_RESOLVING_STARTED:
    //    return "SM_IDENTITY_RESOLVING_STARTED";
    //  case SM_IDENTITY_RESOLVING_FAILED:
    //    return "SM_IDENTITY_RESOLVING_FAILED";
    //  case SM_IDENTITY_RESOLVING_SUCCEEDED:
    //    return "SM_IDENTITY_RESOLVING_SUCCEEDED";
    //  case SM_AUTHORIZATION_REQUEST:
    //    return "SM_AUTHORIZATION_REQUEST";
    //  case SM_AUTHORIZATION_RESULT:
    //    return "SM_AUTHORIZATION_RESULT";
    //  case GAP_SECURITY_LEVEL:
    //    return "GAP_SECURITY_LEVEL";
    //  case GAP_DEDICATED_BONDING_COMPLETED:
    //    return "GAP_DEDICATED_BONDING_COMPLETED";
    //  case GAP_LE_ADVERTISING_REPORT:
    //    return "GAP_LE_ADVERTISING_REPORT";
    //  case HCI_EVENT_HSP_META:
    //    return "HCI_EVENT_HSP_META";

//  case HSP_SUBEVENT_AUDIO_CONNECTION_COMPLETE:
//    return "HSP_SUBEVENT_AUDIO_CONNECTION_COMPLETE";
//  case HSP_SUBEVENT_AUDIO_DISCONNECTION_COMPLETE:
//    return "HSP_SUBEVENT_AUDIO_DISCONNECTION_COMPLETE";
//  case HSP_SUBEVENT_MICROPHONE_GAIN_CHANGED:
//    return "HSP_SUBEVENT_MICROPHONE_GAIN_CHANGED";
//  case HSP_SUBEVENT_SPEAKER_GAIN_CHANGED:
//    return "HSP_SUBEVENT_SPEAKER_GAIN_CHANGED";
//  case HSP_SUBEVENT_HS_COMMAND:
//    return "HSP_SUBEVENT_HS_COMMAND";
//  case HSP_SUBEVENT_AG_INDICATION:
//    return "HSP_SUBEVENT_AG_INDICATION";
//  case HSP_SUBEVENT_ERROR:
//    return "HSP_SUBEVENT_ERROR";
//  case HSP_SUBEVENT_RING:
//    return "HSP_SUBEVENT_RING";

    //  case HCI_EVENT_HFP_META:
    //    return "HCI_EVENT_HFP_META";

//  case HFP_SUBEVENT_AUDIO_CONNECTION_COMPLETE:
//    return "HFP_SUBEVENT_AUDIO_CONNECTION_COMPLETE";
//  case HFP_SUBEVENT_SUPPORTED_FEATURES_EXCHANGE:
//    return "HFP_SUBEVENT_SUPPORTED_FEATURES_EXCHANGE";

    //  case ANCS_CLIENT_CONNECTED:
    //    return "ANCS_CLIENT_CONNECTED";
    ///  case ANCS_CLIENT_NOTIFICATION:
    //    return "ANCS_CLIENT_NOTIFICATION";
    //  case ANCS_CLIENT_DISCONNECTED:
    //    return "ANCS_CLIENT_DISCONNECTED";
  case HCI_EVENT_VENDOR_SPECIFIC:
    return "HCI_EVENT_VENDOR_SPECIFIC";
    */
  }

  sprintf(_hciEventBuf, "0x%02x", code);
  return _hciEventBuf;
}


/*
static char _substateBuf[20];

const char *substateName(uint8_t state) {
  sprintf(_substateBuf, "[Substate 0x%02x]", state);
  return _substateBuf;
}
*/


  /*
const char *substateName(hci_substate_t state) {

  switch ( state ) {
  case HCI_INIT_SEND_RESET:
    return "SEND_RESET";

  case HCI_INIT_W4_SEND_RESET:
    return "W4_SEND_RESET";
  case HCI_INIT_SEND_READ_LOCAL_VERSION_INFORMATION:
    return "SEND_READ_LOCAL_VERSION_INFORMATION";
  case HCI_INIT_W4_SEND_READ_LOCAL_VERSION_INFORMATION:
    return "W4_SEND_READ_LOCAL_VERSION_INFORMATION";
  case HCI_INIT_SET_BD_ADDR:
    return "SET_BD_ADDR";
  case HCI_INIT_W4_SET_BD_ADDR:
    return "W4_SET_BD_ADDR";
  case HCI_INIT_SEND_RESET_ST_WARM_BOOT:
    return "SEND_RESET_ST_WARM_BOOT";
  case HCI_INIT_W4_SEND_RESET_ST_WARM_BOOT:
    return "W4_SEND_RESET_ST_WARM_BOOT";
  case HCI_INIT_SEND_BAUD_CHANGE:
    return "SEND_BAUD_CHANGE";
  case HCI_INIT_W4_SEND_BAUD_CHANGE:
    return "W4_SEND_BAUD_CHANGE";
  case HCI_INIT_CUSTOM_INIT:
    return "CUSTOM_INIT";
  case HCI_INIT_W4_CUSTOM_INIT:
    return "W4_CUSTOM_INIT";
  case HCI_INIT_SEND_RESET_CSR_WARM_BOOT:
    return "SEND_RESET_CSR_WARM_BOOT";
  case HCI_INIT_W4_CUSTOM_INIT_CSR_WARM_BOOT:
    return "CSR_WARM_BOOT";
  case HCI_INIT_READ_BD_ADDR:
    return "READ_BD_ADDR";
  case HCI_INIT_W4_READ_BD_ADDR:
    return "W4_READ_BD_ADDR";
  case HCI_INIT_READ_LOCAL_SUPPORTED_COMMANDS:
    return "READ_LOCAL_SUPPORTED_COMMANDS";
  case HCI_INIT_W4_READ_LOCAL_SUPPORTED_COMMANDS:
    return "W4_READ_LOCAL_SUPPORTED_COMMANDS";
  case HCI_INIT_READ_BUFFER_SIZE:
    return "READ_BUFFER_SIZE";
  case HCI_INIT_W4_READ_BUFFER_SIZE:
    return "W4_READ_BUFFER_SIZE";
  case HCI_INIT_READ_LOCAL_SUPPORTED_FEATUES:
    return "READ_LOCAL_SUPPORTED_FEATUES";
  case HCI_INIT_W4_READ_LOCAL_SUPPORTED_FEATUES:
    return "W4_READ_LOCAL_SUPPORTED_FEATUES";
  case HCI_INIT_SET_EVENT_MASK:
    return "SET_EVENT_MASK";
  case HCI_INIT_W4_SET_EVENT_MASK:
    return "W4_SET_EVENT_MASK";
  case HCI_INIT_WRITE_SIMPLE_PAIRING_MODE:
    return "WRITE_SIMPLE_PAIRING_MODE";
  case HCI_INIT_W4_WRITE_SIMPLE_PAIRING_MODE:
    return "W4_WRITE_SIMPLE_PAIRING_MODE";
  case HCI_INIT_WRITE_PAGE_TIMEOUT:
    return "WRITE_PAGE_TIMEOUT";
  case HCI_INIT_W4_WRITE_PAGE_TIMEOUT:
    return "W4_WRITE_PAGE_TIMEOUT";
  case HCI_INIT_WRITE_CLASS_OF_DEVICE:
    return "WRITE_CLASS_OF_DEVICE";
  case HCI_INIT_W4_WRITE_CLASS_OF_DEVICE:
    return "W4_WRITE_CLASS_OF_DEVICE";
  case HCI_INIT_WRITE_LOCAL_NAME:
    return "WRITE_LOCAL_NAME";
  case HCI_INIT_W4_WRITE_LOCAL_NAME:
    return "W4_WRITE_LOCAL_NAME";
  case HCI_INIT_WRITE_SCAN_ENABLE:
    return "WRITE_SCAN_ENABLE";
  case HCI_INIT_W4_WRITE_SCAN_ENABLE:
    return "W4_WRITE_SCAN_ENABLE";
  case HCI_INIT_LE_READ_BUFFER_SIZE:
    return "LE_READ_BUFFER_SIZE";
  case HCI_INIT_W4_LE_READ_BUFFER_SIZE:
    return "W4_LE_READ_BUFFER_SIZE";
  case HCI_INIT_WRITE_LE_HOST_SUPPORTED:
    return "WRITE_LE_HOST_SUPPORTED";
  case HCI_INIT_W4_WRITE_LE_HOST_SUPPORTED:
    return "W4_WRITE_LE_HOST_SUPPORTED";
  case HCI_INIT_LE_SET_SCAN_PARAMETERS:
    return "LE_SET_SCAN_PARAMETERS";
  case HCI_INIT_W4_LE_SET_SCAN_PARAMETERS:
    return "W4_LE_SET_SCAN_PARAMETERS";
  case HCI_INIT_DONE:
    return "DONE";
  case HCI_FALLING_ASLEEP_DISCONNECT:
    return "FALLING_ASLEEP_DISCONNECT";
  case HCI_FALLING_ASLEEP_W4_WRITE_SCAN_ENABLE:
    return "FALLING_ASLEEP_W4_WRITE_SCAN_ENABLE";
  case HCI_FALLING_ASLEEP_COMPLETE:
    return "FALLING_ASLEEP_COMPLETE";
  case HCI_INIT_AFTER_SLEEP:
    return "AFTER_SLEEP";
  }

  sprintf(_substateBuf, "[Substate 0x%02x]", state);
  return _substateBuf;
}
  */

//
// OCF
//


static char _ocfBuf[8];

const char *ocfName(uint16_t opcode) {
  uint16_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  switch ( ogf ) {
  case OGF_LINK_CONTROL:
    switch ( ocf ) {
    case 0x001:
      return "Inquiry";
    case 0x002:
      return "Inquiry_Cancel";
    case 0x003:
      return "Periodic_Inquiry_Mode";
    case 0x004:
      return "Exit_Periodic_Inquiry_Mode";
    case 0x005:
      return "Create_Connection";
    case 0x006:
      return "Disconnect";
    case 0x008:
      return "Create_Connection_Cancel";
    case 0x009:
      return "Accept_Connection_Request";
    case 0x00a:
      return "Reject_Connection_Request";
    case 0x00b:
      return "Link_Key_Request_Reply";
    case 0x00c:
      return "Link_Key_Request_Negative_Reply";
    case 0x00d:
      return "PIN_Code_Request_Reply";
    case 0x00e:
      return "PIN_Code_Negative_Reply";
    case 0x00f:
      return "Change_Connection_Packet_Type";
    case 0x011:
      return "Authentication_Requested";
    case 0x013:
      return "Set_Connection_Encryption";
    case 0x015:
      return "Change_Connection_Link_Key";
    case 0x017:
      return "Master_Link_Key";
    case 0x019:
      return "Remote_Name_Request";
    case 0x01a:
      return "Remote_Name_Request_Cancel";
    case 0x01b:
      return "Read_Remote_Supported_Features";
    case 0x01c:
      return "Read_Remote_Extended_Features";
    case 0x01d:
      return "Read_Remote_Version_Information";
    case 0x01f:
      return "Read_Clock_Offset";
    case 0x020:
      return "Read_LMP_Handle";
    case 0x028:
      return "Setup_Synchronous_Connection";
    case 0x029:
      return "Accept_Synchronous_Connection_Request";
    case 0x02a:
      return "Reject_Synchronous_Connection_Request";
    case 0x02b:
      return "IO_Capability_Request_Reply";
    case 0x02c:
      return "User_Confirmation_Request_Reply";
    case 0x02d:
      return "User_Confirmation_Request_Negative_Reply";
    case 0x02e:
      return "User_Passkey_Request_Reply";
    case 0x02f:
      return "User_Passkey_Request_Negative_Reply";
    }
    break;

  case OGF_LINK_POLICY:
    switch ( ocf ) {
    case 0x01:
      return "HCI_Hold_Mode";
    case 0x03:
      return "HCI_Sniff_Mode";
    case 0x04:
      return "HCI_Exit_Sniff_Mode";
    case 0x05:
      return "HCI_Park_State";
    case 0x06:
      return "HCI_Exit_Park_State";
    case 0x07:
      return "HCI_QoS_Setup";
    case 0x09:
      return "HCI_Role_Discovery";
    case 0x0b:
      return "HCI_Switch_Role";
    case 0x0c:
      return "HCI_Read_Link_Policy_Settings";
    case 0x0d:
      return "HCI_Write_Link_Policy_Settings";
    case 0x0e:
      return "HCI_Read_Default_Link_Policy_Settings";
    case 0x0f:
      return "HCI_Write_Default_Link_Policy_Settings";
    case 0x10:
      return "HCI_Flow_Specification";
    case 0x11:
      return "HCI_Sniff_Subrating";
    }
    break;

  case OGF_CONTROLLER_BASEBAND:
    switch ( ocf ) {
    case HCI_SET_EVENT_MASK:
      return "HCI_Set_Event_Mask";
    case HCI_RESET:
      return "HCI_Reset";
    case HCI_SET_EVENT_FILTER:
      return "HCI_Set_Event_Filter";
    case HCI_FLUSH:
      return "HCI_Flush";
    case HCI_READ_PIN_TYPE:
      return "HCI_Read_PIN_Type";
    case HCI_WRITE_PIN_TYPE:
      return "HCI_Write_PIN_Type";
    case HCI_CREATE_NEW_UNIT_KEY:
      return "HCI_Create_New_Unit_Key";
    case HCI_READ_STORED_LINK_KEY:
      return "HCI_Read_Stored_Link_Key";
    case HCI_WRITE_STORED_LINK_KEY:
      return "HCI_Write_Stored_Link_Key";
    case HCI_DELETE_STORED_LINK_KEY:
      return "HCI_Delete_Stored_Link_Key";
    case HCI_WRITE_LOCAL_NAME:
      return "HCI_Write_Local_Name";
    case HCI_READ_LOCAL_NAME:
      return "HCI_Read_Local_Name";
    case HCI_READ_CONNECTION_ACCEPT_TIMEOUT:
      return "HCI_Read_Connection_Accept_Timeout";
    case HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT:
      return "HCI_Write_Connection_Accept_Timeout";
    case HCI_READ_PAGE_TIMEOUT:
      return "HCI_Read_Page_Timeout";
    case HCI_WRITE_PAGE_TIMEOUT:
      return "HCI_Write_Page_Timeout";
    case HCI_READ_SCAN_ENABLE:
      return "HCI_Read_Scan_Enable";
    case HCI_WRITE_SCAN_ENABLE:
      return "HCI_Write_Scan_Enable";
    case HCI_READ_PAGE_SCAN_ACTIVITY:
      return "HCI_Read_Page_Scan_Activity";
    case HCI_WRITE_PAGE_SCAN_ACTIVITY:
      return "HCI_Write_Page_Scan_Activity";
    case HCI_READ_INQUIRY_SCAN_ACTIVITY:
      return "HCI_Read_Inquiry_Scan_Activity";
    case HCI_WRITE_INQUIRY_SCAN_ACTIVITY:
      return "HCI_Write_Inquiry_Scan_Activity";
    case HCI_READ_AUTHENTICATION_ENABLE:
      return "HCI_Read_Authentication_Enable";
    case HCI_WRITE_AUTHENTICATION_ENABLE:
      return "HCI_Write_Authentication_Enable";
    case HCI_READ_CLASS_OF_DEVICE:
      return "HCI_Read_Class_of_Device";
    case HCI_WRITE_CLASS_OF_DEVICE:
      return "HCI_Write_Class_of_Device";
    case HCI_READ_VOICE_SETTING:
      return "HCI_Read_Voice_Setting";
    case HCI_WRITE_VOICE_SETTING:
      return "HCI_Write_Voice_Setting";
      
    case HCI_READ_SIMPLE_PAIRING_MODE:
      return "HCI_Read_Simple_Pairing_Mode";
    case HCI_WRITE_SIMPLE_PAIRING_MODE:
      return "HCI_Write_Simple_Pairing_Mode";
    }
    break;

  case OGF_INFORMATIONAL_PARAMETERS:
    switch ( ocf ) {
    case HCI_READ_LOCAL_VERSION_INFORMATION:
      return "HCI_Read_Local_Version_Information";
    case HCI_READ_LOCAL_SUPPORTED_COMMANDS:
      return "HCI_Read_Local_Supported_Commands";
    case HCI_READ_LOCAL_SUPPORTED_FEATURES:
      return "HCI_Read_Local_Supported_Features";
    case HCI_READ_LOCAL_EXTENDED_FEATURES:
      return "HCI_Read_Local_Extended_Features";
    case HCI_READ_BUFFER_SIZE:
      return "HCI_Read_Buffer_Size";
    case HCI_READ_BD_ADDR:
      return "HCI_Read_BD_ADDR";
    case HCI_READ_DATA_BLOCK_SIZE:
      return "HCI_Read_Data_Block_Size";
    }      
    break;
 
  case OGF_STATUS_PARAMETERS:
    switch ( ocf ) {
    case 0x01:
      return "HCI_Read_Failed_Contact_Counter";
    case 0x02:
      return "HCI_Reset_Failed_Contact_Counter";
    case 0x03:
      return "HCI_Read_Link_Quality";
    case 0x05:
      return "HCI_Read_RSSI";
    case 0x06:
      return "HCI_Read_AFH_Chanel_Map";
    }    
    break;

    //  case OGF_TESTING_COMMANDS:  // 0x06

  case OGF_LE_CONTROLLER:
    switch ( ocf ) {
    case 0x01:
      return "HCI_LE_Set_Event_Mask";
    case 0x02:
      return "HCI_LE_Read_Buffer_Size";
    case 0x03:
      return "HCI_LE_Read_Local_Supported_Features";
    case 0x05:
      return "HCI_LE_Set_Random_Address";
    case 0x06:
      return "HCI_LE_Set_Advertising_Parameters";
    case 0x07:
      return "HCI_LE_Read_Advertising_Channel_Tx_Parameters";
    case 0x08:
      return "HCI_LE_Set_Advertising_Data";
    case 0x09:
      return "HCI_LE_Set_Scan_Response_Data";
    case 0x0a:
      return "HCI_LE_Set_Advertise_Enable";
    case 0x0b:
      return "HCI_LE_Set_Scan_Parameters";
    case 0x0c:
      return "HCI_LE_Set_Scan_Enable";

    case 0x0d:
      return "HCI_LE_Create_Connection";
    case 0x0e:
      return "HCI_LE_Create_Connection_Cancel";

    case 0x0f:
      return "HCI_LE_Read_White_List_Size";
    case 0x10:
      return "HCI_LE_Clear_White_List_Size";
    case 0x11:
      return "HCI_LE_Add_Device_To_White_List";
    case 0x12:
      return "HCI_LE_Remove_Device_To_White_List";
    case 0x13:
      return "HCI_LE_Connection_Update";
    case 0x14:
      return "HCI_LE_Set_Host_Channel_Classification";
    case 0x15:
      return "HCI_LE_Read_Channel_Map";
    case 0x16:
      return "HCI_LE_Read_Remote_Used_Features";
    case 0x17:
      return "HCI_LE_Encrypt";
    case 0x18:
      return "HCI_LE_Rand";
    case 0x19:
      return "HCI_LE_Start_Encryption";
    case 0x1a:
      return "HCI_LE_Long_Term_Key_Request_Reply";
    case 0x1b:
      return "HCI_LE_Long_Term_Key_Request_Negative_Reply";
    case 0x1c:
      return "HCI_LE_Read_Supported_States";
    case 0x1d:
      return "HCI_LE_Receiver_Test";
    case 0x1e:
      return "HCI_LE_Transmitter_Test";
    case 0x1f:
      return "HCI_LE_Test_End";
    case 0x20:
      return "LE_Remote_Connection_Parameter_Request_Reply";
    case 0x21:
      return "LE_Remote_Connection_Parameter_Request_Negative_Reply";
    }    
    break;
    
  case OGF_BTSTACK:
    break;    

  case OGF_VENDOR:
    break;    
  }

  sprintf(_ocfBuf, "0x%04x", ocf);
  return _ocfBuf;
}








// 0 0 0 0  0 0    1 1  0 1 1 1  0 1 0 1

// HCI Command bits 15 - 10  OGF  9 - 0  OCF

void LogTransportPacket(const char *tag, const char *usbStatus, const char *bluetoothEvent, uint8_t *packet, int size) {
  LogTransport("TransportPacket");

  /*
  uint16_t opcode = ( ( packet[1] << 8 ) | packet[0] );
  uint8_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  LogTransport("%s %s %s  opcode(0x%04x) ogf(0x%02x) ocf(0x%04x)  size(%d)   %02x %02x %02x", tag, usbStatus, bluetoothEvent, opcode, ogf, ocf, size, packet[0], packet[1], packet[2] );
  */

  // LogTransport("%s %s  0x%04x ogf(%s) ocf(0x%04x)  size(%d)   %02x %02x %02x", tag, bluetoothEvent, opcode, ogfName(ogf), ocf, size, packet[0], packet[1], packet[2] );
  //  LogTransport("%s  0x%04x ogf(%s) ocf(0x%04x)  size(%d)   %02x %02x %02x", bluetoothEvent, opcode, ogfName(ogf), ocf, size, packet[0], packet[1], packet[2] );

  /*
  //  LogTransport("LogPacket  opcode(0x%04x)  ogf(0x%02x)  ocf(0x%04x)", opcode, ogf, ocf );

  char *local_name = NULL;
  int len = 0;
  switch ( ogf ) {

  case 0:
    //LogHCI("%s  OGF = 0  size(%d)  %s", tag, size, get_packet_string(packet, size) );
    break;

  case OGF_CONTROLLER_BASEBAND:
    switch ( ocf ) {

    case HCI_WRITE_LOCAL_NAME:
      local_name = (char *)&packet[3];
      len = strlen(local_name);
      LogTransport("%s %s - %s  size(%d) len(%d)  '%s'", tag, ogfName(ogf), ocfName(ogf, ocf), size, len, local_name );
      break;

    default:
      LogTransport("%s  0x%04x  %s : %s  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
      break;
    }
    break;


  default:
    LogTransport("%s  0x%04x  ogf(%s) ocf(%s)  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
  }
  */
}


void LogConnPacket(const char *tag, uint8_t *packet, int size) {
  LogConn("%s  size: %d  %02x %02x", tag, size, packet[0], packet[1] );
  /*
  if ( size > 1 ) {
    uint8_t code = packet[0];
    if ( code != 0x6c )
      LogConn("%s  size: %d  %02x %02x", tag, size, packet[0], packet[1] );
  } else
    LogConn("%s  size: %d", tag, size);
  */

  /*
  uint16_t opcode = ( ( packet[1] << 8 ) | packet[0] );
  uint8_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  LogConn("%s  opcode(0x%04x) ogf(0x%02x) ocf(0x%04x)  size(%d)   %02x %02x %02x", tag, opcode, ogf, ocf, size, packet[0], packet[1], packet[2] );
  */

  /*
  LogConn("LogConnPacket  %02x%02x  OGF(%d)  %02x %02x %02x", ogf, packet[1], packet[0], packet[0], packet[1], packet[2] );

  //  LogConn("LogPacket  opcode(0x%04x)  ogf(0x%02x)  ocf(0x%04x)", opcode, ogf, ocf );

  char *local_name = NULL;
  int len = 0;
  switch ( ogf ) {

  case 0:
    //LogHCI("%s  OGF = 0  size(%d)  %s", tag, size, get_packet_string(packet, size) );
    break;

  case OGF_CONTROLLER_BASEBAND:
    switch ( ocf ) {

    case HCI_WRITE_LOCAL_NAME:
      local_name = (char *)&packet[3];
      len = strlen(local_name);
      LogConn("%s %s - %s  size(%d) len(%d)  '%s'", tag, ogfName(ogf), ocfName(ogf, ocf), size, len, local_name );
      break;

    default:
      LogConn("%s  0x%04x  %s : %s  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
      break;
    }
    break;


  default:
    LogConn("%s  0x%04x  ogf(%s) ocf(%s)  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
  }
  */
}


void LogHCIPacket(const char *tag, uint8_t *packet, int size) {
  uint16_t opcode = ( ( packet[1] << 8 ) | packet[0] );
  uint16_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  LogHCI("LogHCIPacket  0x%04x  %2x : %2x   %d  %02x %02x %02x", opcode, ogf, ocf, size, packet[0], packet[1], packet[2] );

  //  LogHCI("<- Send Packet       0x%04x  %s : %s    size(%d)   %02x %02x %02x", opcode, ogfName(opcode), ocfName(opcode), size, packet[0], packet[1], packet[2] );


  // LogHCI("< Send HCI Packet  0x%04x  %s : %s    size(%d)   %02x %02x %02x", opcode, ogfName(opcode), ocfName(opcode), size, packet[0], packet[1], packet[2] );
  // LogHCI("< Send HCI Packet        0x%04x  %s : %s    size(%d)   %02x %02x %02x", opcode, ogfName(opcode), ocfName(opcode), size, packet[0], packet[1], packet[2] );

  //  LogHCI("<- Send Packet       0x%04x  %s : %s    size(%d)   %02x %02x %02x", opcode, ogfName(opcode), ocfName(opcode), size, packet[0], packet[1], packet[2] );


  //  LogHCI("HCIPacket");

  /*
  uint16_t opcode = ( ( packet[1] << 8 ) | packet[0] );
  uint8_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  LogHCI("%s  opcode(0x%04x) ogf(0x%02x) ocf(0x%04x)  size(%d)   %02x %02x %02x", tag, opcode, ogf, ocf, size, packet[0], packet[1], packet[2] );
  */

  /*
  //LogHCI("LogPacket  opcode(0x%04x)  ogf(0x%02x)  ocf(0x%04x)", opcode, ogf, ocf );

  char *local_name = NULL;
  int len = 0;
  switch ( ogf ) {

  case 0:
    LogHCI("%s  OGF = 0  size(%d)  %s", tag, size, get_packet_string(packet, size) );
    break;

  case OGF_CONTROLLER_BASEBAND:
    switch ( ocf ) {

    case HCI_WRITE_LOCAL_NAME:
      local_name = (char *)&packet[3];
      len = strlen(local_name);
      LogHCI("%s %s - %s  size(%d) len(%d)  '%s'", tag, ogfName(ogf), ocfName(ogf, ocf), size, len, local_name );
      break;

    default:
      LogHCI("%s  0x%04x  %s : %s  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
      break;
    }
    break;


  default:
    LogHCI("%s  0x%04x  ogf(%s) ocf(%s)  size(%d) %s", tag, opcode, ogfName(ogf), ocfName(ogf, ocf), size, get_packet_string(packet, size) );
  }
  */
}


/*
char _cmdBuf[250];

const char *cmdName(uint16_t opcode) {
  uint8_t ogf = ( ( opcode & 0xfc00 ) >> 10 );
  uint16_t ocf = ( opcode & 0x03ff );

  sprintf(_cmdBuf, "%s : %s", ogfName(ogf), ocfName(ogf, ocf) );
  return _cmdBuf;
}
*/

// HCI Event

void LogEvent(const char *tag, uint8_t *packet, int size) {
  uint8_t code = packet[0];
  uint8_t param_len = packet[1];
  int num_cmd_packets = 0;
  uint16_t cmd_opcode = 0;

  //  LogHCI("");

  switch ( code ) {
  case DAEMON_EVENT_HCI_PACKET_SENT:
    //LogHCI("-> HCI Event         %s", hciEventName(code) );
    // LogHCI("-> HCI Event         0x%02x", code );
    return;

  case HCI_EVENT_COMMAND_COMPLETE:
    num_cmd_packets = (int)packet[2];
    cmd_opcode = ( ( packet[4] << 8 ) | packet[3] );
    // LogHCI("> Recv HCI Event   %s   #cmd: %d  opcode(0x%04x)   size(%d)  %02x %02x %02x",  hciEventName(code), num_cmd_packets, cmd_opcode, size, packet[0], packet[1], packet[2] );
    // LogHCI("<- Send Packet       0x%04x  %s : %s    size(%d)   %02x %02x %02x", opcode, ogfName(opcode), ocfName(opcode), size, packet[0], packet[1], packet[2] );
    LogHCI("-> Command_Complete  0x%04x  %s : %s  #cmd: %d  size(%d)  %02x %02x %02x",  cmd_opcode, ogfName(cmd_opcode), ocfName(cmd_opcode), num_cmd_packets, size, packet[0], packet[1], packet[2] );

    // LogHCI("> Recv HCI Event   0x%04x  %s  %s : %s  #cmd: %d  size(%d)  %02x %02x %02x",  cmd_opcode, hciEventName(code), ogfName(cmd_opcode), ocfName(cmd_opcode),
    //        num_cmd_packets, size, packet[0], packet[1], packet[2] );
    return;

    //  case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
    //    LogHCI("-> HCI Event         %s   param_len(%02x)  size(%d)  %02x %02x %02x %02x %02x %02x %02x", hciEventName(code), param_len, packet[2], size,
    //           packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6] );
    //    return;

  default:
    //LogHCI("-> HCI Event         code(0x%02x) %s   param_len(%02x)  size(%d)  %02x %02x", code, hciEventName(code), param_len, size, packet[0], packet[1] );
    // LogHCI("-> HCI Event         code(0x%02x)  0x%02x   param_len(%02x)  size(%d)  %02x %02x", code, code, param_len, size, packet[0], packet[1] );
    return;
  }

}


