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
#if (CHIP_FLASH_CTRL_VER <= 1)

#include "hal_norflaship.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "reg_norflaship_v1.h"
#ifdef PROGRAMMER
#include "task_schedule.h"
#else
#define TASK_SCHEDULE true
#endif

//====================================================================================
// Flash IP Operations
//====================================================================================

/*
 Supported Command List (Based on GD25Q32C):

 parameter NOP       = 8'h00 ;
 parameter WRR       = 8'h01 ;
 parameter PP        = 8'h02 ;
 parameter READ      = 8'h03 ;
 parameter WRDI      = 8'h04 ;
 parameter RDSR      = 8'h05 ;
 parameter WREN      = 8'h06 ;
 parameter FAST_READ = 8'h0B ;
 parameter P4E       = 8'h20 ;
 parameter SE2       = 8'h20 ;
 parameter CLSR      = 8'h30 ;
 parameter QPP       = 8'h32 ;
 parameter RCR       = 8'h35 ;
 parameter DOR       = 8'h3B ;
 parameter P8E       = 8'h40 ;
 parameter OTPP      = 8'h42 ;
 parameter PSR       = 8'h42 ;
 parameter ESR       = 8'h44 ;
 parameter RSR       = 8'h48 ;
 parameter OTPR      = 8'h4B ;
 parameter BE32      = 8'h52 ;
 parameter CE        = 8'h60 ;
 parameter QOR       = 8'h6B ;
 parameter PES       = 8'h75 ;
 parameter PER       = 8'h7A ;
 parameter READ_ID   = 8'h90 ;
 parameter RDID      = 8'h9F ;
 parameter HPM       = 8'hA3 ;
 parameter RES       = 8'hAB ;
 parameter DP        = 8'hB9 ;
 parameter DIOR      = 8'hBB ;
 parameter BE        = 8'hC7 ;
 parameter BE64      = 8'hD8 ;
 parameter SE        = 8'hD8 ;
 parameter QIOWR     = 8'hE7 ;
 parameter QIOR      = 8'hEB ;
 parameter CRR       = 8'hFF ;
*/

/* register memory address */
#define NORFLASHIP_BASEADDR FLASH_CTRL_BASE

#define norflaship_readb(a)                                                    \
  (*(volatile unsigned char *)(NORFLASHIP_BASEADDR + a))

#define norflaship_read32(a)                                                   \
  (*(volatile unsigned int *)(NORFLASHIP_BASEADDR + a))

#define norflaship_write32(v, a)                                               \
  ((*(volatile unsigned int *)(NORFLASHIP_BASEADDR + a)) = v)

