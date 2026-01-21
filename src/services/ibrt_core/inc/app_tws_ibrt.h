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
#ifndef __APP_TWS_IBRT__
#define __APP_TWS_IBRT__
#include "app_tws_profile_sync.h"
#include "a2dp_api.h"
#include "cmsis_os.h"
#include "rfcomm_api.h"
#include "spp_api.h"
#include "hfp_api.h"
#include "bt_drv_reg_op.h"

#define ACCESS_MODE_ARRAY_MAX           10
#define APP_IBRT_RECONNECT_TIMEOUT_MS   5

#define LETOHOST16(ptr)  (uint16_t)( ((uint16_t) *((uint8_t*)(ptr)+1) << 8) | \
                                ((uint16_t) *((uint8_t*)(ptr))) )

#define LETOHOST32(ptr)  (uint32_t)( ((uint32_t) *((uint8_t*)(ptr)+3) << 24) | \
                                ((uint32_t) *((uint8_t*)(ptr)+2) << 16) | \
                                ((uint32_t) *((uint8_t*)(ptr)+1) << 8)  | \
                                ((uint32_t) *((uint8_t*)(ptr))) )


#define UINT16LOW(num)   ((uint8_t)(num & 0xFF))
#define UINT16HIGH(num)  ((uint8_t)((num >> 8) & 0xFF))

#define APP_IBRT_SUPV_TO (0x640)

#define NV_RAM_RECORD_NUM   (10)

#define INVALID_ERROR   0xFF
#define INVALID_HANDLE  0xFFFF
#define BD_ADDR_LEN     6

#define IBRT_DISABLE         0
#define IBRT_ENABLE          1
#define IBRT_SWITCH          2

#define IBRT_NONE_ROLE       0
#define IBRT_SNIFFER_ROLE    1
#define IBRT_FORWARD_ROLE    2

#define IBRT_TWS_SNIFF_MAX_INTERVAL (254)
#define IBRT_TWS_SNIFF_MIN_INTERVAL (254)

#define IBRT_TWS_LINK_LARGE_DURATION           (12)
#define IBRT_TWS_LINK_DEFAULT_DURATION         (8)

#define IBRT_TWS_PRIVATE_TWS_ENABLE            (1)
#define IBRT_TWS_PRIVATE_TWS_DURATION          (2)
#define IBRT_TWS_PRIVATE_TWS_INTERVAL          (4)

#define IBRT_ENCRYPT_DISABLE 0x00
#define IBRT_ENCRYPT_ENABLE  0x01
#define IBRT_ENCRYPT_REFRESH 0x02

#define  IBRT_TWS_BT_TPOLL_DEFAULT             (80)

typedef uint8_t ibrt_link_mode_e;
#define IBRT_ACTIVE_MODE     0x00
#define IBRT_HOLD_MODE       0x01
#define IBRT_SNIFF_MODE      0x02
#define IBRT_PARK_MODE       0x03
#define IBRT_SCATTER_MODE    0x04

typedef enum
{
    NO_LINK_TYPE,
    SNOOP_LINK,
    MOBILE_LINK,
    TWS_LINK,
} ibrt_link_type_e;

typedef uint8_t ibrt_role_e;
#define   IBRT_MASTER       0
#define   IBRT_SLAVE        1
#define   IBRT_UNKNOW       0xff

/*
 * tx data protect in tws switch
 */
#ifndef __TWS_SWITCH_TX_DATA_PROTECT__
#define __TWS_SWITCH_TX_DATA_PROTECT__
#endif

typedef uint8_t tss_state_e;
#define  TSS_IDLE_STATE          (0x00)
#define  TSS_CRITICAL_WAIT_STATE (0x01)

#define  TSS_TX_WAIT_STATE       (0x02)
#define  TSS_TIMER_WAIT_STATE    (0x04)
#define  TSS_CMD_STATE           (0x08)
#define  TSS_SLAVE_WAIT_STATE    (0x10)


/*
 * wait some high priority criticals to be finished
 * such as sending accept response of avdtp start/suspend cmd
 * then launch tws switch
 */
