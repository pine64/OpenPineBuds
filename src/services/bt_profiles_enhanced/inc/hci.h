/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/


#ifndef __HCI_H__
#define __HCI_H__

#include "string.h"
#include "btlib_type.h"
#include "stdint.h"
#include "bt_sys_cfg.h"
#include "hci_buff.h"

#ifdef IBRT
typedef void (*hci_tx_buf_tss_process_cb_type)(void);
typedef void (*bt_hci_acl_ecc_softbit_handler_func)(uint16_t*,uint16_t*, uint16_t, uint8_t*);
#endif

typedef enum hci_command_type {
    CMD_TYPE_SYNC_GLOBAL_ONLY = 0,
    CMD_TYPE_SYNC_CONN_ONLY,
    CMD_TYPE_SYNC_MULTI,
    CMD_TYPE_FREE,
} hci_command_type_t;

#define HCI_MAX_PARAM_SIZE      255
#define HCI_COMMAND_FIFO_SIZE   258                     /* size of the transmit fifo    */
/* Packet types :                                                                       */
#define HCI_CMD_PACKET          0x01
#define HCI_ACL_PACKET          0x02
#define HCI_SCO_PACKET          0x03
#define HCI_EVT_PACKET          0x04
#define HCI_BLE_PACKET          0x05//to distinush from acl to ble data
#define HCI_ACL_PACKET_TX       0x12                    /* internal: used for trace only */

/* HCI_COMMAND_STATUS_EVT_FIXED_SIZE = evtcode (1) + len (1) + status (1) + num packet (1) + opcode (2) */
#define HCI_COMMAND_STATUS_EVT_FIXED_SIZE   6

/* HCI_COMMAND_COMP_EVT_FIXED_SIZE = evtcode (1) + len (1) + num packet (1) + opcode (2) + ... */
#define HCI_COMMAND_COMP_EVT_FIXED_SIZE   5

/* HCI_COMMAND_EVT_FIXED_SIZE = evtcode (1) + len (1) + ... */
#define HCI_COMMAND_EVT_FIXED_SIZE   2

/* HCI_COMMAND_EVT_FIXED_SIZE = evtcode (1) + len (1) + msgclass(1) + msgtag(1) + ... */
#define HCI_DEBUG_EVT_FIXED_SIZE     4

#define HCI_DEBUG_GROUP_CODE         0x3F
#define HCI_DEBUG_PRINT_EVENT_CLASS  0x00

/* this variable should be use to perform a control flow for HCI commands               */
#define HCI_NUM_COMMAND_PACKETS      1

#define BLE_ACL_DATA_HDR_LEN 5//HCI_ACL_HDR_LEN + HCI_TRANSPORT_HDR_LEN

#define BLE_ADV_DATA_LENGTH 31
#define BLE_SCAN_RSP_DATA_LENGTH 31


/******************************************************************************************
   OGF opcodes group define
   hence the values
******************************************************************************************/
#define HCI_OGF_BIT_OFFSET                ((INT8U) 10) /* Number of bit shifts */


#define HCI_OPCODE_MASK                       0x03FF


/******************************************************************************************
   OCF opcode defines
******************************************************************************************/

/******************************************************************************************
   OCF opcode defines - Link Control Commands  (OGF: 0x01)
******************************************************************************************/
//#define HCI_INQUIRY                         0x0401
#define HCI_INQUIRY_CANCEL                    0x0402
#define HCI_PERIODIC_INQUIRY_MODE             0x0403
#define HCI_EXIT_PERIODIC_INQUIRY_MODE        0x0404
//#define HCI_CREATE_CONNECTION                   0x0405
//#define HCI_DISCONNECT                      0x0406
#define HCI_ADDSCO_CONN                       0x0407
//#define HCI_CREATE_CONNECTION_CANCEL          0x0408
//#define HCI_ACCEPT_CONNECTION_REQ           0x0409
//#define HCI_REJECT_CONNECTION_REQ               0x040A
//#define HCI_LINK_KEY_REQ_REPLY                  0x040B
//#define HCI_LINK_KEY_REQ_NEG_REPLY          0x040C
//#define HCI_PIN_CODE_REQ_REPLY                  0x040D
//#define HCI_PIN_CODE_REQ_NEG_REPLY          0x040E
#define HCI_CHANGE_CONN_PKT_TYPE              0x040F
//#define HCI_AUTH_REQ                        0x0411
//#define HCI_SET_CONN_ENCRYPTION                 0x0413
#define HCI_CHANGE_CONN_LINK_KEY              0x0415
#define HCI_MASTER_LINK_KEY                   0x0417
//#define HCI_REMOTE_NAME_REQ                 0x0419
//#define HCI_REMOTE_NAME_CANCEL              0x041A
#define HCI_READ_REMOTE_SUP_FEATURES          0x041B
#define HCI_READ_REMOTE_VER_INFO              0x041D
#define HCI_READ_CLOCK_OFFSET                 0x041F
#define HCI_READREMOTE_EXT_FEATURES           0x041C
#define HCI_READ_LMP_HANDLE                   0x0420
//#define HCI_SETUP_SYNC_CONN                     0x0428
//#define HCI_ACCEPT_SYNC_CONN                0x0429
//#define HCI_REJECT_SYNC_CONN                0x042A

/******************************************************************************************
   OCF opcode defines - Link Policy Commands  (OGF 0x02)
 ******************************************************************************************/
#define HCI_HOLD_MODE                             0x0801
#define HCI_PARK_MODE                             0x0805
#define HCI_EXIT_PARK_MODE                        0x0806
//#define HCI_QOS_SETUP                             0x0807
#define HCI_ROLE_DISCOVERY                        0x0809
//#define HCI_SWITCH_ROLE                           0x080B
#define HCI_READ_LP_SETTINGS                      0x080C           //LP:LINK POLICY
#define HCI_READ_DEFAULT_LP_SETTINGS              0x080E
#define HCI_WRITE_DEFAULT_LP_SETTINGS             0x080F
#define HCI_FLOW_SPECIFICATION                    0x0810

/******************************************************************************************
 OCF opcode defines -Host Controller and Baseband Commands (OGF 0x03)
******************************************************************************************/
#define HCI_SET_EVENT_MASK                    0x0C01
#define HCI_RESET                             0x0C03
#define HCI_SET_EVENT_FILTER                  0x0C05
#define HCI_FLUSH                             0x0C08
#define HCI_READ_PIN_TYPE                     0x0C09
#define HCI_WRITE_PIN_TYPE                    0x0C0A
#define HCI_CREATE_NEW_UNIT_KEY               0x0C0B
//#define HCI_DELETE_STORED_LINK_KEY          0x0C12
//#define HCI_WRITE_LOCAL_NAME                  0x0C13
#define HCI_READ_LOCAL_NAME                   0x0C14
#define HCI_READ_CONN_ACCEPT_TIMEOUT          0x0C15
#define HCI_WRITE_CONN_ACCEPT_TIMEOUT         0x0C16
#define HCI_READ_PAGE_TIMEOUT                 0x0C17
#define HCI_WRITE_PAGE_TIMEOUT                0x0C18
#define HCI_READ_SCAN_ENABLE                  0x0C19
//#define HCI_WRITE_SCAN_ENABLE                   0x0C1A
#define HCI_READ_PAGESCAN_ACTIVITY            0x0C1B
#define HCI_WRITE_PAGESCAN_ACTIVITY           0x0C1C
#define HCI_READ_INQUIRYSCAN_ACTIVITY         0x0C1D
#define HCI_WRITE_INQUIRYSCAN_ACTIVITY        0x0C1E
#define HCI_READ_AUTH_ENABLE                  0x0C1F
//#define HCI_WRITE_AUTH_ENABLE                   0x0C20
#define HCI_READ_ENCRY_MODE                   0x0C21
#define HCI_WRITE_ENCRY_MODE                  0x0C22
#define HCI_READ_CLASS_OF_DEVICE              0x0C23
//#define HCI_WRITE_CLASS_OF_DEVICE               0x0C24
#define HCI_READ_VOICE_SETTING                0x0C25
#define HCI_WRITE_VOICE_SETTING               0x0C26
#define HCI_READ_AUTO_FLUSH_TIMEOUT           0x0C27
#define HCI_WRITE_AUTO_FLUSH_TIMEOUT          0x0C28
#define HCI_READ_NUM_BCAST_RETRANSMIT         0x0C29
#define HCI_WRITE_NUM_BCAST_RETRANSMIT        0x0C2A
#define HCI_READ_HOLD_MODE_ACTIVITY           0x0C2B
#define HCI_WRITE_HOLD_MODE_ACTIVITY          0x0C2C
#define HCI_READ_TX_POWER_LEVEL               0x0C2D
#define HCI_READ_SYNC_FLOW_CON_ENABLE         0x0C2E
#define HCI_WRITE_SYNC_FLOW_CON_ENABLE        0x0C2F
#define HCI_SET_HCTOHOST_FLOW_CONTROL         0x0C31
#define HCI_HOST_BUFFER_SIZE                  0x0C33
#define HCI_HOST_NUM_COMPLETED_PACKETS        0x0C35
#define HCI_READ_LINK_SUPERV_TIMEOUT          0x0C36
//#define HCI_WRITE_LINK_SUPERV_TIMEOUT         0x0C37
#define HCI_READ_NUM_SUPPORTED_IAC            0x0C38
#define HCI_READ_CURRENT_IAC_LAP              0x0C39
#define HCI_WRITE_CURRENT_IAC_LAP             0x0C3A
#define HCI_READ_PAGESCAN_PERIOD_MODE         0x0C3B
#define HCI_WRITE_PAGESCAN_PERIOD_MODE        0x0C3C

