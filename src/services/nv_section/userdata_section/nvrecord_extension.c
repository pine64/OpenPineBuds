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
#ifdef NEW_NV_RECORD_ENABLED
#include "nvrecord_extension.h"
#include "besbt.h"
#include "cmsis.h"
#include "crc32.h"
#include "customparam_section.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "mpu.h"
#include "norflash_api.h"
#include "norflash_drv.h"
#include "nvrecord_ble.h"
#include "nvrecord_bt.h"
#include "nvrecord_dma_config.h"
#include "nvrecord_env.h"
#include "nvrecord_fp_account_key.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

extern uint32_t __userdata_start[];
extern uint32_t __userdata_end[];
extern void nvrecord_rebuild_system_env(struct nvrecord_env_t *pSystemEnv);
extern void nvrecord_rebuild_paired_bt_dev_info(
    NV_RECORD_PAIRED_BT_DEV_INFO_T *pPairedBtInfo);
#ifdef GFPS_ENABLED
extern void
nvrecord_rebuild_fp_account_key(NV_FP_ACCOUNT_KEY_RECORD_T *pFpAccountKey);
#endif
#ifdef NVREC_BAIDU_DATA_SECTION
extern void
nvrecord_rebuild_dma_configuration(NV_DMA_CONFIGURATION_T *pDmaConfig);
#endif

typedef enum {
  NV_STATE_IDLE,
  NV_STATE_MAIN_ERASING,
  NV_STATE_MAIN_ERASED,
  NV_STATE_MAIN_WRITTING,
  NV_STATE_MAIN_WRITTEN,
  NV_STATE_MAIN_DONE,
  NV_STATE_BAK_ERASING,
  NV_STATE_BAK_ERASED,
  NV_STATE_BAK_WRITTING,
  NV_STATE_BAK_WRITTEN,
  NV_STATE_BAK_DONE,
} NV_STATE;

typedef struct {
  bool is_update;
  NV_STATE state;
  uint32_t written_size;
} NV_FLUSH_STATE;

static NV_FLUSH_STATE nv_flsh_state;
static bool nvrec_init = false;
static uint32_t _user_data_main_start;
static uint32_t _user_data_bak_start;
static uint8_t _nv_burn_buf[NV_EXTENSION_PAGE_SIZE];

NV_EXTENSION_RECORD_T *nvrecord_extension_p = NULL;

/*
 *Note: the NV_EXTENSION_MIRROR_RAM_SIZE must be power of 2
 */
#if defined(__ARM_ARCH_8M_MAIN__)
#define __NV_BUF_MPU_ALIGNED __ALIGNED(0x20)
#else
/*
 * armv7 mpu require the address must be aligned to the section size and
 * the section size must be algined to power of 2
 */
#define __NV_BUF_MPU_ALIGNED __ALIGNED(NV_EXTENSION_MIRROR_RAM_SIZE)
#endif

static NV_MIRROR_BUF_T local_extension_data __NV_BUF_MPU_ALIGNED
    __attribute__((section(".sram_data"))) = {
        .nv_record =
            {
                {
                    // header
                    NV_EXTENSION_MAGIC_NUMBER,
                    NV_EXTENSION_MAJOR_VERSION,
                    NV_EXTENSION_MINOR_VERSION,
                    NV_EXTENSION_VALID_LEN,
                    0,
                },

                {
                    // system info
                },

                {// bt_pair_info
                 0},

                {
                    // ble_pair_info

                },

#ifdef TWS_SYSTEM_ENABLED
                {
                    // tws_info

                },
#endif

#ifdef GFPS_ENABLED
                {// fp_account_key_rec
                 0},
#endif

#ifdef NVREC_BAIDU_DATA_SECTION
                {
                    // dma_config
                    BAIDU_DATA_DEF_FM_FREQ,
                },
#endif

#ifdef TILE_DATAPATH
                {{0}},
#endif

#ifdef OTA_ENABLED
                {
                    {
                        0,
                    },
                },
#endif

#if defined(BISTO_ENABLED)
                {
                    true,
                },
#endif

#if 1 // def TX_IQ_CAL
                {
                    BT_IQ_INVALID_MAGIC_NUM,
                    {0},
                    {0},
                },
#endif
                // TODO:
                // If want to extend the nvrecord while keeping the history
                // information,
                // append the new items to the tail of NV_EXTENSION_RECORD_T and
                // set their intial content here
            },
};

STATIC_ASSERT(sizeof(local_extension_data) <= NV_EXTENSION_MIRROR_RAM_SIZE,
              "NV local buffer too small");

static int nv_record_extension_flush(bool is_async);

