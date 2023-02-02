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
#include "btapp.h"
#include "cmsis.h"
#include "hal_location.h"
#include "heap_api.h"
#include "ldacBT.h"
#include "plat_types.h"
#include <string.h>

typedef struct {
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint32_t frame_samples;
  uint16_t curSubSequenceNumber;
  uint16_t totalSubSequenceNumber;
  uint8_t *buffer;
  uint32_t buffer_len;
} a2dp_audio_ldac_decoder_frame_t;

#ifndef LDAC_MTU_LIMITER
#define LDAC_MTU_LIMITER (200)
#endif
#define DECODE_LDAC_PCM_FRAME_LENGTH (256 * 4 * 2 * 2)

#define LDAC_LIST_SAMPLES (256)

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_ldac_lastframe_info;

static uint16_t ldac_mtu_limiter = LDAC_MTU_LIMITER;
// static btif_media_header_t ldac_header_parser_header_prev = {0,};
// static bool ldac_header_parser_ready = false;
// static bool ldac_chnl_mode_mono = false;

HANDLE_LDAC_BT LdacDecHandle = NULL;
// static uint8_t *ldac_mempoll = NULL;
heap_handle_t ldac_memhandle = NULL;

static uint32_t l2cap_frame_samples = LDAC_LIST_SAMPLES;

/* Convert LDAC Error Code to string */
#define CASE_RETURN_STR(const)                                                 \
  case const:                                                                  \
    return #const;
static const char *ldac_ErrCode2Str(int ErrCode) {
  switch (ErrCode) {
    CASE_RETURN_STR(LDACBT_ERR_NONE);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_BIT_ALLOCATION);
    CASE_RETURN_STR(LDACBT_ERR_NOT_IMPLEMENTED);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL_ENCODE);
    CASE_RETURN_STR(LDACBT_ERR_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_BAND);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_A);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_B);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_C);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_D);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_E);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_IDSF);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_SPEC);
    CASE_RETURN_STR(LDACBT_ERR_BIT_PACKING);
    CASE_RETURN_STR(LDACBT_ERR_ALLOC_MEMORY);
    CASE_RETURN_STR(LDACBT_ERR_FATAL_HANDLE);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SYNCWORD);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SMPL_FORMAT);
    CASE_RETURN_STR(LDACBT_ERR_ILL_PARAM);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_STATUS);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_NSHIFT);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_MODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADMODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_A);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_B);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_C);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_D);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_NBANDS);
    CASE_RETURN_STR(LDACBT_ERR_PACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_DEC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_INPUT_BUFFER_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_FRAME_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_LENGTH_OVER);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_ALIGN_OVER);
    CASE_RETURN_STR(LDACBT_ERR_ALTER_EQMID_LIMITED);
    CASE_RETURN_STR(LDACBT_ERR_ILL_EQMID);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ILL_NUM_CHANNEL);
    CASE_RETURN_STR(LDACBT_ERR_ILL_MTU_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_HANDLE_NOT_INIT);
  default:
    return "unknown-error-code";
  }
}

