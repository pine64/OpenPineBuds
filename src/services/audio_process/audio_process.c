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
/*******************************************************************************
** namer    : Audio Process
** description  : Manage auido process algorithms, include :hw iir eq, sw iir
*eq,
**                  hw fir eq, drc...
** version  : V1.0
** author   : Yunjie Huo
** modify   : 2018.12.4
** todo     : NULL
** MIPS     :   NO PROCESS: 34M(TWS, one ear, Mono, AAC, 48k)
**              DRC1: 36M(3 bands)
**              DRC2: 12M
*******************************************************************************/
/*
Audio Flow:
    DECODE --> SW IIR EQ --> DRC --> LIMTER --> VOLUME --> HW IIR EQ --> SPK

                                                       +-----------------------------+
                                                       |             DAC | | |
+--------+     +-----------+    +-----+    +--------+  | +--------+
+-----------+ |  +-----+ |        | PCM |           |    |     |    |        |
| |        |    |           | |  |     | | DECODE +---->+ SW IIR EQ +--->+ DRC
+--->+ LIMTER +--->+ VOLUME +--->+ HW IIR EQ +--->+ SPK | |        |     | | |
|    |        |  | |        |    |           | |  |     |
+--------+     +-----------+    +-----+    +--------+  | +--------+
+-----------+ |  +-----+
                                                       +-----------------------------+

| ------------ | ------------------------- | -------- | ----------- |
| Algorithm    | description               | MIPS(M)  | RAM(kB)     |
| ------------ | ------------------------- | -------- | ----------- |
| DRC          | Dynamic Range Compression | 12M/band | 13          |
| Limiter/DRC2 | Limiter                   | 12M      | 5           |
| EQ           | Equalizer                 | 1M/band  | Almost zero |
*/

#include "audio_process.h"
#include "audio_cfg.h"
#include "drc.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hw_codec_iir_process.h"
#include "hw_iir_process.h"
#include "limiter.h"
#include "stdbool.h"
#include "string.h"
#include "tgt_hardware.h"

#if defined(USB_EQ_TUNING)
#if !defined(__HW_DAC_IIR_EQ_PROCESS__) && !defined(__SW_IIR_EQ_PROCESS__)
#error                                                                         \
    "Either HW_DAC_IIR_EQ_PROCESS or SW_IIR_EQ_PROCESS should be defined when enabling USB_EQ_TUNING"
#endif
#endif

#if defined(__PC_CMD_UART__) || defined(USB_EQ_TUNING)
#include "hal_cmd.h"

#if defined(__SW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_SW_IIR_UPDATE_CFG
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
#define AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
#define AUDIO_EQ_HW_IIR_UPDATE_CFG
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#define AUDIO_EQ_HW_FIR_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC2__
#define AUDIO_DRC2_UPDATE_CFG
#endif

#endif

#if defined(__SW_IIR_EQ_PROCESS__)
extern const IIR_CFG_T *const audio_eq_sw_iir_cfg_list[EQ_SW_IIR_LIST_NUM];
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
extern const IIR_CFG_T *const POSSIBLY_UNUSED
    audio_eq_hw_dac_iir_cfg_list[EQ_HW_DAC_IIR_LIST_NUM];
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
extern const IIR_CFG_T *const POSSIBLY_UNUSED
    audio_eq_hw_iir_cfg_list[EQ_HW_IIR_LIST_NUM];
#endif

#ifdef __HW_FIR_EQ_PROCESS__
extern const FIR_CFG_T *const audio_eq_hw_fir_cfg_list[EQ_HW_FIR_LIST_NUM];
#endif

#ifdef __AUDIO_DRC__
extern const DrcConfig audio_drc_cfg;
#endif

#ifdef __AUDIO_DRC2__
extern const LimiterConfig audio_drc2_cfg;
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) ||                                     \
    defined(AUDIO_EQ_HW_FIR_UPDATE_CFG) ||                                     \
    defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG) ||                                 \
    defined(AUDIO_EQ_HW_IIR_UPDATE_CFG) || defined(AUDIO_DRC_UPDATE_CFG) ||    \
    defined(AUDIO_DRC2_UPDATE_CFG)
#define AUDIO_UPDATE_CFG
#endif

#ifdef __AUDIO_DRC__
#define AUDIO_DRC_NEEDED_SIZE (1024 * 13)
#else
#define AUDIO_DRC_NEEDED_SIZE (0)
#endif

#ifdef __AUDIO_DRC2__
#define AUDIO_DRC2_NEEDED_SIZE (1024 * 5)
#else
#define AUDIO_DRC2_NEEDED_SIZE (0)
#endif

#define AUDIO_MEMORY_SIZE (AUDIO_DRC_NEEDED_SIZE + AUDIO_DRC2_NEEDED_SIZE)

#if AUDIO_MEMORY_SIZE > 0
#include "audio_memory.h"
#endif

#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif

typedef signed int pcm_24bits_t;
typedef signed short int pcm_16bits_t;