#ifdef TILE_DATAPATH
static void nvrecord_rebuild_tileconfig(NV_TILE_INFO_CONFIG_T *tileConfig) {
  memset((uint8_t *)tileConfig, 0, sizeof(NV_TILE_INFO_CONFIG_T));
}
#endif

#ifdef OTA_ENABLED
static void nvrecord_rebuild_ota_info(NV_OTA_INFO_T *ota_info) {
  memset((uint8_t *)ota_info, 0, OTA_DEVICE_CNT * sizeof(NV_OTA_INFO_T));
}
#endif

#if defined(BISTO_ENABLED)
static void nvrecord_rebuild_gsound_info(NV_GSOUND_INFO_T *gsound_info) {
  memset((uint8_t *)gsound_info, 0, sizeof(NV_GSOUND_INFO_T));
}
#endif

#if 1 // def TX_IQ_CAL
static void
nvrecord_rebuild_btiqcalconfig(BT_IQ_CALIBRATION_CONFIG_T *btIqCalConfig) {
  memset((uint8_t *)btIqCalConfig, 0, sizeof(BT_IQ_CALIBRATION_CONFIG_T));
  btIqCalConfig->validityMagicNum = BT_IQ_INVALID_MAGIC_NUM;
}
#endif

static bool nv_record_data_is_valid(NV_EXTENSION_RECORD_T *nv_record) {
  bool is_valid = false;

  NVRECORD_HEADER_T *pExtRecInFlash = &nv_record->header;
  uint8_t *pData = (uint8_t *)nv_record + NV_EXTENSION_HEADER_SIZE;

  TRACE(0, "nv ext magic 0x%x valid len %d", pExtRecInFlash->magicNumber,
        pExtRecInFlash->validLen);

  if ((NV_EXTENSION_MAJOR_VERSION == pExtRecInFlash->majorVersion) &&
      (NV_EXTENSION_MAGIC_NUMBER == pExtRecInFlash->magicNumber)) {
    // check whether the data length is valid
    if (pExtRecInFlash->validLen <=
        NV_EXTENSION_SIZE - NV_EXTENSION_HEADER_SIZE) {
      // check crc32
      uint32_t crc = crc32(0, pData, pExtRecInFlash->validLen);
      TRACE(1, "generated crc32: 0x%x, header crc: 0x%x", crc,
            pExtRecInFlash->crc32);
      if (crc == pExtRecInFlash->crc32) {
        // correct
        TRACE(2, "Nv extension is valid.");

        TRACE(2, "Former nv ext valid len %d", pExtRecInFlash->validLen);
        TRACE(2, "Current FW version nv ext valid len %d",
              NV_EXTENSION_VALID_LEN);

        if (NV_EXTENSION_VALID_LEN < pExtRecInFlash->validLen) {
          TRACE(0, "Valid length of extension must be increased,"
                   "use the default value.");
        } else {
          if (NV_EXTENSION_VALID_LEN > pExtRecInFlash->validLen) {
            TRACE(2, "NV extension is extended! (0x%x) -> (0x%x)",
                  pExtRecInFlash->validLen, NV_EXTENSION_VALID_LEN);
          }
          is_valid = true;
        }
      }
    }
  }

  return is_valid;
}

static bool nv_record_items_is_valid(NV_EXTENSION_RECORD_T *nv_record) {
  nv_record = nv_record;
#if 0
    NV_RECORD_PAIRED_BT_DEV_INFO_T *pbt_pair_info;
    pbt_pair_info = &nv_record->bt_pair_info;
    TRACE(1,"%s: pairedDevNum: %d ", __func__,pbt_pair_info->pairedDevNum);
    for(uint8_t i = 0; i < pbt_pair_info->pairedDevNum; i++)
    {
        DUMP8("0x%x,", (uint8_t*)pbt_pair_info->pairedBtDevInfo[i].record.bdAddr.address,6);
    }
#endif
  // TODO:  add cheking for nv_record items.

  return true;
}

