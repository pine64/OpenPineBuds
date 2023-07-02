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

#ifndef __BTM_HCI_H__
#define __BTM_HCI_H__

#include "bt_sys_cfg.h"
#include "string.h"

/* HCI Error Codes */
#define HCI_STATUS_OK                               0x00
#define HCI_ERR_UNKNOWN_HCI_CMD                     0x01
#define HCI_ERR_NO_CONNECTION                       0x02
#define HCI_ERR_HARDWARE_FAILURE                    0x03
#define HCI_ERR_PAGE_TIMEOUT                        0x04
#define HCI_ERR_AUTH_FAILURE                        0x05
#define HCI_ERR_KEY_MISSING                         0x06
#define HCI_ERR_MEMORY_FULL                         0x07
#define HCI_ERR_CONN_TIMEOUT                        0x08
#define HCI_ERR_MAX_NUM_CONNS                       0x09
#define HCI_ERR_MAX_SCO_CONNS                       0x0A
#define HCI_ERR_ACL_ALREADY_EXISTS                  0x0B
#define HCI_ERR_CMD_DISALLOWED                      0x0C
#define HCI_ERR_HOST_REJ_NO_RESOURCES               0x0D
#define HCI_ERR_HOST_REJ_SECURITY                   0x0E
#define HCI_ERR_HOST_REJ_PERSONAL_DEV               0x0F
#define HCI_ERR_HOST_TIMEOUT                        0x10
#define HCI_ERR_UNSUPP_FEATUR_PARM_VAL              0x11
#define HCI_ERR_INVAL_HCI_PARM_VAL                  0x12
#define HCI_ERR_CONN_TERM_USER_REQ                  0x13
#define HCI_ERR_CONN_TERM_LOW_RESOURCES             0x14
#define HCI_ERR_CONN_TERM_POWER_OFF                 0x15
#define HCI_ERR_CONN_TERM_LOCAL_HOST                0x16
#define HCI_ERR_REPEATED_ATTEMPTS                   0x17
#define HCI_ERR_PAIRING_DISALLOWED                  0x18
#define HCI_ERR_UNKNOWN_LMP_PDU                     0x19
#define HCI_ERR_UNSUPP_REMOTE_FEATURE               0x1A
#define HCI_ERR_SCO_OFFSET_REJECTED                 0x1B
#define HCI_ERR_SCO_INTERVAL_REJECTED               0x1C
#define HCI_ERR_SCO_AIR_MODE_REJECTED               0x1D
#define HCI_ERR_INVALID_LMP_PARM                    0x1E
#define HCI_ERR_UNSPECIFIED_ERROR                   0x1F
#define HCI_ERR_UNSUPP_LMP_PARM                     0x20
#define HCI_ERR_ROLE_CHANGE_DISALLOWED              0x21
#define HCI_ERR_LMP_RESPONSE_TIMEDOUT               0x22
#define HCI_ERR_LMP_ERR_TRANSACT_COLL               0x23
#define HCI_ERR_LMP_PDU_DISALLOWED                  0x24
#define HCI_ERR_ENCRYPTN_MODE_UNACCEPT              0x25
#define HCI_ERR_UNIT_KEY_USED                       0x26
#define HCI_ERR_QOS_NOT_SUPPORTED                   0x27
#define HCI_ERR_INSTANT_PASSED                      0x28
#define HCI_ERR_PAIRING_W_UNIT_KEY_UNSUPP           0x29
#define HCI_ERR_DIFFERENT_TRANSACTION_COLLISION     0x2A
#define HCI_ERR_INSUFF_RESOURCES_FOR_SCATTER_MODE   0x2B
#define HCI_ERR_QOS_UNACCEPTABLE_PARAMETER          0x2C
#define HCI_ERR_QOS_REJECTED                        0x2D
#define HCI_ERR_CHANNEL_CLASSIF_NOT_SUPPORTED       0x2E
#define HCI_ERR_INSUFFICIENT_SECURITY               0x2F
#define HCI_ERR_PARAMETER_OUT_OF_MANDATORY_RANGE    0x30
#define HCI_ERR_SCATTER_MODE_NO_LONGER_REQUIRED     0x31
#define HCI_ERR_ROLE_SWITCH_PENDING                 0x32
#define HCI_ERR_SCATTER_MODE_PARM_CHNG_PENDING      0x33
#define HCI_ERR_RESERVED_SLOT_VIOLATION             0x34
#define HCI_ERR_SWITCH_FAILED                       0x35
#define HCI_ERR_EXTENDED_INQ_RESP_TOO_LARGE         0x36
#define HCI_ERR_SECURE_SIMPLE_PAIR_NOT_SUPPORTED    0x37
#define HCI_ERR_HOST_BUSY_PAIRING                   0x38

