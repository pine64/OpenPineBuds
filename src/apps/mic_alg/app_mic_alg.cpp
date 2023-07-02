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
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "resources.h"
//#include "app_factory.h"
#include "string.h"

// for audio
#include "app_audio.h"
#include "app_utils.h"
#include "audioflinger.h"
#include "hal_timer.h"

#include "app_mic_alg.h"
#include "tgt_hardware.h"

#ifdef NOTCH_FILTER
#include "autowah.h"
#endif

#ifdef WL_NSX
#include "nsx_main.h"
#endif
#include "app_overlay.h"
#include "math.h"

#include "apps.h"

#ifdef WEBRTC_AGC
#include "agc_main.h"
#endif

#ifdef WL_NSX
#define WEBRTC_NSX_BUFF_SIZE (14000)
#endif

#ifdef WL_VAD
#include "vad_user.h"
#endif

#ifdef WL_DEBUG_MODE
#include "nvrecord_env.h"
#endif

#ifdef REMOTE_UART
#include "app_remoter_uart.h"
#endif

#if defined(WL_AEC)
#include "wl_sco_process.h"
#endif
#include "speech_memory.h"

#ifdef I2C_SENSOR
#include "app_i2c_sensor.h"
#endif

static inline float clampf(float v, float min, float max) {
  return v < min ? min : (v > max ? max : v);
}

#ifdef WL_NSX_5MS
#define BT_AUDIO_FACTORMODE_BUFF_SIZE (160 * 2)
#else
#define BT_AUDIO_FACTORMODE_BUFF_SIZE (6 * 320 * 16)
#endif

#define NSX_FRAME_SIZE 160

static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;

#if defined(WL_AEC)
static short POSSIBLY_UNUSED aec_out[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];
static short POSSIBLY_UNUSED far_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];
#endif

static short POSSIBLY_UNUSED out_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];
static short POSSIBLY_UNUSED tmp_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];

// static short revert_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];
// static short audio_uart_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE>>2];

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2

static short POSSIBLY_UNUSED one_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];
static short POSSIBLY_UNUSED two_buff[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];

static short POSSIBLY_UNUSED left_out[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];
static short POSSIBLY_UNUSED right_out[BT_AUDIO_FACTORMODE_BUFF_SIZE >> 2];

static void POSSIBLY_UNUSED aaudio_div_stero_to_rmono(int16_t *dst_buf,
                                                      int16_t *src_buf,
                                                      uint32_t src_len) {
  // Copy from tail so that it works even if dst_buf == src_buf
  for (uint32_t i = 0; i < src_len >> 1; i++) {
    dst_buf[i] = src_buf[i * 2 + 1];
  }
}
static void POSSIBLY_UNUSED aaudio_div_stero_to_lmono(int16_t *dst_buf,
                                                      int16_t *src_buf,
                                                      uint32_t src_len) {
  // Copy from tail so that it works even if dst_buf == src_buf
  for (uint32_t i = 0; i < src_len >> 1; i++) {
    dst_buf[i] = src_buf[i * 2 + 0];
  }
}

static void POSSIBLY_UNUSED audio_mono2stereo_16bits(int16_t *dst_buf,
                                                     int16_t *left_buf,
                                                     int16_t *right_buf,
                                                     uint32_t src_len) {
  uint32_t i = 0;
  for (i = 0; i < src_len; ++i) {
    dst_buf[i * 2 + 0] = left_buf[i];
    dst_buf[i * 2 + 1] = right_buf[i];
  }
}

#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3

#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 4

#endif

extern uint8_t supress_step;
extern uint32_t transfer_factor;
extern uint32_t diff_energy;
extern uint32_t level_shift;

static inline double convert_multiple_to_db(uint32_t multiple) {
  return 20 * log10(multiple);
}
#define DUMP_FRAME_LEN 0x3C0
static short POSSIBLY_UNUSED revert_buff[2 + 1 * DUMP_FRAME_LEN];
int32_t tx_pcmbuf32[960];

extern int app_reset(void);

extern void app_bt_volumeup();
extern void app_bt_volumedown();

void vol_state_process(uint32_t db_val) {
  TRACE(2, "db value is:%d volume_is:%d ", db_val,
        app_bt_stream_local_volume_get());

  if ((db_val < 52) && (app_bt_stream_local_volume_get() > 10)) {
    app_bt_volumedown();
  } else if ((db_val > 60) && (app_bt_stream_local_volume_get() < 13)) {
    app_bt_volumeup();
  } else if ((db_val > 72) && (app_bt_stream_local_volume_get() < 15)) {
    app_bt_volumeup();
  }
}