static void nv_record_set_default(NV_EXTENSION_RECORD_T *nv_record) {
  memset((uint8_t *)nv_record, 0, sizeof(NV_EXTENSION_RECORD_T));
  nvrecord_rebuild_system_env(&(nv_record->system_info));
  nvrecord_rebuild_paired_bt_dev_info(&(nv_record->bt_pair_info));
  nvrecord_rebuild_paired_ble_dev_info(&(nv_record->ble_pair_info));
#ifdef GFPS_ENABLED
  nvrecord_rebuild_fp_account_key(&(nv_record->fp_account_key_rec));
#endif

#ifdef NVREC_BAIDU_DATA_SECTION
  nvrecord_rebuild_dma_configuration(&(nv_record->dma_config));
#endif

#ifdef TILE_DATAPATH
  nvrecord_rebuild_tileconfig(&nv_record->tileConfig);
#endif

#if defined(BISTO_ENABLED)
  nvrecord_rebuild_gsound_info(&nv_record->gsound_info);
#endif

#ifdef OTA_ENABLED
  nvrecord_rebuild_ota_info((NV_OTA_INFO_T *)&nv_record->ota_info);
#endif

#if 1 // def TX_IQ_CAL
  nvrecord_rebuild_btiqcalconfig(&nv_record->btIqCalConfig);
#endif

  nv_record->header.magicNumber = NV_EXTENSION_MAGIC_NUMBER;
  nv_record->header.majorVersion = NV_EXTENSION_MAJOR_VERSION;
  nv_record->header.minorVersion = NV_EXTENSION_MINOR_VERSION;
  nv_record->header.validLen = NV_EXTENSION_VALID_LEN;
  nv_record->header.crc32 =
      crc32(0, ((uint8_t *)nv_record + NV_EXTENSION_HEADER_SIZE),
            NV_EXTENSION_VALID_LEN);
}

static void _nv_record_extension_init(void) {
  enum NORFLASH_API_RET_T result;
  uint32_t sector_size = 0;
  uint32_t block_size = 0;
  uint32_t page_size = 0;

  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);
  result = norflash_api_register(
      NORFLASH_API_MODULE_ID_USERDATA_EXT, HAL_NORFLASH_ID_0,
      ((uint32_t)__userdata_start),
      ((uint32_t)__userdata_end - (uint32_t)__userdata_start), block_size,
      sector_size, page_size, NV_EXTENSION_SIZE * 2, nv_extension_callback);
  ASSERT(result == NORFLASH_API_OK,
         "_nv_record_extension_init: module register failed! result = %d.",
         result);
}

uint32_t nv_record_pre_write_operation(void) {
  uint32_t lock = int_lock_global();
  mpu_clear(MPU_ID_USER_DATA_SECTION);
  return lock;
}

void nv_record_post_write_operation(uint32_t lock) {
  int ret = 0;
  uint32_t nv_start = (uint32_t)&local_extension_data.nv_record;
  uint32_t len = NV_EXTENSION_MIRROR_RAM_SIZE;

  ret = mpu_set(MPU_ID_USER_DATA_SECTION, nv_start, len, 0, MPU_ATTR_READ);
  int_unlock_global(lock);
  TRACE(2, "set mpu 0x%x len %d result %d", nv_start, len, ret);
}

