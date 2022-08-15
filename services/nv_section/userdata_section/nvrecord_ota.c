/**
 * @file nvrecord_ota.c
 * @author BES AI team
 * @version 0.1
 * @date 2020-04-21
 * 
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "string.h"
#include "nv_section_dbg.h"
#include "nvrecord_ota.h"
#include "ota_common.h"

#ifdef BISTO_ENABLED
#include "nvrecord_gsound.h"
#endif

/*********************external function declearation************************/

/************************private macro defination***************************/

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/
static NV_OTA_INFO_T *nvrecord_ota_p = NULL;

/****************************function defination****************************/
void nv_record_ota_init(void)
{
    if (NULL == nvrecord_ota_p)
    {
        nvrecord_ota_p = nvrecord_extension_p->ota_info;
    }
}

void nv_record_ota_get_ptr(void **ptr)
{
    *ptr = nvrecord_ota_p;
}

void nv_record_ota_update_breakpoint(uint8_t user,
                                     uint8_t deviceIndex,
                                     uint32_t otaOffset)
{
    NV_OTA_INFO_T *pOta = &nvrecord_ota_p[deviceIndex];
    bool update = false;

    ASSERT(OTA_STAGE_ONGOING == pOta->otaStatus,
           "try to update break point when OTA not in progress");

    uint32_t lock = nv_record_pre_write_operation();
    if (user != pOta->user)
    {
        LOG_I("OTA user update:%d->%d", pOta->user, user);
        pOta->user = user;
        update = true;
    }

    if (otaOffset != pOta->breakPoint)
    {
        LOG_I("OTA break point update:%x->%x", pOta->breakPoint, otaOffset);
        pOta->breakPoint = otaOffset;
        update = true;
    }
    nv_record_post_write_operation(lock);

    if (update)
    {
        nv_record_extension_update();
        nv_record_execute_async_flush();
    }
}

bool nv_record_ota_update_info(uint8_t user,
                               uint8_t deviceIndex,
                               uint8_t status,
                               uint32_t totalImageSize,
                               const char *session)
{
    NV_OTA_INFO_T *pOta = &nvrecord_ota_p[deviceIndex];

    LOG_I("%s", __func__);
    LOG_I("ota_status:%d->%d, user:%d, device_index:%d, imageSize:%d, version:%s",
          pOta->otaStatus,
          status,
          user,
          deviceIndex,
          totalImageSize,
          session);

    bool ret = true;
    bool update = false;

#ifdef GSOUND_HOTWORD_ENABLED
    if ((OTA_USER_COLORFUL == user) &&
        (OTA_DEVICE_HOTWORD == deviceIndex))
    {
        ret = nv_record_gsound_rec_is_model_insert_allowed(session);
    }
#endif

    if (ret)
    {
        uint32_t lock = nv_record_pre_write_operation();
        if (!memcmp(pOta->version, session, strlen(session) + 1))
        {
            memset(pOta->version,
                   0,
                   ARRAY_SIZE(pOta->version));
            update = true;
        }

        if (pOta->otaStatus != status)
        {
            pOta->otaStatus = status;
            update = true;
            LOG_I("update device %d ota status to %d",
                  deviceIndex,
                  pOta->otaStatus);
        }

        if (OTA_STAGE_IDLE == status)
        {
            if (0 != pOta->breakPoint)
            {
                pOta->breakPoint = 0;
                update = true;
            }
        }
        else if (OTA_STAGE_ONGOING == status)
        {
            ASSERT(totalImageSize, "Received total image size is 0.");
            ASSERT(session, "Received session pointer is NULL.");

            if (pOta->imageSize != totalImageSize)
            {
                pOta->imageSize = totalImageSize;
                update = true;
            }
        }
        else if (OTA_STAGE_DONE == status)
        {
            ASSERT(totalImageSize == pOta->imageSize,
                   "total image size changed:%d->%d.", pOta->imageSize, totalImageSize);

            if (totalImageSize != pOta->breakPoint)
            {
                pOta->breakPoint = totalImageSize;
                update = true;
            }
        }
        nv_record_post_write_operation(lock);

        if (update)
        {
            LOG_I("will update info into flash.");
            nv_record_extension_update();
            nv_record_execute_async_flush();
        }
    }

    return ret;
}
