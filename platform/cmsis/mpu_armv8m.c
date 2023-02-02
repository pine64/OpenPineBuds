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
#ifndef __ARM_ARCH_ISA_ARM

#include "cmsis.h"
#include "hal_trace.h"
#include "mpu.h"

#define NORM_MEM_WT_RA_ATTR ARM_MPU_ATTR_MEMORY_(1, 0, 1, 0)
#define NORM_MEM_WB_WA_ATTR ARM_MPU_ATTR_MEMORY_(1, 1, 1, 1)
#define DEV_MEM_ATTR_OUTER ARM_MPU_ATTR_MEMORY_(0, 0, 0, 0)

static void init_mair_attr(void) {
  ARM_MPU_SetMemAttr(MAIR_ATTR_FLASH,
                     ARM_MPU_ATTR(NORM_MEM_WT_RA_ATTR, NORM_MEM_WT_RA_ATTR));
  ARM_MPU_SetMemAttr(MAIR_ATTR_INT_SRAM,
                     ARM_MPU_ATTR(NORM_MEM_WT_RA_ATTR, NORM_MEM_WT_RA_ATTR));
  ARM_MPU_SetMemAttr(MAIR_ATTR_EXT_SRAM,
                     ARM_MPU_ATTR(NORM_MEM_WB_WA_ATTR, NORM_MEM_WB_WA_ATTR));
  ARM_MPU_SetMemAttr(
      MAIR_ATTR_DEVICE,
      ARM_MPU_ATTR(DEV_MEM_ATTR_OUTER, ARM_MPU_ATTR_DEVICE_nGnRnE));
}

static int mpu_enable(void) {
  int flags = 0;

  if (get_cpu_id()) {
    /*
     * if cpu is CP, we will use different mpu maps, that is, every map needed
     * by CP will be mapped, and the memory is not mapped will issue abort when
     * access
     */
    flags = MPU_CTRL_HFNMIENA_Msk;
  } else {
    /*
     * mpu maps use default map designed by arm, that is, if the memory is not
     * mapped, the mpu attibutes will use the default setting by ARM.
     */
    flags = MPU_CTRL_PRIVDEFENA_Msk;
  }
  ARM_MPU_Enable(flags);
  __DSB();
  __ISB();
  return 0;
}

static int mpu_disable(void) {
  __DMB();
  ARM_MPU_Disable();
  return 0;
}

int mpu_open(void) {
  int i;

  if ((MPU->TYPE & MPU_TYPE_DREGION_Msk) == 0) {
    return 1;
  }

  for (i = 0; i < MPU_ID_QTY; i++) {
    ARM_MPU_ClrRegion(i);
  }

  init_mair_attr();

  return 0;
}

int mpu_close(void) {
  ARM_MPU_Disable();

  return 0;
}

static int mpu_set_armv8(enum MPU_ID_T id, uint32_t addr, uint32_t len,
                         enum MPU_ATTR_T ap_attr,
                         enum MAIR_ATTR_TYPE_T mem_attr) {
  uint32_t rbar;
  uint32_t rlar;
  uint8_t xn;
  uint8_t ro;
  uint32_t lock;

  if (id >= MPU_ID_QTY) {
    return 2;
  }
  if (len < 32) {
    return 3;
  }
  if (addr & 0x1F) {
    return 4;
  }
  if (ap_attr >= MPU_ATTR_QTY) {
    return 5;
  }

  if (ap_attr == MPU_ATTR_READ_WRITE_EXEC || ap_attr == MPU_ATTR_READ_EXEC ||
      ap_attr == MPU_ATTR_EXEC) {
    xn = 0;
  } else {
    xn = 1;
  }

  if (ap_attr == MPU_ATTR_READ_WRITE_EXEC || ap_attr == MPU_ATTR_READ_WRITE) {
    ro = 0;
  } else if (ap_attr == MPU_ATTR_READ_EXEC || ap_attr == MPU_ATTR_READ ||
             ap_attr == MPU_ATTR_EXEC) {
    ro = 1;
  } else {
    // Cannot support no access
    return 6;
  }

  // Sharebility: Outer Shareable
  // Non-privilege Access: Enabled
  rbar = ARM_MPU_RBAR(addr, 2, ro, 1, xn);
  /* rlar = ARM_MPU_RLAR((addr + len - 1), MAIR_ATTR_INT_SRAM); */
  rlar = ARM_MPU_RLAR((addr + len - 1), mem_attr);

  lock = int_lock();

  ARM_MPU_SetRegion(id, rbar, rlar);
  int_unlock(lock);

  return 0;
}