static void nv_record_extension_init(void) {
  uint32_t lock;
  bool main_is_valid = false;
  bool bak_is_valid = false;
  bool data_is_valid = false;

  if (nvrec_init) {
    return;
  }
  _user_data_main_start = (uint32_t)__userdata_start;
  _user_data_bak_start = (uint32_t)__userdata_start + NV_EXTENSION_SIZE;

  lock = nv_record_pre_write_operation();
  _nv_record_extension_init();

  nv_flsh_state.is_update = false;
  nv_flsh_state.written_size = 0;
  nv_flsh_state.state = NV_STATE_IDLE;

  nvrecord_extension_p = &local_extension_data.nv_record;

  if (nvrecord_extension_p->header.magicNumber != NV_EXTENSION_MAGIC_NUMBER ||
      nvrecord_extension_p->header.majorVersion != NV_EXTENSION_MAJOR_VERSION ||
      nvrecord_extension_p->header.minorVersion != NV_EXTENSION_MINOR_VERSION ||
      nvrecord_extension_p->header.validLen >=
          (NV_EXTENSION_SIZE - NV_EXTENSION_HEADER_SIZE)) {
    ASSERT(0, "%s: local_extension_data error!(0x%x,0x%x,0x%x,0x%x)", __func__,
           nvrecord_extension_p->header.magicNumber,
           nvrecord_extension_p->header.majorVersion,
           nvrecord_extension_p->header.minorVersion,
           nvrecord_extension_p->header.validLen);
  }

  // Check main sector.
  if (nv_record_data_is_valid((NV_EXTENSION_RECORD_T *)_user_data_main_start) &&
      nv_record_items_is_valid(
          (NV_EXTENSION_RECORD_T *)_user_data_main_start)) {
    TRACE(2, "%s,main sector is valid.", __func__);
    main_is_valid = true;
  } else {
    TRACE(1, "%s,main sector is invalid!", __func__);
    main_is_valid = false;
  }

  // Check bak secotr.
  if (nv_record_data_is_valid((NV_EXTENSION_RECORD_T *)_user_data_bak_start) &&
      nv_record_items_is_valid(
          (NV_EXTENSION_RECORD_T *)_user_data_main_start)) {
    TRACE(2, "%s,bak sector is valid.", __func__);
    bak_is_valid = true;
  } else {
    TRACE(1, "%s,bak sector is invalid!", __func__);
    bak_is_valid = false;
  }

  if (main_is_valid) {
    data_is_valid = true;
    if (!bak_is_valid) {
      nv_flsh_state.is_update = true;
      nv_flsh_state.state = NV_STATE_MAIN_DONE;
    }
  } else {
    if (bak_is_valid) {
      data_is_valid = true;
      nv_flsh_state.is_update = true;
      nv_flsh_state.state = NV_STATE_IDLE;
    } else {
      data_is_valid = false;
    }
  }

  if (data_is_valid) {
    TRACE(2, "%s,data is valid.", __func__);
    if (main_is_valid) {
      memcpy((uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
             (uint8_t *)_user_data_main_start + NV_EXTENSION_HEADER_SIZE,
             NV_EXTENSION_VALID_LEN);
    } else {
      memcpy((uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
             (uint8_t *)_user_data_bak_start + NV_EXTENSION_HEADER_SIZE,
             NV_EXTENSION_VALID_LEN);
    }
    nvrecord_extension_p->header.crc32 =
        crc32(0, ((uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE),
              nvrecord_extension_p->header.validLen);
  } else {
    TRACE(1, "%s,data invalid, rebuild... ", __func__);
    nv_record_set_default(nvrecord_extension_p);
    TRACE(2, "%s,rebuild crc = 0x%x. ", __func__,
          nvrecord_extension_p->header.crc32);
    // need to update the content in the flash
    nv_record_extension_update();
  }

  nvrec_init = true;

  nv_record_post_write_operation(lock);

  if (nv_flsh_state.is_update) {
    nv_record_extension_flush(false);
  }
  TRACE(2, "%s,done.", __func__);
}

NV_EXTENSION_RECORD_T *nv_record_get_extension_entry_ptr(void) {
  return nvrecord_extension_p;
}

void nv_record_extension_update(void) { nv_flsh_state.is_update = true; }

static int nv_record_extension_flush_main(bool is_async) {
  uint32_t crc;
  uint32_t lock;
  enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

  if (NULL == nvrecord_extension_p) {
    TRACE(1, "%s,nvrecord_extension_p is null.", __func__);
    goto _func_end;
  }

  // TRACE(3, "%s is_async %d state %d", __func__, is_async,
  // nv_flsh_state.state);

  if (is_async) {
    if (nv_flsh_state.state == NV_STATE_IDLE &&
        nv_flsh_state.is_update == true) {
      TRACE(3, "%s: async flush begin!", __func__);
    }

    hal_trace_pause();
    lock = int_lock_global();

    if (nv_flsh_state.state == NV_STATE_IDLE ||
        nv_flsh_state.state == NV_STATE_MAIN_ERASING) {
      if (nv_flsh_state.state == NV_STATE_IDLE) {
        nv_flsh_state.state = NV_STATE_MAIN_ERASING;
        TRACE(4, "%s: NV_STATE_MAIN_ERASING", __func__);
      }

      ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                               (uint32_t)(__userdata_start), NV_EXTENSION_SIZE,
                               true);
      if (ret == NORFLASH_API_OK) {
        nv_flsh_state.state = NV_STATE_MAIN_ERASED;
        TRACE(4, "%s: NV_STATE_MAIN_ERASED", __func__);
        TRACE(4, "%s: norflash_api_erase ok,addr = 0x%x.", __func__,
              _user_data_main_start);
      } else if (ret == NORFLASH_API_BUFFER_FULL) {
        norflash_api_flush();
      } else {
        ASSERT(0, "%s: norflash_api_erase err,ret = %d,addr = 0x%x.", __func__,
               ret, _user_data_main_start);
      }
    } else if (nv_flsh_state.state == NV_STATE_MAIN_ERASED ||
               nv_flsh_state.state == NV_STATE_MAIN_WRITTING) {
      if (nv_flsh_state.state == NV_STATE_MAIN_ERASED) {
        nv_flsh_state.state = NV_STATE_MAIN_WRITTING;
        TRACE(4, "%s: NV_STATE_MAIN_WRITTING", __func__);
      }
      uint32_t tmpLock = nv_record_pre_write_operation();
      crc = crc32(0, (uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
                  NV_EXTENSION_VALID_LEN);
      nvrecord_extension_p->header.crc32 = crc;
      ASSERT(nv_record_data_is_valid(nvrecord_extension_p) &&
                 nv_record_items_is_valid(nvrecord_extension_p),
             "%s nv_record is invalid!", __func__);
      ret = norflash_api_write(
          NORFLASH_API_MODULE_ID_USERDATA_EXT, (uint32_t)(__userdata_start),
          (uint8_t *)nvrecord_extension_p, NV_EXTENSION_SIZE, true);
      nv_record_post_write_operation(tmpLock);
      if (ret == NORFLASH_API_OK) {
        nv_flsh_state.is_update = false;
        nv_flsh_state.state = NV_STATE_MAIN_WRITTEN;
        TRACE(4, "%s: NV_STATE_MAIN_WRITTEN", __func__);
        TRACE(4, "%s: norflash_api_write ok,addr = 0x%x.", __func__,
              _user_data_main_start);
      } else if (ret == NORFLASH_API_BUFFER_FULL) {
        norflash_api_flush();
      } else {
        ASSERT(0, "%s: norflash_api_write err,ret = %d,addr = 0x%x.", __func__,
               ret, _user_data_main_start);
      }
    } else {
      if (norflash_api_get_used_buffer_count(
              NORFLASH_API_MODULE_ID_USERDATA_EXT, NORFLASH_API_ALL) == 0) {
        nv_flsh_state.state = NV_STATE_MAIN_DONE;
        // TRACE(1,"%s: NV_STATE_MAIN_DONE", __func__);
        TRACE(1, "%s: async flush done.", __func__);
      } else {
        norflash_api_flush();
      }
    }
    int_unlock_global(lock);
    hal_trace_continue();
  } else {
    TRACE(4, "%s: sync flush begin!", __func__);
    if (nv_flsh_state.state == NV_STATE_IDLE ||
        nv_flsh_state.state == NV_STATE_MAIN_ERASING) {
      do {
        hal_trace_pause();
        lock = int_lock_global();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                 (uint32_t)(__userdata_start),
                                 NV_EXTENSION_SIZE, true);
        int_unlock_global(lock);
        hal_trace_continue();
        if (ret == NORFLASH_API_OK) {
          nv_flsh_state.state = NV_STATE_MAIN_ERASED;
          TRACE(4, "%s: norflash_api_erase ok,addr = 0x%x.", __func__,
                _user_data_main_start);
        } else if (ret == NORFLASH_API_BUFFER_FULL) {
          do {
            norflash_api_flush();
          } while (norflash_api_get_free_buffer_count(NORFLASH_API_ERASING) ==
                   0);
        } else {
          ASSERT(0, "%s: norflash_api_erase err,ret = %d,addr = 0x%x.",
                 __func__, ret, _user_data_main_start);
        }

      } while (ret == NORFLASH_API_BUFFER_FULL);
    }

    if (nv_flsh_state.state == NV_STATE_MAIN_ERASED ||
        nv_flsh_state.state == NV_STATE_MAIN_WRITTING) {
      do {
        hal_trace_pause();
        uint32_t tmpLock = nv_record_pre_write_operation();
        crc =
            crc32(0, (uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
                  NV_EXTENSION_VALID_LEN);
        nvrecord_extension_p->header.crc32 = crc;
        ASSERT(nv_record_data_is_valid(nvrecord_extension_p) &&
                   nv_record_items_is_valid(nvrecord_extension_p),
               "%s nv_record is invalid!", __func__);
        ret = norflash_api_write(
            NORFLASH_API_MODULE_ID_USERDATA_EXT, (uint32_t)(__userdata_start),
            (uint8_t *)nvrecord_extension_p, NV_EXTENSION_SIZE, true);
        nv_record_post_write_operation(tmpLock);
        hal_trace_continue();
        if (ret == NORFLASH_API_OK) {
          nv_flsh_state.is_update = false;
          nv_flsh_state.state = NV_STATE_MAIN_WRITTEN;
          TRACE(4, "%s: norflash_api_write ok,addr = 0x%x.", __func__,
                _user_data_main_start);
        } else if (ret == NORFLASH_API_BUFFER_FULL) {
          do {
            norflash_api_flush();
          } while (norflash_api_get_free_buffer_count(NORFLASH_API_WRITTING) ==
                   0);
        } else {
          ASSERT(0, "%s: norflash_api_write err,ret = %d,addr = 0x%x.",
                 __func__, ret, _user_data_main_start);
        }
      } while (ret == NORFLASH_API_BUFFER_FULL);
    }

    do {
      norflash_api_flush();
    } while (norflash_api_get_used_buffer_count(
                 NORFLASH_API_MODULE_ID_USERDATA_EXT, NORFLASH_API_ALL) > 0);

    nv_flsh_state.state = NV_STATE_MAIN_DONE;
    TRACE(1, "%s: sync flush done.", __func__);
  }
_func_end:
  return (ret == NORFLASH_API_OK) ? 0 : 1;
}

static int nv_record_extension_flush_bak(bool is_async) {
  uint32_t lock;
  enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;
  uint8_t *burn_buf = (uint8_t *)_nv_burn_buf;

  if (is_async) {
    hal_trace_pause();
    lock = int_lock_global();
    if (nv_flsh_state.state == NV_STATE_MAIN_DONE ||
        nv_flsh_state.state == NV_STATE_BAK_ERASING) {
      if (nv_flsh_state.state == NV_STATE_MAIN_DONE) {
        nv_flsh_state.state = NV_STATE_BAK_ERASING;
        TRACE(3, "%s: async flush begin", __func__);
      }
      ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                               _user_data_bak_start, NV_EXTENSION_SIZE, true);
      if (ret == NORFLASH_API_OK) {
        nv_flsh_state.state = NV_STATE_BAK_ERASED;
        TRACE(4, "%s: norflash_api_erase ok,addr = 0x%x.", __func__,
              _user_data_bak_start);
        // TRACE(4,"%s: NV_STATE_BAK_ERASED", __func__);
      } else if (ret == NORFLASH_API_BUFFER_FULL) {
        norflash_api_flush();
      } else {
        ASSERT(0, "%s: norflash_api_erase err,ret = %d,addr = 0x%x.", __func__,
               ret, _user_data_bak_start);
      }
    } else if (nv_flsh_state.state == NV_STATE_BAK_ERASED ||
               nv_flsh_state.state == NV_STATE_BAK_WRITTING) {
      if (nv_flsh_state.state == NV_STATE_BAK_ERASED) {
        nv_flsh_state.state = NV_STATE_BAK_WRITTING;
        nv_flsh_state.written_size = 0;
        // TRACE(4,"%s: NV_STATE_BAK_WRITTING", __func__);
      }
      do {
        ret = norflash_api_read(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                _user_data_main_start +
                                    nv_flsh_state.written_size,
                                burn_buf, NV_EXTENSION_PAGE_SIZE);
        ASSERT(ret == NORFLASH_API_OK,
               "norflash_api_read failed! ret = %d, addr = 0x%x.", (int32_t)ret,
               _user_data_main_start + nv_flsh_state.written_size);

        ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                 _user_data_bak_start +
                                     nv_flsh_state.written_size,
                                 burn_buf, NV_EXTENSION_PAGE_SIZE, true);
        if (ret == NORFLASH_API_OK) {
          nv_flsh_state.written_size += NV_EXTENSION_PAGE_SIZE;

          if (nv_flsh_state.written_size == NV_EXTENSION_SIZE) {
            nv_flsh_state.state = NV_STATE_BAK_WRITTEN;
            // TRACE(4,"%s: NV_STATE_BAK_WRITTEN", __func__);
            break;
          }
        } else if (ret == NORFLASH_API_BUFFER_FULL) {
          norflash_api_flush();
          break;
        } else {
          ASSERT(0, "%s: norflash_api_write err,ret = %d,addr = 0x%x.",
                 __func__, ret,
                 _user_data_bak_start + nv_flsh_state.written_size);
        }
      } while (1);
    } else {
      if (norflash_api_get_used_buffer_count(
              NORFLASH_API_MODULE_ID_USERDATA_EXT, NORFLASH_API_ALL) == 0) {
        nv_flsh_state.state = NV_STATE_BAK_DONE;
        TRACE(3, "%s: async flush done.", __func__);
      } else {
        norflash_api_flush();
      }
    }
    int_unlock_global(lock);
    hal_trace_continue();
  } else {
    TRACE(4, "%s: sync flush begin.", __func__);
    if (nv_flsh_state.state == NV_STATE_MAIN_DONE ||
        nv_flsh_state.state == NV_STATE_BAK_ERASING) {
      do {
        hal_trace_pause();
        lock = int_lock_global();
        ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                 _user_data_bak_start, NV_EXTENSION_SIZE, true);
        int_unlock_global(lock);
        hal_trace_continue();
        if (ret == NORFLASH_API_OK) {
          nv_flsh_state.state = NV_STATE_BAK_ERASED;
          TRACE(4, "%s: norflash_api_erase ok,addr = 0x%x.", __func__,
                _user_data_bak_start);
        } else if (ret == NORFLASH_API_BUFFER_FULL) {
          do {
            norflash_api_flush();
          } while (norflash_api_get_free_buffer_count(NORFLASH_API_ERASING) ==
                   0);
        } else {
          ASSERT(0, "%s: norflash_api_erase err,ret = %d,addr = 0x%x.",
                 __func__, ret, _user_data_bak_start);
        }
      } while (ret == NORFLASH_API_BUFFER_FULL);
    }

    if (nv_flsh_state.state == NV_STATE_BAK_ERASED ||
        nv_flsh_state.state == NV_STATE_BAK_WRITTING) {
      nv_flsh_state.state = NV_STATE_BAK_WRITTING;
      nv_flsh_state.written_size = 0;

      hal_trace_pause();
      do {
        lock = int_lock_global();
        ret = norflash_api_read(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                _user_data_main_start +
                                    nv_flsh_state.written_size,
                                burn_buf, NV_EXTENSION_PAGE_SIZE);
        ASSERT(ret == NORFLASH_API_OK,
               "norflash_api_read failed! ret = %d, addr = 0x%x.", (int32_t)ret,
               _user_data_main_start + nv_flsh_state.written_size);

        ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                                 _user_data_bak_start +
                                     nv_flsh_state.written_size,
                                 burn_buf, NV_EXTENSION_PAGE_SIZE, true);
        int_unlock_global(lock);
        if (ret == NORFLASH_API_OK) {
          nv_flsh_state.written_size += NV_EXTENSION_PAGE_SIZE;
          if (nv_flsh_state.written_size == NV_EXTENSION_SIZE) {
            nv_flsh_state.state = NV_STATE_BAK_WRITTEN;
            TRACE(3, "%s: NV_STATE_BAK_WRITTEN", __func__);
            break;
          }
        } else if (ret == NORFLASH_API_BUFFER_FULL) {
          do {
            norflash_api_flush();
          } while (norflash_api_get_free_buffer_count(NORFLASH_API_WRITTING) ==
                   0);
        } else {
          ASSERT(0, "%s: norflash_api_write err,ret = %d,addr = 0x%x.",
                 __func__, ret,
                 _user_data_bak_start + nv_flsh_state.written_size);
        }
      } while (1);
    }
    hal_trace_continue();

    // nv_flsh_state.state == NV_STATE_BAK_WRITTEN;
    do {
      norflash_api_flush();
    } while (norflash_api_get_used_buffer_count(
                 NORFLASH_API_MODULE_ID_USERDATA_EXT, NORFLASH_API_ALL) > 0);

    nv_flsh_state.state = NV_STATE_BAK_DONE;
    TRACE(1, "%s: sync flush done.", __func__);
  }

  if (nv_flsh_state.state == NV_STATE_BAK_DONE) {
    nv_flsh_state.state = NV_STATE_IDLE;
    TRACE(1, "%s: NV_STATE_IDLE", __func__);
  }

  return (ret == NORFLASH_API_OK) ? 0 : 1;
}

