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
#include "hal_cmu.h"
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_cmu)
#ifdef AON_CMU_BASE
#include CHIP_SPECIFIC_HDR(reg_aoncmu)
#endif
#include "cmsis.h"
#include "hal_analogif.h"
#include "hal_bootmode.h"
#include "hal_cache.h"
#include "hal_chipid.h"
#include "hal_iomux.h"
#include "hal_location.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"

#if defined(CHIP_HAS_USB) &&                                                   \
    (defined(MCU_HIGH_PERFORMANCE_MODE) &&                                     \
     !(defined(ULTRA_LOW_POWER) || defined(OSC_26M_X4_AUD2BB)))
#define USB_PLL_INIT_ON
#endif
#if (!defined(ULTRA_LOW_POWER) && !defined(OSC_26M_X4_AUD2BB)) ||              \
    (!defined(FLASH_LOW_SPEED) && !defined(OSC_26M_X4_AUD2BB)) ||              \
    (defined(PSRAM_ENABLE) && !defined(PSRAM_LOW_SPEED))
#define AUD_PLL_INIT_ON
#endif

// SIMU_RES
#define CMU_SIMU_RES_PASSED (0x9A55)
#define CMU_SIMU_RES_FAILED (0xFA11)

typedef void (*HAL_POWER_DOWN_WAKEUP_HANDLER)(void);

static struct CMU_T *const cmu = (struct CMU_T *)CMU_BASE;
#ifdef AON_CMU_BASE
static struct AONCMU_T *const POSSIBLY_UNUSED aoncmu =
    (struct AONCMU_T *)AON_CMU_BASE;
#endif

#ifdef HAL_CMU_VALID_CRYSTAL_FREQ
static const uint32_t valid_crystal_freq_list[] = HAL_CMU_VALID_CRYSTAL_FREQ;
#define CRYSTAL_FREQ_ATTR BOOT_DATA_LOC
#else
#define CRYSTAL_FREQ_ATTR const
#endif

static uint32_t CRYSTAL_FREQ_ATTR crystal_freq = HAL_CMU_DEFAULT_CRYSTAL_FREQ;

void BOOT_TEXT_FLASH_LOC hal_cmu_set_crystal_freq_index(uint32_t index) {
#ifdef HAL_CMU_VALID_CRYSTAL_FREQ
  if (index >= ARRAY_SIZE(valid_crystal_freq_list)) {
    index %= ARRAY_SIZE(valid_crystal_freq_list);
  }
  crystal_freq = valid_crystal_freq_list[index];
#endif
}

uint32_t BOOT_TEXT_SRAM_LOC hal_cmu_get_crystal_freq(void) {
  return crystal_freq;
}

uint32_t BOOT_TEXT_FLASH_LOC hal_cmu_get_default_crystal_freq(void) {
  return HAL_CMU_DEFAULT_CRYSTAL_FREQ;
}

void hal_cmu_write_lock(void) { cmu->WRITE_UNLOCK = 0xCAFE0000; }

void hal_cmu_write_unlock(void) { cmu->WRITE_UNLOCK = 0xCAFE0001; }

void hal_cmu_sys_reboot(void) { hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL); }

void hal_cmu_simu_init(void) { cmu->SIMU_RES = 0; }

void hal_cmu_simu_pass(void) { cmu->SIMU_RES = CMU_SIMU_RES_PASSED; }

void hal_cmu_simu_fail(void) { cmu->SIMU_RES = CMU_SIMU_RES_FAILED; }

void hal_cmu_simu_tag(uint8_t shift) { cmu->SIMU_RES |= (1 << shift); }

void hal_cmu_simu_set_val(uint32_t val) { cmu->SIMU_RES = val; }

uint32_t hal_cmu_simu_get_val(void) { return cmu->SIMU_RES; }

void hal_cmu_set_wakeup_pc(uint32_t pc) {
#ifdef RAMRET_BASE
  uint32_t *wake_pc =
#ifdef CHIP_BEST2000
      (uint32_t *)RAMRET_BASE;
#else
      (uint32_t *)&aoncmu->WAKEUP_PC;

  STATIC_ASSERT(sizeof(HAL_POWER_DOWN_WAKEUP_HANDLER) <= sizeof(uint32_t),
                "Invalid func ptr size");
#endif

  *wake_pc = pc;
#endif
}

void hal_cmu_rom_wakeup_check(void) {
#ifdef RAMRET_BASE
  union HAL_HW_BOOTMODE_T hw;
  uint32_t sw;
  HAL_POWER_DOWN_WAKEUP_HANDLER *wake_fn =
#ifdef CHIP_BEST2000
      (HAL_POWER_DOWN_WAKEUP_HANDLER *)RAMRET_BASE;
#else
      (HAL_POWER_DOWN_WAKEUP_HANDLER *)&aoncmu->WAKEUP_PC;
#endif

  hw = hal_rom_hw_bootmode_get();
  if (hw.watchdog == 0 && hw.global == 0) {
    sw = hal_sw_bootmode_get();
    if ((sw & HAL_SW_BOOTMODE_POWER_DOWN_WAKEUP) && *wake_fn) {
      (*wake_fn)();
    }
  }

  *wake_fn = NULL;
#endif
}

