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


#ifndef __BTM_H__
#define __BTM_H__

#include "bt_sys_cfg.h"
#include "btlib_type.h"
#include "bt_co_list.h"
#include "btm_devicedb.h"
#include "btm_security.h"
#include "btm_i.h"

struct btm_device_mode_t
{
    enum device_mode_dis_enum discoverable;
    enum device_mode_conn_enum connectable;
};

struct btm_inquiry_result_item_t
{
    struct list_node list;

    struct bdaddr_t remote;

    uint16 timeout_count;  /*=0: disable , count down to 1: timeout happen*/

    /* received in inquiry result */
    uint8 page_scan_repetition_mode;
    uint8 page_scan_period_mode;
    uint8 class_dev[3];
    uint16 clk_off;
};

typedef uint8  connection_role;

#define BCR_MASTER   0x00
#define BCR_SLAVE    0x01
#define BCR_ANY      0x02

#define GAP_INVALID_CONIDX                      0xFF
#define BTM_MAX_LINK_NUMS                       0x02
#define BTM_MAX_xSCO_NUMS                       0x02

#define BTM_FEAT_3SLOT_PACKETS      0,0,0x01
#define BTM_FEAT_5SLOT_PACKETS      0,0,0x02
#define BTM_FEAT_ENCRYPTION         0,0,0x04
#define BTM_FEAT_SLOT_OFFSET        0,0,0x08
#define BTM_FEAT_TIMING_ACC         0,0,0x10
#define BTM_FEAT_ROLE_SWITCH        0,0,0x20
#define BTM_FEAT_HOLD_MODE          0,0,0x40
#define BTM_FEAT_SNIFF_MODE         0,0,0x80
#define BTM_FEAT_PWR_CTRL_REQ       0,1,0x02
#define BTM_FEAT_CQDDR              0,1,0x04
#define BTM_FEAT_SCO_LINK           0,1,0x08
#define BTM_FEAT_HV2_PACKETS        0,1,0x10
#define BTM_FEAT_HV3_PACKETS        0,1,0x20
#define BTM_FEAT_ULAW_SYNC_DATA     0,1,0x40
#define BTM_FEAT_ALAW_SYNC_DATA     0,1,0x80
#define BTM_FEAT_CVSD_SYNC_DATA     0,2,0x01
#define BTM_FEAT_PAGE_PARA_NEGO     0,2,0x02
#define BTM_FEAT_PWR_CTRL           0,2,0x04
#define BTM_FEAT_TRANS_SYNC_DATA    0,2,0x08
#define BTM_FEAT_FLOW_CTRL_LST_BIT  0,2,0x10
#define BTM_FEAT_FLOW_CTRL_MID_BIT  0,2,0x20
#define BTM_FEAT_FLOW_CTRL_MST_BIT  0,2,0x40
#define BTM_FEAT_BROADCAST_ENCRYT   0,2,0x80
#define BTM_FEAT_EDR_2M_MODE        0,3,0x02
#define BTM_FEAT_EDR_3M_MODE        0,3,0x04
#define BTM_FEAT_ENHANCED_ISCAN     0,3,0x08
#define BTM_FEAT_INTERLACED_ISCAN   0,3,0x10
#define BTM_FEAT_INTERLACED_PSCAN   0,3,0x20
#define BTM_FEAT_RSSI_WITH_INQRES   0,3,0x40
#define BTM_FEAT_ESCO_LINK          0,3,0x80
#define BTM_FEAT_EV4_PACKETS        0,4,0x01
#define BTM_FEAT_EV5_PACKETS        0,4,0x02
#define BTM_FEAT_AFH_CAPAB_SLAVE    0,4,0x08
#define BTM_FEAT_AFH_CLASS_SLAVE    0,4,0x10
#define BTM_FEAT_BREDR_NOT_SUPP     0,4,0x20
#define BTM_FEAT_LE_CTRL_SUPP       0,4,0x40
#define BTM_FEAT_3SLOT_EDR_ACL      0,4,0x80
#define BTM_FEAT_5SLOT_EDR_ACL      0,5,0x01
#define BTM_FEAT_SNIFF_SUBRATING    0,5,0x02
#define BTM_FEAT_PAUSE_ENCRYPT      0,5,0x04
#define BTM_FEAT_AFH_CAPAB_MASTER   0,5,0x08
#define BTM_FEAT_AFH_CLASS_MASTER   0,5,0x10
#define BTM_FEAT_EDR_ESCO_2M_MODE   0,5,0x20
#define BTM_FEAT_EDR_ESCO_3M_MODE   0,5,0x40
#define BTM_FEAT_3SLOT_EDR_ESCO     0,5,0x80
#define BTM_FEAT_EXTENDED_INQRES    0,6,0x01
#define BTM_FEAT_SIMU_LE_BREDR_CTRL 0,6,0x02
#define BTM_FEAT_SECURE_SIMPLE_PAIR 0,6,0x08
#define BTM_FEAT_ENCAPSULATED_PDU   0,6,0x10
#define BTM_FEAT_ERR_DATA_REPORT    0,6,0x20
#define BTM_FEAT_NONFLUSH_PBF       0,6,0x40
#define BTM_FEAT_LINKSUPTO_CHANGE   0,7,0x01
#define BTM_FEAT_INQ_TX_PWR_LEVEL   0,7,0x02
#define BTM_FEAT_ENHANCED_PWR_CTRL  0,7,0x04
#define BTM_FEAT_EXTENDED_FEATURES  0,7,0x80
#define BTM_FEAT_SSP_HOST_SUPP      1,0,0x01
#define BTM_FEAT_LE_HOST_SUPP       1,0,0x02
#define BTM_FEAT_SIMU_LE_BREDR_HOST 1,0,0x04
#define BTM_FEAT_SEC_CONN_HOST_SUPP 1,0,0x08
#define BTM_FEAT_SLAVE_BROAD_MSTOP  2,0,0x01
#define BTM_FEAT_SLAVE_BROAD_SLVOP  2,0,0x02
#define BTM_FEAT_SYNCHRON_TRAIN     2,0,0x04
#define BTM_FEAT_SYNCHRON_SCAN      2,0,0x08
#define BTM_FEAT_INQRES_NOTIFY      2,0,0x10
#define BTM_FEAT_GENERAL_INTERSCAN  2,0,0x20
#define BTM_FEAT_COARSE_CLOCK_ADJ   2,0,0x40
#define BTM_FEAT_SEC_CONN_CTRL_SUPP 2,1,0x01
#define BTM_FEAT_PING               2,1,0x02
#define BTM_FEAT_TRAIN_NUDGING      2,1,0x08
#define BTM_FEAT_SLOT_AVAIL_MASK    2,1,0x10

