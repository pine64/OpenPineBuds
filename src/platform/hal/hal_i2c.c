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
#include "hal_i2c.h"
#include "cmsis_nvic.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_i2cip.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "string.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif

#ifdef I2C_DEBUG
#define HAL_I2C_TRACE(attr, str, ...) TRACE(attr, str, ##__VA_ARGS__)
#define HAL_I2C_ERROR(attr, str, ...) TRACE(attr, str, ##__VA_ARGS__)
#else
#define HAL_I2C_TRACE(attr, str, ...) TRACE_DUMMY(attr, str, ##__VA_ARGS__)
#define HAL_I2C_ERROR(attr, str, ...) TRACE(attr, str, ##__VA_ARGS__)
#endif

#ifdef I2C_SENSOR_ENGINE
#ifndef I2C_USE_DMA
#define I2C_USE_DMA
#endif
#endif

#define HAL_I2C_TX_TL I2CIP_TX_TL_QUARTER
#define HAL_I2C_RX_TL I2CIP_RX_TL_THREE_QUARTER

#define HAL_I2C_DMA_TX_TL I2CIP_TX_TL_HALF
#define HAL_I2C_DMA_RX_TL I2CIP_DMA_TX_TL_1_BYTE

#define HAL_I2C_DMA_TX 1
#define HAL_I2C_DMA_RX 0

#define HAL_I2C_RESTART 1
#define HAL_I2C_NO_RESTART 0

#define HAL_I2C_YES 1
#define HAL_I2C_NO 0

/* state machine */
#ifdef I2C_SM_TASK_NUM
#define HAL_I2C_SM_TASK_NUM_MAX I2C_SM_TASK_NUM
#else
#define HAL_I2C_SM_TASK_NUM_MAX 2
#endif

#ifdef I2C_SM_DMA_BUF_SIZE
#define HAL_I2C_SM_DMA_BUF_LEN_MAX I2C_SM_DMA_BUF_SIZE
#else
#define HAL_I2C_SM_DMA_BUF_LEN_MAX 1024
#endif

/* i2c synchronization timeout ms */
#define HAL_I2C_SYNC_TM_MS (2000)
#define HAL_I2C_DLY_MS (1)
#define HAL_I2C_WAIT_ACT_MS (1000)
#define HAL_I2C_WAIT_TFE_MS (1000)
#define HAL_I2C_WAIT_TFNF_MS (1000)
#define HAL_I2C_WAIT_RFNE_MS (1000)

enum HAL_I2C_SM_STATE_T {
  HAL_I2C_SM_CLOSED = 0,
  HAL_I2C_SM_IDLE,
  HAL_I2C_SM_RUNNING,
};

enum HAL_I2C_SM_TASK_ACTION_T {
  HAL_I2C_SM_TASK_ACTION_NONE = 0,
  HAL_I2C_SM_TASK_ACTION_M_SEND,
  HAL_I2C_SM_TASK_ACTION_M_RECV,
};

enum HAL_I2C_SM_TASK_STATE_T {
  HAL_I2C_SM_TASK_STATE_START = (1 << 0),
  HAL_I2C_SM_TASK_STATE_STOP = (1 << 1),
  HAL_I2C_SM_TASK_STATE_TX_ABRT = (1 << 2),
  HAL_I2C_SM_TASK_STATE_ACT = (1 << 3),
  HAL_I2C_SM_TASK_STATE_FIFO_ERR = (1 << 4),
};

struct HAL_I2C_SM_TASK_T {
  /* send or recv buffer */
  const uint8_t *tx_buf;
  uint8_t *rx_buf;
  uint16_t tx_txn_len;
  uint16_t rx_txn_len;
  uint16_t txn_cnt;
  uint16_t tx_pos;
  uint16_t rx_pos;
  uint16_t rx_cmd_sent;

  /* device control */
  uint8_t stop;
  uint8_t restart_after_write;
  uint16_t target_addr;

  /* task control */
  uint32_t state;
  uint32_t action;

  /* system control */
  /* TODO : os and non os version */
  volatile uint32_t lock;
  uint32_t transfer_id;
  uint32_t errcode;
  HAL_I2C_TRANSFER_HANDLER_T handler;
};

struct HAL_I2C_SM_T {
  /* device related */
  struct HAL_I2C_CONFIG_T cfg;

  enum HAL_I2C_SM_STATE_T state;

  /* state machine related */
#if defined(I2C_TASK_MODE) || defined(I2C_SENSOR_ENGINE)
  uint8_t in_task;
  uint8_t out_task;
  uint8_t task_count;
  struct HAL_I2C_SM_TASK_T task[HAL_I2C_SM_TASK_NUM_MAX];

  /* dma related */
#ifdef I2C_USE_DMA
  struct HAL_DMA_CH_CFG_T tx_dma_cfg;
  struct HAL_DMA_CH_CFG_T rx_dma_cfg;
  /* i2cip cmd_data use 16bit and read action is driven by write action */
  /* when use dma to read from i2c, we need to use another dma to write
   * cmd/stop/restart */
  /* cmd/stop/restart + data use 16bit width, so dma buffer are 2 times of orgin
   * data buffer */
  uint16_t dma_tx_buf[HAL_I2C_SM_DMA_BUF_LEN_MAX / 2];
#endif
#endif
};
/* state machine end */

struct HAL_I2C_MOD_NAME_T {};

struct HAL_I2C_HW_DESC_T {
  uint32_t base;
  enum HAL_CMU_MOD_ID_T mod;
  enum HAL_CMU_MOD_ID_T apb;
  IRQn_Type irq;
#ifdef I2C_USE_DMA
  enum HAL_DMA_PERIPH_T rx_periph;
  enum HAL_DMA_PERIPH_T tx_periph;
#endif
};

static const struct HAL_I2C_HW_DESC_T i2c_desc[HAL_I2C_ID_NUM] = {
    {
        .base = I2C0_BASE,
        .mod = HAL_CMU_MOD_O_I2C0,
        .apb = HAL_CMU_MOD_P_I2C0,
        .irq = I2C0_IRQn,
#ifdef I2C_USE_DMA
        .rx_periph = HAL_GPDMA_I2C0_RX,
        .tx_periph = HAL_GPDMA_I2C0_TX,
#endif
    },
#if (CHIP_HAS_I2C > 1)
    {
        .base = I2C1_BASE,
        .mod = HAL_CMU_MOD_O_I2C1,
        .apb = HAL_CMU_MOD_P_I2C1,
        .irq = I2C1_IRQn,
#ifdef I2C_USE_DMA
        .rx_periph = HAL_GPDMA_I2C1_RX,
        .tx_periph = HAL_GPDMA_I2C1_TX,
#endif
    },
#endif
};

static const char *const invalid_id = "Invalid I2C ID: %d";

static struct HAL_I2C_SM_T hal_i2c_sm[HAL_I2C_ID_NUM];

/* simple mode */
#ifdef I2C_SIMPLE_MODE
static void hal_i2c_simple_proc(enum HAL_I2C_ID_T id);

static HAL_I2C_INT_HANDLER_T hal_i2c_int_handlers[HAL_I2C_ID_NUM] = {NULL};
#endif
/* simple mode end */

#ifdef I2C_SENSOR_ENGINE
static enum HAL_SENSOR_ENGINE_ID_T i2c_sensor_id[HAL_I2C_ID_NUM];
static HAL_I2C_SENSOR_ENG_HANDLER_T i2c_sensor_handler[HAL_I2C_ID_NUM];

#ifdef I2C_SENSOR_ENGINE
static void hal_i2c_sensor_eng_proc(enum HAL_I2C_ID_T id);
#endif
#endif

static uint32_t _i2c_get_base(enum HAL_I2C_ID_T id) {
  return i2c_desc[id].base;
}

static void POSSIBLY_UNUSED hal_i2c_delay_ms(int ms) { osDelay(ms); }

static uint32_t _i2c_adjust_period_cnt(uint32_t period_cnt, uint16_t trising_ns,
                                       uint16_t tfalling_ns,
                                       uint16_t pclk_mhz) {
  uint32_t old_period_cnt;
  uint16_t rising_falling_cycle;

  old_period_cnt = period_cnt;

  // Round down the rising and falling cycle, so that the period count is
  // rounded up, and the SCL freq is always <= the requested freq.
  rising_falling_cycle = ((trising_ns + tfalling_ns) * pclk_mhz) / 1000;

  if (period_cnt > rising_falling_cycle) {
    period_cnt -= rising_falling_cycle;
  } else {
    period_cnt = 0;
  }

  HAL_I2C_TRACE(5, "%s: period_cnt=%u->%u trising_ns=%u tfalling_ns=%u",
                __FUNCTION__, old_period_cnt, period_cnt, trising_ns,
                tfalling_ns);

  return period_cnt;
}

static void _i2c_get_clk_cnt(uint32_t period_cnt, uint16_t tlow_ns,
                             uint16_t thigh_ns, uint16_t spklen,
                             uint16_t pclk_mhz, uint16_t *plcnt,
                             uint16_t *phcnt) {
#define IC_SCL_LOW_CYCLE_ADD (1)
// NOTE: H/w spec says that (6 + spklen) cycles is added for SCL high interval,
// but tests show that 1 more cycle is needed
#define IC_SCL_HIGH_CYCLE_ADD (6 + spklen + 1)

#define MIN_IC_SCL_LCNT (7 + spklen + IC_SCL_LOW_CYCLE_ADD)
#define MIN_IC_SCL_HCNT (5 + spklen + IC_SCL_HIGH_CYCLE_ADD)

  uint32_t lcnt, hcnt;
  uint16_t min_lcnt, min_hcnt;

  HAL_I2C_TRACE(
      6, "%s: period_cnt=%u tlow_ns=%u thigh_ns=%u spklen=%u pclk_mhz=%u",
      __FUNCTION__, period_cnt, tlow_ns, thigh_ns, spklen, pclk_mhz);

  min_lcnt = (tlow_ns * pclk_mhz + 1000 - 1) / 1000;
  if (min_lcnt < MIN_IC_SCL_LCNT) {
    min_lcnt = MIN_IC_SCL_LCNT;
  }
  min_hcnt = (thigh_ns * pclk_mhz + 1000 - 1) / 1000;
  if (min_hcnt < MIN_IC_SCL_HCNT) {
    min_hcnt = MIN_IC_SCL_HCNT;
  }

  if (min_lcnt + min_hcnt > period_cnt) {
    HAL_I2C_ERROR(5,
                  "%s:WARNING: period_cnt=%u too small: min_lcnt=%u "
                  "min_hcnt=%u pclk_mhz=%u",
                  __FUNCTION__, period_cnt, min_lcnt, min_hcnt, pclk_mhz);

    lcnt = min_lcnt;
    hcnt = min_hcnt;
  } else {
    lcnt = (period_cnt + 1) / 2;
    if (min_lcnt >= min_hcnt) {
      if (lcnt < min_lcnt) {
        lcnt = min_lcnt;
      }
      hcnt = period_cnt - lcnt;
      if (hcnt < min_hcnt) {
        hcnt = min_hcnt;
      }
    } else {
      hcnt = lcnt;
      if (hcnt < min_hcnt) {
        hcnt = min_hcnt;
      }
      lcnt = period_cnt - hcnt;
      if (lcnt < min_lcnt) {
        lcnt = min_lcnt;
      }
    }
  }

  lcnt -= IC_SCL_LOW_CYCLE_ADD;
  hcnt -= IC_SCL_HIGH_CYCLE_ADD;

  ASSERT(lcnt <= UINT16_MAX && hcnt <= UINT16_MAX,
         "%s: lcnt=%u or hcnt=%u overflow", __FUNCTION__, lcnt, hcnt);

  *plcnt = lcnt;
  *phcnt = hcnt;
}