#define HCI_READ_PAGESCAN_MODE                0x0C3D
#define HCI_WRITE_PAGESCAN_MODE               0x0C3E

#define SET_AFH_HOST_CHL_CLASSFICAT           0x0C3F
#define HCI_READ_INQUIRYSCAN_TYPE             0x0C42
#define HCI_WRITE_INQUIRYSCAN_TYPE            0x0C43
#define HCI_READ_INQUIRY_MODE                 0x0C44
#define HCI_WRITE_INQUIRY_MODE                0x0C45
#define HCI_READ_PAGESCAN_TYPE                0x0C46
//#define HCI_WRITE_PAGESCAN_TYPE               0x0C47
#define HCI_READ_AFH_CHL_ASSESS_MODE          0x0C48
#define HCI_WRITE_AFH_CHL_ASSESS_MODE         0x0C49
#define HCI_WRITE_EXTENDED_INQ_RESP           0x0C52
#define HCI_WRITE_SIMPLE_PAIRING_MODE         0x0C56
#define HCI_READ_DEF_ERR_DATA_REPORTING       0x0C5A
#define HCI_WRITE_DEF_ERR_DATA_REPORTING      0x0C5B

/******************************************************************************************
 OCF opcode defines -Information Parameters (OGF  0x04)
******************************************************************************************/
#define HCI_READ_LOCAL_VER_INFO               0x1001
#define HCI_READ_LOCAL_SUP_COMMANDS           0x1002
#define HCI_READ_LOCAL_SUP_FEATURES           0x1003
#define HCI_READ_LOCAL_EXT_FEATURES           0x1004
#define HCI_READ_BUFFER_SIZE                  0x1005
#define HCI_READ_BD_ADDR                      0x1009

/******************************************************************************************
 OCF opcode defines -Status Parameters (0GF 0X05)
******************************************************************************************/
#define HCI_READ_FAILED_CONTACT_COUNT              0x1401
#define HCI_RESET_FAILED_CONTACT_COUNT             0x1402
#define HCI_READ_LINK_QUALITY                      0x1403
#define HCI_READ_RSSI                              0x1405
#define HCI_READ_AFH_CHANNEL_MAP                   0x1406
#define HCI_READ_CLOCK                             0x1407

/******************************************************************************************
 *OCF opcode defines -Testing Commands (OGF 0X06)
******************************************************************************************/
#define HCI_READ_LOOPBACK_MODE                0x1801
#define HCI_WRITE_LOOPBACK_MODE               0x1802
#define HCI_ENABLE_DUT_MODE                   0x1803

/******************************************************************************************
 *OCF opcode defines -BLE Commands (OGF 0X08)
******************************************************************************************/
#define HCI_BLE_SET_EVENT_MASK                0x2001
#define HCI_LE_READ_BUFFER_SIZE               0x2002


/******************************************************************************************
 *OCF opcode defines -Vendor Commands (OGF 0xff)
******************************************************************************************/

/*
#define HCI_READ_LMP_PARAM               0x2001
#define HCI_SET_AFH                      0x2002
#define HCI_SET_BD_ADDR                  0x2004
#define HCI_PRJ_VERSION                  0x2005
#define HCI_GET_PKT_STATICS              0x2006
#define HCI_READ_MEMORY                  0x2007
#define HCI_WRITE_MEMORY                 0x2008
#define HCI_READ_HW_REGISTER             0x2009
#define HCI_WRITE_HW_REGISTER            0x200A
#define HCI_TEST_CONTROL                 0x200B
#define HCI_SEND_PDU                     0x2010
#define HCI_SET_SCO_CHANNEL              0x2011
#define HCI_SET_ESCO_CHANNEL             0x2012
#define HCI_DBG_OPCODE                   0x203f
#define HCI_SET_UART_BAUD_RATE           0x2013
#define HCI_SET_UART_PORT                0x2014
#define HCI_SET_CLOCK                    0x2015
#define HCI_GET_PKTS_ERR                 0x2016
#define HCI_DEEP_SLEEP                   0x2019
#define HCI_SET_SCOOVER_TYPE             0x201A
#define HCI_GET_SCOOVER_TYPE             0x201B
*/

#define HCI_READ_LMP_PARAM               0xFC01
#define HCI_SET_AFH                      0xFC02
//#define HCI_SET_BD_ADDR                    0xFC04
#define HCI_PRJ_VERSION                  0xFC05
#define HCI_GET_PKT_STATICS              0xFC06
//#define HCI_READ_MEMORY                  0xFC07
//#define HCI_WRITE_MEMORY                 0xFC08
#define HCI_READ_HW_REGISTER             0xFC09
#define HCI_WRITE_HW_REGISTER            0xFC0A
#define HCI_TEST_CONTROL                 0xFC0B
#define HCI_SEND_PDU                     0xFC10
#define HCI_SET_SCO_CHANNEL              0xFC11
#define HCI_SET_ESCO_CHANNEL             0xFC12
#define HCI_DBG_OPCODE                   0xFC3f
#define HCI_SET_UART_BAUD_RATE           0xFC13
#define HCI_SET_UART_PORT                0xFC14
#define HCI_SET_CLOCK                    0xFC15
#define HCI_GET_PKTS_ERR                 0xFC16
#define HCI_DEEP_SLEEP                   0xFC19
//#define HCI_SET_SCOOVER_TYPE             0xFC1A
#define HCI_SET_SCOOVER_TYPE             0xFC04
//#define HCI_GET_SCOOVER_TYPE             0xFC1B
#define HCI_GET_SCOOVER_TYPE             0xFC03

#define HCI_CONFIG_WRITE             0xFC1C
#define HCI_CONFIG_READ             0xFC1D
#define HCI_CONFIG_FIXED_FREQ             0xFC1E
#define HCI_CONFIG_HOP_FREQ               0xFC1F
#define HCI_GET_IVT_SECCODE             0xfc20
#define HCI_SET_IVT_SECCODE             0xfc21
#define HCI_SET_CLK_DBGMODE             0xfc22
#define HCI_SET_SLAVE_TEST_MODE         0xfc23

#define HCI_OGF_VENDOR  0x3f
#define HCI_OCF_CONFIG_WRITE     0x1C
#define HCI_OCF_CONFIG_READ      0x1D



#define HCI_EVTCODE_VENDOR  0xff


