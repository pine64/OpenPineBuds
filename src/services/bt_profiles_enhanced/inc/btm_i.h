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
#ifndef __BTM_I_H__
#define __BTM_I_H__

#include "btlib_type.h"
#include "co_ppbuff.h"
#include "bt_sys_cfg.h"
#include "hci.h"
#include "btm_hci.h"

#define SUPPORT_AV 1
#define SUPPORT_AV_C 1
#define SUPPORT_OPP 0
#define SUPPORT_FTP 0
#define SUPPORT_SPP 1
#define SUPPORT_AG 0
#define SUPPORT_DUN	0
#define SUPPORT_HSHF 0  //sdk
#define SUPPORT_HID  0

#define SUPPORT_OBEX    0
#define SUPPORT_PBAP    0

#define SUPPORT_L2CAP_ENHANCED_RETRANS	0

#define PROFILE_NONE    0x00
#define PROFILE_HSHF    0x01
#define PROFILE_SPP     0x02
#define PROFILE_OPP     0x04
#define PROFILE_FTP     0x08
#define PROFILE_AG	    0x10
#define PROFILE_AV      0x20
#define PROFILE_DUN		0x40
#define PROFILE_AV_C    0x80

#define ASCP_CMD_BASE          0x00
#define HSHF_CMD_BASE          0x10
#define SPP_CMD_BASE           0x20
#define OPP_CMD_BASE           0x30
#define FTP_CMD_BASE           0x40
#define AV_CMD_BASE           0x50
#define	AG_CMD_BASE	0x60
#define DUN_CMD_BASE	0x70

#define ASCP_EVNT_BASE        0x80
#define HSHF_EVNT_BASE        0x90
#define SPP_EVNT_BASE         0xA0
#define OPP_EVNT_BASE         0xB0
#define FTP_EVNT_BASE         0xC0
#define MISC_EVNT_BASE        0xC0	//BUG 15 owen.liu
#define AV_EVNT_BASE         0xD0
#define	AG_EVNT_BASE	    0xE0
#define	DUN_EVNT_BASE		0xF0  //share with AV_C (used in AT)
#define AV_C_EVNT_BASE      0xF0  //share with DUN(used in ascp)

#define BT_ECODE_NO_ERROR                   0x00
#define BT_ECODE_UNKNOWN_HCI_CMD            0x01
#define BT_ECODE_NO_CONNECTION              0x02
#define BT_ECODE_HARDWARE_FAILURE           0x03
#define BT_ECODE_PAGE_TIMEOUT               0x04
#define BT_ECODE_AUTHENTICATE_FAILURE       0x05
#define BT_ECODE_MISSING_KEY                0x06
#define BT_ECODE_MEMORY_FULL                0x07
#define BT_ECODE_CONNECTION_TIMEOUT         0x08
#define BT_ECODE_MAX_CONNECTIONS            0x09
#define BT_ECODE_MAX_SCO_CONNECTIONS        0x0a
#define BT_ECODE_ACL_ALREADY_EXISTS         0x0b
#define BT_ECODE_COMMAND_DISALLOWED         0x0c
#define BT_ECODE_LIMITED_RESOURCE           0x0d
#define BT_ECODE_SECURITY_ERROR             0x0e
#define BT_ECODE_PERSONAL_DEVICE            0x0f
#define BT_ECODE_CONN_ACCEPT_TIMEOUT        0x10
#define BT_ECODE_UNSUPPORTED_FEATURE        0x11
#define BT_ECODE_INVALID_HCI_PARM           0x12
#define BT_ECODE_REMOTE_USER_TERMINATED     0x13
#define BT_ECODE_LOW_RESOURCES              0x14
#define BT_ECODE_POWER_OFF                  0x15
#define BT_ECODE_LOCAL_TERMINATED           0x16
#define BT_ECODE_REPEATED_ATTEMPTS          0x17
#define BT_ECODE_PAIRING_NOT_ALLOWED        0x18
#define BT_ECODE_UNKNOWN_LMP_PDU            0x19
#define BT_ECODE_UNSUPPORTED_REMOTE         0x1a
#define BT_ECODE_SCO_OFFSET_REJECT          0x1b
#define BT_ECODE_SCO_INTERVAL_REJECT        0x1c
#define BT_ECODE_SCO_AIR_MODE_REJECT        0x1d
#define BT_ECODE_INVALID_LMP_PARM           0x1e
#define BT_ECODE_UNSPECIFIED_ERR            0x1f
#define BT_ECODE_UNSUPPORTED_LMP_PARM       0x20
#define BT_ECODE_ROLE_CHG_NOT_ALLOWED       0x21
#define BT_ECODE_LMP_RESPONSE_TIMEOUT       0x22
#define BT_ECODE_LMP_TRANS_COLLISION        0x23
#define BT_ECODE_LMP_PDU_NOT_ALLOWED        0x24
#define BT_ECODE_ENCRYP_MODE_NOT_ACC        0x25
#define BT_ECODE_UNIT_KEY_USED              0x26
#define BT_ECODE_QOS_NOT_SUPPORTED          0x27
#define BT_ECODE_INSTANT_PASSED             0x28
#define BT_ECODE_PAIR_UNITKEY_NO_SUPP       0x29
#define BT_ECODE_NOT_FOUND                  0xf1
#define BT_ECODE_REQUEST_CANCELLED          0xf2