static uint32_t app_mic_alg_data_come(uint8_t *buf, uint32_t len) {
  uint32_t pcm_len = len >> 1;

  short POSSIBLY_UNUSED *tx_pcmbuf16 = (short *)buf;

  // DUMP16("%d, ",tx_pcmbuf16,30);
  // memcpy(tmp_buff,pcm_buff,len);

  int32_t stime = 0;
  static int32_t nsx_cnt = 0;
  static int32_t dump_cnt = 0;

  nsx_cnt++;
  dump_cnt++;

  DUMP16("%d,", tx_pcmbuf16, 30);
  if (false == (nsx_cnt & 0x3F)) {
    stime = hal_sys_timer_get();
    // TRACE("aecm  echo time: lens:%d  g_time_cnt:%d ",len, g_time_cnt);
  }

#ifdef WL_DET
  if (nsx_cnt > 100) {
    static double last_sum = 0, last_avg = 0;

    uint32_t sum_ss = 0;
    // short db_val = 0;
    double db_sum = 0;
    for (uint32_t i_cnt = 0; i_cnt < pcm_len; i_cnt++) {
      sum_ss += ABS(tx_pcmbuf16[i_cnt]);
    }

    sum_ss = 1 * sum_ss / pcm_len;

    db_sum = convert_multiple_to_db(sum_ss);
    // db_val = (short)(100*db_sum);
    last_sum += db_sum;
    last_avg = last_sum / nsx_cnt;

    db_sum = db_sum * (double)0.02 + last_avg * (double)0.98;

    // TRACE(2,"db value is:%d sum_ss:%d ",(uint32_t)db_sum,sum_ss);
    // TRACE(2,"db value is:%d ",(uint32_t)db_sum);
    vol_state_process((uint32_t)db_sum);
  }

#endif

  if (false == (nsx_cnt & 0x3F)) {
    // TRACE("drc 48 mic_alg 16k nsx 3 agc 15 closed speed  time:%d ms and
    // pcm_lens:%d freq:%d ", TICKS_TO_MS(hal_sys_timer_get() - stime),
    // pcm_len,hal_sysfreq_get()); TRACE("notch 500 mic_alg 16k nsx 3 agc 15
    // closed speed  time:%d ms and pcm_lens:%d freq:%d ",
    // TICKS_TO_MS(hal_sys_timer_get() - stime), pcm_len,hal_sysfreq_get());
    TRACE(2, "denoise det  speed  time:%d ms and pcm_lens:%d freq:%d ",
          TICKS_TO_MS(hal_sys_timer_get() - stime), pcm_len, hal_sysfreq_get());
  }

  if (a2dp_cache_status == APP_AUDIO_CACHE_QTY) {
    a2dp_cache_status = APP_AUDIO_CACHE_OK;
  }
  return len;
}

// static uint32_t app_mic_uart_playback_data(uint8_t *buf, uint32_t len)
// {
//     if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){
// #if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2
//         #ifdef WL_AEC
//         app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
//         app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf,
//         app_audioloop_play_cache, len/2/2); #else
//         app_audio_pcmbuff_get((uint8_t *)buf, len);
//         #endif
// #else
//         app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
//         app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf,
//         app_audioloop_play_cache, len/2/2);
// #endif
//     }
//     return len;
// }

static uint8_t buff_capture[BT_AUDIO_FACTORMODE_BUFF_SIZE];

int app_mic_alg_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq) {
  struct AF_STREAM_CONFIG_T stream_cfg;
  static bool isRun = false;

  TRACE(2, "app_mic_alg  work:%d op:%d freq:%d", isRun, on, freq);

  if (isRun == on)
    return 0;

  if (on) {

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

    a2dp_cache_status = APP_AUDIO_CACHE_QTY;

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    stream_cfg.bits = AUD_BITS_16;

    stream_cfg.channel_num = AUD_CHANNEL_NUM_1;

    stream_cfg.sample_rate = AUD_SAMPRATE_48000;

    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;

    stream_cfg.vol = CODEC_SADC_VOL;
    stream_cfg.io_path = AUD_INPUT_PATH_ASRMIC;

    stream_cfg.handler = app_mic_alg_data_come;

    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
    stream_cfg.data_size =
        BT_AUDIO_FACTORMODE_BUFF_SIZE * stream_cfg.channel_num;
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

    // stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
    // stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    // stream_cfg.handler = app_mic_uart_playback_data;

    // stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
    // stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
    // af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

    // af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    TRACE(2, "app_mic_uart ss loopback on");

  } else {
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    // af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    // af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    TRACE(2, "app_mic_16k loopback off");

    // app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  }

  isRun = on;
  return 0;
}
