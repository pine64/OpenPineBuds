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
#include "hal_i2s_tdm.h"
#include "hal_dma.h"
#include "hal_i2s.h"
#include "hal_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define I2S_TDM_TRACE TRACE
#define I2S_TDM_DUMP8 DUMP8
#else
#define I2S_TDM_TRACE(n, str, ...)
#define I2S_TDM_DUMP8(str, ...)
#endif

#define I2S_TDM_FRAME_SIZE_MAX 512
#define I2S_TDM_TX_FRAME_NUM 2
#define I2S_TDM_TX_FRAME_SIZE I2S_TDM_FRAME_SIZE_MAX / 16

static struct HAL_DMA_DESC_T i2s_tdm_tx_dma_desc[HAL_I2S_ID_QTY]
                                                [I2S_TDM_TX_FRAME_NUM];
static uint16_t I2S_TDM_BUF_ALIGN
    i2s_tdm_tx_buf[HAL_I2S_ID_QTY][I2S_TDM_TX_FRAME_NUM][I2S_TDM_TX_FRAME_SIZE];
static struct HAL_DMA_CH_CFG_T tx_dma_cfg[HAL_I2S_ID_QTY];
static struct HAL_I2S_TDM_CONFIG_T i2s_tdm_cfg[HAL_I2S_ID_QTY];

static inline bool i2s_tdm_cycles_in_arrays(uint16_t cycles) {
  if (cycles == (uint16_t)HAL_I2S_TDM_CYCLES_16 ||
      cycles == (uint16_t)HAL_I2S_TDM_CYCLES_32 ||
      cycles == (uint16_t)HAL_I2S_TDM_CYCLES_64 ||
      cycles == (uint16_t)HAL_I2S_TDM_CYCLES_128 ||
      cycles == (uint16_t)HAL_I2S_TDM_CYCLES_256 ||
      cycles == (uint16_t)HAL_I2S_TDM_CYCLES_512)
    return true;
  else
    return false;
}

static inline bool i2s_tdm_fs_cycles_in_arrays(uint16_t fs_cycles) {
  if (fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_1 ||
      fs_cycles == (uint16_t)HAL_TDM_FS_CYCLES_8 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_16 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_32 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_64 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_128 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_256 ||
      fs_cycles == (uint16_t)HAL_I2S_TDM_FS_CYCLES_ONE_LESS)
    return true;
  else
    return false;
}

static inline bool i2s_tdm_slot_cycles_in_arrays(uint8_t slot_cycles) {
  if (slot_cycles == (uint8_t)HAL_I2S_TDM_SLOT_CYCLES_16 ||
      slot_cycles == (uint8_t)HAL_I2S_TDM_SLOT_CYCLES_32)
    return true;
  else
    return false;
}

static void i2s_tdm0_tx_handler(uint8_t chan, uint32_t remains, uint32_t error,
                                struct HAL_DMA_DESC_T *lli) {
#if 0
    static int cnt = 0;

    cnt++;
    if (cnt % 600 == 0)
    {
        I2S_TDM_TRACE(4,"I2S_TDM0-TX: remains=%ld, error=%ld, cnt=%d,tx_buff_len = %d.", remains, error, cnt,sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_0]));
        I2S_TDM_DUMP8("0x%x,",i2s_tdm_tx_buf[HAL_I2S_ID_0],sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_0]) <= 32 ? sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_0]) : 32 );
    }
#endif
}

static void i2s_tdm1_tx_handler(uint8_t chan, uint32_t remains, uint32_t error,
                                struct HAL_DMA_DESC_T *lli) {
#if 0
    static int cnt = 0;

    cnt++;
    if (cnt % 600 == 0) {
        I2S_TDM_TRACE(4,"I2S_TDM1-TX: remains=%ld, error=%ld, cnt=%d,tx_buff_len = %d.", remains, error, cnt,sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_1]));
        I2S_TDM_DUMP8("0x%x,",i2s_tdm_tx_buf[HAL_I2S_ID_1],sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_1]) <= 32 ? sizeof(i2s_tdm_tx_buf[HAL_I2S_ID_1]) : 32 );
    }
#endif
}

int32_t hal_i2s_tdm_open(enum HAL_I2S_ID_T i2s_id, enum HAL_I2S_MODE_T mode) {
  int ret;

  I2S_TDM_TRACE(3, "%s: i2s_id = %d,mode = %d.", __func__, i2s_id, mode);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);
  // i2s open playback and capture.
  ret = hal_i2s_open(i2s_id, AUD_STREAM_PLAYBACK, mode);
  if (ret) {
    I2S_TDM_TRACE(2, "%s: hal_i2s_open playback failed.ret = %d.", __func__,
                  ret);
    goto __func_fail;
  }
  ret = hal_i2s_open(i2s_id, AUD_STREAM_CAPTURE, mode);
  if (ret) {
    I2S_TDM_TRACE(2, "%s: hal_i2s_open capture failed.ret = %d.", __func__,
                  ret);
    goto __func_fail;
  }

  I2S_TDM_TRACE(1, "%s done.", __func__);
  return 0;

