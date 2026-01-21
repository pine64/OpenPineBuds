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
#if defined(CHIP_HAS_PSRAM) && (CHIP_PSRAM_CTRL_VER >= 2)

#include "hal_cache.h"
#include "hal_location.h"
#include "hal_psram.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "pmu.h"
#include "reg_psram_mc_v2.h"
#include "reg_psram_phy_v2.h"
#include "string.h"

#define PSRAM_RESET
//#define PSRAM_DUAL_8BIT
//#define PSRAM_WRAP_ENABLE

//#define PSRAM_DEBUG
#ifdef PSRAM_DEBUG
#define PSRAM_TRACE TRACE_IMM
#else
#define PSRAM_TRACE(...)
#endif

#ifdef PSRAM_DEBUG
#define PSRAM_TRACENOCRLF_NOTS REL_TRACE_NOCRLF_NOTS
#else
#define PSRAM_TRACENOCRLF_NOTS(...)
#endif

#define TX_FIFO_DEPTH 8
#define RX_FIFO_DEPTH 8

// MR0
#define MR0_DRIVE_STR_SHIFT 0
#define MR0_DRIVE_STR_MASK (0x3 << MR0_DRIVE_STR_SHIFT)
#define MR0_DRIVE_STR(n) BITFIELD_VAL(MR0_DRIVE_STR, n)
#define MR0_READ_LATENCY_SHIFT 2
#define MR0_READ_LATENCY_MASK (0x7 << MR0_READ_LATENCY_SHIFT)
#define MR0_READ_LATENCY(n) BITFIELD_VAL(MR0_READ_LATENCY, n)
#define MR0_LT (1 << 5)
#define MR0_FIXED_00_SHIFT 6
#define MR0_FIXED_00_MASK (0x3 << MR0_FIXED_00_SHIFT)
#define MR0_FIXED_00(n) BITFIELD_VAL(MR0_FIXED_00, n)

// MR1
#define MR1_VENDOR_ID_SHIFT 0
#define MR1_VENDOR_ID_MASK (0x1F << MR1_VENDOR_ID_SHIFT)
#define MR1_VENDOR_ID(n) BITFIELD_VAL(MR1_VENDOR_ID, n)
#define MR1_DENSITY_SHIFT 5
#define MR1_DENSITY_MASK (0x3 << MR1_DENSITY_SHIFT)
#define MR1_DENSITY(n) BITFIELD_VAL(MR1_DENSITY, n)
#define MR1_ULP (1 << 7)

// MR2
#define MR2_VENDOR_ID_SHIFT 0
#define MR2_VENDOR_ID_MASK (0x7 << MR2_VENDOR_ID_SHIFT)
#define MR2_VENDOR_ID(n) BITFIELD_VAL(MR2_VENDOR_ID, n)
#define MR2_DEV_ID_SHIFT 3
#define MR2_DEV_ID_MASK (0x3 << MR2_DEV_ID_SHIFT)
#define MR2_DEV_ID(n) BITFIELD_VAL(MR2_DEV_ID, n)
#define MR2_RSVD (1 << 5)
#define MR2_FIXED_1 (1 << 6)
#define MR2_GB (1 << 7)

// MR4
#define MR4_PASR_SHIFT 0
#define MR4_PASR_MASK (0x7 << MR4_PASR_SHIFT)
#define MR4_PASR(n) BITFIELD_VAL(MR4_PASR, n)
#define MR4_RF (1 << 3)
#define MR4_FIXED_0 (1 << 4)
#define MR4_WRITE_LATENCY_SHIFT 5
#define MR4_WRITE_LATENCY_MASK (0x7 << MR4_WRITE_LATENCY_SHIFT)
#define MR4_WRITE_LATENCY(n) BITFIELD_VAL(MR4_WRITE_LATENCY, n)

// MR6
#define MR6_RSVD_SHIFT 0
#define MR6_RSVD_MASK (0xF << MR6_RSVD_SHIFT)
#define MR6_RSVD(n) BITFIELD_VAL(MR6_RSVD, n)
#define MR6_HALF_SLEEP_SHIFT 4
#define MR6_HALF_SLEEP_MASK (0xF << MR6_HALF_SLEEP_SHIFT)
#define MR6_HALF_SLEEP(n) BITFIELD_VAL(MR6_HALF_SLEEP, n)

