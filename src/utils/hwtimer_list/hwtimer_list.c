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
#include "hwtimer_list.h"
#include "plat_types.h"
#define IGNORE_HAL_TIMER_RAW_API_CHECK
#include "cmsis.h"
#include "hal_timer_raw.h"
#include "hal_trace.h"
#include "stdio.h"

#if defined(ROM_BUILD) && !defined(SIMU)
#error                                                                         \
    "The user of raw timer API must be unique. Now rom is using raw timer API."
#endif

#define CHECK_ACTIVE_NULL_IN_IRQ 1

#ifndef HWTIMER_NUM
#define HWTIMER_NUM 10
#endif

// #define HWTIMER_TEST

enum HWTIMER_STATE_T {
  HWTIMER_STATE_FREE = 0,
  HWTIMER_STATE_ALLOC,
  HWTIMER_STATE_ACTIVE,
  HWTIMER_STATE_FIRED,
  HWTIMER_STATE_CALLBACK,

  HWTIMER_STATE_QTY
};

struct HWTIMER_T {
  enum HWTIMER_STATE_T state;
  struct HWTIMER_T *next;
  uint32_t time;
  HWTIMER_CALLBACK_T callback;
  void *param;
};

struct HWTIMER_LIST_T {
  struct HWTIMER_T timer[HWTIMER_NUM];
  struct HWTIMER_T *free;
  struct HWTIMER_T *active;
  struct HWTIMER_T *fired;
};

static struct HWTIMER_LIST_T hwtimer_list;
static uint32_t err_irq_active_null = 0;
static uint32_t err_irq_early = 0;

static void hwtimer_handler(uint32_t elapsed) {
  struct HWTIMER_T *pre;
  struct HWTIMER_T *last;
  uint32_t lock = 0;

  lock = int_lock();

  if (hwtimer_list.active == NULL) {
    err_irq_active_null++;
    TRACE(1, "HWTIMER irq when active is null: might be deleted? %u",
          err_irq_active_null);
#if (CHECK_ACTIVE_NULL_IN_IRQ)
    ASSERT(hal_timer_is_enabled() == 0,
           "HWTIMER collapsed: irq when active is null");
#endif
    goto _exit;
  }
  // Update elapsed time
  elapsed = hal_timer_get_elapsed_time();
  if (hwtimer_list.active->time > elapsed + HAL_TIMER_LOAD_DELTA) {
    err_irq_early++;
    TRACE(1,
          "HWTIMER irq occurred early: old active timer might be deleted? %u",
          err_irq_early);
    ASSERT(hal_timer_is_enabled(), "HWTIMER collapsed: irq occurred too early");
    goto _exit;
  }

  if (elapsed > hwtimer_list.active->time) {
    elapsed -= hwtimer_list.active->time;
  } else {
    elapsed = 0;
  }
  pre = hwtimer_list.active;
  // TODO: Check state here ?
  pre->state = HWTIMER_STATE_FIRED;
  last = hwtimer_list.active->next;
  while (last && last->time <= elapsed) {
    elapsed -= last->time;
    pre = last;
    // TODO: Check state here ?
    pre->state = HWTIMER_STATE_FIRED;
    last = last->next;
  }
  pre->next = NULL;
  hwtimer_list.fired = hwtimer_list.active;
  hwtimer_list.active = last;
  if (last) {
    last->time -= elapsed;
    hal_timer_start(last->time);
#if (CHECK_ACTIVE_NULL_IN_IRQ)
  } else {
    hal_timer_stop();
#endif
  }

  while (hwtimer_list.fired) {
    last = hwtimer_list.fired;
    hwtimer_list.fired = last->next;
    // TODO: Check state here ?
    last->state = HWTIMER_STATE_CALLBACK;
    last->next = NULL;
    // Now this timer can be restarted, but not stopped or freed
    if (last->callback) {
      int_unlock(lock);
      last->callback(last->param);
      lock = int_lock();
    }
    if (last->state == HWTIMER_STATE_CALLBACK) {
      last->state = HWTIMER_STATE_ALLOC;
    }
  }

_exit:
  int_unlock(lock);
}

void hwtimer_init(void) {
  int i;

  for (i = 0; i < HWTIMER_NUM - 1; i++) {
    hwtimer_list.timer[i].state = HWTIMER_STATE_FREE;
    hwtimer_list.timer[i].next = &hwtimer_list.timer[i + 1];
  }
  hwtimer_list.timer[HWTIMER_NUM - 1].next = NULL;
  hwtimer_list.free = &hwtimer_list.timer[0];
  hwtimer_list.active = NULL;
  hwtimer_list.fired = NULL;
  hal_timer_setup(HAL_TIMER_TYPE_ONESHOT, hwtimer_handler);
}