__func_fail:
  I2S_TDM_TRACE(2, "%s failed. ret = %d.", __func__, ret);
  return ret;
}

int32_t hal_i2s_tdm_setup(enum HAL_I2S_ID_T i2s_id, uint32_t sample_rate,
                          struct HAL_I2S_TDM_CONFIG_T *i2s_tdm_cfg) {
  int i, j, m, n;
  int ret;
  struct HAL_I2S_CONFIG_T i2s_cfg;
  uint16_t *buf;
  uint32_t cycles;
  uint32_t fs_cycles;
  uint32_t slot_cycles;
  uint32_t fs_remain;

  I2S_TDM_TRACE(3, "%s: i2s_id = %d,sample_rate = %d.", __func__, i2s_id,
                sample_rate);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);
  ASSERT(i2s_tdm_cycles_in_arrays(i2s_tdm_cfg->cycles), "%s: cycles(%d) error!",
         __func__, i2s_tdm_cfg->cycles);
  ASSERT(i2s_tdm_fs_cycles_in_arrays(i2s_tdm_cfg->fs_cycles),
         "%s: fs_cycles(%d) error!", __func__, i2s_tdm_cfg->fs_cycles);
  ASSERT(i2s_tdm_slot_cycles_in_arrays(i2s_tdm_cfg->slot_cycles),
         "%s: slot_cycles(%d) error!", __func__, i2s_tdm_cfg->slot_cycles);

  i2s_tdm_cfg[i2s_id] = *i2s_tdm_cfg;
  cycles = i2s_tdm_cfg->cycles;
  fs_cycles = i2s_tdm_cfg->fs_cycles == HAL_I2S_TDM_FS_CYCLES_ONE_LESS
                  ? cycles - 1
                  : i2s_tdm_cfg->fs_cycles;
  slot_cycles = i2s_tdm_cfg->slot_cycles;

  memset(&i2s_cfg, 0, sizeof(i2s_cfg));
  i2s_cfg.use_dma = true;
  i2s_cfg.sync_start = true;
  i2s_cfg.chan_sep_buf = false;
  i2s_cfg.bits = slot_cycles;
  i2s_cfg.channel_num = 2;
  i2s_cfg.channel_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
  i2s_cfg.sample_rate = (sample_rate * (cycles / i2s_cfg.bits)) / 2;
  ret = hal_i2s_setup_stream(i2s_id, AUD_STREAM_PLAYBACK, &i2s_cfg);
  if (ret) {
    I2S_TDM_TRACE(2, "%s: playback failed.ret = %d.", __func__, ret);
    goto __func_fail;
  }
  i2s_cfg.sync_start = false;
  ret = hal_i2s_setup_stream(i2s_id, AUD_STREAM_CAPTURE, &i2s_cfg);
  if (ret) {
    I2S_TDM_TRACE(1, "hal_i2s_setup_stream capture failed.ret = %d.", ret);
    goto __func_fail;
  }

  // Set tx buffer, output signal as slave device ws.
  I2S_TDM_TRACE(3, "%s: cycles = %d, fs_cycles = %d", __func__, cycles,
                fs_cycles);
  for (i = 0; i < I2S_TDM_TX_FRAME_NUM; i++) {
    buf = (uint16_t *)(&i2s_tdm_tx_buf[i2s_id][i][0]);
    for (j = 0; j < I2S_TDM_TX_FRAME_SIZE / (cycles / 16); j++) {
      fs_remain = fs_cycles;
      for (m = 0; m < (cycles / 16); m++) {
        buf[j * (cycles / 16) + m] = 0;
        for (n = 0; n < 16; n++) {
          if (fs_remain > 0) {
            buf[j * (cycles / 16) + m] |= (1 << n);
            fs_remain--;
          } else {
            break;
          }
        }
      }
    }
  }

  I2S_TDM_TRACE(1, "%s done.", __func__);
  return 0;

__func_fail:
  I2S_TDM_TRACE(2, "%s failed. ret = %d.", __func__, ret);
  return ret;
}

