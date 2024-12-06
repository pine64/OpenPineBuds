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
#ifdef CHIP_HAS_PSRAMUHS

#include "hal_psramuhs.h"
#include "hal_cache.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "pmu.h"
#include "psramuhsphy.h"
#include "reg_psramuhs_mc.h"

// #define PSRAMUHS_DUAL_8BIT
// #define PSRAMUHS_BURST_REFRESH
// #define PSRAMUHS_DUMMY_CYCLE
// #define PSRAMUHS_PRA_ENABLE
// #define PSRAMUHS_DUAL_SWITCH
// #define PSRAMUHS_DIG_LOOPBACK
// #define PSRAMUHS_ANA_LOOPBACK
// #define PSRAMUHS_AUTO_PRECHARGE
// #define PSRAMUHS_WRAP_ENABLE

#if defined(CHIP_BEST2001) && !defined(PSRAMUHS_DUMMY_CYCLE)
#define PSRAMUHS_DUMMY_CYCLE
#endif

#define TX_FIFO_DEPTH 4
#define RX_FIFO_DEPTH 4

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

static struct PSRAMUHS_MC_T *const psramuhs_mc =
    (struct PSRAMUHS_MC_T *)PSRAMUHS_CTRL_BASE;

#if (PSRAMUHS_SPEED != 0)
static const uint32_t psramuhs_run_clk = PSRAMUHS_SPEED * 1000 * 1000;
#else
#error "invalid PSRAMUHS_SPEED"
#endif

static int hal_psramuhsip_mc_busy(void) {
  return !!(psramuhs_mc->REG_404 & PSRAM_UHS_MC_BUSY);
}

static int hal_psramuhsip_wb_busy(void) {
  return !!(psramuhs_mc->REG_404 & PSRAM_UHS_MC_WB_FILL_LEVEL_MASK);
}

int hal_psramuhsip_mc_in_sleep(void) {
  return GET_BITFIELD(psramuhs_mc->REG_404, PSRAM_UHS_MC_CP_FSM_STATE) ==
         CP_FSM_STATE_PD;
}

int hal_psramuhsip_rx_fifo_empty(void) {
  return !!(psramuhs_mc->REG_404 & PSRAM_UHS_MC_MGR_RXFIFO_R_EMPTY);
}

int hal_psramuhsip_tx_fifo_full(void) {
  return !!(psramuhs_mc->REG_404 & PSRAM_UHS_MC_MGR_TXFIFO_W_FULL);
}

uint32_t hal_psramuhsip_get_rx_fifo_len(void) {
  return GET_BITFIELD(psramuhs_mc->REG_404, PSRAM_UHS_MC_MGR_RXFIFO_FULL_CNT);
}

uint32_t hal_psramuhsip_get_tx_fifo_free_len(void) {
  return GET_BITFIELD(psramuhs_mc->REG_404, PSRAM_UHS_MC_MGR_TXFIFO_EMPTY_CNT);
}

void hal_psramuhsip_mc_busy_wait(void) {
  while (hal_psramuhsip_mc_busy())
    ;
}

void hal_psramuhsip_wb_busy_wait(void) {
  while (hal_psramuhsip_wb_busy())
    ;
}

void hal_psramuhsip_flush_tx_fifo(void) {
  hal_psramuhsip_mc_busy_wait();
  psramuhs_mc->REG_01C = PSRAM_UHS_MC_MGR_TX_FIFO_CLR;
  hal_psramuhsip_mc_busy_wait();
}

void hal_psramuhsip_flush_rx_fifo(void) {
  hal_psramuhsip_mc_busy_wait();
  psramuhs_mc->REG_01C = PSRAM_UHS_MC_MGR_RX_FIFO_CLR;
  hal_psramuhsip_mc_busy_wait();
}

void hal_psramuhsip_flush_all_fifo(void) {
  hal_psramuhsip_mc_busy_wait();
  psramuhs_mc->REG_01C =
      PSRAM_UHS_MC_MGR_TX_FIFO_CLR | PSRAM_UHS_MC_MGR_RX_FIFO_CLR;
  hal_psramuhsip_mc_busy_wait();
}

