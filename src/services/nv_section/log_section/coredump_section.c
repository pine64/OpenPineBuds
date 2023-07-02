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
#include <stdio.h>
#include <string.h>

#include "cmsis.h"
#include "coredump_section.h"
#include "hal_norflash.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "norflash_api.h"

/*
Usage:
arm-none-eabi-gdb elf_file -ex "target remote | ./tools/bins/lin64/CrashDebug
--elf elf_file --dump coredump.dat"
*/

extern uint32_t __coredump_section_start;
extern uint32_t __coredump_section_end;

static const uint32_t coredump_flash_start_addr =
    (uint32_t)&__coredump_section_start;
static const uint32_t coredump_flash_end_addr =
    (uint32_t)&__coredump_section_end;
static uint32_t coredump_flash_addr_offset =
    (uint32_t)&__coredump_section_start;

void coredump_to_flash_init() {
  uint32_t block_size = 0;
  uint32_t sector_size = 0;
  uint32_t page_size = 0;
  uint32_t buffer_len = 0;
  enum NORFLASH_API_RET_T result;

  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);
  buffer_len = COREDUMP_SECTOR_SIZE;
  TRACE(4,
        "%s: coredump_flash_start_addr = 0x%x, coredump_flash_end_addr = 0x%x, "
        "buff_len = 0x%x.",
        __func__, coredump_flash_start_addr, coredump_flash_end_addr,
        buffer_len);

  result = norflash_api_register(
      NORFLASH_API_MODULE_ID_COREDUMP, HAL_NORFLASH_ID_0,
      coredump_flash_start_addr,
      coredump_flash_end_addr - coredump_flash_start_addr, block_size,
      sector_size, page_size, buffer_len, NULL);

  if (result == NORFLASH_API_OK) {
    TRACE(0, "coredump_to_flash_init ok.");
  } else {
    TRACE(1, "coredump_to_flash_init failed,result = %d.", result);
  }
}

void core_dump_erase_section() {
  // erase whole core dump section
  enum NORFLASH_API_RET_T ret = 0;
  uint32_t lock;
  do {
    lock = int_lock_global();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_COREDUMP,
                             coredump_flash_addr_offset, COREDUMP_SECTOR_SIZE,
                             false);
    int_unlock_global(lock);
    if (ret != NORFLASH_API_OK) {
      TRACE(3, "%s:offset = 0x%x,ret = %d.", __func__,
            coredump_flash_addr_offset, ret);
    }
    coredump_flash_addr_offset += COREDUMP_SECTOR_SIZE;
    if (coredump_flash_addr_offset == coredump_flash_end_addr) {
      coredump_flash_addr_offset = coredump_flash_start_addr;
      break;
    }
  } while (1);
}

int32_t core_dump_write_large(const uint8_t *ptr, uint32_t len) {
  enum NORFLASH_API_RET_T ret;
  uint32_t write_len;
  uint32_t remain_len = len;
  uint32_t lock;
  // TRACE(4,"%s offset = 0x%x ptr = 0x%x len=0x%x",__func__,
  // coredump_flash_addr_offset, ptr, len);
  do {
    if (coredump_flash_addr_offset % COREDUMP_SECTOR_SIZE != 0) {
      write_len = COREDUMP_SECTOR_SIZE -
                  (coredump_flash_addr_offset -
                   (coredump_flash_addr_offset & ~COREDUMP_SECTOR_SIZE_MASK));
      lock = int_lock_global();
      ret =
          norflash_api_write(NORFLASH_API_MODULE_ID_COREDUMP,
                             coredump_flash_addr_offset, ptr, write_len, false);
      int_unlock_global(lock);
      if (ret != NORFLASH_API_OK) {
        TRACE(4, "%s: addr = 0x%x,write_len = 0x%x,ret = %d.", __func__,
              coredump_flash_addr_offset, write_len, ret);
        return (int32_t)ret;
      }
      coredump_flash_addr_offset += write_len;
      ptr += write_len;
      remain_len -= write_len;
    }

    if (remain_len < COREDUMP_SECTOR_SIZE) {
      lock = int_lock_global();
      ret = norflash_api_write(NORFLASH_API_MODULE_ID_COREDUMP,
                               coredump_flash_addr_offset, ptr, remain_len,
                               false);
      int_unlock_global(lock);
      if (ret != NORFLASH_API_OK) {
        TRACE(4, "%s: addr = 0x%x,remain_len = 0x%x,ret = %d.", __func__,
              coredump_flash_addr_offset, remain_len, ret);
        return (int32_t)ret;
      }
      coredump_flash_addr_offset += remain_len;
      ptr += remain_len;
      remain_len = 0;
    } else {
      lock = int_lock_global();
      ret = norflash_api_write(NORFLASH_API_MODULE_ID_COREDUMP,
                               coredump_flash_addr_offset, ptr,
                               COREDUMP_SECTOR_SIZE, false);
      int_unlock_global(lock);
      if (ret != NORFLASH_API_OK) {
        TRACE(3, "%s: addr = 0x%x,ret = %d.", __func__,
              coredump_flash_addr_offset, ret);
        return (int32_t)ret;
      }
      coredump_flash_addr_offset += COREDUMP_SECTOR_SIZE;
      ptr += COREDUMP_SECTOR_SIZE;
      remain_len -= COREDUMP_SECTOR_SIZE;
    }
  } while (remain_len > 0);
  // TRACE(3,"%s offset = 0x%x ptr = 0x%x",__func__, coredump_flash_addr_offset,
  // ptr);
  return 0;
}

int32_t core_dump_write(const uint8_t *ptr, uint32_t len) {
  enum NORFLASH_API_RET_T ret;
  uint32_t i;
  uint32_t lock;
  // TRACE(2,"%s: len = 0x%x", __func__,len);
  // write by byte to avoid cross flash sector boundary
  for (i = 0; i < len; i++) {
    lock = int_lock_global();
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_COREDUMP,
                             coredump_flash_addr_offset++, ptr++, 1, false);
    int_unlock_global(lock);
    if (ret != NORFLASH_API_OK) {
      TRACE(4, "%s: offset = 0x%x ret = %d ch =%02x", __func__,
            coredump_flash_addr_offset, ret, *(ptr - 1));
    }
  }
  return 0;
}