// MR8
#define MR8_BL_SHIFT 0
#define MR8_BL_MASK (0x3 << MR8_BL_SHIFT)
#define MR8_BL(n) BITFIELD_VAL(MR8_BL, n)
#define MR8_BT (1 << 2)
#define MR8_FIXED_0 (1 << 3)
#define MR8_RSVD_SHIFT 4
#define MR8_RSVD_MASK (0x7 << MR8_RSVD_SHIFT)
#define MR8_RSVD(n) BITFIELD_VAL(MR8_RSVD, n)
#define MR8_FIXED_00 (1 << 7)

enum PSRAM_CMD_T {
  PSRAM_CMD_SYNC_READ = 0x00,
  PSRAM_CMD_SYNC_WRITE = 0x80,
  PSRAM_CMD_4BYTE_READ = 0x3F,
  PSRAM_CMD_4BYTE_WRITE = 0xBF,
  PSRAM_CMD_REG_READ = 0x40,
  PSRAM_CMD_REG_WRITE = 0xC0,
  PSRAM_CMD_GLOBAL_RESET = 0xFF,
};

enum CP_FSM_STATE_T {
  CP_FSM_STATE_SELF_REFRESH = 1,
  CP_FSM_STATE_PD = 2,
  CP_FSM_STATE_READY = 4,
};

enum MEMIF_CMD_T {
  MEMIF_NO_CMD = 0x00,
  MEMIF_WRITE = 0x01,
  MEMIF_READ = 0x02,
  MEMIF_MRS = 0x05,
  MEMIF_MRR = 0x06,
  MEMIF_REF = 0x08,
  MEMIF_SREF = 0x09,
  MEMIF_PD = 0x10,
  MEMIF_NOP = 0x20,
  MEMIF_RST = 0xFF,
  MEMIF_ZQCL = 0x85,
  MEMIF_ZQCS = 0x45,
  MEMIF_ZQCRST = 0x25,
  MEMIF_START_CLOCK = 0x40,
  MEMIF_STOP_CLOCK = 0x80,
  MEMIF_NEW_CMD = 0x7F,
};

static struct PSRAM_MC_T *const psram_mc = (struct PSRAM_MC_T *)PSRAM_CTRL_BASE;
static struct PSRAM_PHY_T *const psram_phy =
    (struct PSRAM_PHY_T *)(PSRAM_CTRL_BASE + 0x8000);

static const uint32_t psram_cfg_clk = 48 * 1000 * 1000;

#if (PSRAM_SPEED != 0)
static const uint32_t psram_run_clk = PSRAM_SPEED * 1000 * 1000;
#else
#error "invalid PSRAMUHS_SPEED"
#endif

static void psram_chip_timing_config(uint32_t clk, bool psram_first);

int hal_psramip_mc_busy(void) {
  return !!(psram_mc->REG_404 & PSRAM_ULP_MC_BUSY);
}

static int hal_psramip_wb_busy(void) {
  return !!(psram_mc->REG_404 & PSRAM_ULP_MC_WB_FILL_LEVEL_MASK);
}

int hal_psramip_mc_in_sleep(void) {
  return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_CP_FSM_STATE) ==
         CP_FSM_STATE_PD;
}

int hal_psramip_rx_fifo_empty(void) {
  return !!(psram_mc->REG_404 & PSRAM_ULP_MC_MGR_RXFIFO_R_EMPTY);
}

int hal_psramip_tx_fifo_full(void) {
  return !!(psram_mc->REG_404 & PSRAM_ULP_MC_MGR_TXFIFO_W_FULL);
}

uint32_t hal_psramip_get_rx_fifo_len(void) {
  return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_MGR_RXFIFO_FULL_CNT);
}

uint32_t hal_psramip_get_tx_fifo_free_len(void) {
  return GET_BITFIELD(psram_mc->REG_404, PSRAM_ULP_MC_MGR_TXFIFO_EMPTY_CNT);
}

void hal_psramip_mc_busy_wait(void) {
  while (hal_psramip_mc_busy())
    ;
}

void hal_psramip_wb_busy_wait(void) {
  while (hal_psramip_wb_busy())
    ;
}

void hal_psramip_flush_tx_fifo(void) {
  hal_psramip_mc_busy_wait();
  psram_mc->REG_01C = PSRAM_ULP_MC_MGR_TX_FIFO_CLR;
  hal_psramip_mc_busy_wait();
}

