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
#ifndef __APP_TWS_IBRT_CMD_HANDLER__
#define __APP_TWS_IBRT_CMD_HANDLER__
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_cmd_audio_analysis.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#include "app_ibrt_voice_report.h"
#include "app_ibrt_keyboard.h"

#define IBRT_PROFILE_ARRAY_SIZE                 (12)
#define IBRT_WAIT_PROFILE_TIMEOUT               (10000)
#define IBRT_NEW_PROFILE_WAIT_TIMEOUT           (245)

#define RSP_TIMEOUT_DEFAULT                     (5000)
#define RSP_TIMEOUT_FAST_ACK                    (1000)
#define APP_TWS_IBRT_MAX_DATA_SIZE              (672)
#define APP_TWS_IBRT_CMDCODE_SIZE               sizeof(uint16_t)
#define APP_TWS_IBRT_CMDSEQ_SIZE                sizeof(uint16_t)
#define APP_TWS_IBRT_CMDHEAD_SIZE               (APP_TWS_IBRT_CMDCODE_SIZE+APP_TWS_IBRT_CMDSEQ_SIZE)

#define IBRT_BESAUD_RX_BUFF_SIZE                (672)
#define IBRT_BESAUD_TX_BUFF_SIZE                (672*4)
#define IBRT_TIMEOUT_INVALID                    (0)
#define app_ibrt_cmd_rsp_timeout_handler_null   (0)
#define app_ibrt_cmd_rsp_handler_null           (0)
#define app_ibrt_cmd_rx_handler_null            (0)

#define RSP_TIMEOUT_VOICE_REPORT        (1000)

typedef int (*TWS_CMD_HANDLER_T)(void **cmd_tbl, uint16_t *cmd_size);

enum CMD_ID_T
{
    TWS_CMD_IBRT = 0,
    TWS_CMD_CUSTOMER,
    TWS_CMD_OTA,
    TWS_CMD_IBRT_OTA,
    TWS_CMD_NUM,
};

typedef enum
{
    APP_TWS_ACCEPT,
    APP_TWS_NOT_ACCEPT,
} app_tws_switch_e;

typedef enum
{
    APP_TWS_CMD_PROFILE_DATA_EXCHANGE               = 0x8001,
    APP_TWS_CMD_PROFILE_DATA_EXCHANGE_DONE          = 0x8002,
    APP_TWS_CMD_SET_CODEC_TYPE                      = 0x8003,
    APP_TWS_CMD_RSP                                 = 0x8004,
    APP_TWS_CMD_SHARE_TWS_INFO                      = 0x8005,
    APP_TWS_CMD_SWITCH_ROLE                         = 0x8006,
    APP_TWS_CMD_SEND_PLAYBACK_INFO                  = 0x8007,
    APP_TWS_CMD_SET_TRIGGER_TIME                    = 0x8008,
    APP_TWS_CMD_NEED_RETRIGGER                      = 0x8009,
    APP_TWS_CMD_SYNC_TUNE                           = 0x800A,
    APP_TWS_CMD_SET_LATENCYFACTOR                   = 0x800B,
    APP_TWS_CMD_START_IBRT                          = 0x800C,
    APP_TWS_CMD_STOP_IBRT                           = 0x800D,
    APP_TWS_CMD_SET_ENV                             = 0x800E,
    APP_TWS_CMD_PROFILE_DATA_REQ                    = 0x800F,
    APP_TWS_CMD_FAST_ACK_REQ                        = 0x8010,
    APP_TWS_CMD_A2DP_STATUS_SYNC                    = 0x8011,
    APP_TWS_CMD_HFP_STATUS_SYNC                     = 0x8012,
    APP_TWS_CMD_VOICE_REPORT_REQUEST                = 0x8013,
    APP_TWS_CMD_VOICE_REPORT_START                  = 0x8014,
    APP_TWS_CMD_KEYBOARD_REQUEST                    = 0x8015,
    APP_TWS_CMD_TRANSFER_SM                         = 0x8016,
    APP_TWS_CMD_SYNC_ANC_STATUS                     = 0x8017,
    APP_TWS_CMD_SET_INTERVAL                        = 0x8018,
    APP_TWS_CMD_NEW_MOBILE_INCOMING                 = 0x8019,
    APP_TWS_CMD_GET_PEER_MOBILE_RSSI                = 0x801A,
    APP_TWS_CMD_NOTIFY_LINK_POLICY                  = 0x801B,
    APP_TWS_CMD_SLAVE_CONNECT_BOX                   = 0x801C,
    APP_TWS_CMD_SLAVE_CONNECT_BOX_RSP               = 0x801D,
    APP_TWS_CMD_SLAVE_DISCONNECT_BOX_RSP            = 0x801E,
    APP_TWS_CMD_SEND_CONTROLLER_PROFILE             = 0x801F,
    APP_TWS_CMD_REV_SNOOP_INFO_COMPLETE             = 0x8020,
    APP_TWS_CMD_GET_PEER_RAW_RSSI                   = 0x8021,
    APP_TWS_CMD_SYNC_VOLUME_INFO                    = 0x8022,
    APP_TWS_CMD_PERFORM_ACTION                      = 0x8023,
    APP_TWS_CMD_EXIT_SNIFF_MODE                     = 0x8024,
    APP_TWS_CMD_STOP_IBRT_FAILED                    = 0x8025,
    APP_TWS_CMD_SHARE_COMMON_INFO                   = 0x8026,
    APP_TWS_CMD_SYNC_MIX_PROMPT_REQ                 = 0x8027,
    APP_TWS_CMD_STOP_PEER_PROMPT_REQ                = 0x8028,
    APP_TWS_CMD_LET_PEER_PLAY_PROMPT                = 0x8029,
    APP_TWS_CMD_PAIRING_MODE_REFRESH                = 0x802A,
    APP_TWS_CMD_LET_MASTER_PREPARE_RS               = 0x802B, // for slave triggered role switch, BISTO need the master to control the gsound role switch
    APP_TWS_CMD_LET_SLAVE_CONTINUE_RS               = 0x802C, // for slave triggered role switch, tell the slave gsound role switch has completed
    APP_TWS_CMD_AI_SEND_CMD_TO_PEER                 = 0x802D, // ai send cmd to peer whitout rsp
    APP_TWS_CMD_AI_SEND_CMD_TO_PEER_WITH_RSP        = 0x802E, // ai send cmd to peer whit rsp
    APP_TWS_CMD_MOBILE_LINK_PLAYBACK_INFO           = 0x802F,
    APP_TWS_CMD_DISC_MOBILE                         = 0x8030,
    APP_TWS_CMD_SPECIAL_ALL_CANCEL         = 0xffff,
} app_tws_cmd_code_e;

