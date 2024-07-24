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
#ifndef __APP_TWS_IBRT_UI__
#define __APP_TWS_IBRT_UI__
#include "app_tws_ibrt.h"
#include "cmsis_os.h"
#include "bt_drv_reg_op.h"
#include "app_tws_ibrt_cmd_handler.h"

#define SNOOP_INFO_DATA_HEADER                  0XE1D2C3B4
#define SNOOP_INFO_DATA_HEADER_LENGTH           4
#define SNOOP_INFO_DATA_LENGTH_MAX              512
#define SNOOP_SCAN_WINDOW_IN_MS                 30
#define SNOOP_SCAN_INTERVAL_IN_MS               60
#define SNOOP_ADV_INTERVAL_IN_MS                20

#define  IBRT_EVENT_Q_LENGTH                   (20)
#define  MSS_RETRY_COUNTER_MAX                 (10)

#define  IBRT_UI_STOP_IBRT                      1
#define  IBRT_UI_CONNECT_EVT(evt)  ((evt == BTIF_BTEVENT_LINK_CONNECT_CNF)|| \
                                    (evt == BTIF_BTEVENT_LINK_CONNECT_IND)|| \
                                    (evt == BTIF_BTEVENT_ENCRYPTION_CHANGE))

#define  IBRT_UI_DISCONNECT_EVT(evt)  (evt == BTIF_STACK_LINK_DISCONNECT_COMPLETE)
#define  IBRT_UI_INVALID_RSSI                      (100)

#define  IBRT_UI_MIN_RSSI                          (-100)
#define  IBRT_UI_BLE_ADV_THRESHOLD                 (60000)//ms
/*
 * IBRT UI event
 */
typedef  uint8_t   ibrt_controller_error_type;
#define  IBRT_PEER_SM_RUNNING_TIMEOUT   (0)
#define  IBRT_CONTROLLER_TWS_NO_03      (1)
#define  IBRT_CONTROLLER_MOBILE_NO_03   (2)
#define  IBRT_CONTROLLER_TWS_NO_05      (3)
#define  IBRT_CONTROLLER_MOBILE_NO_05   (4)
#define  IBRT_CONTROLLER_DEAD           (5)
#define  IBRT_CONTROLLER_RX_SEQ_ERROR   (6)
#define  IBRT_MOBILE_ENCRYP_ERROR       (7)
#define  IBRT_TWS_BESAUD_CONNECT_ERROR  (8)

/*
 * IBRT UI event
 */
#define  IBRT_EVENT_TYPE_MASK           (0X7F)
#define  IBRT_SKIP_FALSE_TRIGGER_MASK   (0x80)
typedef  uint8_t   ibrt_event_type;
#define  IBRT_NONE_EVENT                (0)
#define  IBRT_OPEN_BOX_EVENT            (1)
#define  IBRT_FETCH_OUT_EVENT           (2)
#define  IBRT_PUT_IN_EVENT              (3)
#define  IBRT_CLOSE_BOX_EVENT           (4)

#define  IBRT_WEAR_UP_EVENT             (5)
#define  IBRT_WEAR_DOWN_EVENT           (6)

#define  IBRT_RECONNECT_EVENT           (7)
#define  IBRT_PHONE_CONNECT_EVENT       (8)
#define  IBRT_TRANSFER_EVENT            (9)
#define  IBRT_NEW_MOBILE_INCOMING_EVENT (10)

//transfer event type
#define  IBRT_PEER_OPEN_BOX_EVENT       (11)
#define  IBRT_PEER_FETCH_OUT_EVENT      (12)
#define  IBRT_PEER_WEAR_UP_EVENT        (13)
#define  IBRT_PEER_RECONNECT_EVENT      (14)

#define  IBRT_TWS_SWITCH_EVENT          (15)
#define  IBRT_TWS_PAIRING_EVENT         (16)
#define  IBRT_FREEMAN_PAIRING_EVENT     (17)
#define  IBRT_CONNECT_SECOND_MOBILE_EVENT (18)
#define  IBRT_DISCONNECT_MOBILE_TWS_EVENT (19)
#define  IBRT_RX_SEQ_ERROR_RECOVER_EVENT  (20)
#define  IBRT_PROFILE_DATA_UPDATE_EVENT   (21)
#define  IBRT_TWS_CONNECT_SYNC_INFO_EVENT (22)
#define  IBRT_CHOICE_MOIBLE_CONNECT_EVENT (23)

#define  IBRT_IS_RECONNECT_EVENT(event)        ((event == IBRT_RECONNECT_EVENT) || (event == IBRT_PEER_RECONNECT_EVENT))
#define  IBRT_RECONNECT_EVENT_TYPE(event)      ((event == IBRT_RECONNECT_EVENT) || (event == IBRT_PEER_RECONNECT_EVENT))
#define  IBRT_CONNECT_MOBILE_EVENT(event)      ((event == IBRT_OPEN_BOX_EVENT)||(event == IBRT_FETCH_OUT_EVENT)|| \
                                                (event == IBRT_WEAR_UP_EVENT) ||(event == IBRT_PEER_OPEN_BOX_EVENT)|| \
                                                (event == IBRT_PEER_FETCH_OUT_EVENT) || (event == IBRT_PEER_WEAR_UP_EVENT) || \
                                                (event == IBRT_RECONNECT_EVENT))