enum device_mode_dis_enum{
    DEVICE_MODE_DISCOVERABLE = 0x01,
    DEVICE_MODE_NON_DISCOVERABLE = 0x0
};
enum device_mode_conn_enum{
    DEVICE_MODE_CONNECTABLE = 0x01,
    DEVICE_MODE_NON_CONNECTABLE = 0x0,
    DEVICE_MODE_NO_CHANGE = 0x04
};

enum acl_pkt_boundary_enum{
    ACL_START = 0x02,
    ACL_CONTINUE = 0x01
};
enum btm_security_event_enum {
    BTM_SECURITY_AUTORITY_SUCCESS=1,
    BTM_SECURITY_AUTORITY_FAILURE
};

enum btm_l2cap_event_enum {
    BTM_EV_CONN_ACL_OPENED=1,
    BTM_EV_CONN_ACL_CLOSED,
    BTM_EV_SECURITY_AUTORITY_SUCCESS,
    BTM_EV_SECURITY_AUTORITY_FAILURE
};

enum conn_sco_stat_enum{
    BTM_CONN_SCO_OPENED=1,
    BTM_CONN_SCO_CLOSED,
    BTM_CONN_SCO_WAIT      /*the sco connection is waiting for the acl connection to be connected first*/
};

enum btm_pairing_event
{
	PAIRING_OK,
	PAIRING_TIMEOUT,
	PAIRING_FAILED,
	UNPAIR_OK
};	// change

/* enum for inquiry events */
enum btm_inquiry_event {
	INQUIRY_DONE,			// inquiry is done
	NEW_REMOTE_DEV_IND,		// found a new remote device
	INQUIRY_CANCEL_OK,      // cancel inquiry
	INQUIRY_CANCEL_FAIL
};

enum btm_name_event
{
	NAME_DONE,
	NAME_FAIL,
	NAME_CANCEL_OK,
	NAME_CANCEL_FAIL
};

typedef void (*btm_pairing_callback_t)(enum btm_pairing_event event,void *pdata);

typedef void (*btm_chip_init_ready_callback_t)(int status);

/*bt event definition to application layer*/
#define BTEVENT_INQUIRY_RESULT       1

#define BTEVENT_INQUIRY_COMPLETE     2

#define BTEVENT_INQUIRY_CANCELED     3

#define BTEVENT_LINK_CONNECT_IND     4

#define BTEVENT_SCO_CONNECT_IND      5

#define BTEVENT_LINK_DISCONNECT      6

#define BTEVENT_LINK_CONNECT_CNF     7

#define BTEVENT_LINK_CON_RESTRICT    8

#define BTEVENT_MODE_CHANGE          9

#define BTEVENT_ACCESSIBLE_CHANGE   10

#define BTEVENT_AUTHENTICATED       11

#define BTEVENT_ENCRYPTION_CHANGE   12

#define BTEVENT_SECURITY_CHANGE     13

#define BTEVENT_ROLE_CHANGE         14

#define BTEVENT_SCO_DISCONNECT      15

#define BTEVENT_SCO_CONNECT_CNF     16

#define BTEVENT_SIMPLE_PAIRING_COMPLETE       17

#define BTEVENT_REMOTE_FEATURES               18

#define BTEVENT_REM_HOST_FEATURES             19

#define BTEVENT_LINK_SUPERV_TIMEOUT_CHANGED   20