typedef struct {
  enum AUD_BITS_T sample_bits;
  enum AUD_SAMPRATE_T sample_rate;
  enum AUD_CHANNEL_NUM_T sw_ch_num;
  enum AUD_CHANNEL_NUM_T hw_ch_num;

#if defined(__SW_IIR_EQ_PROCESS__)
  bool sw_iir_enable;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
  bool hw_dac_iir_enable;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
  bool hw_iir_enable;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
  bool hw_fir_enable;
#endif

#if AUDIO_MEMORY_SIZE > 0
  uint8_t *audio_heap;
#endif

#ifdef __AUDIO_DRC__
  DrcState *drc_st;
#endif

#ifdef __AUDIO_DRC2__
  LimiterState *drc2_st;
#endif

#ifdef AUDIO_UPDATE_CFG
  bool update_cfg;
#endif

#ifdef USB_EQ_TUNING
  bool eq_updated_cfg;
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
  IIR_CFG_T sw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
  IIR_CFG_T hw_dac_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
  IIR_CFG_T hw_iir_cfg;
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
  FIR_CFG_T hw_fir_cfg;
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
  bool drc_update;
  DrcConfig drc_cfg;
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
  bool drc2_update;
  LimiterConfig drc2_cfg;
#endif
} AUDIO_PROCESS_T;

static AUDIO_PROCESS_T audio_process = {
    .sample_bits = AUD_BITS_NULL,
    .sample_rate = AUD_SAMPRATE_NULL,
    .sw_ch_num = AUD_CHANNEL_NUM_NULL,
    .hw_ch_num = AUD_CHANNEL_NUM_NULL,

#if defined(__SW_IIR_EQ_PROCESS__)
    .sw_iir_enable = false,
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
    .hw_dac_iir_enable = false,
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
    .hw_iir_enable = false,
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
    .hw_fir_enable = false,
#endif

#ifdef AUDIO_UPDATE_CFG
    .update_cfg = false,
#endif

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
    .sw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
    .hw_dac_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
    .hw_iir_cfg = {.num = 0},
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
    .hw_fir_cfg = {.len = 0},
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
    .drc_update = false,
    .drc_cfg = {.knee = 0,
                .filter_type = {-1, -1},
                .band_num = 1,
                .look_ahead_time = 0,
                .band_settings =
                    {
                        {0, 0, 1, 1, 1, 1},
                        {0, 0, 1, 1, 1, 1},
                    }},
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
    .drc2_update = false,
    .drc2_cfg =
        {
            .knee = 0,
            .look_ahead_time = 0,
            .threshold = 0,
            .makeup_gain = 0,
            .ratio = 1000,
            .attack_time = 1,
            .release_time = 1,
        },
#endif
};

int audio_eq_set_cfg(const FIR_CFG_T *fir_cfg, const IIR_CFG_T *iir_cfg,
                     AUDIO_EQ_TYPE_T audio_eq_type) {
#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__) ||        \
    defined(__HW_DAC_IIR_EQ_PROCESS__) || defined(__HW_IIR_EQ_PROCESS__)
  switch (audio_eq_type) {
#if defined(__SW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_SW_IIR: {
    if (iir_cfg) {
      audio_process.sw_iir_enable = false;
#ifdef USB_EQ_TUNING
      if (audio_process.eq_updated_cfg) {
        iir_set_cfg(&audio_process.sw_iir_cfg);
      } else
#endif
      {
        iir_set_cfg(iir_cfg);
      }
      audio_process.sw_iir_enable = true;
    } else {
      audio_process.sw_iir_enable = false;
    }
  } break;
#endif

#if defined(__HW_FIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_FIR: {
    if (fir_cfg) {
      audio_process.hw_fir_enable = false;
      fir_set_cfg(fir_cfg);
      audio_process.hw_fir_enable = true;
    } else {
      audio_process.hw_fir_enable = false;
    }
  } break;
#endif

#if defined(__HW_DAC_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_DAC_IIR: {
    if (iir_cfg) {
      HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL;
      enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
      int i;
      for (i = 1; i < 129; i++) {
        if (audio_process.sample_rate == AUD_SAMPRATE_7350 * i)
          break;
        if (audio_process.sample_rate == AUD_SAMPRATE_8000 * i)
          break;
      }
      TRACE(1, "audio_process.sample_rate i:%d", i);
      if (i == 129)
        i = 6;                                   // set default eq parameter;
      sample_rate_hw_dac_iir = i * 8463.541666f; // AUD_SAMPRATE_8463
#else
      sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif
      audio_process.hw_dac_iir_enable = false;
#ifdef USB_EQ_TUNING
      if (audio_process.eq_updated_cfg) {
        hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,
                                              &audio_process.hw_dac_iir_cfg);
      } else
#endif
      {
        hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir, iir_cfg);
      }
      ASSERT(hw_iir_cfg_dac != NULL, "[%s] codec IIR parameter error!",
             __func__);
      hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir,
                           HW_CODEC_IIR_DAC);
      audio_process.hw_dac_iir_enable = true;
    } else {
      audio_process.hw_dac_iir_enable = false;
    }
  } break;
#endif