HWTIMER_ID hwtimer_alloc(HWTIMER_CALLBACK_T callback, void *param) {
  struct HWTIMER_T *timer;
  uint32_t lock;

  timer = NULL;

  lock = int_lock();
  if (hwtimer_list.free != NULL) {
    timer = hwtimer_list.free;
    hwtimer_list.free = hwtimer_list.free->next;
  }
  int_unlock(lock);

  if (timer == NULL) {
    return NULL;
  }

  ASSERT(timer->state == HWTIMER_STATE_FREE, "HWTIMER-ALLOC: Invalid state: %d",
         timer->state);
  timer->state = HWTIMER_STATE_ALLOC;
  timer->callback = callback;
  timer->param = param;
  timer->next = NULL;

  return timer;
}

enum E_HWTIMER_T hwtimer_free(HWTIMER_ID id) {
  enum E_HWTIMER_T ret;
  struct HWTIMER_T *timer;
  uint32_t lock;

  timer = (struct HWTIMER_T *)id;

  if (timer < &hwtimer_list.timer[0] ||
      timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
    return E_HWTIMER_INVAL_ID;
  }

  ret = E_HWTIMER_OK;

  lock = int_lock();

  if (timer->state != HWTIMER_STATE_ALLOC) {
    ret = E_HWTIMER_INVAL_ST;
    goto _exit;
  }
  timer->state = HWTIMER_STATE_FREE;

  timer->next = hwtimer_list.free;
  hwtimer_list.free = timer;

_exit:
  int_unlock(lock);

  return ret;
}

static enum E_HWTIMER_T __hwtimer_start_int(HWTIMER_ID id, int update,
                                            HWTIMER_CALLBACK_T callback,
                                            void *param, unsigned int ticks) {
  enum E_HWTIMER_T ret;
  struct HWTIMER_T *timer;
  struct HWTIMER_T *pre;
  struct HWTIMER_T *next;
  uint32_t lock;
  uint32_t cur_time;

  timer = (struct HWTIMER_T *)id;

  if (timer < &hwtimer_list.timer[0] ||
      timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
    return E_HWTIMER_INVAL_ID;
  }

  if (ticks < HAL_TIMER_LOAD_DELTA) {
    ticks = HAL_TIMER_LOAD_DELTA;
  }

  ret = E_HWTIMER_OK;

  lock = int_lock();

  if (timer->state != HWTIMER_STATE_ALLOC &&
      timer->state != HWTIMER_STATE_CALLBACK) {
    ret = E_HWTIMER_INVAL_ST;
    goto _exit;
  }
  timer->state = HWTIMER_STATE_ACTIVE;

  if (update) {
    timer->callback = callback;
    timer->param = param;
  }

  if (hwtimer_list.active == NULL) {
    timer->next = NULL;
    hwtimer_list.active = timer;
    hal_timer_start(ticks);
  } else {
    cur_time = hal_timer_get();
    ASSERT(cur_time <= hwtimer_list.active->time ||
               cur_time <= HAL_TIMER_LOAD_DELTA,
           "HWTIMER-START collapsed: cur=%u active=%u", cur_time,
           hwtimer_list.active->time);
    if (cur_time > ticks) {
      hwtimer_list.active->time = cur_time - ticks;
      timer->next = hwtimer_list.active;
      hwtimer_list.active = timer;
      hal_timer_stop();
      hal_timer_start(ticks);
    } else {
      pre = hwtimer_list.active;
      next = hwtimer_list.active->next;
      ticks -= cur_time;
      while (next && next->time < ticks) {
        ticks -= next->time;
        pre = next;
        next = next->next;
      }
      if (next) {
        next->time -= ticks;
      }
      pre->next = timer;
      timer->next = next;
    }
  }
  timer->time = ticks;

_exit:
  int_unlock(lock);

  return ret;
}

enum E_HWTIMER_T hwtimer_start(HWTIMER_ID id, unsigned int ticks) {
  return __hwtimer_start_int(id, 0, NULL, NULL, ticks);
}

enum E_HWTIMER_T hwtimer_update_then_start(HWTIMER_ID id,
                                           HWTIMER_CALLBACK_T callback,
                                           void *param, unsigned int ticks) {
  return __hwtimer_start_int(id, 1, callback, param, ticks);
}