/* voice setting*/
#define INPUT_CODING_LINEAR 0x0000
#define INPUT_CODING_ULAW 0x0100
#define INPUT_CODING_ALAW 0x0200
#define INPUT_DATA_FORMAT_1S 0x0000
#define INPUT_DATA_FORMAT_2S 0x0040
#define INPUT_DATA_FORMAT_SIGN 0x0080
#define INPUT_DATA_FORMAT_UNSIGN 0x00C0
#define INPUT_SAMPLE_SIZE_8BITS 0x0000
#define INPUT_SAMPLE_SIZE_16BITS 0x0020
#define AIR_CODING_FORMAT_CVSD 0x0000
#define AIR_CODING_FORMAT_ULAW 0x0001
#define AIR_CODING_FORMAT_ALAW 0x0002
#define AIR_CODING_FORMAT_TRANSPARENT 0x0003
#define AIR_CODING_FORMAT_MSBC 0x0060


/* ACL */
#define HCI_PKT_TYPE_DM1  0x0008
#define HCI_PKT_TYPE_DH1  0x0010
#define HCI_PKT_TYPE_DM3  0x0400
#define HCI_PKT_TYPE_DH3  0x0800
#define HCI_PKT_TYPE_DM5  0x4000
#define HCI_PKT_TYPE_DH5  0x8000
#define HCI_PKT_TYPE_ACL  0xcc18

/* sco esco */
#define HCI_PKT_TYPE_HV1  0x0001
#define HCI_PKT_TYPE_HV2  0x0002
#define HCI_PKT_TYPE_HV3  0x0004
#define HCI_PKT_TYPE_EV3  0x0008
#define HCI_PKT_TYPE_EV4  0x0010
#define HCI_PKT_TYPE_EV5  0x0020
#define HCI_PKT_TYPE_2EV3   0x0040
#define HCI_PKT_TYPE_3EV3   0x0080
#define HCI_PKT_TYPE_2EV5   0x0100
#define HCI_PKT_TYPE_3EV5   0x0200
#define HCI_PKT_TYPE_SCO  0x003f


#define HCI_HANDLE_MASK                        0x0FFF
#define HCI_FLAG_PB_MASK                      0x3000
#define HCI_FLAG_BROADCAST_MASK       0xC000



#define BROADCAST_NONE                         0x0000
#define BROADCAST_ACTIVE                      0x4000
#define BROADCAST_PARKED                     0x8000


/*create connection param*/
#define ALLOW_ROLE_SWITCH_YES   0x01
#define ALLOW_ROLE_SWITCH_NO    0x00


/*accept connection req param*/
#define ROLE_BECAME_MASTER 0x00
#define ROLE_REMAIN_SLAVE 0x01


/* link policy */
#define HCI_LP_ENABLE_ROLE_SWITCH_MASK  0x01
#define HCI_LP_ENABLE_HOLD_MODE_MASK        0x02
#define HCI_LP_ENABLE_SNIFF_MODE_MASK       0x04
#define HCI_LP_ENABLE_PARK_MODE_MASK        0x08


#ifdef __IAG_BLE_INCLUDE__
#define HCI_BLE_SUBEVT_CODE_CONNECT_CPMPLETE    1
#endif /* __IAG_BLE_INCLUDE__ */

/******************************************************************************************
 * Event Evtcode Definition  (OLD)
 *****************************************************************************************/
#define HCI_MasterLinkKeyCompleteEvt_Code                0x0A
#define HCI_PageScanModeChangeEvt_Code                   0x1F
#define HCI_PageScanRepetitionModeChangeEvt_Code         0x20
#define HCI_InquiryResultEvt_withRSSI                    0x22

