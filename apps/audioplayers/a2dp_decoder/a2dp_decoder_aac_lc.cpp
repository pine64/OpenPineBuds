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
// Standard C Included Files
#include "a2dp_decoder_internal.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "heap_api.h"
#include "plat_types.h"
#include <string.h>

#if defined(A2DP_AAC_ON)
#include "aacdecoder_lib.h"
#include "aacenc_lib.h"
#include "heap_api.h"

#ifndef AAC_MTU_LIMITER
#define AAC_MTU_LIMITER (32) /*must <= 23*/
#endif
#define AAC_OUTPUT_FRAME_SAMPLES (1024)
#define DECODE_AAC_PCM_FRAME_LENGTH (2048)

#define AAC_READBUF_SIZE (900)

#define AAC_MEMPOOL_SIZE (40596)

typedef struct {
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint8_t *aac_buffer;
  uint32_t aac_buffer_len;
} a2dp_audio_aac_decoder_frame_t;

int a2dp_audio_aac_lc_reorder_init(void);
int a2dp_audio_aac_lc_reorder_deinit(void);

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;

static HANDLE_AACDECODER aacDec_handle = NULL;
static uint8_t *aac_mempoll = NULL;
heap_handle_t aac_memhandle = NULL;

static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_aac_lastframe_info;

static uint16_t aac_mtu_limiter = AAC_MTU_LIMITER;

static a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_reorder_p = NULL;
static a2dp_audio_aac_decoder_frame_t aac_decoder_last_valid_frame = {
    0,
};
static bool aac_decoder_last_valid_frame_ready = false;