#define HCI_GET_OGF(val)  ((unsigned int)val>>10)
#define HCI_GET_OCF(val)  ((unsigned int)val & 0x3ff)
#define HCI_GET_OPCODE(ogf,ocf)  ((ogf<<10) + ocf)

#ifdef HCI_AFH
#define HCI_AFHEvt_Code     0xFD
#define HCI_ChannelEvt_Code  0xFE
#endif





/******************************************************************************************
 *OCF opcode defines -Debug Commands (OGF )
******************************************************************************************/

/* LM/HCI Errors                                                                        */
#define HCI_ERROR_NO_ERROR             0x00
#define HCI_ERROR_UNKNOWN_HCI_CMD      0x01
#define HCI_ERROR_NO_CONNECTION        0x02
#define HCI_ERROR_HARDWARE_FAILURE     0x03
#define HCI_ERROR_PAGE_TIMEOUT         0x04
#define HCI_ERROR_AUTHENTICATE_FAILURE 0x05
#define HCI_ERROR_MISSING_KEY          0x06
#define HCI_ERROR_MEMORY_FULL          0x07
#define HCI_ERROR_CONNECTION_TIMEOUT   0x08
#define HCI_ERROR_MAX_CONNECTIONS      0x09
#define HCI_ERROR_MAX_SCO_CONNECTIONS  0x0a
#define HCI_ERROR_ACL_ALREADY_EXISTS   0x0b
#define HCI_ERROR_COMMAND_DISALLOWED   0x0c
#define HCI_ERROR_LIMITED_RESOURCE     0x0d
#define HCI_ERROR_SECURITY_ERROR       0x0e
#define HCI_ERROR_PERSONAL_DEVICE      0x0f
#define HCI_ERROR_HOST_TIMEOUT         0x10
#define HCI_ERROR_UNSUPPORTED_FEATURE  0x11
#define HCI_ERROR_INVALID_HCI_PARM     0x12
#define HCI_ERROR_USER_TERMINATED      0x13
#define HCI_ERROR_LOW_RESOURCES        0x14
#define HCI_ERROR_POWER_OFF            0x15
#define HCI_ERROR_LOCAL_TERMINATED     0x16
#define HCI_ERROR_REPEATED_ATTEMPTS    0x17
#define HCI_ERROR_PAIRING_NOT_ALLOWED  0x18
#define HCI_ERROR_UNKNOWN_LMP_PDU      0x19
#define HCI_ERROR_UNSUPPORTED_REMOTE   0x1a
#define HCI_ERROR_SCO_OFFSET_REJECT    0x1b
#define HCI_ERROR_SCO_INTERVAL_REJECT  0x1c
#define HCI_ERROR_SCO_AIR_MODE_REJECT  0x1d
#define HCI_ERROR_INVALID_LMP_PARM     0x1e
#define HCI_ERROR_UNSPECIFIED_ERR      0x1f
#define HCI_ERROR_UNSUPPORTED_LMP_PARM 0x20
#define HCI_ERROR_ROLE_CHG_NOT_ALLOWED 0x21
#define HCI_ERROR_LMP_RESPONSE_TIMEOUT 0x22
#define HCI_ERROR_LMP_TRANS_COLLISION  0x23
#define HCI_ERROR_LMP_PDU_NOT_ALLOWED  0x24
#define HCI_ERROR_ENCRYP_MODE_NOT_ACC  0x25
#define HCI_ERROR_UNIT_KEY_USED        0x26
#define HCI_ERROR_QOS_NOT_SUPPORTED    0x27
#define HCI_ERROR_INSTANT_PASSED       0x28
#define HCI_ERROR_PAIR_UNITKEY_NO_SUPP 0x29
#define HCI_ERROR_NOT_FOUND            0xf1
#define HCI_ERROR_REQUEST_CANCELLED    0xf2
#define HCI_ERROR_INVALID_SDP_PDU      0xd1
#define HCI_ERROR_SDP_DISCONNECT       0xd2
#define HCI_ERROR_SDP_NO_RESOURCES     0xd3
#define HCI_ERROR_SDP_INTERNAL_ERR     0xd4
#define HCI_ERROR_STORE_LINK_KEY_ERR   0xe0


/****************************************************************************

   HCI_COMMAND, Define Parammeter Length
 *****************************************************************************/
#define HCI_INQUIRY_PARAM_LEN                    ((INT8U) 5)
#define HCI_INQUIRY_CANCEL_PARAM_LEN             ((INT8U) 0)
#define HCI_PERIODIC_INQUIRY_MODE_PARAM_LEN      ((INT8U) 9)
#define HCI_EXIT_PERIODIC_INQUIRY_MODE_PARAM_LEN ((INT8U) 0)
#define HCI_CREATE_CONNECTION_PARAM_LEN          ((INT8U) 13)
#define HCI_DISCONNECT_PARAM_LEN                 ((INT8U) 3)
#define HCI_ADD_SCO_CONNECTION_PARAM_LEN         ((INT8U) 4)
#define HCI_ACCEPT_CONNECTION_REQ_PARAM_LEN      ((INT8U) 7)
#define HCI_REJECT_CONNECTION_REQ_PARAM_LEN      ((INT8U) 7)
#define HCI_LINK_KEY_REQ_REPLY_PARAM_LEN         ((INT8U) 22)
#define HCI_LINK_KEY_REQ_NEG_REPLY_PARAM_LEN     ((INT8U) 6)
#define HCI_PIN_CODE_REQ_REPLY_PARAM_LEN         ((INT8U) 23)
#define HCI_PIN_CODE_REQ_NEG_REPLY_PARAM_LEN     ((INT8U) 6)
#define HCI_CHANGE_CONN_PKT_TYPE_PARAM_LEN       ((INT8U) 4)
#define HCI_AUTH_REQ_PARAM_LEN                   ((INT8U) 2)
#define HCI_SET_CONN_ENCRYPTION_PARAM_LEN        ((INT8U) 3)
#define HCI_CHANGE_CONN_LINK_KEY_PARAM_LEN       ((INT8U) 2)
#define HCI_MASTER_LINK_KEY_PARAM_LEN            ((INT8U) 1)
#define HCI_REMOTE_NAME_REQ_PARAM_LEN            ((INT8U) 10)
#define HCI_REMOTE_NAME_REQ_CANCEL_LEN           ((INT8U) 6)
#define HCI_READ_REMOTE_SUPP_FEATURES_PARAM_LEN  ((INT8U) 2)
#define HCI_READ_REMOTE_SUPP_FEATURES_EXT_LEN    ((INT8U) 3)
#define HCI_READ_REMOTE_VER_INFO_PARAM_LEN       ((INT8U) 2)
#define HCI_READ_CLOCK_OFFSET_PARAM_LEN          ((INT8U) 2)
#define HCI_READ_LMP_HANDLE_PARAM_LEN            ((INT8U) 2)
#define HCI_SETUP_SYNC_CONNECTION_PARAM_LEN      ((INT8U) 17)
#define HCI_ACCEPT_SYNC_CONNECTION_PARAM_LEN     ((INT8U) 21)
#define HCI_REJECT_SYNC_CONNECTION_PARAM_LEN     ((INT8U) 7)

#define HCI_HOLD_MODE_PARAM_LEN                  ((INT8U) 6)
#define HCI_SNIFF_MODE_PARAM_LEN                 ((INT8U) 10)
#define HCI_EXIT_SNIFF_MODE_PARAM_LEN            ((INT8U) 2)
#define HCI_PARK_MODE_PARAM_LEN                  ((INT8U) 6)
#define HCI_EXIT_PARK_MODE_PARAM_LEN             ((INT8U) 2)
#define HCI_QOS_SETUP_PARAM_LEN                  ((INT8U) 20)
#define HCI_ROLE_DISCOVERY_PARAM_LEN             ((INT8U) 2)
#define HCI_SWITCH_ROLE_PARAM_LEN                ((INT8U) 7)
#define HCI_READ_LINK_POLICY_SETTINGS_PARAM_LEN  ((INT8U) 2)
#define HCI_WRITE_LINK_POLICY_SETTINGS_PARAM_LEN ((INT8U) 4)
#define HCI_READ_DEF_LP_SETTINGS_PARAM_LEN       ((INT8U) 0)
#define HCI_WRITE_DEF_LP_SETTINGS_PARAM_LEN      ((INT8U) 2)