void hal_psramip_flush_rx_fifo(void) {
  hal_psramip_mc_busy_wait();
  psram_mc->REG_01C = PSRAM_ULP_MC_MGR_RX_FIFO_CLR;
  hal_psramip_mc_busy_wait();
}

void hal_psramip_flush_all_fifo(void) {
  hal_psramip_mc_busy_wait();
  psram_mc->REG_01C =
      PSRAM_ULP_MC_MGR_TX_FIFO_CLR | PSRAM_ULP_MC_MGR_RX_FIFO_CLR;
  hal_psramip_mc_busy_wait();
}

void hal_psramip_xfer_addr_len(uint32_t addr, uint32_t len) {
  psram_mc->REG_008 = addr;
  psram_mc->REG_00C = len;
}

void hal_psramip_write_fifo(uint32_t *data, uint32_t len) {
  for (int i = 0; i < len; i++) {
    psram_mc->REG_014 = *data++;
  }
}

void hal_psramip_read_fifo(uint32_t *data, uint32_t len) {
  for (int i = 0; i < len; i++) {
    *data++ = psram_mc->REG_018;
  }
}

void hal_psramip_set_reg_data_mask(void) {
#ifdef PSRAM_DUAL_8BIT
  psram_mc->REG_010 = 0xFC;
#else
  psram_mc->REG_010 = 0xFE;
#endif
}

void hal_psramip_set_mem_data_mask(void) { psram_mc->REG_010 = 0; }

void hal_psramip_set_cmd(enum MEMIF_CMD_T cmd) { psram_mc->REG_004 = cmd; }

POSSIBLY_UNUSED void psram_read_reg(uint32_t reg, uint32_t *val) {
  hal_psramip_flush_all_fifo();
  hal_psramip_xfer_addr_len(reg, 1);
  hal_psramip_set_cmd(MEMIF_MRR);
  while (hal_psramip_rx_fifo_empty())
    ;
  hal_psramip_read_fifo(val, 1);
}

static void psram_send_cmd_reg(enum MEMIF_CMD_T cmd, uint32_t reg,
                               uint32_t val) {
#ifdef PSRAM_DUAL_8BIT
  val &= 0xFF;
  val |= (val << 8);
#endif
  hal_psramip_flush_all_fifo();
  // hal_psramip_set_reg_data_mask();
  hal_psramip_write_fifo(&val, 1);
  hal_psramip_xfer_addr_len(reg, 1);
  hal_psramip_set_cmd(cmd);
  while (hal_psramip_get_tx_fifo_free_len() != TX_FIFO_DEPTH)
    ;
  hal_psramip_mc_busy_wait();
  // hal_psramip_set_mem_data_mask();
}

static void psram_write_reg(uint32_t reg, uint32_t val) {
  psram_send_cmd_reg(MEMIF_MRS, reg, val);
}

static void psram_single_cmd(enum MEMIF_CMD_T cmd) {
  hal_psramip_flush_all_fifo();
  hal_psramip_set_cmd(cmd);
  hal_psramip_mc_busy_wait();
}

static POSSIBLY_UNUSED void psram_reset(void) { psram_single_cmd(MEMIF_RST); }

static void psram_set_timing(uint32_t clk) {
  uint32_t reg;
  uint32_t val;

#if PSRAMSIZE == 0x800000
  reg = 8;
#ifdef PSRAM_WRAP_ENABLE
  // Wrap 32
  val = MR8_BL(1);
#else
  // Wrap 1k
  val = MR8_BL(0x3);
#endif
  psram_write_reg(reg, val);
#endif
  reg = 0;
  if (clk <= 66000000) {
    val = 2;
  } else if (clk <= 109000000) {
    val = 3;
  } else if (clk <= 133000000) {
    val = 4;
  } else if (clk <= 166000000) {
    val = 5;
  } else {
    val = 6;
  }
  // Latency type: Variable
  val = MR0_DRIVE_STR(3) | MR0_READ_LATENCY(val);
  psram_write_reg(reg, val);

  reg = 4;
  if (clk <= 166000000) {
    val = 0;
  } else {
    val = 4;
  }
  // Fast Refresh,
  val = MR4_PASR(0) | MR4_WRITE_LATENCY(val);
  psram_write_reg(reg, val);
}

