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

#ifdef BTDUMP_BASE

#include "hal_btdump.h"
#include "hal_cmu.h"

#define BTDUMP_DMA_CTL_REG_OFFSET 0x30
#define BTDUMP_DMA_CTL_MASK 0xFFFFFFFE
#define BTDUMP_DEVICE_EN_MASK 0xFFFFFFFE
#define BTDUMP_FM_PSD_MASK 0xFFFFFFDF
#define BTDUMP_DEVICE_EN (1 << 0)
#define BTDUMP_DMA_CTL_EN (1 << 0)
#define BTDUMP_FM_PSD_EN (1 << 5)
#define BTDUMP_DMA_CTL_DISABLE (0 << 0)
#define BTDUMP_DEVICE_DISABLE (0 << 0)

#define btdump_read32(b, a) (*(volatile uint32_t *)(b + a))

#define btdump_write32(v, b, a) ((*(volatile uint32_t *)(b + a)) = v)

void hal_btdump_clk_enable(void) {
  hal_cmu_clock_enable(HAL_CMU_MOD_H_BT_DUMP);
  hal_cmu_reset_clear(HAL_CMU_MOD_H_BT_DUMP);
}

void hal_btdump_clk_disable(void) {
  hal_cmu_reset_set(HAL_CMU_MOD_H_BT_DUMP);
  hal_cmu_clock_disable(HAL_CMU_MOD_H_BT_DUMP);
}

void hal_btdump_enable(void) {
  uint32_t val;

  val = btdump_read32(BTDUMP_BASE, BTDUMP_DMA_CTL_REG_OFFSET);
  val &= BTDUMP_DMA_CTL_MASK;
  val |= BTDUMP_DMA_CTL_EN;
  btdump_write32(val, BTDUMP_BASE, BTDUMP_DMA_CTL_REG_OFFSET);

  val = btdump_read32(BTDUMP_BASE, 0);
  val &= BTDUMP_DEVICE_EN_MASK;
  val |= BTDUMP_DEVICE_EN;
  val &= BTDUMP_FM_PSD_MASK;
  val |= BTDUMP_FM_PSD_EN;
  btdump_write32(val, BTDUMP_BASE, 0);
}

void hal_btdump_disable(void) {
  uint32_t val;

  val = btdump_read32(BTDUMP_BASE, BTDUMP_DMA_CTL_REG_OFFSET);
  val &= BTDUMP_DMA_CTL_MASK;
  val |= BTDUMP_DMA_CTL_DISABLE;
  btdump_write32(val, BTDUMP_BASE, BTDUMP_DMA_CTL_REG_OFFSET);

  val = btdump_read32(BTDUMP_BASE, 0);
  val &= BTDUMP_DEVICE_EN_MASK;
  val |= BTDUMP_DEVICE_DISABLE;
  btdump_write32(val, BTDUMP_BASE, 0);
}

#endif