#if defined(__HW_IIR_EQ_PROCESS__)
  case AUDIO_EQ_TYPE_HW_IIR: {
    if (iir_cfg) {
      HW_IIR_CFG_T *hw_iir_cfg = NULL;
      audio_process.hw_iir_enable = false;
      hw_iir_cfg = hw_iir_get_cfg(audio_process.sample_rate, iir_cfg);
      ASSERT(hw_iir_cfg != NULL, "[%s] 0x%x codec IIR parameter error!",
             __func__, (unsigned int)hw_iir_cfg);
      hw_iir_set_cfg(hw_iir_cfg);
      audio_process.hw_iir_enable = true;
    } else {
      audio_process.hw_iir_enable = false;
    }

  } break;
#endif

  default: {
    ASSERT(false, "[%s]Error eq type!", __func__);
  }
  }
#endif

  return 0;
}

int SRAM_TEXT_LOC audio_process_run(uint8_t *buf, uint32_t len) {
  int POSSIBLY_UNUSED pcm_len = 0;

  if (audio_process.sample_bits == AUD_BITS_16) {
    pcm_len = len / sizeof(pcm_16bits_t);
  } else if (audio_process.sample_bits == AUD_BITS_24) {
    pcm_len = len / sizeof(pcm_24bits_t);
  } else {
    ASSERT(0, "[%s] bits(%d) is invalid", __func__, audio_process.sample_bits);
  }

  if (audio_process.sw_ch_num == audio_process.hw_ch_num) {
    // do nothing
  } else if (audio_process.sw_ch_num == AUD_CHANNEL_NUM_1 &&
             audio_process.hw_ch_num == AUD_CHANNEL_NUM_2) {
    if (audio_process.sample_bits == AUD_BITS_16) {
      int16_t *pcm_buf = (int16_t *)buf;
      for (uint32_t i = 0, j = 0; i < pcm_len; i += 2, j++) {
        pcm_buf[j] = pcm_buf[i];
      }
    } else {
      int32_t *pcm_buf = (int32_t *)buf;
      for (uint32_t i = 0, j = 0; i < pcm_len; i += 2, j++) {
        pcm_buf[j] = pcm_buf[i];
      }
    }

    pcm_len /= 2;
  } else {
    ASSERT(0, "[%s] sw_ch_num(%d) or hw_ch_num(%d) is invalid", __FUNCTION__,
           audio_process.sw_ch_num, audio_process.hw_ch_num);
  }

#ifdef AUDIO_PROCESS_DUMP
  int *buf32 = (int *)buf;
  for (int i = 0; i < 1024; i++)
    dump_buf[i] = buf32[2 * i] >> 8;
  audio_dump_clear_up();
  audio_dump_add_channel_data(0, dump_buf, 1024);

#endif

  // int32_t s_time,e_time;
  // s_time = hal_fast_sys_timer_get();

#ifdef __SW_IIR_EQ_PROCESS__
  if (audio_process.sw_iir_enable) {
    iir_run(buf, pcm_len);
  }
#endif

#ifdef __HW_FIR_EQ_PROCESS__
  if (audio_process.hw_fir_enable) {
    fir_run(buf, pcm_len);
  }
#endif

#ifdef __AUDIO_DRC__
#ifdef AUDIO_DRC_UPDATE_CFG
  if (audio_process.drc_update) {
    drc_set_config(audio_process.drc_st, &audio_process.drc_cfg);
    audio_process.drc_update = false;
  }
#endif

  drc_process(audio_process.drc_st, buf, pcm_len);
#endif

  // int32_t m_time = hal_fast_sys_timer_get();

#ifdef __AUDIO_DRC2__
#ifdef AUDIO_DRC2_UPDATE_CFG
  if (audio_process.drc2_update) {
    limiter_set_config(audio_process.drc2_st, &audio_process.drc2_cfg);
    audio_process.drc2_update = false;
  }
#endif

  limiter_process(audio_process.drc2_st, buf, pcm_len);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
  if (audio_process.hw_iir_enable) {
    hw_iir_run(buf, pcm_len);
  }
#endif

  if (audio_process.sw_ch_num == audio_process.hw_ch_num) {
    // do nothing
  } else if (audio_process.sw_ch_num == AUD_CHANNEL_NUM_1 &&
             audio_process.hw_ch_num == AUD_CHANNEL_NUM_2) {
    if (audio_process.sample_bits == AUD_BITS_16) {
      int16_t *pcm_buf = (int16_t *)buf;
      for (int32_t i = pcm_len - 1, j = 2 * pcm_len - 1; i >= 0; i--, j -= 2) {
        pcm_buf[j + 1] = pcm_buf[i];
        pcm_buf[j + 0] = pcm_buf[i];
      }
    } else {
      int32_t *pcm_buf = (int32_t *)buf;
      for (int32_t i = pcm_len - 1, j = 2 * pcm_len - 2; i >= 0; i--, j -= 2) {
        pcm_buf[j + 1] = pcm_buf[i];
        pcm_buf[j + 0] = pcm_buf[i];
      }
    }

    pcm_len *= 2;
  } else {
    ASSERT(0, "[%s] sw_ch_num(%d) or hw_ch_num(%d) is invalid", __FUNCTION__,
           audio_process.sw_ch_num, audio_process.hw_ch_num);
  }

#ifdef AUDIO_PROCESS_DUMP
  // for(int i=0;i<1024;i++)
  //    dump_buf[i] = buf32[2 * i+0]>>8;
  // audio_dump_add_channel_data(1, dump_buf, 1024);
  audio_dump_run();
#endif

  // e_time = hal_fast_sys_timer_get();
  // TRACE(4,"[%s] Sample len = %d, drc1 %d us, drc2 %d us",
  //    __func__, pcm_len, FAST_TICKS_TO_US(m_time - s_time),
  //    FAST_TICKS_TO_US(e_time - m_time));

  return 0;
}

