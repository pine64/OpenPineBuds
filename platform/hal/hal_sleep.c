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
#include "hal_sleep.h"
#include "analog.h"
#include "cmsis.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_gpadc.h"
#include "hal_location.h"
#include "hal_norflash.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "pmu.h"

// static uint8_t SRAM_STACK_LOC sleep_stack[128];

static HAL_SLEEP_HOOK_HANDLER sleep_hook_handler[HAL_SLEEP_HOOK_USER_QTY];
static HAL_DEEP_SLEEP_HOOK_HANDLER
    deep_sleep_hook_handler[HAL_DEEP_SLEEP_HOOK_USER_QTY];

static uint32_t cpu_wake_lock_map;
static uint32_t sys_wake_lock_map;
static uint32_t chip_wake_lock_map;

#ifdef SLEEP_STATS_TRACE
static uint32_t stats_trace_interval;
static uint32_t stats_trace_time;
#endif
static uint32_t stats_interval;
static uint32_t stats_start_time;
static uint32_t light_sleep_time;
static uint32_t sys_deep_sleep_time;
static uint32_t chip_deep_sleep_time;
static bool stats_started;
static bool stats_valid;
static uint8_t light_sleep_ratio;
static uint8_t sys_deep_sleep_ratio;
static uint8_t chip_deep_sleep_ratio;

void hal_sleep_start_stats(uint32_t stats_interval_ms,
                           uint32_t trace_interval_ms) {
  uint32_t lock;

  lock = int_lock();
  if (stats_interval_ms) {
    stats_interval = MS_TO_TICKS(stats_interval_ms);
    stats_start_time = hal_sys_timer_get();
    light_sleep_time = 0;
    sys_deep_sleep_time = 0;
    chip_deep_sleep_time = 0;
    stats_valid = false;
    stats_started = true;
  } else {
    stats_started = false;
  }
  int_unlock(lock);

#ifdef SLEEP_STATS_TRACE
  if (stats_interval_ms && trace_interval_ms) {
    stats_trace_interval = MS_TO_TICKS(trace_interval_ms);
  } else {
    stats_trace_interval = 0;
  }
#endif
}

int hal_sleep_get_stats(struct CPU_USAGE_T *usage) {
  int ret;
  uint32_t lock;

  lock = int_lock();
  if (stats_valid) {
    usage->light_sleep = light_sleep_ratio;
    usage->sys_deep_sleep = sys_deep_sleep_ratio;
    usage->chip_deep_sleep = chip_deep_sleep_ratio;
    usage->busy = 100 - (light_sleep_ratio + sys_deep_sleep_ratio +
                         chip_deep_sleep_ratio);
    ret = 0;
  } else {
    ret = 1;
  }
  int_unlock(lock);

  return ret;
}

static int hal_sleep_cpu_busy(void) {
  if (cpu_wake_lock_map || hal_cmu_lpu_busy()) {
    return 1;
  } else {
    return 0;
  }
}

static int hal_sleep_sys_busy(void) {
  if (sys_wake_lock_map || hal_sysfreq_busy() || hal_dma_busy()) {
    return 1;
  } else {
    return 0;
  }
}

int hal_sleep_set_sleep_hook(enum HAL_SLEEP_HOOK_USER_T user,
                             HAL_SLEEP_HOOK_HANDLER handler) {
  if (user >= ARRAY_SIZE(sleep_hook_handler)) {
    return 1;
  }
  sleep_hook_handler[user] = handler;
  return 0;
}

static int SRAM_TEXT_LOC hal_sleep_exec_sleep_hook(void) {
  int i;
  int ret;

  for (i = 0; i < ARRAY_SIZE(sleep_hook_handler); i++) {
    if (sleep_hook_handler[i]) {
      ret = sleep_hook_handler[i]();
      if (ret) {
        return ret;
      }
    }
  }

  return 0;
}

