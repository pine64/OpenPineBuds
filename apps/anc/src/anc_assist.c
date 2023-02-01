

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
#include "anc_assist.h"
#include "anc_assist_algo.h"
#include "anc_process.h"
#include "arm_math.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "hal_aud.h"
#include "hal_codec.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "speech_cfg.h"
#include "speech_memory.h"
#include "speech_ssat.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
#include "main_classify.h"
#endif

static void _close_mic_anc_assist();
static void _open_mic_anc_assist();
#define _SAMPLE_RATE (16000)

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
#define _FRAME_LEN (128)
#else
#define _FRAME_LEN (160)
#endif

#define _CHANNEL_NUM_MAX (3)
#define SAMPLE_BYTES (sizeof(ASSIST_PCM_T))
#define AF_STREAM_BUFF_SIZE (_FRAME_LEN * SAMPLE_BYTES * _CHANNEL_NUM_MAX * 2)
#define ANC_ADPT_STREAM_ID AUD_STREAM_ID_3

#define _FRAME_LEN_MAX (160)
#define _SAMPLE_BITS_MAX (32)

static uint8_t __attribute__((aligned(4))) af_stream_buff[AF_STREAM_BUFF_SIZE];
static ASSIST_PCM_T af_stream_mic1[_FRAME_LEN_MAX * (_SAMPLE_BITS_MAX / 8)];
static ASSIST_PCM_T af_stream_mic2[_FRAME_LEN_MAX * (_SAMPLE_BITS_MAX / 8)];
static ASSIST_PCM_T af_stream_mic3[_FRAME_LEN_MAX * (_SAMPLE_BITS_MAX / 8)];
int MIC_NUM = 0;
int MIC_MAP = 0;

#if defined(ANC_ASSIST_PILOT_ENABLED)
#define _PLAY_SAMPLE_RATE (8000)
#define _PLAY_FRAME_LEN (80)
#define AF_PLAY_STREAM_BUFF_SIZE (_PLAY_FRAME_LEN * SAMPLE_BYTES * 1 * 2)
static uint8_t __attribute__((aligned(4)))
af_play_stream_buff[AF_PLAY_STREAM_BUFF_SIZE];
#endif

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
ClassifyState *NoiseClassify_st = NULL;
#endif

#if defined(ANC_ASSIST_PILOT_ENABLED) || defined(ANC_ASSIST_HESS_ENABLED) ||   \
    defined(ANC_ASSIST_PNC_ENABLED) ||                                         \
    defined(ANC_ASSIST_DEHOWLING_ENABLED) ||                                   \
    defined(ANC_ASSIST_WNR_ENABLED) ||                                         \
    defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)

extern const struct_anc_cfg *anc_coef_list_48k[1];
void anc_assist_change_curve(int curve_id) {
  TRACE(2, "[%s] change anc curve %d", __func__, curve_id);
  anc_set_cfg(anc_coef_list_48k[0], ANC_FEEDFORWARD, ANC_GAIN_NO_DELAY);
  anc_set_cfg(anc_coef_list_48k[0], ANC_FEEDBACK, ANC_GAIN_NO_DELAY);
}
bool audio_engine_tt_is_on() { return 1; }

#define _tgt_ff_gain (512)
void anc_assist_set_anc_gain(float gain_ch_l, float gain_ch_r,
                             enum ANC_TYPE_T anc_type) {

  TRACE(2, "[%s] set anc gain %d", __func__, (int)(100 * gain_ch_l));
  uint32_t tgt_ff_gain_l, tgt_ff_gain_r;
  tgt_ff_gain_l = (uint32_t)(_tgt_ff_gain * gain_ch_l);
  tgt_ff_gain_r = (uint32_t)(_tgt_ff_gain * gain_ch_r);
  anc_set_gain(tgt_ff_gain_l, tgt_ff_gain_r, anc_type);
}
#endif

#if defined(ANC_ASSIST_PILOT_ENABLED)
static LeakageDetectionState *pilot_st = NULL;

