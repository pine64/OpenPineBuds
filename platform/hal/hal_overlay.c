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
#include "plat_types.h"
#include "hal_overlay.h"
#include "hal_sysfreq.h"
#include "hal_cache.h"
#include "hal_trace.h"
#include "cmsis.h"
#ifdef __ARMCC_VERSION
#include "link_sym_armclang.h"
#endif

extern uint32_t __overlay_text_start__[];
extern uint32_t __overlay_text_exec_start__[];
extern uint32_t __load_start_overlay_text0[];
extern uint32_t __load_stop_overlay_text0[];
extern uint32_t __load_start_overlay_text1[];
extern uint32_t __load_stop_overlay_text1[];
extern uint32_t __load_start_overlay_text2[];
extern uint32_t __load_stop_overlay_text2[];
extern uint32_t __load_start_overlay_text3[];
extern uint32_t __load_stop_overlay_text3[];
extern uint32_t __load_start_overlay_text4[];
extern uint32_t __load_stop_overlay_text4[];
extern uint32_t __load_start_overlay_text5[];
extern uint32_t __load_stop_overlay_text5[];
extern uint32_t __load_start_overlay_text6[];
extern uint32_t __load_stop_overlay_text6[];
extern uint32_t __load_start_overlay_text7[];
extern uint32_t __load_stop_overlay_text7[];
extern uint32_t __overlay_text_exec_end__[];

extern uint32_t __overlay_data_start__[];
extern uint32_t __load_start_overlay_data0[];
extern uint32_t __load_stop_overlay_data0[];
extern uint32_t __load_start_overlay_data1[];
extern uint32_t __load_stop_overlay_data1[];
extern uint32_t __load_start_overlay_data2[];
extern uint32_t __load_stop_overlay_data2[];
extern uint32_t __load_start_overlay_data3[];
extern uint32_t __load_stop_overlay_data3[];
extern uint32_t __load_start_overlay_data4[];
extern uint32_t __load_stop_overlay_data4[];
extern uint32_t __load_start_overlay_data5[];
extern uint32_t __load_stop_overlay_data5[];
extern uint32_t __load_start_overlay_data6[];
extern uint32_t __load_stop_overlay_data6[];
extern uint32_t __load_start_overlay_data7[];
extern uint32_t __load_stop_overlay_data7[];

#ifndef NO_OVERLAY

#define OVERLAY_IS_FREE  0
#define OVERLAY_IS_USED  1

#define CHECK_OVERLAY_ID(id) \
    do {\
        if ((id < 0) || (id >= HAL_OVERLAY_ID_QTY)) { \
            ASSERT(0, "overlay id error %d", id); \
        } \
    } while (0)


static bool segment_state[HAL_OVERLAY_ID_QTY];

static enum HAL_OVERLAY_ID_T cur_overlay_id = HAL_OVERLAY_ID_QTY;

static uint32_t * const text_load_start[HAL_OVERLAY_ID_QTY] = {
    __load_start_overlay_text0,
    __load_start_overlay_text1,
    __load_start_overlay_text2,
    __load_start_overlay_text3,
    __load_start_overlay_text4,
    __load_start_overlay_text5,
    __load_start_overlay_text6,
    __load_start_overlay_text7,
};

static uint32_t * const text_load_stop[HAL_OVERLAY_ID_QTY] = {
    __load_stop_overlay_text0,
    __load_stop_overlay_text1,
    __load_stop_overlay_text2,
    __load_stop_overlay_text3,
    __load_stop_overlay_text4,
    __load_stop_overlay_text5,
    __load_stop_overlay_text6,
    __load_stop_overlay_text7,
};

static uint32_t * const data_load_start[HAL_OVERLAY_ID_QTY] = {
    __load_start_overlay_data0,
    __load_start_overlay_data1,
    __load_start_overlay_data2,
    __load_start_overlay_data3,
    __load_start_overlay_data4,
    __load_start_overlay_data5,
    __load_start_overlay_data6,
    __load_start_overlay_data7,
};