#define BTM_MAX_FEATURE_PAGE (3)

struct btm_feature_t
{
    uint8 max_page;
    uint8 feature[8];
};

#define BTM_AUTH_WAIT_CMPL 0x01 
#define BTM_AUTH_WAIT_MSS  0x02
#define REMOTE_VERSION_LEN 5

struct btm_conn_item_t
{
    struct list_node list;
    struct list_node sco_conn_list;
    struct pp_buff_head   tx_queue;
    struct pp_buff * ppb_recv;

    struct bdaddr_t remote;
    void *cmgr_handler;
    uint16 conn_handle;

    uint8 used;
    /*1: positive connet to the remote or 0: negtive be connected*/
    uint8 positive;

    /* received in inquiry result */
    uint8 page_scan_repetition_mode;
    uint16 clk_off;

#if CFG_BTM_DISC_ACL_IN_BTM_TIMER==1
    uint8 disconn_flag;  /*if this is need to disconn*/
    uint8 disconn_count; /*to undercount to disconn*/
#endif
    uint8 discReason_saved;
    uint8 discReason;
    uint8 lowpower_flag;  /*if is lowpower. decided to buffer tx data*/
    uint8 lowpower_count; /*to count when to enter lowpower*/
    uint8 sniff_count;  /*to count how many sniff req sent*/
    uint8 role_switch_pending;
    uint8 authen_enable_flag;
    uint8 authen_pending;
    uint8 encry_enable_flag;/*tell if the entryption is enabled in this acl conn*/
    uint8 encry_need_flag;/*tell if the entryption is need*/
    connection_role role;
    uint8 state;
    uint8 mode;
    uint8 authState;
	uint8 io_cap;
	uint8 oob_present;
	uint8 authen_requirement;
    uint8 conn_dev_idx;
    uint16 rx_complete_count; /* Host HCI RX packets complete number, controller to host flow control */
    uint8 remote_version[5];
    struct btm_feature_t remote_feature[BTM_MAX_FEATURE_PAGE];
#if (BTM_HCI_HOST_FLOW_CONTROL_ENABLE == 1)
    uint8 fc_bt_tx_acl_unconfirmed;
#endif
#if (CFG_BTM_USE_INPLACE_BUFFER_FOR_ACL==1)
    uint8 *btm_acl_inplace_buff;
#endif
};
    