static void _i2c_set_speed(enum HAL_I2C_ID_T id, int speed_mode, int speed) {
#define MAX_HS_SPK_NS 10
#define MAX_FSP_SPK_NS 50
#define MAX_FS_SPK_NS 50

#define SS_THOLD_NS 1800     // [300, 3450]
#define FS_THOLD_NS 600      // [300,  900]
#define FSP_THOLD_NS 300     // [  0,    -]
#define HS_100PF_THOLD_NS 30 // [  0,   70]
#define HS_400PF_THOLD_NS 70 // [  0,  150]

#define SS_TRISING_NS 300      // [  0, 1000]
#define FS_TRISING_NS 50       // [ 20,  300]
#define FSP_TRISING_NS 30      // [  0,  120]
#define HS_100PF_TRISING_NS 30 // [ 10,   40] for SCL, [ 20,   80] for SDA
#define HS_400PF_TRISING_NS 30 // [ 10,   80] for SCL, [ 20,  160] for SDA

#define SS_TFALLING_NS 30       // [  0,  300]
#define FS_TFALLING_NS 30       // [ 20,  300]
#define FSP_TFALLING_NS 30      // [ 20,  120]
#define HS_100PF_TFALLING_NS 30 // [ 10,   40] for SCL, [ 20,   80] for SDA
#define HS_400PF_TFALLING_NS 30 // [ 10,   80] for SCL, [ 20,  160] for SDA

#define MIN_SS_TLOW_NS 4700
#define MIN_SS_THIGH_NS 4000
#define MIN_FS_TLOW_NS 1300
#define MIN_FS_THIGH_NS 600
#define MIN_FSP_TLOW_NS 500
#define MIN_FSP_THIGH_NS 260
#define MIN_HS_100PF_TLOW_NS 160
#define MIN_HS_100PF_THIGH_NS 60
#define MIN_HS_400PF_TLOW_NS 320
#define MIN_HS_400PF_THIGH_NS 120

// Round down the spike suppression limit value
#define GET_SPKLEN_VAL(s) ((s)*pclk_mhz / 1000)

  uint32_t reg_base = _i2c_get_base(id);
  uint32_t min_mclk, pclk, period_cnt;
  uint16_t lcnt, hcnt, hold_cycle, spklen;
  uint16_t tlow_ns, thigh_ns, thold_ns, trising_ns, tfalling_ns;
  uint8_t spk_ns;
  uint16_t pclk_mhz;

  if (speed_mode == IC_SPEED_MODE_MAX) {
    min_mclk = speed * 50;
  } else {
    min_mclk = speed * 40;
  }
  pclk = 0;
#ifdef PERIPH_PLL_FREQ
  if (PERIPH_PLL_FREQ / 2 > 2 * hal_cmu_get_crystal_freq()) {
    // Init to OSC_X2
    pclk = 2 * hal_cmu_get_crystal_freq();
    if (min_mclk > pclk) {
      pclk = PERIPH_PLL_FREQ / 2;
      hal_cmu_i2c_set_div(2);
    } else {
      pclk = 0;
    }
  }
#endif
  if (pclk == 0) {
    enum HAL_CMU_PERIPH_FREQ_T periph_freq;

    // Init to OSC
    pclk = hal_cmu_get_crystal_freq();
    if (min_mclk > pclk) {
      pclk *= 2;
      periph_freq = HAL_CMU_PERIPH_FREQ_52M;
    } else {
      periph_freq = HAL_CMU_PERIPH_FREQ_26M;
    }

    // NOTE: All I2C controllers share the same module clock configuration
    hal_cmu_i2c_set_freq(periph_freq);
  }

  pclk_mhz = pclk / 1000000;
  period_cnt = (pclk + speed - 1) / speed;

  switch (speed_mode) {
  case IC_SPEED_MODE_MAX:

    spk_ns = MAX_HS_SPK_NS;
    tlow_ns = MIN_HS_100PF_TLOW_NS;
    thigh_ns = MIN_HS_100PF_THIGH_NS;
    thold_ns = HS_100PF_THOLD_NS;
    trising_ns = HS_100PF_TRISING_NS;
    tfalling_ns = HS_100PF_TFALLING_NS;

    spklen = GET_SPKLEN_VAL(spk_ns);
    if (spklen == 0) {
      spklen = 1;
    }
    i2cip_w_hs_spklen(reg_base, spklen);

    period_cnt =
        _i2c_adjust_period_cnt(period_cnt, trising_ns, tfalling_ns, pclk_mhz);
    _i2c_get_clk_cnt(period_cnt, tlow_ns, thigh_ns, spklen, pclk_mhz, &lcnt,
                     &hcnt);

    i2cip_w_high_speed_hcnt(reg_base, hcnt);
    i2cip_w_high_speed_lcnt(reg_base, lcnt);

    i2cip_w_speed(reg_base, I2CIP_HIGH_SPEED_MASK);

    // Continue to config fast mode
#ifdef I2S_FSP_MODE
    period_cnt = (pclk + I2C_FSP_SPEED - 1) / I2C_FSP_SPEED;
#else
    period_cnt = (pclk + I2C_FAST_SPEED - 1) / I2C_FAST_SPEED;
#endif

  case IC_SPEED_MODE_FAST:

#ifdef I2S_FSP_MODE
    if (speed > I2C_FAST_SPEED) {
      spk_ns = MAX_FSP_SPK_NS;
      tlow_ns = MIN_FSP_TLOW_NS;
      thigh_ns = MIN_FSP_THIGH_NS;
      thold_ns = FSP_THOLD_NS;
      trising_ns = FSP_TRISING_NS;
      tfalling_ns = FSP_TFALLING_NS;
    } else
#endif
    {
      spk_ns = MAX_FS_SPK_NS;
      tlow_ns = MIN_FS_TLOW_NS;
      thigh_ns = MIN_FS_THIGH_NS;
      trising_ns = FS_TRISING_NS;
      tfalling_ns = FS_TFALLING_NS;
    }

    spklen = GET_SPKLEN_VAL(spk_ns);
    if (spklen == 0) {
      spklen = 1;
    }
    i2cip_w_fs_spklen(reg_base, spklen);

    period_cnt =
        _i2c_adjust_period_cnt(period_cnt, trising_ns, tfalling_ns, pclk_mhz);
    _i2c_get_clk_cnt(period_cnt, tlow_ns, thigh_ns, spklen, pclk_mhz, &lcnt,
                     &hcnt);

    i2cip_w_fast_speed_hcnt(reg_base, hcnt);
    i2cip_w_fast_speed_lcnt(reg_base, lcnt);

    // Update sda hold time and speed mode if in fast mode
    // Skip if in high speed mode
    if (speed_mode == IC_SPEED_MODE_FAST) {
      thold_ns = FS_THOLD_NS;
      i2cip_w_speed(reg_base, I2CIP_FAST_SPEED_MASK);
    }

    break;

  case IC_SPEED_MODE_STANDARD:
  default:

    spk_ns = MAX_FS_SPK_NS;
    tlow_ns = MIN_SS_TLOW_NS;
    thigh_ns = MIN_SS_THIGH_NS;
    thold_ns = SS_THOLD_NS;
    trising_ns = SS_TRISING_NS;
    tfalling_ns = SS_TFALLING_NS;

    spklen = GET_SPKLEN_VAL(spk_ns);
    if (spklen == 0) {
      spklen = 1;
    }
    i2cip_w_fs_spklen(reg_base, spklen);

    period_cnt =
        _i2c_adjust_period_cnt(period_cnt, trising_ns, tfalling_ns, pclk_mhz);
    _i2c_get_clk_cnt(period_cnt, tlow_ns, thigh_ns, spklen, pclk_mhz, &lcnt,
                     &hcnt);

    i2cip_w_standard_speed_hcnt(reg_base, hcnt);
    i2cip_w_standard_speed_lcnt(reg_base, lcnt);
    i2cip_w_speed(reg_base, I2CIP_STANDARD_SPEED_MASK);
    break;
  }

  // Master mode: min = 1; slave mode: min = (spklen + 7)
  hold_cycle = (thold_ns * pclk_mhz + 1000 - 1) / 1000;
  i2cip_w_sda_hold_time(reg_base, hold_cycle);

  HAL_I2C_TRACE(1, "crystal freq=%d", pclk);
  HAL_I2C_TRACE(5, "i2c-%d mode=%d lcnt=%d hcnt=%d hold=%d", id, speed_mode,
                lcnt, hcnt, hold_cycle);
}

static uint8_t _i2c_set_bus_speed(enum HAL_I2C_ID_T id, unsigned int speed) {
  uint32_t speed_mode;

  if (speed > I2C_FSP_SPEED)
    speed_mode = IC_SPEED_MODE_MAX;
  else if (speed > I2C_FAST_SPEED)
#ifdef I2S_FSP_MODE
    speed_mode = IC_SPEED_MODE_FAST;
#else
    speed_mode = IC_SPEED_MODE_MAX;
#endif
  else if (speed > I2C_STANDARD_SPEED)
    speed_mode = IC_SPEED_MODE_FAST;
  else
    speed_mode = IC_SPEED_MODE_STANDARD;

  _i2c_set_speed(id, speed_mode, speed);

  return 0;
}

/* state machine related */
static void hal_i2c_sm_init(enum HAL_I2C_ID_T id,
                            const struct HAL_I2C_CONFIG_T *cfg) {
  memcpy(&hal_i2c_sm[id].cfg, cfg, sizeof(*cfg));
  hal_i2c_sm[id].state = HAL_I2C_SM_IDLE;
#if defined(I2C_TASK_MODE) || defined(I2C_SENSOR_ENGINE)
  hal_i2c_sm[id].in_task = 0;
  hal_i2c_sm[id].out_task = 0;
  hal_i2c_sm[id].task_count = 0;

#ifdef I2C_USE_DMA
  hal_i2c_sm[id].tx_dma_cfg.ch = HAL_DMA_CHAN_NONE;
  hal_i2c_sm[id].rx_dma_cfg.ch = HAL_DMA_CHAN_NONE;
#else
  hal_i2c_sm[id].cfg.use_dma = 0;
#endif
#endif
}

#if defined(I2C_TASK_MODE) || defined(I2C_SENSOR_ENGINE)
static uint32_t hal_i2c_sm_commit(enum HAL_I2C_ID_T id, const uint8_t *tx_buf,
                                  uint16_t tx_txn_len, uint8_t *rx_buf,
                                  uint16_t rx_txn_len, uint16_t txn_cnt,
                                  uint16_t target_addr, uint32_t action,
                                  uint32_t transfer_id,
                                  HAL_I2C_TRANSFER_HANDLER_T handler) {
  uint32_t cur = hal_i2c_sm[id].in_task;

  hal_i2c_sm[id].task[cur].tx_buf = tx_buf;
  hal_i2c_sm[id].task[cur].rx_buf = rx_buf;
  hal_i2c_sm[id].task[cur].stop = 1;
  hal_i2c_sm[id].task[cur].lock = 1;
  hal_i2c_sm[id].task[cur].state = 0;
  hal_i2c_sm[id].task[cur].rx_txn_len = rx_txn_len;
  hal_i2c_sm[id].task[cur].tx_txn_len = tx_txn_len;
  hal_i2c_sm[id].task[cur].txn_cnt = txn_cnt;
  hal_i2c_sm[id].task[cur].tx_pos = 0;
  hal_i2c_sm[id].task[cur].rx_pos = 0;
  hal_i2c_sm[id].task[cur].rx_cmd_sent = 0;
  hal_i2c_sm[id].task[cur].errcode = 0;
  hal_i2c_sm[id].task[cur].action = action;
  hal_i2c_sm[id].task[cur].handler = handler;
  hal_i2c_sm[id].task[cur].restart_after_write = 1;
  hal_i2c_sm[id].task[cur].target_addr = target_addr;
  hal_i2c_sm[id].task[cur].transfer_id = transfer_id;

  hal_i2c_sm[id].in_task = (cur + 1) % HAL_I2C_SM_TASK_NUM_MAX;
  hal_i2c_sm[id].task_count++;
  return cur;
}

static void hal_i2c_sm_done(enum HAL_I2C_ID_T id) {
  hal_i2c_sm[id].in_task = 0;
  hal_i2c_sm[id].out_task = 0;
  hal_i2c_sm[id].task_count = 0;
  hal_i2c_sm[id].state = HAL_I2C_SM_IDLE;

  HAL_I2C_TRACE(1, "%s", __func__);
}

static enum HAL_I2C_SM_TASK_STATE_T
_i2c_chk_clr_task_error(uint32_t reg_base, uint32_t ip_int_status,
                        uint32_t tx_abrt_source) {
  enum HAL_I2C_SM_TASK_STATE_T state = 0;
  uint32_t tmp1;

  /* tx abort interrupt */
  if (ip_int_status & I2CIP_INT_STATUS_TX_ABRT_MASK) {
    state |= HAL_I2C_SM_TASK_STATE_TX_ABRT;

    /* sbyte_norstrt is special to clear : restart disable but user want to send
     * a restart */
    /* to fix this bit : enable restart , clear speical , clear gc_or_start bit
     * temporary */
    tmp1 = i2cip_r_target_address_reg(reg_base);
    if (tx_abrt_source & I2CIP_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT_MASK) {
      i2cip_w_restart(reg_base, HAL_I2C_YES);
      i2cip_w_special_bit(reg_base, HAL_I2C_NO);
      i2cip_w_gc_or_start_bit(reg_base, HAL_I2C_NO);
    }
    i2cip_r_clr_tx_abrt(reg_base);
    /* restore register after clear */
    if (tx_abrt_source & I2CIP_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT_MASK) {
      i2cip_w_target_address_reg(reg_base, tmp1);
    }
  }
  /* tx overflow interrupt */
  if (ip_int_status & I2CIP_INT_MASK_TX_OVER_MASK) {
    state |= HAL_I2C_SM_TASK_STATE_FIFO_ERR;
    i2cip_r_clr_tx_over(reg_base);
  }
  /* rx overflow interrupt */
  if (ip_int_status & I2CIP_INT_MASK_RX_OVER_MASK) {
    state |= HAL_I2C_SM_TASK_STATE_FIFO_ERR;
    i2cip_r_clr_rx_over(reg_base);
  }
  /* rx underflow interrupt */
  if (ip_int_status & I2CIP_INT_MASK_RX_UNDER_MASK) {
    state |= HAL_I2C_SM_TASK_STATE_FIFO_ERR;
    i2cip_r_clr_rx_under(reg_base);
  }

  return state;
}