#endif

#if defined(ANC_ASSIST_HESS_ENABLED) || defined(ANC_ASSIST_PNC_ENABLED) ||     \
    defined(ANC_ASSIST_DEHOWLING_ENABLED) || defined(ANC_ASSIST_WNR_ENABLED)
static ANCAssistMultiState *anc_assist_multi_st = NULL;
#endif

ANC_ASSIST_MODE_T g_anc_assist_mode = ANC_ASSIST_MODE_QTY;
void anc_assist_open(ANC_ASSIST_MODE_T mode) {
  g_anc_assist_mode = mode;

  // normal init
#if defined(ANC_ASSIST_PILOT_ENABLED)
  pilot_st = LeakageDetection_create(160, 0);
#endif

#if defined(ANC_ASSIST_HESS_ENABLED) || defined(ANC_ASSIST_PNC_ENABLED) ||     \
    defined(ANC_ASSIST_DEHOWLING_ENABLED) || defined(ANC_ASSIST_WNR_ENABLED)
  anc_assist_multi_st = ANCAssistMulti_create(_SAMPLE_RATE, _FRAME_LEN, 128);
#endif

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
  NoiseClassify_st = classify_create(_SAMPLE_RATE, _FRAME_LEN);
#endif

  // audio_dump_init(160,sizeof(short),3);

  if (mode == ANC_ASSIST_MODE_QTY) {
    return;
  } else {
    if (mode == ANC_ASSIST_STANDALONE || mode == ANC_ASSIST_MUSIC) {
      _open_mic_anc_assist();
    }
    if (mode == ANC_ASSIST_PHONE_8K) {
      // normal init 8k
    } else if (mode == ANC_ASSIST_PHONE_16K) {
      // normal init 16k
    }
  }
}

void anc_assist_close() {

#if defined(ANC_ASSIST_PILOT_ENABLED)
  LeakageDetection_destroy(pilot_st);
#endif
#if defined(ANC_ASSIST_HESS_ENABLED) || defined(ANC_ASSIST_PNC_ENABLED) ||     \
    defined(ANC_ASSIST_DEHOWLING_ENABLED) || defined(ANC_ASSIST_WNR_ENABLED)
  ANCAssistMulti_destroy(anc_assist_multi_st);
#endif

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
  classify_destroy(NoiseClassify_st);
#endif

  // ext_heap_deinit();

  if (g_anc_assist_mode == ANC_ASSIST_MODE_QTY) {
    return;
  } else {
    if (g_anc_assist_mode == ANC_ASSIST_STANDALONE ||
        g_anc_assist_mode == ANC_ASSIST_MUSIC) {
      _close_mic_anc_assist();
    }
    if (g_anc_assist_mode == ANC_ASSIST_PHONE_8K) {
      // normal init 8k
    } else if (g_anc_assist_mode == ANC_ASSIST_PHONE_16K) {
      // normal init 16k
    }
  }
}