#ifndef HAL_CMU_PLL_T
void hal_cmu_rom_enable_pll(void) {
#ifdef CHIP_HAS_USB
  hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SYS);
  hal_cmu_sys_select_pll(HAL_CMU_PLL_USB);
  hal_cmu_flash_select_pll(HAL_CMU_PLL_USB);
#else
  hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
  hal_cmu_sys_select_pll(HAL_CMU_PLL_AUD);
  hal_cmu_flash_select_pll(HAL_CMU_PLL_AUD);
#endif
}

void hal_cmu_programmer_enable_pll(void) {
  hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
  hal_cmu_flash_select_pll(HAL_CMU_PLL_AUD);
  hal_cmu_sys_select_pll(HAL_CMU_PLL_AUD);
}

void BOOT_TEXT_FLASH_LOC hal_cmu_init_pll_selection(void) {
  // !!!!!!
  // CAUTION:
  // hal_cmu_pll_enable()/hal_cmu_pll_disable() must be called after
  // hal_chipid_init(), for the init div values are extracted in
  // hal_chipid_init().
  // !!!!!!

#if defined(CHIP_BEST1000) || defined(CHIP_BEST2000)
#ifdef CHIP_HAS_USB
  // Enable USB PLL before switching (clock mux requirement)
  // -- USB PLL might not be started in ROM
  hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SYS);
#endif
  hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
#else // !(best1000 || best2000)
  // Disable the PLL which might be enabled in ROM
#ifdef CHIP_HAS_USB
  hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_ALL);
#else
  hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_ALL);
#endif
#endif // !(best1000 || best2000)

#ifdef FLASH_LOW_SPEED
#ifdef CHIP_HAS_USB
  // Switch flash clock to USB PLL, and then shutdown USB PLL,
  // to save power consumed in clock divider
  hal_cmu_flash_select_pll(HAL_CMU_PLL_USB);
#endif
#else
  // Switch flash clock to audio PLL
  hal_cmu_flash_select_pll(HAL_CMU_PLL_AUD);
#endif

#ifdef CHIP_HAS_PSRAM
#ifdef PSRAM_LOW_SPEED
#ifdef CHIP_HAS_USB
  // Switch psram clock to USB PLL, and then shutdown USB PLL,
  // to save power consumed in clock divider
  hal_cmu_mem_select_pll(HAL_CMU_PLL_USB);
#endif
#else
  // Switch psram clock to audio PLL
  hal_cmu_mem_select_pll(HAL_CMU_PLL_AUD);
#endif
#endif

  // Select system PLL after selecting flash/psram PLLs
#ifdef ULTRA_LOW_POWER
  hal_cmu_low_freq_mode_init();
#else
#if defined(MCU_HIGH_PERFORMANCE_MODE) && defined(CHIP_HAS_USB)
  // Switch system clocks to USB PLL
  hal_cmu_sys_select_pll(HAL_CMU_PLL_USB);
#else
  // Switch system clocks to audio PLL
  hal_cmu_sys_select_pll(HAL_CMU_PLL_AUD);
#endif
#endif

#if defined(CHIP_BEST1000) || defined(CHIP_BEST2000)
#ifndef USB_PLL_INIT_ON
  // Disable USB PLL after switching (clock mux requirement)
  hal_cmu_pll_disable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SYS);
#endif
#ifndef AUD_PLL_INIT_ON
  hal_cmu_pll_disable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
#endif
#else // !(best1000 || best2000)
#ifdef USB_PLL_INIT_ON
  hal_cmu_pll_enable(HAL_CMU_PLL_USB, HAL_CMU_PLL_USER_SYS);
#endif
#ifdef AUD_PLL_INIT_ON
  hal_cmu_pll_enable(HAL_CMU_PLL_AUD, HAL_CMU_PLL_USER_SYS);
#endif
#endif // !(best1000 || best2000)

#if defined(MCU_HIGH_PERFORMANCE_MODE) && !defined(ULTRA_LOW_POWER) &&         \
    defined(OSC_26M_X4_AUD2BB)
#error "Error configuration: MCU_HIGH_PERFORMANCE_MODE has no effect"
#endif
}
#endif // !HAL_CMU_PLL_T

static void BOOT_TEXT_FLASH_LOC hal_cmu_init_periph_clock(void) {
#ifdef PERIPH_PLL_FREQ
  hal_cmu_periph_set_div(1);
#endif

  // TODO: Move the following SDIO freq setting to hal_sdio.c
#ifdef CHIP_HAS_SDIO
  hal_cmu_sdio_set_freq(HAL_CMU_PERIPH_FREQ_26M);
#endif
}