#define  IBRT_IS_TRANSFER_EVENT(event)         ((event == IBRT_PEER_OPEN_BOX_EVENT) || (event == IBRT_PEER_FETCH_OUT_EVENT) || \
                                                (event == IBRT_PEER_WEAR_UP_EVENT) || (event == IBRT_TRANSFER_EVENT))

#define  IBRT_RECONNECT_EVENT_QUEUED()         (app_ibrt_ui_event_has_been_queued(IBRT_RECONNECT_EVENT) || app_ibrt_ui_event_has_been_queued(IBRT_PEER_RECONNECT_EVENT))
#define  IBRT_TRANSFER_EVENT_FILTER(event)     (IBRT_IS_TRANSFER_EVENT(event) && app_ibrt_ui_event_has_been_queued(event))
#define  IBRT_RECONNECT_EVENT_FILTER(event)    (IBRT_RECONNECT_EVENT_TYPE(event) && IBRT_RECONNECT_EVENT_QUEUED())

#define  IBRT_UI_RADICAL_SAN_INTERVAL_BY_NV_ROLE   ((app_tws_ibrt_get_bt_ctrl_ctx()->nv_role) ? \
    app_ibrt_ui_get_ctx()->config.radical_scan_interval_nv_slave : \
    app_ibrt_ui_get_ctx()->config.radical_scan_interval_nv_master)

#define  IBRT_BOTH_HEADSET_WEARED(p_ibrt_ui) ((p_ibrt_ui->box_state == IBRT_OUT_BOX_WEARED) && \
                                              (p_ibrt_ui->peer_tws_info.box_state == IBRT_OUT_BOX_WEARED) && \
                                              p_ibrt_ui->config.wear_updown_detect_supported)

typedef  uint8_t   ibrt_box_state;
/*
 * BOX state
 */
#define IBRT_BOX_UNKNOWN    (0)
#define IBRT_IN_BOX_CLOSED  (1)
#define IBRT_IN_BOX_OPEN    (2)
#define IBRT_OUT_BOX        (3)
#define IBRT_OUT_BOX_WEARED (4)
/*
 * BOX connect state
 */
#define IBRT_BOX_CONNECT_IDLE       (0)
#define IBRT_BOX_CONNECT_MASTER     (1<<0)
#define IBRT_BOX_CONNECT_SLAVE      (1<<1)

//HCI opcode
#define    IBRT_HCI_CREATE_CON_CMD_OPCODE       0x0405
#define    IBRT_HCI_EXIT_SNIFF_MODE_CMD_OPCODE  0x0804
#define    IBRT_HCI_SWITCH_ROLE_CMD_OPCODE      0x080B
#define    IBRT_HCI_STOP_IBRT_OPCODE            0xFCA8
#define    IBRT_HCI_START_IBRT_OPCODE           0xFCA3
#define    IBRT_HCI_RESET_OPCODE                0x0C03
#define    IBRT_HCI_DSIC_CON_CMD_OPCODE         0x0406
#define    IBRT_HCI_SET_ENV_CMD_OPCODE          0xFC8E

typedef enum
{
    START_IBRT_TRIGGER_EXIT,
    TWS_SWITCH_TRIGGER_EXIT,
} ibrt_ui_exit_sniff_trigger_e;

typedef enum
{
    TWS_INIT_TRIGGER,
    MOBILE_INIT_TRIGGER,
    START_IBRT_TRIGGER,
    EXCHANGE_PROFILE_TRIGGER,
    SCO_CONNECTED_TRIGGER,
    SCO_DISCONNECT_TRIGGER,
    MOBILE_MSS_TRIGGER,
    OTA_START_TRIGGER,
    OTA_STOP_TRIGGER,
    START_DUAL_MIC_RECORDING_TRIGGER,
    STOP_DUAL_MIC_RECORDING_TRIGGER,
} ibrt_ui_trigger_link_policy_e;

typedef enum
{
    IBRT_CONNECTE_TRIGGER,
    IBRT_DISCONNECT_TRIGGER,
    IBRT_RECONNECT_TWS_TRIGGER,
    IBRT_RECONNECT_MOBILE_TRIGGER,
    IBRT_CLOSE_SCAN_TIMER_TRIGGER,
    IBRT_SEARCH_SLAVE_TRIGGER,
    IBRT_MASTER_CON_SLAVE_TRIGGER,
    IBRT_NV_SLAVE_CLOSE_PSCAN_TRIGGER,
    IBRT_NV_SLAVE_REOPEN_PSCAN_TRIGGER,
    IBRT_FREEMAN_PAIR_TRIGGER,
    IBRT_CLOSE_BOX_TRIGGER,
    IBRT_OPEN_BOX_TRIGGER,
    IBRT_ENTER_PAIRING_MODE_TRIGGER,
    IBRT_ROLE_CHANGE_TRIGGER,
    IBRT_A2DP_PLAYING_TRIGGER,
    IBRT_A2DP_SUSPEND_TRIGGER,
    IBRT_SCO_PLAYING_TRIGGER,
    IBRT_SCO_SUSPEND_TRIGGER,
    IBRT_FASTPAIR_TRIGGER,
    IBRT_CLOSE_ALL_SCAN,
    IBRT_RESTORE_SCAN,
    IBRT_TRIGGER_NUM,
} ibrt_ui_trigger_scan_e;

typedef enum
{
    IBRT_PAIR_IDLE,
    IBRT_PAIR_W4_TWS_DISCONNECTED,
    IBRT_PAIR_W4_MOBILE_DISCONNECTED,
    IBRT_PAIR_W4_TWS_CONNECTED,
    IBRT_PAIR_W4_TWS_INFO_EXCHANGED,
} ibrt_pairing_state_e;