/*
 * frame_size stands for samples per channel
 */
int audio_process_open(enum AUD_SAMPRATE_T sample_rate,
                       enum AUD_BITS_T sample_bits,
                       enum AUD_CHANNEL_NUM_T sw_ch_num,
                       enum AUD_CHANNEL_NUM_T hw_ch_num, int32_t frame_size,
                       void *eq_buf, uint32_t len) {
  TRACE(
      5,
      "[%s] sample_rate = %d, sample_bits = %d, sw_ch_num = %d, hw_ch_num = %d",
      __func__, sample_rate, sample_bits, sw_ch_num, hw_ch_num);
#ifdef AUDIO_PROCESS_DUMP
  audio_dump_init(1024, sizeof(short), 2);
#endif
  audio_process.sample_rate = sample_rate;
  audio_process.sample_bits = sample_bits;
  audio_process.sw_ch_num = sw_ch_num;
  audio_process.hw_ch_num = hw_ch_num;

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
  void *fir_eq_buf = eq_buf;
  uint32_t fir_len = len / 2;
  void *iir_eq_buf = (uint8_t *)eq_buf + fir_len;
  uint32_t iir_len = len / 2;
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)
  void *fir_eq_buf = eq_buf;
  uint32_t fir_len = len;
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
  void *iir_eq_buf = eq_buf;
  uint32_t iir_len = len;
#endif

#ifdef __SW_IIR_EQ_PROCESS__
  iir_open(sample_rate, sample_bits, sw_ch_num);
#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
  audio_eq_set_cfg(NULL, &audio_process.sw_iir_cfg, AUDIO_EQ_TYPE_SW_IIR);
#endif
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
  enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
  int i;
  for (i = 1; i < 129; i++) {
    if (audio_process.sample_rate == AUD_SAMPRATE_7350 * i)
      break;
    if (audio_process.sample_rate == AUD_SAMPRATE_8000 * i)
      break;
  }
  TRACE(1, "sample_rate i:%d", i);
  if (i == 129)
    i = 6;                                   // set default eq parameter;
  sample_rate_hw_dac_iir = i * 8463.541666f; // AUD_SAMPRATE_8463
#else
  sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif

  hw_codec_iir_open(sample_rate_hw_dac_iir, HW_CODEC_IIR_DAC, CODEC_OUTPUT_DEV);
#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
  audio_eq_set_cfg(NULL, &audio_process.hw_dac_iir_cfg,
                   AUDIO_EQ_TYPE_HW_DAC_IIR);
#endif
#endif

#ifdef __HW_IIR_EQ_PROCESS__
  hw_iir_open(sample_rate, sample_bits, sw_ch_num, iir_eq_buf, iir_len);
#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
  audio_eq_set_cfg(NULL, &audio_process.hw_iir_cfg, AUDIO_EQ_TYPE_HW_IIR);
#endif
#endif

#ifdef __HW_FIR_EQ_PROCESS__
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
  hal_cmu_fir_high_speed_enable(HAL_CMU_FIR_USER_EQ);
#endif
  fir_open(sample_rate, sample_bits, sw_ch_num, fir_eq_buf, fir_len);
#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
  audio_eq_set_cfg(&audio_process.hw_fir_cfg, NULL, AUDIO_EQ_TYPE_HW_FIR);
#endif
#endif

#if AUDIO_MEMORY_SIZE > 0
  syspool_get_buff(&audio_process.audio_heap, AUDIO_MEMORY_SIZE);
  audio_heap_init(audio_process.audio_heap, AUDIO_MEMORY_SIZE);
#endif

#ifdef __AUDIO_DRC__
  audio_process.drc_st =
      drc_create(sample_rate, frame_size, sample_bits, sw_ch_num,
#ifdef AUDIO_DRC_UPDATE_CFG
                 &audio_process.drc_cfg
#else
                 &audio_drc_cfg
#endif
      );
#ifdef AUDIO_DRC_UPDATE_CFG
  audio_process.drc_update = false;
#endif
#endif

#ifdef __AUDIO_DRC2__
  audio_process.drc2_st =
      limiter_create(sample_rate, frame_size, sample_bits, sw_ch_num,
#ifdef AUDIO_DRC2_UPDATE_CFG
                     &audio_process.drc2_cfg
#else
                     &audio_drc2_cfg
#endif
      );
#ifdef AUDIO_DRC2_UPDATE_CFG
  audio_process.drc2_update = false;
#endif
#endif

#if defined(__PC_CMD_UART__) && defined(USB_AUDIO_APP)
  hal_cmd_open();
#endif

  return 0;
}