static void _i2c_show_error_code(uint32_t errcode) {
#ifdef DEBUG
  if (errcode & HAL_I2C_ERRCODE_SLVRD_INTX)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_SLVRD_INTX");
  if (errcode & HAL_I2C_ERRCODE_SLV_ARBLOST)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_SLV_ARBLOST");
  if (errcode & HAL_I2C_ERRCODE_SLVFLUSH_TXFIFO)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_SLVFLUSH_TXFIFO");
  if (errcode & HAL_I2C_ERRCODE_MASTER_DIS)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_MASTER_DIS");
  if (errcode & HAL_I2C_ERRCODE_10B_RD_NORSTRT)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_10B_RD_NORSTRT");
  if (errcode & HAL_I2C_ERRCODE_SBYTE_NORSTRT)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_SBYTE_NORSTRT");
  if (errcode & HAL_I2C_ERRCODE_HS_NORSTRT)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_HS_NORSTRT");
  if (errcode & HAL_I2C_ERRCODE_SBYTE_ACKDET)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_SBYTE_ACKDET");
  if (errcode & HAL_I2C_ERRCODE_HS_ACKDET)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_HS_ACKDET");
  if (errcode & HAL_I2C_ERRCODE_GCALL_READ)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_GCALL_READ");
  if (errcode & HAL_I2C_ERRCODE_GCALL_NOACK)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_GCALL_NOACK");
  if (errcode & HAL_I2C_ERRCODE_TXDATA_NOACK)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_TXDATA_NOACK");
  if (errcode & HAL_I2C_ERRCODE_10ADDR2_NOACK)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_10ADDR2_NOACK");
  if (errcode & HAL_I2C_ERRCODE_10ADDR1_NOACK)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_10ADDR1_NOACK");
  if (errcode & HAL_I2C_ERRCODE_7B_ADDR_NOACK)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_7B_ADDR_NOACK");

  if (errcode & HAL_I2C_ERRCODE_INV_PARAM)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_INV_PARAM");
  if (errcode & HAL_I2C_ERRCODE_IN_USE)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_IN_USE");
  if (errcode & HAL_I2C_ERRCODE_FIFO_ERR)
    HAL_I2C_ERROR(0, "i2c err : HAL_I2C_ERRCODE_FIFO_ERR");
#endif
}

#ifdef I2C_USE_DMA
static void hal_i2c_tx_dma_handler(uint8_t chan, uint32_t remain_tsize,
                                   uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_I2C_ID_T id;

  for (id = HAL_I2C_ID_0; id < HAL_I2C_ID_NUM; id++) {
    if (hal_i2c_sm[id].tx_dma_cfg.ch == chan) {
    }
  }
}

static void hal_i2c_rx_dma_handler(uint8_t chan, uint32_t remain_tsize,
                                   uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_I2C_ID_T id;

  for (id = HAL_I2C_ID_0; id < HAL_I2C_ID_NUM; id++) {
    if (hal_i2c_sm[id].rx_dma_cfg.ch == chan) {
    }
  }
}

static void hal_i2c_dma_release(enum HAL_I2C_ID_T id) {
  uint32_t reg_base;
  struct HAL_DMA_CH_CFG_T *tx_dma_cfg = NULL, *rx_dma_cfg = NULL;

  if (hal_i2c_sm[id].cfg.use_dma == 0) {
    return;
  }

  reg_base = _i2c_get_base(id);

  i2cip_w_tx_dma_enable(reg_base, HAL_I2C_NO);
  i2cip_w_rx_dma_enable(reg_base, HAL_I2C_NO);

  tx_dma_cfg = &hal_i2c_sm[id].tx_dma_cfg;
  rx_dma_cfg = &hal_i2c_sm[id].rx_dma_cfg;

  HAL_I2C_TRACE(1, "i2c tx free dma ch %d", tx_dma_cfg->ch);
  if (tx_dma_cfg->ch != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(tx_dma_cfg->ch);
    hal_gpdma_free_chan(tx_dma_cfg->ch);
    tx_dma_cfg->ch = HAL_DMA_CHAN_NONE;
  }

  HAL_I2C_TRACE(1, "i2c rx free dma ch %d", rx_dma_cfg->ch);
  if (rx_dma_cfg->ch != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(rx_dma_cfg->ch);
    hal_gpdma_free_chan(rx_dma_cfg->ch);
    rx_dma_cfg->ch = HAL_DMA_CHAN_NONE;
  }
}

static void hal_i2c_dma_config(enum HAL_I2C_ID_T id,
                               const struct HAL_I2C_SM_TASK_T *out_task) {
  uint32_t reg_base;
  uint32_t i, txn;
  uint32_t txn_start, org_start;
  struct HAL_DMA_CH_CFG_T *tx_dma_cfg = NULL, *rx_dma_cfg = NULL;
  enum HAL_I2C_SM_TASK_ACTION_T action;
  uint32_t total_tx_len;

  if (hal_i2c_sm[id].cfg.use_dma == 0) {
    return;
  }

  reg_base = _i2c_get_base(id);
  action = out_task->action;

  total_tx_len = out_task->tx_txn_len * out_task->txn_cnt;
  if (action == HAL_I2C_SM_TASK_ACTION_M_RECV) {
    total_tx_len += out_task->rx_txn_len * out_task->txn_cnt;
  }
  ASSERT(total_tx_len < ARRAY_SIZE(hal_i2c_sm[id].dma_tx_buf),
         "%s: xfer size too large: action=%d total_tx_len=%u (should <= %u)",
         __FUNCTION__, action, total_tx_len,
         ARRAY_SIZE(hal_i2c_sm[id].dma_tx_buf));

  i2cip_w_tx_dma_enable(reg_base, HAL_I2C_YES);
  i2cip_w_tx_dma_tl(reg_base, HAL_I2C_DMA_TX_TL);

  tx_dma_cfg = &hal_i2c_sm[id].tx_dma_cfg;
  memset(tx_dma_cfg, 0, sizeof(*tx_dma_cfg));
  tx_dma_cfg->dst = 0; // useless
  tx_dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
  tx_dma_cfg->dst_periph = i2c_desc[id].tx_periph;
  tx_dma_cfg->dst_width = HAL_DMA_WIDTH_HALFWORD;
  tx_dma_cfg->handler = hal_i2c_tx_dma_handler;
  tx_dma_cfg->src_bsize = HAL_DMA_BSIZE_1;
  tx_dma_cfg->src_periph = 0; // useless
  tx_dma_cfg->src_tsize = total_tx_len;
  tx_dma_cfg->src_width = HAL_DMA_WIDTH_HALFWORD;
  tx_dma_cfg->try_burst = 1;
  tx_dma_cfg->type = HAL_DMA_FLOW_M2P_DMA;
  tx_dma_cfg->src = (uint32_t)hal_i2c_sm[id].dma_tx_buf;
  tx_dma_cfg->ch =
      hal_gpdma_get_chan(tx_dma_cfg->dst_periph, HAL_DMA_HIGH_PRIO);
  HAL_I2C_TRACE(1, "i2c tx get dma ch %d", tx_dma_cfg->ch);
  ASSERT(tx_dma_cfg->ch != HAL_DMA_CHAN_NONE, "I2C: Failed to get tx dma chan");

  HAL_I2C_TRACE(3, "tx size %d cnt %d total tx size %d", out_task->tx_txn_len,
                out_task->txn_cnt, total_tx_len);

  memset(&hal_i2c_sm[id].dma_tx_buf[0], 0, sizeof(hal_i2c_sm[id].dma_tx_buf));

  for (txn = 0; txn < out_task->txn_cnt; txn++) {
    org_start = out_task->tx_txn_len * txn;
    txn_start = (out_task->tx_txn_len + out_task->rx_txn_len) * txn;
    for (i = 0; i < out_task->tx_txn_len; i++) {
      // lo byte of short : for data
      // hi byte of short : for cmd/stop/restart
      hal_i2c_sm[id].dma_tx_buf[txn_start + i] =
          out_task->tx_buf[org_start + i];
    }
    if (txn && out_task->restart_after_write) {
      hal_i2c_sm[id].dma_tx_buf[txn_start] |= I2CIP_CMD_DATA_RESTART_MASK;
    }
  }

  if (action == HAL_I2C_SM_TASK_ACTION_M_RECV) {
    i2cip_w_rx_dma_enable(reg_base, HAL_I2C_YES);
    i2cip_w_rx_dma_tl(reg_base, HAL_I2C_DMA_RX_TL);

    for (txn = 0; txn < out_task->txn_cnt; txn++) {
      txn_start = (out_task->tx_txn_len + out_task->rx_txn_len) * txn +
                  out_task->tx_txn_len;
      for (i = 0; i < out_task->rx_txn_len; i++) {
        // lo byte of short : for data
        // hi byte of short : for cmd/stop/restart
        hal_i2c_sm[id].dma_tx_buf[txn_start + i] = I2CIP_CMD_DATA_CMD_READ_MASK;
      }
      if (out_task->restart_after_write) {
        hal_i2c_sm[id].dma_tx_buf[txn_start] |= I2CIP_CMD_DATA_RESTART_MASK;
      }
    }

    HAL_I2C_TRACE(2, "rx size %d cnt %d", out_task->rx_txn_len,
                  out_task->txn_cnt);

    rx_dma_cfg = &hal_i2c_sm[id].rx_dma_cfg;
    memset(rx_dma_cfg, 0, sizeof(*rx_dma_cfg));
    rx_dma_cfg->dst = (uint32_t)(out_task->rx_buf);
    rx_dma_cfg->dst_bsize = HAL_DMA_BSIZE_1;
    rx_dma_cfg->dst_periph = 0; // useless
    rx_dma_cfg->dst_width = HAL_DMA_WIDTH_BYTE;
    rx_dma_cfg->handler = hal_i2c_rx_dma_handler;
    rx_dma_cfg->src_bsize = HAL_DMA_BSIZE_1;
    rx_dma_cfg->src_periph = i2c_desc[id].rx_periph;
    rx_dma_cfg->src_tsize = out_task->rx_txn_len * out_task->txn_cnt;
    rx_dma_cfg->src_width = HAL_DMA_WIDTH_BYTE;
    rx_dma_cfg->try_burst = 0;
    rx_dma_cfg->src = 0; // useless
    rx_dma_cfg->type = HAL_DMA_FLOW_P2M_DMA;
    rx_dma_cfg->ch =
        hal_gpdma_get_chan(rx_dma_cfg->src_periph, HAL_DMA_HIGH_PRIO);
    HAL_I2C_TRACE(1, "i2c rx get dma ch %d", rx_dma_cfg->ch);
    ASSERT(tx_dma_cfg->ch != HAL_DMA_CHAN_NONE,
           "I2C: Failed to get rx dma chan");
  }

  if (out_task->stop) {
    hal_i2c_sm[id].dma_tx_buf[total_tx_len - 1] |= I2CIP_CMD_DATA_STOP_MASK;
  }
}
#endif
#endif

#ifdef I2C_TASK_MODE
static void hal_i2c_sm_done_task(enum HAL_I2C_ID_T id) {
  struct HAL_I2C_SM_TASK_T *task;
  uint32_t reg_base = _i2c_get_base(id);

  task = &(hal_i2c_sm[id].task[hal_i2c_sm[id].out_task]);

  if (task->errcode) {
    HAL_I2C_ERROR(1, "i2c err : 0x%X", task->errcode);
    _i2c_show_error_code(task->errcode);
  }

  if (task->stop || task->errcode) {
    HAL_I2C_TRACE(0, "disable i2c");
    i2cip_w_enable(reg_base, HAL_I2C_NO);
  }

#ifdef I2C_USE_DMA
  if (hal_i2c_sm[id].cfg.use_dma) {
    hal_i2c_dma_release(id);
  }
#endif

  if (hal_i2c_sm[id].cfg.use_sync) {
    /* FIXME : os and non-os - different proc */
    task->lock = 0;
  } else {
    if (task->handler) {
      task->handler(id, task->transfer_id, task->tx_buf,
                    task->tx_txn_len * task->txn_cnt, task->rx_buf,
                    task->rx_txn_len * task->txn_cnt, task->errcode);
    }
  }

  hal_i2c_sm[id].out_task =
      (hal_i2c_sm[id].out_task + 1) % HAL_I2C_SM_TASK_NUM_MAX;
  --hal_i2c_sm[id].task_count;
}

