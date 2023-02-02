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
#include "hal_cache.h"
#include "cmsis.h"
#include "hal_location.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#if (CHIP_CACHE_VER < 2)
#include "hal_norflash.h"
#endif
#include "hal_timer.h"

#define HAL_CACHE_YES 1
#define HAL_CACHE_NO 0

/* cache controller */
#if 0
#elif defined(CHIP_BEST1000)
#define CACHE_SIZE 0x1000
#define CACHE_LINE_SIZE 0x10
#elif defined(CHIP_BEST2000) || defined(CHIP_BEST3001)
#define CACHE_SIZE 0x2000
#define CACHE_LINE_SIZE 0x10
#elif defined(CHIP_BEST2001)
#define CACHE_SIZE 0x4000
#define CACHE_LINE_SIZE 0x20
#else
#define CACHE_SIZE 0x2000
#define CACHE_LINE_SIZE 0x20
#endif

#define CACHE_ASSOCIATIVITY_WAY_NUM 4

/* reg value */
#define CACHE_ENABLE_REG_OFFSET 0x00
#define CACHE_INI_CMD_REG_OFFSET 0x04
#define WRITEBUFFER_ENABLE_REG_OFFSET 0x08
#define WRITEBUFFER_FLUSH_REG_OFFSET 0x0C
#define LOCK_UNCACHEABLE_REG_OFFSET 0x10
#define INVALIDATE_ADDRESS_REG_OFFSET 0x14
#define INVALIDATE_SET_CMD_REG_OFFSET 0x18
// Since best2300
#define MONITOR_ENABLE_REG_OFFSET 0x1C
#define MONITOR_CNT_READ_HIT0_REG_OFFSET 0x20
#define MONITOR_CNT_READ_HIT1_REG_OFFSET 0x24
#define MONITOR_CNT_READ_MISS0_REG_OFFSET 0x28
#define MONITOR_CNT_READ_MISS1_REG_OFFSET 0x2C
// Since best2300p
#define STATUS_REG_OFFSET 0x30
// Since best2300p
#define SYNC_CMD_REG_OFFSET 0x34

#define CACHE_EN (1 << 0)
// Since best2300
#define WRAP_EN (1 << 1)

#define WRITEBUFFER_EN (1 << 0)
// Since best2300
#define WRITE_BACK_EN (1 << 1)

#define LOCK_UNCACHEABLE (1 << 0)

// Since best2300
#define MONITOR_EN (1 << 0)

#define CNT_READ_HIT_31_0_SHIFT (0)
#define CNT_READ_HIT_31_0_MASK (0xFFFFFFFF << CNT_READ_HIT_31_0_SHIFT)
#define CNT_READ_HIT_31_0(n) BITFIELD_VAL(CNT_READ_HIT_31_0, n)

#define CNT_READ_HIT_39_32_SHIFT (0)
#define CNT_READ_HIT_39_32_MASK (0xFF << CNT_READ_HIT_39_32_SHIFT)
#define CNT_READ_HIT_39_32(n) BITFIELD_VAL(CNT_READ_HIT_39_32, n)

// Since best2300p
#define STATUS_FETCHING (1 << 0)

/* read write */
#define cacheip_write32(v, b, a) (*((volatile uint32_t *)(b + a)) = v)
#define cacheip_read32(b, a) (*((volatile uint32_t *)(b + a)))

__STATIC_FORCEINLINE void cacheip_enable_cache(uint32_t reg_base, uint32_t v) {
  uint32_t val;
  if (v) {
    val = CACHE_EN;
  } else {
    val = 0;
  }
  cacheip_write32(val, reg_base, CACHE_ENABLE_REG_OFFSET);
}
__STATIC_FORCEINLINE void cacheip_enable_wrap(uint32_t reg_base, uint32_t v) {
  uint32_t val;
  val = cacheip_read32(reg_base, CACHE_ENABLE_REG_OFFSET);
  if (v) {
    val |= WRAP_EN;
  } else {
    val &= ~WRAP_EN;
  }
  cacheip_write32(val, reg_base, CACHE_ENABLE_REG_OFFSET);
}
__STATIC_FORCEINLINE POSSIBLY_UNUSED int
cacheip_wrap_enabled(uint32_t reg_base) {
  uint32_t val;
  val = cacheip_read32(reg_base, CACHE_ENABLE_REG_OFFSET);
  return !!(val & WRAP_EN);
}
__STATIC_FORCEINLINE void cacheip_init_cache(uint32_t reg_base) {
  cacheip_write32(1, reg_base, CACHE_INI_CMD_REG_OFFSET);
}
__STATIC_FORCEINLINE void cacheip_enable_writebuffer(uint32_t reg_base,
                                                     uint32_t v) {
  // PSRAM controller V2 has an embedded write buffer and the cache write buffer
  // can be ignored
#if defined(CHIP_HAS_PSRAM) && defined(PSRAM_ENABLE) &&                        \
    defined(CHIP_PSRAM_CTRL_VER) && (CHIP_PSRAM_CTRL_VER == 1)
  uint32_t val;

  val = cacheip_read32(reg_base, WRITEBUFFER_ENABLE_REG_OFFSET);
  if (v) {
    val |= WRITEBUFFER_EN;
  } else {
    val &= ~WRITEBUFFER_EN;
  }
  cacheip_write32(val, reg_base, WRITEBUFFER_ENABLE_REG_OFFSET);
#endif
}
__STATIC_FORCEINLINE void cacheip_enable_writeback(uint32_t reg_base,
                                                   uint32_t v) {
  // Cache implements write back feature since PSRAM controller V2
#if !defined(CHIP_BEST2001)
#if (defined(CHIP_HAS_PSRAM) && defined(PSRAM_ENABLE) &&                       \
     defined(CHIP_PSRAM_CTRL_VER) && (CHIP_PSRAM_CTRL_VER >= 2)) ||            \
    (defined(CHIP_HAS_PSRAMUHS) && defined(PSRAMUHS_ENABLE))
  uint32_t val;

  val = cacheip_read32(reg_base, WRITEBUFFER_ENABLE_REG_OFFSET);
  if (v) {
    val |= WRITE_BACK_EN;
  } else {
    val &= ~WRITE_BACK_EN;
  }
  cacheip_write32(val, reg_base, WRITEBUFFER_ENABLE_REG_OFFSET);