#define  TSS_DELAY_CRITICAL_TIMEOUT_MS  50

/*
 * wait profile layer tx data to hci txbuf list
 * wait controller tx data to air
 */
#define  TSS_DELAY_HCI_CMD_TIMEOUT_MS  20



// hci tx buf type mapping
#define IBRT_HBT_TX_ACL     0x05  //#define HBT_TX_ACL   0x05
#define IBRT_HBT_TX_BLE     0x08  //#define HBT_TX_BLE   0x08

#define    CODING_FORMAT_CVSD          0x02
#define    CODING_FORMAT_TRANSP        0x03

typedef struct
{
    uint8_t  codec_type;
    uint8_t  sample_bit;
    uint8_t  sample_rate;
    uint8_t  vendor_para;
} __attribute__((packed)) ibrt_codec_t;

typedef struct
{
    uint8_t  backup_index;
    uint8_t  pickup_index;
    uint8_t  array_counter;
    btif_accessible_mode_t access_mode_array[ACCESS_MODE_ARRAY_MAX];
} access_mode_backup_t;

typedef enum {
    AUDIO_CHANNEL_SELECT_STEREO,
    AUDIO_CHANNEL_SELECT_LRMERGE,
    AUDIO_CHANNEL_SELECT_LCHNL,
    AUDIO_CHANNEL_SELECT_RCHNL,
} AUDIO_CHANNEL_SELECT_E;

typedef enum {
    IBRT_ROLE_SWITCH_USER_AI = 0,
    IBRT_ROLE_SWITCH_USER_OTA,
    IBRT_ROLE_SWITCH_USER_COUNT
}IBRT_ROLE_SWITCH_USER_E;

typedef struct
{
    bool lowlayer_monitor_enable;

    uint16_t long_private_poll_interval ;
    uint16_t default_private_poll_interval;
    uint16_t short_private_poll_interval;

    uint16_t default_private_poll_interval_in_sco;
    uint16_t short_private_poll_interval_in_sco;
    uint16_t default_bt_tpoll;

    bool     tws_switch_tx_data_protect;
    uint32_t tws_cmd_send_timeout;
    uint32_t tws_cmd_send_counter_threshold;
    uint32_t tws_switch_stable_timeout;

    uint16_t mobile_page_timeout;
    uint16_t tws_connection_timeout;

    bool delay_exit_sniff;
    uint32_t delay_ms_exit_sniff;
    uint8_t audio_sync_mismatch_resume_version;
} ibrt_core_config_t;