int hal_sleep_set_deep_sleep_hook(enum HAL_DEEP_SLEEP_HOOK_USER_T user,
                                  HAL_DEEP_SLEEP_HOOK_HANDLER handler) {
  if (user >= ARRAY_SIZE(deep_sleep_hook_handler)) {
    return 1;
  }
  deep_sleep_hook_handler[user] = handler;
  return 0;
}

static int SRAM_TEXT_LOC hal_sleep_exec_deep_sleep_hook(void) {
  int i;
  int ret;

  for (i = 0; i < ARRAY_SIZE(deep_sleep_hook_handler); i++) {
    if (deep_sleep_hook_handler[i]) {
      ret = deep_sleep_hook_handler[i]();
      if (ret) {
        return ret;
      }
    }
  }

  return 0;
}

int SRAM_TEXT_LOC hal_sleep_irq_pending(void) {
#if defined(__GIC_PRESENT) && (__GIC_PRESENT)
  int i;

  for (i = 0; i < (USER_IRQn_QTY + 31) / 32; i++) {
    if (GICDistributor->ICPENDR[i] & GICDistributor->ISENABLER[i]) {
      return 1;
    }
  }
#else
#if 0
    int i;

    for (i = 0; i < (USER_IRQn_QTY + 31) / 32; i++) {
        if (NVIC->ICPR[i] & NVIC->ISER[i]) {
            return 1;
        }
    }
#else
  // If there is any pending and enabled exception (including sysTick)
  if (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) {
    return 1;
  }
#endif
#endif

  return 0;
}

int SRAM_TEXT_LOC hal_sleep_specific_irq_pending(const uint32_t *irq,
                                                 uint32_t cnt) {
  int i;
  uint32_t check_cnt;

  check_cnt = (USER_IRQn_QTY + 31) / 32;
  if (check_cnt > cnt) {
    check_cnt = cnt;
  }

#if defined(__GIC_PRESENT) && (__GIC_PRESENT)
  for (i = 0; i < check_cnt; i++) {
    if (GICDistributor->ICPENDR[i] & GICDistributor->ISENABLER[i] & irq[i]) {
      return 1;
    }
  }
#else
  for (i = 0; i < check_cnt; i++) {
    if (NVIC->ICPR[i] & NVIC->ISER[i] & irq[i]) {
      return 1;
    }
  }
#endif

  return 0;
}

void WEAK bt_drv_sleep(void) {}

void WEAK bt_drv_wakeup(void) {}

static enum HAL_SLEEP_STATUS_T SRAM_TEXT_LOC hal_sleep_lowpower_mode(void) {
  enum HAL_SLEEP_STATUS_T ret;
  enum HAL_CMU_LPU_SLEEP_MODE_T mode;
  uint32_t time = 0;

  ret = HAL_SLEEP_STATUS_LIGHT;

  // Deep sleep hook
  if (hal_sleep_exec_deep_sleep_hook() || hal_trace_busy()) {
    return ret;
  }

  if (stats_started) {
    time = hal_sys_timer_get();
  }

  // Modules (except for psram and flash) sleep
  hal_gpadc_sleep();
  if (chip_wake_lock_map == 0) {
    analog_sleep();
    pmu_sleep();
  }
  bt_drv_sleep();

  // End of sleep

  // psram_sleep();
  hal_norflash_sleep(HAL_NORFLASH_ID_0);

  // Now neither psram nor flash are usable

  if (!hal_sleep_irq_pending()) {
    if (chip_wake_lock_map) {
      mode = HAL_CMU_LPU_SLEEP_MODE_SYS;
    } else {
      mode = HAL_CMU_LPU_SLEEP_MODE_CHIP;
    }
    hal_cmu_lpu_sleep(mode);
    ret = HAL_SLEEP_STATUS_DEEP;
  }

  hal_norflash_wakeup(HAL_NORFLASH_ID_0);
  // psram_wakeup();

  // Now both psram and flash are usable

  if (chip_wake_lock_map == 0) {
    pmu_wakeup();
    analog_wakeup();
  }
  bt_drv_wakeup();