static void hal_psram_phy_dll_config(uint32_t clk) {
  uint32_t phy_clk;
  uint32_t range;
  uint32_t val;

  val = psram_phy->REG_050;
  val &= ~PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY;
  psram_phy->REG_050 = val;
  phy_clk = clk;
  if (phy_clk <= 100000000 / 2) {
    range = 3;
  } else if (phy_clk <= 150000000 / 2) {
    range = 2;
  } else if (phy_clk <= 300000000 / 2) {
    range = 1;
  } else {
    range = 0;
  }
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_DLL_RANGE, range);
  psram_phy->REG_050 = val;
  val |= PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY;
  psram_phy->REG_050 = val;
}

static void hal_psram_phy_init(uint32_t clk) {
  uint32_t val;
  val = psram_phy->REG_048;
  val |= PSRAM_ULP_PHY_REG_LDO_PU | PSRAM_ULP_PHY_REG_LDO_PRECHARGE;
  psram_phy->REG_048 = val;
  hal_sys_timer_delay_us(10);

  val &= ~PSRAM_ULP_PHY_REG_LDO_PRECHARGE;
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_LDO_IEN1, 0xc);
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_LDO_IEN2, 0x5);
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_LDO_VTUNE, 0x0);
  psram_phy->REG_048 = val;

  val = psram_phy->REG_04C;
  val |= PSRAM_ULP_PHY_REG_PSRAM_PU;
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_PSRAM_SWRC, 0x3);
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_PSRAM_TXDRV, 0x3);
  psram_phy->REG_04C = val;

  val = psram_phy->REG_050;
  val |= PSRAM_ULP_PHY_REG_DLL_PU;
  // val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_DLL_SWRC, 0x3);
  psram_phy->REG_050 = val;
  hal_sys_timer_delay_us(2);

  val |= PSRAM_ULP_PHY_REG_DLL_RESETB;
  psram_phy->REG_050 = val;
  hal_sys_timer_delay_us(20);

  hal_psram_phy_dll_config(clk);
}

static void hal_psram_mc_set_timing(uint32_t clk) {
  uint32_t val;

  if (clk <= 166000000) {
    val = PSRAM_ULP_MC_WRITE_LATENCY(0);
  } else {
    val = PSRAM_ULP_MC_WRITE_LATENCY(2);
  }
  psram_mc->REG_028 = val;
#if (CHIP_PSRAM_CTRL_VER == 2)
  if (clk <= 66000000) {
    val = PSRAM_ULP_MC_READ_LATENCY(2);
  } else if (clk <= 109000000) {
    val = PSRAM_ULP_MC_READ_LATENCY(3);
  } else if (clk <= 133000000) {
    val = PSRAM_ULP_MC_READ_LATENCY(4);
  } else if (clk <= 166000000) {
    val = PSRAM_ULP_MC_READ_LATENCY(5);
  } else {
    val = PSRAM_ULP_MC_READ_LATENCY(6);
  }
  psram_mc->REG_02C = val;
#else
  // Min latency: 2 cycles
  psram_mc->REG_02C = PSRAM_ULP_MC_READ_LATENCY(2);
#endif
  // tRC >= 55 ns
  val = (clk / 1000000 * 55 + (1000 - 1)) / 1000;
  psram_mc->REG_050 = PSRAM_ULP_MC_T_RC(val);
  val = 2;
  psram_mc->REG_058 = PSRAM_ULP_MC_T_CPHR(val);
  psram_mc->REG_068 = PSRAM_ULP_MC_T_MRR(val);
  val = 6;
  psram_mc->REG_060 = PSRAM_ULP_MC_T_CPHW(val);
#ifdef CHIP_BEST2001
  val += 1;
#endif
  psram_mc->REG_06C = PSRAM_ULP_MC_T_MRS(val);
  // tCEM <= 2.5 us
  val = clk / 1000000 * 25 / 10;
  psram_mc->REG_070 = PSRAM_ULP_MC_T_CEM(val);
  // tRST >= 2 us
  val = clk / 1000000 * 2 + 1;
  psram_mc->REG_074 = PSRAM_ULP_MC_T_RST(val);
  // tHS >= 4 us
  val = clk / 1000000 * 4 + 1;
  psram_mc->REG_080 = PSRAM_ULP_MC_T_HS(val);
  // tXPHS in [60 ns, 4 us]
  val = (clk / 1000000 * 60 + (1000 - 1)) / 1000;
  psram_mc->REG_084 = PSRAM_ULP_MC_T_XPHS(val);
  // tXHS >= 70 us
  val = clk / 1000000 * 70 + 1;
  psram_mc->REG_088 = PSRAM_ULP_MC_T_XHS(val);
  psram_mc->REG_09C = PSRAM_ULP_MC_WR_DMY_CYC(1);
  // NOP dummy cycles, same as tXPHS in [60 ns, 4 us]
  val = (clk / 1000000 * 60 + (1000 - 1)) / 1000;
  psram_mc->REG_0A0 =
      PSRAM_ULP_MC_STOP_CLK_IN_NOP | PSRAM_ULP_MC_NOP_DMY_CYC(val);
  psram_mc->REG_0A4 = PSRAM_ULP_MC_QUEUE_IDLE_CYCLE(5000);
}