#define HCI_EV_INQUIRY_COMPLETE 0x01
 struct hci_ev_inquiry_complete {
    uint8 status;
}__attribute__ ((packed));

#define HCI_EV_INQUIRY_RESULT        0x02
#define HCI_EV_INQUIRY_RESULT_RSSI   0x22
#define HCI_EV_INQUIRY_RESULT_EXTINQ 0x2F

#define PAGE_SCAN_REPETITION_MODE_R0    0x00
#define PAGE_SCAN_REPETITION_MODE_R1    0x01
#define PAGE_SCAN_REPETITION_MODE_R2    0x02
#define PAGE_SCAN_PERIOD_MODE_P0    0x00
#define PAGE_SCAN_PERIOD_MODE_P1    0x01
#define PAGE_SCAN_PERIOD_MODE_P2    0x02
 struct hci_ev_inquiry_result {
    uint8 num_responses;
    struct bdaddr_t bdaddr;
    uint8 page_scan_repetition_mode;
    uint8 reserved1;
    uint8 reserved2;/*must be 0*/
    uint8 class_dev[3];
    uint16 clk_off;
}__attribute__ ((packed));



#define HCI_EV_CONN_COMPLETE    0x03
 struct hci_ev_conn_complete {
    uint8     status;
    uint16   handle;
    struct bdaddr_t bdaddr;
    uint8     link_type;
    uint8     encr_mode;
}__attribute__ ((packed));

#define HCI_EV_CONN_REQUEST 0x04

#define HCI_LINK_TYPE_SCO 0x00
#define HCI_LINK_TYPE_ACL 0x01
#define HCI_LINK_TYPE_ESCO 0x02
 struct hci_ev_conn_request {
    struct bdaddr_t bdaddr;
    uint8     class_dev[3];
    uint8     link_type;
}__attribute__ ((packed));

#define HCI_EV_DISCONN_COMPLETE 0x05
 struct hci_ev_disconn_complete {
    uint8     status;
    uint16   handle;
    uint8     reason;
}__attribute__ ((packed));

#define HCI_EV_AUTHENTICATION_COMPLETE  0x06
 struct hci_ev_authentication_complete {
    uint8     status;
    uint16   handle;
}__attribute__ ((packed));

#define HCI_EV_REMOTENAMEREQ_COMPLETE   0x07
#define HCI_REMOTENAME_MAX    248
 struct hci_ev_remote_name_req_complete {
    uint8    status;
    struct   bdaddr_t bdaddr;
    uint8    name[HCI_REMOTENAME_MAX];
}__attribute__ ((packed));

#define HCI_EV_ENCRYPTION_CHANGE    0x08
 struct hci_ev_encryption_change {
    uint8    status;
    uint16   conn_handle;
    uint8    encryption_enable;
}__attribute__ ((packed));


#define HCI_EV_READ_REMOTE_FEATURES 0x0B
 struct hci_ev_read_remote_features{
    uint8    status;
    uint16   conn_handle;
    uint8    features[8];
}__attribute__ ((packed));


#define HCI_EV_READ_REMOTE_VERSION  0x0C
 struct hci_ev_read_remote_version{
    uint8    status;
    uint16   conn_handle;
    uint8    lmp_version;
    uint16 manufacturer_name;
    uint16  lmp_subversion;
}__attribute__ ((packed));



#define HCI_EV_QOSSETUP_COMPLETE                     0x0D

 struct hci_ev_qossetup_complete{
    uint8    status;
    uint16   conn_handle;
    uint8    flags;
    uint8 service_type;
    uint32 token_rate;
    uint32 peak_bandwith;
    uint32 latency;
    uint32 delay_v;
}__attribute__ ((packed));


#define HCI_EV_CMD_COMPLETE 0x0e

 struct hci_ev_cmd_complete{
    uint8   num_hci_cmd_packets;
    uint16  cmd_opcode;
    uint8   param[1];
}__attribute__ ((packed));

#define HCI_EV_CMD_STATUS   0x0f
 struct hci_ev_cmd_status{
    uint8   status;
    uint8   num_hci_cmd_packets;
    uint16  cmd_opcode;
}__attribute__ ((packed));

