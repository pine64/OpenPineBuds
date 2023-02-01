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
#include "app_ibrt_customif_cmd.h"
#include "app_dip.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt_trace.h"
#include "app_tws_if.h"
#include "string.h"
#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#endif
#ifdef __DUAL_MIC_RECORDING__
#include "ai_control.h"
#include "app_recording_handle.h"
#endif
#include "apps.h"
#if defined(IBRT)
/*
 * custom cmd handler add here, this is just a example
 */

#define app_ibrt_custom_cmd_rsp_timeout_handler_null (0)
#define app_ibrt_custom_cmd_rsp_handler_null (0)
#define app_ibrt_custom_cmd_rx_handler_null (0)

#ifdef BISTO_ENABLED
static void app_ibrt_bisto_dip_sync(uint8_t *p_buff, uint16_t length);
static void app_ibrt_bisto_dip_sync_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length);
#endif
#ifdef __DUAL_MIC_RECORDING__
static void app_ibrt_customif_audio_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_audio_send_done(uint16_t cmdcode,
                                              uint16_t rsp_seq,
                                              uint8_t *ptrParam,
                                              uint16_t paramLen);
static void app_ibrt_customif_audio_send_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length);
#endif
void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length);

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(
    uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq,
                                                         uint8_t *p_buff,
                                                         uint16_t length);
static void app_ibrt_customif_test3_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length);

static void app_ibrt_customif_test4_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length);
static const app_tws_cmd_instance_t g_ibrt_custom_cmd_handler_table[] = {
#ifdef GFPS_ENABLED
    {APP_TWS_CMD_SHARE_FASTPAIR_INFO, "SHARE_FASTPAIR_INFO",
     app_ibrt_share_fastpair_info,
     app_ibrt_shared_fastpair_info_received_handler, 0,
     app_ibrt_custom_cmd_rsp_timeout_handler_null,
     app_ibrt_custom_cmd_rsp_handler_null},
#endif

#ifdef BISTO_ENABLED
    {APP_TWS_CMD_BISTO_DIP_SYNC, "BISTO_DIP_SYNC", app_ibrt_bisto_dip_sync,
     app_ibrt_bisto_dip_sync_handler, 0, app_ibrt_cmd_rsp_timeout_handler_null,
     app_ibrt_cmd_rsp_handler_null},
#endif
#ifdef __DUAL_MIC_RECORDING__
    {
        APP_IBRT_CUSTOM_CMD_DMA_AUDIO,
        "TWS_CMD_DMA_AUDIO",
        app_ibrt_customif_audio_send,
        app_ibrt_customif_audio_send_handler,
        0,
        app_ibrt_custom_cmd_rsp_timeout_handler_null,
        app_ibrt_custom_cmd_rsp_handler_null,
        app_ibrt_customif_audio_send_done,
    },
#endif
    {APP_IBRT_CUSTOM_CMD_TEST1, "TWS_CMD_TEST1",
     app_ibrt_customif_test1_cmd_send, app_ibrt_customif_test1_cmd_send_handler,
     0, app_ibrt_custom_cmd_rsp_timeout_handler_null,
     app_ibrt_custom_cmd_rsp_handler_null},
    {APP_IBRT_CUSTOM_CMD_TEST2, "TWS_CMD_TEST2",
     app_ibrt_customif_test2_cmd_send, app_ibrt_customif_test2_cmd_send_handler,
     RSP_TIMEOUT_DEFAULT, app_ibrt_customif_test2_cmd_send_rsp_timeout_handler,
     app_ibrt_customif_test2_cmd_send_rsp_handler},
    {APP_IBRT_CUSTOM_CMD_TEST3, "TWS_CMD_TEST3",
     app_ibrt_customif_test3_cmd_send, app_ibrt_customif_test3_cmd_send_handler,
     0, app_ibrt_custom_cmd_rsp_timeout_handler_null,
     app_ibrt_custom_cmd_rsp_handler_null},
    {APP_IBRT_CUSTOM_CMD_TEST4, "TWS_CMD_TEST4",
     app_ibrt_customif_test4_cmd_send, app_ibrt_customif_test4_cmd_send_handler,
     0, app_ibrt_custom_cmd_rsp_timeout_handler_null,
     app_ibrt_custom_cmd_rsp_handler_null},
};

int app_ibrt_customif_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size) {
  *cmd_tbl = (void *)&g_ibrt_custom_cmd_handler_table;
  *cmd_size = ARRAY_SIZE(g_ibrt_custom_cmd_handler_table);
  return 0;
}

#ifdef BISTO_ENABLED
static void app_ibrt_bisto_dip_sync(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_BISTO_DIP_SYNC, p_buff, length);
}

static void app_ibrt_bisto_dip_sync_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length) {
  gsound_mobile_type_get_callback(*(MOBILE_CONN_TYPE_E *)p_buff);
}
#endif
#ifdef __DUAL_MIC_RECORDING__
static void app_ibrt_customif_audio_send(uint8_t *p_buff, uint16_t length) {
  TRACE(1, "%s", __func__);
  app_recording_send_data_to_master();
}