#define IBRT_TWS_PERFORM_USER_ACITON                0xff
#define IBRT_TWS_PERFORM_CONNECT_SECOND_MOBILE      0x01
#define IBRT_TWS_PERFORM_DISCONNECT_MOBILE_TWS_LINK 0x02
#define IBRT_TWS_PERFORM_PEER_TWS_SWITCH_DONE       0x03
#define IBRT_TWS_PERFORM_CHOICE_MOBILE_CONNECT      0x04
#define IBRT_TWS_PERFORM_AVRCP_NOTIFY_REGISTER      0x05

typedef void (*app_tws_cmd_send_handler_t)(uint8_t*, uint16_t);
typedef void (*app_tws_cmd_receivd_handler_t)(uint16_t, uint8_t*, uint16_t);
typedef void (*app_tws_rsp_timeout_handle_t)(uint16_t, uint8_t*, uint16_t);
typedef void (*app_tws_rsp_handle_t)(uint16_t, uint8_t*, uint16_t);
typedef void (*app_tws_cmd_tx_done_handler_t) (uint16_t, uint16_t, uint8_t*, uint16_t);

typedef struct
{
    uint32_t                        cmdcode;
    const char                      *log_cmd_code_str;
    app_tws_cmd_send_handler_t      tws_cmd_send;
    app_tws_cmd_receivd_handler_t   cmdhandler;             /**< command handler function */
    uint32_t                        timeout_ms;
    app_tws_rsp_timeout_handle_t    app_tws_rsp_timeout_handle;
    app_tws_rsp_handle_t            app_tws_rsp_handle;
    app_tws_cmd_tx_done_handler_t   app_tws_cmd_tx_done_handler;
} __attribute__((packed)) app_tws_cmd_instance_t;

typedef struct
{
    uint16_t                        opcode;
    uint8_t                         conhdl_pos;
    const char                      *log_cmd_code_str;
} __attribute__((packed)) app_ibrt_cmd_filter_t;

typedef struct
{
    uint16_t  cmdcode;
    uint16_t  cmdseq;
    uint8_t   content[APP_TWS_IBRT_MAX_DATA_SIZE];
} __attribute__((packed)) app_tws_ibrt_cmd_t;