#define HCI_EV_HARDWARE_ERROR   0x10
 struct hci_ev_hardware_error{
    uint8 hw_code;
}__attribute__ ((packed));


#define HCI_EV_ROLE_CHANGE                           0x12
 struct hci_ev_role_change{
    uint8 status;
    struct bdaddr_t bdaddr;
    uint8     new_role;
}__attribute__ ((packed));

 struct hci_ev_cmd_complete_read_stored_linkkey{
    uint8     status;
    uint16   max_num_keys;
    uint8    num_keys_read;
}__attribute__ ((packed));

 struct hci_ev_cmd_complete_le_read_buffer_size{
    uint8     status;
    uint16   le_data_packet_length;
    uint8    total_num_le_data_packets;
}__attribute__ ((packed));

 struct hci_ev_cmd_complete_role_discovery{
    uint8     status;
    uint16   connection_handle;
    uint8    current_role;
}__attribute__ ((packed));


struct hci_ev_cmd_complete_read_local_version{
    uint8 status;
    uint8 hci_version;
    uint16 hci_revision;
    uint8 lmp_version;
    uint16 manu_name;
    uint16 lmp_subversion;
}__attribute__ ((packed));

struct hci_ev_cmd_complete_read_local_sup_features{
    uint8 status;
    uint8 features[8];
}__attribute__ ((packed));

struct hci_ev_cmd_complete_read_local_ext_features{
    uint8 status;
    uint8 page_num;
    uint8 max_page_num;
    uint8 features[8];
}__attribute__ ((packed));

#define HCI_EV_NUM_OF_CMPLT 0x13
 struct hci_ev_num_of_complete{
    uint8  num_handle;
    uint16 handle;
    uint16 num_of_comp;
}__attribute__ ((packed));

 struct hci_ev_num_of_complete_item{
    uint16 handle;
    uint16 num_of_comp;
}__attribute__ ((packed));

#define HCI_EV_MODE_CHANGE  0x14

#define HCI_MODE_ACTIVE 0x00
#define HCI_MODE_HOLD 0x01
#define HCI_MODE_SNIFF 0x02
#define HCI_MODE_PARK 0x03

 struct hci_ev_mode_change {
    uint8     status;
    uint16   handle;
    uint8    current_mode;
    uint16  interval;
}__attribute__ ((packed));

#define HCI_EV_RETURN_LINKKEYS  0x15

 struct hci_ev_return_linkkeys{
    uint8     num_keys;
    struct bdaddr_t bdaddr;
    uint8   link_key[16];
}__attribute__ ((packed));

struct hci_evt_packet_t {
    uint8 evt_code;
    uint8 length;
    uint8 data[1];
}__attribute__ ((packed));

