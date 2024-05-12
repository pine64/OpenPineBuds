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
#include "app_utils.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"
// #include "local_wav.h"

#define CHANNEL_NUM (2)

#define CHAR_BYTES (1)
#define SHORT_BYTES (2)
#define INT_BYTES (4)

#define SAMPLE_BITS (16)
#define SAMPLE_BYTES (SAMPLE_BITS / 8)

#define TX_SAMPLE_RATE (16000)
#define RX_SAMPLE_RATE (16000)

#define TX_FRAME_LEN (256)
#define RX_FRAME_LEN (256)

#define TX_BUF_SIZE (TX_FRAME_LEN * CHANNEL_NUM * SAMPLE_BYTES * 2)
#define RX_BUF_SIZE (RX_FRAME_LEN * CHANNEL_NUM * SAMPLE_BYTES * 2)

#if SAMPLE_BYTES == SHORT_BYTES
typedef short VOICE_PCM_T;
#elif SAMPLE_BYTES == INT_BYTES
typedef int VOICE_PCM_T;
#else
#error "Invalid SAMPLE_BYTES!!!"
#endif

static uint8_t POSSIBLY_UNUSED codec_capture_buf[TX_BUF_SIZE];
static uint8_t POSSIBLY_UNUSED codec_playback_buf[RX_BUF_SIZE];

static uint32_t POSSIBLY_UNUSED codec_capture_cnt = 0;
static uint32_t POSSIBLY_UNUSED codec_playback_cnt = 0;

#define CODEC_STREAM_ID AUD_STREAM_ID_0

static uint32_t codec_capture_callback(uint8_t *buf, uint32_t len) {
  int POSSIBLY_UNUSED pcm_len = len / sizeof(VOICE_PCM_T) / CHANNEL_NUM;
  VOICE_PCM_T POSSIBLY_UNUSED *pcm_buf[CHANNEL_NUM];
  int interval_len = len * 2 / CHANNEL_NUM;

  for (int i = 0; i < CHANNEL_NUM; i++) {
    pcm_buf[i] = (VOICE_PCM_T *)(buf + i * interval_len);
  }

  // TRACE(2,"[%s] cnt = %d", __func__, codec_capture_cnt++);

  audio_dump_add_channel_data(0, pcm_buf[0], pcm_len);
  audio_dump_add_channel_data(1, pcm_buf[0], pcm_len);
  audio_dump_run();

  return len;
}

static uint32_t codec_playback_callback(uint8_t *buf, uint32_t len) {
  int POSSIBLY_UNUSED pcm_len = len / sizeof(VOICE_PCM_T) / CHANNEL_NUM;
  VOICE_PCM_T POSSIBLY_UNUSED *pcm_buf[CHANNEL_NUM];
  int interval_len = len * 2 / CHANNEL_NUM;

  for (int i = 0; i < CHANNEL_NUM; i++) {
    pcm_buf[i] = (VOICE_PCM_T *)(buf + i * interval_len);
  }

  // TRACE(2,"[%s] cnt = %d", __func__, codec_playback_cnt++);

  return len;
}

static int voice_start(bool on) {
  int ret = 0;
  static bool isRun = false;
  enum APP_SYSFREQ_FREQ_T freq = APP_SYSFREQ_208M;
  struct AF_STREAM_CONFIG_T stream_cfg;

  if (isRun == on) {
    return 0;
  }

  if (on) {
    TRACE(1, "[%s]] ON", __func__);

    af_set_priority(AF_USER_TEST, osPriorityHigh);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);
    TRACE(2, "[%s] sys freq calc : %d\n", __func__,
          hal_sys_timer_calc_cpu_freq(5, 0));

    // Initialize Cqueue
    codec_capture_cnt = 0;
    codec_playback_cnt = 0;

    memset(&stream_cfg, 0, sizeof(stream_cfg));
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)CHANNEL_NUM;
    stream_cfg.data_size = TX_BUF_SIZE;
    stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)TX_SAMPLE_RATE;
    stream_cfg.bits = (enum AUD_BITS_T)SAMPLE_BITS;
    stream_cfg.vol = 12;
    stream_cfg.chan_sep_buf = true;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
    stream_cfg.handler = codec_capture_callback;
    stream_cfg.data_ptr = codec_capture_buf;

    TRACE(3, "[%s] codec capture sample_rate: %d, data_size: %d", __func__,
          stream_cfg.sample_rate, stream_cfg.data_size);
    af_stream_open(CODEC_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
    ASSERT(ret == 0, "codec capture failed: %d", ret);

    memset(&stream_cfg, 0, sizeof(stream_cfg));
    stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)CHANNEL_NUM;
    stream_cfg.data_size = RX_BUF_SIZE;
    stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)RX_SAMPLE_RATE;
    stream_cfg.bits = (enum AUD_BITS_T)SAMPLE_BITS;
    stream_cfg.vol = 12;
    stream_cfg.chan_sep_buf = true;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
    stream_cfg.handler = codec_playback_callback;
    stream_cfg.data_ptr = codec_playback_buf;

    TRACE(3, "[%s] codec playback sample_rate: %d, data_size: %d", __func__,
          stream_cfg.sample_rate, stream_cfg.data_size);
    af_stream_open(CODEC_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
    ASSERT(ret == 0, "codec playback failed: %d", ret);

    audio_dump_init(TX_FRAME_LEN, sizeof(VOICE_PCM_T), 1);

    // Start
    af_stream_start(CODEC_STREAM_ID, AUD_STREAM_CAPTURE);
    af_stream_start(CODEC_STREAM_ID, AUD_STREAM_PLAYBACK);
  } else {
    // Close stream
    af_stream_stop(CODEC_STREAM_ID, AUD_STREAM_PLAYBACK);
    af_stream_stop(CODEC_STREAM_ID, AUD_STREAM_CAPTURE);

    audio_dump_deinit();

    af_stream_close(CODEC_STREAM_ID, AUD_STREAM_PLAYBACK);
    af_stream_close(CODEC_STREAM_ID, AUD_STREAM_CAPTURE);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);

    af_set_priority(AF_USER_TEST, osPriorityAboveNormal);

    TRACE(1, "[%s] OFF", __func__);
  }

  isRun = on;
  return 0;
}

static bool voice_test_status = true;
void voice_test(void) {
  TRACE(2, "[%s] status = %d", __func__, voice_test_status);

  voice_start(voice_test_status);
  voice_test_status = !voice_test_status;
}