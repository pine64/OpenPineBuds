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
#include "hal_location.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_psc)

#define PSC_WRITE_ENABLE 0xCAFE0000

static struct AONPSC_T *const psc = (struct AONPSC_T *)AON_PSC_BASE;

void BOOT_TEXT_FLASH_LOC hal_psc_init(void) {
  // Setup wakeup mask
  psc->REG_080 = 0xFFFFFFFF;
  psc->REG_084 = 0xFFFFFFFF;
}

void SRAM_TEXT_LOC hal_psc_core_auto_power_down(void) {
  psc->REG_018 = PSC_WRITE_ENABLE | 0;
  psc->REG_000 = PSC_WRITE_ENABLE | PSC_AON_MCU_PG_AUTO_EN;
  psc->REG_010 = PSC_WRITE_ENABLE | PSC_AON_MCU_POWERDN_START;
}

void SRAM_TEXT_LOC hal_psc_mcu_auto_power_up(void) {
  psc->REG_014 = PSC_WRITE_ENABLE | PSC_AON_MCU_POWERUP_START;
}

void BOOT_TEXT_FLASH_LOC hal_psc_codec_enable(void) {
  psc->REG_078 = PSC_WRITE_ENABLE | PSC_AON_CODEC_PSW_EN_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_REG | PSC_AON_CODEC_ISO_EN_DR |
                 PSC_AON_CODEC_ISO_EN_REG | PSC_AON_CODEC_CLK_STOP_DR |
                 PSC_AON_CODEC_CLK_STOP_REG;
  hal_sys_timer_delay(MS_TO_TICKS(1));
  psc->REG_078 = PSC_WRITE_ENABLE | PSC_AON_CODEC_PSW_EN_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_ISO_EN_DR |
                 PSC_AON_CODEC_ISO_EN_REG | PSC_AON_CODEC_CLK_STOP_DR |
                 PSC_AON_CODEC_CLK_STOP_REG;
  psc->REG_078 = PSC_WRITE_ENABLE | PSC_AON_CODEC_PSW_EN_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_ISO_EN_DR |
                 PSC_AON_CODEC_CLK_STOP_DR | PSC_AON_CODEC_CLK_STOP_REG;
  psc->REG_078 = PSC_WRITE_ENABLE | PSC_AON_CODEC_PSW_EN_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_DR | PSC_AON_CODEC_ISO_EN_DR |
                 PSC_AON_CODEC_CLK_STOP_DR;
}

void BOOT_TEXT_FLASH_LOC hal_psc_codec_disable(void) {
  psc->REG_078 = PSC_WRITE_ENABLE | PSC_AON_CODEC_PSW_EN_DR |
                 PSC_AON_CODEC_PSW_EN_REG | PSC_AON_CODEC_RESETN_ASSERT_DR |
                 PSC_AON_CODEC_RESETN_ASSERT_REG | PSC_AON_CODEC_ISO_EN_DR |
                 PSC_AON_CODEC_ISO_EN_REG | PSC_AON_CODEC_CLK_STOP_DR |
                 PSC_AON_CODEC_CLK_STOP_REG;
}

void BOOT_TEXT_FLASH_LOC hal_psc_bt_enable(void) {
  psc->REG_038 = PSC_WRITE_ENABLE | PSC_AON_BT_PSW_EN_DR |
                 PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_RESETN_ASSERT_REG |
                 PSC_AON_BT_ISO_EN_DR | PSC_AON_BT_ISO_EN_REG |
                 PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
  hal_sys_timer_delay(MS_TO_TICKS(1));
  psc->REG_038 = PSC_WRITE_ENABLE | PSC_AON_BT_PSW_EN_DR |
                 PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_ISO_EN_DR |
                 PSC_AON_BT_ISO_EN_REG | PSC_AON_BT_CLK_STOP_DR |
                 PSC_AON_BT_CLK_STOP_REG;
  psc->REG_038 = PSC_WRITE_ENABLE | PSC_AON_BT_PSW_EN_DR |
                 PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_ISO_EN_DR |
                 PSC_AON_BT_CLK_STOP_DR | PSC_AON_BT_CLK_STOP_REG;
  psc->REG_038 = PSC_WRITE_ENABLE | PSC_AON_BT_PSW_EN_DR |
                 PSC_AON_BT_RESETN_ASSERT_DR | PSC_AON_BT_ISO_EN_DR |
                 PSC_AON_BT_CLK_STOP_DR;

#ifdef JTAG_BT
  psc->REG_064 |= PSC_AON_CODEC_RESERVED(1 << 3);
  psc->REG_064 &= ~PSC_AON_CODEC_RESERVED(1 << 2);
#endif
}

void BOOT_TEXT_FLASH_LOC hal_psc_bt_disable(void) {
#ifdef JTAG_BT
  psc->REG_064 &= ~PSC_AON_CODEC_RESERVED(1 << 3);
  psc->REG_064 |= PSC_AON_CODEC_RESERVED(1 << 2);
#endif

  psc->REG_038 = PSC_WRITE_ENABLE | PSC_AON_BT_PSW_EN_DR |
                 PSC_AON_BT_PSW_EN_REG | PSC_AON_BT_RESETN_ASSERT_DR |
                 PSC_AON_BT_RESETN_ASSERT_REG | PSC_AON_BT_ISO_EN_DR |
                 PSC_AON_BT_ISO_EN_REG | PSC_AON_BT_CLK_STOP_DR |
                 PSC_AON_BT_CLK_STOP_REG;
}