static void *a2dp_audio_aac_lc_frame_malloc(uint32_t packet_len) {
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
  uint8_t *aac_buffer = NULL;

  aac_buffer = (uint8_t *)a2dp_audio_heap_malloc(AAC_READBUF_SIZE);
  aac_decoder_frame_p =
      (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_heap_malloc(
          sizeof(a2dp_audio_aac_decoder_frame_t));
  aac_decoder_frame_p->aac_buffer = aac_buffer;
  aac_decoder_frame_p->aac_buffer_len = packet_len;
  return (void *)aac_decoder_frame_p;
}

static void a2dp_audio_aac_lc_free(void *packet) {
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p =
      (a2dp_audio_aac_decoder_frame_t *)packet;
  a2dp_audio_heap_free(aac_decoder_frame_p->aac_buffer);
  a2dp_audio_heap_free(aac_decoder_frame_p);
}

static void a2dp_audio_aac_lc_decoder_init(void) {
  if (aacDec_handle == NULL) {
    TRANSPORT_TYPE transportFmt = TT_MP4_LATM_MCP1;

    aacDec_handle = aacDecoder_Open(transportFmt, 1 /* nrOfLayers */);
    ASSERT_A2DP_DECODER(aacDec_handle, "aacDecoder_Open failed");

    aacDecoder_SetParam(aacDec_handle, AAC_PCM_LIMITER_ENABLE, 0);
    aacDecoder_SetParam(aacDec_handle, AAC_DRC_ATTENUATION_FACTOR, 0);
    aacDecoder_SetParam(aacDec_handle, AAC_DRC_BOOST_FACTOR, 0);
  }
}

static void a2dp_audio_aac_lc_decoder_deinit(void) {
  if (aacDec_handle) {
    aacDecoder_Close(aacDec_handle);
    aacDec_handle = NULL;
  }
}

static void a2dp_audio_aac_lc_decoder_reinit(void) {
  if (aacDec_handle) {
    a2dp_audio_aac_lc_decoder_deinit();
  }
  a2dp_audio_aac_lc_decoder_init();
}

#ifdef A2DP_CP_ACCEL
struct A2DP_CP_AAC_LC_IN_FRM_INFO_T {
  uint16_t sequenceNumber;
  uint32_t timestamp;
};

struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T {
  struct A2DP_CP_AAC_LC_IN_FRM_INFO_T in_info;
  uint16_t frame_samples;
  uint16_t decoded_frames;
  uint16_t frame_idx;
  uint16_t pcm_len;
};

static bool cp_codec_reset;

extern "C" uint32_t get_in_cp_frame_cnt(void);
extern "C" unsigned int set_cp_reset_flag(uint8_t evt);
extern uint32_t app_bt_stream_get_dma_buffer_samples(void);

int a2dp_cp_aac_lc_cp_decode(void);

static int TEXT_AAC_LOC a2dp_cp_aac_lc_after_cache_underflow(void) {
#ifdef A2DP_CP_ACCEL
  cp_codec_reset = true;
#endif
  return 0;
}

static int a2dp_cp_aac_lc_mcu_decode(uint8_t *buffer, uint32_t buffer_bytes) {
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
  list_node_t *node = NULL;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  int ret, dec_ret;
  struct A2DP_CP_AAC_LC_IN_FRM_INFO_T in_info;
  struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *p_out_info;
  uint8_t *out;
  uint32_t out_len;
  uint32_t out_frame_len;
  uint32_t check_sum = 0;

  uint32_t cp_buffer_frames_max = 0;
  cp_buffer_frames_max = app_bt_stream_get_dma_buffer_samples() / 2;
  if (cp_buffer_frames_max % (a2dp_audio_aac_lastframe_info.frame_samples)) {
    cp_buffer_frames_max =
        cp_buffer_frames_max / (a2dp_audio_aac_lastframe_info.frame_samples) +
        1;
  } else {
    cp_buffer_frames_max =
        cp_buffer_frames_max / (a2dp_audio_aac_lastframe_info.frame_samples);
  }

  out_frame_len = sizeof(*p_out_info) + buffer_bytes;

  ret = a2dp_cp_decoder_init(out_frame_len, cp_buffer_frames_max * 2);
  if (ret) {
    TRACE_A2DP_DECODER_W("[MCU][AAC] cp_decoder_init failed: ret=%d", ret);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }
  while ((node = a2dp_audio_list_begin(list)) != NULL) {
    aac_decoder_frame_p =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);

    in_info.sequenceNumber = aac_decoder_frame_p->sequenceNumber;
    in_info.timestamp = aac_decoder_frame_p->timestamp;

    ret = a2dp_cp_put_in_frame(&in_info, sizeof(in_info),
                               aac_decoder_frame_p->aac_buffer,
                               aac_decoder_frame_p->aac_buffer_len);
    if (ret) {
      TRACE_A2DP_DECODER_D("[MCU][AAC] piff !!!!!!ret: %d ", ret);
      break;
    }
    check_sum = a2dp_audio_decoder_internal_check_sum_generate(
        aac_decoder_frame_p->aac_buffer, aac_decoder_frame_p->aac_buffer_len);
    a2dp_audio_list_remove(list, aac_decoder_frame_p);
  }
  ret = a2dp_cp_get_full_out_frame((void **)&out, &out_len);
  if (ret) {
    if (!get_in_cp_frame_cnt()) {
      TRACE_A2DP_DECODER_I("[MCU][AAC] cp cache underflow list:%d in_cp:%d",
                           a2dp_audio_list_length(list), get_in_cp_frame_cnt());
      return A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
    if (!a2dp_audio_sysfreq_boost_running()) {
      a2dp_audio_sysfreq_boost_start(1);
    }
    osDelay(12);
    ret = a2dp_cp_get_full_out_frame((void **)&out, &out_len);
    if (ret) {
      TRACE_A2DP_DECODER_I("[MCU][AAC] cp cache underflow list:%d in_cp:%d",
                           a2dp_audio_list_length(list), get_in_cp_frame_cnt());
      a2dp_cp_aac_lc_after_cache_underflow();
      return A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
  }

  if (out_len == 0) {
    memset(buffer, 0, buffer_bytes);
    a2dp_cp_consume_full_out_frame();
    TRACE_A2DP_DECODER_I("[MCU][AAC] olz!!!%d ", __LINE__);
    return A2DP_DECODER_NO_ERROR;
  }
  if (out_len != out_frame_len) {
    TRACE_A2DP_DECODER_I("[MCU][AAC] Bad out len %u (should be %u)", out_len,
                         out_frame_len);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }
  p_out_info = (struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *)out;
  if (p_out_info->pcm_len) {
    a2dp_audio_aac_lastframe_info.sequenceNumber =
        p_out_info->in_info.sequenceNumber;
    a2dp_audio_aac_lastframe_info.timestamp = p_out_info->in_info.timestamp;
    a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.frame_samples = p_out_info->frame_samples;
    a2dp_audio_aac_lastframe_info.decoded_frames += p_out_info->decoded_frames;
    a2dp_audio_aac_lastframe_info.undecode_frames =
        a2dp_audio_list_length(list) +
        a2dp_cp_get_in_frame_cnt_by_index(p_out_info->frame_idx) - 1;
    a2dp_audio_aac_lastframe_info.check_sum =
        check_sum ? check_sum : a2dp_audio_aac_lastframe_info.check_sum;
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_aac_lastframe_info);
  }

  if (p_out_info->pcm_len == buffer_bytes) {
    memcpy(buffer, p_out_info + 1, p_out_info->pcm_len);
    dec_ret = A2DP_DECODER_NO_ERROR;
  } else {
    TRACE_A2DP_DECODER_I("[MCU][AAC] olne!!!%d ", __LINE__);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }

  ret = a2dp_cp_consume_full_out_frame();
  if (ret) {

    TRACE_A2DP_DECODER_I(
        "[MCU][AAC] cp consume_full_out_frame() failed: ret=%d", ret);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }
  return dec_ret;
}

#ifdef __CP_EXCEPTION_TEST__
static bool _cp_assert = false;
int cp_assert(void) {
  _cp_assert = true;
  return 0;
}
#endif

TEXT_AAC_LOC
int a2dp_cp_aac_lc_cp_decode(void) {
  int ret;
  enum CP_EMPTY_OUT_FRM_T out_frm_st;
  uint8_t *out;
  uint32_t out_len;
  bool need_refill = false;
  uint8_t *dec_start;
  uint32_t dec_len;
  struct A2DP_CP_AAC_LC_IN_FRM_INFO_T *p_in_info;
  struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *p_out_info;
  uint8_t *in_buf;
  uint32_t in_len;
  uint32_t dec_sum;
  int error;
  uint32_t aac_maxreadBytes = AAC_READBUF_SIZE;
  UINT bufferSize = 0, bytesValid = 0;
  AAC_DECODER_ERROR decoder_err = AAC_DEC_OK;
  CStreamInfo *stream_info = NULL;
  if (cp_codec_reset) {
    cp_codec_reset = false;
    a2dp_audio_aac_lc_decoder_init();
  }

#ifdef __CP_EXCEPTION_TEST__
  if (_cp_assert) {
    _cp_assert = false;
    *(int *)0 = 1;
    // ASSERT_A2DP_DECODER(0, "ASSERT_A2DP_DECODER  %s %d", __func__, __LINE__);
  }
#endif
  out_frm_st = a2dp_cp_get_emtpy_out_frame((void **)&out, &out_len);
  if (out_frm_st != CP_EMPTY_OUT_FRM_OK &&
      out_frm_st != CP_EMPTY_OUT_FRM_WORKING) {
    return 1;
  }
  ASSERT_A2DP_DECODER(out_len > sizeof(*p_out_info),
                      "%s: Bad out_len %u (should > %u)", __func__, out_len,
                      sizeof(*p_out_info));

  p_out_info = (struct A2DP_CP_AAC_LC_OUT_FRM_INFO_T *)out;
  if (out_frm_st == CP_EMPTY_OUT_FRM_OK) {
    p_out_info->pcm_len = 0;
    p_out_info->decoded_frames = 0;
  }
  ASSERT_A2DP_DECODER(out_len > sizeof(*p_out_info) + p_out_info->pcm_len,
                      "%s: Bad out_len %u (should > %u + %u)", __func__,
                      out_len, sizeof(*p_out_info), p_out_info->pcm_len);

  dec_start = (uint8_t *)(p_out_info + 1) + p_out_info->pcm_len;
  dec_len = out_len - (dec_start - (uint8_t *)out);

  if (dec_len < DECODE_AAC_PCM_FRAME_LENGTH) {
    TRACE_A2DP_DECODER_W("[CP][AAC] aac_lc_decode pcm_len = %d \n", dec_len);
    return 2;
  }
  if (!aacDec_handle) {
    TRACE_A2DP_DECODER_W("[CP][AAC] aac_lc_decode not ready");
    return 3;
  }

  dec_sum = 0;
  error = 0;

  while (dec_sum < dec_len && error == 0) {
    ret = a2dp_cp_get_in_frame((void **)&in_buf, &in_len);
    if (ret) {
      return 4;
    }
    ASSERT_A2DP_DECODER(in_len > sizeof(*p_in_info),
                        "[CP][AAC] Bad in_len %u (should > %u)", in_len,
                        sizeof(*p_in_info));

    p_in_info = (struct A2DP_CP_AAC_LC_IN_FRM_INFO_T *)in_buf;
    if (need_refill) {
      p_in_info->sequenceNumber = UINT16_MAX;
    }
    in_buf += sizeof(*p_in_info);
    in_len -= sizeof(*p_in_info);

    if (in_len < 64)
      aac_maxreadBytes = 64;
    else if (in_len < 128)
      aac_maxreadBytes = 128;
    else if (in_len < 256)
      aac_maxreadBytes = 256;
    else if (in_len < 512)
      aac_maxreadBytes = 512;
    else if (in_len < 1024)
      aac_maxreadBytes = 1024;

    bufferSize = aac_maxreadBytes;
    bytesValid = aac_maxreadBytes;
    decoder_err =
        aacDecoder_Fill(aacDec_handle, &in_buf, &bufferSize, &bytesValid);
    if (decoder_err != AAC_DEC_OK) {
      TRACE_A2DP_DECODER_W("[CP][AAC] decoder_Fill failed:0x%x", decoder_err);
      // if aac failed reopen it again
      if (is_aacDecoder_Close(aacDec_handle)) {
        a2dp_audio_aac_lc_decoder_reinit();
        TRACE_A2DP_DECODER_I("[CP][AAC] aac_lc_decode reinin codec \n");
      }
      error = 1;
      goto end_decode;
    }

    /* decode one AAC frame */
    decoder_err =
        aacDecoder_DecodeFrame(aacDec_handle, (short *)(dec_start + dec_sum),
                               (dec_len - dec_sum) / 2, 0 /* flags */);
    TRACE_A2DP_DECODER_D("[CP][AAC] decoder seq:%d len:%d err:%x",
                         p_in_info->sequenceNumber, (dec_len - dec_sum),
                         decoder_err);
    if (decoder_err != AAC_DEC_OK) {
      TRACE_A2DP_DECODER_W("[CP][AAC] aac_lc_decode failed:0x%x seq:%d",
                           decoder_err, p_in_info->sequenceNumber);
      // if aac failed reopen it again
      a2dp_audio_aac_lc_decoder_reinit();
      TRACE_A2DP_DECODER_I("[CP][AAC]aac_lc_decode reinin codec \n");
      if (!need_refill) {
        need_refill = true;
        ret = a2dp_cp_consume_in_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_in_frame() failed: ret=%d",
               __func__, ret);
        continue;
      } else {
        need_refill = false;
        error = 1;
        goto end_decode;
      }
    }

    stream_info = aacDecoder_GetStreamInfo(aacDec_handle);
    if (!stream_info || stream_info->sampleRate <= 0) {
      TRACE_A2DP_DECODER_I("[CP][AAC]aac_lc_decode invalid stream info");
      error = 1;
      goto end_decode;
    }

    bufferSize = stream_info->frameSize * stream_info->numChannels *
                 2; // sizeof(pcm_buffer[0]);
    ASSERT_A2DP_DECODER(AAC_OUTPUT_FRAME_SAMPLES == bufferSize / 4,
                        "aac_lc_decode output mismatch samples:%d",
                        bufferSize / 4);

    dec_sum += bufferSize;

  end_decode:
    memcpy(&p_out_info->in_info, p_in_info, sizeof(*p_in_info));
    p_out_info->decoded_frames++;
    p_out_info->frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
    p_out_info->frame_idx = a2dp_cp_get_in_frame_index();
    if (need_refill) {
      TRACE_A2DP_DECODER_W("[CP][AAC] resume refill seq:%d",
                           p_in_info->sequenceNumber);
    } else {
      ret = a2dp_cp_consume_in_frame();
      ASSERT_A2DP_DECODER(
          ret == 0, "[CP][AAC] a2dp_cp_consume_in_frame() failed: ret=%d", ret);
    }
  }

  p_out_info->pcm_len += dec_sum;

  if (error || out_len <= sizeof(*p_out_info) + p_out_info->pcm_len) {
    ret = a2dp_cp_consume_emtpy_out_frame();
    ASSERT_A2DP_DECODER(
        ret == 0, "[CP][AAC] a2dp_cp_consume_emtpy_out_frame() failed: ret=%d",
        ret);
  }

  return error;
}
#endif