int audio_process_close(void) {
#ifdef __SW_IIR_EQ_PROCESS__
  audio_process.sw_iir_enable = false;
  iir_close();
#endif

#ifdef __HW_DAC_IIR_EQ_PROCESS__
  audio_process.hw_dac_iir_enable = false;
  hw_codec_iir_close(HW_CODEC_IIR_DAC);
#endif

#ifdef __HW_IIR_EQ_PROCESS__
  audio_process.hw_iir_enable = false;
  hw_iir_close();
#endif

#ifdef __HW_FIR_EQ_PROCESS__
  audio_process.hw_fir_enable = false;
  fir_close();
#if defined(CHIP_BEST1000) && defined(FIR_HIGHSPEED)
  hal_cmu_fir_high_speed_disable(HAL_CMU_FIR_USER_EQ);
#endif
#endif

#ifdef __AUDIO_DRC__
#ifdef AUDIO_DRC_UPDATE_CFG
  audio_process.drc_update = false;
#endif
  drc_destroy(audio_process.drc_st);
  audio_process.drc_st = NULL;
#endif

#ifdef __AUDIO_DRC2__
#ifdef AUDIO_DRC2_UPDATE_CFG
  audio_process.drc2_update = false;
#endif
  limiter_destroy(audio_process.drc2_st);
  audio_process.drc2_st = NULL;
#endif

#if AUDIO_MEMORY_SIZE > 0
  size_t total = 0, used = 0, max_used = 0;
  audio_memory_info(&total, &used, &max_used);
  TRACE(3, "AUDIO MALLOC MEM: total - %d, used - %d, max_used - %d.", total,
        used, max_used);
  ASSERT(used == 0, "[%s] used != 0", __func__);
#endif

#if defined(__PC_CMD_UART__) && defined(USB_AUDIO_APP)
  hal_cmd_close();
#endif

  return 0;
}

#if defined(__PC_CMD_UART__) || defined(USB_EQ_TUNING)
int audio_ping_callback(uint8_t *buf, uint32_t len) {
  // TRACE(0,"");
  return 0;
}

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) && !defined(USB_EQ_TUNING)
#ifndef USB_AUDIO_APP
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(IIR_CFG_T));

  if (len != sizeof(IIR_CFG_T)) {
    return 1;
  }

  memcpy(&audio_process.sw_iir_cfg, buf, sizeof(IIR_CFG_T));
  TRACE(3, "band num:%d gain0:%d, gain1:%d",
        (int32_t)audio_process.sw_iir_cfg.num,
        (int32_t)(audio_process.sw_iir_cfg.gain0 * 10),
        (int32_t)(audio_process.sw_iir_cfg.gain1 * 10));
  for (uint8_t i = 0; i < audio_process.sw_iir_cfg.num; i++) {
    TRACE(5, "band num:%d type %d gain:%d fc:%d q:%d", i,
          (int32_t)(audio_process.sw_iir_cfg.param[i].type),
          (int32_t)(audio_process.sw_iir_cfg.param[i].gain * 10),
          (int32_t)(audio_process.sw_iir_cfg.param[i].fc * 10),
          (int32_t)(audio_process.sw_iir_cfg.param[i].Q * 10));
  }

#ifdef __SW_IIR_EQ_PROCESS__
  {
    iir_set_cfg(&audio_process.sw_iir_cfg);
    audio_process.sw_iir_enable = true;
  }