#define HCI_EV_PIN_CODE_REQ 0x16
 struct hci_ev_pin_code_req {
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_EV_LINK_KEY_REQ 0x17
 struct hci_ev_link_key_req {
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_EV_LINK_KEY_NOTIFY  0x18
 struct hci_ev_link_key_notify {
    struct bdaddr_t bdaddr;
    uint8    link_key[16];
    uint8    key_type;
}__attribute__ ((packed));

#define HCI_EV_DATABUF_OVERFLOW 0x1A
 struct hci_ev_databuf_overflow{
    uint8 link_type;
}__attribute__ ((packed));


#define HCI_EV_MAX_SLOT_CHANGE  0x1B
 struct hci_ev_max_slot_change {
    uint16 handle;
    uint8   max_slots;
}__attribute__ ((packed));

#define HCI_EV_READ_CLKOFF_Code              0x1C
 struct hci_ev_read_clkoff{
    uint8    status;
    uint16     handle;
    uint16    clkoff;
}__attribute__ ((packed));

#define HCI_EV_CONNPKT_TYPE_CHANGE           0x1D
 struct hci_ev_connpkt_type_change {
    uint8 status;
    uint16 handle;
    uint16 pkt_type;
}__attribute__ ((packed));

#define HCI_EV_QOS_VIOLATION                         0x1E
 struct hci_ev_qos_violation {
    uint16 handle;
}__attribute__ ((packed));

#define HCI_EV_FLOW_SPECIFICATION                    0x21
 struct hci_ev_flow_specification {
    uint8 status;
    uint16 handle;
    uint8 flags;
    uint8 flow_dir;
    uint8 service_type;
    uint32 token_rate;
    uint32 token_bucket;
    uint32 peak_bandwidth;
    uint32 latency;
}__attribute__ ((packed));

#define HCI_EV_READ_REMOTE_EXTFEATURES   0x23
 struct hci_ev_read_remote_extfeatures {
    uint8 status;
    uint16 handle;
    uint8 page_num;
    uint8 max_page_num;
    uint8 ext_features[8];
}__attribute__ ((packed));

#define HCI_EV_SYNC_CONN_COMPLETE   0x2c
 struct hci_ev_sync_conn_complete {
    uint8     status;
    uint16   handle;
    struct bdaddr_t bdaddr;
    uint8 link_type;
    uint8 tx_interval;
    uint8 retx_window;
    uint16 rx_pkt_length;
    uint16 tx_pkt_length;
    uint8 air_mode;
}__attribute__ ((packed));

#define HCI_EV_ENCRYPT_KEY_REFRESH_COMPLETE    0x30

#define HCI_EV_IO_CAPABILITY_REQUEST    0x31
struct hci_ev_io_capability_request {
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_EV_IO_CAPABILITY_RESPONSE    0x32
struct hci_ev_io_capability_response {
    struct bdaddr_t bdaddr;
    uint8 io_capability;
    uint8 oob_data_present;
    uint8 authentication_requirement;
}__attribute__ ((packed));

#define HCI_EV_USER_CONFIRMATION_REQUEST    0x33
struct hci_ev_user_confirmation_request {
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_EV_SIMPLE_PAIRING_COMPLETE    0x36
struct hci_ev_simple_pairing_complete{
    uint8 status;
    struct bdaddr_t bdaddr;
}__attribute__ ((packed));

#define HCI_EV_LINK_SUPERV_TIMEOUT_CHANGED    0x38

#define HCI_LE_META_EVT    0x3E
#define HCI_LE_EV_CONN_COMPLETE 0x01
#define HCI_LE_EV_ADV_REPORT    0x02

#define HCI_EV_SYNC_CONN_CHANGE                      0x2D
 struct hci_ev_sync_conn_change {
    uint8     status;
    uint16   handle;
    uint8 tx_interval;
    uint8 retx_window;
    uint16 rx_pkt_length;
    uint16 tx_pkt_length;
}__attribute__ ((packed));

#define HCI_EV_SNIFF_SUBRATING          0x2E
 struct hci_ev_sniff_subrating {
    uint8 status;
    uint16 handle;
    uint16 maximum_transmit_lantency;
    uint16 maximum_receive_lantency;
    uint16 minimum_remote_timeout;
    uint16 minimum_local_timeout;
}__attribute__ ((packed));

#define HCI_EV_DEBUG                                 0xFF
 struct hci_ev_debug {
    uint16   debug_evt_code;
    uint8 param[1];
}__attribute__ ((packed));

struct get_buffer
{
    /// length of buffer
    uint8_t length;
    /// data of 128 bytes length
    uint8_t data[128];
};

struct hci_ev_rd_mem_cmp_evt
{
    ///Status
    uint8_t status;
    ///buffer structure to return
    struct get_buffer buf;
};

/* vendor event */
#define HCI_EV_VENDOR_EVENT                (0xFF)

//sub event code
#define HCI_DBG_TRACE_WARNING_EVT_CODE      0x01
#define HCI_SCO_SNIFFER_STATUS_EVT_CODE     0x02
#define HCI_ACL_SNIFFER_STATUS_EVT_CODE     0x03
#define HCI_TWS_EXCHANGE_CMP_EVT_CODE       0x04
#define HCI_NOTIFY_CURRENT_ADDR_EVT_CODE    0x05
#define HCI_NOTIFY_DATA_XFER_EVT_CODE       0x06
#define HCI_START_SWITCH_EVT_CODE           0x09
#define HCI_LL_MONITOR_EVT_CODE             0x0A
#define HCI_DBG_LMP_MESSAGE_RECORD_EVT_SUBCODE  0x0B
#define HCI_GET_TWS_SLAVE_MOBILE_RSSI_CODE             0x0C

#endif /* __BTM_HCI_H__ */