static void hal_i2c_sm_next_task(enum HAL_I2C_ID_T id) {
  uint32_t out_task_index;
  uint32_t reg_base, reinit;
  enum HAL_I2C_SM_TASK_ACTION_T action;
  struct HAL_I2C_SM_TASK_T *out_task = 0;

  reg_base = _i2c_get_base(id);

  if (hal_i2c_sm[id].task_count <= 0) {
    HAL_I2C_TRACE(0, "no task");
    hal_i2c_sm_done(id);
    return;
  }

  out_task_index = hal_i2c_sm[id].out_task;
  out_task = &(hal_i2c_sm[id].task[out_task_index]);
  action = out_task->action;

  /* tx : quarter trigger TX EMPTY INT */
  i2cip_w_tx_threshold(reg_base, HAL_I2C_TX_TL);
  if (action == HAL_I2C_SM_TASK_ACTION_M_RECV) {
    /* rx : three quarter trigger RX FULL INT */
    i2cip_w_rx_threshold(reg_base, HAL_I2C_RX_TL);
  }

#ifdef I2C_USE_DMA
  if (hal_i2c_sm[id].cfg.use_dma) {
    i2cip_init_int_mask(reg_base, I2CIP_INT_MASK_STOP_DET_MASK |
                                      I2CIP_INT_MASK_ERROR_MASK);

    /* prepare for dma operation */
    hal_i2c_dma_config(id, out_task);
  } else
#endif
  {
    /* open all interrupt */
    i2cip_init_int_mask(reg_base,
                        (I2CIP_INT_MASK_ALL & ~(I2CIP_INT_MASK_START_DET_MASK |
                                                I2CIP_INT_MASK_ACTIVITY_MASK)));
  }

  reinit = i2cip_r_enable_status(reg_base);
  /* not enable : reconfig i2cip with new-task params */
  /* enable : same operation with pre task */
  if (!(reinit & I2CIP_ENABLE_STATUS_ENABLE_MASK)) {
    HAL_I2C_TRACE(0, "enable i2c");
    i2cip_w_restart(reg_base, HAL_I2C_YES);
    i2cip_w_target_address(reg_base, out_task->target_addr);
    i2cip_w_enable(reg_base, HAL_I2C_YES);
  }

#ifdef I2C_USE_DMA
  if (hal_i2c_sm[id].cfg.use_dma) {
    if (action == HAL_I2C_SM_TASK_ACTION_M_RECV) {
      HAL_I2C_TRACE(0, "enable rx dma");
      hal_gpdma_start(&hal_i2c_sm[id].rx_dma_cfg);
    }
    HAL_I2C_TRACE(0, "enable tx dma");
    hal_gpdma_start(&hal_i2c_sm[id].tx_dma_cfg);
  }
#endif
}

static uint32_t hal_i2c_sm_wait_task_if_need(enum HAL_I2C_ID_T id,
                                             uint32_t task_idx,
                                             uint32_t tm_ms) {
  int tmcnt;
  struct HAL_I2C_SM_TASK_T *task = 0;

  /* FIXME : task_id maybe invalid cause so-fast device operation */
  task = &(hal_i2c_sm[id].task[task_idx]);

  if (!(hal_i2c_sm[id].cfg.use_sync))
    return 0;

  /* FIXME : os and non-os - different proc */
  tmcnt = tm_ms / HAL_I2C_DLY_MS;
  while (1) {
    uint32_t lock = task->lock;

    if (!lock)
      break;

    if (!tmcnt) {
      HAL_I2C_TRACE(1, "wait lock timeout %d ms", tm_ms);
      task->errcode = HAL_I2C_ERRCODE_SYNC_TIMEOUT;
      hal_i2c_sm_done_task(id);
      hal_i2c_sm_next_task(id);
      break;
    }
    hal_i2c_delay_ms(HAL_I2C_DLY_MS);
    tmcnt--;
  }
  return task->errcode;
}

static void hal_i2c_sm_kickoff(enum HAL_I2C_ID_T id) {
  if (hal_i2c_sm[id].state == HAL_I2C_SM_IDLE) {
    hal_i2c_sm[id].state = HAL_I2C_SM_RUNNING;
    hal_i2c_sm_next_task(id);
  }
}

static void hal_i2c_sm_proc(enum HAL_I2C_ID_T id) {
  uint32_t reg_base = 0;
  enum HAL_I2C_SM_STATE_T state;
  enum HAL_I2C_SM_TASK_STATE_T task_state;
  struct HAL_I2C_SM_TASK_T *task;
  uint32_t i = 0, restart = 0, stop = 0, data = 0;
  uint32_t ip_int_status = 0, tx_abrt_source = 0;
  uint8_t rx_limit, tx_limit, rx_cnt, tx_cnt;
  uint32_t total_tx_len, total_rx_len;
  uint32_t txn_idx, txn_pos;

  reg_base = _i2c_get_base(id);
  state = hal_i2c_sm[id].state;
  task = &(hal_i2c_sm[id].task[hal_i2c_sm[id].out_task]);

  ip_int_status = i2cip_r_int_status(reg_base);
  tx_abrt_source = i2cip_r_tx_abrt_source(reg_base);

  HAL_I2C_TRACE(4, "id:%d, ip_int_status=0x%X tx_abrt_source=0x%X state=%d", id,
                ip_int_status, tx_abrt_source, state);

  task_state = _i2c_chk_clr_task_error(reg_base, ip_int_status, tx_abrt_source);

  if (state != HAL_I2C_SM_RUNNING) {
    HAL_I2C_ERROR(3,
                  "*** WARNING: No i2c task running: id=%d ip_int_status=0x%X "
                  "tx_abrt_source=0x%X",
                  id, ip_int_status, tx_abrt_source);
    i2cip_r_clr_all_intr(reg_base);
    return;
  }

  HAL_I2C_TRACE(1, "RUNNING action=%d", task->action);

  task->state |= task_state;

  if (task->state &
      (HAL_I2C_SM_TASK_STATE_TX_ABRT | HAL_I2C_SM_TASK_STATE_FIFO_ERR)) {
    HAL_I2C_ERROR(5,
                  "*** ERROR:%s: id=%d task_state=0x%X ip_int_status=0x%X "
                  "tx_abrt_source=0x%X",
                  __func__, id, task->state, ip_int_status, tx_abrt_source);
    task->errcode = tx_abrt_source;
    if (task->state & HAL_I2C_SM_TASK_STATE_FIFO_ERR) {
      task->errcode |= HAL_I2C_ERRCODE_FIFO_ERR;
    }
    /* done task on any error */
    hal_i2c_sm_done_task(id);
    hal_i2c_sm_next_task(id);
    return;
  }

  /* stop det interrupt */
  if (ip_int_status & I2CIP_INT_STATUS_STOP_DET_MASK) {
    task->state |= HAL_I2C_SM_TASK_STATE_STOP;
    i2cip_r_clr_stop_det(reg_base);
  }

  /* start det interrupt */
  if (ip_int_status & I2CIP_INT_STATUS_START_DET_MASK) {
    task->state |= HAL_I2C_SM_TASK_STATE_START;
    i2cip_r_clr_start_det(reg_base);
  }

  /* activeity det interrupt */
  if (ip_int_status & I2CIP_INT_STATUS_ACTIVITY_MASK) {
    task->state |= HAL_I2C_SM_TASK_STATE_ACT;
    i2cip_r_clr_activity(reg_base);
  }

  switch (task->action) {
  case HAL_I2C_SM_TASK_ACTION_M_SEND: {
    if (hal_i2c_sm[id].cfg.use_dma == 0) {
      /* tx empty : means tx fifo is at or below IC_TX_TL :
         need to write more data : we can NOT clear this bit, cleared by hw */
      if (ip_int_status & I2CIP_INT_STATUS_TX_EMPTY_MASK) {
        total_tx_len = task->tx_txn_len * task->txn_cnt;
        tx_limit = i2cip_r_tx_fifo_level(reg_base);
        if (tx_limit < I2CIP_TX_FIFO_DEPTH) {
          tx_limit = I2CIP_TX_FIFO_DEPTH - tx_limit;
        } else {
          tx_limit = 0;
        }
        HAL_I2C_TRACE(4, "m_send: tx_pos=%d tx_txn_len=%d cnt=%d tx_limit=%d",
                      task->tx_pos, task->tx_txn_len, task->txn_cnt, tx_limit);
        for (i = task->tx_pos, tx_cnt = 0;
             ((i < total_tx_len) && (tx_cnt < tx_limit)); ++i, ++tx_cnt) {
          /* last byte : we need to decide stop */
          if (i == total_tx_len - 1) {
            stop = task->stop ? I2CIP_CMD_DATA_STOP_MASK : 0;
          } else {
            stop = 0;
          }
          if (task->txn_cnt == 1) {
            txn_pos = i;
          } else {
            txn_pos = i % task->tx_txn_len;
          }
          /* first byte : need to decide restart */
          if (i && task->restart_after_write && txn_pos == 0) {
            restart = I2CIP_CMD_DATA_RESTART_MASK;
          } else {
            restart = 0;
          }
          /* write data to FIFO */
          i2cip_w_cmd_data(reg_base, task->tx_buf[i] |
                                         I2CIP_CMD_DATA_CMD_WRITE_MASK |
                                         restart | stop);

          HAL_I2C_TRACE(3, "m_send: data=0x%02X restart=0x%X stop=0x%X",
                        task->tx_buf[i], restart, stop);
        }

        task->tx_pos = i;

        /* all write action done : do NOT need tx empty int */
        if (task->tx_pos == total_tx_len) {
          i2cip_clear_int_mask(reg_base, I2CIP_INT_MASK_TX_EMPTY_MASK);
        }
        HAL_I2C_TRACE(1, "m_send: i2c status=0x%X", i2cip_r_status(reg_base));
      }
    }

    /* stop condition : done task */
    if (task->state & HAL_I2C_SM_TASK_STATE_STOP) {
      HAL_I2C_TRACE(1, "m_send: task->state:0x%X", task->state);
      hal_i2c_sm_done_task(id);
      hal_i2c_sm_next_task(id);
    }
    break;
  }
  case HAL_I2C_SM_TASK_ACTION_M_RECV: {
    if (hal_i2c_sm[id].cfg.use_dma == 0) {
      /* rx full : need to read */
      if (ip_int_status & I2CIP_INT_STATUS_RX_FULL_MASK) {
        total_rx_len = task->rx_txn_len * task->txn_cnt;
        rx_limit = i2cip_r_rx_fifo_level(reg_base);
        HAL_I2C_TRACE(4,
                      "m_recv:full: rx_pos=%d rx_txn_len=%d cnt=%d rx_limit=%d",
                      task->rx_pos, task->rx_txn_len, task->txn_cnt, rx_limit);
        for (i = task->rx_pos, rx_cnt = 0;
             ((i < total_rx_len) && (rx_cnt < rx_limit)); ++i, ++rx_cnt) {
          task->rx_buf[i] = i2cip_r_cmd_data(reg_base);
          HAL_I2C_TRACE(2, "m_recv:full: rx_buf[%d] 0x%X", i, task->rx_buf[i]);
        }
        task->rx_pos = i;
      }

      /* tx empty : means tx fifo is at or below IC_TX_TL :
         need to write more data : we can NOT clear this bit, cleared by hw */
      if (ip_int_status & I2CIP_INT_STATUS_TX_EMPTY_MASK) {
        total_tx_len = (task->tx_txn_len + task->rx_txn_len) * task->txn_cnt;
        tx_limit = i2cip_r_tx_fifo_level(reg_base);
        if (tx_limit < I2CIP_TX_FIFO_DEPTH) {
          tx_limit = I2CIP_TX_FIFO_DEPTH - tx_limit;
        } else {
          tx_limit = 0;
        }
        rx_limit = task->rx_cmd_sent - task->rx_pos + 1;
        if (rx_limit < I2CIP_RX_FIFO_DEPTH) {
          rx_limit = I2CIP_RX_FIFO_DEPTH - rx_limit;
        } else {
          rx_limit = 0;
        }
        HAL_I2C_TRACE(6,
                      "m_recv:txEmpty: tx_pos=%d tx_txn_len=%d rx_txn_len=%d "
                      "cnt=%d tx_limit=%d rx_limit=%d",
                      task->tx_pos, task->tx_txn_len, task->rx_txn_len,
                      task->txn_cnt, tx_limit, rx_limit);
        if (tx_limit > rx_limit) {
          tx_limit = rx_limit;
        }
        for (i = task->tx_pos, tx_cnt = 0;
             ((i < total_tx_len) && (tx_cnt < tx_limit)); ++i, ++tx_cnt) {
          /* last byte : we need to decide stop */
          if (i == (total_tx_len - 1)) {
            stop = task->stop ? I2CIP_CMD_DATA_STOP_MASK : 0;
          } else {
            stop = 0;
          }
          if (task->txn_cnt == 1) {
            txn_idx = 0;
            txn_pos = i;
          } else {
            txn_idx = i / (task->tx_txn_len + task->rx_txn_len);
            txn_pos = i % (task->tx_txn_len + task->rx_txn_len);
          }
          /* first byte : need to decide restart */
          if (i && task->restart_after_write &&
              (txn_pos == 0 || txn_pos == task->tx_txn_len)) {
            restart = I2CIP_CMD_DATA_RESTART_MASK;
          } else {
            restart = 0;
          }
          /* real write data */
          if (txn_pos < task->tx_txn_len) {
            if (task->txn_cnt == 1) {
              data = task->tx_buf[txn_pos];
            } else {
              data = task->tx_buf[txn_idx * task->tx_txn_len + txn_pos];
            }
          } else {
            data = I2CIP_CMD_DATA_CMD_READ_MASK;
            task->rx_cmd_sent++;
          }

          i2cip_w_cmd_data(reg_base, data | restart | stop);
          HAL_I2C_TRACE(4, "m_recv:tx: data[%u]=0x%02X restart=0x%X stop=0x%X",
                        i, data, restart, stop);
        }

        task->tx_pos = i;

        /* all write action done */
        if (task->tx_pos == total_tx_len) {
          i2cip_clear_int_mask(reg_base, I2CIP_INT_MASK_TX_EMPTY_MASK);
        }
      }
    }

    /* stop condition : need to read out all rx fifo */
    if (task->state & HAL_I2C_SM_TASK_STATE_STOP) {
      if (hal_i2c_sm[id].cfg.use_dma) {
#ifdef I2C_USE_DMA
        if (hal_i2c_sm[id].rx_dma_cfg.ch == HAL_DMA_CHAN_NONE) {
          HAL_I2C_TRACE(0, "m_recv:stop:WARNING: bad rx dma chan!");
        } else {
          if (hal_dma_chan_busy(hal_i2c_sm[id].rx_dma_cfg.ch)) {
            HAL_I2C_TRACE(0, "m_recv:stop:WARNING: rx dma not finished yet!");
          }
        }
#endif
      } else {
        rx_limit = i2cip_r_rx_fifo_level(reg_base);
        HAL_I2C_TRACE(1, "m_recv:stop: rx_limit=%d", rx_limit);
        for (i = task->rx_pos, rx_cnt = 0;
             ((rx_cnt < rx_limit) && (i < task->rx_txn_len)); ++i, ++rx_cnt) {
          task->rx_buf[i] = i2cip_r_cmd_data(reg_base);
          HAL_I2C_TRACE(2, "m_recv:stop: rx_buf[%d] 0x%X", i, task->rx_buf[i]);
        }
        task->rx_pos = i;
        if (task->rx_pos != task->rx_txn_len * task->txn_cnt) {
          HAL_I2C_TRACE(
              3,
              "m_recv:stop:WARNING: rx_pos(%u) != rx_txn_len(%u) * txn_cnt(%u)",
              task->rx_pos, task->rx_txn_len, task->txn_cnt);
        }
      }
      hal_i2c_sm_done_task(id);
      hal_i2c_sm_next_task(id);
    }
    break;
  }
  default:
    break;
  }
}