#define HCI_SET_EVENT_MASK_PARAM_LEN             ((INT8U) 8)
#define HCI_RESET_PARAM_LEN                      ((INT8U) 0)
#define HCI_SET_EVENT_FILTER_PARAM_LEN           ((INT8U) 8) /* Variable */
#define HCI_FLUSH_PARAM_LEN                      ((INT8U) 2)
#define HCI_READ_PIN_TYPE_PARAM_LEN              ((INT8U) 0)
#define HCI_WRITE_PIN_TYPE_PARAM_LEN             ((INT8U) 1)
#define HCI_CREATE_NEW_UNIT_KEY_PARAM_LEN        ((INT8U) 0)
#define HCI_READ_STORED_LINK_KEY_PARAM_LEN       ((INT8U) 7)
#define HCI_WRITE_STORED_LINK_KEY_PARAM_LEN      ((INT8U) 23) /* Variable */
#define HCI_DELETE_STORED_LINK_KEY_PARAM_LEN     ((INT8U) 7)
#define HCI_CHANGE_LOCAL_NAME_PARAM_LEN          ((INT8U) 248)
#define HCI_READ_LOCAL_NAME_PARAM_LEN            ((INT8U) 0)
#define HCI_READ_CONN_ACCEPT_TIMEOUT_PARAM_LEN   ((INT8U) 0)
#define HCI_WRITE_CONN_ACCEPT_TIMEOUT_PARAM_LEN  ((INT8U) 2)
#define HCI_READ_PAGE_TIMEOUT_PARAM_LEN          ((INT8U) 0)
#define HCI_WRITE_PAGE_TIMEOUT_PARAM_LEN         ((INT8U) 2)
#define HCI_READ_SCAN_ENABLE_PARAM_LEN           ((INT8U) 0)
#define HCI_WRITE_SCAN_ENABLE_PARAM_LEN          ((INT8U) 1)
#define HCI_READ_PAGESCAN_ACTIVITY_PARAM_LEN     ((INT8U) 0)
#define HCI_WRITE_PAGESCAN_ACTIVITY_PARAM_LEN    ((INT8U) 4)
#define HCI_READ_INQUIRYSCAN_ACTIVITY_PARAM_LEN  ((INT8U) 0)
#define HCI_WRITE_INQUIRYSCAN_ACTIVITY_PARAM_LEN ((INT8U) 4)
#define HCI_READ_AUTH_ENABLE_PARAM_LEN           ((INT8U) 0)
#define HCI_WRITE_AUTH_ENABLE_PARAM_LEN          ((INT8U) 1)
#define HCI_READ_ENC_MODE_PARAM_LEN              ((INT8U) 0)
#define HCI_WRITE_ENC_MODE_PARAM_LEN             ((INT8U) 1)
#define HCI_READ_CLASS_OF_DEVICE_PARAM_LEN       ((INT8U) 0)
#define HCI_WRITE_CLASS_OF_DEVICE_PARAM_LEN      ((INT8U) 3)
#define HCI_READ_VOICE_SETTING_PARAM_LEN         ((INT8U) 0)
#define HCI_WRITE_VOICE_SETTING_PARAM_LEN        ((INT8U) 2)
#define HCI_READ_AUTO_FLUSH_TIMEOUT_PARAM_LEN    ((INT8U) 2)
#define HCI_WRITE_AUTO_FLUSH_TIMEOUT_PARAM_LEN   ((INT8U) 4)
#define HCI_READ_NUM_BCAST_RETXS_PARAM_LEN       ((INT8U) 0)
#define HCI_WRITE_NUM_BCAST_RETXS_PARAM_LEN      ((INT8U) 1)
#define HCI_READ_HOLD_MODE_ACTIVITY_PARAM_LEN    ((INT8U) 0)
#define HCI_WRITE_HOLD_MODE_ACTIVITY_PARAM_LEN   ((INT8U) 1)
#define HCI_READ_TX_POWER_LEVEL_PARAM_LEN        ((INT8U) 3)
#define HCI_READ_SYNC_FLOW_CON_ENABLE_PARAM_LEN   ((INT8U) 0)
#define HCI_WRITE_SYNC_FLOW_CON_ENABLE_PARAM_LEN  ((INT8U) 1)
#define HCI_SET_HCTOHOST_FLOW_CONTROL_PARAM_LEN  ((INT8U) 1)
#define HCI_HOST_BUFFER_SIZE_PARAM_LEN           ((INT8U) 7)
#define HCI_HOST_NUM_COMPLETED_PACKETS_PARAM_LEN ((INT8U) 5) /* Variable */
#define HCI_READ_LINK_SUPERV_TIMEOUT_PARAM_LEN   ((INT8U) 2)
#define HCI_WRITE_LINK_SUPERV_TIMEOUT_PARAM_LEN  ((INT8U) 4)
#define HCI_READ_NUM_SUPPORTED_IAC_PARAM_LEN     ((INT8U) 0)
#define HCI_READ_CURRENT_IAC_LAP_PARAM_LEN       ((INT8U) 0)
#define HCI_WRITE_CURRENT_IAC_LAP_PARAM_LEN      ((INT8U) 4) /* Variable */
#define HCI_READ_PAGESCAN_PERIOD_MODE_PARAM_LEN  ((INT8U) 0)
#define HCI_WRITE_PAGESCAN_PERIOD_MODE_PARAM_LEN ((INT8U) 1)
#define HCI_READ_PAGESCAN_MODE_PARAM_LEN         ((INT8U) 0)
#define HCI_WRITE_PAGESCAN_MODE_PARAM_LEN        ((INT8U) 1)

#define HCI_READ_INQUIRYSCAN_TYPE_PARAM_LEN         ((INT8U) 0)
#define HCI_WRITE_INQUIRYSCAN_TYPE_PARAM_LEN        ((INT8U) 1)
#define HCI_READ_INQUIRY_MODE_PARAM_LEN             ((INT8U) 0)
#define HCI_WRITE_INQUIRY_MODE_PARAM_LEN            ((INT8U) 1)

#define HCI_READ_AFH_CHANNEL_ASSESSMENT_MODE_LEN    ((INT8U) 0)
#define HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE_LEN   ((INT8U) 1)


#define HCI_READ_LOCAL_VER_INFO_PARAM_LEN        ((INT8U) 0)
#define HCI_READ_LOCAL_SUPP_CMD_PARAM_LEN        ((INT8U) 0)
#define HCI_READ_LOCAL_SUPP_FEATURES_PARAM_LEN   ((INT8U) 0)
#define HCI_READ_LOCAL_FEATURES_EXT_PARAM_LEN    ((INT8U) 1)
#define HCI_READ_BUFFER_SIZE_PARAM_LEN           ((INT8U) 0)
#define HCI_READ_COUNTRY_CODE_PARAM_LEN          ((INT8U) 0)
#define HCI_READ_BD_ADDR_PARAM_LEN               ((INT8U) 0)

#define HCI_READ_FAILED_CONTACT_COUNT_PARAM_LEN  ((INT8U) 2)
#define HCI_RESET_FAILED_CONTACT_COUNT_PARAM_LEN ((INT8U) 2)
#define HCI_GET_LINK_QUALITY_PARAM_LEN           ((INT8U) 2)
#define HCI_READ_RSSI_PARAM_LEN                  ((INT8U) 2)
#define HCI_READ_AFH_CHANNEL_MAP_PARAM_LEN       ((INT8U) 2)
#define HCI_READ_CLOCK_PARAM_LEN                 ((INT8U) 3)

#define HCI_READ_LOOPBACK_MODE_PARAM_LEN         ((INT8U) 0)
#define HCI_WRITE_LOOPBACK_MODE_PARAM_LEN        ((INT8U) 1)
#define HCI_ENABLE_DUT_MODE_PARAM_LEN            ((INT8U) 0)