void hal_psramuhsip_xfer_addr_len(uint32_t addr, uint32_t len) {
  psramuhs_mc->REG_008 = addr;
  psramuhs_mc->REG_00C = len;
}

void hal_psramuhsip_write_fifo(uint32_t *data, uint32_t len) {
  for (int i = 0; i < len; i++) {
    psramuhs_mc->REG_014 = *data++;
  }
}

void hal_psramuhsip_read_fifo(uint32_t *data, uint32_t len) {
  for (int i = 0; i < len; i++) {
    *data++ = psramuhs_mc->REG_018;
  }
}

void hal_psramuhsip_set_reg_data_mask(void) {
#ifdef PSRAMUHS_DUAL_8BIT
  psramuhs_mc->REG_010 = 0xFFFC;
#else
  psramuhs_mc->REG_010 = 0xFFFE;
#endif
}

void hal_psramuhsip_set_mem_data_mask(void) { psramuhs_mc->REG_010 = 0; }

void hal_psramuhsip_set_reg_fifo_width(void) { psramuhs_mc->REG_0B0 = 0; }

void hal_psramuhsip_set_mem_fifo_width(void) {
  // 128 bits if PSRAMUHS_DUAL_8BIT; 64 bits otherwise
  psramuhs_mc->REG_0B0 = PSRAM_UHS_MC_MGR_FIFO_TEST_EN;
}

void hal_psramuhsip_set_cmd(enum MEMIF_CMD_T cmd) {
  psramuhs_mc->REG_004 = cmd;
}

POSSIBLY_UNUSED void psramuhs_read_reg(uint32_t reg, uint32_t *val) {
  hal_psramuhsip_flush_all_fifo();
  hal_psramuhsip_xfer_addr_len(reg, 1);
  hal_psramuhsip_set_cmd(MEMIF_MRR);
  while (hal_psramuhsip_rx_fifo_empty())
    ;
  hal_psramuhsip_read_fifo(val, 1);
}

static void psramuhs_send_cmd_reg(enum MEMIF_CMD_T cmd, uint32_t reg,
                                  uint32_t val) {
#if !defined(PSRAMUHS_DIG_LOOPBACK) && !defined(PSRAMUHS_ANA_LOOPBACK)
#ifdef PSRAMUHS_DUAL_8BIT
  val &= 0xFF;
  val |= (val << 8);
#endif
  hal_psramuhsip_flush_all_fifo();
  // hal_psramuhsip_set_reg_fifo_width();
  // hal_psramuhsip_set_reg_data_mask();
  hal_psramuhsip_write_fifo(&val, 1);
  hal_psramuhsip_xfer_addr_len(reg, 1);
  hal_psramuhsip_set_cmd(cmd);
  while (hal_psramuhsip_get_tx_fifo_free_len() != TX_FIFO_DEPTH)
    ;
  hal_psramuhsip_mc_busy_wait();
  // hal_psramuhsip_set_mem_fifo_width();
  // hal_psramuhsip_set_mem_data_mask();
#endif
}

static void psramuhs_write_reg(uint32_t reg, uint32_t val) {
  psramuhs_send_cmd_reg(MEMIF_MRS, reg, val);
}

static void psramuhs_single_cmd(enum MEMIF_CMD_T cmd) {
#if !defined(PSRAMUHS_DIG_LOOPBACK) && !defined(PSRAMUHS_ANA_LOOPBACK)
  hal_psramuhsip_flush_all_fifo();
  hal_psramuhsip_set_cmd(cmd);
  hal_psramuhsip_mc_busy_wait();
#endif
}

static POSSIBLY_UNUSED void psramuhs_start_clock(void) {
  psramuhs_single_cmd(MEMIF_START_CLOCK);
}

static POSSIBLY_UNUSED void psramuhs_stop_clock(void) {
  psramuhs_single_cmd(MEMIF_STOP_CLOCK);
}

static POSSIBLY_UNUSED void psramuhs_reset(void) {
  psramuhs_single_cmd(MEMIF_RST);
}

static POSSIBLY_UNUSED void psramuhs_zq_calib_reset(void) {
  psramuhs_send_cmd_reg(MEMIF_ZQCRST, 7, 0);
}