void hal_cmu_rom_setup(void) {
  hal_cmu_lpu_wait_26m_ready();
  hal_cmu_simu_init();
  hal_cmu_rom_clock_init();
  hal_cmu_timer0_select_slow();
#ifdef TIMER1_BASE
  hal_cmu_timer1_select_fast();
#endif
  hal_sys_timer_open();

  // Init sys clock
  hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);

  // Init flash clock (this should be done before load_boot_settings, for
  // security register read)
  hal_cmu_flash_set_freq(HAL_CMU_FREQ_26M);
  // Reset flash controller (for JTAG reset and run)
  // Enable flash controller (flash controller is reset by default since
  // BEST1400)
  hal_cmu_reset_set(HAL_CMU_MOD_O_FLASH);
  hal_cmu_reset_set(HAL_CMU_MOD_H_FLASH);
  hal_cmu_reset_clear(HAL_CMU_MOD_H_FLASH);
  hal_cmu_reset_clear(HAL_CMU_MOD_O_FLASH);

  // Disable cache (for JTAG reset and run)
  hal_cache_disable(HAL_CACHE_ID_I_CACHE);
  hal_cache_disable(HAL_CACHE_ID_D_CACHE);

  // Init APB clock
  hal_cmu_apb_init_div();
}

void hal_cmu_programmer_setup(void) {
  hal_cmu_ema_init();
  hal_sys_timer_open();

#ifdef JTAG_ENABLE
  hal_iomux_set_jtag();
  hal_cmu_jtag_clock_enable();
#endif

  int ret;
  // Open analogif (ISPI)
  ret = hal_analogif_open();
  if (ret) {
    hal_cmu_simu_tag(31);
    do {
      volatile int i = 0;
      i++;
    } while (1);
  }
  // Init chip id
  // 1) Read id from ana/rf/pmu
  // 2) Init clock settings in ana/rf/pmu if the default h/w register values are
  // bad
  hal_chipid_init();

  // Enable OSC X2/X4 in cmu after enabling their source in hal_chipid_init()
  hal_cmu_osc_x2_enable();
  hal_cmu_osc_x4_enable();
}

void BOOT_TEXT_FLASH_LOC hal_cmu_setup(void) {
  int ret;
  enum HAL_CMU_FREQ_T freq;

  hal_iomux_set_default_config();
#ifdef JTAG_ENABLE
  hal_iomux_set_jtag();
  hal_cmu_jtag_clock_enable();
#endif
  hal_cmu_module_init_state();
  hal_cmu_ema_init();
  hal_cmu_timer0_select_slow();
#ifdef TIMER1_BASE
  hal_cmu_timer1_select_fast();
#endif
  hal_sys_timer_open();
  hal_hw_bootmode_init();

  // Init system/flash/memory clocks before initializing clock setting
  // and before switching PLL
  hal_norflash_set_freq(HAL_CMU_FREQ_26M);
  hal_cmu_mem_set_freq(HAL_CMU_FREQ_26M);
  hal_cmu_sys_set_freq(HAL_CMU_FREQ_26M);

  // Set ISPI module freq
  hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
  // Open analogif (ISPI)
  ret = hal_analogif_open();
  if (ret) {
    hal_cmu_simu_tag(31);
    do {
      volatile int i = 0;
      i++;
    } while (1);
  }
  // Init chip id
  // 1) Read id from ana/rf/pmu
  // 2) Init clock settings in ana/rf/pmu if the default h/w register values are
  // bad
  hal_chipid_init();

#ifdef CALIB_SLOW_TIMER
  // Calib slow timer after determining the crystal freq
  hal_sys_timer_calib();
#endif

  // Enable OSC X2/X4 in cmu after enabling their source in hal_chipid_init()
  hal_cmu_osc_x2_enable();
  hal_cmu_osc_x4_enable();

  // Init PLL selection
  hal_cmu_init_pll_selection();

  // Init peripheral clocks
  hal_cmu_init_periph_clock();

  // Sleep setting
#ifdef NO_LPU_26M
  while (hal_cmu_lpu_init(HAL_CMU_LPU_CLK_NONE) == -1)
    ;
#else
  while (hal_cmu_lpu_init(HAL_CMU_LPU_CLK_26M) == -1)
    ;
#endif
    // Init sys freq after applying the sleep setting (which might change sys
    // freq)
#ifdef NO_LPU_26M
  hal_sys_timer_delay(MS_TO_TICKS(20));
#endif

  // Init system clock
#ifdef ULTRA_LOW_POWER
  freq = HAL_CMU_FREQ_52M;
#else
  freq = HAL_CMU_FREQ_104M;
#endif
  hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, freq);

  // Init flash
  hal_norflash_init();
}