extern ASSIST_PCM_T ref_buf_data[80];
void anc_assist_process(uint8_t *buf, int len) {

  int32_t frame_len = len / SAMPLE_BYTES / MIC_NUM;
  ASSERT(frame_len == _FRAME_LEN, "[%s] frame len(%d) is invalid.", __func__,
         frame_len);
  ASSIST_PCM_T *pcm_buf = (ASSIST_PCM_T *)buf;

  ASSIST_PCM_T *mic1 = (ASSIST_PCM_T *)af_stream_mic1;
  ASSIST_PCM_T *mic2 = (ASSIST_PCM_T *)af_stream_mic2;
  ASSIST_PCM_T *mic3 = (ASSIST_PCM_T *)af_stream_mic3;

  for (int32_t i = 0; i < frame_len; i++) {
    mic1[i] = pcm_buf[MIC_NUM * i + 0];
    mic2[i] = pcm_buf[MIC_NUM * i + 1];
    mic3[i] = pcm_buf[MIC_NUM * i + 2];
  }
  // audio_dump_clear_up();
  // audio_dump_add_channel_data(0,mic1,160);
  // audio_dump_add_channel_data(1,mic2,160);
  // audio_dump_add_channel_data(2,mic3,160);
  // audio_dump_run();
  // TRACE(2,"in callback");
#if defined(ANC_ASSIST_PILOT_ENABLED)
  LeakageDetection_process(pilot_st, AF_ANC_OFF, mic3, ref_buf_data, frame_len);
#endif

#if defined(ANC_ASSIST_HESS_ENABLED) || defined(ANC_ASSIST_PNC_ENABLED) ||     \
    defined(ANC_ASSIST_DEHOWLING_ENABLED) || defined(ANC_ASSIST_WNR_ENABLED)
  ANCAssistMulti_process(anc_assist_multi_st, mic1, mic2, mic3, frame_len);
#endif

#if defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
  static int last_classify_res = -1;
  classify_process(NoiseClassify_st, mic1, last_classify_res);
#endif

  if (g_anc_assist_mode == ANC_ASSIST_PHONE_16K) {
    // down sample
  }

  // process fft

  // wnr

  // pnc

  // hess

  // pilot adpt
}

static uint32_t anc_assist_callback(uint8_t *buf, uint32_t len) {
#ifdef TEST_MIPS
  start_ticks = hal_fast_sys_timer_get();
#endif
  anc_assist_process(buf, len);

#ifdef TEST_MIPS
  end_ticks = hal_fast_sys_timer_get();
  used_mips = (end_ticks - start_ticks) * 1000 / (start_ticks - pre_ticks);
  TRACE(2, "[%s] Usage: %d in a thousand (MIPS).", __func__, used_mips);
  // wnr_ticks = start_ticks;
  // TRACE(2,"[%s] WNR frame takes %d ms.", __func__,
  // FAST_TICKS_TO_MS((start_ticks - pre_ticks)*100));
  pre_ticks = start_ticks;
#endif
  return 0;
}

#if defined(ANC_ASSIST_PILOT_ENABLED)
static uint32_t anc_assist_playback_callback(uint8_t *buf, uint32_t len) {
  get_pilot_data(buf, len);
  // TRACE(2,"playing data %d",len);
  return 0;
}
#endif