#define IS_REMOTE_FEAT_SUPPORT(conn, FEAT_MASK) \
    btm_is_remote_feature_support(conn, FEAT_MASK)

struct btm_sco_conn_item_t
{
    struct list_node list;
    struct btm_conn_item_t *conn;  /*acl connection*/
    uint16 conn_handle;
    uint8 link_type;  /* HCI_LINK_TYPE_ESCO or HCI_LINK_TYPE_SCO */
    enum conn_sco_stat_enum status;
    connection_role role;
    uint8 index;
    uint8 used;
};

enum btm_stack_state {
    BTM_STACK_Initializing = 0,
    BTM_STACK_Ready = 1,
};

enum btm_stack_init_sub_state {
    BTM_INIT_ST_RESET = 0,
    BTM_INIT_ST_SET_VOICE_SETTTING,
    BTM_INIT_ST_READ_BUFFER_SIZE, 
    BTM_INIT_ST_LE_READ_BUFFER_SIZE,
    BTM_INIT_ST_HOST_BUFFER_SIZE, 
    BTM_INIT_ST_SET_HCITOHOST_FLOW_CONTROL, 
    BTM_INIT_ST_WRITE_PAGE_TIEMOUT, 
    BTM_INIT_ST_READ_PAGE_TIMEOUT, 
    BTM_INIT_ST_SET_BLE_ADDRESS, 
    BTM_INIT_ST_SET_BD_ADDRESS, 
    BTM_INIT_ST_SET_EVENT_MASK, 
    BTM_INIT_ST_SET_BLE_EVENT_MASK, 
    BTM_INIT_ST_READ_LOCAL_VER_INFO, 
    BTM_INIT_ST_READ_LOCAL_SUP_COMMANDS, 
    BTM_INIT_ST_READ_LOCAL_FEATURES, 
    BTM_INIT_ST_READ_LOCAL_EXT_FEATURES, 
    BTM_INIT_ST_READ_LOCAL_EXT_FEATURES_1, 
    BTM_INIT_ST_READ_LOCAL_EXT_FEATURES_2, 
    BTM_INIT_ST_READ_BD_ADDRESS, 
    BTM_INIT_ST_READ_INQUIRY_MODE, 
    BTM_INIT_ST_READ_DEF_ERR_DATA_REPORTING, 
    BTM_INIT_ST_WRITE_SAMPLE_PAIRING_MODE, 
    BTM_INIT_ST_WRITE_CLASS_OF_DEVICE, 
    BTM_INIT_ST_WRITE_LOCAL_NAME, 
    BTM_INIT_ST_WRITE_SYNC_CONFIG, 
    BTM_INIT_ST_WRITE_DEF_ERR_DATA_REPORTING, 
    BTM_INIT_ST_WRITE_DEFAULT_LP_SETTINGS, 
    BTM_INIT_ST_WRITE_PAGESCAN_ACTIVITY, 
    BTM_INIT_ST_WRITE_INQUIRYSCAN_ACTIVITY, 
    BTM_INIT_ST_WRITE_INQUIRYSCAN_TYPE, 
    BTM_INIT_ST_WRITE_PAGESCAN_TYPE,     

    BTM_INIT_ST_NUM,
};

struct btm_sync_conn_param {
    uint16 max_latency;
    uint16 packet_type;
    uint16 voice_setting;
    uint8  retrans_effort;
    uint32 receive_bandwidth;
    uint32 transmit_bandwidth;    
};

struct btm_ctrl_t {
    enum btm_stack_state stack_state;
    enum btm_stack_init_sub_state init_sub_state;
    uint16 init_sub_state_opcode;
    uint8 pairing_flag;  /*tell whether the device in pairing state, 1:yes, 0:no*/
    uint32 pairing_timeout;
    void (*btm_pairing_notify_callback)(enum btm_pairing_event event,void *pdata);
    uint8 security_waitfor_linkkey_reply;
    struct bdaddr_t security_waitfor_linkkey_reply_bdaddr;

