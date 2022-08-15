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
#ifndef __SYSTEM_ARMCM_H__
#define __SYSTEM_ARMCM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"

void SystemInit (void);

#ifdef UNALIGNED_ACCESS

__STATIC_FORCEINLINE bool get_unaligned_access_status(void) { return true; }

__STATIC_FORCEINLINE bool config_unaligned_access(bool enable) { return true; }

#else

bool get_unaligned_access_status(void);

bool config_unaligned_access(bool enable);

#endif

uint32_t get_cpu_id(void);

#ifdef __cplusplus
}
#endif

#endif

