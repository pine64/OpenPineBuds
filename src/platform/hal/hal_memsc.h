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
#ifndef __HAL_MEMSC_H__
#define __HAL_MEMSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

enum HAL_MEMSC_ID_T {
    HAL_MEMSC_ID_0,
    HAL_MEMSC_ID_1,
    HAL_MEMSC_ID_2,
    HAL_MEMSC_ID_3,

    HAL_MEMSC_ID_QTY
};

int hal_memsc_lock(enum HAL_MEMSC_ID_T id);

void hal_memsc_unlock(enum HAL_MEMSC_ID_T id);

bool hal_memsc_avail(enum HAL_MEMSC_ID_T id);

#ifdef __cplusplus
}
#endif

#endif