    // local
    uint8 bt_version;
    uint8 bt_features[8];
    uint8 bt_ext_features[2][8];

    // esco
    uint8 sco_param_select;
    uint8 sco_default_param_select;
    struct btm_sync_conn_param sco_custom_param;

    // security
    uint8 security_bonding_mode;
    uint8 security_auth_requirements;
    uint8 security_io_capability;
    uint8 security_oob_present;

#if (BTM_HCI_HOST_FLOW_CONTROL_ENABLE == 1)
    uint16 fc_bt_tx_cmd_left;
    uint8 fc_bt_tx_acl_left;
    uint8 fc_bt_tx_acl_total;
    uint8 fc_bt_rx_acl_left;

#ifdef __IAG_BLE_INCLUDE__
    uint8 fc_ble_tx_acl_left;
    uint8 fc_ble_tx_acl_total;
    uint8 fc_ble_share_bt_tx_packet;
    uint8 bleAclRxPacketsLeft;
    uint8 bleFlags;
#endif

#endif /* BTM_HCI_HOST_FLOW_CONTROL_ENABLE */
    void (*btm_event_report)(uint16 evt_id, void* pdata);
    void (*btm_cmgr_event_report)(uint16 evt_id, void* conn);
    bool conn_req_cb_enable;
    uint8 con_num;

    uint8 sync_cmd_busy;
    hci_buff_t *sync_cmd_curr;
};

#define BTM(x) btm_ctrl.x

struct btm_conn_env_t {
    struct btm_conn_item_t *conn_item;
};

struct btm_pts_ctrl_t{
    uint8_t pts_mode ;
    uint8_t pts_accept ; 
};