static void hal_psram_init_calib(void) {
  uint32_t delay;

  while ((psram_phy->REG_058 & PSRAM_ULP_PHY_DLL_LOCK) == 0)
    ;

  delay = GET_BITFIELD(psram_phy->REG_058, PSRAM_ULP_PHY_DLL_DLY_IN);
  // ASSERT(delay < (PSRAM_ULP_PHY_DLL_DLY_IN_MASK >>
  // PSRAM_ULP_PHY_DLL_DLY_IN_SHIFT),
  //    "%s: Bad DLL_DLY_IN=0x%X reg=0x%08X", __func__, delay,
  //    psram_phy->REG_058);

  delay /= 2;
  psram_phy->REG_054 = PSRAM_ULP_PHY_REG_PSRAM_TX_CEB_DLY(delay) |
                       PSRAM_ULP_PHY_REG_PSRAM_TX_CLK_DLY(delay) |
                       PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY(delay) |
                       PSRAM_ULP_PHY_REG_PSRAM_RX_DQS_DLY(delay);
}

static void hal_psram_mc_init(uint32_t clk) {
#ifdef PSRAM_DUAL_8BIT
  psram_mc->REG_000 = PSRAM_ULP_MC_CHIP_BIT;
#else
  psram_mc->REG_000 = 0;
#endif
  psram_mc->REG_020 = 0;
  psram_mc->REG_024 =
#ifndef CHIP_BEST2001
      PSRAM_ULP_MC_ENTRY_SLEEP_IDLE |
#endif
      PSRAM_ULP_MC_AUTOWAKEUP_EN | PSRAM_ULP_MC_PD_MR(6) |
      PSRAM_ULP_MC_PD_CMD(0xF0);
#ifdef PSRAM_WRAP_ENABLE
  // Burst len: 32 bytes, page: 1K
  psram_mc->REG_034 =
      PSRAM_ULP_MC_BURST_LENGTH(1) | PSRAM_ULP_MC_PAGE_BOUNDARY(0);
#else
  // 8MB psram
  // Burst len: 1K, page: 1K
  psram_mc->REG_034 =
      PSRAM_ULP_MC_BURST_LENGTH(4) | PSRAM_ULP_MC_PAGE_BOUNDARY(0);
#endif
  // AHB bus width: 32 bits
  psram_mc->REG_038 = 0;
  // Write buffer level with high priority: 0~7
  psram_mc->REG_03C = PSRAM_ULP_MC_HIGH_PRI_LEVEL(4);
#ifdef PSRAM_WRAP_ENABLE
  psram_mc->REG_040 = PSRAM_ULP_MC_CP_WRAP_EN;
#else
  psram_mc->REG_040 = PSRAM_ULP_MC_WRAP_CRT_RET_EN;
#endif
  psram_mc->REG_044 = 0;
  psram_mc->REG_048 = 0;

  hal_psramip_set_reg_data_mask();

  hal_psram_mc_set_timing(clk);

  psram_mc->REG_400 = PSRAM_ULP_MC_INIT_COMPLETE;

  hal_psram_init_calib();
}

void hal_psram_sleep(void) {
  hal_psramip_mc_busy_wait();
  if (!hal_psramip_mc_in_sleep()) {
#ifndef CHIP_BEST2001
    psram_mc->REG_024 &= ~PSRAM_ULP_MC_ENTRY_SLEEP_IDLE;
#endif
    hal_psramip_mc_busy_wait();
    hal_psramip_set_cmd(MEMIF_PD);
    hal_psramip_mc_busy_wait();
  }
}