static int nv_record_extension_flush(bool is_async) {
  int ret = 0;
  // TRACE(3, "%s state %d is_update %d", __func__, nv_flsh_state.state,
  // nv_flsh_state.is_update);
  do {
    if (nv_flsh_state.state == NV_STATE_IDLE &&
        nv_flsh_state.is_update == FALSE) {
      break;
    }

    if ((nv_flsh_state.state == NV_STATE_IDLE &&
         nv_flsh_state.is_update == TRUE) ||
        nv_flsh_state.state == NV_STATE_MAIN_ERASING ||
        nv_flsh_state.state == NV_STATE_MAIN_ERASED ||
        nv_flsh_state.state == NV_STATE_MAIN_WRITTING ||
        nv_flsh_state.state == NV_STATE_MAIN_WRITTEN) {
      ret = nv_record_extension_flush_main(is_async);
      if (is_async) {
        break;
      }
    }

    if (nv_flsh_state.state == NV_STATE_MAIN_DONE ||
        nv_flsh_state.state == NV_STATE_BAK_ERASING ||
        nv_flsh_state.state == NV_STATE_BAK_ERASED ||
        nv_flsh_state.state == NV_STATE_BAK_WRITTING ||
        nv_flsh_state.state == NV_STATE_BAK_WRITTEN ||
        nv_flsh_state.state == NV_STATE_BAK_DONE) {
      ret = nv_record_extension_flush_bak(is_async);
    }
  } while (!is_async);
  return ret;
}