#endif

  return 0;
}
#else
int audio_eq_sw_iir_callback(uint8_t *buf, uint32_t len) {
  // IIR_CFG_T *rx_iir_cfg = NULL;

  // rx_iir_cfg = (IIR_CFG_T *)buf;

  // TRACE(3,"[%s] left gain = %f, right gain = %f", __func__,
  // rx_iir_cfg->gain0, rx_iir_cfg->gain1);

  // for(int i=0; i<rx_iir_cfg->num; i++)
  // {
  //     TRACE(5,"[%s] iir[%d] gain = %f, f = %f, Q = %f", __func__, i,
  //     rx_iir_cfg->param[i].gain, rx_iir_cfg->param[i].fc,
  //     rx_iir_cfg->param[i].Q);
  // }

  // audio_eq_set_cfg(NULL,(const IIR_CFG_T *)rx_iir_cfg,AUDIO_EQ_TYPE_SW_IIR);

  iir_update_cfg_tbl(buf, len);

  return 0;
}
#endif
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
int audio_eq_hw_fir_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(FIR_CFG_T));

  if (len != sizeof(FIR_CFG_T)) {
    return 1;
  }

  FIR_CFG_T *rx_fir_cfg = NULL;

  rx_fir_cfg = (FIR_CFG_T *)buf;

  TRACE(3, "[%s] left gain = %d, right gain = %d", __func__, rx_fir_cfg->gain0,
        rx_fir_cfg->gain1);

  TRACE(6, "[%s] len = %d, coef: %d, %d......%d, %d", __func__, rx_fir_cfg->len,
        rx_fir_cfg->coef[0], rx_fir_cfg->coef[1],
        rx_fir_cfg->coef[rx_fir_cfg->len - 2],
        rx_fir_cfg->coef[rx_fir_cfg->len - 1]);

  rx_fir_cfg->gain0 = 6;
  rx_fir_cfg->gain1 = 6;

  if (rx_fir_cfg) {
    memcpy(&audio_process.fir_cfg, rx_fir_cfg, sizeof(audio_process.fir_cfg));
    audio_process.fir_enable = true;
    fir_set_cfg(&audio_process.fir_cfg);
  } else {
    audio_process.fir_enable = false;
  }

  return 0;
}
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
int audio_drc_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(DrcConfig));

  if (len != sizeof(DrcConfig)) {
    TRACE(1, "[%s] WARNING: Length is different", __func__);

    return 1;
  }

  if (audio_process.drc_st == NULL) {
    TRACE(1, "[%s] WARNING: audio_process.drc2_st = NULL", __func__);
    TRACE(1, "[%s] WARNING: Please Play music, then tuning Limiter", __func__);

    return 2;
  }

  memcpy(&audio_process.drc_cfg, buf, sizeof(DrcConfig));
  audio_process.drc_update = true;

  return 0;
}
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
int audio_drc2_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(LimiterConfig));

  if (len != sizeof(LimiterConfig)) {
    TRACE(1, "[%s] WARNING: Length is different", __func__);

    return 1;
  }

  if (audio_process.drc2_st == NULL) {
    TRACE(1, "[%s] WARNING: audio_process.drc2_st = NULL", __func__);
    TRACE(1, "[%s] WARNING: Please Play music, then tuning Limiter", __func__);

    return 2;
  }

  memcpy(&audio_process.drc2_cfg, buf, sizeof(LimiterConfig));
  audio_process.drc2_update = true;

  return 0;
}
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG) && !defined(USB_EQ_TUNING)
int audio_eq_hw_dac_iir_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(IIR_CFG_T));

  if (len != sizeof(IIR_CFG_T)) {
    return 1;
  }

  memcpy(&audio_process.hw_dac_iir_cfg, buf, sizeof(IIR_CFG_T));
  TRACE(3, "band num:%d gain0:%d, gain1:%d",
        (int32_t)audio_process.hw_dac_iir_cfg.num,
        (int32_t)(audio_process.hw_dac_iir_cfg.gain0 * 10),
        (int32_t)(audio_process.hw_dac_iir_cfg.gain1 * 10));
  for (uint8_t i = 0; i < audio_process.hw_dac_iir_cfg.num; i++) {
    TRACE(5, "band num:%d type %d gain:%d fc:%d q:%d", i,
          (int32_t)(audio_process.hw_dac_iir_cfg.param[i].type),
          (int32_t)(audio_process.hw_dac_iir_cfg.param[i].gain * 10),
          (int32_t)(audio_process.hw_dac_iir_cfg.param[i].fc * 10),
          (int32_t)(audio_process.hw_dac_iir_cfg.param[i].Q * 10));
  }

#ifdef __HW_DAC_IIR_EQ_PROCESS__
  {
    HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL;
    enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
    int i;
    for (i = 1; i < 129; i++) {
      if (audio_process.sample_rate == AUD_SAMPRATE_7350 * i)
        break;
      if (audio_process.sample_rate == AUD_SAMPRATE_8000 * i)
        break;
    }
    TRACE(1, "audio_process.sample_rate i:%d", i);
    if (i == 129)
      i = 6;                                   // set default eq parameter;
    sample_rate_hw_dac_iir = i * 8463.541666f; // AUD_SAMPRATE_8463
#else
    sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif
    hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,
                                          &audio_process.hw_dac_iir_cfg);
    ASSERT(hw_iir_cfg_dac != NULL, "[%s] %d codec IIR parameter error!",
           __func__, (uint32_t)hw_iir_cfg_dac);

    // hal_codec_iir_dump(hw_iir_cfg_dac);

    hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir,
                         HW_CODEC_IIR_DAC);
    audio_process.hw_dac_iir_enable = true;
  }
#endif

  return 0;
}
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
int audio_eq_hw_iir_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof(IIR_CFG_T));

  if (len != sizeof(IIR_CFG_T)) {
    return 1;
  }

  memcpy(&audio_process.hw_iir_cfg, buf, sizeof(IIR_CFG_T));
  TRACE(3, "band num:%d gain0:%d, gain1:%d",
        (int32_t)audio_process.hw_iir_cfg.num,
        (int32_t)(audio_process.hw_iir_cfg.gain0 * 10),
        (int32_t)(audio_process.hw_iir_cfg.gain1 * 10));

  for (uint8_t i = 0; i < audio_process.hw_iir_cfg.num; i++) {
    TRACE(5, "band num:%d type %d gain:%d fc:%d q:%d", i,
          (int32_t)(audio_process.hw_iir_cfg.param[i].type),
          (int32_t)(audio_process.hw_iir_cfg.param[i].gain * 10),
          (int32_t)(audio_process.hw_iir_cfg.param[i].fc * 10),
          (int32_t)(audio_process.hw_iir_cfg.param[i].Q * 10));
  }