  hal_gpadc_wakeup();
  // Modules (except for psram and flash) wakeup

  // End of wakeup

  if (stats_started) {
    time = hal_sys_timer_get() - time;
    if (chip_wake_lock_map) {
      sys_deep_sleep_time += time;
    } else {
      chip_deep_sleep_time += time;
    }
  }

  return ret;
}

// GCC has trouble in detecting static function usage in embedded ASM
// statements. The following function might be optimized away if there is no
// explicted call in C codes. Specifying "used" (or "noclone") attribute on the
// function can avoid the mistaken optimization.
static enum HAL_SLEEP_STATUS_T SRAM_TEXT_LOC NOINLINE USED
hal_sleep_proc(int light_sleep) {
  enum HAL_SLEEP_STATUS_T ret;
  uint32_t time = 0;
  uint32_t interval;

  ret = HAL_SLEEP_STATUS_LIGHT;

  // Check the sleep conditions in interrupt-locked context
  if (hal_sleep_cpu_busy()) {
    // Cannot sleep
  } else {
    // Sleep hook
    if (hal_sleep_exec_sleep_hook()) {
      goto _exit_sleep;
    }

    if (hal_sleep_sys_busy()) {
      // Light sleep

      if (stats_started) {
        time = hal_sys_timer_get();
      }

#ifdef NO_LIGHT_SLEEP
      // WFI during USB ISO transfer might generate very weak (0.1 mV) 1K tone
      // interference ???
      while (!hal_sleep_irq_pending())
        ;
#else
#ifndef __ARM_ARCH_ISA_ARM
      SCB->SCR = 0;
#endif
      __DSB();
      __WFI();
#endif

      if (stats_started) {
        light_sleep_time += hal_sys_timer_get() - time;
      }
#ifdef DEBUG
    } else if (hal_trace_busy()) {
      // Light sleep with trace busy only

      if (stats_started) {
        time = hal_sys_timer_get();
      }

      // No irq will be generated when trace becomes idle, so the trace status
      // should be kept polling actively instead of entering WFI
      while (!hal_sleep_irq_pending() && hal_trace_busy())
        ;

      if (stats_started) {
        light_sleep_time += hal_sys_timer_get() - time;
      }

      if (!hal_sleep_irq_pending()) {
        goto _deep_sleep;
      }
#endif
    } else {
      // Deep sleep

    _deep_sleep:
      POSSIBLY_UNUSED;

      if (light_sleep) {
        ret = HAL_SLEEP_STATUS_DEEP;
      } else {
        ret = hal_sleep_lowpower_mode();
      }
    }
  }

_exit_sleep:
  if (stats_started) {
    time = hal_sys_timer_get();
    interval = time - stats_start_time;
    if (interval >= stats_interval) {
      if (light_sleep_time > UINT32_MAX / 100) {
        light_sleep_ratio = (uint64_t)light_sleep_time * 100 / interval;
      } else {
        light_sleep_ratio = light_sleep_time * 100 / interval;
      }
      if (sys_deep_sleep_time > UINT32_MAX / 100) {
        sys_deep_sleep_ratio = (uint64_t)sys_deep_sleep_time * 100 / interval;
      } else {
        sys_deep_sleep_ratio = sys_deep_sleep_time * 100 / interval;
      }
      if (chip_deep_sleep_time > UINT32_MAX / 100) {
        chip_deep_sleep_ratio = (uint64_t)chip_deep_sleep_time * 100 / interval;
      } else {
        chip_deep_sleep_ratio = chip_deep_sleep_time * 100 / interval;
      }
      stats_valid = true;
      light_sleep_time = 0;
      sys_deep_sleep_time = 0;
      chip_deep_sleep_time = 0;
      stats_start_time = time;
    }
#ifdef SLEEP_STATS_TRACE
    if (stats_valid && stats_trace_interval) {
      uint32_t time = hal_sys_timer_get();
      if (time - stats_trace_time >= stats_trace_interval) {
        TRACE(4, "CPU USAGE: busy=%d light=%d sys_deep=%d chip_deep=%d",
              100 - (light_sleep_ratio + sys_deep_sleep_ratio +
                     chip_deep_sleep_ratio),
              light_sleep_ratio, sys_deep_sleep_ratio, chip_deep_sleep_ratio);
        stats_trace_time = time;
#ifdef DEBUG_SLEEP_USER
        TRACE(4, "SLEEP_USER: cpulock=0x%X syslock=0x%X irq=0x%08X_%08X",
              cpu_wake_lock_map, sys_wake_lock_map,
              (NVIC->ICPR[1] & NVIC->ISER[1]), (NVIC->ICPR[0] & NVIC->ISER[0]));
        hal_sysfreq_print();
#endif
      }
    }
#endif
  }

  return ret;
}