#if defined(__cplusplus)
extern "C" {
#endif

extern struct btm_pts_ctrl_t btm_pts_ctrl;
extern struct btm_ctrl_t btm_ctrl;
struct btm_inquiry_result_item_t *btm_inquiry_result_search ( struct bdaddr_t *bdaddr );
struct btm_inquiry_result_item_t *btm_inquiry_result_find_or_add ( struct bdaddr_t *bdaddr );
struct btm_conn_item_t *btm_conn_add_new ( struct bdaddr_t *bdaddr );
struct btm_conn_item_t *btm_conn_search ( struct bdaddr_t *bdaddr );
uint16 btm_conn_find_scohdl_by_connhdl(uint16 conn_handle);
struct btm_conn_item_t *btm_conn_search_linkup ( struct bdaddr_t *bdaddr );
struct btm_conn_item_t *btm_conn_find_or_add ( struct bdaddr_t *bdaddr );
struct btm_sco_conn_item_t *btm_conn_sco_find_or_add( struct btm_conn_item_t *conn);
struct btm_conn_item_t *btm_conn_acl_search_by_handle( uint16 conn_handle);
struct btm_sco_conn_item_t *btm_conn_sco_search_by_handle( uint16 conn_handle);
bool btm_is_remote_feature_support(struct btm_conn_item_t* conn, uint8 page, uint8 i, uint8 mask);
void btm_conn_disconnect_process(uint16 handle, uint8 status, uint8 reason);
int8 btlib_hcicmd_read_remote_name(struct bdaddr_t *bdaddr, uint8 page_scan_repetition_mode, uint16 clk_off);
void btm_sco_conn_status_notify  (struct bdaddr_t *remote_bdaddr, enum conn_sco_stat_enum sco_conn_notify_type);
void btm_conn_acl_process_tx(struct btm_conn_item_t *conn);
int8 btm_conn_acl_send_ppb_done(uint16 conn_handle, struct pp_buff *ppb);
void btm_register_event_report( void (*evt_cb)(uint16 evt_id, void *pdata));
void btm_register_cmgr_event_report(void (*evt_cb)(uint16 evt_id, void* conn));
extern uint8 gapc_get_conidx(uint16 conhdl);
extern void gapc_inc_rx_packet_count(uint8 idx,uint8 inc_count);
extern uint8 gapc_get_rx_packet_counnt(uint8 idx);
extern uint8 gapc_get_conn_handle(uint8 idx);
extern void gapc_rx_packet_count_reset(uint8 idx);
bool btm_conn_need_authentication(struct btm_conn_item_t *conn);
uint8 btm_conn_allocate_dev_idx(void);
void btm_conn_free_dev_idx(uint8 idx);
void btm_conn_set_item_by_idx(struct btm_conn_item_t *p_conn_item, uint8 dev_idx);
struct l2cap_conn *l2cap_conn_search(struct bdaddr_t *bdaddr);
struct l2cap_conn *l2cap_conn_add_new(struct bdaddr_t *bdaddr);
struct bdaddr_t *btm_get_address_from_rem_dev(struct btm_conn_item_t *rem_dev);
struct btm_conn_item_t * btm_conn_get_item_by_idx(uint8 dev_idx);
uint8 btm_get_bt_version(void);
uint8 btm_get_bt_features(uint8 index);
void btm_print_statistic(void);
void btm_conn_delete_free(struct btm_conn_item_t *conn);
struct btm_sco_conn_item_t *btm_conn_sco_find( struct btm_conn_item_t *conn);
struct btm_sco_conn_item_t *btm_sco_malloc_add( struct btm_conn_item_t *conn);
void btm_sco_delete_free(struct btm_sco_conn_item_t *sco);
int8 btm_get_pending_hci_cmd(uint16 opcode, struct hci_cmd_packet **cmd);
bool btm_acl_role_switch_pending(uint16_t conn_handle);

#if (CFG_BTM_USE_INPLACE_BUFFER_FOR_ACL==1)
void btm_inplacebuff_init(void);
#endif
#if (BTM_HCI_HOST_FLOW_CONTROL_ENABLE == 1)
void btm_fc_init(void);
// bt : host to controller
void btm_fc_bt_host_to_controller_cmd_tx_left_update(uint16 v);
void btm_fc_bt_host_to_controller_cmd_tx_left_dec(uint8 v);
int8 btm_fc_bt_host_to_controller_cmd_can_send(void);
void btm_fc_bt_host_to_controller_acl_unconfirmed_inc(uint16 handle, uint8 v);
void btm_fc_bt_host_to_controller_acl_unconfirmed_dec(uint16 handle, uint8 v);
void btm_fc_bt_host_to_controller_acl_counter_inc(uint8 v);
void btm_fc_bt_host_to_controller_acl_counter_dec(uint8 v);
int8 btm_fc_bt_host_to_controller_can_send(void);
int8 btm_fc_bt_hci_buff_list_pop(uint8 type, hci_buff_t **buff);
#if defined(__IAG_BLE_INCLUDE__)
void bleUpdateFlowControl(uint16 handle);
void BleHciSendCompletedPackets(BOOL TimerFired, BOOL isForceSend);
bool bridge_get_ble_handle(uint16 connHandle);
BOOL btm_ble_conn_is_up(uint16 handle);
// bt : host to controller
void btm_fc_ble_host_to_controller_acl_unconfirmed_inc(uint16 handle, uint8 v);
void btm_fc_ble_host_to_controller_acl_unconfirmed_dec(uint16 handle, uint8 v);
void btm_fc_ble_host_to_controller_acl_counter_inc(uint8 v);
void btm_fc_ble_host_to_controller_acl_counter_dec(uint8 v);
int8 btm_fc_ble_host_to_controller_can_send(void);
void btm_fc_ble_host_to_controller_flow_check(void);
#endif /* __IAG_BLE_INCLUDE__ */
// bt : controller to host
void btm_fc_bt_controller_to_host_acl_counter_inc(uint8 v);
void btm_fc_bt_controller_to_host_acl_counter_dec(uint8 v);
void btm_fc_bt_update_host_num_complete_pkts(uint16 handle, uint8 inc, bool right_now);
void btm_fc_bt_send_host_num_completed_pkts(void *arg);
void btm_fc_print_statistic(void);

#endif /* BTM_HCI_HOST_FLOW_CONTROL_ENABLE */
uint8_t btm_sco_conn_count();
uint8_t btm_get_sco_max_number();
void btm_set_sco_max_number(uint8_t num);
#ifdef __cplusplus 
}
#endif

#endif /* __BTM_H__ */