typedef enum
{
    IBRT_PAIR_ACTION_IDLE,
    IBRT_PAIR_ACTION_TWS_DISCONNECTED,
    IBRT_PAIR_ACTION_MOBILE_DISCONNECTED,
    IBRT_PAIR_ACTION_TWS_CONNECTED,
    IBRT_PAIR_ACTION_TWS_INFO_EXCHANGED,
} ibrt_pairing_action_e;

typedef enum
{
    IBRT_UI_IDLE,
    IBRT_UI_IDLE_WAIT,
    IBRT_UI_W4_TWS_CONNECTION,
    IBRT_UI_W4_TWS_INFO_EXCHANGE_COMPLETE,
    IBRT_UI_W4_MOBILE_CONNECTION,
    IBRT_UI_W4_MOBILE_MSS_COMPLETE,
    IBRT_UI_W4_SET_ENV_COMPLETE,
    IBRT_UI_W4_MOBILE_ENTER_ACTIVE_MODE,
    IBRT_UI_W4_START_IBRT_COMPLETE,
    IBRT_UI_W4_IBRT_DATA_EXCHANGE_COMPLETE,
    IBRT_UI_W4_TWS_SWITCH_COMPLETE,
    IBRT_UI_W4_TWS_STOP_IBRT_COMPLETE,
    IBRT_UI_W4_TWS_BT_MSS_COMPLETE,
    IBRT_UI_W4_TWS_DISCONNECTED,
    IBRT_UI_W4_MOBILE_DISCONNECTED,
    IBRT_UI_W4_TWS_DISCONNECT_TO_IDLE,
    IBRT_UI_W4_MOBILE_CONNECTION_TO_IDLE,
    IBRT_UI_W4_NEW_MOBILE_INCOMING_RSP,
    IBRT_UI_W4_BLE_CONNECTION,
    IBRT_UI_W4_SNOOP_INFO_COMPLETE,
    IBRT_UI_W4_PHONE_CONNECT_FAILED,
    IBRT_UI_W4_TWS_SWITCH_PREPARE_COMPLETE,
    IBRT_UI_W4_BOTH_ACTIVE,
    //Add new state

    IBRT_UI_W4_SM_STOP,
    IBRT_UI_W4_END,
    IBRT_UI_W4_UNKNOWN,
} ibrt_ui_state_e;

//Mapping to ibrt ui state
typedef enum
{
    IBRT_ACTION_IDLE,
    IBRT_ACTION_IDLE_WAIT,
    IBRT_ACTION_TWS_CONNECT,
    IBRT_ACTION_INFO_EXCHANGE,
    IBRT_ACTION_MOBILE_CONNECT,
    IBRT_ACTION_MOBILE_MSS_COMPLETE,
    IBRT_ACTION_SET_ENV_COMPLETE,
    IBRT_ACTION_MOBILE_ENTER_ACTIVE_MODE,
    IBRT_ACTION_START_IBRT,
    IBRT_ACTION_DATA_EXCHANGE,
    IBRT_ACTION_TWS_SWITCH,
    IBRT_ACTION_STOP_IBRT,
    IBRT_ACTION_TWS_BT_MSS_COMPLETE,
    IBRT_ACTION_TWS_DISCONNECT,
    IBRT_ACTION_MOBILE_DISCONNECT,  //14
    IBRT_ACTION_TWS_DISCONNECT_TO_IDLE,
    IBRT_ACTION_MOBILE_CONNECTION_TO_IDLE,
    IBRT_ACTION_NEW_MOBILE_INCOMING_RSP,
    IBRT_ACTION_BLE_CONNECTION,
    IBRT_ACTION_SNOOP_INFO_COMPLETE,
    IBRT_ACTION_PHONE_CONNECT_FAILED,
    IBRT_ACTION_TWS_SWITCH_PREPARE_COMPLETE,
    IBRT_ACTION_BOTH_ACTIVE,
    //Add new action

    IBRT_ACTION_SM_STOP,
    IBRT_ACTION_END,
} ibrt_action_e;

typedef struct
{
    uint8_t   num_hci_cmd_packets;
    uint16_t  cmd_opcode;
    uint8_t   param[1];
} __attribute__ ((packed)) ibrt_cmd_comp_t;

typedef struct
{
    bool w4_set_env_complete;
    bool w4_exit_sniff_complete;
    bool w4_start_ibrt;
} ibrt_start_ctrl_t;

typedef struct
{
    ibrt_action_e action;
    ibrt_ui_state_e  state;
} ibrt_act_state_t;

typedef enum
{
    IBRT_UI_NO_ERROR,
    IBRT_UI_RSP_TIMEOUT,
    IBRT_UI_NOT_ACCEPT,
    IBRT_UI_CONNECT_FAILED,
    IBRT_UI_STATUS_ERROR,
    IBRT_UI_CONNECTION_TIMEOUT=0x08,
    IBRT_UI_ACL_ALREADY_EXIST=0x0B,
    IBRT_UI_CMD_DISALLOWED=0X0C,
    IBRT_UI_LIMITED_RESOURCE =0x0D,
    IBRT_UI_HOST_ACCEPT_TIMEOUT=0x10,
    IBRT_UI_CONN_TERM_USER_REQ=0x13,
    IBRT_UI_CONN_TERM_LOW_RESOURCES=0x14,
    IBRT_UI_MOBILE_CONN_DISCONNECTED=0X2A,
    IBRT_UI_TWS_CONN_DISCONNECTED=0X2B,
    IBRT_UI_SNOOP_DISCONNECTED=0X2C,
    IBRT_UI_CONNECTION_INCOMING=0x99,
    IBRT_UI_EVT_STATUS_ERROR=0x9A,
    IBRT_UI_TWS_CMD_SEND_FAILED=0X9B,
    IBRT_UI_MOBILE_PAIR_CANCLED=0X9C,
} ibrt_ui_error_e;