static POSSIBLY_UNUSED void psramuhs_zq_calib(void) {
  psramuhs_send_cmd_reg(MEMIF_ZQCL, 5, 0);
}

static void psramuhs_set_timing(uint32_t clk) {
  uint32_t reg;
  uint32_t val;
  uint32_t burst_len;

  reg = 0;
  if (clk <= 200000000) {
    val = 7;
  } else if (clk <= 333000000) {
    val = 6;
  } else if (clk <= 400000000) {
    val = 5;
  } else if (clk <= 533000000) {
    val = 4;
  } else if (clk <= 667000000) {
    val = 0;
  } else if (clk <= 800000000) {
    val = 1;
  } else if (clk <= 933000000) {
    val = 2;
  } else {
    val = 3;
  }
  // Latency, drive
  val = (val << 0) | (2 << 3);
#if 0
    //refresh disable
    val |= (1 << 7);
#endif
  psramuhs_write_reg(reg, val);

  reg = 2;
#if defined(__ARM_ARCH_ISA_ARM) || defined(DSP_ENABLE)
#ifdef PSRAMUHS_DUAL_8BIT
  burst_len = 0x1; // 32B
#else
  burst_len = 0x2; // 64B
#endif
#else
#ifdef PSRAMUHS_DUAL_8BIT
  burst_len = 0x0; // 16B
#else
  burst_len = 0x1; // 32B
#endif
#endif
#ifdef PSRAMUHS_PRA_ENABLE
  // Burst len, pra enable
  val = (burst_len << 0) | (1 << 4);
#else
  // Burst len, auto-precharge
#ifdef PSRAMUHS_AUTO_PRECHARGE
  val = (burst_len << 0) | (1 << 3);
#else
  val = (burst_len << 0);
#endif
#endif
  // wrap enable
#ifdef PSRAMUHS_WRAP_ENABLE
  val |= (1 << 2);
#endif
  psramuhs_write_reg(reg, val);

  reg = 6;
  // Vref trim
  val = (8 << 0);
  psramuhs_write_reg(reg, val);
  hal_sys_timer_delay_us(5);

  psramuhs_zq_calib_reset();
  psramuhs_zq_calib();
}

void hal_psramuhs_sleep(void) {}

void hal_psramuhs_wakeup(void) {}