static uint32_t * const data_load_stop[HAL_OVERLAY_ID_QTY] = {
    __load_stop_overlay_data0,
    __load_stop_overlay_data1,
    __load_stop_overlay_data2,
    __load_stop_overlay_data3,
    __load_stop_overlay_data4,
    __load_stop_overlay_data5,
    __load_stop_overlay_data6,
    __load_stop_overlay_data7,
};


/*must called by lock get */
static void invalid_overlay_cache(enum HAL_OVERLAY_ID_T id)
{
    // TODO: PSRAM is large enough and no need to overlay
    return;

#if defined(CHIP_HAS_PSRAM) && defined(PSRAM_ENABLE)
    if (((uint32_t)__overlay_text_exec_start__ >= PSRAM_BASE &&
            (uint32_t)__overlay_text_exec_start__ < PSRAM_BASE + PSRAM_SIZE) ||
            ((uint32_t)__overlay_text_exec_start__ >= PSRAMX_BASE &&
            (uint32_t)__overlay_text_exec_start__ < PSRAMX_BASE + PSRAM_SIZE)) {
        hal_cache_invalidate(HAL_CACHE_ID_I_CACHE,
            (uint32_t)__overlay_text_exec_start__,
            (uint32_t)text_load_stop[id] - (uint32_t)text_load_start[id]);
    }

    if (((uint32_t)__overlay_data_start__ >= PSRAM_BASE &&
            (uint32_t)__overlay_data_start__ < PSRAM_BASE + PSRAM_SIZE) ||
            ((uint32_t)__overlay_data_start__ >= PSRAMX_BASE &&
            (uint32_t)__overlay_data_start__ < PSRAMX_BASE + PSRAM_SIZE)) {
        hal_cache_invalidate(HAL_CACHE_ID_D_CACHE,
            (uint32_t)__overlay_data_start__,
            (uint32_t)data_load_stop[id] - (uint32_t)data_load_start[id]);
    }
#endif
}

enum HAL_OVERLAY_RET_T hal_overlay_load(enum HAL_OVERLAY_ID_T id)
{
    enum HAL_OVERLAY_RET_T ret;

    CHECK_OVERLAY_ID(id);
    ret = HAL_OVERLAY_RET_OK;

    uint32_t lock;
    uint32_t *dst, *src;

    if (hal_overlay_is_used()) {
        ASSERT(0, "overlay %d is in use", id);
        return HAL_OVERLAY_RET_IN_USE;
    }

    hal_sysfreq_req(HAL_SYSFREQ_USER_OVERLAY, HAL_CMU_FREQ_52M);
    lock = int_lock();

    if (cur_overlay_id == HAL_OVERLAY_ID_QTY) {
        cur_overlay_id = HAL_OVERLAY_ID_IN_CFG;
    } else if (cur_overlay_id == HAL_OVERLAY_ID_IN_CFG) {
        ret = HAL_OVERLAY_RET_IN_CFG;
    } else {
        ret = HAL_OVERLAY_RET_IN_USE;
    }

    if (ret != HAL_OVERLAY_RET_OK) {
        goto _exit;
    }

    hal_overlay_acquire(id);

    for (dst = __overlay_text_start__, src = text_load_start[id];
            src < text_load_stop[id];
            dst++, src++) {
        *dst = *src;
    }

    for (dst = __overlay_data_start__, src = data_load_start[id];
            src < data_load_stop[id];
            dst++, src++) {
        *dst = *src;
    }

    cur_overlay_id = id;
    segment_state[id] = OVERLAY_IS_USED;

_exit:
    int_unlock(lock);
    hal_sysfreq_req(HAL_SYSFREQ_USER_OVERLAY, HAL_CMU_FREQ_32K);

    return ret;
}

enum HAL_OVERLAY_RET_T hal_overlay_unload(enum HAL_OVERLAY_ID_T id)
{
    enum HAL_OVERLAY_RET_T ret;

