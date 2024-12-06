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
#include "app_audio.h"
#include "app_overlay.h"
#include "app_ring_merge.h"
#include "audio_prompt_sbc.h"
#include "bt_sco_chain.h"
#include "cmsis_os.h"
#include "cqueue.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "hfp_api.h"
#include "iir_resample.h"
#include "plat_types.h"
#include "tgt_hardware.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TX_RX_PCM_MASK
#include "bt_drv_interface.h"
#endif

#define ENABLE_LPC_PLC

#define ENABLE_PLC_ENCODER

// BT
#include "a2dp_api.h"
#include "plc_utils.h"
extern "C" {

#include "plc_8000.h"
#include "speech_utils.h"
#if defined(HFP_1_6_ENABLE)
#include "codec_sbc.h"
#ifndef ENABLE_LPC_PLC
#include "plc_16000.h"
#endif
#endif
#if defined(CVSD_BYPASS)
#include "Pcm8k_Cvsd.h"
#endif
#ifndef ENABLE_LPC_PLC
static void *speech_plc;
#endif
}

#if defined(ENABLE_LPC_PLC)
#include "lpc_plc_api.h"
#endif

#if defined(SPEECH_TX_24BIT)
extern int32_t *aec_echo_buf;
#else
extern short *aec_echo_buf;
#endif

// #define SPEECH_RX_PLC_DUMP_DATA

// #define PLC_DEBUG_PRINT_DATA

//#define DEBUG_SCO_DUMP

#ifdef SPEECH_RX_PLC_DUMP_DATA
#include "audio_dump.h"
int16_t *audio_dump_temp_buf = NULL;
#endif

// app_bt_stream.cpp::bt_sco_player(), used buffer size
#define APP_BT_STREAM_USE_BUF_SIZE (1024 * 2)
#if defined(SCO_OPTIMIZE_FOR_RAM)
uint8_t *sco_overlay_ram_buf = NULL;
int sco_overlay_ram_buf_len = 0;
#endif

static bool resample_needed_flag = false;
static int sco_frame_length;
static int codec_frame_length;
static int16_t *resample_buf = NULL;
static IirResampleState *uplink_resample_st = NULL;
static IirResampleState *downlink_resample_st = NULL;

#define MSBC_FRAME_SIZE (60)

#if defined(HFP_1_6_ENABLE)
static btif_sbc_decoder_t *msbc_decoder;
static float msbc_eq_band_gain[CFG_HW_AUD_EQ_NUM_BANDS] = {0, 0, 0, 0,
                                                           0, 0, 0, 0};

#define MSBC_ENCODE_PCM_LEN (240)

#ifndef ENABLE_LPC_PLC
struct PLC_State msbc_plc_state;
#endif

#ifdef ENABLE_PLC_ENCODER
static btif_sbc_encoder_t *msbc_plc_encoder;
static int16_t *msbc_plc_encoder_buffer = NULL;
#define MSBC_CODEC_DELAY (73)
#endif

static btif_sbc_encoder_t *msbc_encoder;
#endif

#if defined(ENABLE_LPC_PLC)
LpcPlcState *msbc_plc_state = NULL;
#endif

#define VOICEBTPCM_TRACE(s, ...)
// TRACE(s, ##__VA_ARGS__)

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) ||                        \
    defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) ||                      \
    defined(CHIP_BEST2001)
#define MSBC_MUTE_PATTERN (0x55)
#else
#define MSBC_MUTE_PATTERN (0x00)
#endif
#define MSBC_LEN_FORMBT_PER_FRAME (60) // Bytes; only for BES platform.
#define SAMPLES_LEN_PER_FRAME (120)
#define MSBC_LEN_PER_FRAME (57 + 3)
#define BYTES_PER_PCM_FRAME (SAMPLES_LEN_PER_FRAME * 2)

// Add 1 to ensure it never be out of bounds when msbc_offset is 1
unsigned char msbc_buf_all[MSBC_LEN_FORMBT_PER_FRAME * 3 + 1];
#if defined(HFP_1_6_ENABLE)
static int msbc_find_first_sync = 0;
static unsigned int msbc_offset = 0;
static unsigned int next_frame_flag = 0;
#endif
static PacketLossState pld;

extern bool bt_sco_codec_is_msbc(void);

int process_downlink_msbc_frames(unsigned char *msbc_buf, unsigned int msbc_len,
                                 unsigned char *pcm_buf, unsigned int pcm_len);
int process_downlink_cvsd_frames(unsigned char *cvsd_buf, unsigned int cvsd_len,
                                 unsigned char *pcm_buf, unsigned int pcm_len);
int process_uplink_msbc_frames(unsigned char *pcm_buf, unsigned int pcm_len,
                               unsigned char *msbc_buf, unsigned int msbc_len);
int process_uplink_cvsd_frames(unsigned char *pcm_buf, unsigned int pcm_len,
                               unsigned char *cvsd_buf, unsigned int cvsd_len);