enum E_HWTIMER_T hwtimer_update(HWTIMER_ID id, HWTIMER_CALLBACK_T callback,
                                void *param) {
  enum E_HWTIMER_T ret;
  struct HWTIMER_T *timer;
  uint32_t lock;

  timer = (struct HWTIMER_T *)id;

  if (timer < &hwtimer_list.timer[0] ||
      timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
    return E_HWTIMER_INVAL_ID;
  }

  ret = E_HWTIMER_OK;

  lock = int_lock();

  if (timer->state == HWTIMER_STATE_ALLOC ||
      timer->state == HWTIMER_STATE_CALLBACK) {
    timer->callback = callback;
    timer->param = param;
  } else {
    ret = E_HWTIMER_INVAL_ST;
  }

  int_unlock(lock);

  return ret;
}

enum E_HWTIMER_T hwtimer_stop(HWTIMER_ID id) {
  enum E_HWTIMER_T ret;
  struct HWTIMER_T *timer;
  struct HWTIMER_T *pre;
  struct HWTIMER_T *next;
  uint32_t cur_time;
  uint32_t elapsed_time;
  uint32_t lock;

  timer = (struct HWTIMER_T *)id;

  if (timer < &hwtimer_list.timer[0] ||
      timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
    return E_HWTIMER_INVAL_ID;
  }

  ret = E_HWTIMER_OK;

  lock = int_lock();

  if (timer->state == HWTIMER_STATE_ALLOC) {
    // Already stopped
    goto _exit;
  } else if (timer->state == HWTIMER_STATE_ACTIVE) {
    // Active timer
    if (hwtimer_list.active == timer) {
      cur_time = hal_timer_get();
      ASSERT(cur_time <= hwtimer_list.active->time ||
                 cur_time <= HAL_TIMER_LOAD_DELTA,
             "HWTIMER-STOP collapsed: cur=%u active=%u", cur_time,
             hwtimer_list.active->time);
      hal_timer_stop();
      next = hwtimer_list.active->next;
      if (next) {
        if (cur_time == 0) {
          elapsed_time = hal_timer_get_elapsed_time();
          ASSERT(elapsed_time + HAL_TIMER_LOAD_DELTA >=
                     hwtimer_list.active->time,
                 "HWTIMER-STOP collapsed: elapsed=%u active=%u", elapsed_time,
                 hwtimer_list.active->time);
          if (elapsed_time > hwtimer_list.active->time) {
            elapsed_time -= hwtimer_list.active->time;
          } else {
            elapsed_time = 0;
          }
          pre = next;
          while (pre && pre->time <= elapsed_time) {
            elapsed_time -= pre->time;
            pre->time = 0;
            pre = pre->next;
          }
          if (pre) {
            pre->time -= elapsed_time;
          }
        } else {
          next->time += cur_time;
        }
        hal_timer_start(next->time);
      }
      hwtimer_list.active = next;
    } else if (hwtimer_list.active) {
      pre = hwtimer_list.active;
      next = hwtimer_list.active->next;
      while (next && next != timer) {
        pre = next;
        next = next->next;
      }
      if (next == timer) {
        pre->next = next->next;
        if (next->next) {
          next->next->time += timer->time;
        }
      } else {
        ret = E_HWTIMER_NOT_FOUND;
      }
    } else {
      ret = E_HWTIMER_NOT_FOUND;
    }
    ASSERT(ret == E_HWTIMER_OK,
           "HWTIMER-STOP collapsed: active timer 0x%08x not in list 0x%08x",
           (uint32_t)timer, (uint32_t)hwtimer_list.active);
  } else if (timer->state == HWTIMER_STATE_FIRED) {
    // Fired timer
    if (hwtimer_list.fired == timer) {
      // The timer handler is preempted
      hwtimer_list.fired = hwtimer_list.fired->next;
    } else if (hwtimer_list.fired) {
      pre = hwtimer_list.fired;
      next = hwtimer_list.fired->next;
      while (next && next != timer) {
        pre = next;
        next = next->next;
      }
      if (next == timer) {
        pre->next = next->next;
      } else {
        ret = E_HWTIMER_NOT_FOUND;
      }
    } else {
      ret = E_HWTIMER_NOT_FOUND;
    }
    ASSERT(ret == E_HWTIMER_OK,
           "HWTIMER-STOP collapsed: fired timer 0x%08x not in list 0x%08x",
           (uint32_t)timer, (uint32_t)hwtimer_list.fired);
  } else if (timer->state == HWTIMER_STATE_CALLBACK) {
    // The timer handler is preempted and timer is being handled
    ret = E_HWTIMER_IN_CALLBACK;
  } else {
    // Invalid state
    ret = E_HWTIMER_INVAL_ST;
  }