#define HCI_DEBUG_REQUEST_PARAM_LEN              ((INT8U) 2)


/******************************************************************************
        Event Parameter Lengths
 *****************************************************************************/
#define HCI_EV_INQUIRY_COMPLETE_PARAM_LEN                 ((INT8U) 1)
#define HCI_EV_INQUIRY_RESULT_PARAM_LEN                   ((INT8U) 15) /* Variable */
#define HCI_EV_CONN_COMPLETE_PARAM_LEN                    ((INT8U) 11)
#define HCI_EV_CONN_REQUEST_PARAM_LEN                     ((INT8U) 10)
#define HCI_EV_DISCONNECT_COMPLETE_PARAM_LEN              ((INT8U) 4)
#define HCI_EV_AUTH_COMPLETE_PARAM_LEN                    ((INT8U) 3)
#define HCI_EV_REMOTE_NAME_REQ_COMPLETE_MAX_LEN           ((INT8U) 255)
#define HCI_EV_REMOTE_NAME_REQ_COMPLETE_BASIC_LEN         ((INT8U) 7)
#define HCI_EV_ENCRYPTION_CHANGE_PARAM_LEN                ((INT8U) 4)
#define HCI_EV_CHANGE_CONN_LINK_KEY_COMPLETE_PARAM_LEN    ((INT8U) 3)
#define HCI_EV_MASTER_LINK_KEY_COMPLETE_PARAM_LEN         ((INT8U) 4)
#define HCI_EV_READ_REM_SUPP_FEATURES_COMPLETE_PARAM_LEN  ((INT8U) 11)
#define HCI_EV_READ_REMOTE_VER_INFO_COMPLETE_PARAM_LEN    ((INT8U) 8)
#define HCI_EV_QOS_SETUP_COMPLETE_PARAM_LEN               ((INT8U) 21)
#define HCI_EV_COMMAND_COMPLETE_PARAM_LEN                 ((INT8U) 3) /* Variable see below */
#define HCI_EV_COMMAND_STATUS_PARAM_LEN                   ((INT8U) 4)
#define HCI_EV_HARDWARE_ERROR_PARAM_LEN                   ((INT8U) 1)
#define HCI_EV_FLUSH_OCCURRED_PARAM_LEN                   ((INT8U) 2)
#define HCI_EV_ROLE_CHANGE_PARAM_LEN                      ((INT8U) 8)
#define HCI_EV_NUMBER_COMPLETED_PKTS_PARAM_LEN            ((INT8U) 5) /* Variable */
#define HCI_EV_MODE_CHANGE_PARAM_LEN                      ((INT8U) 6)
#define HCI_EV_RETURN_LINK_KEYS_PARAM_LEN                 ((INT8U) 23) /* Variable */
#define HCI_EV_PIN_CODE_REQ_PARAM_LEN                     ((INT8U) 6)
#define HCI_EV_LINK_KEY_REQ_PARAM_LEN                     ((INT8U) 6)
#define HCI_EV_LINK_KEY_NOTIFICATION_PARAM_LEN            ((INT8U) 23)
#define HCI_EV_LOOPBACK_COMMAND_PARAM_LEN                 ((INT8U) 0) /* Variable */
#define HCI_EV_DATA_BUFFER_OVERFLOW_PARAM_LEN             ((INT8U) 1)
#define HCI_EV_MAX_SLOTS_CHANGE_PARAM_LEN                 ((INT8U) 3)
#define HCI_EV_READ_CLOCK_OFFSET_COMPLETE_PARAM_LEN       ((INT8U) 5)
#define HCI_EV_CONN_PACKET_TYPE_CHANGED_PARAM_LEN         ((INT8U) 5)
#define HCI_EV_QOS_VIOLATION_PARAM_LEN                    ((INT8U) 2)
#define HCI_EV_PAGE_SCAN_MODE_CHANGE_PARAM_LEN            ((INT8U) 7)
#define HCI_EV_PAGE_SCAN_REP_MODE_CHANGE_PARAM_LEN        ((INT8U) 7)
#define HCI_EV_DEBUG_PARAM_LEN                            ((INT8U) 20 )


/******************************************************************************
   HCI_COMMAND_COMPLETE, Argument Length Definitions (not full length)
   3 From Command Complete + Return Parameters.
   When an argument length is dependant on the number of elements in the array
   the defined length contains the constant parameter lengths only. The full
   array length must be calculated.
 *****************************************************************************/