static int a2dp_audio_aac_lc_list_checker(void) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
  int cnt = 0;

  do {
    aac_decoder_frame_p =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(0);
    if (aac_decoder_frame_p) {
      a2dp_audio_list_append(list, aac_decoder_frame_p);
    }
    cnt++;
  } while (aac_decoder_frame_p && cnt < AAC_MTU_LIMITER);

  do {
    if ((node = a2dp_audio_list_begin(list)) != NULL) {
      aac_decoder_frame_p =
          (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
      a2dp_audio_list_remove(list, aac_decoder_frame_p);
    }
  } while (node);

  TRACE_A2DP_DECODER_I("[AAC][INIT] cnt:%d list:%d", cnt,
                       a2dp_audio_list_length(list));

  return 0;
}

int a2dp_audio_aac_lc_channel_select(A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel) {
  AAC_DECODER_CHANNEL_SELECT_E aac_decoder_channel_select =
      AAC_DECODER_CHANNEL_SELECT_SELECT_STEREO;
  switch (chnl_sel) {
  case A2DP_AUDIO_CHANNEL_SELECT_STEREO:
    aac_decoder_channel_select = AAC_DECODER_CHANNEL_SELECT_SELECT_STEREO;
    break;
  case A2DP_AUDIO_CHANNEL_SELECT_LRMERGE:
    aac_decoder_channel_select = AAC_DECODER_CHANNEL_SELECT_SELECT_LRMERGE;
    break;
  case A2DP_AUDIO_CHANNEL_SELECT_LCHNL:
    aac_decoder_channel_select = AAC_DECODER_CHANNEL_SELECT_LCHNL;
    break;
  case A2DP_AUDIO_CHANNEL_SELECT_RCHNL:
    aac_decoder_channel_select = AAC_DECODER_CHANNEL_SELECT_RCHNL;
  default:
    break;
  }
  aacDecoder_DecodeFrame_Config(aac_decoder_channel_select);

  return 0;
}