#define BTEVENT_SET_SNIFF_SUBRATING_PARMS_CNF 21

#define BTEVENT_SNIFF_SUBRATE_INFO            22

#define BTEVENT_SET_INQUIRY_MODE_CNF          23

#define BTEVENT_SET_INQ_TX_PWR_LVL_CNF        24

#define BTEVENT_SET_EXT_INQUIRY_RESP_CNF      25

#define BTEVENT_SET_ERR_DATA_REPORTING_CNF    26

#define BTEVENT_KEY_PRESSED                   27

#define BTEVENT_QOS_SETUP_COMPLETE            28

#ifdef __TWS_RECONNECT_USE_BLE__
#define BTEVENT_TWS_BLE_ADV_REPORT_EVENT      29
#endif /*  */

/**  an ACL connection has received an internal data transmit
 *  request while it is in hold, park or sniff mode. The data will still be
 *  passed to the radio in park and sniff modes. However, hold mode will
 *  block data transmit. It may be necessary to return the ACL to active
 *  mode to restore normal data transfer.
 */
#define BTEVENT_ACL_DATA_NOT_ACTIVE 99

/*
Indicate that an ACL connection is sending or receiving data
while it is in active mode. Then, keep resetting the sniff timer.
*/
#define BTEVENT_ACL_DATA_ACTIVE 98
/** Indicates that the HCI failed to initialize.
 */
#define BTEVENT_HCI_INIT_ERROR      100

#define BTEVENT_HCI_INITIALIZED     101
/** Indicates that a fatal error has occurred in the radio or the HCI transport.
 */
#define BTEVENT_HCI_FATAL_ERROR     102

/** Indicates that the HCI has been deinitialized.
 */
#define BTEVENT_HCI_DEINITIALIZED   103

/** Indicates that the HCI cannot be initialized.
 */
#define BTEVENT_HCI_FAILED          104

#define BTEVENT_HCI_COMMAND_SENT    105

/** Indicates the name of a remote device or cancellation of a name request.
 */
#define BTEVENT_NAME_RESULT         30

#define BTEVENT_SCO_DATA_IND        31

/** Outgoing SCO data has been sent and the packet is free for re-use by
 *  the application.
 */
#define BTEVENT_SCO_DATA_CNF        32

#define BTEVENT_LINK_CONNECT_REQ    33

/** Incoming link accept complete.  */
#define BTEVENT_LINK_ACCEPT_RSP     34

/** Incoming link reject complete. . */
#define BTEVENT_LINK_REJECT_RSP     35

#define BTEVENT_COMMAND_COMPLETE    36

#define BTEVENT_SCO_CONNECT_REQ     37

/** Set Audio/Voice settings complete.  */
#define BTEVENT_SCO_VSET_COMPLETE   38

/** SCO link connection process started. */
#define BTEVENT_SCO_STARTED         39

/** Select Device operation complete, "p.select" is valid. */
#define BTEVENT_DEVICE_SELECTED     40

/** The eSCO connection has changed. "p.scoConnect" is valid.
 */
#define BTEVENT_SCO_CONN_CHNG       41

/* Group: Security-related events. */

/** Indicates access request is successful. "p.secToken" is valid. */
#define BTEVENT_ACCESS_APPROVED     50

/** Indicates access request failed. "p.secToken" is valid. */
#define BTEVENT_ACCESS_DENIED       51

/** Request authorization when "errCode" is BEC_NO_ERROR.
 *  "p.remDev" is valid.
 */
#define BTEVENT_AUTHORIZATION_REQ   52

/** Request a Pin for pairing when "errCode" is BEC_NO_ERROR.
 *  "p.pinReq" is valid. If p.pinReq.pinLen is > 0 then SEC_SetPin()
 *  must be called in response to this event with a pin length >=
 *  p.pinReq.pinLen.
 */
#define BTEVENT_PIN_REQ             53

/** Pairing operation is complete.
 */
#define BTEVENT_PAIRING_COMPLETE    54

/** Authentication operation complete. "p.remDev" is valid. */
#define BTEVENT_AUTHENTICATE_CNF    55

/** Encryption operation complete. "p.remDev" is valid. */
#define BTEVENT_ENCRYPT_COMPLETE    56

/** Security mode 3 operation complete. "p.secMode" is valid. */
#define BTEVENT_SECURITY3_COMPLETE  57

