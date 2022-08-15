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
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "cmsis.h"
#include "nvrecord_extension.h"
#include "hal_trace.h"
#include "crc32.h"
#include "hal_norflash.h"
#include "norflash_api.h"
#include "norflash_drv.h"
#include "nvrecord_bt.h"
#include "nvrecord_env.h"
#include "nvrecord_ble.h"
#include "nvrecord_dma_config.h"
#include "nvrecord_fp_account_key.h"
#include "customparam_section.h"
#include "mpu.h"
#include "besbt.h"

extern uint32_t __userdata_start[];
extern uint32_t __userdata_end[];
extern void nvrecord_rebuild_system_env(struct nvrecord_env_t* pSystemEnv);
extern void nvrecord_rebuild_paired_bt_dev_info(NV_RECORD_PAIRED_BT_DEV_INFO_T* pPairedBtInfo);
#ifdef GFPS_ENABLED
extern void nvrecord_rebuild_fp_account_key(NV_FP_ACCOUNT_KEY_RECORD_T* pFpAccountKey);
#endif
#ifdef NVREC_BAIDU_DATA_SECTION
extern void nvrecord_rebuild_dma_configuration(NV_DMA_CONFIGURATION_T* pDmaConfig);
#endif

typedef enum
{
    NV_STATE_IDLE,
    NV_STATE_ERASED,
}NV_STATE;

NV_EXTENSION_RECORD_T *nvrecord_extension_p  = NULL;
static uint8_t isNvExtentionPendingForUpdate = false;
static NV_STATE isNvExtentionState = NV_STATE_IDLE;
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
                            __attribute__((section(".sram_data"))) =
{
    .nv_record = {
        {  // header
            NV_EXTENSION_MAGIC_NUMBER,
            NV_EXTENSION_MAJOR_VERSION,
            NV_EXTENSION_MINOR_VERSION,
            NV_EXTENSION_VALID_LEN,
            0,
        },

        {  // system info
        },

        {  // bt_pair_info
            0
        },

        {  // ble_pair_info

        },

#ifdef TWS_SYSTEM_ENABLED
        {  // tws_info

        },
#endif

#ifdef GFPS_ENABLED
        {  // fp_account_key_rec
            0
        },
#endif

#ifdef NVREC_BAIDU_DATA_SECTION
        {  // dma_config
            BAIDU_DATA_DEF_FM_FREQ,
        },
#endif

#ifdef TILE_DATAPATH
        {
            {0}
        },
#endif

#if defined(BISTO_ENABLED)
        {
            true,
            GSOUND_OTA_STATUS_NONE,
            0,
            0,
        },
#endif

#ifdef TX_IQ_CAL
        {
            BT_IQ_INVALID_MAGIC_NUM,
            {0},
            {0},
        },
#endif
        // TODO:
        // If want to extend the nvrecord while keeping the history information,
        // append the new items to the tail of NV_EXTENSION_RECORD_T and
        // set their intial content here
    }
};
STATIC_ASSERT(sizeof(local_extension_data) <= NV_EXTENSION_MIRROR_RAM_SIZE, "NV local buffer too small");

static void _nv_record_extension_init(void)
{
    enum NORFLASH_API_RET_T result;
    uint32_t sector_size = 0;
    uint32_t block_size = 0;
    uint32_t page_size = 0;

    hal_norflash_get_size(HAL_NORFLASH_ID_0,
               NULL,
               &block_size,
               &sector_size,
               &page_size);
    result = norflash_api_register(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                    HAL_NORFLASH_ID_0,
                    ((uint32_t)__userdata_start),
                    ((uint32_t)__userdata_end - (uint32_t)__userdata_start),
                    block_size,
                    sector_size,
                    page_size,
                    NV_EXTENSION_SIZE*2,
                    nv_extension_callback
                    );
    ASSERT(result == NORFLASH_API_OK,"_nv_record_extension_init: module register failed! result = %d.",result);
}

uint32_t nv_record_pre_write_operation(void)
{
    uint32_t lock = int_lock_global();
    mpu_clear(MPU_ID_USER_DATA_SECTION);
    return lock;
}

void nv_record_post_write_operation(uint32_t lock)
{
    int ret = 0;
    uint32_t nv_start = (uint32_t)&local_extension_data.nv_record;
    uint32_t len = NV_EXTENSION_MIRROR_RAM_SIZE;

    ret = mpu_set(MPU_ID_USER_DATA_SECTION, nv_start, len, 0, MPU_ATTR_READ);
    int_unlock_global(lock);
    TRACE(2,"set mpu 0x%x len %d result %d", nv_start, len, ret);
}