int a2dp_audio_aac_lc_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context) {
  TRACE_A2DP_DECODER_I("[AAC] init");

  a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

  memset(&a2dp_audio_aac_lastframe_info, 0,
         sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));
  a2dp_audio_aac_lastframe_info.stream_info = *config;
  a2dp_audio_aac_lastframe_info.frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
  a2dp_audio_aac_lastframe_info.list_samples = AAC_OUTPUT_FRAME_SAMPLES;
  a2dp_audio_decoder_internal_lastframe_info_set(
      &a2dp_audio_aac_lastframe_info);

  ASSERT_A2DP_DECODER(a2dp_audio_context_p->dest_packet_mut < AAC_MTU_LIMITER,
                      "%s MTU OVERFLOW:%u/%u", __func__,
                      a2dp_audio_context_p->dest_packet_mut, AAC_MTU_LIMITER);

  aac_mempoll = (uint8_t *)a2dp_audio_heap_malloc(AAC_MEMPOOL_SIZE);
  ASSERT_A2DP_DECODER(aac_mempoll, "aac_mempoll = NULL");
  aac_memhandle = heap_register(aac_mempoll, AAC_MEMPOOL_SIZE);

#ifdef A2DP_CP_ACCEL
  int ret;

  cp_codec_reset = true;
  ret = a2dp_cp_init(a2dp_cp_aac_lc_cp_decode, CP_PROC_DELAY_2_FRAMES);
  ASSERT_A2DP_DECODER(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__,
                      ret);
#else
  a2dp_audio_aac_lc_decoder_init();