#define HCI_INQUIRY_CANCEL_ARG_LEN             ((INT8U) 1)
#define HCI_PERIODIC_INQ_MODE_ARG_LEN          ((INT8U) 1)
#define HCI_EXIT_PERIODIC_INQ_MODE_ARG_LEN     ((INT8U) 1)
#define HCI_CREATE_CONN_CANCEL_ARG_LEN          ((INT8U) 6)
#define HCI_LINK_KEY_REQ_REPLY_ARG_LEN         ((INT8U) 7)
#define HCI_LINK_KEY_REQ_NEG_REPLY_ARG_LEN     ((INT8U) 7)
#define HCI_PIN_CODE_REQ_REPLY_ARG_LEN         ((INT8U) 7)
#define HCI_PIN_CODE_REQ_NEG_REPLY_ARG_LEN     ((INT8U) 7)
#define HCI_ROLE_DISCOVERY_ARG_LEN             ((INT8U) 4)
#define HCI_READ_LINK_POLICY_SETTINGS_ARG_LEN  ((INT8U) 5)
#define HCI_WRITE_LINK_POLICY_SETTINGS_ARG_LEN ((INT8U) 3)
#define HCI_SET_SNIFF_SUBRATING_SETTINGS_ARG_LEN ((INT8U) 3)
#define HCI_SET_EVENT_MASK_ARG_LEN             ((INT8U) 1)
#define HCI_RESET_ARG_LEN                      ((INT8U) 1)
#define HCI_SET_EVENT_FILTER_ARG_LEN           ((INT8U) 1)
#define HCI_FLUSH_ARG_LEN                      ((INT8U) 4)
#define HCI_READ_PIN_TYPE_ARG_LEN              ((INT8U) 2)
#define HCI_WRITE_PIN_TYPE_ARG_LEN             ((INT8U) 1)
#define HCI_CREATE_NEW_UNIT_KEY_ARG_LEN        ((INT8U) 1)
#define HCI_READ_STORED_LINK_KEY_ARG_LEN       ((INT8U) 5)
#define HCI_WRITE_STORED_LINK_KEY_ARG_LEN      ((INT8U) 2)
#define HCI_DELETE_STORED_LINK_KEY_ARG_LEN     ((INT8U) 3)
#define HCI_CHANGE_LOCAL_NAME_ARG_LEN          ((INT8U) 1)
#define HCI_READ_LOCAL_NAME_ARG_LEN            ((INT8U) 249)
#define HCI_READ_CONN_ACCEPT_TIMEOUT_ARG_LEN   ((INT8U) 3)
#define HCI_WRITE_CONN_ACCEPT_TIMEOUT_ARG_LEN  ((INT8U) 1)
#define HCI_READ_PAGE_TIMEOUT_ARG_LEN          ((INT8U) 3)
#define HCI_WRITE_PAGE_TIMEOUT_ARG_LEN         ((INT8U) 1)
#define HCI_READ_SCAN_ENABLE_ARG_LEN           ((INT8U) 2)
#define HCI_WRITE_SCAN_ENABLE_ARG_LEN          ((INT8U) 1)
#define HCI_READ_PAGESCAN_ACTIVITY_ARG_LEN     ((INT8U) 5)
#define HCI_WRITE_PAGESCAN_ACTIVITY_ARG_LEN    ((INT8U) 1)
#define HCI_READ_INQUIRYSCAN_ACTIVITY_ARG_LEN  ((INT8U) 5)
#define HCI_WRITE_INQUIRYSCAN_ACTIVITY_ARG_LEN ((INT8U) 1)
#define HCI_READ_AUTH_ENABLE_ARG_LEN           ((INT8U) 2)
#define HCI_WRITE_AUTH_ENABLE_ARG_LEN          ((INT8U) 1)
#define HCI_READ_ENC_MODE_ARG_LEN              ((INT8U) 2)
#define HCI_WRITE_ENC_MODE_ARG_LEN             ((INT8U) 1)
#define HCI_READ_CLASS_OF_DEVICE_ARG_LEN       ((INT8U) 4)
#define HCI_WRITE_CLASS_OF_DEVICE_ARG_LEN      ((INT8U) 1)
#define HCI_READ_VOICE_SETTING_ARG_LEN         ((INT8U) 3)
#define HCI_WRITE_VOICE_SETTING_ARG_LEN        ((INT8U) 1)
#define HCI_READ_AUTO_FLUSH_TIMEOUT_ARG_LEN    ((INT8U) 5)
#define HCI_WRITE_AUTO_FLUSH_TIMEOUT_ARG_LEN   ((INT8U) 3)
#define HCI_READ_NUM_BCASTXS_ARG_LEN           ((INT8U) 2)
#define HCI_WRITE_NUM_BCASTXS_ARG_LEN          ((INT8U) 1)
#define HCI_READ_HOLD_MODE_ACTIVITY_ARG_LEN    ((INT8U) 2)
#define HCI_WRITE_HOLD_MODE_ACTIVITY_ARG_LEN   ((INT8U) 1)
#define HCI_READ_TX_POWER_LEVEL_ARG_LEN        ((INT8U) 4)
#define HCI_READ_SCO_FLOW_CON_ENABLE_ARG_LEN   ((INT8U) 2)
#define HCI_WRITE_SCO_FLOW_CON_ENABLE_ARG_LEN  ((INT8U) 1)
#define HCI_SET_HC_TO_H_FLOW_CONTROL_ARG_LEN   ((INT8U) 1)
#define HCI_HOST_BUFFER_SIZE_ARG_LEN           ((INT8U) 1)
#define HCI_HOST_NUM_COMPLETED_PKTS_ARG_LEN    ((INT8U) 1)
#define HCI_READ_LINK_SUPERV_TIMEOUT_ARG_LEN   ((INT8U) 5)
#define HCI_WRITE_LINK_SUPERV_TIMEOUT_ARG_LEN  ((INT8U) 3)
#define HCI_READ_NUM_SUPPORTED_IAC_ARG_LEN     ((INT8U) 2)
#define HCI_READ_CURRENT_IAC_LAP_ARG_LEN       ((INT8U) 5) /* Variable */
#define HCI_WRITE_CURRENT_IAC_LAP_ARG_LEN      ((INT8U) 1)
#define HCI_READ_PAGESCAN_PERIOD_MODE_ARG_LEN  ((INT8U) 2)
#define HCI_WRITE_PAGESCAN_PERIOD_MODE_ARG_LEN ((INT8U) 1)
#define HCI_READ_PAGESCAN_MODE_ARG_LEN         ((INT8U) 2)
#define HCI_WRITE_PAGESCAN_MODE_ARG_LEN        ((INT8U) 1)
#define HCI_READ_INQUIRYSCAN_TYPE_ARG_LEN      ((INT8U) 2)  /* Add for spec 1.2 */
#define HCI_WRITE_INQUIRYSCAN_TYPE_ARG_LEN     ((INT8U) 1)  /* Add for spec 1.2 */
#define HCI_READ_INQUIRY_MODE_ARG_LEN          ((INT8U) 2)  /* Add for spec 1.2 */
#define HCI_WRITE_INQUIRY_MODE_ARG_LEN         ((INT8U) 1)  /* Add for spec 1.2 */


#define HCI_READ_LOCAL_VER_INFO_ARG_LEN        ((INT8U) 9)
#define HCI_READ_LOCAL_FEATURES_LEN            ((INT8U) 9)
#define HCI_READ_BUFFER_SIZE_ARG_LEN           ((INT8U) 8)
#define HCI_READ_COUNTRY_CODE_ARG_LEN          ((INT8U) 2)
#define HCI_READ_BD_ADDR_ARG_LEN               ((INT8U) 7)
#define HCI_READ_FAILED_CONTACT_COUNT_ARG_LEN  ((INT8U) 5)
#define HCI_RESET_FAILED_CONTACT_COUNT_ARG_LEN ((INT8U) 3)
#define HCI_GET_LINK_QUALITY_ARG_LEN           ((INT8U) 4)
#define HCI_READ_RSSI_ARG_LEN                  ((INT8U) 4)
#define HCI_READ_LOOPBACK_MODE_ARG_LEN         ((INT8U) 2)
#define HCI_WRITE_LOOPBACK_MODE_ARG_LEN        ((INT8U) 1)
#define HCI_ENABLE_DUT_ARG_LEN                 ((INT8U) 1)
#define HCI_MNFR_EXTENSION_ARG_LEN             ((INT8U) 1)


/*HCI_CMD_PACKET: 0x01 + opcode(2 bytes)+length(1 byte)+data[255]*/


struct hci_cmd_packet
{
    struct list_node node;
    uint8 misc_data; /* to adjust with lmp_handle_msg */
    uint8 cmd_flag;  /*must be 0x01*/
    uint16 opcode;
    uint16 handle;
    uint8 length;
    uint8 data[1];
} __attribute__ ((packed));

struct hci_acl_packet
{
    INT16U conn_handle;
    INT16U length;
    INT8U *data;
} __attribute__ ((packed));
struct hci_sco_packet
{
    INT16U conn_handle;
    INT8U length;
    INT8U *data;
} __attribute__ ((packed));


struct hci_evt_packet
{
    uint8 evt_flag; /*must be 0x04*/
    uint8 evt_code;
    uint8 length;
    uint8 data[1];
} __attribute__ ((packed));


#define HCI_INQUIRY                                   0x0401

struct hci_cp_inquiry
{
    uint8     lap[3];
    uint8     length;
    uint8     num_rsp;
} __attribute__ ((packed));

#define HCI_CREATE_CONNECTION                 0x0405
struct hci_cp_create_conn
{
    struct bdaddr_t bdaddr;
    uint16 pkt_type;
    uint8 page_scan_repetition_mode;
    uint8 reserved;
    uint16 clk_off;
    uint8 allow_role_switch;
} __attribute__ ((packed));

#define HCI_DISCONNECT                    0x0406
struct hci_cp_disconnect
{
    uint16   handle;
    uint8     reason;
} __attribute__ ((packed));

#define HCI_ADDSCO_CONN                       0x0407
struct hci_cp_addsco_conn
{
    uint16 conn_handle;
    uint16 pkt_type;
} __attribute__ ((packed));

#define HCI_CREATE_CONNECTION_CANCEL          0x0408
struct hci_create_connection_cancel
{
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_ACCEPT_CONNECTION_REQ             0x0409
struct hci_cp_accept_conn_req
{
    struct bdaddr_t bdaddr;

