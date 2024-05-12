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
#ifndef __APP_IBRT_IF__
#define __APP_IBRT_IF__
#include "cmsis_os.h"
#include "app_tws_ibrt.h"
#include "app_ibrt_ui.h"
#include "app_key.h"

typedef enum {
    APP_IBRT_IF_STATUS_SUCCESS                  = 0,
    APP_IBRT_IF_STATUS_PENDING                  = 1,
    APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS = 2,
    APP_IBRT_IF_STATUS_ERROR_NO_CONNECTION      = 3,
    APP_IBRT_IF_STATUS_ERROR_CONNECTION_EXISTS  = 4,
    APP_IBRT_IF_STATUS_IN_PROGRESS              = 5,
    APP_IBRT_IF_STATUS_ERROR_DUPLICATE_REQUEST  = 6,
    APP_IBRT_IF_STATUS_ERROR_INVALID_STATE      = 7,
    APP_IBRT_IF_STATUS_ERROR_TIMEOUT            = 8,
    APP_IBRT_IF_STATUS_ERROR_ROLE_SWITCH_FAILED = 9,
    APP_IBRT_IF_STATUS_ERROR_UNEXPECTED_VALUE  = 10,
    APP_IBRT_IF_STATUS_ERROR_OP_NOT_ALLOWED    = 11,
} AppIbrtStatus;


typedef enum {
    APP_IBRT_IF_A2DP_IDLE                             = 0,
    APP_IBRT_IF_A2DP_CODEC_CONFIGURED = 1,
    APP_IBRT_IF_A2DP_OPEN                          = 2,
    APP_IBRT_IF_A2DP_STREAMING                = 3,
    APP_IBRT_IF_A2DP_CLOSED                       = 4,
}AppIbrtA2dpState;

typedef enum {
    APP_IBRT_IF_AVRCP_DISCONNECTED   = 0,
    APP_IBRT_IF_AVRCP_CONNECTED      = 1,
    APP_IBRT_IF_AVRCP_PLAYING        = 2,
    APP_IBRT_IF_AVRCP_PAUSED         = 3,
    APP_IBRT_IF_AVRCP_VOLUME_UPDATED = 4,
}AppIbrtAvrcpState;

typedef enum {
    APP_IBRT_IF_HFP_SLC_DISCONNECTED = 0,
    APP_IBRT_IF_HFP_CLOSED           = 1,
    APP_IBRT_IF_HFP_SCO_CLOSED       = 2,
    APP_IBRT_IF_HFP_PENDING          = 3,
    APP_IBRT_IF_HFP_SLC_OPEN         = 4,
    APP_IBRT_IF_HFP_NEGOTIATE        = 5,
    APP_IBRT_IF_HFP_CODEC_CONFIGURED = 6,
    APP_IBRT_IF_HFP_SCO_OPEN         = 7,
    APP_IBRT_IF_HFP_INCOMING_CALL    = 8,
    APP_IBRT_IF_HFP_OUTGOING_CALL    = 9,
    APP_IBRT_IF_HFP_RING_INDICATION  = 10,
} AppIbrtHfpState;

typedef enum {
    APP_IBRT_IF_NO_CALL               = 0,
    APP_IBRT_IF_CALL_ACTIVE         = 1,
    APP_IBRT_IF_HOLD                    = 2,
    APP_IBRT_IF_SETUP_INCOMMING         = 3,
    APP_IBRT_IF_SETUP_OUTGOING         = 4,
    APP_IBRT_IF_SETUP_ALERT        = 5,
} AppIbrtCallStatus;