#ifdef __HW_IIR_EQ_PROCESS__
  {
    HW_IIR_CFG_T *hw_iir_cfg = NULL;
#ifdef __AUDIO_RESAMPLE__
    enum AUD_SAMPRATE_T sample_rate_hw_iir = AUD_SAMPRATE_50781;
#else
    enum AUD_SAMPRATE_T sample_rate_hw_iir = audio_process.sample_rate;
#endif
    hw_iir_cfg = hw_iir_get_cfg(sample_rate_hw_iir, &audio_process.hw_iir_cfg);
    ASSERT(hw_iir_cfg != NULL, "[%s] %d codec IIR parameter error!", __func__,
           hw_iir_cfg);
    hw_iir_set_cfg(hw_iir_cfg);
    audio_process.hw_iir_enable = true;
  }
#endif

  return 0;
}
#endif
#endif // #ifdef __PC_CMD_UART__

#ifdef USB_EQ_TUNING

int audio_eq_usb_iir_callback(uint8_t *buf, uint32_t len) {
  IIR_CFG_T *cur_cfg;

  TRACE(2, "usb_iir_cb: len=[%d - %d]", len, sizeof(IIR_CFG_T));

  if (len != sizeof(IIR_CFG_T)) {
    return 1;
  }

  cur_cfg = (IIR_CFG_T *)buf;
  TRACE(2, "-> sample_rate[%d], num[%d]",
        /*cur_cfg->samplerate,*/ audio_process.sample_rate, cur_cfg->num);

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
  audio_process.sw_iir_cfg.gain0 = cur_cfg->gain0;
  audio_process.sw_iir_cfg.gain1 = cur_cfg->gain1;
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
  audio_process.hw_dac_iir_cfg.gain0 = 0;
  audio_process.hw_dac_iir_cfg.gain1 = 0;
#else
  audio_process.hw_dac_iir_cfg.gain0 = cur_cfg->gain0;
  audio_process.hw_dac_iir_cfg.gain1 = cur_cfg->gain1;
#endif
#endif

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
  if (cur_cfg->num > AUD_DAC_IIR_NUM_EQ) {
    audio_process.hw_dac_iir_cfg.num = AUD_DAC_IIR_NUM_EQ;
  } else {
    audio_process.hw_dac_iir_cfg.num = cur_cfg->num;
  }
  TRACE(1, "-> hw_dac_iir_num[%d]", audio_process.hw_dac_iir_cfg.num);
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
  audio_process.sw_iir_cfg.num =
      cur_cfg->num - audio_process.hw_dac_iir_cfg.num;
  TRACE(1, "-> sw_iir_num[%d]", audio_process.sw_iir_cfg.num);
#endif
  // TRACE(2,"-> iir_num[%d - %d]", audio_process.hw_dac_iir_cfg.num,
  // audio_process.sw_iir_cfg.num);

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
  if (audio_process.hw_dac_iir_cfg.num) {
    memcpy((void *)(&audio_process.hw_dac_iir_cfg.param[0]),
           (void *)(&cur_cfg->param[0]),
           audio_process.hw_dac_iir_cfg.num * sizeof(IIR_PARAM_T));
  }
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
  if (audio_process.sw_iir_cfg.num) {

    memcpy((void *)(&audio_process.sw_iir_cfg.param[0]),
           (void *)(&cur_cfg->param[audio_process.hw_dac_iir_cfg.num]),
           audio_process.sw_iir_cfg.num * sizeof(IIR_PARAM_T));

  } else {

    // set a default filter
    audio_process.sw_iir_cfg.num = 1;

    audio_process.sw_iir_cfg.param[0].fc = 1000.0;
    audio_process.sw_iir_cfg.param[0].gain = 0.0;
    audio_process.sw_iir_cfg.param[0].type = IIR_TYPE_PEAK;
    audio_process.sw_iir_cfg.param[0].Q = 1.0;
  }
#endif

  if (audio_process.sample_rate) {
    audio_process.update_cfg = true;
  }

  audio_process.eq_updated_cfg = true;

  return 0;
}

void audio_eq_usb_eq_update(void) {
  if (audio_process.update_cfg) {

#if defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
    HW_CODEC_IIR_CFG_T *hw_iir_cfg_dac = NULL;

    if (audio_process.hw_dac_iir_cfg.num) {
      enum AUD_SAMPRATE_T sample_rate_hw_dac_iir;
#ifdef __AUDIO_RESAMPLE__
      int i;
      for (i = 1; i < 129; i++) {
        if (audio_process.sample_rate == AUD_SAMPRATE_7350 * i)
          break;
        if (audio_process.sample_rate == AUD_SAMPRATE_8000 * i)
          break;
      }
      TRACE(1, "audio_process.sample_rate i:%d", i);
      if (i == 129)
        i = 6;                                   // set default eq parameter;
      sample_rate_hw_dac_iir = i * 8463.541666f; // AUD_SAMPRATE_8463
#else
      sample_rate_hw_dac_iir = audio_process.sample_rate;
#endif
      hw_iir_cfg_dac = hw_codec_iir_get_cfg(sample_rate_hw_dac_iir,
                                            &audio_process.hw_dac_iir_cfg);

      hw_codec_iir_set_cfg(hw_iir_cfg_dac, sample_rate_hw_dac_iir,
                           HW_CODEC_IIR_DAC);
      audio_process.hw_dac_iir_enable = true;

    } else {

      audio_process.hw_dac_iir_enable = false;
    }
#endif

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG)
    iir_set_cfg(&audio_process.sw_iir_cfg);
    audio_process.sw_iir_enable = true;
#endif

    audio_process.update_cfg = false;

    /*
                    TRACE(4,"USB EQ Update: en[%d-%d], num[%d-%d]",
                                    audio_process.hw_dac_iir_enable,
       audio_process.sw_iir_enable, audio_process.hw_dac_iir_cfg.num,
       audio_process.sw_iir_cfg.num);
    */
  }
}

