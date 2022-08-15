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
#ifndef __PATCH_H__
#define __PATCH_H__

#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PATCH_TYPE_T {
    PATCH_TYPE_CODE,
    PATCH_TYPE_DATA,
    PATCH_TYPE_FUNC,

    PATCH_TYPE_QTY
};

typedef int PATCH_ID;

int patch_open(uint32_t remap_addr);

PATCH_ID patch_enable(enum PATCH_TYPE_T type, uint32_t addr, uint32_t data);

int patch_disable(PATCH_ID patch_id);

void patch_close(void);

#ifdef __cplusplus
}
#endif

#endif