  if (ret == E_HWTIMER_OK) {
    timer->state = HWTIMER_STATE_ALLOC;
    timer->next = NULL;
  }

_exit:
  int_unlock(lock);

  return ret;
}

#ifdef HWTIMER_TEST

int hwtimer_get_index(HWTIMER_ID id) {
  struct HWTIMER_T *timer;
  int i;

  timer = (struct HWTIMER_T *)id;

  if (timer < &hwtimer_list.timer[0] ||
      timer > &hwtimer_list.timer[HWTIMER_NUM - 1]) {
    return -1;
  }

  for (i = 0; i < HWTIMER_NUM; i++) {
    if (timer == &hwtimer_list.timer[i]) {
      return i;
    }
  }

  return -1;
}

void hwtimer_dump(void) {
  int i;
  int idx;
  bool checked[HWTIMER_NUM];
  char buf[100], *pos;
  const char *end = buf + sizeof(buf);
  struct HWTIMER_T *timer;
  uint32_t lock;
  enum HWTIMER_STATE_T state;

  for (i = 0; i < HWTIMER_NUM; i++) {
    checked[i] = false;
  }

  TRACE(0, "------\nHWTIMER LIST DUMP");
  lock = int_lock();
  for (i = 0; i < 3; i++) {
    pos = buf;
    if (i == 0) {
      pos += snprintf(pos, end - pos, "ACTIVE: ");
      timer = hwtimer_list.active;
      state = HWTIMER_STATE_ACTIVE;
    } else if (i == 1) {
      pos += snprintf(pos, end - pos, "FIRED : ");
      timer = hwtimer_list.fired;
      state = HWTIMER_STATE_FIRED;
    } else {
      pos += snprintf(pos, end - pos, "FREE  : ");
      timer = hwtimer_list.free;
      state = HWTIMER_STATE_FREE;
    }
    while (timer) {
      idx = hwtimer_get_index(timer);
      if (idx == -1) {
        pos += snprintf(pos, end - pos, "<NA: %p>", timer);
        break;
      } else if (checked[idx]) {
        pos += snprintf(pos, end - pos, "<DUP: %d>", idx);
        break;
      } else if (timer->state != state) {
        pos += snprintf(pos, end - pos, "<ST-%d: %d> ", timer->state, idx);
      } else if (state == HWTIMER_STATE_ACTIVE) {
        pos += snprintf(pos, end - pos, "%d-%u ", idx, timer->time);
      } else {
        pos += snprintf(pos, end - pos, "%d ", idx);
      }
      checked[idx] = true;
      timer = timer->next;
    }
    TRACE(buf);
  }
  int_unlock(lock);
  pos = buf;
  pos += snprintf(pos, end - pos, "ALLOC : ");
  for (i = 0; i < HWTIMER_NUM; i++) {
    if (!checked[i]) {
      if (hwtimer_list.timer[i].state == HWTIMER_STATE_ALLOC) {
        pos += snprintf(pos, end - pos, "%d ", i);
      } else {
        pos += snprintf(pos, end - pos, "<ST-%d: %d> ",
                        hwtimer_list.timer[i].state, i);
      }
    }
  }
  pos += snprintf(pos, end - pos, "\n------");
  TRACE(buf);
}

#define HWTIMER_TEST_NUM (HWTIMER_NUM + 2)

static HWTIMER_ID test_id[HWTIMER_TEST_NUM];

static void timer_stop_test(int id) {
  int ret;

  ret = hwtimer_stop(test_id[id]);
  TRACE(3, "[%u] Stop %d / %d", hal_sys_timer_get(), id, ret);
  hwtimer_dump();
}

static void timer_callback(void *param) {
  int id;

  id = (int)param;

  TRACE(2, "[%u] TIMER-CALLBACK: %d", hal_sys_timer_get(), id);

  if (id == 3) {
    timer_stop_test(3);
    timer_stop_test(5);
    timer_stop_test(7);
  }
}

void hwtimer_test(void) {
  int i;
  int ret;
  uint32_t lock;

  hwtimer_init();

  for (i = 0; i < HWTIMER_TEST_NUM; i++) {
    test_id[i] = hwtimer_alloc(timer_callback, (void *)i);
    ret = hwtimer_start(test_id[i], (i + 1) * 10);
    TRACE(4, "[%u] START-TIMER: %u / %p / %d", hal_sys_timer_get(), i,
          test_id[i], ret);
  }

  hwtimer_dump();

  lock = int_lock();
  hal_sys_timer_delay(55);
  timer_stop_test(0);
  timer_stop_test(2);
  int_unlock(lock);

  hal_sys_timer_delay(300);
  hwtimer_dump();
}

#endif