int process_downlink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len,
                                     uint8_t *out_buf, uint32_t out_len,
                                     int32_t codec_type) {
  // TRACE(3,"[%s] in_len = %d, out_len = %d", __FUNCTION__, in_len, out_len);

#if defined(SPEECH_RX_24BIT)
  out_len /= 2;
#endif

  int16_t *pcm_buf = (int16_t *)out_buf;
  int pcm_len = out_len / sizeof(int16_t);

  if (resample_needed_flag == true) {
    pcm_buf = resample_buf;
    pcm_len = sco_frame_length;
  }

  if (bt_sco_codec_is_msbc()) {
    process_downlink_msbc_frames(in_buf, in_len, (uint8_t *)pcm_buf,
                                 pcm_len * sizeof(int16_t));

    // Down sampling
    if (resample_needed_flag) {
      iir_resample_process(downlink_resample_st, pcm_buf, (int16_t *)out_buf,
                           pcm_len);
      pcm_buf = (int16_t *)out_buf;
      pcm_len >>= 1;
    }
  } else {
    process_downlink_cvsd_frames(in_buf, in_len, (uint8_t *)pcm_buf,
                                 pcm_len * sizeof(int16_t));

    // Up sampling
    if (resample_needed_flag) {
      iir_resample_process(downlink_resample_st, pcm_buf, (int16_t *)out_buf,
                           pcm_len);
      pcm_buf = (int16_t *)out_buf;
      pcm_len <<= 1;
    }
  }

#if defined(SPEECH_RX_24BIT)
  int32_t *buf32 = (int32_t *)out_buf;
  for (int i = pcm_len - 1; i >= 0; i--) {
    buf32[i] = ((int32_t)pcm_buf[i] << 8);
  }
#endif

  speech_rx_process(pcm_buf, &pcm_len);

#if defined(SPEECH_RX_24BIT)
  out_len *= 2;
#endif

  return 0;
}

int process_uplink_bt_voice_frames(uint8_t *in_buf, uint32_t in_len,
                                   uint8_t *ref_buf, uint32_t ref_len,
                                   uint8_t *out_buf, uint32_t out_len,
                                   int32_t codec_type) {
  // TRACE(3,"[%s] in_len = %d, out_len = %d", __FUNCTION__, in_len, out_len);

#if defined(SPEECH_TX_24BIT)
  int32_t *pcm_buf = (int32_t *)in_buf;
  int pcm_len = in_len / sizeof(int32_t);
#else
  int16_t *pcm_buf = (int16_t *)in_buf;
  int pcm_len = in_len / sizeof(int16_t);
#endif

#if defined(SPEECH_TX_AEC_CODEC_REF)
  ASSERT(pcm_len % (SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1) == 0,
         "[%s] pcm_len(%d) should be divided by %d", __FUNCTION__, pcm_len,
         SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1);
  // copy reference buffer
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                       \
    defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT) ||                 \
    defined(SPEECH_TX_THIRDPARTY)
  for (int i = SPEECH_CODEC_CAPTURE_CHANNEL_NUM, j = 0; i < pcm_len;
       i += SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1, j++) {
    aec_echo_buf[j] = pcm_buf[i];
  }
#endif
  for (int i = 0, j = 0; i < pcm_len; i += SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1,
           j += SPEECH_CODEC_CAPTURE_CHANNEL_NUM) {
    for (int k = 0; k < SPEECH_CODEC_CAPTURE_CHANNEL_NUM; k++)
      pcm_buf[j + k] = pcm_buf[i + k];
  }
  pcm_len = pcm_len / (SPEECH_CODEC_CAPTURE_CHANNEL_NUM + 1) *
            SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
#elif (defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) ||                    \
       defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT) ||              \
       defined(SPEECH_TX_THIRDPARTY))
  int ref_pcm_len = ref_len / sizeof(int16_t);
  ASSERT(pcm_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM == ref_pcm_len,
         "[%s] Length error: %d / %d != %d", __func__, pcm_len,
         SPEECH_CODEC_CAPTURE_CHANNEL_NUM, ref_pcm_len);

  for (int i = 0; i < ref_pcm_len; i++) {
    aec_echo_buf[i] = ref_buf[i];
  }
#endif
  speech_tx_process(pcm_buf, aec_echo_buf, &pcm_len);

#if defined(SPEECH_TX_24BIT)
  int32_t *buf24 = (int32_t *)pcm_buf;
  int16_t *buf16 = (int16_t *)pcm_buf;
  for (int i = 0; i < pcm_len; i++)
    buf16[i] = (buf24[i] >> 8);
#endif

  int16_t *pcm_buf_16bits = (int16_t *)pcm_buf;

  if (bt_sco_codec_is_msbc()) {
    // Up sampling
    if (resample_needed_flag) {
      iir_resample_process(uplink_resample_st, (int16_t *)pcm_buf_16bits,
                           resample_buf, pcm_len);
      pcm_buf_16bits = resample_buf;
      pcm_len = sco_frame_length;
    }

    process_uplink_msbc_frames((uint8_t *)pcm_buf_16bits,
                               pcm_len * sizeof(int16_t), out_buf, out_len);
  } else {
    // Down sampling
    if (resample_needed_flag) {
      iir_resample_process(uplink_resample_st, (int16_t *)pcm_buf_16bits,
                           resample_buf, pcm_len);
      pcm_buf_16bits = resample_buf;
      pcm_len = sco_frame_length;
    }

    process_uplink_cvsd_frames((uint8_t *)pcm_buf_16bits,
                               pcm_len * sizeof(int16_t), out_buf, out_len);
  }
  return 0;
}