void hal_psram_wakeup(void) {
  hal_psramip_mc_busy_wait();
#ifndef CHIP_BEST2001
  psram_mc->REG_024 |= PSRAM_ULP_MC_ENTRY_SLEEP_IDLE;
#endif
}

static void psram_chip_timing_config(uint32_t clk, bool update_psram_first) {
  enum HAL_CMU_FREQ_T freq;

  if (clk <= 52000000) {
    freq = HAL_CMU_FREQ_104M;
  } else if (clk <= 104000000) {
    freq = HAL_CMU_FREQ_208M;
  } else {
#ifdef HAL_CMU_FREQ_T
    freq = HAL_CMU_FREQ_390M;
#else
    freq = HAL_CMU_FREQ_208M;
#endif
  }

  if (update_psram_first) {
    psram_set_timing(clk);
  }

  hal_cmu_mem_set_freq(freq);
  hal_sys_timer_delay_us(3);
  hal_psram_phy_dll_config(clk);
  hal_psram_init_calib();
  hal_psram_mc_set_timing(clk);
  if (!update_psram_first) {
    psram_set_timing(clk);
  }
}

static bool psramphy_check_write_valid() {
  int i;
  volatile uint32_t *psram_base = (volatile uint32_t *)PSRAM_NC_BASE;
  for (i = 0; i < 0x8; ++i) {
    *(psram_base + i) = 0xffffffff;
  }
  for (i = 0; i < 0x8; ++i) {
    *(psram_base + i) = ((i << 0) | (i << 8) | (i << 16) | (i << 24));
  }
  hal_psramip_wb_busy_wait();
  hal_psramip_mc_busy_wait();
  for (i = 0; i < 0x8; ++i) {
    uint32_t check_val = *(psram_base + i);
    if (check_val != ((i << 0) | (i << 8) | (i << 16) | (i << 24))) {
      // PSRAM_TRACE(2,"write fail, %p = 0x%x", (uint32_t)(psram_base+i),
      // check_val);
      return false;
    }
  }
  return true;
}