/* ip ops */
uint8_t norflaship_continuous_read_mode_bit(uint8_t mode_bit) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(TX_CONFIG2_BASE);
  val &= ~(TX_MODBIT_MASK);
  val |= (mode_bit << TX_MODBIT_SHIFT) & TX_MODBIT_MASK;
  norflaship_write32(val, TX_CONFIG2_BASE);
  return 0;
}
uint8_t norflaship_continuous_read_off(void) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(TX_CONFIG2_BASE);
  val &= ~(TX_CONMOD_MASK);
  norflaship_write32(val, TX_CONFIG2_BASE);
  return 0;
}
uint8_t norflaship_continuous_read_on(void) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(TX_CONFIG2_BASE);
  val |= (TX_CONMOD_MASK);
  norflaship_write32(val, TX_CONFIG2_BASE);
  return 0;
}
uint32_t norflaship_write_txfifo(const uint8_t *val, uint32_t len) {
  uint32_t i = 0;
  for (i = 0; i < len; ++i) {
    norflaship_write32(val[i], TXDATA_BASE);
  }
  return 0;
}
uint32_t norflaship_v1_write_txfifo_safe(const uint8_t *val, uint32_t len) {
  uint32_t i;
  uint32_t st;
  for (i = 0; i < len; ++i) {
    st = norflaship_read32(INT_STATUS_BASE);
    if (st & TXFIFOFULL_MASK) {
      break;
    }
    norflaship_write32(val[i], TXDATA_BASE);
  }
  return len - i;
}
uint32_t norflaship_v1_write_txfifo_all(const uint8_t *val, uint32_t len) {
  uint32_t st;
  while (len > 0) {
    st = norflaship_read32(INT_STATUS_BASE);
    if (st & TXFIFOFULL_MASK) {
      continue;
    }
    norflaship_write32(*val, TXDATA_BASE);
    val++;
    len--;
  }
  return 0;
}
uint8_t norflaship_read_rxfifo_count(void) {
  uint32_t val = 0;
  val = norflaship_readb(INT_STATUS_BASE);
  return ((val & RXFIFOCOUNT_MASK) >> RXFIFOCOUNT_SHIFT);
}
uint8_t norflaship_read_rxfifo(void) {
  uint32_t val = 0;
  val = norflaship_readb(RXDATA_BASE);
  return val & 0xff;
}
void norflaship_blksize(uint32_t blksize) {
  uint32_t val = 0;
  val = norflaship_read32(TX_CONFIG2_BASE);
  val = ((~(TX_BLKSIZE_MASK)) & val) | (blksize << TX_BLKSIZE_SHIFT);
  norflaship_write32(val, TX_CONFIG2_BASE);
}
void norflaship_cmd_addr(uint8_t cmd, uint32_t address) {
  norflaship_busy_wait();
  norflaship_write32(address << TX_ADDR_SHIFT | (cmd << TX_CMD_SHIFT),
                     TX_CONFIG1_BASE);
}
void norflaship_ext_tx_cmd(uint8_t cmd, uint32_t tx_len) {
  norflaship_busy_wait();
  norflaship_write32(cmd << EXT_CMD_SHIFT, TX_CONFIG1_BASE);
}
void norflaship_ext_rx_cmd(uint8_t cmd, uint32_t tx_len, uint32_t rx_len) {
  norflaship_busy_wait();
  norflaship_blksize(rx_len);
  norflaship_write32(EXT_CMD_RX_MASK | (cmd << EXT_CMD_SHIFT), TX_CONFIG1_BASE);
}
void norflaship_cmd_done(void) { norflaship_busy_wait(); }
void norflaship_rxfifo_count_wait(uint8_t cnt) {
  uint32_t st = 0;
  do {
    st = norflaship_read32(INT_STATUS_BASE);
  } while ((((st & RXFIFOCOUNT_MASK) >> RXFIFOCOUNT_SHIFT) < cnt) &&
           TASK_SCHEDULE);
}
void norflaship_rxfifo_empty_wait(void) {
  uint32_t st = 0;
  do {
    st = norflaship_read32(INT_STATUS_BASE);
  } while ((st & RXFIFOEMPTY_MASK) && TASK_SCHEDULE);
}
void norflaship_busy_wait(void) {
  uint32_t st = 0;
  do {
    st = norflaship_read32(INT_STATUS_BASE);
  } while ((st & BUSY_MASK) && TASK_SCHEDULE);
}
int norflaship_is_busy(void) {
  uint32_t st = 0;
  st = norflaship_read32(INT_STATUS_BASE);
  return !!(st & BUSY_MASK);
}
void norflaship_clear_fifos(void) {
  norflaship_busy_wait();
  norflaship_write32(RXFIFOCLR_MASK | TXFIFOCLR_MASK, FIFO_CONFIG_BASE);
  norflaship_busy_wait();
}
void norflaship_clear_rxfifo(void) {
  norflaship_busy_wait();
  norflaship_write32(RXFIFOCLR_MASK, FIFO_CONFIG_BASE);
  norflaship_busy_wait();
}
void norflaship_clear_txfifo(void) {
  norflaship_busy_wait();
  norflaship_write32(TXFIFOCLR_MASK, FIFO_CONFIG_BASE);
  norflaship_busy_wait();
}
void norflaship_div(uint32_t div) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  val = (~(CLKDIV_MASK)&val) | (div << CLKDIV_SHIFT);
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
uint32_t norflaship_get_div(void) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  return (val & CLKDIV_MASK) >> CLKDIV_SHIFT;
}
void norflaship_cmdquad(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= CMDQUAD_MASK;
  else
    val &= ~CMDQUAD_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
uint32_t norflaship_get_pos_neg(void) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  return !!(val & POS_NEG_MASK);
}
void norflaship_pos_neg(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= POS_NEG_MASK;
  else
    val &= ~POS_NEG_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
uint32_t norflaship_get_neg_phase(void) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  return !!(val & NEG_PHASE_MASK);
}
void norflaship_neg_phase(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= NEG_PHASE_MASK;
  else
    val &= ~NEG_PHASE_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
uint32_t norflaship_get_samdly(void) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  return (val & SAMDLY_MASK) >> SAMDLY_SHIFT;
}
void norflaship_samdly(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  val = (~(SAMDLY_MASK)&val) | (v << SAMDLY_SHIFT);
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_dual_mode(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= DUALMODE_MASK;
  else
    val &= ~DUALMODE_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_hold_pin(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= HOLDPIN_MASK;
  else
    val &= ~HOLDPIN_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_wpr_pin(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= WPRPIN_MASK;
  else
    val &= ~WPRPIN_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_quad_mode(uint32_t v) {
  uint32_t val = 0;
  norflaship_busy_wait();
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= QUADMODE_MASK;
  else
    val &= ~QUADMODE_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_dummyclc(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  val = (~(DUMMYCLC_MASK)&val) | (v << DUMMYCLC_SHIFT);
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_dummyclcen(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= DUMMYCLCEN_MASK;
  else
    val &= ~DUMMYCLCEN_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_addrbyte4(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE1_CONFIG_BASE);
  if (v)
    val |= ADDRBYTE4_MASK;
  else
    val &= ~ADDRBYTE4_MASK;
  norflaship_write32(val, MODE1_CONFIG_BASE);
}
void norflaship_ruen(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(PULL_UP_DOWN_CONFIG_BASE);
  val = (~(SPIRUEN_MASK)&val) | (v << SPIRUEN_SHIFT);
  norflaship_write32(val, PULL_UP_DOWN_CONFIG_BASE);
}
void norflaship_rden(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(PULL_UP_DOWN_CONFIG_BASE);
  val = (~(SPIRDEN_MASK)&val) | (v << SPIRDEN_SHIFT);
  norflaship_write32(val, PULL_UP_DOWN_CONFIG_BASE);
}
void norflaship_dualiocmd(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE2_CONFIG_BASE);
  val = (~(DUALIOCMD_MASK)&val) | (v << DUALIOCMD_SHIFT);
  norflaship_write32(val, MODE2_CONFIG_BASE);
}
void norflaship_rdcmd(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE2_CONFIG_BASE);
  val = (~(RDCMD_MASK)&val) | (v << RDCMD_SHIFT);
  norflaship_write32(val, MODE2_CONFIG_BASE);
}
void norflaship_frdcmd(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE2_CONFIG_BASE);
  val = (~(FRDCMD_MASK)&val) | (v << FRDCMD_SHIFT);
  norflaship_write32(val, MODE2_CONFIG_BASE);
}
void norflaship_qrdcmd(uint32_t v) {
  uint32_t val = 0;
  val = norflaship_read32(MODE2_CONFIG_BASE);
  val = (~(QRDCMDBIT_MASK)&val) | (v << QRDCMDBIT_SHIFT);
  norflaship_write32(val, MODE2_CONFIG_BASE);
}
uint32_t norflaship_get_rdcmd(void) {
  uint32_t val;
  val = norflaship_read32(MODE2_CONFIG_BASE);
  return ((RDCMD_MASK & val) >> RDCMD_SHIFT);
}
void norflaship_sleep(void) {}
void norflaship_wakeup(void) {}

#endif