void nv_extension_callback(void *param) {
  NORFLASH_API_OPERA_RESULT *opera_result;
  opera_result = (NORFLASH_API_OPERA_RESULT *)param;

  TRACE(6, "%s:type = %d, addr = 0x%x,len = 0x%x,remain = %d,result = %d.",
        __func__, opera_result->type, opera_result->addr, opera_result->len,
        opera_result->remain_num, opera_result->result);
}

void nv_record_init(void) {
  nv_record_open(section_usrdata_ddbrecord);

  nv_custom_parameter_section_init();
}

bt_status_t nv_record_open(SECTIONS_ADP_ENUM section_id) {
  nv_record_extension_init();
#ifdef FLASH_SUSPEND
  hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_USER_NVRECORD,
                           nv_record_flash_flush_in_sleep);
#else
  hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_USER_NVRECORD,
                                nv_record_flash_flush_in_sleep);
#endif
  return BT_STS_SUCCESS;
}

void nv_record_update_runtime_userdata(void) { nv_record_extension_update(); }

int nv_record_touch_cause_flush(void) {
  nv_record_update_runtime_userdata();
  return 0;
}

void nv_record_sector_clear(void) {
  uint32_t lock;
  enum NORFLASH_API_RET_T ret;

  lock = int_lock_global();
  ret =
      norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                         (uint32_t)__userdata_start, NV_EXTENSION_SIZE, false);
  ASSERT(ret == NORFLASH_API_OK,
         "%s: norflash_api_erase(0x%x) failed! ret = %d.", __func__,
         (uint32_t)__userdata_start, (int32_t)ret);

  ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                           (uint32_t)__userdata_start + NV_EXTENSION_SIZE,
                           NV_EXTENSION_SIZE, false);
  ASSERT(ret == NORFLASH_API_OK,
         "%s: norflash_api_erase(0x%x) failed! ret = %d.", __func__,
         (uint32_t)__userdata_start + NV_EXTENSION_SIZE, (int32_t)ret);
  // pmu_reboot();
  int_unlock_global(lock);
}

