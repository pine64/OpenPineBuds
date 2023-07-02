/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifndef __REG_SLAVE_I2C_H__
#define __REG_SLAVE_I2C_H__

#include "plat_types.h"

struct SLAVE_I2C_T {
    __IO uint32_t EN;                       // 0x000
    __IO uint32_t ID;                       // 0x004
    __IO uint32_t SMP;                      // 0x008
    __IO uint32_t RESERVED_00C[5];          // 0x00C
    __IO uint32_t TBP;                      // 0x020
};

#define I2C_EN                              (1 << 0)
#define W_FILTERLEN_SHIFT                   7
#define W_FILTERLEN_MASK                    (0xFF << W_FILTERLEN_SHIFT)
#define W_FILTERLEN(n)                      BITFIELD_VAL(W_FILTERLEN, n)
#define R_FILTERLEN_SHIFT                   8
#define R_FILTERLEN_MASK                    (0xFF << R_FILTERLEN_SHIFT)
#define R_FILTERLEN(n)                      BITFIELD_VAL(R_FILTERLEN, n)

#define DEV_ID_SHIFT                        0
#define DEV_ID_MASK                         (0xFF << DEV_ID_SHIFT)
#define DEV_ID(n)                           BITFIELD_VAL(DEV_ID, n)

#define SMP_I2C_SEL_SHIFT                   0
#define SMP_I2C_SEL_MASK                    (0xF << SMP_I2C_SEL_SHIFT)
#define SMP_I2C_SEL(n)                      BITFIELD_VAL(SMP_I2C_SEL, n)

#define TIMEOUT_BYPASS                      (1 << 0)

#endif

