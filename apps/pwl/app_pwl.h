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
#ifndef __APP_PWL_H__
#define __APP_PWL_H__

#include "tgt_hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

enum APP_PWL_ID_T {
    APP_PWL_ID_0 = 0,
    APP_PWL_ID_1 = 1,

#if (CFG_HW_PLW_NUM == 1)
    APP_PWL_ID_QTY = 1,
#elif (CFG_HW_PLW_NUM == 2)
    APP_PWL_ID_QTY = 2,
#else
    APP_PWL_ID_QTY = 0,
#endif
};

struct APP_PWL_CFG_T {
    struct PWL_CYCLE_ONCE {
        uint8_t  level;
        uint32_t time;
    }part[10];
    uint8_t parttotal;
    uint8_t startlevel;
    bool periodic;
};

int app_pwl_open(void);

int app_pwl_start(enum APP_PWL_ID_T id);

int app_pwl_setup(enum APP_PWL_ID_T id, struct APP_PWL_CFG_T *cfg);

int app_pwl_stop(enum APP_PWL_ID_T id);

int app_pwl_close(void);

#ifdef __cplusplus
}
#endif

#endif