void app_ibrt_cmd_handler_init(void);
int app_ibrt_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);
int app_ibrt_set_cmdhandle(enum CMD_ID_T cmd_id, TWS_CMD_HANDLER_T handler);
void app_ibrt_cmd_rx_handler(uint8_t* p_data_buff, uint16_t length);
void app_ibrt_data_send_handler(void);
void app_ibrt_data_receive_handler(void);
void app_ibrt_cmd_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
bool app_ibrt_send_cmd_without_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_cmd_with_rsp(uint16_t cmdcode, uint8_t *p_buff, uint16_t length);
bool app_ibrt_waiting_cmd_rsp(void);
void app_ibrt_profile_data_exchange_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_profiles(void);
void app_ibrt_send_tws_switch_cmd(uint8_t *p_buff, uint16_t length);
void app_ibrt_tws_switch_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_tws_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_share_tws_info_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_cmd_rsp(uint8_t *p_buff, uint16_t length);
void app_ibrt_inform_start_ibrt_mode(uint8_t *p_buff, uint16_t length);
void app_ibrt_start_ibrt_mode_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_inform_stop_ibrt_mode(uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_ibrt_mode_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_start_ibrt_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_ibrt_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_ibrt_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_start_ibrt_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_codec_type_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_codec_type_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_tws_info_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_tws_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_data_exchange_rsp_timeout_handler(uint8_t *p_buff, uint16_t length);
void app_ibrt_data_exchange_done(uint8_t *p_buff, uint16_t length);
void app_ibrt_data_exchange_done_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_env_cmd(uint8_t *p_buff, uint16_t length);
void app_ibrt_set_env_cmd_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_env_cmd_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_env_cmd_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_profile_data_req_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_profile_data_req(uint8_t *p_buff, uint16_t length);
uint8_t app_ibrt_send_profile_data(uint32_t profile_mask,bool tx_silence);
void app_ibrt_wait_profile_ready_timer_cb(void const *n);
void app_ibrt_delay_profile_send_timer_cb(void const *n);
void app_ibrt_delay_profile_send_timer(uint32_t millisec);
void app_ibrt_wait_profile_ready_timeout_handler(void);
void app_ibrt_profile_ready_notify(uint32_t profile);
uint8_t app_ibrt_profile_check_and_resync(void);
void app_ibrt_sync_profile_init_status(void);
void app_ibrt_fast_ack_req_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_fast_ack_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_ibrt_fast_ack_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_fast_ack_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
uint16_t app_ibrt_find_cmd_table_index(uint16_t cmdcode, app_tws_cmd_instance_t **cmd_tbl);
void app_ibrt_transfer_sm(uint8_t *p_buff, uint16_t length);
void app_ibrt_transfer_sm_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_anc_status(uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_anc_status_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_set_interval(uint8_t *p_buff, uint16_t length);
void app_ibrt_set_interval_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_perform_action(uint8_t *p_buff, uint16_t length);
void app_ibrt_perform_action_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_init_tws_trx_cmd_queue(void);
void app_ibrt_reset_tws_trx_cmd_queue(void);
void app_ibrt_new_mobile_incoming(uint8_t *p_buff, uint16_t length);
void app_ibrt_new_mobile_incoming_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_notify_link_policy(uint8_t *p_buff, uint16_t length);
void app_ibrt_notify_link_policy_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_connect_box(uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_connect_box_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_connect_box_rsp(uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_connect_box_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_disconnect_box_rsp(uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_disconnect_box_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_new_mobile_incoming_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_new_mobile_incoming_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_ui_debug_controller_timer_cb(void const *current_evt);
void app_ibrt_recieve_bt_controller_profile_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_bt_controller_profile(uint8_t *p_buff, uint16_t input_length);
void app_ibrt_send_snoop_info_send_complete(uint8_t *p_buff, uint16_t length);
void app_ibrt_snoop_info_received_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_volume_info_send(uint8_t *p_buff, uint16_t length);
void app_ibrt_get_volume_info_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_exit_sniff(uint8_t *p_buff, uint16_t length);
void app_ibrt_exit_sniff_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_ibrt_failed(uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_ibrt_failed_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_common_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_share_common_info_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_common_info_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_share_common_info_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_mix_prompt_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_sync_mix_prompt_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_prompt_play_req_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_prompt_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_peer_prompt_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_stop_peer_prompt_req(uint8_t *p_buff, uint16_t length);
void app_ibrt_pairing_mode_refresh(uint8_t *p_buff, uint16_t length);
void app_ibrt_pairing_mode_refresh_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_let_master_prepare_rs(uint8_t *p_buff, uint16_t length);
void app_ibrt_master_prepare_rs(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_let_slave_continue_rs(uint8_t *p_buff, uint16_t length);
void app_ibrt_slave_continue_rs(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_send_cmd_to_peer(uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_send_cmd_with_rsp_to_peer(uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_rev_cmd_with_rsp_to_peer_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_rev_cmd_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_ai_rev_cmd_rsp_from_peer_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
void app_ibrt_send_mobile_link_playback_info(uint8_t *p_buff, uint16_t length);
void app_ibrt_send_mobile_link_playback_info_handler(uint16_t rsp_seq, uint8_t *ptrParam, uint16_t paramLen);
void app_ibrt_disconnect_mobile(uint8_t *p_buff, uint16_t length);
void app_ibrt_disconnect_mobile_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
#endif