int process_downlink_msbc_frames(unsigned char *msbc_buf, unsigned int msbc_len,
                                 unsigned char *pcm_buf, unsigned int pcm_len) {
#if defined(HFP_1_6_ENABLE)

  btif_sbc_pcm_data_t pcm_data;
  unsigned int msbc_offset_lowdelay = 0;
  unsigned int i, j;
  unsigned char *msbc_buffer = (unsigned char *)msbc_buf;
  int frame_flag[6]; // 1: good frame; 0:bad frame;
  bt_status_t ret;
  unsigned int frame_counter = 0;
  unsigned short byte_decode = 0;
  unsigned int msbc_offset_total = 0;
  int msbc_offset_drift[6] = {
      0,
  };

  short *dec_pcm_buf = (short *)pcm_buf;
  unsigned char dec_msbc_buf[MSBC_LEN_PER_FRAME] = {
      0,
  };

  // unsigned int timer_begin=hal_sys_timer_get();

  // TRACE(2,"process_downlink_msbc_frames:pcm_len:%d,msbc_len:%d",pcm_len,msbc_len);

  // TRACE(1,"decode_msbc_frame,msbc_len:%d",msbc_len);
  for (i = 0; i < msbc_len; i++) {
    msbc_buf_all[i + MSBC_LEN_FORMBT_PER_FRAME] = msbc_buffer[i];
  }

  /*
  for(i =0; i<msbc_len/2; i=i+10)
  {
     TRACE(10,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,",
     DecMsbcBufAll[i],DecMsbcBufAll[i+1],DecMsbcBufAll[i+2],DecMsbcBufAll[i+3],DecMsbcBufAll[i+4],
     DecMsbcBufAll[i+5],DecMsbcBufAll[i+6],DecMsbcBufAll[i+7],DecMsbcBufAll[i+8],DecMsbcBufAll[i+9]);
  }
  */

  for (j = 0; j < msbc_len / MSBC_LEN_FORMBT_PER_FRAME; j++) {
    frame_flag[j] = 0;
  }

  if (msbc_find_first_sync == 0) {
    for (i = 0; i < msbc_len; i++) {
      if ((msbc_buf_all[i] == 0x01) && ((msbc_buf_all[i + 1] & 0x0f) == 0x08) &&
          (msbc_buf_all[i + 2] == 0xad))
        break;
    }

    TRACE(1, "sync......:%d", i);

    if (i < msbc_len) {
      msbc_find_first_sync = 1;
      msbc_offset = i % MSBC_LEN_FORMBT_PER_FRAME;
      msbc_offset_total = i;
    } else {
      for (j = 0; j < msbc_len / MSBC_LEN_FORMBT_PER_FRAME; j++) {
        frame_flag[j] = 1;
      }
    }
  }

  if (msbc_find_first_sync == 1) {
    int value = 0;
    unsigned char headerm1 = 0;
    unsigned char header0 = 0;
    unsigned char header1 = 0;
    unsigned char header2 = 0;
    unsigned char header3 = 0;
    //        unsigned char tail0 = 0;

    if (msbc_offset == 0 || msbc_offset == 1) {
      msbc_offset_lowdelay = msbc_offset + 60;
      // msbc_offset_lowdelay=msbc_offset;
    } else {
      msbc_offset_lowdelay = msbc_offset;
    }

    // check again
    for (j = 0; j < (msbc_len / MSBC_LEN_FORMBT_PER_FRAME); j++) {
      if (next_frame_flag == 1) {
        next_frame_flag = 0;
        frame_flag[j] = 1;
        continue;
      }
      if (msbc_offset_lowdelay == 0 && j == 0) {
        headerm1 = 0x01;
      } else {
        headerm1 = msbc_buf_all[msbc_offset_lowdelay +
                                j * MSBC_LEN_FORMBT_PER_FRAME - 1];
      }

      header0 =
          msbc_buf_all[msbc_offset_lowdelay + j * MSBC_LEN_FORMBT_PER_FRAME];
      header1 = msbc_buf_all[msbc_offset_lowdelay +
                             j * MSBC_LEN_FORMBT_PER_FRAME + 1];
      header2 = msbc_buf_all[msbc_offset_lowdelay +
                             j * MSBC_LEN_FORMBT_PER_FRAME + 2];
      header3 = msbc_buf_all[msbc_offset_lowdelay +
                             j * MSBC_LEN_FORMBT_PER_FRAME + 3];

      /*if ((headerm1 == 0x01) && ((header0 & 0x0f) == 0x08) && (header1 ==
      0xad) || (header0 == 0x01) && ((header1 & 0x0f) == 0x08) && (header2 ==
      0xad) || (header1 == 0x01) && ((header2 & 0x0f) == 0x08) && (header3 ==
      0xad))
      {
          frame_flag[j] = 0;
      }*/
      if ((headerm1 == 0x01) && ((header0 & 0x0f) == 0x08) &&
          (header1 == 0xad)) {
        frame_flag[j] = 0;
        // It seems that offset is reduced by 1
        msbc_offset_drift[j] = -1;
        TRACE(1, "[%s] msbc_offset is reduced by 1", __FUNCTION__);
        /*
            tail0 = msbc_buf_all[msbc_offset_lowdelay +
           j*MSBC_LEN_FORMBT_PER_FRAME + 59 - 1]; if (tail0 == 0x00 || tail0 ==
           0x01|| tail0==0xff)
            {
                frame_flag[j] = 0;
            }
            else
            {
                frame_flag[j] = 1;
                next_frame_flag = 1;
            }
            */
      } else if ((header0 == 0x01) && ((header1 & 0x0f) == 0x08) &&
                 (header2 == 0xad)) {
        frame_flag[j] = 0;
        /*
            tail0 = msbc_buf_all[msbc_offset_lowdelay +
           j*MSBC_LEN_FORMBT_PER_FRAME + 59]; if (tail0 == 0x00 || tail0 ==
           0x01|| tail0==0xff)
            {
                frame_flag[j] = 0;
            }
            else
            {
                frame_flag[j] = 1;
                next_frame_flag = 1;
            }
            */
      } else if ((header1 == 0x01) && ((header2 & 0x0f) == 0x08) &&
                 (header3 == 0xad)) {
        frame_flag[j] = 0;
        msbc_offset_drift[j] = 1;
        TRACE(1, "[%s] msbc_offset is increased by 1", __FUNCTION__);
        /*
            tail0 = msbc_buf_all[msbc_offset_lowdelay +
           j*MSBC_LEN_FORMBT_PER_FRAME + 59 + 1]; if (tail0 == 0x00 ||
           tail0==0x01|| tail0==0xff)
            {
                frame_flag[j] = 0;
            }
            else
            {
                frame_flag[j] = 1;
                next_frame_flag = 1;
            }
            */
      } else {
        if ((header0 == MSBC_MUTE_PATTERN) &&
            ((header1 & 0x0f) == (MSBC_MUTE_PATTERN & 0x0f)) &&
            (header2 == MSBC_MUTE_PATTERN)) {
          frame_flag[j] = 1;
        } else {
          if ((msbc_offset_lowdelay + j * MSBC_LEN_FORMBT_PER_FRAME) >=
              msbc_offset_total) {
            frame_flag[j] = 3;
          } else {
            frame_flag[j] = 1;
          }
        }
      }
    }

    for (j = 0; j < msbc_len / MSBC_LEN_FORMBT_PER_FRAME; j++) {
      value = value | frame_flag[j];
    }
    // abnormal msbc packet.
    if (value > 1)
      msbc_find_first_sync = 0;
  }

  while ((frame_counter < msbc_len / MSBC_LEN_FORMBT_PER_FRAME) &&
         (frame_counter < pcm_len / BYTES_PER_PCM_FRAME)) {
    // TRACE(3,"[%s] decoding, offset %d, offset drift %d", __FUNCTION__,
    // msbc_offset, msbc_offset_drift[frame_counter]);
    // skip first byte when msbc_offset == 0 and msbc_offset_drift == -1
    unsigned int start_idx = 0;
    if (msbc_offset_lowdelay == 0 && msbc_offset_drift[frame_counter] == -1) {
      start_idx = 1;
      dec_msbc_buf[0] = 0x01;
    }
    for (i = start_idx; i < MSBC_LEN_PER_FRAME; i++) {
      // DecMsbcBuf[i]=DecMsbcBufAll[i+msbc_offset_lowdelay+frame_counter*MSBC_LEN_FORMBT_PER_FRAME+2];
      dec_msbc_buf[i] = msbc_buf_all[i + msbc_offset_lowdelay +
                                     msbc_offset_drift[frame_counter] +
                                     frame_counter * MSBC_LEN_FORMBT_PER_FRAME];
    }

    // TRACE(1,"msbc header:0x%x",DecMsbcBuf[0]);

#ifdef SPEECH_RX_PLC_DUMP_DATA
    audio_dump_add_channel_data(2, (short *)dec_msbc_buf,
                                MSBC_LEN_PER_FRAME / 2);
#endif

    plc_type_t plc_type = packet_loss_detection_process(&pld, dec_msbc_buf);

    if (plc_type != PLC_TYPE_PASS) {
      memset(dec_pcm_buf, 0, SAMPLES_LEN_PER_FRAME * sizeof(int16_t));
      goto do_plc;
    }

    pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
    pcm_data.numChannels = 1;
    pcm_data.dataLen = 0;
    pcm_data.data = (uint8_t *)dec_pcm_buf;

    ret = btif_sbc_decode_frames(msbc_decoder, (unsigned char *)dec_msbc_buf,
                                 MSBC_LEN_PER_FRAME, &byte_decode, &pcm_data,
                                 SAMPLES_LEN_PER_FRAME * 2, msbc_eq_band_gain);

    // ASSERT(ret == BT_STS_SUCCESS, "[%s] msbc decoder should never fail",
    // __FUNCTION__);
    if (ret != BT_STS_SUCCESS) {
      plc_type = PLC_TYPE_DECODER_ERROR;
      packet_loss_detection_update_histogram(&pld, plc_type);
    }

  do_plc:
    if (plc_type == PLC_TYPE_PASS) {
#if defined(ENABLE_LPC_PLC)
      lpc_plc_save(msbc_plc_state, dec_pcm_buf);
#else
      PLC_good_frame(&msbc_plc_state, dec_pcm_buf, dec_pcm_buf);
#endif
#ifdef SPEECH_RX_PLC_DUMP_DATA
      audio_dump_add_channel_data(0, (short *)dec_pcm_buf,
                                  MSBC_ENCODE_PCM_LEN / 2);
#endif
    } else {
      TRACE(1, "PLC bad frame, plc type: %d", plc_type);
#if defined(PLC_DEBUG_PRINT_DATA)
      DUMP8("0x%02x, ", dec_msbc_buf, 60);
#endif

#ifdef SPEECH_RX_PLC_DUMP_DATA
      for (uint32_t i = 0; i < MSBC_ENCODE_PCM_LEN / 2; i++) {
        audio_dump_temp_buf[i] = (plc_type - 1) * 5000;
      }
      audio_dump_add_channel_data(0, audio_dump_temp_buf,
                                  MSBC_ENCODE_PCM_LEN / 2);
#endif
#if defined(ENABLE_LPC_PLC)
      lpc_plc_generate(msbc_plc_state, dec_pcm_buf,
#if defined(ENABLE_PLC_ENCODER)
                       msbc_plc_encoder_buffer
#else
                       NULL
#endif
      );

#if defined(ENABLE_PLC_ENCODER)
      pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
      pcm_data.numChannels = 1;
      pcm_data.dataLen = MSBC_ENCODE_PCM_LEN;
      pcm_data.data = (uint8_t *)(msbc_plc_encoder_buffer + MSBC_CODEC_DELAY);

      btif_plc_update_sbc_decoder_state(msbc_plc_encoder, &pcm_data,
                                        msbc_decoder, msbc_eq_band_gain);
#endif

#else
      pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
      pcm_data.numChannels = 1;
      pcm_data.dataLen = 0;
      pcm_data.data = (uint8_t *)dec_pcm_buf;

      ret =
          btif_sbc_decode_frames(msbc_decoder, (unsigned char *)indices0,
                                 MSBC_LEN_PER_FRAME, &byte_decode, &pcm_data,
                                 SAMPLES_LEN_PER_FRAME * 2, msbc_eq_band_gain);

      PLC_bad_frame(&msbc_plc_state, dec_pcm_buf, dec_pcm_buf);

      ASSERT(ret == BT_STS_SUCCESS, "[%s] msbc decoder should never fail",
             __FUNCTION__);
#endif
    }

#ifdef SPEECH_RX_PLC_DUMP_DATA
    audio_dump_add_channel_data(1, (short *)dec_pcm_buf,
                                MSBC_ENCODE_PCM_LEN / 2);
    audio_dump_run();
#endif

    dec_pcm_buf = dec_pcm_buf + SAMPLES_LEN_PER_FRAME;
    frame_counter++;
  }

  for (i = 0; i < MSBC_LEN_FORMBT_PER_FRAME; i++) {
    msbc_buf_all[i] = msbc_buf_all[i + msbc_len];
  }
#endif
  // TRACE(1,"msbc + plc:%d", (hal_sys_timer_get()-timer_begin));
  return 0;
}

