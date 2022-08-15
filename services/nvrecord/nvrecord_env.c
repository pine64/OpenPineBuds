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
#if !defined(NEW_NV_RECORD_ENABLED)

#include <assert.h>
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "hal_trace.h"
#include "customparam_section.h"

#define NV_RECORD_ENV_KEY  "env_key"

extern nv_record_struct nv_record_config;
static struct nvrecord_env_t *nvrecord_env_p = NULL;

static int nv_record_env_new(void)
{

    nvrecord_env_p = (struct nvrecord_env_t *)pool_malloc(sizeof(struct nvrecord_env_t));

    if (!nvrecord_env_p)
    {
        TRACE(1,"%s pool_malloc failed ", __func__);
        return -1;
    }
    nvrec_config_set_int(nv_record_config.config,section_name_other,(const char *)NV_RECORD_ENV_KEY,(int)nvrecord_env_p);

    nvrecord_env_p->media_language.language = NVRAM_ENV_MEDIA_LANGUAGE_DEFAULT;
    nvrecord_env_p->ibrt_mode.mode = NVRAM_ENV_TWS_MODE_DEFAULT;
    nvrecord_env_p->ibrt_mode.tws_connect_success = 0;
    nvrecord_env_p->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT;

    nvrecord_env_p->aiManagerInfo.voice_key_enable = false;
    nvrecord_env_p->aiManagerInfo.setedCurrentAi = 0;
    nvrecord_env_p->aiManagerInfo.aiStatusDisableFlag = 0;
    nvrecord_env_p->aiManagerInfo.amaAssistantEnableStatus = 1;
    //nv_record_update_runtime_userdata();

    TRACE(2,"%s nvrecord_env_p:0x%x",__func__, (uint32_t)nvrecord_env_p);

    return 0;
}

static int nv_record_env_rebuild(void)
{
    if(nv_record_config_rebuild() != BT_STS_SUCCESS)
        return -1;

    if(nv_record_env_new())
        return -1;
    nv_record_config.is_update = true;
    nv_record_config.state = NV_STATE_IDLE;
    nv_record_config.written_size = 0;
    nv_record_flash_flush();

    return 0;
}

int nv_record_env_init(void)
{
    int nRet = 0;
    nv_record_open(section_usrdata_ddbrecord);

    nv_custom_parameter_section_init();

    nvrecord_env_p = (struct nvrecord_env_t *)nvrec_config_get_int(nv_record_config.config, (const char *)section_name_other,(const char *)NV_RECORD_ENV_KEY, NVRAM_ENV_INVALID);

    if (nvrecord_env_p == (struct nvrecord_env_t *)NVRAM_ENV_INVALID)
    {
        TRACE(0,"NVRAM_ENV_INVALID !!");
        if (nv_record_env_rebuild() < 0)
            nRet = -1;
        else
            TRACE(0,"NVRAM REBUILD SUCCESS !!");
    }
    TRACE(2,"%s nvrecord_env_p: 0x%x",__func__, (uint32_t)nvrecord_env_p);
    return nRet;
}

int nv_record_env_get(struct nvrecord_env_t **nvrecord_env)
{
    if (!nvrecord_env)
        return -1;

    if (!nvrecord_env_p)
        return -1;

    *nvrecord_env = nvrecord_env_p;

    return 0;
}

int nv_record_env_set(struct nvrecord_env_t *nvrecord_env)
{
    if (!nvrecord_env)
        return -1;

    if (!nvrecord_env_p)
        return -1;

    nv_record_update_runtime_userdata();

    return 0;
}

void nv_record_update_factory_tester_status(uint32_t status)
{
    if (nvrecord_env_p)
    {
        nvrecord_env_p->factory_tester_status.status = status;
    }
}

void nv_record_update_ibrt_info(uint32_t newMode,bt_bdaddr_t *ibrtPeerAddr)
{
    if (nvrecord_env_p)
    {
        TRACE(2,"##%s,%d",__func__,newMode);
        nvrecord_env_p->ibrt_mode.mode = newMode;
        nv_record_config.is_update = true;
        memcpy(nvrecord_env_p->ibrt_mode.record.bdAddr.address,ibrtPeerAddr->address,6);
    }
}

#endif // #if !defined(NEW_NV_RECORD_ENABLED)

