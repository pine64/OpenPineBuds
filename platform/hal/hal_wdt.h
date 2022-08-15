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
#ifndef WDT_HAL_H
#define WDT_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

enum HAL_WDT_ID_T {
    HAL_WDT_ID_0 = 0,
    HAL_WDT_ID_NUM,
};

enum HAL_WDT_EVENT_T {
    HAL_WDT_EVENT_FIRE = 0,
};

#define HAL_WDT_YES 1
#define HAL_WDT_NO 0

typedef void (*HAL_WDT_IRQ_CALLBACK)(enum HAL_WDT_ID_T id, uint32_t status);
/* hal api */
void hal_wdt_set_irq_callback(enum HAL_WDT_ID_T id, HAL_WDT_IRQ_CALLBACK handler);

/* mandatory operations */
int hal_wdt_start(enum HAL_WDT_ID_T id);
int hal_wdt_stop(enum HAL_WDT_ID_T id);

/* optional operations */
int hal_wdt_ping(enum HAL_WDT_ID_T id);
int hal_wdt_set_timeout(enum HAL_WDT_ID_T id, unsigned int);
unsigned int hal_wdt_get_timeleft(enum HAL_WDT_ID_T id);

#ifdef __cplusplus
}
#endif

#endif /* WDT_HAL_H */