void hal_i2c_irq_handler(void) {
  enum HAL_I2C_ID_T id;

  for (id = HAL_I2C_ID_0; id < HAL_I2C_ID_NUM; id++) {
    if (NVIC_GetActive(i2c_desc[id].irq)) {
      hal_i2c_sm_proc(id);
    }
  }
}
#endif

#ifdef I2C_SIMPLE_MODE
void hal_i2c_irq_handler_s(void) {
  enum HAL_I2C_ID_T id;

  for (id = HAL_I2C_ID_0; id < HAL_I2C_ID_NUM; id++) {
    if (NVIC_GetActive(i2c_desc[id].irq)) {
      hal_i2c_simple_proc(id);
    }
  }
}
#endif

#ifdef I2C_SENSOR_ENGINE
void hal_i2c_irq_handler_sensor_eng(void) {
  enum HAL_I2C_ID_T id;

  for (id = HAL_I2C_ID_0; id < HAL_I2C_ID_NUM; id++) {
    if (NVIC_GetActive(i2c_desc[id].irq)) {
      hal_i2c_sensor_eng_proc(id);
    }
  }
}
#endif

static int POSSIBLY_UNUSED hal_i2c_nvic_setup_irq(enum HAL_I2C_ID_T id,
                                                  uint32_t handler) {
  IRQn_Type irqt = i2c_desc[id].irq;

  NVIC_SetVector(irqt, handler);
  NVIC_SetPriority(irqt, IRQ_PRIORITY_NORMAL);
  return 0;
}

static int hal_i2c_nvic_enable_irq(enum HAL_I2C_ID_T id, int enabled) {
  IRQn_Type irqt = i2c_desc[id].irq;

  if (enabled) {
    NVIC_ClearPendingIRQ(irqt);
    NVIC_EnableIRQ(irqt);
  } else {
    NVIC_DisableIRQ(irqt);
    NVIC_ClearPendingIRQ(irqt);
  }
  return 0;
}

uint32_t hal_i2c_open(enum HAL_I2C_ID_T id,
                      const struct HAL_I2C_CONFIG_T *cfg) {
  uint32_t reg_base;

  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].state != HAL_I2C_SM_CLOSED) {
    return HAL_I2C_ERRCODE_IN_USE;
  }

  hal_i2c_sm_init(id, cfg);

  hal_cmu_clock_enable(i2c_desc[id].mod);
  hal_cmu_clock_enable(i2c_desc[id].apb);
  hal_cmu_reset_clear(i2c_desc[id].mod);
  hal_cmu_reset_clear(i2c_desc[id].apb);

  reg_base = _i2c_get_base(id);
  HAL_I2C_TRACE(2, "i2c id=%d reg_base=0x%X", id, reg_base);

  i2cip_w_enable(reg_base, HAL_I2C_NO);

  i2cip_w_target_address(reg_base, 0x18);

  /* clear */
  i2cip_w_clear_ctrl(reg_base);

  /* as master */
  if (cfg->as_master) {
    i2cip_w_disable_slave(reg_base, HAL_I2C_YES);
    i2cip_w_master_mode(reg_base, HAL_I2C_YES);
  } else {
    i2cip_w_disable_slave(reg_base, HAL_I2C_NO);
    i2cip_w_master_mode(reg_base, HAL_I2C_NO);

    /* address as slave */
    i2cip_w_address_as_slave(reg_base, cfg->addr_as_slave);
  }

  /* speed */
  _i2c_set_bus_speed(id, cfg->speed);
  if (0) {
#ifdef I2C_SIMPLE_MODE
  } else if (cfg->mode == HAL_I2C_API_MODE_SIMPLE) {
    HAL_I2C_TRACE(0, "simple mode");
    hal_i2c_nvic_setup_irq(id, (uint32_t)hal_i2c_irq_handler_s);

    /* only open slave related int, polling in master */
    /* read req (master read us), rx full (master write us) */
    if (cfg->as_master) {
      i2cip_init_int_mask(reg_base, I2CIP_INT_UNMASK_ALL);
    } else {
      i2cip_init_int_mask(reg_base, I2CIP_INT_MASK_RD_REQ_MASK |
                                        I2CIP_INT_MASK_RX_FULL_MASK);
    }

    /* rx threshold, 1 byte trigger RX_FULL */
    i2cip_w_rx_threshold(reg_base, I2CIP_RX_TL_1_BYTE);

    /* tx threshold, 1 byte trigger TX_EMPTY */
    i2cip_w_tx_threshold(reg_base, I2CIP_TX_TL_1_BYTE);

    /* enable restart */
    i2cip_w_restart(reg_base, HAL_I2C_YES);

    /* enable i2c */
    i2cip_w_enable(reg_base, HAL_I2C_YES);
#endif
#ifdef I2C_TASK_MODE
  } else if (cfg->mode == HAL_I2C_API_MODE_TASK) {
    /* only use task irq handler when master mode */
    HAL_I2C_TRACE(0, "task mode");
    if (!cfg->as_master) {
      HAL_I2C_TRACE(0, "not as master");
      return HAL_I2C_ERRCODE_INV_PARAM;
    }
#ifndef I2C_USE_DMA
    if (cfg->use_dma) {
      HAL_I2C_TRACE(0, "using DMA when I2C_USE_DMA is NOT enabled");
      return HAL_I2C_ERRCODE_INV_PARAM;
    }
#endif

    hal_i2c_nvic_setup_irq(id, (uint32_t)hal_i2c_irq_handler);
#endif
#ifdef I2C_SENSOR_ENGINE
  } else if (cfg->mode == HAL_I2C_API_MODE_SENSOR_ENGINE) {
    HAL_I2C_TRACE(0, "sensor engine mode");
    ASSERT(cfg->as_master, "%s: i2c master should be used with sensor engine",
           __FUNCTION__);
    ASSERT(cfg->use_dma, "%s: i2c dma should be used with sensor engine",
           __FUNCTION__);

    hal_i2c_nvic_setup_irq(id, (uint32_t)hal_i2c_irq_handler_sensor_eng);
#endif
  } else {
    ASSERT(false, "%s: Bad i2c api mode: %u", __FUNCTION__, cfg->mode);
  }

  hal_i2c_nvic_enable_irq(id, HAL_I2C_YES);

  return 0;
}

uint32_t hal_i2c_close(enum HAL_I2C_ID_T id) {
  uint32_t reg_base;
  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].state == HAL_I2C_SM_CLOSED) {
    return 0;
  }

  reg_base = _i2c_get_base(id);

  hal_i2c_nvic_enable_irq(id, HAL_I2C_NO);
  i2cip_w_enable(reg_base, HAL_I2C_NO);

#if defined(I2C_TASK_MODE) || defined(I2C_SENSOR_ENGINE)
#ifdef I2C_USE_DMA
  hal_i2c_dma_release(id);
#endif
#endif

  hal_cmu_reset_set(i2c_desc[id].apb);
  hal_cmu_reset_set(i2c_desc[id].mod);
  hal_cmu_clock_disable(i2c_desc[id].apb);
  hal_cmu_clock_disable(i2c_desc[id].mod);

  hal_i2c_sm[id].state = HAL_I2C_SM_CLOSED;

  return 0;
}

/* task mode */
#ifdef I2C_TASK_MODE
uint32_t hal_i2c_task_msend(enum HAL_I2C_ID_T id, uint16_t device_addr,
                            const uint8_t *tx_buf, uint16_t tx_item_len,
                            uint16_t item_cnt, uint32_t transfer_id,
                            HAL_I2C_TRANSFER_HANDLER_T handler) {
  uint32_t task_idx;
  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_TASK) {
    HAL_I2C_TRACE(0, "send: not task mode");
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  task_idx =
      hal_i2c_sm_commit(id, tx_buf, tx_item_len, NULL, 0, item_cnt, device_addr,
                        HAL_I2C_SM_TASK_ACTION_M_SEND, transfer_id, handler);

  hal_i2c_sm_kickoff(id);
  return hal_i2c_sm_wait_task_if_need(id, task_idx, HAL_I2C_SYNC_TM_MS);
}

uint32_t hal_i2c_task_send(enum HAL_I2C_ID_T id, uint16_t device_addr,
                           const uint8_t *tx_buf, uint16_t tx_len,
                           uint32_t transfer_id,
                           HAL_I2C_TRANSFER_HANDLER_T handler) {
  return hal_i2c_task_msend(id, device_addr, tx_buf, tx_len, 1, transfer_id,
                            handler);
}