void nv_record_rebuild(void) {
  if (nvrecord_extension_p) {
    uint32_t nv_start;
    TRACE(1, "%s: begin.", __func__);
    uint32_t lock = int_lock_global();
    mpu_clear(MPU_ID_USER_DATA_SECTION);
    nv_record_set_default(nvrecord_extension_p);
    nv_start = (uint32_t)&local_extension_data.nv_record;
    mpu_set(MPU_ID_USER_DATA_SECTION, nv_start, NV_EXTENSION_MIRROR_RAM_SIZE, 0,
            MPU_ATTR_READ);
    nv_record_extension_update();
    nv_record_flash_flush();
    int_unlock_global(lock);
    TRACE(1, "%s: done.", __func__);
  } else {
    TRACE(1, "%s: begin. nvrecord_extension_p = null.", __func__);
    uint32_t lock = int_lock_global();
    nv_record_sector_clear();
    nvrec_init = false;
    nv_record_extension_init();
    int_unlock_global(lock);
    TRACE(1, "%s: done.", __func__);
  }
}

#define NV_RECORD_FLUSH_EXECUTION_INTERVAL_MS (20 * 60 * 1000)
static uint32_t lastFlushCheckPointInMs = 0;

static void nv_record_reset_flush_checkpoint(void) {
  lastFlushCheckPointInMs = GET_CURRENT_MS();
  ;
}

static bool nv_record_is_timer_expired_to_check(void) {
  uint32_t passedTimerMs;
  uint32_t currentTimerMs = GET_CURRENT_MS();

  if (0 == lastFlushCheckPointInMs) {
    passedTimerMs = currentTimerMs;
  } else {
    if (currentTimerMs > lastFlushCheckPointInMs) {
      passedTimerMs = currentTimerMs - lastFlushCheckPointInMs;
    } else {
      passedTimerMs = 0xFFFFFFFF - lastFlushCheckPointInMs + currentTimerMs + 1;
    }
  }

  if (passedTimerMs > NV_RECORD_FLUSH_EXECUTION_INTERVAL_MS) {
    return true;
  } else {
    return false;
  }
}

void nv_record_flash_flush(void) {
  nv_record_extension_flush(false);
  nv_record_reset_flush_checkpoint();
}

void nv_record_execute_async_flush(void) {
  int ret = nv_record_extension_flush(true);
  if (0 == ret) {
    nv_record_reset_flush_checkpoint();
  }
}

int nv_record_flash_flush_in_sleep(void) {
  if ((NV_STATE_IDLE == nv_flsh_state.state) &&
      !nv_record_is_timer_expired_to_check()) {
    return 0;
  }

  nv_record_execute_async_flush();
  return 0;
}

#endif // #if defined(NEW_NV_RECORD_ENABLED)