#endif
  a2dp_audio_aac_lc_reorder_init();
  a2dp_audio_aac_lc_list_checker();

  a2dp_audio_aac_lc_channel_select(a2dp_audio_context_p->chnl_sel);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_deinit(void) {
#ifdef A2DP_CP_ACCEL
  a2dp_cp_deinit();
#endif
  a2dp_audio_aac_lc_decoder_deinit();
  a2dp_audio_aac_lc_reorder_deinit();
  size_t total = 0, used = 0, max_used = 0;
  heap_memory_info(aac_memhandle, &total, &used, &max_used);
  a2dp_audio_heap_free(aac_mempoll);
  TRACE_A2DP_DECODER_I(
      "[AAC] deinit MEM: total - %d, used - %d, max_used - %d.", total, used,
      max_used);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_mcu_decode_frame(uint8_t *buffer, uint32_t buffer_bytes) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

  UINT bufferSize = 0, bytesValid = 0;
  AAC_DECODER_ERROR decoder_err = AAC_DEC_OK;
  CStreamInfo *stream_info = NULL;

  bool cache_underflow = false;
  uint32_t aac_maxreadBytes = AAC_READBUF_SIZE;
  int output_byte = 0;

  if (buffer_bytes < DECODE_AAC_PCM_FRAME_LENGTH) {
    TRACE_A2DP_DECODER_W("[MCU][AAC] pcm_len = %d \n", buffer_bytes);
    return A2DP_DECODER_NO_ERROR;
  }
  if (!aacDec_handle) {
    TRACE_A2DP_DECODER_W("[MCU][AAC] aac_lc_decode not ready");
    return A2DP_DECODER_NO_ERROR;
  }

  node = a2dp_audio_list_begin(list);
  if (!node) {
    TRACE_A2DP_DECODER_W("[MCU][AAC] cache underflow");
    cache_underflow = true;
    goto exit;
  } else {
    aac_decoder_frame_p =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);

    if (aac_decoder_frame_p->aac_buffer_len < 64)
      aac_maxreadBytes = 64;
    else if (aac_decoder_frame_p->aac_buffer_len < 128)
      aac_maxreadBytes = 128;
    else if (aac_decoder_frame_p->aac_buffer_len < 256)
      aac_maxreadBytes = 256;
    else if (aac_decoder_frame_p->aac_buffer_len < 512)
      aac_maxreadBytes = 512;
    else if (aac_decoder_frame_p->aac_buffer_len < 1024)
      aac_maxreadBytes = 1024;

    bufferSize = aac_maxreadBytes;
    bytesValid = aac_maxreadBytes;
    decoder_err =
        aacDecoder_Fill(aacDec_handle, &(aac_decoder_frame_p->aac_buffer),
                        &bufferSize, &bytesValid);
    if (decoder_err != AAC_DEC_OK) {
      TRACE_A2DP_DECODER_W("[MCU][AAC] aacDecoder_Fill failed:0x%x",
                           decoder_err);
      // if aac failed reopen it again
      if (is_aacDecoder_Close(aacDec_handle)) {
        a2dp_audio_aac_lc_decoder_reinit();
        TRACE_A2DP_DECODER_I("[MCU][AAC] aac_lc_decode reinin codec \n");
      }
      goto end_decode;
    }

    /* decode one AAC frame */
    decoder_err = aacDecoder_DecodeFrame(aacDec_handle, (short *)buffer,
                                         buffer_bytes / 2, 0 /* flags */);
    TRACE_A2DP_DECODER_D("[MCU][AAC] decoder seq:%d len:%d err:%x",
                         aac_decoder_frame_p->sequenceNumber,
                         aac_decoder_frame_p->aac_buffer_len, decoder_err);
    if (decoder_err != AAC_DEC_OK) {
      TRACE_A2DP_DECODER_W("[MCU][AAC]aac_lc_decode failed:0x%x", decoder_err);
      // if aac failed reopen it again
      if (is_aacDecoder_Close(aacDec_handle)) {
        a2dp_audio_aac_lc_decoder_reinit();
        TRACE_A2DP_DECODER_I("[MCU][AAC]aac_lc_decode reinin codec \n");
      }
      goto end_decode;
    }

    stream_info = aacDecoder_GetStreamInfo(aacDec_handle);
    if (!stream_info || stream_info->sampleRate <= 0) {
      TRACE_A2DP_DECODER_W("[MCU][AAC]aac_lc_decode invalid stream info");
      goto end_decode;
    }

    output_byte = stream_info->frameSize * stream_info->numChannels *
                  2; // sizeof(pcm_buffer[0]);
    ASSERT_A2DP_DECODER(AAC_OUTPUT_FRAME_SAMPLES == output_byte / 4,
                        "aac_lc_decode output mismatch samples:%d",
                        output_byte / 4);
  end_decode:
    a2dp_audio_aac_lastframe_info.sequenceNumber =
        aac_decoder_frame_p->sequenceNumber;
    a2dp_audio_aac_lastframe_info.timestamp = aac_decoder_frame_p->timestamp;
    a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
    a2dp_audio_aac_lastframe_info.frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
    a2dp_audio_aac_lastframe_info.decoded_frames++;
    a2dp_audio_aac_lastframe_info.undecode_frames =
        a2dp_audio_list_length(list) - 1;
    a2dp_audio_aac_lastframe_info.check_sum =
        a2dp_audio_decoder_internal_check_sum_generate(
            aac_decoder_frame_p->aac_buffer,
            aac_decoder_frame_p->aac_buffer_len);
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_aac_lastframe_info);
    a2dp_audio_list_remove(list, aac_decoder_frame_p);
  }
exit:
  if (cache_underflow) {
    a2dp_audio_aac_lastframe_info.undecode_frames = 0;
    a2dp_audio_aac_lastframe_info.check_sum = 0;
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_aac_lastframe_info);
    output_byte = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
  }
  return output_byte;
}

int a2dp_audio_aac_lc_decode_frame(uint8_t *buffer, uint32_t buffer_bytes) {
#ifdef A2DP_CP_ACCEL
  return a2dp_cp_aac_lc_mcu_decode(buffer, buffer_bytes);
#else
  return a2dp_audio_aac_lc_mcu_decode_frame(buffer, buffer_bytes);
#endif
}

