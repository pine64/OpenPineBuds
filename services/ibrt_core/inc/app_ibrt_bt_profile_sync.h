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
#ifndef __APP_IBRT_BT_PROFILE_SYNC__
#define __APP_IBRT_BT_PROFILE_SYNC__

#include "stdint.h"


#define LM_SP_P192_PRE_COMP_POINTS  4
#define DEV_CLASS_LEN       3
#define FEATURE_PAGE_MAX   3
#define KEY_LEN             0x10
#define SRES_LEN            0x04
#define ACO_LEN             12
#define PIN_CODE_MAX_LEN    0x10
#define CHNL_MAP_LEN        0x0A
#define BD_NAME_SIZE        0xF8 // Was 0x20 for BLE HL
#define FEATS_LEN           0x08

#ifdef __cplusplus
extern "C" {
#endif

uint8_t app_ibrt_pack_bt_profile_data(uint8_t* buf, uint16_t* len, uint16_t conhdl);
uint8_t app_ibrt_unpack_bt_profile_data(uint8_t* buf, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif
