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
#include "nvrecord_fp_account_key.h"
#include "hal_trace.h"
#include "co_math.h" 


extern nv_record_struct nv_record_config;
static NV_FP_ACCOUNT_KEY_RECORD_T *nvrecord_fp_account_key_p = NULL;

NV_FP_ACCOUNT_KEY_RECORD_T* nv_record_get_fp_data_structure_info(void)
{
    return nvrecord_fp_account_key_p;
}

void nv_record_update_fp_data_structure(NV_FP_ACCOUNT_KEY_RECORD_T* pFpData)
{
    if (!memcmp((uint8_t *)pFpData, (uint8_t *)nvrecord_fp_account_key_p, 
        sizeof(NV_FP_ACCOUNT_KEY_RECORD_T)))
    {
        // the updated fp data is the same as the local content, do nothing
        
    }
    else
    {
        TRACE(0,"Fast pair non-volatile data needs to be updated to aligned with peer device.");
        *nvrecord_fp_account_key_p = *pFpData;
        nv_record_update_runtime_userdata();
    }
}

void nv_record_fp_account_key_init(void)
{
    if (NULL == nvrecord_fp_account_key_p)
    {
        nvrecord_fp_account_key_p = 
            (NV_FP_ACCOUNT_KEY_RECORD_T *)nvrec_config_get_int(nv_record_config.config, 
            (const char *)section_name_fp_account_key, 
            (const char *)NV_RECORD_ACCOUNT_KEY, 
            NVRAM_ACCOUNT_KEY_INVALID);
    }
    
    if ((NV_FP_ACCOUNT_KEY_RECORD_T *)NVRAM_ACCOUNT_KEY_INVALID == nvrecord_fp_account_key_p)
    {
        nvrecord_fp_account_key_p = (NV_FP_ACCOUNT_KEY_RECORD_T *)pool_malloc(sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
        if (!nvrecord_fp_account_key_p)
        {
            ASSERT(0,"%s pool_malloc failed",__func__);
            return;
        }
        
        memset(nvrecord_fp_account_key_p, 0, sizeof(NV_FP_ACCOUNT_KEY_RECORD_T));
        
        nvrec_config_set_int(nv_record_config.config, section_name_fp_account_key,
            (const char *)NV_RECORD_ACCOUNT_KEY, (int)nvrecord_fp_account_key_p);
    }	
}

void nv_record_fp_account_key_add(NV_FP_ACCOUNT_KEY_ENTRY_T* param_rec)
{
    TRACE(0,"Add account key:");
    DUMP8("%02x ", param_rec->key, sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    if (FP_ACCOUNT_KEY_RECORD_NUM == nvrecord_fp_account_key_p->key_count)
    {
        // discard the earliest key
        memmove((uint8_t *)&(nvrecord_fp_account_key_p->accountKey[0]), 
            (uint8_t *)&(nvrecord_fp_account_key_p->accountKey[1]),
            (FP_ACCOUNT_KEY_RECORD_NUM - 1)*sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));

        nvrecord_fp_account_key_p->key_count--;
    }

    memcpy((uint8_t *)&(nvrecord_fp_account_key_p->accountKey[nvrecord_fp_account_key_p->key_count]), 
            param_rec,
            sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    nvrecord_fp_account_key_p->key_count++;
    
    nv_record_update_runtime_userdata();
}

void nv_record_fp_account_info_reset(void) {
  uint8_t size = nv_record_fp_account_key_count();
  for (uint8_t i = 0 ; i < size ; i++){
    memset((uint8_t *)&(nvrecord_fp_account_key_p->accountKey[i]), 0,
           sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
  }
  nvrecord_fp_account_key_p->key_count = 0;

  nv_record_fp_update_name(NULL, 0);
}

void nv_record_fp_account_key_delete(void)
{
    nv_record_fp_account_info_reset();
}

bool nv_record_fp_account_key_get_by_index(uint8_t index, uint8_t* outputKey)
{
    if (nvrecord_fp_account_key_p->key_count < (index + 1))
    {
        return false;
    }
    
    memcpy(outputKey, (uint8_t *)&(nvrecord_fp_account_key_p->accountKey[index]),
        sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));

    TRACE(1,"Get account key %d as:", index);
    DUMP8("%02x ", outputKey, sizeof(NV_FP_ACCOUNT_KEY_ENTRY_T));
    
    return true;
}

uint8_t nv_record_fp_account_key_count(void)
{
    return nvrecord_fp_account_key_p->key_count;
}

void nv_record_fp_update_name(uint8_t* ptrName, uint32_t nameLen)
{
    TRACE(1,"update name len %d", nameLen);
    
    memset(nvrecord_fp_account_key_p->name, 0, FP_MAX_NAME_LEN);
    if (nameLen > 0)
    {
        DUMP8("0x%02x ", ptrName, nameLen);
        memcpy(nvrecord_fp_account_key_p->name, ptrName, nameLen);
    }
    nvrecord_fp_account_key_p->nameLen = nameLen;
    nv_record_update_runtime_userdata();
}

uint8_t* nv_record_fp_get_name_ptr(uint32_t* ptrNameLen)
{
    *ptrNameLen = nvrecord_fp_account_key_p->nameLen;
    return nvrecord_fp_account_key_p->name;
}
#endif //#if !defined(NEW_NV_RECORD_ENABLED)