uint32_t hal_i2c_task_mrecv(enum HAL_I2C_ID_T id, uint16_t device_addr,
                            const uint8_t *tx_buf, uint16_t tx_item_len,
                            uint8_t *rx_buf, uint16_t rx_item_len,
                            uint16_t item_cnt, uint32_t transfer_id,
                            HAL_I2C_TRANSFER_HANDLER_T handler) {
  uint32_t task_idx;
  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_TASK) {
    HAL_I2C_TRACE(0, "recv: not task mode");
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  task_idx = hal_i2c_sm_commit(
      id, tx_buf, tx_item_len, rx_buf, rx_item_len, item_cnt, device_addr,
      HAL_I2C_SM_TASK_ACTION_M_RECV, transfer_id, handler);

  hal_i2c_sm_kickoff(id);
  return hal_i2c_sm_wait_task_if_need(id, task_idx, HAL_I2C_SYNC_TM_MS);
}

uint32_t hal_i2c_task_recv(enum HAL_I2C_ID_T id, uint16_t device_addr,
                           const uint8_t *tx_buf, uint16_t tx_len,
                           uint8_t *rx_buf, uint16_t rx_len,
                           uint32_t transfer_id,
                           HAL_I2C_TRANSFER_HANDLER_T handler) {
  return hal_i2c_task_mrecv(id, device_addr, tx_buf, tx_len, rx_buf, rx_len, 1,
                            transfer_id, handler);
}

uint32_t hal_i2c_send(enum HAL_I2C_ID_T id, uint32_t device_addr, uint8_t *buf,
                      uint32_t reg_len, uint32_t value_len,
                      uint32_t transfer_id,
                      HAL_I2C_TRANSFER_HANDLER_T handler) {
  return hal_i2c_task_send(id, device_addr, buf, reg_len + value_len,
                           transfer_id, handler);
}

uint32_t hal_i2c_recv(enum HAL_I2C_ID_T id, uint32_t device_addr, uint8_t *buf,
                      uint32_t reg_len, uint32_t value_len,
                      uint8_t restart_after_write, uint32_t transfer_id,
                      HAL_I2C_TRANSFER_HANDLER_T handler) {
  return hal_i2c_task_recv(id, device_addr, buf, reg_len, buf + reg_len,
                           value_len, transfer_id, handler);
}
#endif
/* task mode end */

/* simple mode */
#ifdef I2C_SIMPLE_MODE
static void hal_i2c_simple_proc(enum HAL_I2C_ID_T id) {
  uint32_t reg_base = _i2c_get_base(id);
  uint32_t irq_status = i2cip_r_raw_int_status(reg_base);
  uint32_t abt_source = i2cip_r_tx_abrt_source(reg_base);
  HAL_I2C_INT_HANDLER_T h = hal_i2c_int_handlers[id];

  i2cip_r_clr_all_intr(reg_base);

  if (h) {
    HAL_I2C_TRACE(3, "%s:irq=0x%X abt=0x%X", __func__, irq_status, abt_source);
    h(id, irq_status, abt_source);
  }

  // TODO: clean irq after callback done ?
  i2cip_r_clr_all_intr(reg_base);

  HAL_I2C_TRACE(3, "%s:0x%X 0x%X", __func__, i2cip_r_status(reg_base),
                i2cip_r_raw_int_status(reg_base));
}

static uint32_t _i2c_check_tx_abrt_error(enum HAL_I2C_ID_T id) {
  uint32_t reg_base = _i2c_get_base(id);

  if (i2cip_r_raw_int_status(reg_base) & I2CIP_RAW_INT_STATUS_TX_ABRT_MASK)
    return i2cip_r_tx_abrt_source(reg_base);

  return 0;
}

uint32_t hal_i2c_slv_write(enum HAL_I2C_ID_T id, uint8_t *buf, uint32_t len,
                           uint32_t *act_write) {
  uint32_t reg_base = 0, i = 0, ret = 0;

  reg_base = _i2c_get_base(id);

  *act_write = 0;

  for (i = 0; i < len;) {
    /* Tx Fifo Not Full */
    if (i2cip_r_status(reg_base) & I2CIP_STATUS_TFNF_MASK) {
      i2cip_w_cmd_data(reg_base, buf[i]);
      ++(*act_write);
      ++i;
    } else {
      /* wait for TFNF & not rx done & no error */
      while (!(i2cip_r_status(reg_base) & I2CIP_STATUS_TFNF_MASK) &&
             !(i2cip_r_raw_int_status(reg_base) &
               I2CIP_RAW_INT_STATUS_RX_DONE_MASK) &&
             !(ret = _i2c_check_tx_abrt_error(reg_base)))
        ;

      if (ret || (i2cip_r_raw_int_status(reg_base) &
                  I2CIP_RAW_INT_STATUS_RX_DONE_MASK))
        break;
    }
  }

  /* FIXME : tx empty is not end of transmit, i2cip may transmit the last byte
   */
  while (!(i2cip_r_status(reg_base) & I2CIP_STATUS_TFE_SHIFT) &&
         !(ret = _i2c_check_tx_abrt_error(reg_base)))
    ;

  /* wait to idle */
  /* FIXME : time out */
  while (i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK)
    ;

  return ret;
}

uint32_t hal_i2c_slv_read(enum HAL_I2C_ID_T id, uint8_t *buf, uint32_t len,
                          uint32_t *act_read) {
  uint32_t reg_base = 0, i = 0, ret = 0, depth = 0;

  reg_base = _i2c_get_base(id);

  *act_read = 0;
  /* slave mode : just read */
  for (i = 0; i < len;) {
    /* Rx Fifo Not Empty */
    if (i2cip_r_status(reg_base) & I2CIP_STATUS_RFNE_MASK) {
      buf[i] = i2cip_r_cmd_data(reg_base);
      ++(*act_read);
      ++i;
    } else {
      /* wait for RFNE & no stop & no error */
      while (!(i2cip_r_status(reg_base) & I2CIP_STATUS_RFNE_MASK) &&
             !(i2cip_r_raw_int_status(reg_base) &
               I2CIP_RAW_INT_STATUS_STOP_DET_MASK) &&
             !(ret = _i2c_check_tx_abrt_error(reg_base))) {
      }

      if (ret || (i2cip_r_raw_int_status(reg_base) &
                  I2CIP_RAW_INT_STATUS_STOP_DET_MASK)) {
        HAL_I2C_TRACE(2, "drv:i2c slave read ret 0x%X, raw status 0x%X", ret,
                      i2cip_r_raw_int_status(reg_base));
        break;
      }
    }
  }

  /* may left some bytes in rx fifo */
  depth = i2cip_r_rx_fifo_level(reg_base);
  HAL_I2C_TRACE(1, "drv:i2c slave read depth %d", depth);
  while (depth > 0 && i < len) {
    buf[i] = i2cip_r_cmd_data(reg_base);
    ++(*act_read);
    ++i;
    --depth;
  }

  /* wait to idle */
  while (i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK)
    ;

  return 0;
}

static void i2c_clear_special_tx_abrt(uint32_t reg_base) {
  uint32_t abrt = i2cip_r_tx_abrt_source(reg_base);

  i2cip_r_clr_tx_abrt(reg_base);

  if (abrt & I2CIP_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT_MASK) {
    i2cip_w_restart(reg_base, HAL_I2C_YES);
    i2cip_w_special_bit(reg_base, HAL_I2C_NO);
    i2cip_w_gc_or_start_bit(reg_base, HAL_I2C_NO);
    i2cip_r_clr_tx_abrt(reg_base);
  }
}

static uint32_t _i2c_check_raw_int_status(uint32_t reg_base, uint32_t mask) {
  uint32_t regval = i2cip_r_raw_int_status(reg_base);

  HAL_I2C_TRACE(3, "%s: raw status=0x%X mask=0x%X", __func__, regval, mask);
  return regval & mask;
}

