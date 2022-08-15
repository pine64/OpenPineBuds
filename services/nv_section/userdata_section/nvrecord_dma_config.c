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
#include <assert.h>
#include "nvrecord_extension.h"
#include "nvrecord_dma_config.h"
#include "nvrecord.h"
#include "hal_trace.h"

#ifdef NVREC_BAIDU_DATA_SECTION
extern uint32_t* __factory_start;

void nvrecord_rebuild_dma_configuration(NV_DMA_CONFIGURATION_T* pDmaConfig)
{
    memset((uint8_t *)pDmaConfig, 0, sizeof(NV_DMA_CONFIGURATION_T));
    pDmaConfig->fmfreq = BAIDU_DATA_DEF_FM_FREQ;
}

int nvrec_baidu_data_init(void)
{
    if (NULL == nvrecord_extension_p)
    {
        return -1;
    }

    uint32_t lock = nv_record_pre_write_operation();
    nvrecord_rebuild_dma_configuration(&(nvrecord_extension_p->dma_config));
    nv_record_update_runtime_userdata();
    nv_record_post_write_operation(lock);
    return 0;
}

int nvrec_get_fm_freq(void)
{
    if (NULL == nvrecord_extension_p)
    {
        return -1;
    }

    int _fmfreq = nvrecord_extension_p->dma_config.fmfreq;

    TRACE(2,"%s:get fm freq %d", __func__, _fmfreq);
    return _fmfreq;
}

int nvrec_set_fm_freq(int fmfreq)
{
    if (NULL == nvrecord_extension_p)
    {
        return -1;
    }

    uint32_t lock = nv_record_pre_write_operation();
    nvrecord_extension_p->dma_config.fmfreq = fmfreq;
    nv_record_update_runtime_userdata();
    nv_record_post_write_operation(lock);

#if defined(NVREC_BAIDU_DATA_FLUSH_DIRECT)
    nv_record_flash_flush_int(false);
#endif

    return 0;
}

int nvrec_get_rand(char *rand)
{
    char _rand[BAIDU_DATA_RAND_LEN];
    int8_t copy_len = 0;

    if (NULL == rand)
    {
        return 1;
    }

    if (NULL == nvrecord_extension_p)
    {
        //_rand = BAIDU_DATA_DEF_RAND;
        copy_len = (sizeof(BAIDU_DATA_DEF_RAND)<BAIDU_DATA_RAND_LEN)?sizeof(BAIDU_DATA_DEF_RAND):BAIDU_DATA_RAND_LEN;
        memcpy(_rand, BAIDU_DATA_DEF_RAND, copy_len);
    }
    else
    {
        memcpy(_rand, nvrecord_extension_p->dma_config.rand, BAIDU_DATA_RAND_LEN);
    }

    memcpy(rand, _rand, BAIDU_DATA_RAND_LEN);
    TRACE(2,"%s:rand %s", __func__, rand);
    return 0;
}

int nvrec_set_rand(char *rand)
{
    if (NULL == rand)
    {
        return -1;
    }

    if (NULL == nvrecord_extension_p)
    {
        return -1;
    }

    uint32_t lock = nv_record_pre_write_operation();
    memcpy(nvrecord_extension_p->dma_config.rand, rand, BAIDU_DATA_RAND_LEN);
    nv_record_update_runtime_userdata();
    nv_record_post_write_operation(lock);

#if defined(NVREC_BAIDU_DATA_FLUSH_DIRECT)
    nv_record_flash_flush_int(false);
#endif

    return 0;
}

//#define GET_SN_FROM_FACTORY
int nvrec_dev_get_sn(char *sn)
{

    if (NULL == sn)
    {
        return -1;
    }
#ifdef GET_SN_FROM_FACTORY
    unsigned int sn_addr;
    sn_addr = (unsigned int)(__factory_start + rev2_dev_prod_sn);
#else
    char sn_addr[BAIDU_DATA_SN_LEN] = "01234567890123sf";
#endif
    memcpy((void *)sn, (void *)sn_addr, BAIDU_DATA_SN_LEN);

    return 0;
}

#endif // #ifdef NVREC_BAIDU_DATA_SECTION

#endif // #ifdef NEW_NV_RECORD_ENABLED