static void hal_psram_calib_range(uint32_t range) {
  uint32_t val;
  uint32_t delay;
  uint8_t tx_dqs, rx_dqs;
  uint8_t inc_delay, volume;
  uint8_t cali_valid[0x20][0x20];
  uint8_t cali_value[0x20][0x20];

  ASSERT(range <=
             (PSRAM_ULP_PHY_DLL_DLY_IN_MASK >> PSRAM_ULP_PHY_DLL_DLY_IN_SHIFT),
         "ERROR, bad ana phy range:%d", range);

  val = psram_phy->REG_050;
  val &= ~(PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY);
  psram_phy->REG_050 = val;
  val = SET_BITFIELD(val, PSRAM_ULP_PHY_REG_DLL_RANGE, range);
  psram_phy->REG_050 = val;
  val |= (PSRAM_ULP_PHY_REG_DLL_RESETB | PSRAM_ULP_PHY_REG_DLL_CK_RDY);
  psram_phy->REG_050 = val;

  hal_sys_timer_delay_us(100);
  while ((psram_phy->REG_058 & PSRAM_ULP_PHY_DLL_LOCK) == 0)
    ;

  val = psram_phy->REG_058;
  if ((val & PSRAM_ULP_PHY_DLL_ALL_ONE)) {
    PSRAM_TRACE(2, "%s: all one, increase range=%d", __func__, range + 1);
    return hal_psram_calib_range(range + 1);
  }

  delay = GET_BITFIELD(val, PSRAM_ULP_PHY_DLL_DLY_IN);
  PSRAM_TRACE(4, "%s, range:%d, T/4 = 0x%x(psram_phy->REG_058:0x%x)", __func__,
              range, delay / 2, val);
  if (delay > (PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_MASK >>
               PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_SHIFT) &&
      range < 3) {
    PSRAM_TRACE("%s: bad delay (T/2 > 0x1f). increase range=%d", __func__,
                range + 1);
    return hal_psram_calib_range(range + 1);
  }

  inc_delay = delay / 8;
  if (inc_delay == 0)
    inc_delay = 1;

  // volume =
  // (PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_MASK>>PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_SHIFT)
  // / inc_delay;
  volume = MIN(delay, (PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_MASK >>
                       PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY_SHIFT)) /
           inc_delay;

  PSRAM_TRACE(2, "volume:%d, inc_delay:%d", volume, inc_delay);

  uint8_t all_valid = 1;

  memset(cali_valid, 0, sizeof(cali_valid));
  for (tx_dqs = 0; tx_dqs <= volume; tx_dqs++) {
    for (rx_dqs = 0; rx_dqs <= volume; rx_dqs++) {
      psram_phy->REG_054 =
          PSRAM_ULP_PHY_REG_PSRAM_TX_CEB_DLY(delay / 2) |
          PSRAM_ULP_PHY_REG_PSRAM_TX_CLK_DLY(delay / 2) |
          PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY(tx_dqs * inc_delay) |
          PSRAM_ULP_PHY_REG_PSRAM_RX_DQS_DLY(rx_dqs * inc_delay);
      cali_valid[tx_dqs][rx_dqs] = psramphy_check_write_valid();
      if (cali_valid[tx_dqs][rx_dqs] == 0)
        all_valid = 0;
    }
  }

  if (all_valid && range < (PSRAM_ULP_PHY_DLL_DLY_IN_MASK >>
                            PSRAM_ULP_PHY_DLL_DLY_IN_SHIFT)) {
    PSRAM_TRACE(2, "%s: all valid increase range=%d", __func__, range + 1);
    // return hal_psram_calib_range(range+1);
  }

  memset(cali_value, 0, sizeof(cali_value));
  PSRAM_TRACENOCRLF_NOTS("\r\n\r\n "
                         "-----------------------------------------------------"
                         "----------------- \r\n");
  PSRAM_TRACENOCRLF_NOTS("    rx_dqs");
  for (tx_dqs = 0; tx_dqs <= volume; tx_dqs++) {
    PSRAM_TRACENOCRLF_NOTS(" %2d ", tx_dqs * inc_delay);
  }
  PSRAM_TRACENOCRLF_NOTS("\r\n");
  for (tx_dqs = 0; tx_dqs <= volume; tx_dqs++) {
    PSRAM_TRACENOCRLF_NOTS("tx_dqs:%2d ", tx_dqs * inc_delay);
    for (rx_dqs = 0; rx_dqs <= volume; rx_dqs++) {
      PSRAM_TRACENOCRLF_NOTS("  %d ", cali_valid[tx_dqs][rx_dqs]);
      if (cali_valid[tx_dqs][rx_dqs]) {
        uint8_t len_from_zero;
        int8_t p;
        p = tx_dqs;
        while (p >= 0) {
          if (cali_valid[p][rx_dqs] == 0)
            break;
          p--;
        }
        len_from_zero = tx_dqs - p;
        cali_value[tx_dqs][rx_dqs] = len_from_zero;

        p = tx_dqs;
        while (p <= volume) {
          if (cali_valid[p][rx_dqs] == 0)
            break;
          p++;
        }
        len_from_zero = p - tx_dqs;
        cali_value[tx_dqs][rx_dqs] =
            MIN(cali_value[tx_dqs][rx_dqs], len_from_zero);

        p = rx_dqs;
        while (p >= 0) {
          if (cali_valid[tx_dqs][p] == 0)
            break;
          p--;
        }
        len_from_zero = rx_dqs - p;
        cali_value[tx_dqs][rx_dqs] =
            MIN(cali_value[tx_dqs][rx_dqs], len_from_zero);

        p = rx_dqs;
        while (p <= volume) {
          if (cali_valid[tx_dqs][p] == 0)
            break;
          p++;
        }
        len_from_zero = p - rx_dqs;
        cali_value[tx_dqs][rx_dqs] =
            MIN(cali_value[tx_dqs][rx_dqs], len_from_zero);
      }
    }
    PSRAM_TRACENOCRLF_NOTS("\r\n");
  }
  PSRAM_TRACENOCRLF_NOTS(" ----------------------------------------------------"
                         "---------------------- \r\n");

#if 0
    PSRAM_TRACENOCRLF_NOTS("\r\n\r\n ---------------------------------------------------------------------- \r\n");
    PSRAM_TRACENOCRLF_NOTS("    rx_dqs");
    for (tx_dqs=0; tx_dqs<=volume; tx_dqs++) {
        PSRAM_TRACENOCRLF_NOTS(" %2d ", tx_dqs*inc_delay);
    }
    PSRAM_TRACENOCRLF_NOTS("\r\n");
    for (tx_dqs=0; tx_dqs<=volume; tx_dqs++) {
        PSRAM_TRACENOCRLF_NOTS("tx_dqs:%2d ", tx_dqs*inc_delay);
        for (rx_dqs=0; rx_dqs<=volume; rx_dqs++) {
            PSRAM_TRACENOCRLF_NOTS("  %d ", cali_value[tx_dqs][rx_dqs]);
        }
        PSRAM_TRACENOCRLF_NOTS("\r\n");
    }
    PSRAM_TRACENOCRLF_NOTS(" -------------------------------------------------------------------------- \r\n");
#endif

  uint32_t position = 0;
  uint8_t max_value = 0;
  for (tx_dqs = 0; tx_dqs <= volume; tx_dqs++) {
    for (rx_dqs = 0; rx_dqs <= volume; rx_dqs++) {
      if (cali_value[tx_dqs][rx_dqs] > max_value) {
        max_value = cali_value[tx_dqs][rx_dqs];
        position = tx_dqs * (volume + 1) + rx_dqs;
      }
    }
  }
  PSRAM_TRACENOCRLF_NOTS("position:%d\r\n", position);
  tx_dqs = position / (volume + 1) * inc_delay;
  rx_dqs = (position % (volume + 1)) * inc_delay;
  PSRAM_TRACENOCRLF_NOTS("most optimal position. tx_dqs:%d, rx_dqs:%d\r\n",
                         tx_dqs, rx_dqs);

  psram_phy->REG_054 = PSRAM_ULP_PHY_REG_PSRAM_TX_CEB_DLY(delay / 2) |
                       PSRAM_ULP_PHY_REG_PSRAM_TX_CLK_DLY(delay / 2) |
                       PSRAM_ULP_PHY_REG_PSRAM_TX_DQS_DLY(tx_dqs) |
                       PSRAM_ULP_PHY_REG_PSRAM_RX_DQS_DLY(rx_dqs);
}
static void hal_psram_calib(uint32_t clk) {
  uint32_t phy_clk;
  uint32_t range;
  PSRAM_TRACE("%s, speed:%d", __func__, clk);
  phy_clk = clk;
  if (phy_clk <= 100000000 / 2) {
    range = 3;
  } else if (phy_clk <= 150000000 / 2) {
    range = 2;
  } else if (phy_clk <= 300000000 / 2) {
    range = 1;
  } else {
    range = 0;
  }
  hal_psram_calib_range(range);
}
void hal_psram_snoop_enable() {
  psram_mc->REG_044 &= ~PSRAM_ULP_MC_SNP_DISABLE;
}
void hal_psram_snoop_disable() {
  psram_mc->REG_044 |= PSRAM_ULP_MC_SNP_DISABLE;
}