uint32_t hal_i2c_mst_write(enum HAL_I2C_ID_T id, uint32_t dev_addr,
                           const uint8_t *buf, uint32_t len,
                           uint32_t *act_write, uint32_t restart, uint32_t stop,
                           uint32_t yield) {
  uint32_t i, tar, wrcnt, ret = 0;
  uint32_t reg_base;
  uint32_t start_time, timeout;
  uint32_t res, sto;

  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_SIMPLE) {
    HAL_I2C_TRACE(0, "mst_write: not simple mode");
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  HAL_I2C_TRACE(7, "%s:id=%d addr=0x%02X buf=0x%X len=%d res=%d sto=%d",
                __func__, id, dev_addr, (int)buf, len, restart, stop);

  reg_base = _i2c_get_base(id);

  // update TAR
  tar = i2cip_r_target_address_reg(reg_base);
  if (tar != dev_addr) {
    timeout = MS_TO_TICKS(HAL_I2C_WAIT_ACT_MS);
    start_time = hal_sys_timer_get();
    while ((i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK)) {
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait bus idle timeout", __func__);
        ret = HAL_I2C_ERRCODE_ACT_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }

    i2cip_w_enable(reg_base, HAL_I2C_NO);
    i2cip_w_target_address(reg_base, dev_addr);
    i2cip_w_enable(reg_base, HAL_I2C_YES);
    HAL_I2C_TRACE(3, "%s:update tar to 0x%02X from 0x%02X", __func__, dev_addr,
                  tar);
  }

  // check error
  if (_i2c_check_raw_int_status(reg_base,
                                I2CIP_RAW_INT_STATUS_TX_ABRT_MASK |
                                    I2CIP_RAW_INT_STATUS_TX_OVER_MASK |
                                    I2CIP_RAW_INT_STATUS_RX_OVER_MASK |
                                    I2CIP_RAW_INT_STATUS_RX_UNDER_MASK)) {
    i2cip_r_clr_all_intr(reg_base);
    i2c_clear_special_tx_abrt(reg_base);
    HAL_I2C_TRACE(1, "clear error status done before xfer: status=0x%X",
                  i2cip_r_raw_int_status(reg_base));
  }

  // start to transmit
  wrcnt = 0;
  for (i = 0; i < len; i++) {
    if (i == 0) {
      res = restart ? I2CIP_CMD_DATA_RESTART_MASK : 0;
    } else {
      res = 0;
    }
    if (i == (len - 1)) {
      sto = stop ? I2CIP_CMD_DATA_STOP_MASK : 0;
    } else {
      sto = 0;
    }

    timeout = MS_TO_TICKS(HAL_I2C_WAIT_TFNF_MS);
    start_time = hal_sys_timer_get();
    while (!(i2cip_r_status(reg_base) & I2CIP_STATUS_TFNF_MASK)) {
      ret = _i2c_check_raw_int_status(reg_base,
                                      I2CIP_RAW_INT_STATUS_TX_ABRT_MASK);
      if (ret) {
        i2c_clear_special_tx_abrt(reg_base);
        HAL_I2C_TRACE(2, "%s:error 0x%X", __func__, ret);
        goto exit;
      }
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait tfnf timeout", __func__);
        ret = HAL_I2C_ERRCODE_TFNF_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }
    i2cip_w_cmd_data(reg_base,
                     buf[i] | res | sto | I2CIP_CMD_DATA_CMD_WRITE_MASK);

    HAL_I2C_TRACE(4, "send write cmd: data[%d]=0x%02X res=0x%X sto=0x%X", i,
                  buf[i], res, sto);
    wrcnt++;
  }

  if (act_write)
    *act_write = wrcnt;

  if (stop) {
    // wait bus idle
    timeout = MS_TO_TICKS(HAL_I2C_WAIT_ACT_MS);
    start_time = hal_sys_timer_get();
    while (i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK) {
      if (hal_sys_timer_get() - start_time >= timeout) {
        if (_i2c_check_raw_int_status(reg_base,
                                      I2CIP_RAW_INT_STATUS_TX_ABRT_MASK)) {
          i2cip_r_clr_all_intr(reg_base);
          i2c_clear_special_tx_abrt(reg_base);
          HAL_I2C_TRACE(0, "clear error status done after xfer");
        }
        HAL_I2C_TRACE(1, "%s:wait act timeout", __func__);
        ret = HAL_I2C_ERRCODE_ACT_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }
  } else {
    // wait until txfifo empty
    timeout = MS_TO_TICKS(HAL_I2C_WAIT_TFE_MS);
    start_time = hal_sys_timer_get();
    while (!(i2cip_r_status(reg_base) & I2CIP_STATUS_TFE_MASK)) {
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait tfe timeout", __func__);
        ret = HAL_I2C_ERRCODE_TFE_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }
  }

  HAL_I2C_TRACE(1, "%s:done", __func__);
  return 0;
exit:
  HAL_I2C_TRACE(3, "%s:error=0x%X, status=0x%X", __func__, ret,
                i2cip_r_raw_int_status(reg_base));
  return ret;
}

uint32_t hal_i2c_mst_read(enum HAL_I2C_ID_T id, uint32_t dev_addr, uint8_t *buf,
                          uint32_t len, uint32_t *act_read, uint32_t restart,
                          uint32_t stop, uint32_t yield) {
  uint32_t i, j, tar, rdcnt, wrcnt, ret = 0;
  uint32_t reg_base;
  uint32_t start_time, timeout;
  uint32_t res, sto;
  uint8_t tmp;
  uint8_t rx_ongoing, tx_limit;

  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_SIMPLE) {
    HAL_I2C_TRACE(0, "mst_read: not simple mode");
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  HAL_I2C_TRACE(7, "%s:id=%d addr=0x%02X buf=0x%X len=%d res=%d sto=%d",
                __func__, id, dev_addr, (int)buf, len, restart, stop);

  reg_base = _i2c_get_base(id);

  // update TAR
  tar = i2cip_r_target_address_reg(reg_base);
  if (tar != dev_addr) {
    timeout = MS_TO_TICKS(HAL_I2C_WAIT_ACT_MS);
    start_time = hal_sys_timer_get();
    while ((i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK)) {
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait bus idle timeout", __func__);
        ret = HAL_I2C_ERRCODE_ACT_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }

    i2cip_w_enable(reg_base, HAL_I2C_NO);
    i2cip_w_target_address(reg_base, dev_addr);
    i2cip_w_enable(reg_base, HAL_I2C_YES);
    HAL_I2C_TRACE(3, "%s:update tar to 0x%02X from 0x%02X", __func__, dev_addr,
                  tar);
  }

  // clear fifo by reading
  j = i2cip_r_rx_fifo_level(reg_base);
  if (j) {
    for (i = 0; i < j; i++) {
      tmp = i2cip_r_cmd_data(reg_base);
      HAL_I2C_TRACE(2, "%s:discard data 0x%02X", __func__, tmp);
    }
  }

  // check error
  if (_i2c_check_raw_int_status(reg_base,
                                I2CIP_RAW_INT_STATUS_TX_ABRT_MASK |
                                    I2CIP_RAW_INT_STATUS_TX_OVER_MASK |
                                    I2CIP_RAW_INT_STATUS_RX_OVER_MASK |
                                    I2CIP_RAW_INT_STATUS_RX_UNDER_MASK)) {
    i2cip_r_clr_all_intr(reg_base);
    i2c_clear_special_tx_abrt(reg_base);
    HAL_I2C_TRACE(1, "clear error status done before xfer, status=0x%X",
                  i2cip_r_raw_int_status(reg_base));
  }

  // read data
  timeout = MS_TO_TICKS(HAL_I2C_WAIT_RFNE_MS);
  start_time = hal_sys_timer_get();
  wrcnt = 0;
  rdcnt = 0;
  while (rdcnt < len) {
    // send reading cmd
    rx_ongoing =
        i2cip_r_tx_fifo_level(reg_base) + i2cip_r_rx_fifo_level(reg_base) + 1;
    if (rx_ongoing < I2CIP_RX_FIFO_DEPTH) {
      tx_limit = I2CIP_RX_FIFO_DEPTH - rx_ongoing;
    } else {
      tx_limit = 0;
    }

    for (i = 0; i < tx_limit && wrcnt < len; i++, wrcnt++) {
      if (!(i2cip_r_status(reg_base) & I2CIP_STATUS_TFNF_MASK)) {
        break;
      }
      if (wrcnt == 0) {
        res = restart ? I2CIP_CMD_DATA_RESTART_MASK : 0;
      } else {
        res = 0;
      }
      if (wrcnt == len - 1) {
        sto = stop ? I2CIP_CMD_DATA_STOP_MASK : 0;
      } else {
        sto = 0;
      }

      i2cip_w_cmd_data(reg_base, I2CIP_CMD_DATA_CMD_READ_MASK | res | sto);
      HAL_I2C_TRACE(3, "send read cmd: [%u] res=0x%X, sto=0x%X", wrcnt, res,
                    sto);
    }

    if (i2cip_r_status(reg_base) & I2CIP_STATUS_RFNE_MASK) {
      tmp = i2cip_r_cmd_data(reg_base);
      HAL_I2C_TRACE(2, "i2c recv [%u] 0x%02X", rdcnt, tmp);
      if (buf) {
        buf[rdcnt++] = tmp;
      }
      start_time = hal_sys_timer_get();
    } else {
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait rfne timeout", __func__);
        ret = HAL_I2C_ERRCODE_RFNE_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }
  }

  if (act_read)
    *act_read = rdcnt;

  if (stop) {
    timeout = MS_TO_TICKS(HAL_I2C_WAIT_ACT_MS);
    start_time = hal_sys_timer_get();
    while (i2cip_r_status(reg_base) & I2CIP_STATUS_ACT_MASK) {
      if (hal_sys_timer_get() - start_time >= timeout) {
        HAL_I2C_TRACE(1, "%s:wait act timeout", __func__);
        ret = HAL_I2C_ERRCODE_ACT_TIMEOUT;
        goto exit;
      }
      if (yield) {
        hal_i2c_delay_ms(HAL_I2C_DLY_MS);
      }
    }
  }

  HAL_I2C_TRACE(1, "%s:done", __func__);
  return 0;
exit:
  HAL_I2C_TRACE(3, "%s:error=0x%X, status=0x%X", __func__, ret,
                i2cip_r_raw_int_status(reg_base));
  return ret;
}

uint32_t hal_i2c_simple_send(enum HAL_I2C_ID_T id, uint16_t device_addr,
                             const uint8_t *tx_buf, uint16_t tx_len) {
  return hal_i2c_mst_write(id, device_addr, tx_buf, tx_len, NULL, true, true,
                           false);
}

uint32_t hal_i2c_simple_recv(enum HAL_I2C_ID_T id, uint16_t device_addr,
                             const uint8_t *tx_buf, uint16_t tx_len,
                             uint8_t *rx_buf, uint16_t rx_len) {
  int ret;

  ret = hal_i2c_mst_write(id, device_addr, tx_buf, tx_len, NULL, true, false,
                          false);
  if (ret) {
    HAL_I2C_TRACE(2, "%s: mst_write failed: ret=%d", __func__, ret);
    return ret;
  }

  return hal_i2c_mst_read(id, device_addr, rx_buf, rx_len, NULL, true, true,
                          false);
}

uint32_t hal_i2c_set_interrupt_handler(enum HAL_I2C_ID_T id,
                                       HAL_I2C_INT_HANDLER_T handler) {
  hal_i2c_int_handlers[id] = handler;
  return 0;
}
#endif
/* simple mode end */

/* sensor engine mode */
#ifdef I2C_SENSOR_ENGINE
static void hal_i2c_sensor_eng_proc(enum HAL_I2C_ID_T id) {
  HAL_I2C_TRACE(2, "%s: id=%d", __func__, id);

  uint32_t reg_base = _i2c_get_base(id);
  uint32_t irq_status = i2cip_r_int_status(reg_base);
  uint32_t POSSIBLY_UNUSED irq_raw_status = i2cip_r_raw_int_status(reg_base);
  uint32_t abt_source = i2cip_r_tx_abrt_source(reg_base);
  enum HAL_I2C_SM_TASK_STATE_T task_state;

  task_state = _i2c_chk_clr_task_error(reg_base, irq_status, abt_source);

  HAL_I2C_TRACE(5, "%s: id=%d irq=0x%X/0x%X abt=0x%X", __func__, id, irq_status,
                irq_raw_status, abt_source);
  _i2c_show_error_code(abt_source);

  if (task_state &
      (HAL_I2C_SM_TASK_STATE_TX_ABRT | HAL_I2C_SM_TASK_STATE_FIFO_ERR)) {
    HAL_I2C_ERROR(
        6, "*** Error:%s: id=%d task_state=0x%X irq=0x%X/0x%X abt=0x%X",
        __func__, id, task_state, irq_status, irq_raw_status, abt_source);
    hal_i2c_sensor_engine_stop(i2c_sensor_id[id]);
  }
}

static void _sensor_irq_handler(enum HAL_SENSOR_ENGINE_ID_T id,
                                enum HAL_SENSOR_ENGINE_DEVICE_T device,
                                const uint8_t *buf, uint32_t len) {
  enum HAL_I2C_ID_T i2c_id;

  i2c_id = HAL_I2C_ID_0 + (device - HAL_SENSOR_ENGINE_DEVICE_I2C0);
  ASSERT(i2c_id < HAL_I2C_ID_NUM, invalid_id, i2c_id);

  if (i2c_sensor_handler[i2c_id]) {
    i2c_sensor_handler[i2c_id](i2c_id, buf, len);
  }
}

uint32_t
hal_i2c_sensor_engine_start(enum HAL_I2C_ID_T id,
                            const struct HAL_I2C_SENSOR_ENGINE_CONFIG_T *cfg) {
  enum HAL_I2C_SM_TASK_ACTION_T action;
  const struct HAL_I2C_SM_TASK_T *out_task;
  struct HAL_SENSOR_ENGINE_CFG_T sensor_cfg;
  uint32_t reg_base;

  HAL_I2C_ERROR(2, "%s: id=%d", __func__, id);
  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_SENSOR_ENGINE) {
    HAL_I2C_TRACE(1, "i2c-se start: not sensor engine mode: %d",
                  hal_i2c_sm[id].cfg.mode);
    return HAL_I2C_ERRCODE_INV_PARAM;
  }
  if (hal_i2c_sm[id].state != HAL_I2C_SM_IDLE) {
    HAL_I2C_TRACE(1, "i2c-se start: not idle: %d", hal_i2c_sm[id].state);
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  reg_base = _i2c_get_base(id);
  action = HAL_I2C_SM_TASK_ACTION_M_RECV;

  hal_i2c_sm[id].state = HAL_I2C_SM_RUNNING;

  hal_i2c_sm_commit(id, cfg->write_buf, cfg->write_txn_len, cfg->read_buf,
                    cfg->read_txn_len, cfg->txn_cnt, cfg->target_addr, action,
                    0, NULL);

  out_task = &(hal_i2c_sm[id].task[hal_i2c_sm[id].out_task]);
  hal_i2c_dma_config(id, out_task);

  i2c_sensor_id[id] = cfg->id;
  i2c_sensor_handler[id] = cfg->handler;

  memset(&sensor_cfg, 0, sizeof(sensor_cfg));
  sensor_cfg.id = cfg->id;
  sensor_cfg.device = (id == HAL_I2C_ID_0) ? HAL_SENSOR_ENGINE_DEVICE_I2C0
                                           : HAL_SENSOR_ENGINE_DEVICE_I2C1;
  sensor_cfg.trigger_type = cfg->trigger_type;
  sensor_cfg.trigger_gpio = cfg->trigger_gpio;
  sensor_cfg.period_us = cfg->period_us;
  sensor_cfg.device_address = cfg->target_addr;
  sensor_cfg.tx_dma_cfg = &hal_i2c_sm[id].tx_dma_cfg;
  sensor_cfg.rx_dma_cfg = &hal_i2c_sm[id].rx_dma_cfg;
  sensor_cfg.rx_burst_len = cfg->read_txn_len * cfg->txn_cnt;
  sensor_cfg.rx_burst_cnt = cfg->read_burst_cnt;
  sensor_cfg.handler = _sensor_irq_handler;
  ;
#ifdef I2C_VAD
  sensor_cfg.data_to_vad = 1;
  i2cip_w_data_to_vad(reg_base, HAL_I2C_YES);
#endif

  hal_sensor_engine_open(&sensor_cfg);

  /* tx : quarter trigger TX EMPTY INT */
  i2cip_w_tx_threshold(reg_base, HAL_I2C_TX_TL);
  if (action == HAL_I2C_SM_TASK_ACTION_M_RECV) {
    /* rx : three quarter trigger RX FULL INT */
    i2cip_w_rx_threshold(reg_base, HAL_I2C_RX_TL);
  }

  i2cip_init_int_mask(reg_base, I2CIP_INT_MASK_ERROR_MASK);

  i2cip_w_restart(reg_base, HAL_I2C_YES);
  i2cip_w_target_address(reg_base, out_task->target_addr);

  return 0;
}

uint32_t hal_i2c_sensor_engine_stop(enum HAL_I2C_ID_T id) {
  uint32_t reg_base;
  uint32_t irq_status;
  uint32_t abt_source;

  HAL_I2C_ERROR(2, "%s: id=%d", __func__, id);
  ASSERT(id < HAL_I2C_ID_NUM, invalid_id, id);

  if (hal_i2c_sm[id].cfg.mode != HAL_I2C_API_MODE_SENSOR_ENGINE) {
    HAL_I2C_TRACE(1, "i2c-se stop: not sensor engine mode: %d",
                  hal_i2c_sm[id].cfg.mode);
    return HAL_I2C_ERRCODE_INV_PARAM;
  }
  if (hal_i2c_sm[id].state != HAL_I2C_SM_RUNNING) {
    HAL_I2C_TRACE(1, "i2c-se stop: not running: %d", hal_i2c_sm[id].state);
    return HAL_I2C_ERRCODE_INV_PARAM;
  }

  hal_sensor_engine_close(i2c_sensor_id[id]);

  hal_i2c_dma_release(id);

  reg_base = _i2c_get_base(id);

  HAL_I2C_TRACE(0, "disable i2c");
  i2cip_w_enable(reg_base, HAL_I2C_NO);

  irq_status = i2cip_r_int_status(reg_base);
  abt_source = i2cip_r_tx_abrt_source(reg_base);
  _i2c_chk_clr_task_error(reg_base, irq_status, abt_source);

#ifdef I2C_VAD
  i2cip_w_data_to_vad(reg_base, HAL_I2C_NO);
#endif

  hal_i2c_sm_done(id);

  return 0;
}
#endif
/* sensor engine mode end */

/* gpio iic mode */
#include "hal_gpio.h"

static uint8_t g_i2c_open = false;

#define DURATION_INIT_1 600
#define DURATION_INIT_2 600
#define DURATION_INIT_3 600

#define DURATION_START_1 600
#define DURATION_START_2 600
#define DURATION_START_3 600

#define DURATION_STOP_1 800
#define DURATION_STOP_2 600
#define DURATION_STOP_3 1300

#define DURATION_HIGH 900
#define DURATION_LOW 600

#define HAL_GPIO_I2C_DELAY(duration) hal_sys_timer_delay_ns(duration);

struct HAL_GPIO_I2C_CONFIG_T hal_gpio_i2c_cfg;

inline void GPIO_InitIO(uint8_t port, uint8_t direction, uint8_t val_for_out) {
  hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)port,
                       (enum HAL_GPIO_DIR_T)direction, val_for_out);
}

