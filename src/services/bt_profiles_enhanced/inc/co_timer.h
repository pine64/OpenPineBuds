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

#ifndef __CO_TIMER_H__
#define __CO_TIMER_H__

#if defined(__cplusplus)
extern "C" {
#endif

int co_timer_is_running (uint8 *timer_id);
void co_timer_init(unsigned char timer_task_prio, unsigned int *ptos );
void co_timer_del(uint8 *timer_handle);
void co_timer_cfg(uint8 *timer_handle, unsigned int millisecond, void (*timer_func)(void *), void *arg, int times);
void co_timer_resume(uint8 *timer_handle );
void co_timer_restart(uint8 *timer_handle );
void co_timer_update_and_restart(uint8 *timer_handle,unsigned int millisecond);;
void co_timer_start(uint8 *timer_handle);
void co_timer_stop(uint8 *timer_handle);
int co_timer_new(uint8 *timer_handle, unsigned int millisecond, void (*timer_func)(void *), void *arg, int times);
void co_timer_reset(void );
void co_timer_reduce(uint8 *timer_handle, unsigned int reduece_ms);
void co_timer_reduce_all(unsigned int reduece_ms);
void co_timer_check(void);

#if defined(__cplusplus)
}
#endif

#endif /* __CO_TIMER_H__ */