static void app_ibrt_customif_audio_send_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  ai_function_handle(CALLBACK_STORE_SLAVE_DATA, (void *)p_buff, length);
  // TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_audio_send_done(uint16_t cmdcode,
                                              uint16_t rsp_seq,
                                              uint8_t *ptrParam,
                                              uint16_t paramLen) {
  TRACE(1, "%s", __func__);
  app_recording_audio_send_done();
}
#endif
void app_ibrt_customif_cmd_test(ibrt_custom_cmd_test_t *cmd_test) {
  tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST1, (uint8_t *)cmd_test,
                    sizeof(ibrt_custom_cmd_test_t));
  tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST2, (uint8_t *)cmd_test,
                    sizeof(ibrt_custom_cmd_test_t));
  tws_ctrl_send_cmd(APP_IBRT_CUSTOM_CMD_TEST3, (uint8_t *)cmd_test,
                    sizeof(ibrt_custom_cmd_test_t));
}

void app_ibrt_customif_test1_cmd_send(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_TEST1, p_buff, length);
  TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_test1_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length) {
  TRACE(1, "%s", __func__);
  app_ibrt_search_ui_handle_key((APP_KEY_STATUS *)p_buff, NULL);
}

static void app_ibrt_customif_test2_cmd_send(uint8_t *p_buff, uint16_t length) {
  TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length) {
  tws_ctrl_send_rsp(APP_IBRT_CUSTOM_CMD_TEST2, rsp_seq, NULL, 0);
  TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send_rsp_timeout_handler(
    uint16_t rsp_seq, uint8_t *p_buff, uint16_t length) {
  TRACE(1, "%s", __func__);
}

static void app_ibrt_customif_test2_cmd_send_rsp_handler(uint16_t rsp_seq,
                                                         uint8_t *p_buff,
                                                         uint16_t length) {
  TRACE(1, "%s", __func__);
}

void app_ibrt_customif_test3_cmd_send(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_TEST3, p_buff, length);
  TRACE(1, "%s", __func__);
}

extern void app_ibrt_ui_test_mtu_change_sync_notify(void);
static void app_ibrt_customif_test3_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length) {
  /*TRACE(3,"%s,latency_mode_is_open = %d", __func__,latency_mode_is_open);
  if(*p_buff == latency_mode_is_open)return;
  if(*p_buff == 0)
          latency_mode_is_open = 1;
  else if(*p_buff == 1)
          latency_mode_is_open = 0;
  app_ibrt_ui_test_mtu_change_sync_notify();*/
}

void app_ibrt_customif_test4_cmd_send(uint8_t *p_buff, uint16_t length) {
  TRACE(3, "%s", __func__);
  app_ibrt_send_cmd_without_rsp(APP_IBRT_CUSTOM_CMD_TEST4, p_buff, length);
}
extern void app_ibrt_sync_volume_info();
static void app_ibrt_customif_test4_cmd_send_handler(uint16_t rsp_seq,
                                                     uint8_t *p_buff,
                                                     uint16_t length) {
  TRACE(3, "!!!!app_ibrt_customif_test4_cmd_send_handler");
  app_ibrt_sync_volume_info();
}

#ifdef CUSTOM_BITRATE
//#include "product_config.h"
#include "nvrecord_extension.h"
extern void a2dp_avdtpcodec_aac_user_configure(uint32_t bitrate,
                                               uint8_t user_configure);
extern void a2dp_avdtpcodec_sbc_user_configure(uint32_t bitpool,
                                               uint8_t user_configure);
extern void app_audio_dynamic_update_dest_packet_mtu(uint8_t codec_index,
                                                     uint8_t packet_mtu,
                                                     uint8_t user_configure);

void app_ibrt_user_a2dp_info_sync_tws_share_cmd_send(uint8_t *p_buff,
                                                     uint16_t length) {
  tws_ctrl_send_cmd(APP_TWS_CMD_A2DP_CONFIG_SYNC, p_buff, length);
}

static void app_ibrt_user_a2dp_info_sync(uint8_t *p_buff, uint16_t length) {
  if ((app_tws_ibrt_mobile_link_connected())) {
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_A2DP_CONFIG_SYNC, p_buff, length);
  }
}

static void app_ibrt_user_a2dp_info_sync_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  ibrt_custome_codec_t *a2dp_user_config_ptr = (ibrt_custome_codec_t *)p_buff;
  TRACE(4, "%s %d %d %d", __func__, a2dp_user_config_ptr->aac_bitrate,
        a2dp_user_config_ptr->sbc_bitpool,
        a2dp_user_config_ptr->audio_latentcy);
  if ((app_tws_ibrt_slave_ibrt_link_connected())) {
    a2dp_avdtpcodec_sbc_user_configure(a2dp_user_config_ptr->sbc_bitpool, true);
    a2dp_avdtpcodec_aac_user_configure(a2dp_user_config_ptr->aac_bitrate, true);
    app_audio_dynamic_update_dest_packet_mtu(
        0,
        (a2dp_user_config_ptr->audio_latentcy -
         USER_CONFIG_AUDIO_LATENCY_ERROR) /
            3,
        true); // sbc
    app_audio_dynamic_update_dest_packet_mtu(
        1,
        (a2dp_user_config_ptr->audio_latentcy -
         USER_CONFIG_AUDIO_LATENCY_ERROR) /
            23,
        true); // aac
    uint32_t lock = nv_record_pre_write_operation();
    nv_record_get_extension_entry_ptr()->a2dp_user_info.aac_bitrate =
        a2dp_user_config_ptr->aac_bitrate;
    nv_record_get_extension_entry_ptr()->a2dp_user_info.sbc_bitpool =
        a2dp_user_config_ptr->sbc_bitpool;
    nv_record_get_extension_entry_ptr()->a2dp_user_info.audio_latentcy =
        a2dp_user_config_ptr->audio_latentcy;
    nv_record_post_write_operation(lock);
    nv_record_flash_flush();
  }
}
#endif
#endif