#ifndef __ARM_ARCH_ISA_ARM
static enum HAL_SLEEP_STATUS_T SRAM_TEXT_LOC NOINLINE USED NAKED
hal_sleep_deep_sleep_wrapper(void) {
  asm volatile("push {r4, lr} \n"
               // Switch current stack pointer to MSP
               "mrs r4, control \n"
               "bic r4, #2 \n"
               "msr control, r4 \n"
               "isb \n"
               "movs r0, #0 \n"
               "bl hal_sleep_proc \n"
               // Switch current stack pointer back to PSP
               "orr r4, #2 \n"
               "msr control, r4 \n"
               "isb \n"
               "pop {r4, pc} \n");

#ifndef __ARMCC_VERSION
  return HAL_SLEEP_STATUS_LIGHT;
#endif
}
#endif

enum HAL_SLEEP_STATUS_T SRAM_TEXT_LOC hal_sleep_enter_sleep(void) {
  enum HAL_SLEEP_STATUS_T ret;
  uint32_t lock;

  ret = HAL_SLEEP_STATUS_LIGHT;

#ifdef NO_SLEEP
  return ret;
#endif

  lock = int_lock_global();

#ifndef __ARM_ARCH_ISA_ARM
  if (__get_CONTROL() & 0x02) {
    ret = hal_sleep_deep_sleep_wrapper();
  } else
#endif
  {
    ret = hal_sleep_proc(false);
  }

  int_unlock_global(lock);

  return ret;
}

enum HAL_SLEEP_STATUS_T SRAM_TEXT_LOC hal_sleep_light_sleep(void) {
  enum HAL_SLEEP_STATUS_T ret;
  uint32_t lock;

  ret = HAL_SLEEP_STATUS_LIGHT;

#ifdef NO_SLEEP
  return ret;
#endif

  lock = int_lock_global();

  ret = hal_sleep_proc(true);

  int_unlock_global(lock);

  return ret;
}

int hal_cpu_wake_lock(enum HAL_CPU_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_CPU_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  cpu_wake_lock_map |= (1 << user);
  int_unlock(lock);

  return 0;
}

int hal_cpu_wake_unlock(enum HAL_CPU_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_CPU_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  cpu_wake_lock_map &= ~(1 << user);
  int_unlock(lock);

  return 0;
}

int hal_sys_wake_lock(enum HAL_SYS_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_SYS_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  sys_wake_lock_map |= (1 << user);
  int_unlock(lock);

  return 0;
}

int hal_sys_wake_unlock(enum HAL_SYS_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_SYS_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  sys_wake_lock_map &= ~(1 << user);
  int_unlock(lock);

  return 0;
}

int hal_chip_wake_lock(enum HAL_CHIP_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_CHIP_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  chip_wake_lock_map |= (1 << user);
  int_unlock(lock);

  return 0;
}

int hal_chip_wake_unlock(enum HAL_CHIP_WAKE_LOCK_USER_T user) {
  uint32_t lock;

  if (user >= HAL_CHIP_WAKE_LOCK_USER_QTY) {
    return 1;
  }

  lock = int_lock();
  chip_wake_lock_map &= ~(1 << user);
  int_unlock(lock);

  return 0;
}