#endif // USB_EQ_TUNING

typedef struct {
  uint8_t type;
  uint8_t maxEqBandNum;
  uint16_t sample_rate_num;
  uint8_t sample_rate[50];
} query_eq_info_t;

extern int getMaxEqBand(void);
extern int getSampleArray(uint8_t *buf, uint16_t *num);

extern void hal_cmd_set_res_playload(uint8_t *data, int len);
#define CMD_TYPE_QUERY_DUT_EQ_INFO 0x00
int audio_cmd_callback(uint8_t *buf, uint32_t len) {
  uint8_t type;
  // uint32_t* sample_rate;
  // uint8_t* p;
  query_eq_info_t info;

  type = buf[0];
  // p = buf + 1;

  TRACE(2, "%s type: %d", __func__, type);
  switch (type) {
  case CMD_TYPE_QUERY_DUT_EQ_INFO:
    info.type = CMD_TYPE_QUERY_DUT_EQ_INFO;
    info.maxEqBandNum = getMaxEqBand();
    getSampleArray(info.sample_rate, &info.sample_rate_num);

    hal_cmd_set_res_playload((uint8_t *)&info, 4 + info.sample_rate_num * 4);
    break;
  default:
    break;
  }

  return 0;
}

#ifdef AUDIO_SECTION_ENABLE
int audio_cfg_burn_callback(uint8_t *buf, uint32_t len) {
  TRACE(3, "[%s] len = %d, sizeof(struct) = %d", __func__, len,
        sizeof_audio_cfg());

  if (len != sizeof_audio_cfg()) {
    return 1;
  }

  int res = 0;
  res = store_audio_cfg_into_audio_section((AUDIO_CFG_T *)buf);

  if (res) {
    TRACE(2, "[%s] ERROR: res = %d", __func__, res);
    res += 100;
  } else {
    TRACE(1, "[%s] Store audio cfg into audio section!!!", __func__);
  }

  return res;
}
#endif

int audio_process_init(void) {
#ifdef __PC_CMD_UART__
  hal_cmd_init();

#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
  hal_cmd_register("iir_eq", audio_eq_sw_iir_callback); // Will be removed
  hal_cmd_register("sw_iir_eq", audio_eq_sw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
  hal_cmd_register("iir_eq", audio_eq_hw_dac_iir_callback); // Will be removed
  hal_cmd_register("dac_iir_eq", audio_eq_hw_dac_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
  hal_cmd_register("iir_eq", audio_eq_hw_iir_callback); // Will be removed
  hal_cmd_register("hw_iir_eq", audio_eq_hw_iir_callback);
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
  hal_cmd_register("fir_eq", audio_eq_hw_fir_callback);
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
  hal_cmd_register("drc", audio_drc_callback);
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
  hal_cmd_register("limiter", audio_drc2_callback);
#endif

#ifdef AUDIO_SECTION_ENABLE
  hal_cmd_register("burn", audio_cfg_burn_callback);
  hal_cmd_register("audio_burn", audio_cfg_burn_callback);
#endif

  hal_cmd_register("cmd", audio_cmd_callback);
  hal_cmd_register("ping", audio_ping_callback);
#endif

#ifdef USB_EQ_TUNING

  hal_cmd_init();

#if defined(AUDIO_EQ_SW_IIR_UPDATE_CFG) ||                                     \
    defined(AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG)
  hal_cmd_register("iir_eq", audio_eq_usb_iir_callback);
#endif

  hal_cmd_register("cmd", audio_cmd_callback);
  hal_cmd_register("ping", audio_ping_callback);

#endif

  // load default cfg
#ifdef AUDIO_EQ_SW_IIR_UPDATE_CFG
  memcpy(&audio_process.sw_iir_cfg, audio_eq_sw_iir_cfg_list[0],
         sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_DAC_IIR_UPDATE_CFG
  memcpy(&audio_process.hw_dac_iir_cfg, audio_eq_hw_dac_iir_cfg_list[0],
         sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_IIR_UPDATE_CFG
  memcpy(&audio_process.hw_iir_cfg, audio_eq_hw_iir_cfg_list[0],
         sizeof(IIR_CFG_T));
#endif

#ifdef AUDIO_EQ_HW_FIR_UPDATE_CFG
  memcpy(&audio_process.hw_fir_cfg, audio_eq_hw_fir_cfg_list[0],
         sizeof(FIR_CFG_T));
#endif

#ifdef AUDIO_DRC_UPDATE_CFG
  memcpy(&audio_process.drc_cfg, &audio_drc_cfg, sizeof(DrcConfig));
#endif

#ifdef AUDIO_DRC2_UPDATE_CFG
  memcpy(&audio_process.drc2_cfg, &audio_drc2_cfg, sizeof(LimiterConfig));
#endif

  return 0;
}
