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
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include <assert.h>
#include <string.h>

// for audio
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "audio_dump.h"
#include "audioflinger.h"
#include "speech_memory.h"
#include "speech_ssat.h"

#include "noise_tracker.h"
#include "noise_tracker_callback.h"

/**< NT Engine hooks                    */
#define NT_ENGINE_INIT(callback, ch, threshold)                                \
  noise_tracker_init(callback, ch, threshold)
#define NT_ENGINE_FEED(frame, n_samples) noise_tracker_process(frame, n_samples)

/**< NT configuration settings          */
#define NT_MIC_BITS_PER_SAMPLE (AUD_BITS_16)
#define NT_MIC_SAMPLE_RATE (16000) // (AUD_SAMPRATE_8000)
#define NT_MIC_VOLUME (16)

/**< NT Engine configuration settings   */
#define NT_ENGINE_SAMPLES_PER_FRAME (240)
#define NT_ENGINE_SAMPLE_RATE (16000)
#define NT_ENGINE_SYSFREQ (APP_SYSFREQ_26M) // (APP_SYSFREQ_26M)

/* Utility functions                     */
#define NT_TRACE(s, ...) TRACE(1, "%s: " s, __FUNCTION__, ##__VA_ARGS__);
#define ALIGN4 __attribute__((aligned(4)))

/* Calcluate the audio-capture frame length, FRAME_LEN, based of decimation rate
 * (1 or 2): */
#define FRAME_LEN                                                              \
  (NT_ENGINE_SAMPLES_PER_FRAME * (NT_MIC_SAMPLE_RATE / NT_ENGINE_SAMPLE_RATE))
#define CAPTURE_CHANNEL_NUM (ANC_NOISE_TRACKER_CHANNEL_NUM)
#define CAPTURE_BUF_SIZE (FRAME_LEN * CAPTURE_CHANNEL_NUM * 2 * 2)
static uint8_t codec_capture_buf[CAPTURE_BUF_SIZE] ALIGN4;

STATIC_ASSERT(FRAME_LEN == NT_ENGINE_SAMPLES_PER_FRAME,
              "NT_ENGINE Config error");

/* Local state */
static bool nt_demo_is_streaming = false;
extern uint8_t bt_sco_mode;

/**
 * @breif Handle one-time initialization, like setting up the memory pool
 */
static int nt_demo_init(bool, const void *) {
  static bool done;

  NT_TRACE(0, "Initialize kws_demo");
  if (done)
    return 0;

  done = true;

  return 0;
}

/**
 * @brief DC block IIR filter
 *
 * @param inout     Pointer of the PCM data (modify inplace).
 * @param in_len    Length of the PCM data in the buffer in samples.
 *
 */
static void filter_iir_dc_block(short *inout, int in_len, int stride) {
  static int z = 0;
  int tmp;

  for (int i = 0; i < in_len; i += stride) {
    z = (15 * z + inout[i]) >> 4;
    tmp = inout[i] - z;
    inout[i] = speech_ssat_int16(tmp);
  }
}

/**
 * @brief Process the collected PCM data from MIC.
 *        Resample audio stream to 8KHz and pass audio to kws lib.
 *
 * @param buf       Pointer of the PCM data buffer to access.
 * @param length    Length of the PCM data in the buffer in bytes.
 *
 * @return uint32_t 0 means no error happened
 */
static uint32_t nt_demo_stream_handler(uint8_t *buf, uint32_t length) {
  ASSERT(length == FRAME_LEN * CAPTURE_CHANNEL_NUM * sizeof(int16_t),
         "stream length not matched");

  short *pcm_buf = (short *)buf;
  uint32_t pcm_len = length / 2;

  filter_iir_dc_block(pcm_buf, pcm_len, ANC_NOISE_TRACKER_CHANNEL_NUM);
  NT_ENGINE_FEED(pcm_buf, pcm_len);

  return 0;
}

/**
 * @brief Setup audio streaming from MIC
 *
 * @param do_stream   start / stop streaming
 *
 * @return int 0 means no error happened
 */
static int nt_demo_stream_start(bool do_stream, const void *) {
  struct AF_STREAM_CONFIG_T stream_cfg;

  NT_TRACE(3, "Is running:%d enable:%d, bt_sco_mode:%d", nt_demo_is_streaming,
           do_stream, bt_sco_mode);

  if (bt_sco_mode)
    return 0;

  if (nt_demo_is_streaming == do_stream)
    return 0;
  nt_demo_is_streaming = do_stream;

  if (do_stream) {
    // Request sufficient system clock
    app_sysfreq_req(APP_SYSFREQ_USER_APP_NT, NT_ENGINE_SYSFREQ);
    NT_TRACE(1, "sys freq calc: %d Hz", hal_sys_timer_calc_cpu_freq(5, 0));

    nt_demo_init(true, NULL);

    // Initialize the NT ENGINE and install word-callback
    NT_ENGINE_INIT(nt_demo_words_cb, ANC_NOISE_TRACKER_CHANNEL_NUM, -20);

    memset(&stream_cfg, 0, sizeof(stream_cfg));
    stream_cfg.sample_rate = (AUD_SAMPRATE_T)NT_MIC_SAMPLE_RATE;
    stream_cfg.bits = NT_MIC_BITS_PER_SAMPLE;
    stream_cfg.vol = NT_MIC_VOLUME;
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_INPUT_PATH_NTMIC;
    stream_cfg.channel_num =
        (enum AUD_CHANNEL_NUM_T)ANC_NOISE_TRACKER_CHANNEL_NUM;
    stream_cfg.handler = nt_demo_stream_handler;
    stream_cfg.data_ptr = codec_capture_buf;
    stream_cfg.data_size = CAPTURE_BUF_SIZE;

    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

    NT_TRACE(0, "audio capture ON");

  } else {
    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

    app_sysfreq_req(APP_SYSFREQ_USER_APP_NT, APP_SYSFREQ_32K);
    TRACE(1, "sys freq calc:(32K) %d Hz", hal_sys_timer_calc_cpu_freq(5, 0));

    NT_TRACE(0, "audio capture OFF");
  }

  return 0;
}

#include "app_thirdparty.h"

#define TP_EVENT(event, handler)                                               \
  {THIRDPARTY_FUNC_NO3, THIRDPARTY_ID_DEMO, THIRDPARTY_##event},               \
      (APP_THIRDPARTY_HANDLE_CB_T)handler

THIRDPARTY_HANDLER_TAB(NOISE_TRACKER_LIB_NAME){
    //	{TP_EVENT(INIT,        nt_demo_init),           true,   NULL},
    {TP_EVENT(START, nt_demo_stream_start), true, NULL},
    {TP_EVENT(STOP, nt_demo_stream_start), false, NULL},
};

THIRDPARTY_HANDLER_TAB_SIZE(NOISE_TRACKER_LIB_NAME)