/** A link key is returned. "p.bdLinkKey" is valid.  */
#define BTEVENT_RETURN_LINK_KEYS    58

/** Out of Band data has been received from the host controller. */
#define BTEVENT_LOCAL_OOB_DATA      59

/** Request a Pass Key for simple pairing when "errCode" is BEC_NO_ERROR. The
 *  application should call SEC_SetPassKey() to provide the passkey or reject
 *  the request, and optionally save the link key.
 */
#define BTEVENT_PASS_KEY_REQ        60

/** Request a User Confirmation for simple pairing when "errCode" is
 *  BEC_NO_ERROR.
 */

#define BTEVENT_CONFIRM_NUMERIC_REQ 61

#define BTEVENT_DISPLAY_NUMERIC_IND 62

#define BTEVENT_CONN_PACKET_TYPE_CHNG   63

#define SDEVENT_QUERY_RSP           70

#define SDEVENT_QUERY_ERR           71

#define SDEVENT_QUERY_FAILED        72

#define BTEVENT_SELECT_DEVICE_REQ   80

#define BTEVENT_DEVICE_ADDED        81

#define BTEVENT_DEVICE_DELETED      	82

#define BTEVENT_MAX_SLOT_CHANGED		83

#define BTEVENT_SNIFFER_CONTROL_DONE 	84

#define BTEVENT_LINK_POLICY_CHANGED		85

/* added for pass command status to up level */
#define BTEVENT_COMMAND_STATUS	    	86

typedef uint32_t event_mask_t;

#define BTM_EVTMASK_NO_EVENTS                    0x00000000
#define BTM_EVTMASK_INQUIRY_RESULT               0x00000001
#define BTM_EVTMASK_INQUIRY_COMPLETE             0x00000002
#define BTM_EVTMASK_INQUIRY_CANCELED             0x00000004
#define BTM_EVTMASK_LINK_CONNECT_IND             0x00000008
#define BTM_EVTMASK_SCO_CONNECT_IND              0x00000010
#define BTM_EVTMASK_LINK_DISCONNECT              0x00000020
#define BTM_EVTMASK_LINK_CONNECT_CNF             0x00000040
#define BTM_EVTMASK_LINK_CON_RESTRICT            0x00000080
#define BTM_EVTMASK_MODE_CHANGE                  0x00000100
#define BTM_EVTMASK_ACCESSIBLE_CHANGE            0x00000200
#define BTM_EVTMASK_AUTHENTICATED                0x00000400
#define BTM_EVTMASK_ENCRYPTION_CHANGE            0x00000800
#define BTM_EVTMASK_SECURITY_CHANGE              0x00001000
#define BTM_EVTMASK_ROLE_CHANGE                  0x00002000
#define BTM_EVTMASK_SCO_DISCONNECT               0x00004000
#define BTM_EVTMASK_SCO_CONNECT_CNF              0x00008000
#define BTM_EVTMASK_SIMPLE_PAIRING_COMPLETE      0x00010000
#define BTM_EVTMASK_REMOTE_FEATURES              0x00020000
#define BTM_EVTMASK_REM_HOST_FEATURES            0x00040000
#define BTM_EVTMASK_LINK_SUPERV_TIMEOUT_CHANGED  0x00080000
#define BTM_EVTMASK_SET_SNIFF_SUBR_PARMS         0x00100000
#define BTM_EVTMASK_SNIFF_SUBRATE_INFO           0x00200000
#define BTM_EVTMASK_SET_INQ_MODE                 0x00400000
#define BTM_EVTMASK_SET_INQ_RSP_TX_PWR           0x00800000
#define BTM_EVTMASK_SET_EXT_INQ_RESP             0x01000000
#define BTM_EVTMASK_SET_ERR_DATA_REP             0x02000000
#define BTM_EVTMASK_KEY_PRESSED                  0x04000000
#define BTM_EVTMASK_CONN_PACKET_TYPE_CHNG        0x08000000
#define BTM_EVTMASK_QOS_SETUP_COMPLETE           0x10000000
#define BTM_EVTMASK_MAX_SLOT_CHANGED             0x20000000
#define BTM_EVTMASK_SNIFFER_CONTROL_DONE         0x40000000
#define BTM_EVTMASK_LINK_POLICY_CHANGED	         0x80000000
#define BTM_EVTMASK_ALL_EVENTS                   0xffffffff

