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
#include "cmsis.h"
#include "hal_analogif.h"
#include "hal_location.h"
#include "hal_spi.h"
#include "plat_types.h"

#define ANA_REG_CHIP_ID 0x00

#define ANA_CHIP_ID_SHIFT (4)
#define ANA_CHIP_ID_MASK (0xFFF << ANA_CHIP_ID_SHIFT)
#define ANA_CHIP_ID(n) BITFIELD_VAL(ANA_CHIP_ID, n)
#define ANA_VAL_CHIP_ID 0x18E

// ISPI_ARBITRATOR_ENABLE should be defined when:
// 1) BT and MCU will access RF register at the same time; or
// 2) BT can access PMU/ANA, and BT will access RF register at the same time
//    when MCU is accessing PMU/ANA register

#ifdef ISPI_ARBITRATOR_ENABLE
// Min padding OSC cycles needed: BT=0 MCU=6
// When OSC=26M and SPI=6.5M, min padding SPI cycles is BT=0 MCU=2
#define PADDING_CYCLES 2
#else
#define PADDING_CYCLES 0
#endif

#define ANA_READ_CMD(r) (((1 << 24) | (((r)&0xFF) << 16)) << PADDING_CYCLES)
#define ANA_WRITE_CMD(r, v)                                                    \
  (((((r)&0xFF) << 16) | ((v)&0xFFFF)) << PADDING_CYCLES)
#define ANA_READ_VAL(v) (((v) >> PADDING_CYCLES) & 0xFFFF)

#define ANA_PAGE_1 0xA010
#define ANA_PAGE_0 0xA000

static const BOOT_RODATA_SRAM_LOC uint8_t page_reg[3] = {
    0x00,
    0x60,
    0x80,
};

static const BOOT_RODATA_FLASH_LOC struct HAL_SPI_CFG_T spi_cfg = {
    .clk_delay_half = false,
    .clk_polarity = false,
    .slave = false,
    .dma_rx = false,
    .dma_tx = false,
    .rx_sep_line = false,
    .cs = 0,
    .rate = 6500000,
    .tx_bits = 25 + PADDING_CYCLES,
    .rx_bits = 25 + PADDING_CYCLES,
    .rx_frame_bits = 0,
};

static bool BOOT_BSS_LOC analogif_inited = false;

static int BOOT_TEXT_SRAM_LOC hal_analogif_rawread(unsigned short reg,
                                                   unsigned short *val) {
  int ret;
  unsigned int data;
  unsigned int cmd;

  data = 0;
  cmd = ANA_READ_CMD(reg);
  ret = hal_ispi_recv(&cmd, &data, 4);
  if (ret) {
    return ret;
  }
  *val = ANA_READ_VAL(data);
  return 0;
}

static int BOOT_TEXT_SRAM_LOC hal_analogif_rawwrite(unsigned short reg,
                                                    unsigned short val) {
  int ret;
  unsigned int cmd;

  cmd = ANA_WRITE_CMD(reg, val);
  ret = hal_ispi_send(&cmd, 4);
  if (ret) {
    return ret;
  }
  return 0;
}

int BOOT_TEXT_SRAM_LOC hal_analogif_reg_read(unsigned short reg,
                                             unsigned short *val) {
  uint32_t lock;
  uint32_t idx;
  int ret;

#if defined(USE_CYBERON)
  extern int cyb_efuse_check_status(void);

  if (cyb_efuse_check_status()) {
    if (reg == 0x5e) {
      *val = 49185;
      return 0;
    }
    if (reg == 0x00) {
      *val = 0x20e0;
      return 0;
    }
  }
#endif

  if (reg < 0x100) {
    lock = int_lock();
    ret = hal_analogif_rawread(reg, val);
    int_unlock(lock);
    return ret;
  } else if (reg >= 0x100 && reg <= 0x15F) {
    idx = 0;
  } else if (reg >= 0x160 && reg <= 0x17F) {
    idx = 1;
  } else if (reg >= 0x180 && reg <= 0x1FF) {
    idx = 2;
  } else {
    return -1;
  }

  reg &= 0xFF;

  lock = int_lock();
  hal_analogif_rawwrite(page_reg[idx], ANA_PAGE_1);
  ret = hal_analogif_rawread(reg, val);
  hal_analogif_rawwrite(page_reg[idx], ANA_PAGE_0);
  int_unlock(lock);

  return ret;
}

int BOOT_TEXT_SRAM_LOC hal_analogif_reg_write(unsigned short reg,
                                              unsigned short val) {
  uint32_t lock;
  uint32_t idx;
  int ret;

  if (reg < 0x100) {
    lock = int_lock();
    ret = hal_analogif_rawwrite(reg, val);
    int_unlock(lock);
    return ret;
  } else if (reg >= 0x100 && reg <= 0x15F) {
    idx = 0;
  } else if (reg >= 0x160 && reg <= 0x17F) {
    idx = 1;
  } else if (reg >= 0x180 && reg <= 0x1FF) {
    idx = 2;
  } else {
    return -1;
  }

  reg &= 0xFF;

  lock = int_lock();
  hal_analogif_rawwrite(page_reg[idx], ANA_PAGE_1);
  ret = hal_analogif_rawwrite(reg, val);
  hal_analogif_rawwrite(page_reg[idx], ANA_PAGE_0);
  int_unlock(lock);

  return ret;
}

int BOOT_TEXT_FLASH_LOC hal_analogif_open(void) {
  int ret;
  unsigned short chip_id;
  const struct HAL_SPI_CFG_T *cfg_ptr;
  struct HAL_SPI_CFG_T cfg;

  if (analogif_inited) {
    // Restore the nominal rate
    cfg_ptr = &spi_cfg;
  } else {
    analogif_inited = true;
    // Crystal freq is unknown yet. Let SPI run on half of the nominal rate
    cfg = spi_cfg;
    cfg.rate /= 2;
    cfg_ptr = &cfg;
  }

  ret = hal_ispi_open(cfg_ptr);
  if (ret) {
    return ret;
  }

  ret = hal_analogif_rawread(ANA_REG_CHIP_ID, &chip_id);
  if (ret) {
    return ret;
  }

  if (GET_BITFIELD(chip_id, ANA_CHIP_ID) != ANA_VAL_CHIP_ID) {
    return -1;
  }

  return 0;
}
