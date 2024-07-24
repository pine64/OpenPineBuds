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
#ifndef __HWTIMER_LIST_H__
#define __HWTIMER_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void *HWTIMER_ID;

typedef void (*HWTIMER_CALLBACK_T)(void *param);

enum E_HWTIMER_T {
    E_HWTIMER_OK = 0,
    E_HWTIMER_FAILED,
    E_HWTIMER_INVAL_ID,
    E_HWTIMER_INVAL_ST,
    E_HWTIMER_IN_CALLBACK,
    E_HWTIMER_NOT_FOUND,
};

void hwtimer_init(void);

HWTIMER_ID hwtimer_alloc(HWTIMER_CALLBACK_T callback, void *param);

enum E_HWTIMER_T hwtimer_free(HWTIMER_ID id);

enum E_HWTIMER_T hwtimer_start(HWTIMER_ID id, unsigned int ticks);

enum E_HWTIMER_T hwtimer_update_then_start(HWTIMER_ID id, HWTIMER_CALLBACK_T callback, void *param, unsigned int ticks);

enum E_HWTIMER_T hwtimer_update(HWTIMER_ID id, HWTIMER_CALLBACK_T callback, void *param);

enum E_HWTIMER_T hwtimer_stop(HWTIMER_ID id);

#ifdef __cplusplus
}
#endif

#endif