#ifdef TX_RX_PCM_MASK
#define MSBC_OFFSET_BYTES (0)
#else
#if defined(PCM_FAST_MODE)
#define MSBC_OFFSET_BYTES (1) // This is reletive with chip.
#else
#define MSBC_OFFSET_BYTES (2) // This is reletive with chip.
#endif
#endif

#define MSBC_OFFSET_HEADER0_BYTES                                              \
  ((MSBC_LEN_PER_FRAME - MSBC_OFFSET_BYTES) % MSBC_LEN_PER_FRAME)
#define MSBC_OFFSET_HEADER1_BYTES                                              \
  ((MSBC_LEN_PER_FRAME - MSBC_OFFSET_BYTES + 1) % MSBC_LEN_PER_FRAME)

int process_uplink_msbc_frames(unsigned char *pcm_buf, unsigned int pcm_len,
                               unsigned char *msbc_buf, unsigned int msbc_len) {
#if defined(HFP_1_6_ENABLE)
  uint32_t frame_counter = 0;
  static uint32_t frame_counter_total = 0;
  static btif_sbc_pcm_data_t pcm_data_enc;
  uint16_t bytes_encoded = 0;
  static uint8_t msbc_buf_frame[MSBC_LEN_PER_FRAME * 2];
  uint16_t msbc_len_frame = MSBC_LEN_PER_FRAME - 3;

  // TRACE(2,"process_uplink_msbc_frames:pcm_len:%d,msbc_len:%d",pcm_len,msbc_len);

  while ((frame_counter < msbc_len / MSBC_LEN_FORMBT_PER_FRAME) &&
         (frame_counter < pcm_len / BYTES_PER_PCM_FRAME)) {
    unsigned short *msbc_buf_dst_p;
    pcm_data_enc.data = pcm_buf + frame_counter * BYTES_PER_PCM_FRAME;
    pcm_data_enc.dataLen = BYTES_PER_PCM_FRAME;

#if (MSBC_OFFSET_BYTES == 0) || (MSBC_OFFSET_BYTES == 1) ||                    \
    (MSBC_OFFSET_BYTES == 2)
    // body
    btif_sbc_encode_frames(msbc_encoder, &pcm_data_enc, &bytes_encoded,
                           &(msbc_buf_frame[2 - MSBC_OFFSET_BYTES]),
                           (uint16_t *)&msbc_len_frame, 0xFFFF);
    // tail
    msbc_buf_frame[59 - MSBC_OFFSET_BYTES] = 0x00;
    // header
    msbc_buf_frame[MSBC_OFFSET_HEADER0_BYTES] = 0x01;
    switch ((frame_counter_total % 4)) {
    case 0:
      msbc_buf_frame[MSBC_OFFSET_HEADER1_BYTES] = 0x08;
      break;
    case 1:
      msbc_buf_frame[MSBC_OFFSET_HEADER1_BYTES] = 0x38;
      break;
    case 2:
      msbc_buf_frame[MSBC_OFFSET_HEADER1_BYTES] = 0xc8;
      break;
    default:
      msbc_buf_frame[MSBC_OFFSET_HEADER1_BYTES] = 0xf8;
      break;
    }
    msbc_buf_dst_p =
        (uint16_t *)(msbc_buf + frame_counter * MSBC_LEN_PER_FRAME * 2);

    for (int i = 0; i < MSBC_LEN_PER_FRAME; i++) {
      msbc_buf_dst_p[i] = msbc_buf_frame[i] << 8;
    }
#else
    btif_sbc_encode_frames(msbc_encoder, &pcm_data_enc, &bytes_encoded,
                           &(msbc_buf_frame[MSBC_LEN_PER_FRAME + 2]),
                           (uint16_t *)&msbc_len_frame, 0xFFFF);
    msbc_buf_frame[MSBC_LEN_PER_FRAME + 0] = 0x01;
    switch ((frame_counter_total % 4)) {
    case 0:
      msbc_buf_frame[MSBC_LEN_PER_FRAME + 1] = 0x08;
      break;
    case 1:
      msbc_buf_frame[MSBC_LEN_PER_FRAME + 1] = 0x38;
      break;
    case 2:
      msbc_buf_frame[MSBC_LEN_PER_FRAME + 1] = 0xc8;
      break;
    default:
      msbc_buf_frame[MSBC_LEN_PER_FRAME + 1] = 0xf8;
      break;
    }
    msbc_buf_frame[MSBC_LEN_PER_FRAME + 59] = 0x00;
    msbc_buf_dst_p =
        (uint16_t *)(msbc_buf + frame_counter * MSBC_LEN_PER_FRAME * 2);

    for (int i = 0; i < MSBC_LEN_PER_FRAME; i++) {
      msbc_buf_dst_p[i] = msbc_buf_frame[MSBC_OFFSET_BYTES + i] << 8;
    }

    for (int i = 0; i < MSBC_LEN_PER_FRAME; i++) {
      msbc_buf_frame[i] = msbc_buf_frame[MSBC_LEN_PER_FRAME + i];
    }
#endif
    frame_counter_total++;
    frame_counter++;
  }
#endif
  return 0;
}

