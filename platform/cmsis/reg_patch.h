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
#ifndef __REG_PATCH_H__
#define __REG_PATCH_H__

#include "plat_types.h"
#include "plat_addr_map.h"

#ifdef PATCH_CTRL_BASE

struct PATCH_CTRL_T {
    __IO uint32_t CTRL[PATCH_ENTRY_NUM];
};

struct PATCH_DATA_T {
    __IO uint32_t DATA[PATCH_ENTRY_NUM];
};

#define PATCH_CTRL_ENTRY_EN                     (1 << 0)
#define PATCH_CTRL_ADDR_17_2_SHIFT              2
#define PATCH_CTRL_ADDR_17_2_MASK               (0xFFFF << PATCH_CTRL_ADDR_17_2_SHIFT)
#define PATCH_CTRL_17_2_ADDR(n)                 BITFIELD_VAL(PATCH_CTRL_30_2_ADDR, n)
// For entry 0 only
#define PATCH_CTRL_GLOBAL_EN                    (1 << 31)

#define PATCH_CTRL_ADDR_MASK                    PATCH_CTRL_ADDR_17_2_MASK
#define PATCH_CTRL_ADDR(n)                      ((n) & PATCH_CTRL_ADDR_17_2_MASK)

#endif

#endif