#ifdef __cplusplus
extern "C" {
#endif
typedef APP_IBRT_UI_VENDER_EVENT_HANDLER_IND APP_IBRT_IF_VENDER_EVENT_HANDLER_IND;
typedef APP_IBRT_UI_GLOBAL_HANDLER_IND APP_IBRT_IF_GLOBAL_HANDLER_IND;
typedef APP_IBRT_UI_GLOBAL_EVENT_UPDATE_IND APP_IBRT_IF_GLOBAL_EVENT_UPDATE_IND;
typedef APP_IBRT_UI_PROFILE_STATE_CHANGE_IND APP_IBRT_IF_PROFILE_STATE_CHANGE_IND;
typedef APP_IBRT_UI_CONNECT_MOBILE_HANDLER_IND APP_IBRT_IF_CONNECT_MOBILE_NEEDED_IND;
typedef APP_IBRT_UI_TWS_SWITCH_HANDLER_IND APP_IBRT_IF_TWS_SWITCH_NEEDED_IND;

typedef app_ibrt_ui_t app_ibrt_if_ui_t;
typedef ibrt_ctrl_t app_ibrt_if_ctrl_t;

enum APP_IBRT_IF_SNIFF_CHECKER_USER_T
{
    APP_IBRT_IF_SNIFF_CHECKER_USER_HFP,
    APP_IBRT_IF_SNIFF_CHECKER_USER_A2DP,
    APP_IBRT_IF_SNIFF_CHECKER_USER_SPP,

    APP_IBRT_IF_SNIFF_CHECKER_USER_QTY
};

enum APP_IBRT_IF_SLEEP_HOOK_BLOCKER_T
{
   APP_IBRT_IF_SLEEP_HOOK_BLOCKER_A2DP_STREAMING = 1<<0,
   APP_IBRT_IF_SLEEP_HOOK_BLOCKER_HFP_SCO        = 1<<1,

   APP_IBRT_IF_SLEEP_HOOK_BLOCKER_ALL            = (1<<0)|(1<<1),
};

void app_ibrt_if_register_global_handler_ind(APP_IBRT_IF_GLOBAL_HANDLER_IND handler);
void app_ibrt_if_register_vender_handler_ind(APP_IBRT_IF_VENDER_EVENT_HANDLER_IND handler);
void app_ibrt_if_register_connect_mobile_needed_ind(APP_IBRT_IF_CONNECT_MOBILE_NEEDED_IND connect_moible_need_ind);
void app_ibrt_if_register_tws_switch_needed_ind(APP_IBRT_IF_TWS_SWITCH_NEEDED_IND tws_switch_need_ind);
void app_ibrt_if_register_global_event_update_ind(APP_IBRT_IF_GLOBAL_EVENT_UPDATE_IND handler);
void app_ibrt_if_register_link_connected_ind(APP_IBRT_UI_CONNECTED_HANDLER_IND mobile_connected_ind,
                                             APP_IBRT_UI_CONNECTED_HANDLER_IND ibrt_connected_ind,
                                             APP_IBRT_UI_CONNECTED_HANDLER_IND tws_connected_ind);
void app_ibrt_if_register_profile_state_change_ind(APP_IBRT_IF_PROFILE_STATE_CHANGE_IND handler);
void app_ibrt_if_register_pairing_mode_ind(APP_IBRT_UI_PAIRING_MODE_HANDLER_IND set_callback, APP_IBRT_UI_PAIRING_MODE_HANDLER_IND clear_callback);

int app_ibrt_if_config_load(ibrt_config_t *config);
int app_ibrt_if_reconfig(ibrt_config_t *config);
int app_ibrt_if_ui_reconfig(ibrt_ui_config_t *config);
int app_ibrt_core_if_reconfig(ibrt_config_t *config);
int app_ibrt_if_config(ibrt_ui_config_t *ui_config);
int app_ibrt_if_event_entry(ibrt_event_type event);
void app_ibrt_if_choice_connect_second_mobile(void);
void app_ibrt_if_choice_mobile_connect(uint8_t index);
void app_ibrt_if_disconnect_mobile_tws_link(void);
app_ibrt_if_ui_t* app_ibrt_if_ui_get_ctx(void);
app_ibrt_if_ctrl_t *app_ibrt_if_get_bt_ctrl_ctx(void);
void app_ibrt_if_enter_pairing_after_tws_connected(void);
void app_ibrt_if_enter_freeman_pairing(void);
int app_ibrt_if_voice_report_trig_retrigger(void);
int app_ibrt_if_force_audio_retrigger(void);
int app_ibrt_if_force_audio_retrigger_slave_sync(void);
int app_ibrt_if_keyboard_notify(APP_KEY_STATUS *status, void *param);
void app_ibrt_if_a2dp_lowlatency_scan(uint16_t interval, uint16_t window, uint8_t interlanced);
void app_ibrt_if_a2dp_restore_scan(void);
void app_ibrt_if_sco_lowlatency_scan(uint16_t interval, uint16_t window, uint8_t interlanced);
void app_ibrt_if_sco_restore_scan(void);
#ifdef IBRT_SEARCH_UI
void app_start_tws_serching_direactly();
void app_bt_manager_ibrt_role_process(const btif_event_t *Event);
void app_ibrt_search_ui_init(bool boxOperation,ibrt_event_type evt_type);
void app_ibrt_enter_limited_mode(void);
void app_ibrt_reconfig_btAddr_from_nv();
void app_bt_enter_mono_pairing_mode(void);
#endif

void app_ibrt_ui_automate_test_cmd_handler(uint8_t group_code, uint8_t operation_code, uint8_t *param, uint8_t param_len);
void app_ibrt_auto_test_inform_cmd_received(uint8_t group_code, uint8_t operation_code);
int app_ibrt_ui_test_cmd_handler(unsigned char *buf, unsigned int length);
void app_ibrt_if_ctx_checker(void);
extern "C" bool btdrv_get_page_pscan_coex_enable(void);
int app_ibrt_if_tws_sniff_block(uint32_t block_next_sec);
int app_ibrt_if_sniff_checker_start(enum APP_IBRT_IF_SNIFF_CHECKER_USER_T user);
int app_ibrt_if_sniff_checker_stop(enum APP_IBRT_IF_SNIFF_CHECKER_USER_T user);
int app_ibrt_if_sniff_checker_init(uint32_t delay_ms);
int app_ibrt_if_sniff_checker_reset(void);

void app_ibrt_if_exec_sleep_hook_blocker_set(APP_IBRT_IF_SLEEP_HOOK_BLOCKER_T blocker);
void app_ibrt_if_exec_sleep_hook_blocker_clr(APP_IBRT_IF_SLEEP_HOOK_BLOCKER_T blocker);
bool app_ibrt_if_exec_sleep_hook_blocker_is_a2dp_streaming(void);
bool app_ibrt_if_exec_sleep_hook_blocker_is_hfp_sco(void);
bool app_ibrt_if_exec_sleep_hook_allowed(void);
int app_ibrt_if_config_keeper_clear(void);
int app_ibrt_if_config_keeper_mobile_update(bt_bdaddr_t *addr);
int app_ibrt_if_config_keeper_tws_update(bt_bdaddr_t *addr);
void app_ibrt_ui_adaptive_fa_rx_gain(void);
void app_tws_ibrt_record_sync_id(void);
void app_tws_ibrt_resume_sync_id(void);
void app_ibrt_if_pairing_mode_refresh(void);
bool app_ibrt_if_tws_switch_prepare_needed(uint32_t *wait_ms);
void app_ibrt_if_tws_swtich_prepare(void);
void app_ibrt_if_tws_switch_prepare_done_in_bt_thread(IBRT_ROLE_SWITCH_USER_E user, uint32_t role);
bool app_ibrt_if_start_ibrt_onporcess(void);
bool app_ibrt_if_tws_switch_onporcess(void);
AppIbrtStatus app_ibrt_if_get_a2dp_state(AppIbrtA2dpState *a2dp_state);
AppIbrtStatus app_ibrt_if_get_avrcp_state(AppIbrtAvrcpState *avrcp_state);
AppIbrtStatus app_ibrt_if_get_hfp_state(AppIbrtHfpState *hfp_state);
AppIbrtStatus app_ibrt_if_get_hfp_call_status(AppIbrtCallStatus *call_status);
ibrt_role_e app_ibrt_if_get_ibrt_role();



#ifdef __cplusplus
}
#endif                          /*  */

#endif