#define BTM_ACL_ST_DISCONNECTED  0x00
#define BTM_ACL_ST_OUT_CON       0x01
#define BTM_ACL_ST_IN_CON        0x02
#define BTM_ACL_ST_CONNECTED     0x03
#define BTM_ACL_ST_OUT_DISC      0x04
#define BTM_ACL_ST_OUT_DISC2     0x05
#define BTM_ACL_ST_OUT_CON2      0x06

#define BTM_CMGR_INIT_REQ       0x00
#define BTM_CMGR_DEINIT_REQ     0x01

/*  event param of BTEVENT_LINK_CONNECT_IND*/
struct acl_open_data{
    struct btm_conn_item_t* conn; //acl connection
    uint8 err_code;
    uint32 emask;
};
struct conn_req_data{
    struct btm_conn_item_t *conn;
    uint32   emask;
};
struct acl_close_data{
    struct btm_conn_item_t *conn;
    struct hci_ev_disconn_complete *param;
    uint8 status;
    uint8 reason;
    uint32 emask;
};

struct sco_open_data{
    struct btm_sco_conn_item_t* conn;
    uint8 err_code;
    uint32 emask;
};

struct sco_close_data{
    struct btm_sco_conn_item_t *conn;
    struct hci_ev_disconn_complete *param;
    uint8 status;
    uint8 reason;
    uint32 emask;
};

struct accessible_change_data{
    enum device_mode_dis_enum discoverable;
    enum device_mode_conn_enum connectable;
    uint8 err_code;
    uint32 emask;
};

struct inquiry_cancel_data{
    uint8 err_code;
    uint32 emask;
};

struct command_status_data{
	uint8  status;
	uint8  num_hci_cmd_packets;
	uint16 cmd_opcode;
    uint32 emask;
};

struct role_change_data{
    struct btm_conn_item_t *rem_dev;
	uint8 new_role;
    uint8 err_code;
    uint32 emask;
};

struct mode_change_data{
    struct btm_conn_item_t *rem_dev;
	uint8 cur_mode;
    uint16 interval;
    uint8 err_code;
    uint32 emask;
};

struct acl_data_active{
    struct btm_conn_item_t *rem_dev;
    uint16 data_len;
    uint8 err_code;
    uint32 emask;
};

struct acl_data_not_active{
    struct btm_conn_item_t *rem_dev;
    uint16 data_len;
    uint8 err_code;
    uint32 emask;
};

struct authenticaion_complete_data{
    struct btm_conn_item_t *rem_dev;
    uint32 emask;
    uint8 err_code;
};

struct simple_pairing_complete_data{
    struct btm_conn_item_t *rem_dev;
    uint32 emask;
    uint8 err_code;
};

struct encryption_change_data{
    struct btm_conn_item_t *rem_dev;
	uint8 mode;
    uint32 emask;
    uint8 err_code;
};

struct inquiry_result_data{
    uint8 *data;
    uint8 is_rssi;
    uint8 is_extinq;
    uint8 err_code;
    uint32 emask;
};

struct name_rsp_data{
    uint8    status;
    struct   bdaddr_t bdaddr;
    uint32   emask;
};
struct inquiry_complete_data{
    uint8 status;
    uint8 err_code;
    uint32 emask;
};

struct btm_ctx_input {
    struct bdaddr_t *remote;
    struct ctx_content ctx;
    uint16 conn_handle;
};

