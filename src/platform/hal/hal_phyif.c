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
#ifdef CHIP_HAS_SPIPHY

#include "hal_phyif.h"
#include "cmsis.h"
#include "hal_location.h"
#include "hal_spi.h"
#include "plat_types.h"

#define PHY_READ_CMD(r) ((1 << 24) | (((r)&0xFF) << 16))
#define PHY_WRITE_CMD(r, v) ((((r)&0xFF) << 16) | ((v)&0xFFFF))
#define PHY_READ_VAL(v) ((v)&0xFFFF)

#define SPIPHY_REG_CS(r) ((r) >> 12)
#define SPIPHY_REG_PAGE(r) (((r) >> 8) & 0xF)
#define SPIPHY_REG_OFFSET(r) ((r)&0xFF)

static const struct HAL_SPI_CFG_T spi_cfg = {
    .clk_delay_half = false,
    .clk_polarity = false,
    .slave = false,
    .dma_rx = false,
    .dma_tx = false,
    .rx_sep_line = true,
    .cs = 0,
    .rate = 6500000,
    .tx_bits = 25,
    .rx_bits = 25,
    .rx_frame_bits = 0,
};

static uint8_t BOOT_BSS_LOC phyif_open_map;

static uint8_t BOOT_BSS_LOC phy_cs;

static int hal_phyif_rawread(unsigned short reg, unsigned short *val) {
  int ret;
  unsigned int data;
  unsigned int cmd;

  data = 0;
  cmd = PHY_READ_CMD(reg);
  ret = hal_spiphy_recv(&cmd, &data, 4);
  if (ret) {
    return ret;
  }
  *val = PHY_READ_VAL(data);
  return 0;
}

static int hal_phyif_rawwrite(unsigned short reg, unsigned short val) {
  int ret;
  unsigned int cmd;

  cmd = PHY_WRITE_CMD(reg, val);
  ret = hal_spiphy_send(&cmd, 4);
  if (ret) {
    return ret;
  }
  return 0;
}

int hal_phyif_reg_read(unsigned short reg, unsigned short *val) {
  uint32_t lock;
  int ret;
  uint8_t cs;
  // uint8_t page;

  cs = SPIPHY_REG_CS(reg);
  // page = SPIPHY_REG_PAGE(reg);
  reg = SPIPHY_REG_OFFSET(reg);

  lock = int_lock();
  if (cs != phy_cs) {
    hal_spiphy_activate_cs(cs);
    phy_cs = cs;
  }
  ret = hal_phyif_rawread(reg, val);
  int_unlock(lock);

  return ret;
}

int hal_phyif_reg_write(unsigned short reg, unsigned short val) {
  uint32_t lock;
  int ret;
  uint8_t cs;
  // uint8_t page;

  cs = SPIPHY_REG_CS(reg);
  // page = SPIPHY_REG_PAGE(reg);
  reg = SPIPHY_REG_OFFSET(reg);

  lock = int_lock();
  if (cs != phy_cs) {
    hal_spiphy_activate_cs(cs);
    phy_cs = cs;
  }
  ret = hal_phyif_rawwrite(reg, val);
  int_unlock(lock);

  return ret;
}

int hal_phyif_open(uint32_t cs) {
  int ret;
  uint32_t lock;

  ret = 0;

  lock = int_lock();
  if (phyif_open_map == 0) {
    ret = hal_spiphy_open(&spi_cfg);
  }
  if (ret == 0) {
    phyif_open_map |= (1 << cs);
  }
  int_unlock(lock);

  return ret;
}

int hal_phyif_close(uint32_t cs) {
  int ret;
  uint32_t lock;

  ret = 0;

  lock = int_lock();
  phyif_open_map &= ~(1 << cs);
  if (phyif_open_map == 0) {
    ret = hal_spiphy_close(spi_cfg.cs);
  }
  int_unlock(lock);

  return ret;
}

#endif
