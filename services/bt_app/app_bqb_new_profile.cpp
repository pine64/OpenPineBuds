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

#if defined(__BQB_PROFILE_TEST__) && defined(ENHANCED_STACK)
#include "hal_trace.h"
#include "app_key.h"
#include "conmgr_api.h"
#include "bt_if.h"

static bt_bdaddr_t pts_bt_addr = {{
#if 1
    0x14, 0x71, 0xda, 0x7d, 0x1a, 0x00
#else
    0x13, 0x71, 0xda, 0x7d, 0x1a, 0x00
#endif
}};

void A2dp_pts_Create_Avdtp_Signal_Channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!A2dp_pts_Create_Avdtp_Signal_Channel\n");
    btif_pts_av_create_channel(&pts_bt_addr);
}

void A2dp_pts_set_sink_delay(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!A2dp_pts_set_sink_delay\n");
    btif_pts_av_set_sink_delay();
}

void Avctp_pts_avrcp_connect(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Avctp_pts_avrcp_connect\n");
    btif_pts_ar_connect(&pts_bt_addr);
}

void Avrcp_pts_volume_change_notify(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Avrcp_pts_volume_change_notify\n");
    btif_pts_ar_volume_notify();
}

void Avrcp_pts_set_absolute_volume(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"app_avrcp_set_absolute_volume\n");
    btif_pts_ar_set_absolute_volume();
}

void Hfp_pts_create_service_level_channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_create_service_level_channel\n");
    btif_pts_hf_create_service_link(&pts_bt_addr);
}

void Hfp_pts_disconnect_service_link(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_disconnect_service_link\n");
    btif_pts_hf_disc_service_link();
}

void Hfp_pts_create_audio_link(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_create_audio_link\n");
    btif_pts_hf_create_audio_link();
}

void Hfp_pts_enable_voice_recognition(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_enable_voice_recognition\n");
    btif_pts_hf_vr_enable();
}

void Hfp_pts_disable_voice_recognition(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_disable_voice_recognition\n");
    btif_pts_hf_vr_disable();
}

void Hfp_pts_list_current_calls(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_list_current_calls\n");
    btif_pts_hf_list_current_calls();
}

void Hfp_pts_release_active_call_index2(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_release_active_call_index2\n");
    btif_pts_hf_release_active_call_2();
}

void Hfp_pts_hold_active_call_index2(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_hold_active_call_index2\n");
    btif_pts_hf_hold_active_call_2();
}

void Hfp_pts_release_active_call(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_release_active_call\n");
    btif_pts_hf_release_active_call();
}

void Hfp_pts_hf_indicators_1(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_hf_indicators_1\n");
    btif_pts_hf_send_ind_1();
}

void Hfp_pts_hf_indicators_2(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_hf_indicators_2\n");
    btif_pts_hf_send_ind_2();
}

void Hfp_pts_hf_indicators_3(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_hf_indicators_3\n");
    btif_pts_hf_send_ind_3();
}

void Hfp_pts_update_indicators_value(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!Hfp_pts_update_indicators_value\n");
    btif_pts_hf_update_ind_value();
}

void rfcomm_pts_register_channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"rfcomm_pts_register_channel\n");
    btif_pts_rfc_register_channel();
}

void rfcomm_pts_rfcomm_close(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"rfcomm_pts_rfcomm_close\n");
    btif_pts_rfc_close();
}

void rfcomm_pts_rfcomm_close_dlci0(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"rfcomm_pts_rfcomm_close_dlci0\n");
    btif_pts_rfc_close_dlci_0();
}

void rfcomm_pts_rfcomm_send_data(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"rfcomm_pts_rfcomm_send_data\n");
    btif_pts_rfc_send_data();
}

void Avdtp_pts_send_discover_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_discover_cmd\n");
    btif_pts_av_send_discover();
}

void Avdtp_pts_send_get_capability_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_get_capability_cmd\n");
    btif_pts_av_send_getcap();
}

void Avdtp_pts_send_set_configuration_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_set_configuration_cmd\n");
    btif_pts_av_send_setconf();
}

void Avdtp_pts_send_get_configuration_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_get_configuration_cmd\n");
    btif_pts_av_send_getconf();
}

void Avdtp_pts_send_reconfigure_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_reconfigure_cmd\n");
    btif_pts_av_send_reconf();
}

void Avdtp_pts_send_open_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_open_cmd\n");
    btif_pts_av_send_open();
}

void Avdtp_pts_send_close_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_close_cmd\n");
    btif_pts_av_send_close();
}

void Avdtp_pts_send_abort_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_abort_cmd\n");
    btif_pts_av_send_abort();
}

void Avdtp_pts_send_get_all_capability_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_get_all_capability_cmd\n");
    btif_pts_av_send_getallcap();
}

void Avdtp_pts_send_suspend_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_suspend_cmd\n");
    btif_pts_av_send_suspend();
}

void Avdtp_pts_send_start_cmd(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_send_start_cmd\n");
    btif_pts_av_send_start();
}

void Avdtp_pts_create_media_channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"Avdtp_pts_create_media_channel\n");
    btif_pts_av_create_media_channel();
}

void l2cap_pts_disconnect_channel(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!l2cap_pts_disconnect_channel\n");
    btif_pts_l2c_disc_channel();
}

void l2cap_pts_send_data(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"!!!l2cap_pts_send_data\n");
    btif_pts_l2c_send_data();
}

void pts_spp_client_init(void);
btif_cmgr_handler_t* pts_cmgr_handler;
bt_status_t btif_cmgr_create_data_link(btif_cmgr_handler_t *cmgr_handler,
                                          bt_bdaddr_t *bd_addr);
bt_status_t btif_cmgr_register_handler(btif_cmgr_handler_t *cmgr_handler,
                                          btif_cmgr_callback callback);
void pts_cmgr_callback(btif_cmgr_handler_t *cHandler,
                          cmgr_event_t    Event,
                          bt_status_t     Status);
void spp_pts_client_open(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"spp_pts_client_open\n");

    pts_cmgr_handler = btif_cmgr_handler_create();
    btif_cmgr_register_handler(pts_cmgr_handler,pts_cmgr_callback);
    pts_spp_client_init();
    btif_cmgr_create_data_link(pts_cmgr_handler,&pts_bt_addr);

}

extern struct gap_bdaddr BLE_BdAddr;
extern "C" void pts_ble_addr_init(void);
extern "C" void appm_start_connecting(struct gap_bdaddr* ptBdAddr);
void ble_pts_appm_start_connecting(APP_KEY_STATUS *status, void *param)
{
    TRACE(0,"ble_pts_appm_start_connecting\n");
    pts_ble_addr_init();
    appm_start_connecting(&BLE_BdAddr);
}

#endif