#endif
#endif
}
__STATIC_FORCEINLINE void cacheip_flush_writebuffer(uint32_t reg_base) {
  cacheip_write32(1, reg_base, WRITEBUFFER_FLUSH_REG_OFFSET);
}
__STATIC_FORCEINLINE void cacheip_set_invalidate_address(uint32_t reg_base,
                                                         uint32_t v) {
  cacheip_write32(v, reg_base, INVALIDATE_ADDRESS_REG_OFFSET);
}
__STATIC_FORCEINLINE void cacheip_trigger_invalidate(uint32_t reg_base) {
  cacheip_write32(1, reg_base, INVALIDATE_SET_CMD_REG_OFFSET);
}
__STATIC_FORCEINLINE void cacheip_trigger_sync(uint32_t reg_base) {
  cacheip_write32(1, reg_base, SYNC_CMD_REG_OFFSET);
}
/* cache controller end */

/* hal api */
__STATIC_FORCEINLINE uint32_t _cache_get_reg_base(enum HAL_CACHE_ID_T id) {
  uint32_t base;

  if (id == HAL_CACHE_ID_I_CACHE) {
    base = ICACHE_CTRL_BASE;
  } else if (id == HAL_CACHE_ID_D_CACHE) {
#ifdef DCACHE_CTRL_BASE
    base = DCACHE_CTRL_BASE;
#else
    base = 0;
#endif
  } else {
    base = 0;
  }

  return base;
}
uint8_t BOOT_TEXT_FLASH_LOC hal_cache_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_init_cache(reg_base);
  cacheip_enable_cache(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t SRAM_TEXT_LOC hal_cache_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#if (CHIP_CACHE_VER >= 2)
  uint32_t val;

  do {
    val = cacheip_read32(reg_base, STATUS_REG_OFFSET);
  } while (val & STATUS_FETCHING);
#else
  uint32_t time;

  time = hal_sys_timer_get();
  while (hal_norflash_busy() && (hal_sys_timer_get() - time) < MS_TO_TICKS(2))
    ;
  // Delay for at least 8 cycles till the cache becomes idle
  for (int delay = 0; delay < 8; delay++) {
    asm volatile("nop");
  }
#endif
#endif

  cacheip_enable_cache(reg_base, HAL_CACHE_NO);

  return 0;
}
uint8_t BOOT_TEXT_FLASH_LOC
hal_cache_writebuffer_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writebuffer(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t hal_cache_writebuffer_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writebuffer(reg_base, HAL_CACHE_NO);

  return 0;
}
uint8_t hal_cache_writebuffer_flush(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_flush_writebuffer(reg_base);

  return 0;
}
uint8_t BOOT_TEXT_FLASH_LOC hal_cache_writeback_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writeback(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t hal_cache_writeback_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writeback(reg_base, HAL_CACHE_NO);

  return 0;
}
// Wrap is enabled during flash init
uint8_t BOOT_TEXT_SRAM_LOC hal_cache_wrap_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_wrap(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t hal_cache_wrap_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_wrap(reg_base, HAL_CACHE_NO);

  return 0;
}
// Flash timing calibration might need to invalidate cache
uint8_t BOOT_TEXT_SRAM_LOC hal_cache_invalidate(enum HAL_CACHE_ID_T id,
                                                uint32_t start_address,
                                                uint32_t len) {
  uint32_t reg_base;
  uint32_t end_address;
  uint32_t lock;

#ifndef DCACHE_CTRL_BASE
  if (id == HAL_CACHE_ID_D_CACHE) {
    id = HAL_CACHE_ID_I_CACHE;
  }
#endif

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  lock = int_lock_global();

#if defined(CHIP_BEST2300) || defined(CHIP_BEST1400)
  uint32_t time;

  time = hal_sys_timer_get();
  while (hal_norflash_busy() && (hal_sys_timer_get() - time) < MS_TO_TICKS(2))
    ;
  // Delay for at least 8 cycles till the cache becomes idle
  for (int delay = 0; delay < 8; delay++) {
    asm volatile("nop");
  }
#endif

  if (len >= CACHE_SIZE / 2) {
    cacheip_init_cache(reg_base);
    cacheip_init_cache(reg_base);
  } else {
    end_address = start_address + len;
    start_address &= (~(CACHE_LINE_SIZE - 1));
    while (start_address < end_address) {
      cacheip_set_invalidate_address(reg_base, start_address);
      cacheip_trigger_invalidate(reg_base);
      cacheip_trigger_invalidate(reg_base);
      start_address += CACHE_LINE_SIZE;
    }
  }

  int_unlock_global(lock);

  return 0;
}
uint8_t hal_cache_invalidate_all(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  // warning BEST1501 may change this reg offset to 0x38
  cacheip_write32(1, reg_base, CACHE_INI_CMD_REG_OFFSET);
  cacheip_write32(1, reg_base, CACHE_INI_CMD_REG_OFFSET);
  return 0;
}
uint8_t hal_cache_sync(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cache_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_trigger_sync(reg_base);

  return 0;
}

