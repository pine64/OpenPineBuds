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
#include "a2dp_api.h"
#include "analog.h"
#include "app.h"
#include "app_audio.h"
#include "audioflinger.h"
#include "bluetooth.h"
#include "bt_drv.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include <stdio.h>
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif

#include "a2dp_api.h"
#include "avrcp_api.h"
#include "besbt.h"

#include "app_bt.h"
#include "app_bt_media_manager.h"
#include "apps.h"
#include "bt_drv_interface.h"
#include "cqueue.h"
#include "hci_api.h"
#include "resources.h"
#include "tgt_hardware.h"

#define _FILE_TAG_ "A2DP"
#include "app_bt_func.h"
#include "color_log.h"
#include "os_api.h"

#if (A2DP_DECODER_VER >= 2)
#include "a2dp_decoder.h"
#endif

#include "app_a2dp.h"
#include "app_a2dp_codecs.h"
#include "btapp.h"

extern struct BT_DEVICE_T app_bt_device;

int a2dp_codec_source_init(void) {
  struct BT_DEVICE_T POSSIBLY_UNUSED *bt_dev = &app_bt_device;

#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
  if (bt_dev->src_or_snk == BT_DEVICE_SRC) {
    a2dp_avdtpcodec.codecType = BTIF_AVDTP_CODEC_TYPE_SBC;
    a2dp_avdtpcodec.discoverable = 1;
    a2dp_avdtpcodec.elements = (U8 *)&a2dp_codec_elements;
    a2dp_avdtpcodec.elemLen = 4;
    btif_a2dp_register(bt_dev->a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream,
                       BTIF_A2DP_STREAM_TYPE_SOURCE, &a2dp_avdtpcodec, NULL, 0,
                       0, a2dp_callback);
  }
#endif

  return 0;
}

int a2dp_codec_sink_init(void) {
  int i;
  struct BT_DEVICE_T POSSIBLY_UNUSED *bt_dev = &app_bt_device;

#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
  if (bt_dev->src_or_snk != BT_DEVICE_SRC)
#endif
  {
    for (i = 0; i < BT_DEVICE_NUM; i++) {
      a2dp_codec_sbc_init(i);
#if defined(A2DP_AAC_ON)
      a2dp_codec_aac_init(i);
#endif
#if defined(A2DP_LDAC_ON)
      a2dp_codec_ldac_init(i);
#endif
#if defined(A2DP_LHDC_ON)
      a2dp_codec_lhdc_init(i);
#endif
#if defined(MASTER_USE_OPUS) || defined(ALL_USE_OPUS)
      a2dp_codec_opus_init(i);
#endif
#if defined(A2DP_SCALABLE_ON)
      a2dp_codec_scalable_init(i);
#endif
    }
  }

  return 0;
}

uint8_t a2dp_codec_confirm_stream_state(uint8_t index, uint8_t old_state,
                                        uint8_t new_state) {
  btif_a2dp_confirm_stream_state(app_bt_device.a2dp_stream[index]->a2dp_stream,
                                 old_state, new_state);
#if defined(A2DP_AAC_ON)
  btif_a2dp_confirm_stream_state(
      app_bt_device.a2dp_aac_stream[index]->a2dp_stream, old_state, new_state);
#endif
#if defined(A2DP_LHDC_ON)
  btif_a2dp_confirm_stream_state(
      app_bt_device.a2dp_lhdc_stream[index]->a2dp_stream, old_state, new_state);
#endif
#if defined(A2DP_SCALABLE_ON)
  btif_a2dp_confirm_stream_state(
      app_bt_device.a2dp_scalable_stream[index]->a2dp_stream, old_state,
      new_state);
#endif
#if defined(A2DP_LDAC_ON)
  btif_a2dp_confirm_stream_state(
      app_bt_device.a2dp_ldac_stream[index]->a2dp_stream, old_state, new_state);
#endif

  return 0;
}

static void a2dp_set_codec_info(btif_dev_it_e dev_num, const uint8_t *codec) {
  app_bt_device.codec_type[dev_num] = codec[0];
  app_bt_device.sample_bit[dev_num] = codec[1];
  app_bt_device.sample_rate[dev_num] = codec[2];
#if defined(A2DP_LHDC_ON)
  app_bt_device.a2dp_lhdc_llc[dev_num] = codec[3];
#endif
#if defined(A2DP_LDAC_ON)
  app_ibrt_restore_ldac_info(app_bt_device.sample_rate[dev_num]);
#endif
}