typedef enum
{
    IBRT_CONNECT_MOBILE_FAILED,
    IBRT_MOBILE_DISCONNECTED,
    IBRT_MOBILE_NV_EMPTY,
    IBRT_TWS_PAIR_MODE_ENTER,
    IBRT_FREEMAN_PAIR_MODE_ENTER,
    IBRT_SCAN_TIMEOUT,
    IBRT_MOBILE_CONNECTED,
    IBRT_SNOOP_CONNECTED,
    IBRT_CLOSE_BOX,
    IBRT_USE_PEER_PAIRING_MODE,
} trigger_pairing_mode_type_e;

typedef struct
{
    uint16_t battery_volt;
    uint16_t  mobile_conhandle;
    uint16_t  tws_conhandle;
    rssi_t   raw_rssi;
    int32_t mobile_diff_us;
    int32_t tws_diff_us;
    int8_t  cur_buf_size;
} app_ui_rssi_battery_info_t;

typedef struct
{
    bt_bdaddr_t   mobile_addr;
    bt_bdaddr_t   nv_mobile_addr;
    ibrt_box_state box_state;
    ibrt_role_e   ibrt_role;
    uint8_t       running_event;
    uint8_t       sco_acitve;
    uint8_t       mobile_connected:1;//ibrt master should set this variable
    uint8_t       ibrt_connected:1;//ibrt slave should set this variable
    uint8_t       ibrt_sm_running:1;
    uint8_t       connecting_mobile:1;
    uint32_t      constate;
    uint8_t       pairing_mode;
    bool          need_peer_connect_mobile;
} __attribute__((packed)) app_tws_info_t;

typedef struct
{
    ibrt_event_type ibrt_event_q[IBRT_EVENT_Q_LENGTH];
    uint8_t         front;
    uint8_t         rear;
} ibrt_event_manager_t;

typedef struct
{
    bool freeman_enable;
    bool freeman_dont_role_switch;
    bool tws_use_same_addr;
    bool wear_updown_tws_switch_enable;
    bool slave_reconnect_enable;
    bool enter_pairing_mode;
    bool freeman_accept_mobile_new_pairing;
    bool enter_pairing_on_empty_mobile_addr;
    bool enter_pairing_on_reconnect_mobile_failed;
    bool enter_pairing_on_reconnect_mobile_failed_once;
    bool enter_pairing_on_mobile_disconnect;

    bool check_plugin_excute_closedbox_event;
    bool nv_slave_enter_pairing_on_mobile_disconnect;
    bool nv_slave_enter_pairing_on_empty_mobile_addr;
    bool disc_tws_before_reconnect_mobile;
    bool snoop_via_ble_enable;
    bool tws_switch_according_to_rssi_value;
    bool disable_tws_switch;
    bool disable_stop_ibrt;
    bool lowlayer_monitor_enable;
    bool delay_exit_sniff;
    bool share_tws_info_done;
    uint32_t delay_ms_exit_sniff;
    bool mobile_incoming_filter_unpaired;
    uint8_t rssi_threshold;
    uint8_t role_switch_timer_threshold;
    uint16_t close_box_event_wait_response_timeout;
    uint16_t reconnect_wait_ready_timeout;
    uint16_t reconnect_mobile_wait_ready_timeout;
    uint16_t reconnect_tws_wait_ready_timeout;
    uint16_t reconnect_mobile_wait_response_timeout;
    uint16_t reconnect_ibrt_wait_response_timeout;
    uint16_t nv_master_reconnect_tws_wait_response_timeout;
    uint16_t nv_slave_reconnect_tws_wait_response_timeout;
    uint32_t disable_bt_scan_timeout;
    uint16_t open_reconnect_mobile_max_times;
    uint16_t open_reconnect_tws_max_times;
    uint16_t reconnect_mobile_max_times;
    uint16_t reconnect_tws_max_times;
    uint16_t reconnect_ibrt_max_times;
    uint8_t  tws_reconnect_cycle;
    uint8_t  mobile_reconnect_cycle;

    uint16_t long_private_poll_interval ;
    uint16_t default_private_poll_interval;
    uint16_t short_private_poll_interval;

    uint16_t default_private_poll_interval_in_sco;
    uint16_t short_private_poll_interval_in_sco;
    uint16_t default_bt_tpoll;

    uint16_t tws_page_timeout_on_last_success;
    uint16_t tws_page_timeout_on_last_failed;
    uint16_t tws_page_timeout_on_reconnect_mobile_success;
    uint16_t tws_page_timeout_on_reconnect_mobile_failed;

    uint16_t mobile_page_timeout;
    uint16_t tws_connection_timeout;
    uint8_t  profile_concurrency_supported;
    uint8_t  connect_new_mobile_enable;
    uint16_t wait_time_before_disc_tws;
    bt_bdaddr_t new_mobile_address;

    uint32_t rx_seq_error_timeout;
    uint32_t rx_seq_error_threshold;
    uint32_t rx_seq_recover_wait_timeout;
    uint32_t rssi_monitor_timeout;
    bool     wear_updown_detect_supported;
    uint32_t stop_ibrt_timeout;

    uint16_t radical_scan_interval_nv_slave;
    uint16_t radical_scan_interval_nv_master;
    uint32_t event_hung_timeout;
    uint16_t rssi_tws_switch_threshold;
    uint32_t stop_ibrt_wait_time_after_tws_switch;
    uint32_t tws_conn_failed_wait_time;
    uint32_t sm_running_timeout;
    uint32_t peer_sm_running_timeout;
    uint32_t connect_no_03_timeout;
    uint32_t disconnect_no_05_timeout;
    uint32_t reconnect_peer_sm_running_timeout;

    bool     tws_switch_tx_data_protect;
    uint32_t tws_cmd_send_timeout;
    uint32_t tws_cmd_send_counter_threshold;
    uint32_t tws_switch_stable_timeout;
    bool     invoke_event_when_box_closed;
    bool     tws_stay_when_close_box;
    uint32_t free_tws_timeout;
    bool     filter_duplicate_event;

    uint8_t audio_sync_mismatch_resume_version;
} ibrt_ui_config_t;

typedef struct
{
    uint8_t         connection_index;
    bool            connected;
    bool            disconnect_enable;
    bool            master_notify_enable;
    bool            slave_scaning;
    bt_bdaddr_t     box_ble_addr;
    uint8_t         box_connect_state;
    uint32_t        snoop_info_data_length;
    uint8_t         data[SNOOP_INFO_DATA_LENGTH_MAX];
} ibrt_ui_box_info_t;

typedef struct
{
    ibrt_pairing_state_e tws_pair_state;
    ibrt_pairing_state_e freeman_pair_state;
    bool tws_pairing;
    bool freeman_pairing;
} ibrt_pair_ctrl_t;

typedef struct
{
    ibrt_ui_state_e super_state;
    ibrt_ui_state_e wait_state;
    ibrt_ui_state_e sub_state;
    ibrt_ui_state_e next_state;

    ibrt_event_manager_t event_q_manager;
    ibrt_event_type active_event;

    ibrt_box_state  box_state;
    ibrt_box_state  box_state_future;
    uint8_t         ibrt_sm_running;
    app_tws_info_t  peer_tws_info;

    uint32_t        sm_start_timestamp_ms;

    bool            profile_exchanged;
    bool            tws_connect_success;
    uint8_t         reconnect_mobile_counter;
    uint8_t         reconnect_tws_counter;
    uint8_t         reconnect_ibrt_counter;

    ibrt_pair_ctrl_t pairing_ctrl;

    /*custom page scan parameters*/
    uint16_t        pscan_cust_interval;
    uint16_t        pscan_cust_window;
    uint8_t         pscan_cust_type;

    /*search ui restore inqscan parameter when tws connected*/
    bool            restore_iscan_para;
    bool            audio_mute_enable;
    bool            bonding_success;
    bool            tws_pscan_enhanced_enable;
    bool            encrypt_error_happened;
    bool            silent_mode;
    bool            need_peer_connect_mobile;
    bool            refresh_pairing_enable;
    bool            new_mobile_scan_enable;
    /*for customer config*/
    ibrt_ui_config_t config;

    /*for box information*/
    ibrt_ui_box_info_t ibrt_ui_box;
} app_ibrt_ui_t;

typedef struct
{
    bt_bdaddr_t master_bdaddr;
    bt_bdaddr_t slave_bdaddr;
} ibrt_pairing_info_t;

typedef struct
{
    int32_t a2dp_volume;
    int32_t hfp_volume;
} ibrt_volume_info_t;


typedef struct
{
    bt_bdaddr_t   mobile_addr;
    uint8_t       mobile_connected;
    uint8_t       sco_status;
} __attribute__((packed)) app_tws_start_ibrt_info_t;

typedef bool (*APP_IBRT_UI_TWS_SWITCH_HANDLER_IND)(void);
typedef void (*APP_IBRT_UI_RSSI_BATTERY_HANDLER_IND)(void);
typedef bool (*APP_IBRT_UI_CONNECT_MOBILE_HANDLER_IND)(void);
typedef void (*APP_IBRT_UI_CMD_COMPLETE_HANDLER_IND)(ibrt_cmd_comp_t *);
typedef void (*APP_IBRT_UI_PAIRING_MODE_HANDLER_IND)(trigger_pairing_mode_type_e trigger_type);
typedef void (*APP_IBRT_UI_VENDER_EVENT_HANDLER_IND)(uint8_t, uint8_t *, uint8_t);
typedef void (*APP_IBRT_UI_GLOBAL_HANDLER_IND)(ibrt_link_type_e, uint8_t, uint8_t);
typedef void (*APP_IBRT_UI_GLOBAL_EVENT_UPDATE_IND)(ibrt_event_type, \
        ibrt_ui_state_e, \
        ibrt_ui_state_e, \
        ibrt_action_e, \
        ibrt_ui_error_e);
typedef void (*APP_IBRT_UI_CONNECTED_HANDLER_IND)(bt_bdaddr_t *);
typedef void (*APP_IBRT_UI_PROFILE_STATE_CHANGE_IND)(uint32_t, uint8_t);