static inline int mpu_fram_protect_armv8(uint32_t fr_start, uint32_t fr_end) {
  uint32_t len = fr_end - fr_start;
  int ret;

  if ((fr_start % 32 != 0) || (len % 32 != 0))
    ASSERT(0, "fr_start %x and len %d must be aligned to 32", fr_start, len);

  mpu_disable();
  ret = mpu_set_armv8(MPU_ID_FRAM_TEXT1, fr_start, len, MPU_ATTR_EXEC,
                      MAIR_ATTR_INT_SRAM);
  mpu_enable();

  return ret;
}

static int mpu_code_region_protect(void) {
  uint32_t code_start = RAMX_BASE;
  uint32_t len = RAM_SIZE;

  return mpu_set(MPU_ID_CODE, code_start, len, 0, MPU_ATTR_EXEC);
}

extern uint32_t __sram_text_data_start__[];
extern uint32_t __sram_text_end__[];
static int mpu_sram_text_protect(void) {
  int ret;
  uint32_t start = (uint32_t)__sram_text_data_start__;
  uint32_t end = RAMX_TO_RAM((uint32_t)__sram_text_end__);
  uint32_t len = end - start;

  TRACE(1, "sram start %x size %x", start, len);
  if ((start % 32 != 0) || (len % 32 != 0))
    ASSERT(0, "sram start %x and len %d must be aligned to 32", start, len);

  ret = mpu_set(MPU_ID_SRAM_TEXT, start, len, 0, MPU_ATTR_READ_EXEC);
  return ret;
}

int mpu_set(enum MPU_ID_T id, uint32_t addr, uint32_t len, int srd_bits,
            enum MPU_ATTR_T attr) {
  int ret = -1;
  uint32_t lock;

  lock = int_lock();

  mpu_disable();
  ret = mpu_set_armv8(id, addr, len, attr, MAIR_ATTR_INT_SRAM);
  mpu_enable();

  int_unlock(lock);

  return ret;
}

int mpu_clear(enum MPU_ID_T id) {
  uint32_t lock;

  if (id >= MPU_ID_QTY) {
    return 2;
  }

  lock = int_lock();

  mpu_disable();
  ARM_MPU_ClrRegion(id);
  __DSB();
  __ISB();
  mpu_enable();

  int_unlock(lock);

  return 0;
}

static int mpu_null_check_enable(void) {
  return mpu_set(MPU_ID_NULL_POINTER, 0, 0x800, 0, MPU_ATTR_EXEC);
}

extern uint32_t __fast_sram_text_exec_start__[];
extern uint32_t __fast_sram_text_exec_end__[];

static int mpu_fast_ram_protect(void) {
  uint32_t ramx_start = (uint32_t)__fast_sram_text_exec_start__;
  uint32_t ramx_end = (uint32_t)__fast_sram_text_exec_end__;
  uint32_t ram_start = RAMX_TO_RAM(ramx_start);
  uint32_t ram_end = RAMX_TO_RAM(ramx_end);
  int ret = -1;

  ret = mpu_fram_protect_armv8(ram_start, ram_end);
  return ret;
}

int mpu_setup(void) {
  mpu_null_check_enable();

  mpu_code_region_protect();
  mpu_sram_text_protect();

  mpu_fast_ram_protect();
  return 0;
}

int mpu_setup_cp(const mpu_regions_t *mpu_table, uint32_t region_num) {
  int ret;
  int i;
  uint32_t lock;

  ret = mpu_open();
  if (ret) {
    return ret;
  }

  if (region_num > MPU_ID_QTY) {
    return -1;
  }

  lock = int_lock();
  mpu_disable();
  for (i = 0; i < region_num; i++) {
    const mpu_regions_t *region;
    region = &mpu_table[i];
    ret = mpu_set_armv8(i, region->addr, region->len, region->ap_attr,
                        region->mem_attr);
    if (ret)
      break;
  }

  mpu_enable();
  int_unlock(lock);

  return ret;
}

#endif