typedef struct
{
    int32_t mobile_diff_us;
    int32_t tws_diff_us;
}diff_us_t;
typedef struct
{
    ibrt_role_e init_done;
    ibrt_role_e nv_role;
    ibrt_role_e current_role;
    uint8_t role_switch_debonce_time;
    rssi_t raw_rssi;
    rssi_t peer_raw_rssi;
    int32_t mobile_diff_us;
    int32_t tws_diff_us;
    int8_t  cur_buf_size;
    uint16_t local_battery_volt;
    uint16_t peer_battery_volt;
    bt_bdaddr_t local_addr;
    bt_bdaddr_t peer_addr;
    bt_bdaddr_t mobile_addr;
    uint8_t mobile_linkKey[16];
    uint16_t ibrt_conhandle;
    uint8_t  ibrt_disc_reason;
    uint16_t tws_conhandle;
    uint16_t peer_tws_conhandle;
    uint16_t mobile_conhandle;//if local dev is connected with mobile
    uint16_t peer_mobile_conhandle;
    uint32_t mobile_constate;
    uint32_t tws_constate;
    uint32_t ibrt_constate;
    uint32_t ibrt_ai_role_switch_handle;    //one bit represent a AI
    uint32_t ibrt_role_switch_handle_user;  //one bit represent a user
    osMailQId tws_mailbox;
    ibrt_codec_t a2dp_codec;
    uint32_t     audio_chnl_sel;
    void *p_mobile_a2dp_profile;
    void *p_mobile_hfp_profile;
    void *p_mobile_avrcp_profile;
    void *p_mobile_map_profile;
    void *p_mobile_reseved0_profile;
    void *p_mobile_reseved1_profile;
    btif_remote_device_t *p_tws_remote_dev;
    btif_remote_device_t *p_mobile_remote_dev;
    btif_accessible_mode_t access_mode;
    bool access_mode_sending;
    bool is_ibrt_search_ui;
    uint8_t tws_switch_local_done: 1;
    uint8_t tws_switch_peer_done: 1;
    uint8_t master_tws_switch_pending: 1;
    uint8_t slave_tws_switch_pending: 1;
    uint8_t avrcp_register_notify_event;
    uint8_t avrcp_wait_register_notify_rsp;
    osTimerId wait_profile_ready_timer_id;
    osTimerId delay_profile_send_timer_id;
    osTimerId delay_hci_tss_cmd_timer_id;
    bool wait_profile;
    uint8_t rx_profile_update;
    uint8_t tx_profile_update;
    ibrt_link_mode_e tws_mode;
    ibrt_link_mode_e mobile_mode;
    access_mode_backup_t access_mode_backup;

    uint8_t ibrt_sco_activate;
    hfp_sco_codec_t ibrt_sco_codec;
    uint16_t ibrt_sco_hcihandle;
    uint8_t ibrt_sco_lmphdl;
    bool ibrt_in_poweroff;
    bool w4_ibrt_connected;
    bool mobile_encryp_done;
    uint32_t rx_seq_error;
    uint32_t tws_cmd_send_time;
    bool w4_stop_ibrt_for_rx_seq_error;
    bool dbg_state_timer_ongoing;
    bool stop_ibrt_timer_ongoing;
    bool ibrt_stopped_due_to_checker;
    bool phone_connect_happened;
    bool wait4_reset_complete;
    uint8_t reset_error_type;
    tss_state_e tss_state;
    bool mobile_pair_canceled;
    uint16_t sync_id_mask;
    uint8_t snoop_connected;
    bool w4_tws_exit_sniff;
    ibrt_core_config_t config;
    uint32_t  basic_profiles;
} ibrt_ctrl_t;

typedef struct
{
    ibrt_role_e nv_role;
    bt_bdaddr_t local_addr;
    bt_bdaddr_t peer_addr;
    bt_bdaddr_t mobile_addr;
    uint32_t    audio_chnl_sel;
} ibrt_config_t;

