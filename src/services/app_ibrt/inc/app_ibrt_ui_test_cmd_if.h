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
#ifndef __APP_IBRT_UI_TEST_CMD_IF_H__
#define __APP_IBRT_UI_TEST_CMD_IF_H__
#include <stdint.h>

extern void app_bt_volumedown();
extern void app_bt_volumeup();
extern void app_ibrt_nvrecord_rebuild(void);

#ifdef __cplusplus
extern "C" {
#endif

void btif_pts_hf_create_link_with_pts(void);
void btif_pts_av_create_channel_with_pts(void);
void btif_pts_ar_connect_with_pts(void);


void app_ibrt_ui_audio_play_test(void);
void app_ibrt_ui_audio_pause_test(void);
void app_ibrt_ui_audio_forward_test(void);
void app_ibrt_ui_audio_backward_test(void);
void app_ibrt_ui_avrcp_volume_up_test(void);
void app_ibrt_ui_avrcp_volume_down_test(void);
void app_ibrt_ui_hfsco_create_test(void);
void app_ibrt_ui_hfsco_disc_test(void);
void app_ibrt_ui_call_redial_test(void);
void app_ibrt_ui_call_answer_test(void);
void app_ibrt_ui_call_hangup_test(void);
void app_ibrt_ui_local_volume_up_test(void);
void app_ibrt_ui_local_volume_down_test(void);
void app_ibrt_ui_get_tws_conn_state_test(void);


void app_ibrt_ui_soft_reset_test(void);
void app_ibrt_ui_iic_uart_switch_test(void);
void app_ibrt_ui_get_a2dp_state_test(void);
void app_ibrt_ui_get_avrcp_state_test(void);
void app_ibrt_ui_get_hfp_state_test(void);
void app_ibrt_ui_get_call_status_test();
void app_ibrt_ui_get_ibrt_role_test(void);

void app_ibrt_ui_soft_reset_test(void);
void app_ibrt_ui_iic_uart_switch_test(void);
void app_ibrt_ui_open_box_event_test(void);
void app_ibrt_ui_fetch_out_box_event_test(void);
void app_ibrt_ui_put_in_box_event_test(void);
void app_ibrt_ui_close_box_event_test(void);
void app_ibrt_ui_reconnect_event_test(void);
void app_ibrt_ui_ware_up_event_test(void);
void app_ibrt_ui_ware_down_event_test(void);
void app_ibrt_ui_phone_connect_event_test(void);
void app_ibrt_ui_shut_down_test(void);
void app_ibrt_enable_tports_test(void);
void app_ibrt_ui_tws_swtich_test(void);
void app_ibrt_ui_suspend_ibrt_test(void);
void app_ibrt_ui_resume_ibrt_test(void);
void app_ibrt_ui_pairing_mode_test(void);
void app_ibrt_ui_freeman_pairing_mode_test(void);
void app_ibrt_inquiry_start_test(void);
void app_ibrt_role_switch_test(void);

#ifdef __cplusplus
}
#endif

#endif