static void hal_psramuhs_mc_set_timing(uint32_t clk) {
  uint32_t val1, val2;
  uint32_t val;

  if (clk <= 200000000) {
    val1 = 9;
    val2 = 5;
  } else if (clk <= 333000000) {
    val1 = 13;
    val2 = 5;
  } else if (clk <= 400000000) {
    val1 = 16;
    val2 = 6;
  } else if (clk <= 533000000) {
    val1 = 20;
    val2 = 10;
  } else if (clk <= 667000000) {
    val1 = 24;
    val2 = 12;
  } else if (clk <= 800000000) {
    val1 = 29;
    val2 = 14;
  } else if (clk <= 933000000) {
    val1 = 33;
    val2 = 16;
  } else {
    val1 = 37;
    val2 = 18;
  }
  if (clk >= 466000000)
    val2--;

  psramuhs_mc->REG_028 = PSRAM_UHS_MC_WRITE_LATENCY(val2);
  psramuhs_mc->REG_02C = PSRAM_UHS_MC_READ_LATENCY(val1);

  // tCEM_MAX <= 7.8 us when MRW2[5]==1 && MRR4[2]==0, or <= 3.9 us when
  // MRW2[5]==0 || MRR4[2]==1
  val = (clk / 1000000) * 30 / 10 - 1;
  psramuhs_mc->REG_04C =
      PSRAM_UHS_MC_T_REFI(val) | PSRAM_UHS_MC_NUM_OF_BURST_RFS(0x1000);

  // tRC >= 60 ns
  val = ((clk / 1000000) * 60 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_050 = PSRAM_UHS_MC_T_RC(val);
  // tRFC >= 60 ns
  val = ((clk / 1000000) * 60 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_054 = PSRAM_UHS_MC_T_RFC(val);
#ifdef PSRAMUHS_AUTO_PRECHARGE

  // tCPHR >= 5 ns (auto precharge)
#if PSRAMUHS_SPEED >= 900
  val = ((clk / 1000000) * 5 + (1000 - 1)) / 1000;
#else  /*PSRAMUHS_SPEED == 1000*/
  val = 20;
#endif /*PSRAMUHS_SPEED == 1000*/
  psramuhs_mc->REG_05C = PSRAM_UHS_MC_T_CPHR_AP(val);

  // tCPHW >= 30 ns (auto precharge)
#if PSRAMUHS_SPEED >= 900
  val = ((clk / 1000000) * 30 + (1000 - 1)) / 1000;
#else  /*PSRAMUHS_SPEED == 1000*/
  val = 100;
#endif /*PSRAMUHS_SPEED == 1000*/
  psramuhs_mc->REG_064 = PSRAM_UHS_MC_T_CPHW_AP(val);

#else /*PSRAMUHS_AUTO_PRECHARGE*/

  // tCPHR >= 20 ns (no auto precharge)
#if PSRAMUHS_SPEED >= 900
  val = ((clk / 1000000) * 20 + (1000 - 1)) / 1000;
#else  /*PSRAMUHS_SPEED == 1000*/
  val = 20;
#endif /*PSRAMUHS_SPEED == 1000*/
  psramuhs_mc->REG_058 = PSRAM_UHS_MC_T_CPHR(val);

  // tCPHW >= 30 ns (no auto precharge)
#if PSRAMUHS_SPEED >= 900
  val = ((clk / 1000000) * 30 + (1000 - 1)) / 1000;
#else  /*PSRAMUHS_SPEED == 1000*/
  val = 100;
#endif /*PSRAMUHS_SPEED == 1000*/
  psramuhs_mc->REG_060 = PSRAM_UHS_MC_T_CPHW(val);

#endif /*PSRAMUHS_AUTO_PRECHARGE*/
  // tMRR >=  20ns
  val = 20; //((clk / 1000000) * 20 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_068 = PSRAM_UHS_MC_T_MRR(val);
  // tMRW >= 100 ns
  val = ((clk / 1000000) * 100 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_06C = PSRAM_UHS_MC_T_MRS(val);
  // tCEM >= 4 cycles
  psramuhs_mc->REG_070 = PSRAM_UHS_MC_T_CEM(4);
  // tRST >= 10 us
  val = (clk / 1000000) * 10;
  psramuhs_mc->REG_074 = PSRAM_UHS_MC_T_RST(val);
  // tSRF >= 100 ns
  val = ((clk / 1000000) + (10 - 1)) / 10;
  psramuhs_mc->REG_078 = PSRAM_UHS_MC_T_SRF(val);
  // tSRF >= 70 ns
  val = ((clk / 1000000) * 70 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_07C = PSRAM_UHS_MC_T_XSR(val);
  // tHS >= 150 us
  val = (clk / 1000000) * 150;
  psramuhs_mc->REG_080 = PSRAM_UHS_MC_T_HS(val);
  // tXPHS >= 100 ns
  val = ((clk / 1000000) + (10 - 1)) / 10;
  psramuhs_mc->REG_084 = PSRAM_UHS_MC_T_XPHS(val);
  // tXHS >= 150 us
  val = (clk / 1000000) * 150;
  psramuhs_mc->REG_088 = PSRAM_UHS_MC_T_XHS(val);
  // tZQCAL >= 1 us
  val = (clk / 1000000) * 1;
  psramuhs_mc->REG_08C = PSRAM_UHS_MC_T_ZQCAL(val);
  // tZQCRST >= 1 us
  val = (clk / 1000000) * 1;
  psramuhs_mc->REG_090 = PSRAM_UHS_MC_T_ZQCRST(val);
  // NOP dummy cycles, same as tXPHS >= 100 ns
  val = ((clk / 1000000) + (10 - 1)) / 10;
  psramuhs_mc->REG_0A0 =
      PSRAM_UHS_MC_STOP_CLK_IN_NOP | PSRAM_UHS_MC_NOP_DMY_CYC(val);
  psramuhs_mc->REG_0A4 = PSRAM_UHS_MC_QUEUE_IDLE_CYCLE(50000);
  // tDQSCK in [2 ns, 6.5 ns], expanded time >= tDQSCK_MAX
  val = ((clk / 1000000) * 65 / 10 + (1000 - 1)) / 1000;
  psramuhs_mc->REG_0A8 = PSRAM_UHS_MC_T_EXPANDRD(val);
}

static void hal_psramuhs_mc_init(uint32_t clk) {
  uint32_t val;
  uint32_t burst_len;
  uint32_t boundary;

  val = PSRAM_UHS_MC_CHIP_TYPE;
#ifdef PSRAMUHS_DUAL_8BIT
  val |= PSRAM_UHS_MC_CHIP_BIT;
#endif
#ifdef PSRAMUHS_DUAL_SWITCH
  val |= PSRAM_UHS_MC_CHIP_SWITCH;
#endif
#ifdef PSRAMUHS_16BIT
  val |= PSRAM_UHS_MC_CHIP_IO_X16 | PSRAM_UHS_MC_CHIP_CA_PATTERN(2);
#else
  val |= PSRAM_UHS_MC_CHIP_CA_PATTERN(1);
#endif
#if (PSRAMUHS_SIZE == 0x800000)
  val |= PSRAM_UHS_MC_CHIP_MEM_SIZE(1);
#elif (PSRAMUHS_SIZE == 0x1000000)
  val |= PSRAM_UHS_MC_CHIP_MEM_SIZE(2);
#elif (PSRAMUHS_SIZE == 0x2000000)
  val |= PSRAM_UHS_MC_CHIP_MEM_SIZE(3);
#else
  ASSERT(false, "Bad PSRAMUHS_SIZE=0x%08X", PSRAMUHS_SIZE);
#endif
  psramuhs_mc->REG_000 = val;
  psramuhs_mc->REG_024 =
      PSRAM_UHS_MC_STOP_CLK_IDLE | PSRAM_UHS_MC_AUTOWAKEUP_EN;
  // Burst length: 1=32 bytes (Cortex-M) or 2=64 bytes (Cortex-A), Page
  // boundary: 2K bytes
#ifdef PSRAMUHS_WRAP_ENABLE
#if defined(__ARM_ARCH_ISA_ARM) || defined(DSP_ENABLE)
  burst_len = 0x2; // 64B
#else
  burst_len = 0x1; // 32B
#endif
#else
#ifdef PSRAMUHS_DUAL_8BIT
  burst_len = 0x6; // 0:16B 5;2KB 6:4KB
#else
  burst_len = 0x5; // 0:16B 5;2KB 6:4KB
#endif
#endif
#ifdef PSRAMUHS_DUAL_8BIT
  boundary = 2;
#else
  boundary = 1;
#endif
  psramuhs_mc->REG_034 = PSRAM_UHS_MC_BURST_LENGTH(burst_len) |
                         PSRAM_UHS_MC_PAGE_BOUNDARY(boundary);

  // AXI bus width: 128 bits
  psramuhs_mc->REG_038 = PSRAM_UHS_MC_BUS_WIDTH;
  // Write buffer level with high priority: 0~15
  psramuhs_mc->REG_03C = PSRAM_UHS_MC_HIGH_PRI_LEVEL(8);
#ifdef PSRAMUHS_PRA_ENABLE
  psramuhs_mc->REG_040 = PSRAM_UHS_MC_PRA_ENABLE | PSRAM_UHS_MC_PRA_MAX_CNT(32);
#else
#ifdef PSRAMUHS_AUTO_PRECHARGE
  psramuhs_mc->REG_040 =
      PSRAM_UHS_MC_AUTO_PRECHARGE | PSRAM_UHS_MC_PRA_MAX_CNT(32);
#else
  psramuhs_mc->REG_040 = PSRAM_UHS_MC_PRA_MAX_CNT(32);
#endif
#endif
#ifdef PSRAMUHS_WRAP_ENABLE
  psramuhs_mc->REG_040 |= PSRAM_UHS_MC_CP_WRAP_EN;
#endif
  psramuhs_mc->REG_048 = PSRAM_UHS_MC_FRE_RATIO(2);
#ifdef PSRAMUHS_DUMMY_CYCLE
  psramuhs_mc->REG_840 |= PSRAM_UHS_MC_PHY_DUMMY_CYC_EN;
#endif
#if defined(CHIP_BEST2001)
  psramuhs_mc->REG_840 |= PSRAM_UHS_MC_PHY_IDLE_PAD_EN;
#endif
  psramuhs_mc->REG_844 = PSRAM_UHS_MC_T_WPST(2);

  hal_psramuhsip_set_reg_fifo_width();
  hal_psramuhsip_set_reg_data_mask();

  hal_psramuhs_mc_set_timing(clk);

  psramuhs_mc->REG_400 = PSRAM_UHS_MC_INIT_COMPLETE;

#ifdef PSRAMUHS_DIG_LOOPBACK
  psramuhs_mc->REG_840 |= PSRAM_UHS_MC_PHY_LOOPBACK_EN;
  psramuhs_mc->REG_044 |= PSRAM_UHS_MC_SNP_DISABLE;
#endif
#ifdef PSRAMUHS_ANA_LOOPBACK
  psramuhs_mc->REG_044 |= PSRAM_UHS_MC_SNP_DISABLE;
  psramuhs_mc->REG_0B0 |= PSRAM_UHS_MC_MGR_FIFO_TEST_EN;
  psramuhs_mc->REG_840 |= PSRAM_UHS_MC_ANA_LOOPBACK_EN;
#endif

#if !defined(PSRAMUHS_DIG_LOOPBACK) && !defined(PSRAMUHS_ANA_LOOPBACK)
  psramuhsphy_init_calib();
#endif
}

void hal_psramuhs_mc_entry_auto_lp() {
  psramuhs_mc->REG_024 |=
      PSRAM_UHS_MC_ENTRY_SLEEP_IDLE | PSRAM_UHS_MC_ENTRY_SELF_REFRESH_IDLE;
}

void hal_psramuhs_snoop_enable() {
  psramuhs_mc->REG_044 &= ~PSRAM_UHS_MC_SNP_DISABLE;
}
void hal_psramuhs_snoop_disable() {
  psramuhs_mc->REG_044 |= PSRAM_UHS_MC_SNP_DISABLE;
}
void hal_psramuhs_refresh_enable() {
#ifdef PSRAMUHS_BURST_REFRESH
  psramuhs_mc->REG_020 =
      PSRAM_UHS_MC_REFRESH_MODE | PSRAM_UHS_MC_BURST_REFRESH_EN;
#else
  psramuhs_mc->REG_020 = PSRAM_UHS_MC_REFRESH_MODE;
#endif
}
void hal_psramuhs_init(void) {
#if PSRAMUHS_ENABLE
#error "must set MPU befor 2001 eco"
#endif

  hal_cache_wrap_enable(HAL_CACHE_ID_I_CACHE);
  hal_cache_wrap_enable(HAL_CACHE_ID_D_CACHE);

  hal_cmu_ddr_clock_enable();
  hal_cmu_clock_enable(HAL_CMU_MOD_O_PSRAMUHS);
  hal_cmu_clock_enable(HAL_CMU_MOD_H_PSRAMUHS);
  hal_cmu_reset_clear(HAL_CMU_MOD_O_PSRAMUHS);
  hal_cmu_reset_clear(HAL_CMU_MOD_H_PSRAMUHS);

  psramuhsphy_open(psramuhs_run_clk);
  hal_psramuhs_mc_init(psramuhs_run_clk);
  psramuhs_start_clock();
  hal_sys_timer_delay_us(3);
  psramuhs_reset();
  psramuhs_stop_clock();
  psramuhs_set_timing(psramuhs_run_clk);
  hal_psramuhs_refresh_enable();
#if !defined(PSRAMUHS_DIG_LOOPBACK) && !defined(PSRAMUHS_ANA_LOOPBACK)
  hal_psramuhs_snoop_disable();
  psramuhsphy_calib(psramuhs_run_clk);
  hal_psramuhs_snoop_enable();
#endif
  // hal_psramuhs_mc_entry_auto_lp();
}

#endif