int a2dp_audio_aac_lc_preparse_packet(btif_media_header_t *header,
                                      uint8_t *buffer, uint32_t buffer_bytes) {
  a2dp_audio_aac_lastframe_info.sequenceNumber = header->sequenceNumber;
  a2dp_audio_aac_lastframe_info.timestamp = header->timestamp;
  a2dp_audio_aac_lastframe_info.curSubSequenceNumber = 0;
  a2dp_audio_aac_lastframe_info.totalSubSequenceNumber = 0;
  a2dp_audio_aac_lastframe_info.frame_samples = AAC_OUTPUT_FRAME_SAMPLES;
  a2dp_audio_aac_lastframe_info.list_samples = AAC_OUTPUT_FRAME_SAMPLES;
  a2dp_audio_aac_lastframe_info.decoded_frames = 0;
  a2dp_audio_aac_lastframe_info.undecode_frames = 0;
  a2dp_audio_aac_lastframe_info.check_sum = 0;
  a2dp_audio_decoder_internal_lastframe_info_set(
      &a2dp_audio_aac_lastframe_info);

  TRACE_A2DP_DECODER_I("[AAC][PRE] seq:%d timestamp:%08x",
                       header->sequenceNumber, header->timestamp);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_reorder_init(void) {
  aac_decoder_frame_reorder_p = NULL;
  aac_decoder_last_valid_frame_ready = false;
  aac_decoder_last_valid_frame.sequenceNumber = 0;
  aac_decoder_last_valid_frame.timestamp = 0;
  aac_decoder_last_valid_frame.aac_buffer = NULL;
  aac_decoder_last_valid_frame.aac_buffer_len = 0;

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_reorder_deinit(void) {
  aac_decoder_frame_reorder_p = NULL;
  aac_decoder_last_valid_frame_ready = false;
  aac_decoder_last_valid_frame.sequenceNumber = 0;
  aac_decoder_last_valid_frame.timestamp = 0;
  aac_decoder_last_valid_frame.aac_buffer = NULL;
  aac_decoder_last_valid_frame.aac_buffer_len = 0;
  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_reorder_store_packet(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p,
    btif_media_header_t *header, uint8_t *buffer, uint32_t buffer_bytes) {
  aac_decoder_frame_p->sequenceNumber = header->sequenceNumber;
  aac_decoder_frame_p->timestamp = header->timestamp;
  memcpy(aac_decoder_frame_p->aac_buffer, buffer, buffer_bytes);
  aac_decoder_frame_p->aac_buffer_len = buffer_bytes;
  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_reorder_proc(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p,
    btif_media_header_t *header, uint8_t *buffer, uint32_t buffer_bytes) {
  uint8_t *dest_buf = NULL;
  if ((aac_decoder_frame_p->aac_buffer_len + buffer_bytes) > AAC_READBUF_SIZE) {
    return A2DP_DECODER_NO_ERROR;
  }
  TRACE_A2DP_DECODER_W("[AAC][INPUT][REORDER] proc enter seq:%d len:%d",
                       aac_decoder_frame_p->sequenceNumber,
                       aac_decoder_frame_p->aac_buffer_len);
  dest_buf =
      &aac_decoder_frame_p->aac_buffer[aac_decoder_frame_p->aac_buffer_len];
  memcpy(dest_buf, buffer, buffer_bytes);
  aac_decoder_frame_p->aac_buffer_len += buffer_bytes;
  TRACE_A2DP_DECODER_W("[AAC][INPUT][REORDER] proc exit seq:%d len:%d",
                       aac_decoder_frame_p->sequenceNumber,
                       aac_decoder_frame_p->aac_buffer_len);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_packet_recover_save_last(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame) {
  aac_decoder_last_valid_frame_ready = true;
  aac_decoder_last_valid_frame.sequenceNumber =
      aac_decoder_frame->sequenceNumber;
  aac_decoder_last_valid_frame.timestamp = aac_decoder_frame->timestamp;
  return 0;
}

int a2dp_audio_aac_lc_packet_recover_find_missing(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame) {
  uint16_t diff_seq = 0;
  uint32_t diff_timestamp = 0;
  uint32_t diff = 0;
  float tmp_pkt_cnt = 0;
  uint32_t need_recover_pkt = 0;

  if (!aac_decoder_last_valid_frame_ready) {
    return need_recover_pkt;
  }

  diff_seq = a2dp_audio_get_passed(aac_decoder_frame->sequenceNumber,
                                   aac_decoder_last_valid_frame.sequenceNumber,
                                   UINT16_MAX);
  diff_timestamp =
      a2dp_audio_get_passed(aac_decoder_frame->timestamp,
                            aac_decoder_last_valid_frame.timestamp, UINT32_MAX);

  if (diff_seq > 1) {
    TRACE_A2DP_DECODER_W("[AAC][INPUT][PLC] seq:%d/%d stmp:%d/%d",
                         aac_decoder_frame->sequenceNumber,
                         aac_decoder_last_valid_frame.sequenceNumber,
                         aac_decoder_frame->timestamp,
                         aac_decoder_last_valid_frame.timestamp);
    diff = diff_timestamp / diff_seq;
    if (diff % AAC_OUTPUT_FRAME_SAMPLES == 0) {
      tmp_pkt_cnt = diff_timestamp / AAC_OUTPUT_FRAME_SAMPLES;
    } else {
      tmp_pkt_cnt =
          (float)diff_timestamp /
          ((1000.f / (float)a2dp_audio_context_p->output_cfg.sample_rate) *
           (float)AAC_OUTPUT_FRAME_SAMPLES);
    }
    need_recover_pkt = (uint32_t)(tmp_pkt_cnt + 0.5f);
    TRACE_A2DP_DECODER_W(
        "[AAC][INPUT][PLC] diff_seq:%d diff_stmp:%d diff:%d missing:%d",
        diff_seq, diff_timestamp, diff, need_recover_pkt);
    if (need_recover_pkt == diff_seq) {
      need_recover_pkt--;
      TRACE_A2DP_DECODER_W("[AAC][INPUT][PLC] need_recover_pkt seq:%d",
                           need_recover_pkt);
    } else {
      need_recover_pkt = 0;
    }
  }
  return need_recover_pkt;
}

int a2dp_audio_aac_lc_packet_recover_proc(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  int missing_pkt_cnt = 0;
  missing_pkt_cnt =
      a2dp_audio_aac_lc_packet_recover_find_missing(aac_decoder_frame);
  if (missing_pkt_cnt <= 4 &&
      a2dp_audio_list_length(list) + missing_pkt_cnt < aac_mtu_limiter) {
    for (uint8_t i = 0; i < missing_pkt_cnt; i++) {
      a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p =
          (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(
              aac_decoder_frame->aac_buffer_len);
      aac_decoder_frame_p->sequenceNumber = UINT16_MAX;
      aac_decoder_frame_p->timestamp = UINT32_MAX;
      memcpy(aac_decoder_frame_p->aac_buffer, aac_decoder_frame->aac_buffer,
             aac_decoder_frame->aac_buffer_len);
      aac_decoder_frame_p->aac_buffer_len = aac_decoder_frame->aac_buffer_len;
      a2dp_audio_list_append(list, aac_decoder_frame_p);
    }
  }

  return A2DP_DECODER_NO_ERROR;
}

int inline a2dp_audio_aac_lc_packet_append(
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  a2dp_audio_aac_lc_packet_recover_proc(aac_decoder_frame);
  a2dp_audio_aac_lc_packet_recover_save_last(aac_decoder_frame);
  a2dp_audio_list_append(list, aac_decoder_frame);
  return 0;
}

#if 1
int a2dp_audio_aac_lc_store_packet(btif_media_header_t *header, uint8_t *buffer,
                                   uint32_t buffer_bytes) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  int nRet = A2DP_DECODER_NO_ERROR;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

  if (a2dp_audio_list_length(list) < aac_mtu_limiter &&
      buffer_bytes <= AAC_READBUF_SIZE) {
    if (aac_decoder_frame_reorder_p == NULL) {
      aac_decoder_frame_p =
          (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(
              buffer_bytes);
      aac_decoder_frame_reorder_p = aac_decoder_frame_p;
      TRACE_A2DP_DECODER_W("[AAC][INPUT][REORDER] start seq:%d len:%d",
                           header->sequenceNumber, buffer_bytes);
      a2dp_audio_aac_lc_reorder_store_packet(aac_decoder_frame_p, header,
                                             buffer, buffer_bytes);
    } else {
      if (aac_decoder_frame_reorder_p->aac_buffer[0] == buffer[0] &&
          aac_decoder_frame_reorder_p->aac_buffer[1] == buffer[1]) {
        a2dp_audio_aac_lc_packet_append(aac_decoder_frame_reorder_p);
        aac_decoder_frame_p =
            (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(
                buffer_bytes);
        aac_decoder_frame_reorder_p = aac_decoder_frame_p;
        a2dp_audio_aac_lc_reorder_store_packet(aac_decoder_frame_p, header,
                                               buffer, buffer_bytes);
      } else {
        aac_decoder_frame_p = aac_decoder_frame_reorder_p;
        a2dp_audio_aac_lc_reorder_proc(aac_decoder_frame_p, header, buffer,
                                       buffer_bytes);
        a2dp_audio_aac_lc_packet_append(aac_decoder_frame_p);
        aac_decoder_frame_reorder_p = NULL;
      }
    }
    nRet = A2DP_DECODER_NO_ERROR;
  } else {
    TRACE_A2DP_DECODER_W(
        "[AAC][INPUT] list full current list_len:%d buff_len:%d",
        a2dp_audio_list_length(list), buffer_bytes);
    nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
  }

  return nRet;
}
#else
int a2dp_audio_aac_lc_store_packet(btif_media_header_t *header, uint8_t *buffer,
                                   uint32_t buffer_bytes) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  int nRet = A2DP_DECODER_NO_ERROR;

  if (!a2dp_audio_aac_lc_reorder_valid(header, buffer, buffer_bytes)) {
    return A2DP_DECODER_NO_ERROR;
  }

  if (a2dp_audio_list_length(list) < aac_mtu_limiter &&
      buffer_bytes <= AAC_READBUF_SIZE) {
    a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_aac_lc_frame_malloc(
            buffer_bytes);
    TRACE_A2DP_DECODER_D("[AAC][INPUT]seq:%d len:%d", header->sequenceNumber,
                         buffer_bytes);
    aac_decoder_frame_p->sequenceNumber = header->sequenceNumber;
    aac_decoder_frame_p->timestamp = header->timestamp;
    memcpy(aac_decoder_frame_p->aac_buffer, buffer, buffer_bytes);
    aac_decoder_frame_p->aac_buffer_len = buffer_bytes;
    a2dp_audio_aac_lc_reorder_history(aac_decoder_frame_p);
    a2dp_audio_list_append(list, aac_decoder_frame_p);
    nRet = A2DP_DECODER_NO_ERROR;
  } else {
    TRACE_A2DP_DECODER_I(
        "[AAC][INPUT] list full current list_len:%d buff_len:%d",
        a2dp_audio_list_length(list), buffer_bytes);
    nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
  }

  return nRet;
}
#endif

int a2dp_audio_aac_lc_discards_packet(uint32_t packets) {
  int nRet = A2DP_DECODER_MEMORY_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
  a2dp_cp_reset_frame();
#endif

  if (packets <= a2dp_audio_list_length(list)) {
    for (uint8_t i = 0; i < packets; i++) {
      if ((node = a2dp_audio_list_begin(list)) != NULL) {
        aac_decoder_frame_p =
            (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
        a2dp_audio_list_remove(list, aac_decoder_frame_p);
      }
    }
    nRet = A2DP_DECODER_NO_ERROR;
  }

  TRACE_A2DP_DECODER_I("[AAC][DISCARDS] packets:%d nRet:%d", packets, nRet);
  return nRet;
}

int a2dp_audio_aac_lc_headframe_info_get(
    A2DP_AUDIO_HEADFRAME_INFO_T *headframe_info) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame = NULL;

  if (a2dp_audio_list_length(list) &&
      ((node = a2dp_audio_list_begin(list)) != NULL)) {
    aac_decoder_frame =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
    headframe_info->sequenceNumber = aac_decoder_frame->sequenceNumber;
    headframe_info->timestamp = aac_decoder_frame->timestamp;
    headframe_info->curSubSequenceNumber = 0;
    headframe_info->totalSubSequenceNumber = 0;
  } else {
    memset(headframe_info, 0, sizeof(A2DP_AUDIO_HEADFRAME_INFO_T));
  }

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_info_get(void *info) { return A2DP_DECODER_NO_ERROR; }

int a2dp_audio_aac_lc_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info,
                                         uint32_t mask) {
  int nRet = A2DP_DECODER_SYNC_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  int list_len;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
  a2dp_cp_reset_frame();
#endif

  list_len = a2dp_audio_list_length(list);

  for (uint16_t i = 0; i < list_len; i++) {
    node = a2dp_audio_list_begin(list);
    if (node) {
      aac_decoder_frame_p =
          (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
      TRACE_A2DP_DECODER_D(
          "[synchronize_packet]%d/%d %x/%x",
          aac_decoder_frame_p->sequenceNumber, sync_info->sequenceNumber,
          aac_decoder_frame_p->timestamp, sync_info->timestamp);
      if (A2DP_AUDIO_SYNCFRAME_CHK(aac_decoder_frame_p->sequenceNumber ==
                                       sync_info->sequenceNumber,
                                   A2DP_AUDIO_SYNCFRAME_MASK_SEQ, mask) &&
          A2DP_AUDIO_SYNCFRAME_CHK(aac_decoder_frame_p->timestamp ==
                                       sync_info->timestamp,
                                   A2DP_AUDIO_SYNCFRAME_MASK_TIMESTAMP, mask)) {
        nRet = A2DP_DECODER_NO_ERROR;
        break;
      }
      a2dp_audio_list_remove(list, aac_decoder_frame_p);
    }
  }

  node = a2dp_audio_list_begin(list);
  if (node) {
    aac_decoder_frame_p =
        (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
    TRACE_A2DP_DECODER_I(
        "[MCU][SYNC][AAC] sync pkt nRet:%d SEQ:%d timestamp:%d", nRet,
        aac_decoder_frame_p->sequenceNumber, aac_decoder_frame_p->timestamp);
  } else {
    TRACE_A2DP_DECODER_I("[MCU][SYNC][AAC] sync pkt nRet:%d", nRet);
  }

  return nRet;
}

int a2dp_audio_aac_lc_synchronize_dest_packet_mut(uint16_t packet_mut) {
  list_node_t *node = NULL;
  uint32_t list_len = 0;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;

  list_len = a2dp_audio_list_length(list);
  if (list_len > packet_mut) {
    do {
      node = a2dp_audio_list_begin(list);
      if (node) {
        aac_decoder_frame_p =
            (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
        a2dp_audio_list_remove(list, aac_decoder_frame_p);
      }
    } while (a2dp_audio_list_length(list) > packet_mut);
  }

  if (packet_mut == 0) {
    if (aac_decoder_frame_reorder_p) {
      a2dp_audio_aac_lc_free(aac_decoder_frame_reorder_p);
      a2dp_audio_aac_lc_reorder_init();
    }
  }

  TRACE_A2DP_DECODER_I("[MCU][SYNC][AAC] dest pkt list:%d",
                       a2dp_audio_list_length(list));
  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_convert_list_to_samples(uint32_t *samples) {
  uint32_t list_len = 0;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

  list_len = a2dp_audio_list_length(list);
  *samples = AAC_OUTPUT_FRAME_SAMPLES * list_len;

  TRACE_A2DP_DECODER_I("AUD][DECODER][MCU][AAC] list:%d samples:%d", list_len,
                       *samples);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_aac_lc_discards_samples(uint32_t samples) {
  int nRet = A2DP_DECODER_SYNC_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  a2dp_audio_aac_decoder_frame_t *aac_decoder_frame_p = NULL;
  list_node_t *node = NULL;
  int need_remove_list = 0;
  uint32_t list_samples = 0;
  ASSERT_A2DP_DECODER(!(samples % AAC_OUTPUT_FRAME_SAMPLES),
                      "%s samples err:%d", __func__, samples);

  a2dp_audio_aac_lc_convert_list_to_samples(&list_samples);
  if (list_samples >= samples) {
    need_remove_list = samples / AAC_OUTPUT_FRAME_SAMPLES;
    for (int i = 0; i < need_remove_list; i++) {
      node = a2dp_audio_list_begin(list);
      if (node) {
        aac_decoder_frame_p =
            (a2dp_audio_aac_decoder_frame_t *)a2dp_audio_list_node(node);
        a2dp_audio_list_remove(list, aac_decoder_frame_p);
      }
    }
    nRet = A2DP_DECODER_NO_ERROR;
  }

  return nRet;
}

A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config = {
    {44100, 2, 16},
    1,
    a2dp_audio_aac_lc_init,
    a2dp_audio_aac_lc_deinit,
    a2dp_audio_aac_lc_decode_frame,
    a2dp_audio_aac_lc_preparse_packet,
    a2dp_audio_aac_lc_store_packet,
    a2dp_audio_aac_lc_discards_packet,
    a2dp_audio_aac_lc_synchronize_packet,
    a2dp_audio_aac_lc_synchronize_dest_packet_mut,
    a2dp_audio_aac_lc_convert_list_to_samples,
    a2dp_audio_aac_lc_discards_samples,
    a2dp_audio_aac_lc_headframe_info_get,
    a2dp_audio_aac_lc_info_get,
    a2dp_audio_aac_lc_free,
    a2dp_audio_aac_lc_channel_select,
};
#else
A2DP_AUDIO_DECODER_T a2dp_audio_aac_lc_decoder_config = {
    0,
};
#endif