    CHECK_OVERLAY_ID(id);
    ret = HAL_OVERLAY_RET_OK;

    uint32_t lock;

    lock = int_lock();
    if (cur_overlay_id == id) {
        cur_overlay_id = HAL_OVERLAY_ID_QTY;
    } else if (cur_overlay_id == HAL_OVERLAY_ID_IN_CFG) {
        ret = HAL_OVERLAY_RET_IN_CFG;
    } else {
        ret = HAL_OVERLAY_RET_IN_USE;
    }
    hal_overlay_release(id);
    int_unlock(lock);

    return ret;
}

/*
 * get the overlay's text start address
 */
uint32_t hal_overlay_get_text_address(void)
{
    return (uint32_t)__overlay_text_start__;
}

/*
 * get the whole size of the overlay text
 */
uint32_t hal_overlay_get_text_all_size(void)
{
    uint32_t text_start, text_end;

    text_start = (uint32_t)__overlay_text_exec_start__;
    text_end = (uint32_t)__overlay_text_exec_end__;
    return text_end - text_start;
}

/*
 * get the segment size of one overlay text
 */
uint32_t hal_overlay_get_text_size(enum HAL_OVERLAY_ID_T id)
{
    CHECK_OVERLAY_ID(id);

    return (uint32_t)text_load_stop[id] - (uint32_t)text_load_start[id];
}

/*
 ____________ overlay's text end
 |          |
 |          |
 |  free    |
 |          |
 |__________|free_addr ______________________segment text end
 |          |                    |          |
 |          |                    |          |
 |          |                    |          |
 |          |                    |          |
 |          |                    |          |
 |__________| text's start_addr  |__________|segment text start_addr
 *
 */

/*
 * Use the free space of one segement, this function
 * return the free address of space
 */
uint32_t hal_overlay_get_text_free_addr(enum HAL_OVERLAY_ID_T id)
{
    uint32_t segment_text_sz;
    uint32_t start_addr;
    uint32_t free_addr;

    CHECK_OVERLAY_ID(id);
    segment_text_sz = hal_overlay_get_text_size(id);
    start_addr = hal_overlay_get_text_address();
    free_addr = start_addr + segment_text_sz;

    if (free_addr & 0x3 ) {
        ASSERT(0, "free addr %p is not aligned to 4", (void *)free_addr);
    }
    return free_addr;
}

/*
 * get the free size for one overlay text
 */
uint32_t hal_overlay_get_text_free_size(enum HAL_OVERLAY_ID_T id)
{
    uint32_t all_text_sz;
    uint32_t segment_text_sz;

    CHECK_OVERLAY_ID(id);
    all_text_sz = hal_overlay_get_text_all_size();
    segment_text_sz = hal_overlay_get_text_size(id);

    return all_text_sz - segment_text_sz;
}

/*
 * acquire one overlay segment
 */
void hal_overlay_acquire(enum HAL_OVERLAY_ID_T id)
{
    uint32_t lock;

    CHECK_OVERLAY_ID(id);
    segment_state[id] = OVERLAY_IS_USED;

    lock = int_lock();
    invalid_overlay_cache(id);
    int_unlock(lock);
    return;
}

/*
 * release one overlay segment
 */
void hal_overlay_release(enum HAL_OVERLAY_ID_T id)
{
    CHECK_OVERLAY_ID(id);
    segment_state[id] = OVERLAY_IS_FREE;
    return;
}

/*
 * check if any overlay segment is used
 */
bool hal_overlay_is_used(void)
{
    int i;
    uint32_t lock;

    lock = int_lock();
    for (i = 0; i < HAL_OVERLAY_ID_QTY; i++) {
        // if any segment is in use, the overlay is used;
        if (segment_state[i] == OVERLAY_IS_USED ) {
            int_unlock(lock);
            return true;
        }
    }
    int_unlock(lock);
    return false;
}

#endif