#ifdef __cplusplus
extern "C" {
#endif

/*
 * function declare
 */
void app_ibrt_ui_init(void);
app_ibrt_ui_t* app_ibrt_ui_get_ctx(void);
void app_ibrt_ui_prepare_local_tws_info(app_tws_info_t *local_tws_info);
int app_ibrt_ui_event_entry(ibrt_event_type event);
bool app_ibrt_ui_execute_pending_event(void);
void app_ibrt_ui_global_handler(ibrt_link_type_e link_type, uint8_t evt_type, uint8_t status);
bool app_ibrt_ui_tws_switch_needed(bool ibrt_connected_now);
bool app_ibrt_ui_ibrt_start_needed(void);
bool app_ibrt_ui_ibrt_stop_needed(void);
bool app_ibrt_ui_connect_mobile_needed(void);
bool app_ibrt_ui_ibrt_connected(void);
void app_ibrt_ui_reset_peer_tws_info(void);
void app_ibrt_ui_vender_event_handler(uint8_t evt_type, uint8_t *buffer, uint8_t length);
void app_ibrt_ui_cmd_status_callback(const void *para);
void app_ibrt_ui_cmd_complete_callback(const void *para);
void app_ibrt_ui_judge_ibrt_role(void);
void app_ibrt_ui_reset_local_tws_info(void);
void app_ibrt_ui_event_handler(ibrt_action_e action,ibrt_ui_error_e status);
void app_ibrt_ui_subevent_handler(ibrt_ui_state_e action_state, ibrt_ui_error_e status);
void app_ibrt_ui_fetch_out_event_handler(ibrt_action_e action,ibrt_ui_error_e status);
void app_ibrt_ui_wear_up_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_wear_down_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_tws_switch_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_tws_connect_sync_info_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_new_mobile_incoming_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
uint16_t app_ibrt_ui_get_tws_page_timeout_value();
void app_ibrt_ui_open_box_event_handler(ibrt_action_e action,ibrt_ui_error_e status);
void app_ibrt_ui_put_in_box_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_close_box_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_free_link_handler(ibrt_action_e action, ibrt_ui_error_e status, bool force_free);
bool app_ibrt_ui_peer_tws_sm_running(void);
ibrt_ui_state_e app_ibrt_ui_get_state_mapping_action(ibrt_action_e action);
void app_ibrt_ui_event_response_timer_cb(void const *n);
void app_ibrt_ui_dbg_state_timer_cb(void const *n);
void app_ibrt_ui_event_error_handling_timer_cb(void const *current_evt);
void app_ibrt_ui_disable_scan_timer_cb(void const *trigger_type);
void app_ibrt_ui_peer_sm_running_error_handler_cb(void const *trigger_type);
void app_ibrt_ui_disable_ble_timer_cb(void const *trigger_type);
void app_ibrt_ui_check_rssi_battery_timer_cb(void const *current_evt);
void app_ibrt_ui_common_sm(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_reconnect_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_connect_second_mobile_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_choice_mobile_connect_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_disconnect_mobile_tws_event_handler(ibrt_action_e action,ibrt_ui_error_e status);
void app_ibrt_ui_profile_data_update_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_choice_connect_second_mobile(void);
void app_ibrt_ui_choice_mobile_connect(uint8_t index);
void app_ibrt_ui_perform_received_action(uint8_t *p_buff, uint16_t length);
bool app_ibrt_ui_pagescan_enable_needed(ibrt_ui_trigger_scan_e trigger_type);
bool app_ibrt_ui_inqscan_enable_needed(void);
void app_ibrt_ui_judge_scan_type(ibrt_ui_trigger_scan_e trigger_type, ibrt_link_type_e link_type, uint8_t status);
void app_ibrt_ui_judge_link_policy(ibrt_ui_trigger_link_policy_e trigger_type, btif_link_policy_t policy);
void app_ibrt_ui_restart_event_handler(void);
void app_ibrt_ui_tws_switch_callback(void);
bool app_ibrt_ui_detect_sm_running_collision(void);
bool app_ibrt_ui_set_env_needed(void);
bool app_ibrt_ui_tws_bt_mss_needed(void);
bool app_ibrt_ui_transfer_sm_needed(void);
void app_ibrt_ui_profile_data_exchange(void);
void app_ibrt_ui_tws_transfer_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_phone_connect_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
bool app_ibrt_ui_none_event_happen(void);
bool app_ibrt_ui_none_event_happen(void);
void app_ibrt_ui_none_event_handler(ibrt_action_e action, ibrt_ui_error_e status);
void app_ibrt_ui_pairing_callback(const btif_event_t *event);
bool app_ibrt_ui_delay_start_ibrt_needed(void);
void app_ibrt_ui_action_state_mismatch_handler(ibrt_action_e action, ibrt_ui_error_e status);
bool app_ibrt_ui_is_profile_exchanged(void);
void app_ibrt_ui_set_profile_exchanged(void);
void app_ibrt_ui_clear_profile_exchanged(void);
bool app_ibrt_ui_get_freeman_enable(void);
bool app_ibrt_ui_is_role_switch_allowed(void);
bool app_ibrt_ui_get_nv_salve_enter_pairing_on_mobile_disconnect(void);
bool app_ibrt_ui_get_nv_salve_enter_pairing_on_empty_mobile_addr(void);
void app_ibrt_ui_set_tws_use_same_addr_enable(void);
bool app_ibrt_ui_get_tws_use_same_addr_enable(void);
void app_ibrt_ui_clear_tws_use_same_addr_enable(void);
void app_ibrt_ui_set_freeman_enable(void);
void app_ibrt_ui_clear_freeman_enable(void);
bool app_ibrt_ui_can_tws_switch(void);
bool app_ibrt_ui_tws_switch(void);
bool app_ibrt_ui_get_share_tws_info_done(void);
void app_ibrt_ui_set_share_tws_info_done(void);
void app_ibrt_ui_clear_share_tws_info_done(void);
bool app_ibrt_ui_get_slave_reconnect_enable(void);
void app_ibrt_ui_set_slave_reconnect_enable(void);
void app_ibrt_ui_clear_slave_reconnect_enable(void);
void app_ibrt_ui_set_wear_updown_tws_switch_enable(void);
void app_ibrt_ui_clear_wear_updown_tws_switch_enable(void);
bool app_ibrt_ui_get_wear_updown_tws_switch_enable(void);
void app_ibrt_ui_set_enter_pairing_mode(trigger_pairing_mode_type_e trigger_type);
void app_ibrt_ui_recover_rx_seq_error_timer_cb(void const *n);
bool app_ibrt_ui_get_enter_pairing_mode(void);
void app_ibrt_ui_clear_enter_pairing_mode(trigger_pairing_mode_type_e trigger_type);
void app_ibrt_ui_set_mobile_incoming_filter_unpaired(void);
void app_ibrt_ui_clear_mobile_incoming_filter_unpaired(void);
bool app_ibrt_ui_get_mobile_incoming_filter_unpaired(void);
void app_ibrt_ui_customer_set_bdaddr(uint8_t *nv_master_addr, uint8_t *nv_slave_addr);
bool app_ibrt_ui_is_tws_connecting(void);
bool app_ibrt_ui_is_mobile_connecting(void);
bool app_ibrt_ui_is_data_exchange_waiting(void);
bool app_ibrt_ui_is_tws_switching(void);
void app_ibrt_ui_register_connect_mobile_needed_ind(APP_IBRT_UI_CONNECT_MOBILE_HANDLER_IND handler);
void app_ibrt_ui_register_global_handler_ind(APP_IBRT_UI_GLOBAL_HANDLER_IND handler);
void app_ibrt_ui_register_global_event_update_ind(APP_IBRT_UI_GLOBAL_EVENT_UPDATE_IND handler);
void app_ibrt_ui_register_pairing_mode_ind(APP_IBRT_UI_PAIRING_MODE_HANDLER_IND set_pairing_callback, APP_IBRT_UI_PAIRING_MODE_HANDLER_IND clear_pairing_callback);
void app_ibrt_ui_register_cmd_complete_ind(APP_IBRT_UI_CMD_COMPLETE_HANDLER_IND cmd_complete_ind);
void app_ibrt_ui_register_vender_event_update_ind(APP_IBRT_UI_VENDER_EVENT_HANDLER_IND handler);
void app_ibrt_ui_register_profile_state_change_ind(APP_IBRT_UI_PROFILE_STATE_CHANGE_IND handler);
void app_ibrt_ui_profile_state_change_ind(uint32_t profile,uint8_t connected);
ibrt_ui_error_e app_ibrt_ui_event_queue_insert(ibrt_event_type event);
bool app_ibrt_ui_event_has_been_queued(ibrt_event_type event);
ibrt_ui_error_e app_ibrt_ui_event_queue_delete(ibrt_event_type *p_event);
bool app_ibrt_ui_event_queue_empty(void);
ibrt_ui_error_e app_ibrt_ui_event_queue_get_front(ibrt_event_type *p_event);
ibrt_ui_error_e app_ibrt_ui_event_queue_get_rear(ibrt_event_type *p_event);
void app_ibrt_ui_event_queue_init(void);
bool app_ibrt_ui_enter_idle_wait_needed(ibrt_event_type event, ibrt_event_type peer_event);
void app_ibrt_ui_event_update_entry(ibrt_event_type evt_type, \
                                    ibrt_ui_state_e old_state, \
                                    ibrt_ui_state_e new_state, \
                                    ibrt_action_e action, \
                                    ibrt_ui_error_e status);
uint8_t app_ibrt_ui_get_reconnect_tws_max_times(void);
uint8_t app_ibrt_ui_get_reconnect_mobile_max_times(void);
bool app_ibrt_ui_tws_switch_according_rssi_needed(void);
void app_ibrt_ui_update_feature_box_state(ibrt_event_type event);
void app_ibrt_ui_start_ble_adv_broadcasting(uint8_t pairing_status);
void app_ibrt_ui_stop_ble_adv_broadcasting(void);
uint8_t is_sco_mode (void);
void app_ibrt_ui_link_control_while_calling(void);
bool btdrv_get_accept_new_mobile_enable(void);
void app_ibrt_ui_reset_link_control_while_profile_exchanged(void);
bool app_ibrt_ui_tws_reconnect_one_cycle_ended(uint8_t reconnect_counter);
bool app_ibrt_ui_mobile_reconnect_one_cycle_ended(uint8_t reconnect_counter);
bool app_ibrt_ui_wait_ibrt_connected(void);
bool app_ibrt_ui_connect_mobile_event_pending(void);
uint16_t app_ibrt_ui_get_reconnect_mobile_wait_time(void);

void app_ibrt_ui_box_init(bt_bdaddr_t *box_ble_addr);
bool app_ibrt_ui_get_snoop_via_ble_enable(void);
void app_ibrt_ui_set_snoop_via_ble_enable(void);
void app_ibrt_ui_clear_snoop_via_ble_enable(void);
void app_ibrt_ui_set_ble_connect_index(uint8_t index);
void app_ibrt_ui_set_box_ble_addr(bt_bdaddr_t *addr);
bt_bdaddr_t *app_ibrt_ui_get_box_ble_addr(void);
void app_ibrt_ui_set_master_notify_flag(bool notify_enable);
void app_ibrt_ui_set_slave_scaning(bool scan_enable);
bool app_ibrt_ui_is_slave_scaning(void);
void app_ibrt_ui_set_box_connect_state(uint8_t box_connect_state, bool force_set);
uint8_t app_ibrt_ui_clear_box_connect_state(uint8_t box_connect_state, bool force_clear);
void app_ibrt_ui_set_disconnect_enable(bool disconnect_enable);
uint8_t app_ibrt_ui_snoop_info_send_enable(void);
void app_ibrt_ui_connect_box();
void app_ibrt_ui_disconnect_box(void);
uint32_t app_ibrt_ui_snoop_info_transport(uint8_t *buf, uint32_t len);
void app_ibrt_ui_snoop_info_handler(uint8_t *buf, uint32_t len);
void app_ibrt_ui_freeman_pairing_handler(ibrt_pairing_action_e action,  ibrt_ui_error_e status);
void app_ibrt_ui_tws_pairing_handler(ibrt_pairing_action_e action,  ibrt_ui_error_e status);
void app_ibrt_ui_clear_refresh_pairing_enable(void);
bool app_ibrt_ui_get_refresh_pairing_enable(void);
void app_ibrt_ui_set_refresh_pairing_enable(void);
bool app_ibrt_ui_re_enter_pairing_mode_enable(void);
void app_ibrt_ui_clear_peer_sm_running(void);
void app_ibrt_ui_set_peer_sm_running(void);
bool app_ibrt_ui_duplicate_event_can_filtered(ibrt_event_type t_event);
bool app_ibrt_ui_is_box_closed(void);
ibrt_box_state app_ibrt_ui_get_box_state(void);
void app_ibrt_ui_mobile_link_disconnect_error_handler(void);
void app_ibrt_ui_tws_link_disconnect_error_handler(void);
void app_ibrt_ui_start_reconnect_event(void);
bool app_ibrt_ui_start_reconnect_event_needed(ibrt_action_e action, ibrt_ui_error_e status);
bool app_ibrt_ui_get_audio_mute_enable(void);
void app_ibrt_ui_set_audio_mute_enable(void);
void app_ibrt_ui_clear_audio_mute_enable(void);
void app_ibrt_ui_controller_error_handling(ibrt_controller_error_type error_type, bool reset_controler);
void app_ibrt_ui_set_tws_connect_success_last(void);
void app_ibrt_ui_host_error_handling(ibrt_controller_error_type error_type);
void app_ibrt_ui_clear_tws_connect_success_last(void);
bool app_ibrt_ui_get_tws_connect_success_last(void);
void app_ibrt_ui_start_dbg_state_timer(void);
void app_ibrt_ui_stop_dbg_state_timer(void);
void app_ibrt_ui_controller_dbg_state_checker(void);
void app_ibrt_ui_tws_cmd_send_fail_error_handler(app_tws_cmd_code_e cmd_code);
void app_ibrt_ui_set_local_volume_info(uint8_t * p_buffer);
void app_ibrt_ui_stop_ibrt_condition_checker(void);
void app_ibrt_ui_stop_ibrt_timer_cb(void const *n);
bool app_ibrt_ui_stop_ibrt_timer_condition_fullfilled(void);
bool app_ibrt_if_is_audio_active(void);
void app_ibrt_ui_mobile_link_connected_callback(bt_bdaddr_t * addr);
void app_ibrt_ui_ibrt_link_connected_callback(bt_bdaddr_t * addr);
void app_ibrt_ui_tws_link_connected_callback(bt_bdaddr_t * addr);
void app_ibrt_ui_register_link_connected_ind(APP_IBRT_UI_CONNECTED_HANDLER_IND mobile_connected_ind,
        APP_IBRT_UI_CONNECTED_HANDLER_IND ibrt_connected_ind,
        APP_IBRT_UI_CONNECTED_HANDLER_IND tws_connected_ind);
void app_ibrt_ui_send_user_action(uint8_t *p_buff, uint16_t length);
void app_ibrt_ui_reboot_sdk(void);
void app_ibrt_ui_tws_switch_stable_timer_cb(void const *n);
void app_ibrt_ui_tws_switch_prepare_timer_cb(void const *n);
void app_ibrt_ui_disconnect_ble(void);
void app_ibrt_ui_register_tws_switch_needed_ind(APP_IBRT_UI_TWS_SWITCH_HANDLER_IND handler);
void app_hci_vender_ibrt_disconnected_callback(bool stop_for_seq_error,uint8_t disc_reason);
bool app_ibrt_ui_exit_mobile_sniff_mode_needed(ibrt_ui_exit_sniff_trigger_e exit_sniff_triiger);
bool app_ibrt_ui_exit_tws_sniff_mode_needed(ibrt_ui_exit_sniff_trigger_e exit_sniff_triiger);
void app_ibrt_ui_disconnect_tws_timer_cb(void const *n);
void app_ibrt_ui_exit_sniff(ibrt_ui_exit_sniff_trigger_e exit_sniff_trigger);
#ifdef __cplusplus
}
#endif
#endif