struct btm_ctx_output {
    uint16 conn_handle;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct l2cap_channel;

int8 btm_init(void);

int8 btm_register_chip_init_ready_callback(btm_chip_init_ready_callback_t cb);

int8 btm_device_mode_set(enum device_mode_dis_enum discoverable, enum device_mode_conn_enum connectable);
int8 btm_device_write_iac(uint8 num);
int8 btm_device_write_page_scan_activity(uint16 interval, uint16 window);
int8 btm_device_write_inquiry_scan_activity(uint16 interval, uint16 window);

int8 btm_conn_sco_is_open (struct btm_conn_item_t *conn);
uint16 btm_conn_acl_get_conn_handle ( struct bdaddr_t *remote_bdaddr);


int8 btm_conn_acl_req ( struct bdaddr_t *remote_bdaddr);
int8 btm_conn_acl_is_open( struct bdaddr_t *remote_bdaddr);

int8 btm_conn_sco_req ( struct bdaddr_t *remote_bdaddr);

int8 btm_conn_sco_disconnect(struct bdaddr_t *remote_bdaddr, uint8 reason);  //added by zmchen on 20070205
int8 btm_conn_acl_senddata (struct bdaddr_t *remote_bdaddr, uint8 *data_buf_p, uint16 data_buf_len);

int8 btm_conn_acl_send_ppb (uint16 conn_handle, struct pp_buff *ppb);
int8 btm_conn_acl_send_continue_ppb (uint16 conn_handle, struct pp_buff *ppb);

void btm_conn_acl_fake_connection_complete(uint8_t status, struct bdaddr_t *bdAddr);
int8 btm_conn_acl_close(struct bdaddr_t *bdaddr, uint8 reason);

int8 btm_security_askfor_authority(uint16 conn_handle, uint16 psm, struct l2cap_channel *l2cap_channel);

void btm_pairing_enter(uint32 pairing_timeout, btm_pairing_callback_t callback);

/* for pairing that needs remote device address */
void btm_start_pairing(	struct bdaddr_t remote_addr, btm_pairing_callback_t callback);

void btm_unpair_reomte(struct bdaddr_t remote_addr, btm_pairing_callback_t callback);

void btm_pairing_register_callback(btm_pairing_callback_t callback);

void btm_pairing_exit(void);

void btm_direct_set_audio_param(uint8 param);
uint8 btm_get_audio_default_param(void);
void btm_set_audio_default_param(uint8 param);

/* inquiry functions */
void btm_start_inquiry(void (*btm_start_inquiry_notify_callback)(enum btm_inquiry_event event, void *pData), uint8 inq_period, uint8 num_rsp);
void btm_cancel_inquiry(void (*btm_start_inquiry_notify_callback)(enum btm_inquiry_event event, void *pData));
void btm_remote_name_request(struct hci_cp_remote_name_request *req, void (*btm_name_notify_callback)(enum btm_name_event event,void *pData));
void btm_remote_name_cancel(struct bdaddr_t *remote, void (*btm_name_notify_callback)(enum btm_name_event event,void *pData));

void btm_hcicmd_sniff_mode(struct bdaddr_t *remote_bdaddr, uint16 sniff_max_interval,uint16 sniff_min_interval, uint16 sniff_attempt,uint16 sniff_timeout);
int8 btm_exit_sniff (struct bdaddr_t *remote_bdaddr);

typedef void (*bt_hci_vendor_event_handler_func)(uint8_t* pbuf, uint32_t length);
void register_hci_vendor_event_handler_callback(bt_hci_vendor_event_handler_func func);

typedef bool (*bt_hci_sync_airmode_check_ind_func)(uint8_t status);
void register_hci_sync_airmode_check_ind_callback(bt_hci_sync_airmode_check_ind_func func);

uint32 btm_save_ctx(struct bdaddr_t *remote, uint8_t *buf, uint32_t buf_len);
uint32 btm_restore_ctx(struct btm_ctx_input *input, struct btm_ctx_output *output);

int8 btm_sync_conn_audio_connected(struct bdaddr_t *bdAddr, uint16_t conhdl);
int8 btm_sync_conn_audio_disconnected(struct bdaddr_t *bdAddr, uint16_t conhdl);

int8 btm_acl_conn_connected(struct bdaddr_t *bdAddr, uint16 conn_handle);
int8 btm_acl_conn_disconnected(struct bdaddr_t *bdAddr, uint16 conn_handle, uint8_t status, uint8_t reason);

const char *btm_state2str(uint8 state);
const char *btm_errorcode2str(uint32 errorcode);
const char *btm_cmd_opcode2str(uint16 opcode);

struct btstack_chip_config_t
{
    uint16_t hci_dbg_set_sync_config_cmd_opcode;
    uint16_t hci_dbg_set_sco_switch_cmd_opcode;
};

struct btstack_chip_config_t* btm_get_btstack_chip_config(void);
void btm_set_btstack_chip_config(void* config);
int8 btm_device_set_ecc_ibrt_data_test(uint8  ecc_data_test_en, uint8 ecc_data_len, uint16 ecc_count, uint32 data_pattern);
int8 btm_device_send_prefer_rate(uint16 connhdl, uint8 rate);
#if defined(__cplusplus)
}
#endif

#endif /* __BTM_I_H__ */