    uint8     role;
} __attribute__ ((packed));

#define HCI_REJECT_CONNECTION_REQ         0x040A
struct hci_cp_reject_conn_req
{
    struct bdaddr_t bdaddr;
    uint8     reason;
} __attribute__ ((packed));
#define HCI_LINK_KEY_REQ_REPLY            0x040B
struct hci_cp_link_key_reply
{
    struct bdaddr_t bdaddr;
    uint8     link_key[16];
} __attribute__ ((packed));

#define HCI_LINK_KEY_REQ_NEG_REPLY            0x040C
struct hci_cp_link_key_neg_reply
{
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_PIN_CODE_REQ_REPLY                0x040D
struct hci_cp_pin_code_reply
{
    struct bdaddr_t  bdaddr;
    uint8     pin_len;
    uint8     pin_code[16];
} __attribute__ ((packed));

#define HCI_PIN_CODE_REQ_NEG_REPLY            0x040E
struct hci_cp_pin_code_neg_reply
{
    struct bdaddr_t  bdaddr;
} __attribute__ ((packed));
#define HCI_CHANGE_CONN_PKT_TYPE              0x040F
struct hci_cp_change_conn_pkt_type
{
    uint16 conn_handle;
    uint16 pkt_type;
} __attribute__ ((packed));


#define HCI_AUTH_REQ                      0x0411
struct hci_cp_auth_req
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_SET_CONN_ENCRYPTION                   0x0413
struct hci_cp_set_conn_encryption
{
    uint16 conn_handle;
    uint8 encryption_enable;
} __attribute__ ((packed));


#define HCI_REMOTE_NAME_REQ                   0x0419
struct hci_cp_remote_name_request
{
    struct bdaddr_t bdaddr;
    uint8 page_scan_repetition_mode;
    uint8 reserved;
    uint16 clk_off;
} __attribute__ ((packed));

#define HCI_REMOTE_NAME_CANCEL                0x041A
struct hci_cp_remote_name_cancel
{
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_REMOTE_SUPPORTED_FEATURES   0x041B
struct hci_cp_remote_supported_feat
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_REMOTE_EXTENDED_FEATURES    0x041C
struct hci_cp_remote_extended_feat
{
    uint16 conn_handle;
    uint8 page_num;
} __attribute__ ((packed));

#define HCI_REMOTE_VERSION_INFO         0x041D
struct hci_cp_remote_version_info
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_SETUP_SYNC_CONN           0x0428
struct hci_cp_setup_sync_conn
{
    uint16 conn_handle;
    uint32 tx_bandwidth;
    uint32 rx_bandwidth;
    uint16 max_latency;
    uint16 voice_setting;
    uint8 retx_effort;
    uint16 pkt_type;
} __attribute__ ((packed));

#define HCI_ACCEPT_SYNC_CONN              0x0429
struct hci_cp_accept_sync_conn
{
    struct bdaddr_t bdaddr;
    uint32 tx_bandwidth;
    uint32 rx_bandwidth;
    uint16 max_latency;
    uint16 voice_setting;
    uint8 retx_effort;
    uint16 pkt_type;
} __attribute__ ((packed));


#define HCI_REJECT_SYNC_CONN              0x042A
struct hci_cp_reject_sync_conn
{
    struct bdaddr_t bdaddr;
    uint8     reason;
} __attribute__ ((packed));

#define HCI_IO_CAPABILIRY_RESPONSE           0x042B
struct hci_cp_io_capability_request_reply
{
    struct bdaddr_t bdaddr;
    uint8  io_caps;
    uint8  oob_present;
    uint8  auth_requirements;
}__attribute__ ((packed));

#define HCI_IO_CAPABILIRY_NEGATIVE_REPLY          0x0434
struct hci_cp_io_capability_negative_reply
{
    struct bdaddr_t bdaddr;
    uint8  reason;
}__attribute__ ((packed));

#define HCI_USER_CONFIRMATION_REPLY    0x042C
#define HCI_USER_CONFIRMATION_NEG_REPLY    0x042D
struct hci_cp_usr_confirmation_reply
{
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_SNIFF_MODE                            0x0803
struct hci_cp_sniff_mode
{
    uint16 conn_handle;
    uint16 sniff_max_interval;
    uint16 sniff_min_interval;
    uint16 sniff_attempt;
    uint16 sniff_timeout;
} __attribute__ ((packed));

#define HCI_EXIT_SNIFF_MODE                       0x0804
struct hci_cp_exit_sniff_mode
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_QOS_SETUP                             0x0807
struct hci_qos_setup
{
    uint16 conn_handle;
    uint8  unused;
    uint8  service_type;
    uint32 token_rate;
    uint32 peak_bandwidth;
    uint32 latency;
    uint32 delay_variation;
} __attribute__ ((packed));

#define HCI_ROLE_DISCOVERY                        0x0809
struct hci_write_role_discovery
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_SWITCH_ROLE                       0x080B
struct hci_witch_role
{
    struct bdaddr_t bdaddr;
    uint8           role;
} __attribute__ ((packed));

#define HCI_WRITE_LP_SETTINGS                     0x080D
struct hci_cp_write_link_policy
{
    uint16 conn_handle;
    uint16 link_policy_settings;
} __attribute__ ((packed));

#define HCI_SNIFF_SUBRATING             0x0811
struct hci_cp_sniff_subrating
{
    uint16 conn_handle;
    uint16 maximum_lantency;
    uint16 minimum_remote_timeout;
    uint16 minimum_local_timeout;
} __attribute__ ((packed));


#define HCI_WRITE_SCAN_ENABLE                 0x0C1A
struct hci_cp_write_scan_enable
{
    uint8     scan_enable;
} __attribute__ ((packed));

#define HCI_WRITE_CLASS_OF_DEVICE             0x0C24
struct hci_cp_write_class_of_device
{
    uint8    class_dev[3];
} __attribute__ ((packed));

#define HCI_WRITE_LOCAL_NAME                  0x0C13
struct hci_cp_write_local_name
{
    uint8    local_name[BTM_NAME_MAX_LEN];
} __attribute__ ((packed));

// HCI_WRITE_EXTENDED_INQ_RESP
struct hci_write_extended_inquiry_response
{
    uint8  fec;
    uint8  eir[240];
} __attribute__ ((packed));

#define HCI_LOWLAYER_MONITOR                     0xFC9b
struct hci_cp_lowlayer_monitor
{
    uint16 conn_handle;
    uint8 control_flag;
    uint8 report_format;
    uint32 data_format;
    uint8 report_unit;
} __attribute__ ((packed));

#define HCI_READ_STORED_LINK_KEY              0x0C0D
struct hci_cp_read_stored_linkkey
{
    struct bdaddr_t bdaddr;
    uint8 read_all_flag;
} __attribute__ ((packed));

#define HCI_WRITE_STORED_LINK_KEY             0x0C11
#define HCI_HS_STORED_LINK_KEY                0x201C //
struct hci_cp_write_stored_linkkey
{
    uint8 num_keys_to_write;
    struct bdaddr_t bdaddr;
    uint8    link_key[16];
} __attribute__ ((packed));

#define HCI_DELETE_STORED_LINK_KEY            0x0C12
struct hci_cp_delete_stored_linkkey
{
    struct bdaddr_t bdaddr;
    uint8  delete_all_flag;
} __attribute__ ((packed));

#define HCI_WRITE_AUTH_ENABLE                 0x0C20
struct hci_cp_write_auth_enable
{
    uint8 enable_flag;
} __attribute__ ((packed));


#define HCI_SET_BD_ADDR                  0xFC72
struct hci_cp_set_bdaddr
{
    uint8 addr_type; /* 0 bt, 1 ble */
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_WRITE_MEMORY                  0xFC02
struct hci_cp_write_memory
{
    uint32 address;
    uint8  type;
    uint8  length;
    uint32 value;
} __attribute__ ((packed));

#define HCI_READ_MEMORY                  0xFC01
struct hci_cp_read_memory
{
    uint32 address;
    uint8  type;
    uint8  length;
} __attribute__ ((packed));

#define HCI_WRITE_CURRENT_IAC_LAP        0x0C3A
struct hci_write_current_iac_lap
{
    uint8  iac_lap[4];
} __attribute__ ((packed));


#define HCI_WRITE_INQSCAN_TYPE           0x0C43
struct hci_write_inqscan_type
{
    uint8  inqscan_type;
} __attribute__ ((packed));

#define HCC_DBG_WRITE_SLEEP_EXWAKEUP_EN  0xFC77


#define HCI_WRITE_INQ_MODE              0x0C45
struct hci_write_inqmode
{
    uint8  inqmode;
} __attribute__ ((packed));

#define HCI_WRITE_PAGESCAN_TYPE          0x0C47
struct hci_write_pagescan_type
{
    uint8  pagescan_type;
} __attribute__ ((packed));

//set sco path
#define HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE            0xFC8F
#define HCC_DBG_SET_SCO_SWITCH           0xFC89

#define HCI_DBG_SET_SYNC_CONFIG_CMD_OPCODE_OLD_VER            0xFC51
#define HCC_DBG_SET_SCO_SWITCH_OLD_VER           0xFC62

struct hci_set_switch_sco
{
    uint16  sco_handle;
} __attribute__ ((packed));

#define HCI_DBG_SCO_TX_SILENCE_CMD_OPCODE     0xFC94
struct hci_dbg_sco_tx_silence_cmd
{
    uint16   connhandle;
    uint8    silence_on;
} __attribute__ ((packed));

#define HCI_DBG_SNIFFER_CMD_OPCODE            0xFC95
struct hci_dbg_sniffer_interface
{
    uint16   connhandle;
    uint8    subcode;
} __attribute__ ((packed));

#define HCI_WRITE_LINK_SUPERV_TIMEOUT         0x0C37
struct hci_write_superv_timeout
{
    uint16   connhandle;
    uint16   superv_timeout;
} __attribute__ ((packed));

#define HCI_DBG_START_TWS_EXCHANGE_CMD_OPCODE 0xFC91
struct hci_start_tws_exchange
{
    uint16   tws_slave_conn_handle;
    uint16   mobile_conn_handle;
} __attribute__ ((packed));

#define HCI_DBG_SET_SNIFFER_ENV_CMD_OPCODE    0xFC8E
struct hci_set_sniffer_env
{
    uint8 sniffer_active;
    uint8 sniffer_role;
    struct bdaddr_t monitor_bdaddr;
    struct bdaddr_t sniffer_bdaddr;
} __attribute__ ((packed));

#define HCI_DBG_ENABLE_FASTACK_CMD_OPCODE     0xFCA1
struct hci_enable_fastack
{
    uint16 conn_handle;
    uint8 direction;
    uint8 enable;
} __attribute__ ((packed));

#define HCI_DBG_ENABLE_IBRT_MODE_CMD_OPCODE    0xFCA2
struct hci_ibrt_mode_op
{
    uint8 enable;
    uint8 switchOp;
} __attribute__ ((packed));

#define HCI_DBG_START_IBRT_CMD_OPCODE     0xFCA3
struct hci_start_ibrt
{
    uint16 slaveConnHandle;
    uint16 mobilConnHandle;
} __attribute__ ((packed));

#define HCI_DBG_GET_TWS_SLAVE_OF_MOBILE_RSSI_CMD_OPCODE  0xFCA4
struct hci_get_tws_slave_of_mobile_rssi
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_DBG_STOP_IBRT_CMD_OPCODE     0xFCA8
struct hci_stop_ibrt
{
    uint8 enable;
    uint8 reason;
} __attribute__ ((packed));

#define HCI_DBG_STOP_MULTI_POINT_IBRT_CMD_OPCODE         0xFCBF
struct hci_stop_multi_point_ibrt
{
    uint16 mobile_conhdl;
    uint8 reason;
} __attribute__ ((packed));

#define HCI_DBG_RESUME_IBRT_CMD_OPCODE     0xFCAC
struct hci_resume_ibrt
{
    uint8 enable;
} __attribute__ ((packed));


#define HCI_DBG_SET_LINK_LBRT_CMD_OPCODE          0xFC97
struct hci_set_link_lbrt_enable
{
    uint16 conn_handle;
    uint8  enable;
} __attribute__ ((packed));

#define HCI_DBG_IBRT_SWITCH_CMD_OPCODE          0xFCBE
struct hci_ibrt_switch
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCC_WRITE_RANDOM_ADDR               0x2005
struct hci_write_random_addr
{
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCC_WRITE_ADV_PARAMETER             0x2006
struct hci_write_adv_param
{
    uint16  interval_min;
    uint16  interval_max;
    uint8   adv_type;
    uint8   own_addr_type;
    uint8   peer_addr_type;
    struct bdaddr_t bdaddr;
    uint8   adv_chanmap;
    uint8   adv_filter_policy;
} __attribute__ ((packed));

#define HCC_WRITE_ADV_DATA                  0x2008
struct hci_write_adv_data
{
    uint8 len;
    uint8 data[BLE_ADV_DATA_LENGTH];
} __attribute__ ((packed));

#define HCC_WRITE_SCAN_RSP_DATA             0x2009
struct hci_write_scan_rsp_data
{
    uint8 len;
    uint8 data[BLE_SCAN_RSP_DATA_LENGTH];
} __attribute__ ((packed));

#define HCC_WRITE_ADV_ENABLE                0x200A
struct hci_write_adv_en
{
    uint8 en;
} __attribute__ ((packed));

#define HCC_WRITE_BLE_SCAN_PARAMETER        0x200B
struct hci_write_ble_scan_param
{
    uint8   scan_type;
    uint16  scan_interval;
    uint16  scan_window;
    uint8   own_addr_type;
    uint8   scan_filter_policy;
} __attribute__ ((packed));

#define HCC_WRITE_BLE_SCAN_ENABLE           0x200C
struct hci_write_ble_can_en
{
    uint8 scan_en;
    uint8 filter_duplicat;
} __attribute__ ((packed));


#define HCC_CLEAR_WHITE_LIST                0x2010
#define HCC_ADD_DEVICE_TO_WHITELIST         0x2011
struct hci_add_device_to_wl
{
    uint8 addr_type;
    struct bdaddr_t bdaddr;
} __attribute__ ((packed));

#define HCI_DBG_BTADDR_EXCHANGE_CMD_OPCODE  0xFC92
struct hci_tws_bdaddr_exchange
{
    uint16 conn_handle;
} __attribute__ ((packed));

#define HCI_DBG_SEND_PREFER_RATE_CMD_OPCODE 0xFCA9
struct hci_dbg_send_prefer_rate
{
    uint16 conn_handle;
    uint8 rate;
} __attribute__ ((packed));

#define HCI_DBG_SET_ECC_DATA_TEST_CMD_OPCODE  0xFCBB

#if defined(__cplusplus)
extern "C" {
#endif

int8 btlib_hcicmd_dbg_send_prefer_rate(uint16 conn_handle, uint8 rate);
int8 btlib_send_hci_cmd_direct(uint16 opcode, uint8 *param_data_ptr, uint8 param_len);
void hci_rxtx_buff_process(void);
int8 hci_simulate_event(uint8 event, uint8 *buff, uint32 buff_len);
void hci_rx_data_ind(hci_buff_t *hci_buff);
void hci_bt_tx_acl_flush(uint16 handle);
void hci_bt_command_sync_flush(uint16 handle);
void hci_ble_tx_acl_flush(uint16 handle);
#if (BTM_BLE_USE_INTERNAL_QUEUE_CACHE_ENABLE==1)
void hci_ble_rx_acl_flush(uint16 handle);
#endif
int8 hcicmd_msg_dispatch (struct hci_cmd_packet *head);
struct hci_cmd_packet *hcicmd_packet_create (uint16 opcode, uint8 param_len);
int8 hcicmd_sync_cmd_done(void);
int8 hcicmd_get_current_sync_cmd(struct hci_cmd_packet **head);
int8 btlib_send_ble_data( uint16 conn_handle, uint8 *data_ptr, uint16 data_len, uint8 *priv);
uint8 hci_receive_ble_handle(uint16 connHandle);

typedef void (*event_handle_t)(uint8 *param, uint8 *priv, uint8 *donot_free);
typedef void (*acl_handle_t)(uint16 handle, uint8 *data, uint16 data_len, uint8 *priv, uint8 *donot_free);
typedef void (*sco_handle_t)(uint16 handle, uint8 *data, uint16 data_len, uint8 *priv, uint8* donot_free);

int8 btlib_register_hci_rx_handle(event_handle_t event_handle,acl_handle_t acl_handle, sco_handle_t sco_handle);
void hci_callback(uint8 type, uint16 conn_handle, uint8 *data, uint16 length, uint8 *priv, uint8 *donot_free);
int8 btlib_hcicmd_write_controller_memory(uint32_t addr, uint32_t val,uint8_t type);
int8 btlib_hcicmd_read_controller_memory(uint32_t addr, uint32_t len,uint8_t type);

#if defined(__cplusplus)
}
#endif

#endif /* __HCI_H__ */