static void _open_mic_anc_assist(void) {

  int anc_assist_mic_num = 0;

#if defined(ANC_ASSIST_PILOT_ENABLED)
  anc_assist_mic_num = anc_assist_mic_num | ANC_ASSIST_FB_MIC;
#endif

#if defined(ANC_ASSIST_HESS_ENABLED) ||                                        \
    defined(ANC_ASSIST_NOISE_ADAPTIVE_ENABLED)
  anc_assist_mic_num = anc_assist_mic_num | ANC_ASSIST_FF1_MIC;
#endif

#if defined(ANC_ASSIST_PNC_ENABLED)
  anc_assist_mic_num =
      anc_assist_mic_num | ANC_ASSIST_FF1_MIC | ANC_ASSIST_FB_MIC;
#endif

#if defined(ANC_ASSIST_DEHOWLING_ENABLED)
  anc_assist_mic_num =
      anc_assist_mic_num | ANC_ASSIST_FF1_MIC | ANC_ASSIST_FB_MIC;
#endif

#if defined(ANC_ASSIST_WNR_ENABLED)
  anc_assist_mic_num =
      anc_assist_mic_num | ANC_ASSIST_FF1_MIC | ANC_ASSIST_FF2_MIC;
#endif

  switch (anc_assist_mic_num) {
  case (0): {
    TRACE(2, "[%s] no mic is used", __func__);
    return;
  } break;
  case (1): {
    TRACE(2, "[%s] use fb mic only", __func__);
    MIC_NUM = 3;
    MIC_MAP = AUD_INPUT_PATH_AF_ANC;

  } break;
  case (4): {
    TRACE(2, "[%s] use ff mic only", __func__);
    MIC_NUM = 3;
    MIC_MAP = AUD_INPUT_PATH_ANC_WNR;

  } break;
  case (5): {
    TRACE(2, "[%s] use ff mic and fb mic", __func__);
    MIC_NUM = 3;
    MIC_MAP = AUD_INPUT_PATH_ANC_WNR;

  } break;
  case (6): {
    TRACE(2, "[%s] use ff1 mic and ff2 mic", __func__);
    MIC_NUM = 2;
    MIC_MAP = AUD_INPUT_PATH_AF_ANC;

  } break;
  case (7): {
    TRACE(2, "[%s] use ff1 mic and ff2 mic and fb mic", __func__);
    MIC_NUM = 2;
    MIC_MAP = AUD_INPUT_PATH_AF_ANC;

  } break;

  default: {
    TRACE(2, "[%s] invalid mic order is used", __func__);
  } break;
  }

  MIC_NUM = 3;
  MIC_MAP = AUD_INPUT_PATH_AF_ANC;
  struct AF_STREAM_CONFIG_T stream_cfg;
  TRACE(1, "[%s] ...", __func__);

  memset(&stream_cfg, 0, sizeof(stream_cfg));
  stream_cfg.channel_num = (enum AUD_CHANNEL_NUM_T)MIC_NUM;
  stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)_SAMPLE_RATE;
  stream_cfg.bits = (enum AUD_BITS_T)_SAMPLE_BITS;
  stream_cfg.vol = 12;
  stream_cfg.chan_sep_buf = false;
  stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
  stream_cfg.io_path = (enum AUD_IO_PATH_T)MIC_MAP;
  stream_cfg.handler = anc_assist_callback;
  stream_cfg.data_size = _FRAME_LEN * SAMPLE_BYTES * 2 * MIC_NUM;
  stream_cfg.data_ptr = af_stream_buff;
  ASSERT(stream_cfg.channel_num == MIC_NUM,
         "[%s] channel number(%d) is invalid.", __func__,
         stream_cfg.channel_num);
  TRACE(2, "[%s] sample_rate:%d, data_size:%d", __func__,
        stream_cfg.sample_rate, stream_cfg.data_size);
  TRACE(2, "[%s] af_stream_buff = %p", __func__, af_stream_buff);

  af_stream_open(ANC_ADPT_STREAM_ID, AUD_STREAM_CAPTURE, &stream_cfg);
  af_stream_start(ANC_ADPT_STREAM_ID, AUD_STREAM_CAPTURE);

#if defined(ANC_ASSIST_PILOT_ENABLED)
  // struct AF_STREAM_CONFIG_T stream_cfg;
  TRACE(1, "[%s] set play ...", __func__);

  memset(&stream_cfg, 0, sizeof(stream_cfg));

  stream_cfg.bits = (enum AUD_BITS_T)_SAMPLE_BITS;
  stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
  stream_cfg.sample_rate = (enum AUD_SAMPRATE_T)_PLAY_SAMPLE_RATE;

  stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
  stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
  stream_cfg.vol = 15;

  stream_cfg.handler = anc_assist_playback_callback;
  stream_cfg.data_ptr = af_play_stream_buff;
  stream_cfg.data_size = sizeof(af_play_stream_buff);

  af_stream_open(ANC_ADPT_STREAM_ID, AUD_STREAM_PLAYBACK, &stream_cfg);
  af_stream_start(ANC_ADPT_STREAM_ID, AUD_STREAM_PLAYBACK);
#endif
}

static void _close_mic_anc_assist() {
  TRACE(1, "[%s] ...", __func__);

  af_stream_stop(ANC_ADPT_STREAM_ID, AUD_STREAM_CAPTURE);
  af_stream_close(ANC_ADPT_STREAM_ID, AUD_STREAM_CAPTURE);
  if (g_anc_assist_mode == ANC_ASSIST_STANDALONE ||
      g_anc_assist_mode == ANC_ASSIST_MUSIC) {
    // close capture
  }
  // destroy
}
