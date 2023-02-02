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
#include "plat_addr_map.h"

#ifdef BTPCM_BASE

#include "hal_btpcm.h"
#include "hal_btpcmip.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "plat_types.h"
#include "reg_btpcmip.h"

//#define BTPCM_CLOCK_SOURCE 240000000
#define BTPCM_CLOCK_SOURCE 22579200
//#define BTPCM_CLOCK_SOURCE 48000000
//#define BTPCM_CLOCK_SOURCE 3072000

//#define BTPCM_CLOCK_SOURCE 76800000
//#define BTPCM_CLOCK_SOURCE 84672000

//#define HAL_BTPCM_TX_FIFO_TRIGGER_LEVEL   (BTPCMIP_FIFO_DEPTH/2)
//#define HAL_BTPCM_RX_FIFO_TRIGGER_LEVEL   (BTPCMIP_FIFO_DEPTH/2)

#define HAL_BTPCM_TX_FIFO_TRIGGER_LEVEL (1)
#define HAL_BTPCM_RX_FIFO_TRIGGER_LEVEL (0)

#define HAL_BTPCM_YES 1
#define HAL_BTPCM_NO 0

static const char *const invalid_id = "Invalid BTPCM ID: %d\n";
// static const char * const invalid_ch = "Invalid BTPCM CH: %d\n";

static bool btpcm_opened[HAL_BTPCM_ID_NUM][AUD_STREAM_NUM];

static inline POSSIBLY_UNUSED unsigned char reverse(unsigned char in) {
  uint8_t out = 0;
  uint32_t i = 0;
  for (i = 0; i < 8; ++i) {
    if ((1 << i & in))
      out = out | (1 << (7 - i));
    else
      out = out & (~(1 << (7 - i)));
  }

  return out;
}

static inline uint32_t _btpcm_get_reg_base(enum HAL_BTPCM_ID_T id) {
  ASSERT(id < HAL_BTPCM_ID_NUM, invalid_id, id);

  switch (id) {
  case HAL_BTPCM_ID_0:
  default:
    return BTPCM_BASE;
    break;
  }
  return 0;
}

int hal_btpcm_open(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream) {
  uint32_t reg_base;

  reg_base = _btpcm_get_reg_base(id);

  if (!btpcm_opened[id][AUD_STREAM_PLAYBACK] &&
      !btpcm_opened[id][AUD_STREAM_CAPTURE]) {
    // Init to slave mode before enabling module clock
    // Make sure the clock mux is switched when there is no clock active
    hal_cmu_pcm_set_slave_mode(true);
    hal_cmu_pcm_clock_enable();

    hal_cmu_clock_enable(HAL_CMU_MOD_O_PCM);
    hal_cmu_clock_enable(HAL_CMU_MOD_P_PCM);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_PCM);
    hal_cmu_reset_clear(HAL_CMU_MOD_P_PCM);

    // After release pclk reset, need delay 2 tick to access any pcm reg.
    hal_sys_timer_delay(2);
    btpcmip_pcm_clk_open_en(reg_base, 0);

    btpcmip_w_enable_rx_dma(reg_base, HAL_BTPCM_NO);
    btpcmip_w_enable_tx_dma(reg_base, HAL_BTPCM_NO);

    btpcmip_flush_tx_fifo(reg_base);
    btpcmip_flush_rx_fifo(reg_base);

    btpcmip_w_slot_sel(reg_base, 0);
    btpcmip_w_shortsync(reg_base, HAL_BTPCM_YES);
    btpcmip_w_signin(reg_base, HAL_BTPCM_YES);
    btpcmip_w_mask1mask2(reg_base, 0);
    btpcmip_w_enable_btpcmip(reg_base, HAL_BTPCM_YES);

    // Generate 10 clock cycles for BTPCM module to initialize its states
    for (int i = 0; i < 10; i++) {
      hal_cmu_pcm_set_slave_mode(false);
      hal_cmu_pcm_set_slave_mode(true);
    }

    // btpcmip_w_length(reg_base, 0x05);         //16 bit
  }

  btpcm_opened[id][stream] = true;

  return 0;
}

int hal_btpcm_close(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream) {
  uint32_t reg_base;

  reg_base = _btpcm_get_reg_base(id);

  btpcm_opened[id][stream] = false;

  if (!btpcm_opened[id][AUD_STREAM_PLAYBACK] &&
      !btpcm_opened[id][AUD_STREAM_CAPTURE]) {
    btpcmip_w_enable_btpcmip(reg_base, HAL_BTPCM_NO);

    // don't need flush rx and tx fifo

    hal_cmu_reset_set(HAL_CMU_MOD_P_PCM);
    hal_cmu_reset_set(HAL_CMU_MOD_O_PCM);
    hal_cmu_clock_disable(HAL_CMU_MOD_P_PCM);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_PCM);
    hal_cmu_pcm_clock_disable();
  }

  return 0;
}

int hal_btpcm_start_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream) {
  return 0;
}

int hal_btpcm_stop_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream) {
  return 0;
}

int hal_btpcm_setup_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream,
                           struct HAL_BTPCM_CONFIG_T *cfg) {
  uint32_t reg_base;

  reg_base = _btpcm_get_reg_base(id);

  if (!btpcm_opened[id][stream]) {
    return 1;
  }

  if (stream == AUD_STREAM_PLAYBACK) {
    // dma config
    btpcmip_w_tx_fifo_threshold(reg_base, HAL_BTPCM_TX_FIFO_TRIGGER_LEVEL);
    btpcmip_w_enable_tx_dma(reg_base, HAL_BTPCM_YES);
  } else {
    // dma config
    btpcmip_w_rx_fifo_threshold(reg_base, HAL_BTPCM_RX_FIFO_TRIGGER_LEVEL);
    btpcmip_w_enable_rx_dma(reg_base, HAL_BTPCM_YES);
  }

  // hal_sys_timer_delay(2);

  btpcmip_pcm_clk_open_en(reg_base, 1);

  return 0;
}

int hal_btpcm_send(enum HAL_BTPCM_ID_T id, uint8_t *value, uint32_t value_len) {
  uint32_t i = 0;
  uint32_t reg_base;

  reg_base = _btpcm_get_reg_base(id);

  for (i = 0; i < value_len; i += 4) {
    while (!(btpcmip_r_int_status(reg_base) &
             BTPCMIP_INT_STATUS_TX_FIFO_EMPTY_MASK))
      ;

    btpcmip_w_tx_fifo(reg_base, value[i + 1] << 8 | value[i]);
  }

  return 0;
}
uint8_t hal_btpcm_recv(enum HAL_BTPCM_ID_T id, uint8_t *value,
                       uint32_t value_len) {
  ASSERT(id < HAL_BTPCM_ID_NUM, invalid_id, id);
  return 0;
}

#endif