#ifdef CHIP_HAS_CP
__STATIC_FORCEINLINE uint32_t _cachecp_get_reg_base(enum HAL_CACHE_ID_T id) {
  uint32_t base;

  if (id == HAL_CACHE_ID_I_CACHE) {
    base = ICACHECP_CTRL_BASE;
  } else if (id == HAL_CACHE_ID_D_CACHE) {
#ifdef DCACHECP_CTRL_BASE
    base = DCACHECP_CTRL_BASE;
#else
    base = 0;
#endif
  } else {
    base = 0;
  }

  return base;
}
uint8_t hal_cachecp_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;
  uint32_t main_cache_reg_base;
  enum HAL_CMU_MOD_ID_T mod;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  if (id == HAL_CACHE_ID_D_CACHE) {
    mod = HAL_CMU_H_DCACHECP;
  } else {
    mod = HAL_CMU_H_ICACHECP;
  }
  hal_cmu_clock_enable(mod);
  hal_cmu_reset_clear(mod);

  cacheip_init_cache(reg_base);
  cacheip_enable_cache(reg_base, HAL_CACHE_YES);
  // Init wrap option
  main_cache_reg_base = _cache_get_reg_base(id);
  if (main_cache_reg_base == 0) {
    return 0;
  }
  cacheip_enable_wrap(reg_base, cacheip_wrap_enabled(main_cache_reg_base));

  return 0;
}
uint8_t CP_TEXT_SRAM_LOC hal_cachecp_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;
  enum HAL_CMU_MOD_ID_T mod;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
  uint32_t val;

  do {
    val = cacheip_read32(reg_base, STATUS_REG_OFFSET);
  } while (val & STATUS_FETCHING);
#endif

  cacheip_enable_cache(reg_base, HAL_CACHE_NO);

  if (id == HAL_CACHE_ID_D_CACHE) {
    mod = HAL_CMU_H_DCACHECP;
  } else {
    mod = HAL_CMU_H_ICACHECP;
  }
  hal_cmu_reset_set(mod);
  hal_cmu_clock_disable(mod);

  return 0;
}
uint8_t hal_cachecp_writebuffer_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writebuffer(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t hal_cachecp_writebuffer_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writebuffer(reg_base, HAL_CACHE_NO);

  return 0;
}
uint8_t hal_cachecp_writebuffer_flush(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_flush_writebuffer(reg_base);

  return 0;
}
uint8_t hal_cachecp_writeback_enable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writeback(reg_base, HAL_CACHE_YES);

  return 0;
}
uint8_t hal_cachecp_writeback_disable(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_enable_writeback(reg_base, HAL_CACHE_NO);

  return 0;
}
uint8_t CP_TEXT_SRAM_LOC hal_cachecp_invalidate(enum HAL_CACHE_ID_T id,
                                                uint32_t start_address,
                                                uint32_t len) {
  uint32_t reg_base;
  uint32_t end_address;

#ifndef DCACHECP_CTRL_BASE
  if (id == HAL_CACHE_ID_D_CACHE) {
    id = HAL_CACHE_ID_I_CACHE;
  }
#endif

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  if (len >= CACHE_SIZE / 2) {
    cacheip_init_cache(reg_base);
    cacheip_init_cache(reg_base);
  } else {
    end_address = start_address + len;
    start_address &= (~(CACHE_LINE_SIZE - 1));
    while (start_address < end_address) {
      cacheip_set_invalidate_address(reg_base, start_address);
      cacheip_trigger_invalidate(reg_base);
      cacheip_trigger_invalidate(reg_base);
      start_address += CACHE_LINE_SIZE;
    }
  }

  return 0;
}
uint8_t hal_cachecp_sync(enum HAL_CACHE_ID_T id) {
  uint32_t reg_base = 0;

  reg_base = _cachecp_get_reg_base(id);
  if (reg_base == 0) {
    return 0;
  }

  cacheip_trigger_sync(reg_base);

  return 0;
}
#endif
