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
#ifndef __section_def_h__
#define __section_def_h__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint32_t crc;
    uint32_t reserved0;
    uint32_t reserved1;
} section_head_t;

#ifdef __cplusplus
}
#endif

#endif
