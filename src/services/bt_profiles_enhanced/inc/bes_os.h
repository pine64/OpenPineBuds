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

#include <stdio.h>
#include <stdbool.h>

/* all platform dependences shoule be included only here */

#include "cmsis_os.h"
#include "cmsis.h"
#include "plat_types.h"
#include "hal_trace.h"
#include "hal_timer.h"
#include "string.h"
#include "tgt_hardware.h"
#include "bt_drv_reg_op.h"
#include "intersyshci.h"
#include "besbt_cfg.h"
#include "osif.h"
#include "ddbif.h"
#include "btlib_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define Plt_Assert ASSERT
#define Plt_TICKS_TO_MS(ticks) TICKS_TO_MS(ticks)
#define Plt_DUMP8 DUMP8

#define OS_CRITICAL_METHOD      0
#define OS_ENTER_CRITICAL()     uint32_t os_lock = int_lock()
#define OS_EXIT_CRITICAL()      int_unlock(os_lock)
#define OSTimeDly(a)            osDelay((a)*2)


#if defined(__cplusplus)
}
#endif