#if defined(CHIP_BEST2300A)
#define CVSD_OFFSET_BYTES (120)
#else
#define CVSD_OFFSET_BYTES (120 - 2)
#endif

#define CVSD_PACKET_SIZE (120)

#define CVSD_PACKET_NUM (2)

#define CVSD_MUTE_PATTERN (0x55)

#define CVSD_PCM_SIZE (120)

static POSSIBLY_UNUSED uint8_t
    cvsd_buf_all[CVSD_PACKET_SIZE + CVSD_OFFSET_BYTES];

POSSIBLY_UNUSED plc_type_t check_cvsd_mute_pattern(uint8_t *buf, uint32_t len) {
  for (uint32_t i = 0; i < len; i++)
    if (buf[i] != CVSD_MUTE_PATTERN)
      return PLC_TYPE_PASS;

  return PLC_TYPE_CONTROLLER_MUTE;
}

int process_downlink_cvsd_frames(unsigned char *cvsd_buf, unsigned int cvsd_len,
                                 unsigned char *pcm_buf, unsigned int pcm_len) {
  // TRACE(2,"process_downlink_cvsd_frames
  // pcm_len:%d,cvsd_len:%d",pcm_len,cvsd_len);

#if defined(CVSD_BYPASS) && defined(ENABLE_LPC_PLC)
  ASSERT(cvsd_len % CVSD_PACKET_SIZE == 0, "[%s] cvsd input length(%d) error",
         __FUNCTION__, cvsd_len);
  for (uint32_t i = 0; i < cvsd_len; i += CVSD_PACKET_SIZE) {
    memcpy(&cvsd_buf_all[CVSD_OFFSET_BYTES], cvsd_buf, CVSD_PACKET_SIZE);
    memcpy(cvsd_buf, cvsd_buf_all, CVSD_PACKET_SIZE);
    memcpy(cvsd_buf_all, &cvsd_buf_all[CVSD_PACKET_SIZE], CVSD_OFFSET_BYTES);

    // DUMP16("0x%x, ", cvsd_buf, CVSD_PACKET_SIZE / 2);

    plc_type_t plc_type = check_cvsd_mute_pattern(cvsd_buf, CVSD_PACKET_SIZE);

    if (plc_type != PLC_TYPE_PASS) {
      memset(pcm_buf, 0, CVSD_PCM_SIZE);
      goto do_plc;
    }

    CvsdToPcm8k(cvsd_buf, (short *)(pcm_buf), CVSD_PACKET_SIZE / 2, 0);

  do_plc:
    if (plc_type == PLC_TYPE_PASS) {
      lpc_plc_save(msbc_plc_state, (int16_t *)pcm_buf);
    } else {
      TRACE(1, "PLC bad frame, plc type: %d", plc_type);
#if defined(PLC_DEBUG_PRINT_DATA)
      DUMP16("0x%x, ", cvsd_buf, CVSD_PACKET_SIZE / 2);
#endif
      lpc_plc_generate(msbc_plc_state, (int16_t *)pcm_buf, NULL);
    }

    cvsd_buf += CVSD_PACKET_SIZE;
    pcm_buf += CVSD_PCM_SIZE;
  }
#else
#if defined(CVSD_BYPASS)
  CvsdToPcm8k(cvsd_buf, (short *)(pcm_buf), cvsd_len / 2, 0);
#else
  memcpy(pcm_buf, cvsd_buf, cvsd_len);
#endif
#ifndef ENABLE_LPC_PLC
  speech_plc_8000((PlcSt_8000 *)speech_plc, (short *)pcm_buf, pcm_len);
#endif
#endif

  return 0;
}
int process_uplink_cvsd_frames(unsigned char *pcm_buf, unsigned int pcm_len,
                               unsigned char *cvsd_buf, unsigned int cvsd_len) {
  // TRACE(2,"process_uplink_cvsd_frames
  // pcm_len:%d,cvsd_len:%d",pcm_len,cvsd_len);
#if defined(CVSD_BYPASS)
  Pcm8kToCvsd((short *)pcm_buf, cvsd_buf, pcm_len / 2);
#endif
  return 0;
}
void *voicebtpcm_get_ext_buff(int size) {
  uint8_t *pBuff = NULL;
  if (size % 4) {
    size = size + (4 - size % 4);
  }
  app_audio_mempool_get_buff(&pBuff, size);
  VOICEBTPCM_TRACE(2, "[%s] len:%d", __func__, size);
  return (void *)pBuff;
}
// sco sample rate: encoder/decoder sample rate
// codec sample rate: hardware sample rate
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int codec_sample_rate) {
  uint8_t POSSIBLY_UNUSED *speech_buf = NULL;
  int POSSIBLY_UNUSED speech_len = 0;

  sco_frame_length =
      SPEECH_FRAME_MS_TO_LEN(sco_sample_rate, SPEECH_SCO_FRAME_MS);
  codec_frame_length =
      SPEECH_FRAME_MS_TO_LEN(codec_sample_rate, SPEECH_SCO_FRAME_MS);

  TRACE(3, "[%s] TX: sample rate = %d, frame len = %d", __func__,
        codec_sample_rate, codec_frame_length);
  TRACE(3, "[%s] RX: sample rate = %d, frame len = %d", __func__,
        codec_sample_rate, codec_frame_length);

  memset(cvsd_buf_all, CVSD_MUTE_PATTERN, sizeof(cvsd_buf_all));
#if defined(HFP_1_6_ENABLE)

  memset(msbc_buf_all, MSBC_MUTE_PATTERN & 0xFF, sizeof(msbc_buf_all));
  if (bt_sco_codec_is_msbc()) {
    app_audio_mempool_get_buff((uint8_t **)&msbc_encoder,
                               sizeof(btif_sbc_encoder_t));
    app_audio_mempool_get_buff((uint8_t **)&msbc_decoder,
                               sizeof(btif_sbc_decoder_t));

    // init msbc encoder
    btif_sbc_init_encoder(msbc_encoder);
    msbc_encoder->streamInfo.mSbcFlag = 1;
    msbc_encoder->streamInfo.numChannels = 1;
    msbc_encoder->streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;

    msbc_encoder->streamInfo.bitPool = 26;
    msbc_encoder->streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
    msbc_encoder->streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
    msbc_encoder->streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
    msbc_encoder->streamInfo.numSubBands = 8;

    // init msbc decoder
    const float EQLevel[25] = {
        0.0630957, 0.0794328, 0.1,       0.1258925, 0.1584893,
        0.1995262, 0.2511886, 0.3162278, 0.398107,  0.5011872,
        0.6309573, 0.794328,  1,         1.258925,  1.584893,
        1.995262,  2.5118864, 3.1622776, 3.9810717, 5.011872,
        6.309573,  7.943282,  10,        12.589254, 15.848932}; //-12~12
    uint8_t i;

    for (i = 0; i < sizeof(msbc_eq_band_gain) / sizeof(float); i++) {
      msbc_eq_band_gain[i] = EQLevel[cfg_aud_eq_sbc_band_settings[i] + 12];
    }

    btif_sbc_init_decoder(msbc_decoder);

    msbc_decoder->streamInfo.mSbcFlag = 1;
    msbc_decoder->streamInfo.bitPool = 26;
    msbc_decoder->streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
    msbc_decoder->streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;
    msbc_decoder->streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
    /* Number of blocks used to encode the stream (4, 8, 12, or 16) */
    msbc_decoder->streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
    /* The number of subbands in the stream (4 or 8) */
    msbc_decoder->streamInfo.numSubBands = 8;
    msbc_decoder->streamInfo.numChannels = 1;

    // init msbc plc
#ifndef ENABLE_LPC_PLC
    InitPLC(&msbc_plc_state);
#endif

    next_frame_flag = 0;
    msbc_find_first_sync = 0;

    packet_loss_detection_init(&pld);

#if defined(ENABLE_PLC_ENCODER)
    app_audio_mempool_get_buff((uint8_t **)&msbc_plc_encoder,
                               sizeof(btif_sbc_encoder_t));
    btif_sbc_init_encoder(msbc_plc_encoder);
    msbc_plc_encoder->streamInfo.mSbcFlag = 1;
    msbc_plc_encoder->streamInfo.bitPool = 26;
    msbc_plc_encoder->streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
    msbc_plc_encoder->streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;
    msbc_plc_encoder->streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
    /* Number of blocks used to encode the stream (4, 8, 12, or 16) */
    msbc_plc_encoder->streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
    /* The number of subbands in the stream (4 or 8) */
    msbc_plc_encoder->streamInfo.numSubBands = 8;
    msbc_plc_encoder->streamInfo.numChannels = 1;
    app_audio_mempool_get_buff((uint8_t **)&msbc_plc_encoder_buffer,
                               sizeof(int16_t) *
                                   (SAMPLES_LEN_PER_FRAME + MSBC_CODEC_DELAY));
#endif
  } else
#endif
  {
#ifndef ENABLE_LPC_PLC
    speech_plc = (PlcSt_8000 *)speech_plc_8000_init(voicebtpcm_get_ext_buff);
#endif
  }

#if defined(CVSD_BYPASS)
  Pcm8k_CvsdInit();
#endif

#ifdef SPEECH_RX_PLC_DUMP_DATA
  audio_dump_temp_buf =
      (int16_t *)voicebtpcm_get_ext_buff(sizeof(int16_t) * 120);
  audio_dump_init(120, sizeof(short), 3);
#endif

  resample_needed_flag = (sco_sample_rate == codec_sample_rate) ? 0 : 1;

  if (resample_needed_flag) {
    TRACE(1, "[%s] SCO <-- Resample --> CODEC", __func__);
    resample_buf =
        (int16_t *)voicebtpcm_get_ext_buff(sizeof(int16_t) * sco_frame_length);
  }

#if defined(SCO_OPTIMIZE_FOR_RAM)
  sco_overlay_ram_buf_len =
      hal_overlay_get_text_free_size((enum HAL_OVERLAY_ID_T)APP_OVERLAY_HFP);
  sco_overlay_ram_buf = (uint8_t *)hal_overlay_get_text_free_addr(
      (enum HAL_OVERLAY_ID_T)APP_OVERLAY_HFP);
#endif

  speech_len = app_audio_mempool_free_buff_size() - APP_BT_STREAM_USE_BUF_SIZE;
  speech_buf = (uint8_t *)voicebtpcm_get_ext_buff(speech_len);

  int tx_frame_ms = SPEECH_PROCESS_FRAME_MS;
  int rx_frame_ms = SPEECH_SCO_FRAME_MS;
  speech_init(codec_sample_rate, codec_sample_rate, tx_frame_ms, rx_frame_ms,
              SPEECH_SCO_FRAME_MS, speech_buf, speech_len);

  if (resample_needed_flag) {
    uplink_resample_st = iir_resample_init(
        codec_frame_length,
        iir_resample_choose_mode(codec_sample_rate, sco_sample_rate));
    downlink_resample_st = iir_resample_init(
        sco_frame_length,
        iir_resample_choose_mode(sco_sample_rate, codec_sample_rate));
  }

#if defined(ENABLE_LPC_PLC)
  msbc_plc_state = lpc_plc_create(sco_sample_rate);
#endif

  return 0;
}

int voicebtpcm_pcm_audio_deinit(void) {
  TRACE(1, "[%s] Close...", __func__);
  // TRACE(2,"[%s] app audio buffer free = %d", __func__,
  // app_audio_mempool_free_buff_size());

#if defined(ENABLE_LPC_PLC)
  lpc_plc_destroy(msbc_plc_state);
#endif

  if (resample_needed_flag) {
    iir_resample_destroy(uplink_resample_st);
    iir_resample_destroy(downlink_resample_st);
  }

  speech_deinit();

  packet_loss_detection_report(&pld);

#if defined(SCO_OPTIMIZE_FOR_RAM)
  sco_overlay_ram_buf = NULL;
  sco_overlay_ram_buf_len = 0;
#endif

  // TRACE(1,"Free buf = %d", app_audio_mempool_free_buff_size());

  return 0;
}