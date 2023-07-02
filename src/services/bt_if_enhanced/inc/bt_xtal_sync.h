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
#ifndef __BT_XTAL_SYNC_H__
#define __BT_XTAL_SYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

enum BT_XTAL_SYNC_MODE_T {
    BT_XTAL_SYNC_MODE_MUSIC = 1,
    BT_XTAL_SYNC_MODE_VOICE = 2,
    BT_XTAL_SYNC_MODE_WITH_MOBILE = 3,
    BT_XTAL_SYNC_MODE_WITH_TWS = 4,
};

void bt_xtal_sync(enum BT_XTAL_SYNC_MODE_T mode);
#ifdef BT_XTAL_SYNC_NEW_METHOD
typedef struct {
    float Kp;
    float Kd;
}pid_para_t;
void bt_xtal_sync_new(int32_t rxbit, bool fix_rxbit_en, enum BT_XTAL_SYNC_MODE_T mode);
void bt_xtal_sync_new_new(int32_t rxbit, bool fix_rxbit_en, enum BT_XTAL_SYNC_MODE_T mode);

#endif
void bt_init_xtal_sync(enum BT_XTAL_SYNC_MODE_T mode, int range_min, int range_max, int fcap_range);
void bt_term_xtal_sync(bool xtal_term_default);
void bt_term_xtal_sync_default(void);

#ifdef __cplusplus
}
#endif

#endif
