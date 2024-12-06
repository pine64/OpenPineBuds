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
#include "factory_section.h"
#include "cmsis.h"
#include "crc32.h"
#include "hal_norflash.h"
#include "hal_trace.h"
#include "heap_api.h"
#include "norflash_api.h"
#include "plat_types.h"
#include "pmu.h"
#include "string.h"
#include "tgt_hardware.h"

extern uint32_t __factory_start[];
extern uint32_t __factory_end[];

static factory_section_t *factory_section_p = NULL;
static uint8_t nv_record_dev_rev = nvrec_current_version;

#ifndef BTIF_BD_ADDR_SIZE
#define BTIF_BD_ADDR_SIZE 6
#endif
static void factory_callback(void *param) {
  NORFLASH_API_OPERA_RESULT *opera_result;

  opera_result = (NORFLASH_API_OPERA_RESULT *)param;

  TRACE(5, "%s:type = %d, addr = 0x%x,len = 0x%x,result = %d.", __func__,
        opera_result->type, opera_result->addr, opera_result->len,
        opera_result->result);
}

void factory_section_init(void) {
  enum NORFLASH_API_RET_T result;
  uint32_t sector_size = 0;
  uint32_t block_size = 0;
  uint32_t page_size = 0;

  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);
  result = norflash_api_register(
      NORFLASH_API_MODULE_ID_FACTORY, HAL_NORFLASH_ID_0,
      ((uint32_t)__factory_start) & 0x00FFFFFF,
      (uint32_t)__factory_end - (uint32_t)__factory_start, block_size,
      sector_size, page_size, FACTORY_SECTOR_SIZE, factory_callback);
  ASSERT(result == NORFLASH_API_OK,
         "nv_record_init: module register failed! result = %d.", result);
}

int factory_section_open(void) {
  factory_section_p = (factory_section_t *)__factory_start;

  if (factory_section_p->head.magic != nvrec_dev_magic) {
    factory_section_p = NULL;
    return -1;
  }
  if ((factory_section_p->head.version < nvrec_mini_version) ||
      (factory_section_p->head.version > nvrec_current_version)) {
    factory_section_p = NULL;
    return -1;
  }

  nv_record_dev_rev = factory_section_p->head.version;

  if (1 == nv_record_dev_rev) {
    if (factory_section_p->head.crc !=
        crc32(0, (unsigned char *)(&(factory_section_p->head.reserved0)),
              sizeof(factory_section_t) - 2 - 2 - 4 -
                  (5 + 63 + 2 + 2 + 2 + 1 + 8) * sizeof(int))) {
      factory_section_p = NULL;
      return -1;
    }

    memcpy(bt_addr, factory_section_p->data.bt_address, BTIF_BD_ADDR_SIZE);
    memcpy(ble_addr, factory_section_p->data.ble_address, BTIF_BD_ADDR_SIZE);
    TRACE(2, "%s sucess btname:%s", __func__,
          factory_section_p->data.device_name);
  } else {
    // check the data length
    if (((uint32_t)(&((factory_section_t *)0)->data.rev2_reserved0) +
         factory_section_p->data.rev2_data_len) > 4096) {
      TRACE(1, "nv rec dev data len %d has exceeds the facory sector size!.",
            factory_section_p->data.rev2_data_len);
      return -1;
    }

    if (factory_section_p->data.rev2_crc !=
        crc32(0, (unsigned char *)(&(factory_section_p->data.rev2_reserved0)),
              factory_section_p->data.rev2_data_len)) {
      factory_section_p = NULL;
      return -1;
    }

    memcpy(bt_addr, factory_section_p->data.rev2_bt_addr, BTIF_BD_ADDR_SIZE);
    memcpy(ble_addr, factory_section_p->data.rev2_ble_addr, BTIF_BD_ADDR_SIZE);
    TRACE(2, "%s sucess btname:%s", __func__,
          (char *)factory_section_p->data.rev2_bt_name);
  }

  DUMP8("%02x ", bt_addr, 6);
  DUMP8("%02x ", ble_addr, 6);
  return 0;
}

uint8_t *factory_section_get_bt_address(void) {
  if (factory_section_p) {
    if (1 == nv_record_dev_rev) {
      return (uint8_t *)&(factory_section_p->data.bt_address);
    } else {
      return (uint8_t *)&(factory_section_p->data.rev2_bt_addr);
    }
  } else {
    return NULL;
  }
}

uint8_t *factory_section_get_bt_name(void) {
  if (factory_section_p) {
    if (1 == nv_record_dev_rev) {
      return (uint8_t *)&(factory_section_p->data.device_name);
    } else {
      return (uint8_t *)&(factory_section_p->data.rev2_bt_name);
    }
  } else {
    return (uint8_t *)BT_LOCAL_NAME;
  }
}

uint8_t *factory_section_get_ble_name(void) {
  if (factory_section_p) {
    if (1 == nv_record_dev_rev) {
      return (uint8_t *)BLE_DEFAULT_NAME;
    } else {
      return (uint8_t *)&(factory_section_p->data.rev2_ble_name);
    }
  } else {
    return (uint8_t *)BLE_DEFAULT_NAME;
  }
}

uint32_t factory_section_get_version(void) {
  if (factory_section_p) {
    return nv_record_dev_rev;
  }

  return 0;
}

int factory_section_xtal_fcap_get(unsigned int *xtal_fcap) {
  if (factory_section_p) {
    if (1 == nv_record_dev_rev) {
      *xtal_fcap = factory_section_p->data.xtal_fcap;
    } else {
      *xtal_fcap = factory_section_p->data.rev2_xtal_fcap;
    }
    return 0;
  } else {
    return -1;
  }
}

int factory_section_xtal_fcap_set(unsigned int xtal_fcap) {
  uint8_t *mempool = NULL;
  uint32_t lock;
  enum NORFLASH_API_RET_T ret;

  if (factory_section_p) {
    TRACE(1, "factory_section_xtal_fcap_set:%d", xtal_fcap);
    syspool_init();
    syspool_get_buff((uint8_t **)&mempool, 0x1000);
    memcpy(mempool, factory_section_p, 0x1000);
    if (1 == nv_record_dev_rev) {
      ((factory_section_t *)mempool)->data.xtal_fcap = xtal_fcap;
      ((factory_section_t *)mempool)->head.crc = crc32(
          0,
          (unsigned char *)(&(((factory_section_t *)mempool)->head.reserved0)),
          sizeof(factory_section_t) - 2 - 2 - 4);
    } else {
      ((factory_section_t *)mempool)->data.rev2_xtal_fcap = xtal_fcap;
      ((factory_section_t *)mempool)->data.rev2_crc =
          crc32(0, (unsigned char *)(&(factory_section_p->data.rev2_reserved0)),
                factory_section_p->data.rev2_data_len);
    }
    lock = int_lock_global();

    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_FACTORY,
                             (uint32_t)(__factory_start)&0x00FFFFFF, 0x1000,
                             false);
    ASSERT(ret == NORFLASH_API_OK,
           "factory_section_xtal_fcap_set: erase failed! ret = %d.", ret);
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_FACTORY,
                             (uint32_t)(__factory_start)&0x00FFFFFF,
                             (uint8_t *)mempool, 0x1000, false);
    ASSERT(ret == NORFLASH_API_OK,
           "factory_section_xtal_fcap_set: write failed! ret = %d.", ret);

    int_unlock_global(lock);

    return 0;
  } else {
    return -1;
  }
}

void factory_section_original_btaddr_get(uint8_t *btAddr) {
  if (factory_section_p) {
    TRACE(0, "get factory_section_p");
    if (1 == nv_record_dev_rev) {
      memcpy(btAddr, factory_section_p->data.bt_address, BTIF_BD_ADDR_SIZE);
    } else {
      memcpy(btAddr, factory_section_p->data.rev2_bt_addr, BTIF_BD_ADDR_SIZE);
    }
  } else {
    TRACE(0, "get bt_addr");
    memcpy(btAddr, bt_addr, BTIF_BD_ADDR_SIZE);
  }
}
