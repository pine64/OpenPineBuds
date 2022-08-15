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
#if defined(NEW_NV_RECORD_ENABLED)

#ifdef GFPS_ENABLED

#ifndef __NVRECORD_FP_ACCOUNT_KEY_H__
#define __NVRECORD_FP_ACCOUNT_KEY_H__

#include "nvrecord_extension.h"

#ifdef __cplusplus
extern "C" {
#endif

void nv_record_fp_account_key_add(NV_FP_ACCOUNT_KEY_ENTRY_T* param_rec);
void nv_record_fp_account_key_delete();
void nv_record_fp_account_key_init(void);
bool nv_record_fp_account_key_get_by_index(uint8_t index, uint8_t* outputKey);
uint8_t nv_record_fp_account_key_count(void);
void nvrecord_rebuild_fp_account_key(NV_FP_ACCOUNT_KEY_RECORD_T* pFpAccountKey);
void nv_record_fp_update_name(uint8_t* ptrName, uint32_t nameLen);
uint8_t* nv_record_fp_get_name_ptr(uint32_t* ptrNameLen);
NV_FP_ACCOUNT_KEY_RECORD_T* nv_record_get_fp_data_structure_info(void);
void nv_record_update_fp_data_structure(NV_FP_ACCOUNT_KEY_RECORD_T* pFpData);

void nv_record_fp_update_all(uint8_t *info);

#ifdef __cplusplus
}
#endif
#endif

#endif

#endif // #if defined(NEW_NV_RECORD_ENABLED)