int32_t hal_i2s_tdm_start_stream(enum HAL_I2S_ID_T i2s_id) {
  uint32_t i;
  int ret;

  I2S_TDM_TRACE(2, "%s: i2s_id = %d.", __func__, i2s_id);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);

  memset(&tx_dma_cfg[i2s_id], 0, sizeof(tx_dma_cfg[i2s_id]));
  tx_dma_cfg[i2s_id].dst = 0; // useless
  tx_dma_cfg[i2s_id].dst_bsize = HAL_DMA_BSIZE_4;
  tx_dma_cfg[i2s_id].dst_periph =
      i2s_id == HAL_I2S_ID_0 ? HAL_AUDMA_I2S0_TX : HAL_AUDMA_I2S1_TX;
  tx_dma_cfg[i2s_id].dst_width = HAL_DMA_WIDTH_HALFWORD;
  tx_dma_cfg[i2s_id].handler =
      i2s_id == HAL_I2S_ID_0 ? i2s_tdm0_tx_handler : i2s_tdm1_tx_handler;
  tx_dma_cfg[i2s_id].src_bsize = HAL_DMA_BSIZE_4;
  tx_dma_cfg[i2s_id].src_tsize = (sizeof(i2s_tdm_tx_buf[i2s_id][0]) / 2);
  tx_dma_cfg[i2s_id].src_width = HAL_DMA_WIDTH_HALFWORD;
  tx_dma_cfg[i2s_id].try_burst = 1;
  tx_dma_cfg[i2s_id].type = HAL_DMA_FLOW_M2P_DMA;
  tx_dma_cfg[i2s_id].ch =
      hal_audma_get_chan(tx_dma_cfg[i2s_id].dst_periph, HAL_DMA_HIGH_PRIO);

  for (i = 0; i < I2S_TDM_TX_FRAME_NUM; i++) {
    tx_dma_cfg[i2s_id].src = (uint32_t)&i2s_tdm_tx_buf[i2s_id][i][0];
    ret = hal_audma_init_desc(
        &i2s_tdm_tx_dma_desc[i2s_id][i], &tx_dma_cfg[i2s_id],
        &i2s_tdm_tx_dma_desc[i2s_id][(i + 1) % I2S_TDM_TX_FRAME_NUM], 1);
    if (ret) {
      I2S_TDM_TRACE(2, "%s: hal_audma_init_desc tx failed.ret = %d.", __func__,
                    ret);
      goto __func_fail;
    }
  }

  ret =
      hal_audma_sg_start(&i2s_tdm_tx_dma_desc[i2s_id][0], &tx_dma_cfg[i2s_id]);
  if (ret) {
    I2S_TDM_TRACE(2, "%s: hal_audma_sg_start tx failed.ret = %d.", __func__,
                  ret);
    goto __func_fail;
  }

  // i2s start stream playback and capture.
  ret = hal_i2s_start_stream(i2s_id, AUD_STREAM_PLAYBACK);
  if (ret) {
    I2S_TDM_TRACE(2, "%s: hal_i2s_start_stream failed.ret = %d.", __func__,
                  ret);
  }

  ret = hal_i2s_start_stream(i2s_id, AUD_STREAM_CAPTURE);
  if (ret) {
    I2S_TDM_TRACE(1, "hal_i2s_start_stream tx failed.ret = %d.", ret);
    goto __func_fail;
  }

  I2S_TDM_TRACE(1, "%s done.", __func__);
  return ret;

__func_fail:
  I2S_TDM_TRACE(2, "%s failed.ret = %d.", __func__, ret);
  return ret;
}

int32_t hal_i2s_tdm_stop_stream(enum HAL_I2S_ID_T i2s_id) {
  I2S_TDM_TRACE(2, "%s: i2s_id = %d.", __func__, i2s_id);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);

  hal_dma_stop(tx_dma_cfg[i2s_id].ch);
  hal_i2s_stop_stream(i2s_id, AUD_STREAM_PLAYBACK);
  hal_i2s_stop_stream(i2s_id, AUD_STREAM_CAPTURE);

  I2S_TDM_TRACE(1, "%s done.", __func__);
  return 0;
}

int32_t hal_i2s_tdm_close(enum HAL_I2S_ID_T i2s_id) {
  I2S_TDM_TRACE(2, "%s: i2s_id = %d.", __func__, i2s_id);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);

  hal_i2s_close(i2s_id, AUD_STREAM_PLAYBACK);
  hal_i2s_close(i2s_id, AUD_STREAM_CAPTURE);

  I2S_TDM_TRACE(1, "%s done.", __func__);
  return 0;
}

void hal_i2s_tdm_get_config(enum HAL_I2S_ID_T i2s_id,
                            struct HAL_I2S_TDM_CONFIG_T *tdm_cfg) {
  I2S_TDM_TRACE(2, "%s: i2s_id = %d.", __func__, i2s_id);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);

  *tdm_cfg = i2s_tdm_cfg[i2s_id];
}

void hal_i2s_tdm_set_config(enum HAL_I2S_ID_T i2s_id,
                            struct HAL_I2S_TDM_CONFIG_T *tdm_cfg) {
  I2S_TDM_TRACE(2, "%s: i2s_id = %d.", __func__, i2s_id);
  ASSERT(i2s_id < HAL_I2S_ID_QTY, "%s: i2s_id = %d!", __func__, i2s_id);

  i2s_tdm_cfg[i2s_id] = *tdm_cfg;
}