static void a2dp_get_codec_info(btif_dev_it_e dev_num, uint8_t *codec) {
  codec[0] = app_bt_device.codec_type[dev_num];
  codec[1] = app_bt_device.sample_bit[dev_num];
  codec[2] = app_bt_device.sample_rate[dev_num];
#if defined(A2DP_LHDC_ON)
  codec[3] = app_bt_device.a2dp_lhdc_llc[dev_num];
#endif
}

int a2dp_codec_init(void) {
  a2dp_codec_source_init();
  a2dp_codec_sink_init();
  btif_a2dp_get_codec_info_func(a2dp_get_codec_info);
  btif_a2dp_set_codec_info_func(a2dp_set_codec_info);
  return 0;
}

btif_avdtp_codec_t *app_a2dp_codec_get_avdtp_codec() {
  return (btif_avdtp_codec_t *)&a2dp_avdtpcodec;
}

#if defined(IBRT)
enum AUD_SAMPRATE_T bt_parse_sbc_sample_rate(uint8_t sbc_samp_rate);
uint32_t
app_a2dp_codec_parse_aac_sample_rate(const a2dp_callback_parms_t *info) {
  uint32_t ret_sample_rate = AUD_SAMPRATE_44100;
  btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *)info;

  if (p_info->p.configReq->codec.elements[1] &
      A2DP_AAC_OCTET1_SAMPLING_FREQUENCY_44100) {
    ret_sample_rate = AUD_SAMPRATE_44100;
  } else if (p_info->p.configReq->codec.elements[2] &
             A2DP_AAC_OCTET2_SAMPLING_FREQUENCY_48000) {
    ret_sample_rate = AUD_SAMPRATE_48000;
  }
  return ret_sample_rate;
}
uint32_t
app_a2dp_codec_parse_aac_lhdc_sample_rate(const a2dp_callback_parms_t *info) {
  btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *)info;

  switch (A2DP_LHDC_SR_DATA(p_info->p.configReq->codec.elements[6])) {
  case A2DP_LHDC_SR_96000:
    return AUD_SAMPRATE_96000;
  case A2DP_LHDC_SR_48000:
    return AUD_SAMPRATE_48000;
  case A2DP_LHDC_SR_44100:
    return AUD_SAMPRATE_44100;
  default:
    return AUD_SAMPRATE_44100;
  }
}
uint32_t app_a2dp_codec_get_sample_rate(const a2dp_callback_parms_t *info) {
  btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *)info;
  btif_avdtp_codec_type_t codetype =
      btif_a2dp_get_codec_type((const a2dp_callback_parms_t *)p_info);

  switch (codetype) {
  case BTIF_AVDTP_CODEC_TYPE_SBC:
    return bt_parse_sbc_sample_rate(p_info->p.configReq->codec.elements[0]);
  case BTIF_AVDTP_CODEC_TYPE_MPEG2_4_AAC:
    return app_a2dp_codec_parse_aac_sample_rate(
        (const a2dp_callback_parms_t *)p_info);
  case BTIF_AVDTP_CODEC_TYPE_LHDC:
    return app_a2dp_codec_parse_aac_lhdc_sample_rate(
        (const a2dp_callback_parms_t *)p_info);
  default:
    ASSERT(0, "btif_a2dp_get_sample_rate codetype error!!");
    return 0;
  }
}
uint8_t app_a2dp_codec_get_sample_bit(const a2dp_callback_parms_t *info) {
  btif_avdtp_codec_type_t codetype = btif_a2dp_get_codec_type(info);
  btif_a2dp_callback_parms_t *p_info = (btif_a2dp_callback_parms_t *)info;

  if ((codetype == BTIF_AVDTP_CODEC_TYPE_LHDC) &&
      A2DP_LHDC_FMT_DATA(p_info->p.configReq->codec.elements[6])) {
    return 24;
  } else {
    // AAC and SBC sample bit eq 16
    return 16;
  }
}
#endif