#ifdef __cplusplus
extern "C" {
#endif

void app_tws_ibrt_init(void);
void app_tws_ibrt_start(ibrt_config_t *config, bool is_search_ui);
void app_tws_ibrt_global_callback(const btif_event_t *event);
ibrt_link_type_e app_tws_ibrt_get_remote_link_type(btif_remote_device_t *remote_dev);
void app_tws_ibrt_set_a2dp_codec(const a2dp_callback_parms_t *info);
void app_tws_ibrt_profile_callback(uint32_t profile,void *param1,void *param2);
void app_tws_ibrt_spp_callback(void* spp_devi, void* spp_para);
struct BT_DEVICE_T* app_tws_ibrt_get_bt_device_ctx(void);
ibrt_ctrl_t* app_tws_ibrt_get_bt_ctrl_ctx(void);
bool app_tws_ibrt_mobile_link_connected(void);
bool app_tws_ibrt_tws_link_connected(void);
bt_status_t app_tws_ibrt_create_tws_connection(uint16_t page_timeout);
bt_status_t app_tws_ibrt_create_mobile_connection(uint16_t mobile_page_timeout,const bt_bdaddr_t *mobile_addr);
void app_tws_ibrt_proctect_bt_tx_type(uint16_t length);
bt_status_t app_tws_ibrt_disconnect_connection(btif_remote_device_t *remdev);
void app_tws_ibrt_clear_mobile_reconnecting(void);
void app_tws_ibrt_clear_tws_reconnecting(void);
void app_tws_ibrt_set_tws_reconnecting(void);
bool app_tws_ibrt_is_reconnecting_tws(void);
void app_bt_ibrt_reconnect_mobile_profile(bt_bdaddr_t mobile_addr);
void app_tws_ibrt_reset_bdaddr_to_nv_original(void);
void app_tws_ibrt_set_bdaddr_to_nv_master(void);
bt_status_t app_tws_ibrt_init_access_mode(btif_accessible_mode_t mode);
bt_status_t app_tws_ibrt_set_access_mode(btif_accessible_mode_t mode);
bt_status_t app_tws_ibrt_do_mss_with_mobile(void);
bt_status_t app_tws_ibrt_do_mss_with_tws(void);
bt_status_t app_tws_ibrt_exit_sniff_with_mobile(void);
bt_status_t app_tws_ibrt_exit_sniff_with_tws(void);
btif_connection_role_t app_tws_ibrt_get_local_tws_role(void);
btif_connection_role_t app_tws_ibrt_get_local_mobile_role(void);
void app_tws_ibrt_delay_slave_create_connection(void const *para);
void app_tws_ibrt_store_mobile_link_key(uint8_t *p_link_key);
ibrt_link_type_e app_tws_ibrt_get_link_type_by_addr(bt_bdaddr_t *p_addr);
bool app_tws_ibrt_slave_ibrt_link_connected(void);
void app_tws_ibrt_disconnect_callback(const btif_event_t * event);
void app_tws_ibrt_timing_control(bool enable, uint8_t type, uint16_t acl_interv_acl, uint16_t acl_interv_sco);
void app_tws_ibrt_sniff_timeout_handler(evm_timer_t *timer, unsigned int *skipInternalHandler);
void app_tws_ibrt_sniff_callback(const btif_event_t *event);
void app_tws_ibrt_use_the_same_bd_addr(void);
btif_accessible_mode_t app_tws_ibrt_access_mode_pickup(void);
void app_tws_ibrt_access_mode_backup(btif_accessible_mode_t mode);
void app_tws_ibrt_sdp_disconnect_callback(const void *para);
uint8_t app_tws_ibrt_role_get_callback(const void *para);
const char* app_tws_ibrt_role2str(uint8_t role);
bool app_tws_ibrt_key_already_exist(void *bdaddr);
bool btdrv_get_page_pscan_coex_enable(void);
void app_tws_ibrt_conn_req_pre_treatment_callback(const void *bd_addr);
void app_tws_ibrt_set_tws_pravite_interval(uint16_t tws_conhandle, uint16_t interval);
int app_ibrt_release_cmd_semphore(void);
int app_ibrt_clear_cmd_semphore(void);
void app_tws_ibrt_disconnect_mobile(void);
void app_tws_ibrt_disconnect_all_connection(void);
bool app_tws_ibrt_get_phone_connect_happened(void);
void app_tws_ibrt_clear_phone_connect_happened(void);
void app_tws_ibrt_set_phone_connect_happened(void);
void app_tws_ibrt_hci_tx_buf_tss_process(void);
void app_tws_ibrt_delay_hci_tss_cmd_timer_cb(void const *para);
void app_tws_ibrt_switch_role(void);
void app_tws_ibrt_set_tss_state(tss_state_e state);
tss_state_e app_tws_ibrt_get_tss_state(void);
void app_tws_ibrt_reset_tss_state(void);
void app_tws_ibrt_set_controller_rx_flow_stop(void);
void app_tws_ibrt_clear_controller_rx_flow_stop(void);
uint8_t app_tws_ibrt_get_audio_voice_active_state(void);
uint8_t app_tws_ibrt_get_tss_state_callback(const void *para);

bool app_tws_ibrt_sync_airmode_check_ind_handler(uint8_t voice_setting);
void app_tws_ibrt_register_esco_auto_accept(void);
btif_remote_device_t *app_tws_get_mobile_rem_dev(void);
uint32_t app_tws_get_mobile_conn_state(void);
uint32_t app_tws_get_tws_conn_state(void);
uint16_t app_tws_get_tws_conhdl(void);
bool app_tws_ibrt_sync_airmode_check_ind_handler(uint8_t voice_setting);
uint8_t app_tws_ibrt_nv_role_get_callback(const void *para);
void app_tws_ibrt_update_basic_profiles(uint32_t profile_id,bool is_add);

#ifdef __cplusplus
}
#endif
#endif

