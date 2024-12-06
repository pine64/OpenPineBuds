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

#ifndef NVRECORD_BLE_H
#define NVRECORD_BLE_H

#include "nvrecord_extension.h"

#ifdef __cplusplus
extern "C" {
#endif

int nv_record_blerec_add(const BleDeviceinfo *param_rec);
bool nv_record_ble_record_find_ltk_through_static_bd_addr(uint8_t* pBdAddr, uint8_t *ltk);
bool nv_record_ble_record_Once_a_device_has_been_bonded(void);
void nv_record_ble_delete_entry(uint8_t* pBdAddr);
uint8_t nv_record_ble_fill_irk(uint8_t* ltkToFill);
void nv_record_blerec_init(void);
NV_RECORD_PAIRED_BLE_DEV_INFO_T* nv_record_blerec_get_ptr(void);
void nv_record_blerec_get_local_irk(uint8_t* pIrk);
bool nv_record_blerec_get_bd_addr_from_irk(uint8_t* pBdAddr, uint8_t* pIrk);
void nvrecord_rebuild_paired_ble_dev_info(NV_RECORD_PAIRED_BLE_DEV_INFO_T* pPairedBtInfo);

#ifdef TWS_SYSTEM_ENABLED
void nv_record_extension_update_tws_ble_info(NV_RECORD_PAIRED_BLE_DEV_INFO_T *info);
void nv_record_tws_exchange_ble_info(void);
uint8_t *nv_record_tws_get_self_ble_info(void);
uint8_t *nv_record_tws_get_peer_ble_addr(void);
#endif

#ifdef __cplusplus
}
#endif

#endif 
#endif // #if defined(NEW_NV_RECORD_ENABLED)

