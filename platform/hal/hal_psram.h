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
#ifndef __HAL_PSRAM_H__
#define __HAL_PSRAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

enum HAL_PSRAM_ID_T {
    HAL_PSRAM_ID_0 = 0,
    HAL_PSRAM_ID_NUM,
};

void hal_psram_sleep(void);
void hal_psram_wakeup(void);
void hal_psram_init(void);

#ifdef __cplusplus
}
#endif

#endif