static void nv_record_extension_init(void)
{
    if (NULL == nvrecord_extension_p)
    {
        uint32_t lock = nv_record_pre_write_operation();
        _nv_record_extension_init();
        nvrecord_extension_p = &local_extension_data.nv_record;
        NVRECORD_HEADER_T* pExtRecInFlash =
            &(((NV_EXTENSION_RECORD_T *)__userdata_start)->header);
        TRACE(2,"nv ext magic 0x%x valid len %d", pExtRecInFlash->magicNumber,
            pExtRecInFlash->validLen);

        if ((NV_EXTENSION_MAJOR_VERSION == pExtRecInFlash->majorVersion) &&
            (NV_EXTENSION_MAGIC_NUMBER == pExtRecInFlash->magicNumber))
        {
            // check whether the data length is valid
            if (pExtRecInFlash->validLen <= (NV_EXTENSION_SIZE-NV_EXTENSION_HEADER_SIZE))
            {
                // check crc32
                uint32_t crc = crc32(0,((uint8_t *)pExtRecInFlash + NV_EXTENSION_HEADER_SIZE),
                    pExtRecInFlash->validLen);
                TRACE(2,"generated crc32 0x%x crc32 in flash 0x%x",
                    crc, pExtRecInFlash->crc32);
                if (crc == pExtRecInFlash->crc32)
                {
                    // correct
                    TRACE(0,"Nv extension is valid.");

                    TRACE(1,"Former nv ext valid len %d", pExtRecInFlash->validLen);
                    TRACE(1,"Current FW version nv ext valid len %d", NV_EXTENSION_VALID_LEN);

                    if (NV_EXTENSION_VALID_LEN < pExtRecInFlash->validLen)
                    {
                        TRACE(0,"Valid length of extension must be increased,"
                        "jump over the recovery but use the default value.");
                    }
                    else
                    {
                        memcpy(((uint8_t *)nvrecord_extension_p) + NV_EXTENSION_HEADER_SIZE,
                            ((uint8_t *)pExtRecInFlash) + NV_EXTENSION_HEADER_SIZE, pExtRecInFlash->validLen);

                        // frimware updates the nv extension data structure
                        if (pExtRecInFlash->validLen < NV_EXTENSION_VALID_LEN)
                        {
                            // update the version number
                            nvrecord_extension_p->header.minorVersion = NV_EXTENSION_MINOR_VERSION;

                            // re-calculate the crc32
                            nvrecord_extension_p->header.crc32 = crc32(0, ((uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE),
                                NV_EXTENSION_VALID_LEN);
                            // need to update the content in the flash
                            nv_record_extension_update();
                        }

                        goto exit;
                    }
                }
            }
        }

        // the nv extension is invalid, should be recreated
        nvrecord_rebuild_system_env(&(nvrecord_extension_p->system_info));
        nvrecord_rebuild_paired_bt_dev_info(&(nvrecord_extension_p->bt_pair_info));
        nvrecord_rebuild_paired_ble_dev_info(&(nvrecord_extension_p->ble_pair_info));
    #ifdef GFPS_ENABLED
        nvrecord_rebuild_fp_account_key(&(nvrecord_extension_p->fp_account_key_rec));
    #endif

    #ifdef NVREC_BAIDU_DATA_SECTION
        nvrecord_rebuild_dma_configuration(&(nvrecord_extension_p->dma_config));
    #endif

        pExtRecInFlash->crc32 = crc32(0, ((uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE),
            NV_EXTENSION_VALID_LEN);
        // need to update the content in the flash
        nv_record_extension_update();

    exit:
        nv_record_post_write_operation(lock);

    }
}

NV_EXTENSION_RECORD_T* nv_record_get_extension_entry_ptr(void)
{
    return nvrecord_extension_p;
}

void nv_record_extension_update(void)
{
    isNvExtentionPendingForUpdate = true;
}

static void nv_record_extension_flush(bool is_async)
{
    enum NORFLASH_API_RET_T ret;
    uint32_t lock;
    uint32_t crc;

#if defined(FLASH_SUSPEND) && defined(NVREC_BURN_TEST)
    static uint32_t stime = 0;
    if (hal_sys_timer_get() - stime > MS_TO_TICKS(20000)) {
        stime = hal_sys_timer_get();
        isNvExtentionPendingForUpdate = true;
    }
#endif

    if (isNvExtentionPendingForUpdate)
    {
        if(is_async)
        {
            lock = int_lock_global();
            if(isNvExtentionState == NV_STATE_IDLE)
            {
                ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                            (uint32_t)(__userdata_start),
                            NV_EXTENSION_SIZE,
                            true
                            );
                if(ret == NORFLASH_API_OK)
                {
                    isNvExtentionState = NV_STATE_ERASED;
                    TRACE(2,"%s: norflash_api_erase ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                }
                else if(ret == NORFLASH_API_BUFFER_FULL)
                {
                    // do nothing.
                }
                else
                {
                    ASSERT(0,"%s: norflash_api_erase err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                }
            }
            if(isNvExtentionState == NV_STATE_ERASED)
            {
                uint32_t tmpLock = nv_record_pre_write_operation();
                crc = crc32(0,(uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
                     NV_EXTENSION_VALID_LEN);
                nvrecord_extension_p->header.crc32 = crc;
                nv_record_post_write_operation(tmpLock);
                ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                            (uint32_t)(__userdata_start),
                            (uint8_t *)nvrecord_extension_p,
                            NV_EXTENSION_SIZE,
                            true);
                if(ret == NORFLASH_API_OK)
                {
                    isNvExtentionPendingForUpdate = false;
                    isNvExtentionState = NV_STATE_IDLE;
                    TRACE(2,"%s: norflash_api_write ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                }
                else if(ret == NORFLASH_API_BUFFER_FULL)
                {
                    // do nothing.
                }
                else
                {
                    ASSERT(0,"%s: norflash_api_write err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                }
            }
            int_unlock_global(lock);
        }
        else
        {
            if(isNvExtentionState == NV_STATE_IDLE)
            {
                do
                {
                    lock = int_lock_global();
                    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                            (uint32_t)(__userdata_start),
                            NV_EXTENSION_SIZE,
                            false
                            );
                    int_unlock_global(lock);
                    if(ret == NORFLASH_API_OK)
                    {
                        isNvExtentionState = NV_STATE_ERASED;
                        TRACE(2,"%s: norflash_api_erase ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                    }
                    else if(ret == NORFLASH_API_BUFFER_FULL)
                    {
                        do
                        {
                            norflash_api_flush();
                        }while(norflash_api_get_free_buffer_count(NORFLASH_API_ERASING) == 0);
                    }
                    else
                    {
                        ASSERT(0,"%s: norflash_api_erase err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                    }
                }while(ret == NORFLASH_API_BUFFER_FULL);
            }

            if(isNvExtentionState == NV_STATE_ERASED)
            {
                do
                {
                    lock = nv_record_pre_write_operation();
                    crc = crc32(0,(uint8_t *)nvrecord_extension_p + NV_EXTENSION_HEADER_SIZE,
                         NV_EXTENSION_VALID_LEN);
                    nvrecord_extension_p->header.crc32 = crc;
                    ret = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                            (uint32_t)(__userdata_start),
                            (uint8_t *)nvrecord_extension_p,
                            NV_EXTENSION_SIZE,
                            true);
                    nv_record_post_write_operation(lock);
                    if(ret == NORFLASH_API_OK)
                    {
                        isNvExtentionPendingForUpdate = false;
                        isNvExtentionState = NV_STATE_IDLE;
                        TRACE(2,"%s: norflash_api_write ok,addr = 0x%x.",__func__,(uint32_t)__userdata_start);
                    }
                    else if(ret == NORFLASH_API_BUFFER_FULL)
                    {
                        do
                        {
                            norflash_api_flush();
                        }while(norflash_api_get_free_buffer_count(NORFLASH_API_WRITTING) == 0);
                    }
                    else
                    {
                        ASSERT(0,"%s: norflash_api_write err,ret = %d,addr = 0x%x.",__func__,ret,(uint32_t)__userdata_start);
                    }
                }while(ret == NORFLASH_API_BUFFER_FULL);

                do
                {
                    norflash_api_flush();
                }while(norflash_api_get_used_buffer_count(NORFLASH_API_MODULE_ID_USERDATA_EXT,NORFLASH_API_ALL) > 0);
            }
        }
    }
}

void nv_extension_callback(void* param)
{
    NORFLASH_API_OPERA_RESULT *opera_result;
    opera_result = (NORFLASH_API_OPERA_RESULT*)param;

    TRACE(6,"%s:type = %d, addr = 0x%x,len = 0x%x,remain = %d,result = %d.",
                __func__,
                opera_result->type,
                opera_result->addr,
                opera_result->len,
                opera_result->remain_num,
                opera_result->result);
}

void nv_record_init(void)
{
    nv_record_open(section_usrdata_ddbrecord);

    nv_custom_parameter_section_init();
}

bt_status_t nv_record_open(SECTIONS_ADP_ENUM section_id)
{
    nv_record_extension_init();
    return BT_STS_SUCCESS;
}

void nv_record_update_runtime_userdata(void)
{
    nv_record_extension_update();
}


int nv_record_touch_cause_flush(void)
{
    nv_record_update_runtime_userdata();
    return 0;
}

void nv_record_sector_clear(void)
{
    uint32_t lock;
    enum NORFLASH_API_RET_T ret;

    lock = int_lock_global();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA_EXT,
                          (uint32_t)__userdata_start,
                          NV_EXTENSION_SIZE,
                          false);
    nvrecord_extension_p = NULL;
    int_unlock_global(lock);
    nv_record_update_runtime_userdata();
    ASSERT(ret == NORFLASH_API_OK,
        "nv_record_sector_clear: norflash_api_erase failed! ret = %d.",ret);
}

void nv_record_rebuild(void)
{
    nv_record_sector_clear();
    nv_record_extension_init();
}

void nv_record_flash_flush(void)
{
}

int nv_record_flash_flush_in_sleep(void)
{
    nv_record_extension_flush(true);
    return 0;
}

#if defined(OTA_ENABLED)
void nv_record_extension_update_gsound_ota_session(uint8_t gsoundOtaStatus,
                                                   uint32_t totalImageSize,
                                                   const char *session)
{
    NV_EXTENSION_RECORD_T* pNvExtRec = nv_record_get_extension_entry_ptr();
    TRACE(2,"gsound ota status is %d, is start is %d",
          pNvExtRec->gsound_info.gsoundOtaStatus,
          gsoundOtaStatus);

    if (pNvExtRec->gsound_info.gsoundOtaStatus != gsoundOtaStatus)
    {
        if (GSOUND_OTA_STATUS_NONE == gsoundOtaStatus)
        {
            pNvExtRec->gsound_info.gsoundOtaOffset = 0;
            memset(pNvExtRec->gsound_info.gsoundOtaSessionString,
                   0,
                   sizeof(pNvExtRec->gsound_info.gsoundOtaSessionString));
        }
        else if (GSOUND_OTA_STAGE_ONGOING == gsoundOtaStatus)
        {
            ASSERT(totalImageSize, "Received total image size is 0.");
            ASSERT(session, "Received session pointer is NULL.");
            pNvExtRec->gsound_info.gsoundOtaImageSize = totalImageSize;
            memcpy(pNvExtRec->gsound_info.gsoundOtaSessionString, session, strlen(session)+1);
        }
        else if (GSOUND_OTA_STATUS_COMPLETE == gsoundOtaStatus)
        {
            ASSERT(GSOUND_OTA_STAGE_ONGOING == pNvExtRec->gsound_info.gsoundOtaStatus,
                   "Wrong status transmission.");
            ASSERT(totalImageSize == pNvExtRec->gsound_info.gsoundOtaImageSize,
                   "total image size changed.");

            pNvExtRec->gsound_info.gsoundOtaOffset = totalImageSize;
        }

        pNvExtRec->gsound_info.gsoundOtaStatus = gsoundOtaStatus;

        TRACE(1,"update gsound ota status to %d", pNvExtRec->gsound_info.gsoundOtaStatus);
        nv_record_extension_update();
        nv_record_flash_flush();
    }
}

void nv_record_extension_update_gsound_ota_progress(uint32_t otaOffset)
{
    NV_EXTENSION_RECORD_T *pNvExtRec = nv_record_get_extension_entry_ptr();
    if ((GSOUND_OTA_STAGE_ONGOING == pNvExtRec->gsound_info.gsoundOtaStatus) &&
        (otaOffset <= pNvExtRec->gsound_info.gsoundOtaImageSize))
    {
        pNvExtRec->gsound_info.gsoundOtaOffset = otaOffset;
        nv_record_extension_update();
        nv_record_flash_flush();
    }
}
#endif
#endif // #if defined(NEW_NV_RECORD_ENABLED)