void hal_psram_init(void) {
  hal_cache_wrap_enable(HAL_CACHE_ID_I_CACHE);
  hal_cache_wrap_enable(HAL_CACHE_ID_D_CACHE);

  hal_cmu_mem_set_freq(HAL_CMU_FREQ_104M);
  hal_cmu_clock_enable(HAL_CMU_MOD_O_PSRAM);
  hal_cmu_clock_enable(HAL_CMU_MOD_H_PSRAM);
  hal_cmu_reset_clear(HAL_CMU_MOD_O_PSRAM);
  hal_cmu_reset_clear(HAL_CMU_MOD_H_PSRAM);

  hal_psram_phy_init(psram_cfg_clk);
  hal_sys_timer_delay_us(30);
  hal_psram_mc_init(psram_cfg_clk);

#ifdef PSRAM_RESET
  psram_reset();
  psram_chip_timing_config(psram_run_clk, true);
#else
  uint32_t reg;
  uint32_t val;

  reg = 4;
  psram_read_reg(reg, &val);
  if (val & MR4_WL) {
    psram_chip_timing_config(psram_run_clk, false);
  } else {
    psram_chip_timing_config(psram_run_clk, true);
  }
#endif

  hal_psram_snoop_disable();
  hal_psram_calib(psram_run_clk);
  hal_psram_snoop_enable();
}

#endif