inline void GPIO_WriteIO(uint8_t port, uint8_t data) {
  if (data) {
    hal_gpio_pin_set((enum HAL_GPIO_PIN_T)port);
  } else {
    hal_gpio_pin_clr((enum HAL_GPIO_PIN_T)port);
  }
}
inline uint8_t GPIO_ReadIO(uint8_t port) {
  uint8_t level = 0;
  level = hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)port);
  return level;
}

//
//-------------------------------------------------------------------
// Function:  gpio_i2c_init
// Purpose:  This function is used to init I2C port of the  device
// In:
// Return:      bool
//-------------------------------------------------------------------

static int hal_gpio_i2c_initialize(const struct HAL_GPIO_I2C_CONFIG_T *cfg) {
  struct HAL_IOMUX_PIN_FUNCTION_MAP hal_gpio_i2c_iomux_cfg[] = {
      {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
       HAL_IOMUX_PIN_PULLUP_ENABLE},
      {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
       HAL_IOMUX_PIN_PULLUP_ENABLE},
  };

  hal_gpio_i2c_cfg.scl = cfg->scl;
  hal_gpio_i2c_cfg.sda = cfg->sda;
  hal_gpio_i2c_iomux_cfg[0].pin = (enum HAL_IOMUX_PIN_T)hal_gpio_i2c_cfg.scl;
  hal_gpio_i2c_iomux_cfg[1].pin = (enum HAL_IOMUX_PIN_T)hal_gpio_i2c_cfg.sda;
  hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)hal_gpio_i2c_iomux_cfg,
                 sizeof(hal_gpio_i2c_iomux_cfg) /
                     sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));

  HAL_I2C_TRACE(2, "hal_gpio_i2c_initialize scl=%d sda=%d",
                hal_gpio_i2c_cfg.scl, hal_gpio_i2c_cfg.sda);
  // iTemp = hal_gpio_i2c_cfg.scl | hal_gpio_i2c_cfg.sda  ;
  // Set the GPIO pin to output status
  GPIO_InitIO(hal_gpio_i2c_cfg.scl, 1, 1);
  GPIO_InitIO(hal_gpio_i2c_cfg.sda, 1, 1);
  HAL_GPIO_I2C_DELAY(DURATION_INIT_1);

  // Make the I2C bus in idle status
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);
  HAL_GPIO_I2C_DELAY(DURATION_INIT_1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, 1);
  HAL_GPIO_I2C_DELAY(DURATION_INIT_1);

  for (byte i = 0; i < 30; i++) {
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0); /* low */
    HAL_GPIO_I2C_DELAY(DURATION_LOW);
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);
    HAL_GPIO_I2C_DELAY(DURATION_HIGH);
  }

  return 0;
}

static void hal_gpio_i2c_start(void) /* start or re-start */
{
  GPIO_InitIO(hal_gpio_i2c_cfg.sda, 1, 1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, 1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);

  HAL_GPIO_I2C_DELAY(DURATION_START_1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, 0);
  HAL_GPIO_I2C_DELAY(DURATION_START_2);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0);
  HAL_GPIO_I2C_DELAY(DURATION_START_3); /* start condition */
}

static void hal_gpio_i2c_stop(void) {
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0);
  HAL_GPIO_I2C_DELAY(DURATION_LOW);
  GPIO_InitIO(hal_gpio_i2c_cfg.sda, 1, 0);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, 0);
  HAL_GPIO_I2C_DELAY(DURATION_STOP_1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);
  HAL_GPIO_I2C_DELAY(DURATION_STOP_2);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, 1); /* stop condition */
  HAL_GPIO_I2C_DELAY(DURATION_STOP_3);
}

static uint8_t hal_gpio_i2c_txbyte(uint8_t data) /* return 0 --> ack */
{
  int i;
  uint8_t temp_value = 0;
  for (i = 7; (i >= 0) && (i <= 7); i--) {
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0); /* low */
    HAL_GPIO_I2C_DELAY(DURATION_LOW);
    if (i == 7)
      GPIO_InitIO(hal_gpio_i2c_cfg.sda, 1, 0);
    HAL_GPIO_I2C_DELAY(DURATION_LOW);

    GPIO_WriteIO(hal_gpio_i2c_cfg.sda, ((data >> i) & 0x01));
    HAL_GPIO_I2C_DELAY(DURATION_LOW / 2);
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1); /* high */
    HAL_GPIO_I2C_DELAY(DURATION_HIGH);
  }
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0); /* low */
  HAL_GPIO_I2C_DELAY(DURATION_LOW);
  GPIO_InitIO(hal_gpio_i2c_cfg.sda, 0, 1); /* input  */
  HAL_GPIO_I2C_DELAY(DURATION_LOW / 2);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1); /* high */
  HAL_GPIO_I2C_DELAY(DURATION_HIGH);
  temp_value = GPIO_ReadIO(hal_gpio_i2c_cfg.sda);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0); /* low */
  HAL_GPIO_I2C_DELAY(DURATION_LOW);
  return temp_value;
}

static void hal_gpio_i2c_rxbyte(uint8_t *data, uint8_t ack) {
  int i;
  uint32_t dataCache;

  dataCache = 0;
  for (i = 7; (i >= 0) && (i <= 7); i--) {
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0);
    HAL_GPIO_I2C_DELAY(DURATION_LOW);
    if (i == 7)
      GPIO_InitIO(hal_gpio_i2c_cfg.sda, 0, 1);
    HAL_GPIO_I2C_DELAY(DURATION_LOW);
    GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);
    HAL_GPIO_I2C_DELAY(DURATION_HIGH);
    dataCache |= (GPIO_ReadIO(hal_gpio_i2c_cfg.sda) << i);
    HAL_GPIO_I2C_DELAY(DURATION_LOW / 2);
  }

  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0);
  HAL_GPIO_I2C_DELAY(DURATION_LOW);
  GPIO_InitIO(hal_gpio_i2c_cfg.sda, 1, 1);
  GPIO_WriteIO(hal_gpio_i2c_cfg.sda, ack);
  HAL_GPIO_I2C_DELAY(DURATION_LOW / 2);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 1);
  HAL_GPIO_I2C_DELAY(DURATION_HIGH);
  GPIO_WriteIO(hal_gpio_i2c_cfg.scl, 0); /* low */
  HAL_GPIO_I2C_DELAY(DURATION_LOW);
  *data = (uint8_t)dataCache;
}

uint32_t hal_gpio_i2c_simple_send(uint32_t device_addr, const uint8_t *tx_buf,
                                  uint16_t tx_len) {
  uint32_t i;

  hal_gpio_i2c_start();
  if (device_addr & HAL_I2C_10BITADDR_MASK) {
    hal_gpio_i2c_txbyte(0xF0 | ((device_addr & 0x200) >> 7));
    hal_gpio_i2c_txbyte(device_addr & 0xFF);
  } else {
    hal_gpio_i2c_txbyte((device_addr & 0x7F) << 1);
  }
  for (i = 0; i < tx_len; i++, tx_buf++) {
    hal_gpio_i2c_txbyte(*tx_buf);
  }
  hal_gpio_i2c_stop();

  return 0;
}

uint32_t hal_gpio_i2c_simple_recv(uint32_t device_addr, const uint8_t *tx_buf,
                                  uint16_t tx_len, uint8_t *rx_buf,
                                  uint16_t rx_len) {
  uint8_t tempdata;
  uint32_t i;

  if (tx_len) {
    hal_gpio_i2c_start();
    if (device_addr & HAL_I2C_10BITADDR_MASK) {
      hal_gpio_i2c_txbyte(((device_addr & 0x200) >> 7) | 0xF0);
      hal_gpio_i2c_txbyte(device_addr & 0xFF);
    } else {
      hal_gpio_i2c_txbyte((device_addr & 0x7F) << 1);
    }
    for (i = 0; i < tx_len; i++, tx_buf++) {
      hal_gpio_i2c_txbyte(*tx_buf);
    }
  }

  hal_gpio_i2c_start();
  if (device_addr & HAL_I2C_10BITADDR_MASK) {
    hal_gpio_i2c_txbyte(((device_addr & 0x200) >> 7) | 0xF1);
  } else {
    hal_gpio_i2c_txbyte(((device_addr & 0x7F) << 1) | 1);
  }
  for (i = 0; i < rx_len; i++, rx_buf++) {
    hal_gpio_i2c_rxbyte(&tempdata, (i == rx_len - 1));
    *rx_buf = tempdata;
  }
  hal_gpio_i2c_stop();

  return 0;
}

#define touch_ic_device_addr 0x60
unsigned char I2C_WriteByte(unsigned char reg, unsigned char data) {
  unsigned char result;
  unsigned char buff[2];

  buff[0] = reg;
  buff[1] = data;

  result =
      hal_gpio_i2c_simple_send((unsigned char)touch_ic_device_addr, buff, 2);
  return result;
}

unsigned char I2C_ReadByte(unsigned char reg, unsigned char *data) {
  unsigned char result;

  result = hal_gpio_i2c_simple_recv((unsigned char)touch_ic_device_addr, &reg,
                                    1, data, 1);

  return result;
}

int hal_gpio_i2c_open(const struct HAL_GPIO_I2C_CONFIG_T *cfg) {
  static bool i2cInitDone = false;
  int lock;

  if (g_i2c_open == true) {
    return -1;
  }
  lock = int_lock();
  g_i2c_open = 0;
  int_unlock(lock);

  if (!i2cInitDone) {
    hal_gpio_i2c_initialize(cfg);
    i2cInitDone = 0;
  }

  return 0;
}

int hal_gpio_i2c_close(void) {
  int lock;

  if (g_i2c_open == false) {
    return false;
  }

  lock = int_lock();
  g_i2c_open = false;
  int_unlock(lock);

  return 0;
}

uint32_t hal_gpio_i2c_send(uint32_t device_addr, const uint8_t *buf,
                           uint32_t reg_len, uint32_t value_len) {
  return hal_gpio_i2c_simple_send(device_addr, buf, reg_len + value_len);
}

uint32_t hal_gpio_i2c_recv(uint32_t device_addr, uint8_t *buf, uint32_t reg_len,
                           uint32_t value_len, uint8_t restart_after_write) {
  return hal_gpio_i2c_simple_recv(device_addr, buf, reg_len, buf + reg_len,
                                  value_len);
}
/* gpio iic end */

int app_i2c_demo_init(void) {
  static struct HAL_GPIO_I2C_CONFIG_T i2c_cfg = {
      HAL_GPIO_PIN_P2_0,
      HAL_GPIO_PIN_P2_1,
      0,
  };
  hal_gpio_i2c_open(&i2c_cfg);

  return 0;
}