char a_ErrorCodeStr[128];
const char *get_error_code_string(int error_code) {
  int errApi, errHdl, errBlk;

  errApi = LDACBT_API_ERR(error_code);
  errHdl = LDACBT_HANDLE_ERR(error_code);
  errBlk = LDACBT_BLOCK_ERR(error_code);

  a_ErrorCodeStr[0] = '\0';
  strcat(a_ErrorCodeStr, "API:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errApi));
  strcat(a_ErrorCodeStr, " Handle:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errHdl));
  strcat(a_ErrorCodeStr, " Block:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errBlk));
  return a_ErrorCodeStr;
}

#define LDAC_FRAME_LEN_INDEX_106 106
#define LDAC_FRAME_LEN_INDEX_128 128
#define LDAC_FRAME_LEN_INDEX_160 160
#define LDAC_FRAME_LEN_INDEX_216 216
#define LDAC_FRAME_LEN_INDEX_326 326
#define LDAC_FRAME_LEN_INDEX_161 161

static uint8_t get_ldac_frame_num_by_rawdata(uint8_t *buffer,
                                             uint32_t buffer_bytes) {
  uint32_t frame_len = 0;
  uint32_t frame_len_with_head = 0;
  uint16_t data1 = 0;
  uint16_t data2 = 0;
  uint8_t frame_cnt = 0;
  for (uint32_t i = 0; i < buffer_bytes;
       i += frame_len_with_head, frame_cnt++) {
    // TRACE(4,"buffer:%x %x %x %x
    // ",buffer[i],buffer[i+1],buffer[i+2],buffer[i+3]);
    data1 = (uint16_t)(buffer[i + 1] & 0x07);
    data2 = (uint16_t)buffer[i + 2];
    frame_len = ((data1 << 6 & 0xFFFF) | (data2 >> 2 & 0xFFFF));
    // TRACE("#frame len:%d",frame_len);
    // for ldac head
    frame_len_with_head = frame_len + 4;
  }
  return frame_cnt;
}

#define LDAC_READBUF_SIZE                                                      \
  1024 /* pick something big enough to hold a bunch of frames */

/**
 * Decode LDAC data...
 */
//#include "os_tcb.h"
extern const char *get_error_code_string(int error_code);

#define DCODE_LDAC_PCM_FRAME_LENGTH 10224 * 2

int check_ldac_header(uint8_t *buffer, uint32_t buff_len) {
  int channel_mode = 0;
  int sample_rate = 0;
  int ret = 0;
  if (buff_len < 2)
    ret = -1;
  sample_rate = bt_get_ladc_sample_rate();
  channel_mode = bt_ldac_player_get_channelmode();
  // TRACE(3,"%s,%d,%d",__func__,sample_rate,channel_mode);

  uint32_t i = 0;
  unsigned char sync2 = 0;
  unsigned char cci = 0;

  switch (channel_mode) {
  case LDACBT_CHANNEL_MODE_MONO:
    cci = LDAC_CCI_MONO;
    break;
  case LDACBT_CHANNEL_MODE_DUAL_CHANNEL:
    cci = LDAC_CCI_DUAL_CHANNEL;
    break;
  case LDACBT_CHANNEL_MODE_STEREO:
  default:
    cci = LDAC_CCI_STEREO;
    break;
  }

  if (sample_rate == 1 * 44100) {
    sync2 = (0 << 5) | (cci << 3);
  } else if (sample_rate == 1 * 48000) {
    sync2 = (1 << 5) | (cci << 3);
  } else if (sample_rate == 2 * 44100) {
    sync2 = (2 << 5) | (cci << 3);
  } else if (sample_rate == 2 * 48000) {
    sync2 = (3 << 5) | (cci << 3);
  } else {
    TRACE(0, "sample rate not surpoort !");
    ret = -2;
    return ret;
  }

  for (i = 0; i < buff_len; i++) {
    if (buffer[i] == 0xAA) {
      if ((buffer[i + 1] & 0xF8) == sync2) {
        // TRACE(0,"find ldac header ");
        break;
      }
    }
  }
  if (i >= buff_len) {
    TRACE(0, "no find ldac header ");
    ret = -3;
  }

  return ret;
}

#ifdef A2DP_CP_ACCEL
struct A2DP_CP_LDAC_IN_FRM_INFO_T {
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint16_t curSubSequenceNumber;
  uint16_t totalSubSequenceNumber;
};

struct A2DP_CP_LDAC_OUT_FRM_INFO_T {
  struct A2DP_CP_LDAC_IN_FRM_INFO_T in_info;
  uint16_t frame_samples;
  uint16_t decoded_frames;
  uint16_t frame_idx;
  uint16_t pcm_len;
};

extern "C" uint32_t get_in_cp_frame_cnt(void);
extern "C" unsigned int set_cp_reset_flag(uint8_t evt);
extern uint32_t app_bt_stream_get_dma_buffer_samples(void);

int a2dp_cp_ldac_cp_decode(void);

TEXT_LDAC_LOC
static int a2dp_cp_ldac_after_cache_underflow(void) {
  TRACE(1, "%s", __func__);
  int ret = 0;
  a2dp_cp_deinit();
  ret = a2dp_cp_init(a2dp_cp_ldac_cp_decode, CP_PROC_DELAY_2_FRAMES);
  ASSERT(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__, ret);

  return ret;
}

static int a2dp_cp_ldac_mcu_decode(uint8_t *buffer, uint32_t buffer_bytes) {
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;
  list_node_t *node = NULL;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  int ret, dec_ret;
  struct A2DP_CP_LDAC_IN_FRM_INFO_T in_info;
  struct A2DP_CP_LDAC_OUT_FRM_INFO_T *p_out_info;

  uint8_t *out;
  uint32_t out_len;
  uint32_t out_frame_len;
  // uint8_t count=0;

  uint32_t cp_buffer_frames_max = 0;

  if ((buffer_bytes < DECODE_LDAC_PCM_FRAME_LENGTH &&
       (LDACBT_CHANNEL_MODE_MONO != bt_ldac_player_get_channelmode())) ||
      (buffer_bytes < DECODE_LDAC_PCM_FRAME_LENGTH / 2 &&
       (LDACBT_CHANNEL_MODE_MONO == bt_ldac_player_get_channelmode()))) {
    TRACE(1, "ldac_decode pcm_len = %d \n", buffer_bytes);
    return A2DP_DECODER_NO_ERROR;
  }

  if (!LdacDecHandle) {
    TRACE(0, "ldac decode not ready");
    return A2DP_DECODER_NO_ERROR;
  }

  cp_buffer_frames_max = app_bt_stream_get_dma_buffer_samples() / 2;

  if (cp_buffer_frames_max % (a2dp_audio_ldac_lastframe_info.frame_samples)) {
    cp_buffer_frames_max =
        cp_buffer_frames_max / (a2dp_audio_ldac_lastframe_info.frame_samples) +
        1;
  } else {
    cp_buffer_frames_max =
        cp_buffer_frames_max / (a2dp_audio_ldac_lastframe_info.frame_samples);
  }

  out_frame_len = sizeof(*p_out_info) + buffer_bytes;

  ret = a2dp_cp_decoder_init(out_frame_len, cp_buffer_frames_max * 8);

  if (ret) {
    TRACE(2, "%s: a2dp_cp_decoder_init() failed: ret=%d", __func__, ret);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }

  while ((node = a2dp_audio_list_begin(list)) != NULL) {
    ldac_decoder_frame_p =
        (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);

    in_info.sequenceNumber = ldac_decoder_frame_p->sequenceNumber;
    in_info.timestamp = ldac_decoder_frame_p->timestamp;
    in_info.curSubSequenceNumber = ldac_decoder_frame_p->curSubSequenceNumber;
    in_info.totalSubSequenceNumber =
        ldac_decoder_frame_p->totalSubSequenceNumber;
    ret = a2dp_cp_put_in_frame(&in_info, sizeof(in_info),
                               ldac_decoder_frame_p->buffer,
                               ldac_decoder_frame_p->buffer_len);

    if (ret) {
      // TRACE(2,"%s  piff  !!!!!!ret: %d ",__func__, ret);
      break;
    }
    // count++;
    // TRACE(2,"%s  count  !!!!!!: %d ",__func__,count);
    // TRACE(3,"put seq:%d %d
    // %d",in_info.sequenceNumber,in_info.curSubSequenceNumber,in_info.totalSubSequenceNumber);
    a2dp_audio_list_remove(list, ldac_decoder_frame_p);
  }

  ret = a2dp_cp_get_full_out_frame((void **)&out, &out_len);

  if (ret) {
    TRACE(0, "%s %d cp find cache underflow", __func__, __LINE__);
    TRACE(2, "aud_list_len:%d. cp_get_in_frame:%d",
          a2dp_audio_list_length(list), get_in_cp_frame_cnt());
    a2dp_cp_ldac_after_cache_underflow();
    return A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
  }

  if (out_len == 0) {
    memset(buffer, 0, buffer_bytes);
    a2dp_cp_consume_full_out_frame();
    TRACE(1, "%s  olz!!!", __func__);
    return A2DP_DECODER_NO_ERROR;
  }
  if (out_len != out_frame_len) {
    TRACE(3, "%s: Bad out len %u (should be %u)", __func__, out_len,
          out_frame_len);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }
  p_out_info = (struct A2DP_CP_LDAC_OUT_FRM_INFO_T *)out;
  if (p_out_info->pcm_len) {
    a2dp_audio_ldac_lastframe_info.sequenceNumber =
        p_out_info->in_info.sequenceNumber;
    a2dp_audio_ldac_lastframe_info.timestamp = p_out_info->in_info.timestamp;
    a2dp_audio_ldac_lastframe_info.curSubSequenceNumber =
        p_out_info->in_info.curSubSequenceNumber;
    a2dp_audio_ldac_lastframe_info.totalSubSequenceNumber =
        p_out_info->in_info.totalSubSequenceNumber;
    a2dp_audio_ldac_lastframe_info.frame_samples = p_out_info->frame_samples;
    a2dp_audio_ldac_lastframe_info.decoded_frames += p_out_info->decoded_frames;
    a2dp_audio_ldac_lastframe_info.undecode_frames =
        a2dp_audio_list_length(list) +
        a2dp_cp_get_in_frame_cnt_by_index(p_out_info->frame_idx) - 1;
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_ldac_lastframe_info);
  }

  if (p_out_info->pcm_len == buffer_bytes) {
    memcpy(buffer, p_out_info + 1, p_out_info->pcm_len);
    dec_ret = A2DP_DECODER_NO_ERROR;
  } else {
    TRACE(2, "%s  %d cp decoder error  !!!!!!", __func__, __LINE__);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }

  ret = a2dp_cp_consume_full_out_frame();
  if (ret) {

    TRACE(2, "%s: a2dp_cp_consume_full_out_frame() failed: ret=%d", __func__,
          ret);
    set_cp_reset_flag(true);
    return A2DP_DECODER_DECODE_ERROR;
  }
  return dec_ret;
}

TEXT_LDAC_LOC
int a2dp_cp_ldac_cp_decode(void) {
  int ret;
  enum CP_EMPTY_OUT_FRM_T out_frm_st;
  uint8_t *out;
  uint32_t out_len;
  uint8_t *dec_start;
  uint32_t dec_len;
  struct A2DP_CP_LDAC_IN_FRM_INFO_T *p_in_info;
  struct A2DP_CP_LDAC_OUT_FRM_INFO_T *p_out_info;
  uint8_t *in_buf;
  uint32_t in_len;

  int32_t dec_sum;

  int used_bytes = 0;
  int wrote_bytes = 0;

  out_frm_st = a2dp_cp_get_emtpy_out_frame((void **)&out, &out_len);

  if (out_frm_st != CP_EMPTY_OUT_FRM_OK &&
      out_frm_st != CP_EMPTY_OUT_FRM_WORKING) {
    return out_frm_st;
  }

  ASSERT(out_len > sizeof(*p_out_info), "%s: Bad out_len %u (should > %u)",
         __func__, out_len, sizeof(*p_out_info));

  p_out_info = (struct A2DP_CP_LDAC_OUT_FRM_INFO_T *)out;
  if (out_frm_st == CP_EMPTY_OUT_FRM_OK) {
    p_out_info->pcm_len = 0;
    p_out_info->decoded_frames = 0;
  }

  ASSERT(out_len > sizeof(*p_out_info) + p_out_info->pcm_len,
         "%s: Bad out_len %u (should > %u + %u)", __func__, out_len,
         sizeof(*p_out_info), p_out_info->pcm_len);

  dec_start = (uint8_t *)(p_out_info + 1) + p_out_info->pcm_len;
  dec_len = out_len - (dec_start - (uint8_t *)out);

  dec_sum = 0;

  while (dec_sum < (int32_t)dec_len) {
    ret = a2dp_cp_get_in_frame((void **)&in_buf, &in_len);

    if (ret) {
      TRACE(1, "cp_get_int_frame fail, ret=%d", ret);
      return 4;
    }

    ASSERT(in_len > sizeof(*p_in_info), "%s: Bad in_len %u (should > %u)",
           __func__, in_len, sizeof(*p_in_info));

    p_in_info = (struct A2DP_CP_LDAC_IN_FRM_INFO_T *)in_buf;
    in_buf += sizeof(*p_in_info);
    in_len -= sizeof(*p_in_info);
    // TRACE(2,"decode:seq %d %d %d", p_in_info->sequenceNumber,
    // p_in_info->curSubSequenceNumber,p_in_info->totalSubSequenceNumber);

    if (in_buf[0] != 0xaa) {
      TRACE(2, "decode:seq %d %d", p_in_info->sequenceNumber,
            p_in_info->curSubSequenceNumber);
      DUMP8("%x ", in_buf, 30);
    }

    ret = ldacBT_decode(LdacDecHandle, in_buf, dec_start + dec_sum,
                        LDACBT_SMPL_FMT_S16, in_len, &used_bytes, &wrote_bytes);
    // TRACE("%s wb:%d bb:%d pb:%d",__func__,wrote_bytes,dec_len,dec_sum);
    dec_sum += wrote_bytes;
    if (ret != 0) {
      TRACE(1, "ldac decode error %d", ret);
      TRACE(1, "LdacDecHandle error_code:%d",
            ldacBT_get_error_code(LdacDecHandle));
      ret = A2DP_DECODER_DECODE_ERROR;
      return -1;
    }

    ret = a2dp_cp_consume_in_frame();

    if (ret != 0) {
      TRACE(2, "%s: a2dp_cp_consume_in_frame() failed: ret=%d", __func__, ret);
    }
    ASSERT(ret == 0, "%s: a2dp_cp_consume_in_frame() failed: ret=%d", __func__,
           ret);

    memcpy(&p_out_info->in_info, p_in_info, sizeof(*p_in_info));
    p_out_info->decoded_frames++;
    p_out_info->frame_samples = 256;
    p_out_info->frame_idx = a2dp_cp_get_in_frame_index();
  }

  if (dec_sum != (int32_t)dec_len) {
    TRACE(2, "error!!! dec_sum:%d  != dec_len:%d", dec_sum, dec_len);
    ASSERT(0, "%s", __func__);
  }

  p_out_info->pcm_len += dec_sum;

  if (out_len <= sizeof(*p_out_info) + p_out_info->pcm_len) {
    ret = a2dp_cp_consume_emtpy_out_frame();
    ASSERT(ret == 0, "%s: a2dp_cp_consume_emtpy_out_frame() failed: ret=%d",
           __func__, ret);
  }

  return 0;
}

#endif

int a2dp_audio_ldac_mcu_decode_frame(uint8_t *buffer, uint32_t buffer_bytes) {

  list_node_t *node = NULL;
  uint8_t *temp_buf_ptr1 = NULL;
  uint8_t *temp_buf_ptr2 = NULL;
  uint16_t pcm_output_bytes;
  uint32_t lock;

  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;
  int used_bytes = 0;
  int wrote_bytes = 0;
  bool cache_underflow = false;
  int output_count = 0;
  int result = 0;

  if ((buffer_bytes < DECODE_LDAC_PCM_FRAME_LENGTH &&
       (LDACBT_CHANNEL_MODE_MONO != bt_ldac_player_get_channelmode())) ||
      (buffer_bytes < DECODE_LDAC_PCM_FRAME_LENGTH / 2 &&
       (LDACBT_CHANNEL_MODE_MONO == bt_ldac_player_get_channelmode()))) {
    TRACE(1, "ldac_decode pcm_len = %d \n", buffer_bytes);
    return A2DP_DECODER_NO_ERROR;
  }

  if (!LdacDecHandle) {
    TRACE(0, "ldac decode not ready");
    return A2DP_DECODER_NO_ERROR;
  }

  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

  // TRACE("jtx~~");

  for (pcm_output_bytes = 0; pcm_output_bytes < buffer_bytes;
       pcm_output_bytes += wrote_bytes) {
    node = a2dp_audio_list_begin(list);
    if (!node) {
      TRACE(0, "ldac decode cache underflow !");
      cache_underflow = true;
      goto exit;
    } else {
      ldac_decoder_frame_p =
          (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
      temp_buf_ptr1 = ldac_decoder_frame_p->buffer;
      temp_buf_ptr2 = buffer + wrote_bytes * output_count;

      if (temp_buf_ptr1[0] != 0xaa) {
        TRACE(2, "decode:seq %d %d", ldac_decoder_frame_p->sequenceNumber,
              ldac_decoder_frame_p->curSubSequenceNumber);
        DUMP8("%x ", temp_buf_ptr1, 30);
      }

      lock = int_lock();
      result = ldacBT_decode(
          LdacDecHandle, temp_buf_ptr1, temp_buf_ptr2, LDACBT_SMPL_FMT_S16,
          ldac_decoder_frame_p->buffer_len, &used_bytes, &wrote_bytes);
      output_count++;
      int_unlock(lock);

      //  TRACE("%s wb:%d bb:%d
      //  pb:%d",__func__,wrote_bytes,buffer_bytes,pcm_output_bytes);

      // DUMP8("%x ",temp_buf_ptr2,10);

      if (result != 0) {
        output_count = 0;
        TRACE(4, "%s wb:%d bb:%d pb:%d", __func__, wrote_bytes, buffer_bytes,
              pcm_output_bytes);
        TRACE(2, "decode:seq %d %d", ldac_decoder_frame_p->sequenceNumber,
              ldac_decoder_frame_p->curSubSequenceNumber);
        DUMP8("%x ", temp_buf_ptr1, 10);
        TRACE(1, "ldac decode error %d", result);
        result = A2DP_DECODER_DECODE_ERROR;
        goto exit;
      }
    }
    a2dp_audio_ldac_lastframe_info.sequenceNumber =
        ldac_decoder_frame_p->sequenceNumber;
    a2dp_audio_ldac_lastframe_info.timestamp = ldac_decoder_frame_p->timestamp;
    a2dp_audio_ldac_lastframe_info.curSubSequenceNumber =
        ldac_decoder_frame_p->curSubSequenceNumber;
    a2dp_audio_ldac_lastframe_info.totalSubSequenceNumber =
        ldac_decoder_frame_p->totalSubSequenceNumber;
    a2dp_audio_ldac_lastframe_info.frame_samples =
        ldac_decoder_frame_p->frame_samples;
    a2dp_audio_ldac_lastframe_info.decoded_frames++;
    a2dp_audio_ldac_lastframe_info.undecode_frames =
        a2dp_audio_list_length(list) - 1;
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_ldac_lastframe_info);
    a2dp_audio_list_remove(list, ldac_decoder_frame_p);
  }

exit:
  if (cache_underflow) {
    a2dp_audio_ldac_lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_internal_lastframe_info_set(
        &a2dp_audio_ldac_lastframe_info);
    result = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
  }
  return result;
}

int a2dp_audio_ldac_decode_frame(uint8_t *buffer, uint32_t buffer_bytes) {
  int ret = 0;
  if (bt_ldac_player_get_channelmode() == LDACBT_CHANNEL_MODE_MONO) {
#ifdef A2DP_CP_ACCEL
    ret = a2dp_cp_ldac_mcu_decode(buffer, buffer_bytes / sizeof(int16_t));
#else
    ret = a2dp_audio_ldac_mcu_decode_frame(buffer,
                                           buffer_bytes / sizeof(int16_t));
#endif
    int16_t *out_int16 = (int16_t *)buffer;
    int16_t wrote_samples = buffer_bytes / sizeof(int16_t);
    for (int32_t i = wrote_samples - 1; i >= 0; i--) {
      out_int16[2 * i + 1] = out_int16[i];
      out_int16[2 * i] = out_int16[i];
    }
  } else
#ifdef A2DP_CP_ACCEL
    ret = a2dp_cp_ldac_mcu_decode(buffer, buffer_bytes);
#else
    ret = a2dp_audio_ldac_mcu_decode_frame(buffer, buffer_bytes);
#endif
  return ret;
}

int a2dp_audio_ldac_preparse_packet(btif_media_header_t *header,
                                    uint8_t *buffer, uint32_t buffer_bytes) {
  a2dp_audio_ldac_lastframe_info.sequenceNumber = header->sequenceNumber;
  a2dp_audio_ldac_lastframe_info.timestamp = header->timestamp;
  a2dp_audio_ldac_lastframe_info.curSubSequenceNumber = 0;
  a2dp_audio_ldac_lastframe_info.totalSubSequenceNumber = 0;
  a2dp_audio_ldac_lastframe_info.frame_samples = LDAC_LIST_SAMPLES;
  a2dp_audio_ldac_lastframe_info.list_samples = LDAC_LIST_SAMPLES;
  a2dp_audio_ldac_lastframe_info.decoded_frames = 0;
  a2dp_audio_ldac_lastframe_info.undecode_frames = 0;
  a2dp_audio_decoder_internal_lastframe_info_set(
      &a2dp_audio_ldac_lastframe_info);

  TRACE(4, "%s seq:%d timestamp:%d frame samples:%d", __func__,
        header->sequenceNumber, header->timestamp,
        a2dp_audio_ldac_lastframe_info.frame_samples);

  return A2DP_DECODER_NO_ERROR;
}

static void *a2dp_audio_ldac_frame_malloc(uint32_t packet_len) {
  a2dp_audio_ldac_decoder_frame_t *decoder_frame_p = NULL;
  uint8_t *buffer = NULL;

  buffer = (uint8_t *)a2dp_audio_heap_malloc(packet_len);
  decoder_frame_p = (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_heap_malloc(
      sizeof(a2dp_audio_ldac_decoder_frame_t));
  decoder_frame_p->buffer = buffer;
  decoder_frame_p->buffer_len = packet_len;
  return (void *)decoder_frame_p;
}

void a2dp_audio_ldac_free(void *packet) {
  a2dp_audio_ldac_decoder_frame_t *decoder_frame_p =
      (a2dp_audio_ldac_decoder_frame_t *)packet;
  a2dp_audio_heap_free(decoder_frame_p->buffer);
  a2dp_audio_heap_free(decoder_frame_p);
}

int a2dp_audio_ldac_header_parser(btif_media_header_t *header,
                                  uint32_t frame_num) {
  return 0;
}

int a2dp_audio_ldac_store_packet(btif_media_header_t *header, uint8_t *buffer,
                                 uint32_t buffer_bytes) {

  int nRet = A2DP_DECODER_NOT_SUPPORT;
  int check_header_status = 0;

  uint32_t frame_cnt = 0;
  uint32_t frame_num = 0;
  uint32_t frame_len = 0;
  uint32_t frame_len_with_head = 0;
  uint16_t data1 = 0;
  uint16_t data2 = 0;

  buffer++;
  buffer_bytes--;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

  // TRACE("buffer:%x %x %x %x %x %x
  // %x",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6]);
  // TRACE(1,"buffer:%x",buffer[2]);

  data1 = (uint16_t)(buffer[1] & 0x07);
  data2 = (uint16_t)buffer[2];
  frame_len = ((data1 << 6 & 0xFFFF) | (data2 >> 2 & 0xFFFF));
  // TRACE("#frame len:%d",frame_len);
  frame_num = get_ldac_frame_num_by_rawdata(buffer, buffer_bytes);

  if ((a2dp_audio_list_length(list) + frame_num) < ldac_mtu_limiter) {
    for (uint32_t i = 0; i < buffer_bytes;
         i += frame_len_with_head, frame_cnt++) {

      // TRACE(4,"buffer:%x %x %x %x
      // ",buffer[i],buffer[i+1],buffer[i+2],buffer[i+3]);
      data1 = (uint16_t)(buffer[i + 1] & 0x07);
      data2 = (uint16_t)buffer[i + 2];
      frame_len = ((data1 << 6 & 0xFFFF) | (data2 >> 2 & 0xFFFF));
      // TRACE("#frame len:%d",frame_len);
      // frame_num = get_ldac_frame_num(frame_len);
      // if (!frame_num)
      // {
      //     TRACE(1,"ERROR LDAC FRAME !!! frame_num:%d", frame_num);
      //     DUMP8("%02x ", buffer, 4);
      //     return A2DP_DECODER_DECODE_ERROR;
      // }
      // for ldac head
      frame_len_with_head = frame_len + 4;
      // TRACE(2,"frame len:%d num:%d
      // %d",frame_len,frame_num,a2dp_audio_list_length(list));
      l2cap_frame_samples = 256 * frame_num;

      // TRACE("i %d buffer bytes:%d",i,buffer_bytes);
      check_header_status = check_ldac_header(buffer + i, frame_len_with_head);
      if (!check_header_status) {
        a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p =
            (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_ldac_frame_malloc(
                frame_len_with_head);

        ldac_decoder_frame_p->sequenceNumber = header->sequenceNumber;
        ldac_decoder_frame_p->curSubSequenceNumber = frame_cnt;
        ldac_decoder_frame_p->totalSubSequenceNumber = frame_num;
        ldac_decoder_frame_p->timestamp = header->timestamp;
        ldac_decoder_frame_p->buffer_len = frame_len_with_head;
        ldac_decoder_frame_p->frame_samples = 256;
        memcpy(ldac_decoder_frame_p->buffer, buffer + i, frame_len_with_head);
        // TRACE(5,"seq:%d len:%d i:%d buffer bytes:%d
        // data:%x",header->sequenceNumber,
        // frame_len,i,buffer_bytes,ldac_decoder_frame_p->buffer[0]);
        // TRACE(5,"store seq:%d %d
        // %d",header->sequenceNumber,ldac_decoder_frame_p->curSubSequenceNumber,ldac_decoder_frame_p->totalSubSequenceNumber);
        a2dp_audio_list_append(list, ldac_decoder_frame_p);
      } else {
        TRACE(1, "ERROR LDAC FRAME ret:%d", check_header_status);
        DUMP8("%02x ", buffer + i, 6);
        break;
      }
      nRet = A2DP_DECODER_NO_ERROR;
    }

  } else {

#if 0
        a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;
        do
        {
            ldac_decoder_frame_p = (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_back(list);
            TRACE(3,"remov seq:%d %d %d",ldac_decoder_frame_p->sequenceNumber,ldac_decoder_frame_p->curSubSequenceNumber,ldac_decoder_frame_p->totalSubSequenceNumber);
            a2dp_audio_list_remove(list, ldac_decoder_frame_p);
        }
        while(ldac_decoder_frame_p->curSubSequenceNumber!=0);
#endif
    TRACE(3, "%s list full current list_len step1:%d buff_len:%d", __func__,
          a2dp_audio_list_length(list), buffer_bytes);
    nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
  }

  return nRet;
}

int a2dp_audio_ldac_discards_packet(uint32_t packets) {
  int nRet = A2DP_DECODER_MEMORY_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;
#ifdef A2DP_CP_ACCEL
  a2dp_cp_reset_frame();
#endif
  if (packets <= a2dp_audio_list_length(list)) {
    for (uint8_t i = 0; i < packets; i++) {
      node = a2dp_audio_list_begin(list);
      ldac_decoder_frame_p =
          (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
      a2dp_audio_list_remove(list, ldac_decoder_frame_p);
    }
    nRet = A2DP_DECODER_NO_ERROR;
  }
  TRACE(3, "%s packets:%d nRet:%d", __func__, packets, nRet);
  return nRet;
}

int a2dp_audio_ldac_info_get(void *info) { return A2DP_DECODER_NO_ERROR; }

static int a2dp_audio_ldac_decoder_init(void) {

  int sample_rate = bt_get_ladc_sample_rate();
  int channel_mode = bt_ldac_player_get_channelmode();
  if (LdacDecHandle == NULL) {
    if ((LdacDecHandle = ldacBT_get_handle()) == (HANDLE_LDAC_BT)NULL) {
      TRACE(0, "Error: Can not Get LDAC Handle!\n");
      return 1;
    }
  }

  TRACE(2, "a2dp_audio_ldac_decoder_init sample Rate=%d, channel_mode = %d\n",
        sample_rate, channel_mode);
  // TRACE(1,"sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(0));
  TRACE(0, "ldac need init here!!!! \n");
  if ((LdacDecHandle = ldacBT_get_handle()) == (HANDLE_LDAC_BT)NULL) {
    TRACE(0, "Error: Can not Get LDAC Handle!\n");
    return 1;
  }
  int result = ldacBT_init_handle_decode(LdacDecHandle, channel_mode,
                                         sample_rate, 0, 0, 0);
  if (result) {
    TRACE(1, "[ERR] Initializing LDAC Handle for synthesis! Error code %s\n",
          get_error_code_string(ldacBT_get_error_code(LdacDecHandle)));
    return 2;
  }
  return 0;
}

static void a2dp_audio_ldac_decoder_deinit(void) {
  if (LdacDecHandle != NULL) {
    ldacBT_free_handle(LdacDecHandle);
    LdacDecHandle = NULL;
  }
}

static int a2dp_audio_ldac_list_checker(void) {

  // return 0;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;
  int cnt = 0;

  do {

    // TRACE(1,"cnt:%d",cnt);
    ldac_decoder_frame_p =
        (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_ldac_frame_malloc(216 +
                                                                        4);
    if (ldac_decoder_frame_p) {
      a2dp_audio_list_append(list, ldac_decoder_frame_p);
    }
    cnt++;
  } while (ldac_decoder_frame_p && cnt < LDAC_MTU_LIMITER);

  do {
    node = a2dp_audio_list_begin(list);
    if (node) {
      ldac_decoder_frame_p =
          (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
      a2dp_audio_list_remove(list, ldac_decoder_frame_p);
    }
  } while (node);

  TRACE(3, "%s cnt:%d list:%d", __func__, cnt, a2dp_audio_list_length(list));

  return 0;
}

int a2dp_audio_ldac_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context) {
  TRACE(1, "%s", __func__);
  a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

  memset(&a2dp_audio_ldac_lastframe_info, 0,
         sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));
  a2dp_audio_ldac_lastframe_info.stream_info = *config;
  a2dp_audio_ldac_lastframe_info.frame_samples = 256;
  a2dp_audio_ldac_lastframe_info.list_samples = 256;
  a2dp_audio_decoder_internal_lastframe_info_set(
      &a2dp_audio_ldac_lastframe_info);

  ASSERT(a2dp_audio_context_p->dest_packet_mut < LDAC_MTU_LIMITER,
         "%s MTU OVERFLOW:%u/%u", __func__,
         a2dp_audio_context_p->dest_packet_mut, LDAC_MTU_LIMITER);

#ifdef A2DP_CP_ACCEL
  int ret;
  ret = a2dp_cp_init(a2dp_cp_ldac_cp_decode, CP_PROC_DELAY_2_FRAMES);
  ASSERT(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__, ret);
#endif
  a2dp_audio_ldac_decoder_init();
  a2dp_audio_ldac_list_checker();

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_ldac_deinit(void) {

#ifdef A2DP_CP_ACCEL
  a2dp_cp_deinit();
#endif
  a2dp_audio_ldac_decoder_deinit();
  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_ldac_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info,
                                       uint32_t mask) {
  int nRet = A2DP_DECODER_SYNC_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  int list_len;
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;

  list_len = a2dp_audio_list_length(list);

  for (uint16_t i = 0; i < list_len; i++) {
    node = a2dp_audio_list_begin(list);
    ldac_decoder_frame_p =
        (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
    //  if (A2DP_AUDIO_SYNCFRAME_CHK(ldac_decoder_frame_p->sequenceNumber  ==
    //  sync_info->sequenceNumber, A2DP_AUDIO_SYNCFRAME_MASK_SEQ,       mask)&&
    //      A2DP_AUDIO_SYNCFRAME_CHK(ldac_decoder_frame_p->timestamp       ==
    //      sync_info->timestamp,      A2DP_AUDIO_SYNCFRAME_MASK_TIMESTAMP,
    //      mask))

    if (A2DP_AUDIO_SYNCFRAME_CHK(ldac_decoder_frame_p->sequenceNumber ==
                                     sync_info->sequenceNumber,
                                 A2DP_AUDIO_SYNCFRAME_MASK_SEQ, mask) &&
        A2DP_AUDIO_SYNCFRAME_CHK(ldac_decoder_frame_p->curSubSequenceNumber ==
                                     sync_info->curSubSequenceNumber,
                                 A2DP_AUDIO_SYNCFRAME_MASK_CURRSUBSEQ, mask) &&
        A2DP_AUDIO_SYNCFRAME_CHK(ldac_decoder_frame_p->totalSubSequenceNumber ==
                                     sync_info->totalSubSequenceNumber,
                                 A2DP_AUDIO_SYNCFRAME_MASK_TOTALSUBSEQ, mask)) {
      nRet = A2DP_DECODER_NO_ERROR;
      break;
    }
    a2dp_audio_list_remove(list, ldac_decoder_frame_p);
  }

  node = a2dp_audio_list_begin(list);
  if (node) {
    ldac_decoder_frame_p =
        (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
    TRACE(4, "%s nRet:%d SEQ:%d timestamp:%d", __func__, nRet,
          ldac_decoder_frame_p->sequenceNumber,
          ldac_decoder_frame_p->timestamp);
  } else {
    TRACE(2, "%s nRet:%d", __func__, nRet);
    // TRACE(5,"nRet:%d SEQ:%d timestamp:%d sync %d/%d", nRet,
    // ldac_decoder_frame_p->sequenceNumber,
    // ldac_decoder_frame_p->timestamp,sync_info->sequenceNumber,sync_info->timestamp);
  }

  return nRet;
}

int a2dp_audio_ldac_synchronize_dest_packet_mut(uint16_t packet_mut) {
  list_node_t *node = NULL;
  uint32_t list_len = 0;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame_p = NULL;

  list_len = a2dp_audio_list_length(list);
  if (list_len > packet_mut) {
    do {
      node = a2dp_audio_list_begin(list);
      ldac_decoder_frame_p =
          (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
      a2dp_audio_list_remove(list, ldac_decoder_frame_p);
    } while (a2dp_audio_list_length(list) > packet_mut);
  }

  TRACE(2, "%s list:%d", __func__, a2dp_audio_list_length(list));
  return A2DP_DECODER_NO_ERROR;
}

static int a2dp_audio_ldac_headframe_info_get(
    A2DP_AUDIO_HEADFRAME_INFO_T *headframe_info) {
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  list_node_t *node = NULL;
  a2dp_audio_ldac_decoder_frame_t *decoder_frame_p = NULL;

  if (a2dp_audio_list_length(list)) {
    node = a2dp_audio_list_begin(list);
    decoder_frame_p =
        (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);
    headframe_info->sequenceNumber = decoder_frame_p->sequenceNumber;
    headframe_info->timestamp = decoder_frame_p->timestamp;
    headframe_info->curSubSequenceNumber =
        decoder_frame_p->curSubSequenceNumber;
    headframe_info->totalSubSequenceNumber =
        decoder_frame_p->totalSubSequenceNumber;
  } else {
    memset(headframe_info, 0, sizeof(A2DP_AUDIO_HEADFRAME_INFO_T));
  }

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_ldac_convert_list_to_samples(uint32_t *samples) {
  uint32_t list_len = 0;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

  list_len = a2dp_audio_list_length(list);
  *samples = LDAC_LIST_SAMPLES * list_len;

  TRACE(3, "%s list:%d samples:%d", __func__, list_len, *samples);

  return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_ldac_discards_samples(uint32_t samples) {
  int nRet = A2DP_DECODER_SYNC_ERROR;
  list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
  a2dp_audio_ldac_decoder_frame_t *ldac_decoder_frame = NULL;
  list_node_t *node = NULL;
  int need_remove_list = 0;
  uint32_t list_samples = 0;
  ASSERT(!(samples % LDAC_LIST_SAMPLES), "%s samples err:%d", __func__,
         samples);

  a2dp_audio_ldac_convert_list_to_samples(&list_samples);
  if (list_samples >= samples) {
    need_remove_list = samples / LDAC_LIST_SAMPLES;
    for (int i = 0; i < need_remove_list; i++) {
      node = a2dp_audio_list_begin(list);
      ldac_decoder_frame =
          (a2dp_audio_ldac_decoder_frame_t *)a2dp_audio_list_node(node);

      TRACE(1, "discard seq:%d %d %d", ldac_decoder_frame->sequenceNumber,
            ldac_decoder_frame->curSubSequenceNumber,
            ldac_decoder_frame->totalSubSequenceNumber);
      a2dp_audio_list_remove(list, ldac_decoder_frame);
    }
    nRet = A2DP_DECODER_NO_ERROR;
  }

  return nRet;
}

A2DP_AUDIO_DECODER_T a2dp_audio_ldac_decoder_config = {
    {96000, 2, 16},
    0,
    a2dp_audio_ldac_init,
    a2dp_audio_ldac_deinit,
    a2dp_audio_ldac_decode_frame,
    a2dp_audio_ldac_preparse_packet,
    a2dp_audio_ldac_store_packet,
    a2dp_audio_ldac_discards_packet,
    a2dp_audio_ldac_synchronize_packet,
    a2dp_audio_ldac_synchronize_dest_packet_mut,
    a2dp_audio_ldac_convert_list_to_samples,
    a2dp_audio_ldac_discards_samples,
    a2dp_audio_ldac_headframe_info_get,
    a2dp_audio_ldac_info_get,
    a2dp_audio_ldac_free,
};
