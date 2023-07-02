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
#include "hal_timer.h"
#include "plat_addr_map.h"
#define IGNORE_HAL_TIMER_RAW_API_CHECK
#include "cmsis_nvic.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_timer_raw.h"
#include "reg_timer.h"

//#define ELAPSED_TIMER_ENABLED

#if defined(CHIP_BEST3001) || defined(CHIP_BEST3003) ||                        \
    defined(CHIP_BEST3005) || defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
#define CLOCK_SYNC_WORKAROUND
#endif

#ifdef LOW_SYS_FREQ
#if defined(CHIP_BEST1305) || defined(CHIP_BEST1501) ||                        \
    defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) ||                       \
    defined(CHIP_BEST2300A)
#define FAST_TIMER_WORKAROUND
#endif
#endif

#if defined(__FPU_USED) && (__FPU_USED == 1)
//#define TIMER_USE_FPU
#endif

#define SLOW_TIMER_VAL_DELTA 1
#define SLOW_TIMER_VAL_DELTA_SLEEP 10
#define FAST_TIMER_VAL_DELTA 20

#ifdef CALIB_SLOW_TIMER
#define MAX_CALIB_SYSTICK_HZ (CONFIG_SYSTICK_HZ_NOMINAL * 2)

#define MIN_CALIB_TICKS (10 * (CONFIG_SYSTICK_HZ_NOMINAL / 1000))

#define MAX_CALIB_TICKS (30 * CONFIG_SYSTICK_HZ_NOMINAL)

static uint32_t BOOT_DATA_LOC sys_tick_hz = CONFIG_SYSTICK_HZ_NOMINAL;
static uint32_t BOOT_BSS_LOC slow_val;
static uint32_t BOOT_BSS_LOC fast_val;
#endif

static struct DUAL_TIMER_T *const BOOT_RODATA_SRAM_LOC dual_timer0 =
    (struct DUAL_TIMER_T *)TIMER0_BASE;
#ifdef TIMER1_BASE
static struct DUAL_TIMER_T *const BOOT_RODATA_SRAM_LOC dual_timer1 =
    (struct DUAL_TIMER_T *)TIMER1_BASE;
#endif

static HAL_TIMER_IRQ_HANDLER_T irq_handler = NULL;

// static uint32_t load_value = 0;
static uint32_t start_time;

static void POSSIBLY_UNUSED hal_timer00_irq_handler(void);
static void hal_timer01_irq_handler(void);

__STATIC_FORCEINLINE uint32_t get_timer_value(struct TIMER_T *timer,
                                              uint32_t delta) {
#ifdef CLOCK_SYNC_WORKAROUND
  uint32_t lock;
  uint32_t v1, v2;

  lock = int_lock();
  do {
    v1 = timer->Value;
    v2 = timer->Value;
  } while ((v1 < v2) || (v1 > v2 + delta));
  int_unlock(lock);

  return v2;
#else
  return timer->Value;
#endif
}

__STATIC_FORCEINLINE void clear_timer_irq(struct TIMER_T *timer) {
#ifdef CLOCK_SYNC_WORKAROUND
  do {
    timer->IntClr = 1;
  } while (timer->RIS & TIMER_RIS_RIS);
#else
  timer->IntClr = 1;
#endif
}

__STATIC_FORCEINLINE void set_timer_load(struct TIMER_T *timer, uint32_t load,
                                         uint32_t delta) {
#ifdef CLOCK_SYNC_WORKAROUND
  uint32_t lock;
  uint32_t val;

  lock = int_lock();
  do {
    timer->Load = load;
    val = timer->Value;
  } while ((load < val) || (load > val + delta));
  int_unlock(lock);
#else
  timer->Load = load;
#endif
}

__STATIC_FORCEINLINE void fast_timer_open(void) {
#ifdef TIMER1_BASE
  hal_cmu_timer1_select_fast();
  dual_timer1->timer[0].Control =
      TIMER_CTRL_EN | TIMER_CTRL_PRESCALE_DIV_1 | TIMER_CTRL_SIZE_32_BIT;
#endif
}

void BOOT_TEXT_FLASH_LOC hal_sys_timer_open(void) {
  hal_cmu_timer0_select_slow();
  dual_timer0->timer[0].Control =
      TIMER_CTRL_EN | TIMER_CTRL_PRESCALE_DIV_1 | TIMER_CTRL_SIZE_32_BIT;
  fast_timer_open();
  ;
}

#ifdef CORE_SLEEP_POWER_DOWN
void SRAM_TEXT_LOC hal_sys_timer_wakeup(void) {
  fast_timer_open();
  ;
}
#endif

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_get(void) {
  return -get_timer_value(&dual_timer0->timer[0], SLOW_TIMER_VAL_DELTA);
}

#ifdef CLOCK_SYNC_WORKAROUND
uint32_t SRAM_TEXT_LOC hal_sys_timer_get_in_sleep(void) {
  return -get_timer_value(&dual_timer0->timer[0], SLOW_TIMER_VAL_DELTA_SLEEP);
}
#else
uint32_t hal_sys_timer_get_in_sleep(void)
    __attribute__((alias("hal_sys_timer_get")));
#endif

uint32_t BOOT_TEXT_FLASH_LOC flash_hal_sys_timer_get(void) {
  return -get_timer_value(&dual_timer0->timer[0], SLOW_TIMER_VAL_DELTA);
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_ms_get(void) { return GET_CURRENT_MS(); }

uint32_t BOOT_TEXT_SRAM_LOC hal_fast_sys_timer_get(void) {
#ifdef TIMER1_BASE
#ifdef FAST_TIMER_WORKAROUND
  if (hal_cmu_fast_timer_offline()) {
#ifdef TIMER_USE_FPU
    return (uint32_t)(hal_sys_timer_get() *
                      ((float)CONFIG_FAST_SYSTICK_HZ / CONFIG_SYSTICK_HZ));
#else
    return (uint32_t)(hal_sys_timer_get() * (uint64_t)CONFIG_FAST_SYSTICK_HZ /
                      CONFIG_SYSTICK_HZ);
#endif
  }
#endif // FAST_TIMER_WORKAROUND
  return -get_timer_value(&dual_timer1->timer[0], FAST_TIMER_VAL_DELTA);
#else
  return 0;
#endif
}

uint32_t hal_sys_timer_get_max(void) { return 0xFFFFFFFF; }

void BOOT_TEXT_SRAM_LOC hal_sys_timer_delay(uint32_t ticks) {
  uint32_t start = hal_sys_timer_get();

  while (hal_sys_timer_get() - start < ticks)
    ;
}

#ifdef CLOCK_SYNC_WORKAROUND
void SRAM_TEXT_LOC hal_sys_timer_delay_in_sleep(uint32_t ticks) {
  uint32_t start = hal_sys_timer_get_in_sleep();

  while (hal_sys_timer_get_in_sleep() - start < ticks)
    ;
}
#else
void hal_sys_timer_delay_in_sleep(uint32_t ticks)
    __attribute__((alias("hal_sys_timer_delay")));
#endif

void BOOT_TEXT_FLASH_LOC flash_hal_sys_timer_delay(uint32_t ticks) {
  uint32_t start = flash_hal_sys_timer_get();

  while (flash_hal_sys_timer_get() - start < ticks)
    ;
}

void BOOT_TEXT_SRAM_LOC hal_sys_timer_delay_us(uint32_t us) {
#ifdef TIMER1_BASE
#ifdef FAST_TIMER_WORKAROUND
  if (hal_cmu_fast_timer_offline()) {
    uint32_t start = hal_sys_timer_get();
    uint32_t ticks = US_TO_TICKS(us);

    while (hal_sys_timer_get() - start < ticks)
      ;
  }
#endif // FAST_TIMER_WORKAROUND

  uint32_t start = hal_fast_sys_timer_get();
  uint32_t ticks = US_TO_FAST_TICKS(us);

  while (hal_fast_sys_timer_get() - start < ticks)
    ;
#else
  enum HAL_CMU_FREQ_T freq = hal_cmu_sys_get_freq();
  uint32_t loop;
  uint32_t i;

  // Assuming:
  // 1) system clock uses audio PLL
  // 2) audio PLL is configured as 48K series, 196.608M
  // 3) crystal is 26M

  if (freq == HAL_CMU_FREQ_208M) {
    loop = 197;
  } else if (freq == HAL_CMU_FREQ_104M) {
    loop = 197 / 2;
  } else if (freq == HAL_CMU_FREQ_78M) {
    loop = 197 / 3;
  } else if (freq == HAL_CMU_FREQ_52M) {
    loop = 52;
  } else {
    loop = 26;
  }

  loop = loop * us / 5;
  for (i = 0; i < loop; i++) {
    asm volatile("nop");
  }
#endif
}

void SRAM_TEXT_LOC hal_sys_timer_delay_ns(uint32_t ns) {
#ifdef TIMER1_BASE
#ifdef FAST_TIMER_WORKAROUND
  if (hal_cmu_fast_timer_offline()) {
    uint32_t start = hal_sys_timer_get();
    uint32_t ticks = US_TO_TICKS((ns + (1000 - 1)) / 1000);

    while (hal_sys_timer_get() - start < ticks)
      ;
  }
#endif // FAST_TIMER_WORKAROUND

  uint32_t start = hal_fast_sys_timer_get();
  uint32_t ticks = NS_TO_FAST_TICKS(ns);

  while (hal_fast_sys_timer_get() - start < ticks)
    ;
#else
  enum HAL_CMU_FREQ_T freq = hal_cmu_sys_get_freq();
  uint32_t loop;
  uint32_t i;

  // Assuming:
  // 1) system clock uses audio PLL
  // 2) audio PLL is configured as 48K series, 196.608M
  // 3) crystal is 26M

  if (freq == HAL_CMU_FREQ_208M) {
    loop = 197;
  } else if (freq == HAL_CMU_FREQ_104M) {
    loop = 197 / 2;
  } else if (freq == HAL_CMU_FREQ_78M) {
    loop = 197 / 3;
  } else if (freq == HAL_CMU_FREQ_52M) {
    loop = 52;
  } else {
    loop = 26;
  }

  loop = loop * ns / 5000;
  for (i = 0; i < loop; i++) {
    asm volatile("nop");
  }
#endif
}

static uint32_t NOINLINE
SRAM_TEXT_DEF(measure_cpu_freq_interval)(uint32_t cnt) {
  uint32_t start;
  struct DUAL_TIMER_T *t;
  uint32_t delta;

#ifdef TIMER1_BASE
  t = dual_timer1;
  delta = FAST_TIMER_VAL_DELTA;
#ifdef FAST_TIMER_WORKAROUND
  if (hal_cmu_fast_timer_offline()) {
    t = dual_timer0;
    delta = SLOW_TIMER_VAL_DELTA;
  }
#endif // FAST_TIMER_WORKAROUND
#else
  t = dual_timer0;
  delta = SLOW_TIMER_VAL_DELTA;
#endif

  start = get_timer_value(&t->timer[0], delta);

  asm volatile("_loop:;"
#ifdef __ARM_ARCH_ISA_ARM
               "nop;"
               "nop;"
#endif
               "subs %0, #1;"
               "cmp %0, #0;"
               "bne _loop;"
               :
               : "r"(cnt));

  return start - get_timer_value(&t->timer[0], delta);
}

uint32_t hal_sys_timer_calc_cpu_freq(uint32_t interval_ms, int high_res) {
  uint32_t ref_freq;
  uint32_t cnt;
  uint32_t one_sec;
  uint32_t lock;
  uint32_t run_interval;
  uint32_t base_interval;
  uint32_t freq;

  // Default measurement interval
  if (interval_ms == 0) {
#ifdef TIMER1_BASE
    interval_ms = 10;
#else
    interval_ms = 100;
#endif
  }

  ref_freq = hal_cmu_get_crystal_freq();
  // CPU loop cycle count
  cnt = ref_freq / 4 * interval_ms / 1000;

  // Timer ticks per second
#ifdef TIMER1_BASE
  one_sec = CONFIG_FAST_SYSTICK_HZ;
#ifdef FAST_TIMER_WORKAROUND
  if (hal_cmu_fast_timer_offline()) {
    one_sec = CONFIG_SYSTICK_HZ;
  }
#endif // FAST_TIMER_WORKAROUND
#else
  if (high_res) {
    one_sec = CONFIG_FAST_SYSTICK_HZ;
  } else {
    one_sec = CONFIG_SYSTICK_HZ;
  }
#endif
  // Timer ticks per measurement interval
  base_interval = one_sec * interval_ms / 1000;

  lock = int_lock();

#ifndef TIMER1_BASE
  if (high_res) {
    hal_cmu_timer0_select_fast();
  }
#endif

  run_interval = measure_cpu_freq_interval(cnt);

#ifndef TIMER1_BASE
  if (high_res) {
    hal_cmu_timer0_select_slow();
  }
#endif

  int_unlock(lock);

#ifdef TIMER_USE_FPU
  freq = (uint32_t)((float)ref_freq / run_interval * base_interval);
#else
  freq = (uint32_t)((uint64_t)ref_freq * base_interval / run_interval);
#endif

  if (high_res == 0) {
    freq = (freq + 500000) / 1000000 * 1000000;
  }

  return freq;
}

#ifdef CALIB_SLOW_TIMER
void hal_sys_timer_calib_start(void) {
  uint32_t lock;
  uint32_t slow;
  uint32_t fast;

  lock = int_lock();
  slow = hal_sys_timer_get();
  while (hal_sys_timer_get() == slow)
    ;
  fast = hal_fast_sys_timer_get();
  int_unlock(lock);

  slow_val = slow + 1;
  fast_val = fast;
}

int hal_sys_timer_calib_end(void) {
  uint32_t lock;
  uint32_t slow;
  uint32_t fast;
  uint32_t slow_diff;

  lock = int_lock();
  slow = hal_sys_timer_get();
  while (hal_sys_timer_get() == slow)
    ;
  fast = hal_fast_sys_timer_get();
  int_unlock(lock);

  slow += 1;
  slow_diff = slow - slow_val;

  // Avoid computation error
  if (slow_diff < MIN_CALIB_TICKS) {
    return 1;
  }
  // Avoid fast tick overflow
  if (slow_diff > MAX_CALIB_TICKS) {
    return 2;
  }

#ifdef TIMER_USE_FPU
  sys_tick_hz =
      (uint32_t)((float)CONFIG_FAST_SYSTICK_HZ / (fast - fast_val) * slow_diff);
#else
  uint64_t mul;

  mul = (uint64_t)CONFIG_FAST_SYSTICK_HZ * slow_diff;
  if ((mul >> 32) == 0) {
    sys_tick_hz = (uint32_t)mul / (fast - fast_val);
  } else {
    sys_tick_hz = mul / (fast - fast_val);
  }
#endif

  if (sys_tick_hz > MAX_CALIB_SYSTICK_HZ) {
    sys_tick_hz = MAX_CALIB_SYSTICK_HZ;
  }

  return 0;
}

void hal_sys_timer_calib(void) {
  hal_sys_timer_calib_start();
  hal_sys_timer_delay(MIN_CALIB_TICKS);
  hal_sys_timer_calib_end();
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_systick_hz(void) {
  return sys_tick_hz;
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_ms_to_ticks(uint32_t ms) {
  if (ms <= (~0UL / MAX_CALIB_SYSTICK_HZ)) {
    return (ms * sys_tick_hz / 1000);
  } else {
#ifdef TIMER_USE_FPU
    return (uint32_t)((float)ms / 1000 * sys_tick_hz);
#else
    return ((uint64_t)ms * sys_tick_hz / 1000);
#endif
  }
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_us_to_ticks(uint32_t us) {
  if (us <= (~0UL / MAX_CALIB_SYSTICK_HZ)) {
    return ((us * sys_tick_hz / 1000 + 1000 - 1) / 1000 + 1);
  } else {
#ifdef TIMER_USE_FPU
    return (uint32_t)((float)us / (1000 * 1000) * sys_tick_hz + 1 + 1);
#else
    return (((uint64_t)us * sys_tick_hz / 1000 + 1000 - 1) / 1000 + 1);
#endif
  }
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_ticks_to_ms(uint32_t tick) {
  if (tick <= (~0UL / 1000)) {
    return tick * 1000 / CONFIG_SYSTICK_HZ;
  } else {
#ifdef TIMER_USE_FPU
    return (uint32_t)((float)tick / CONFIG_SYSTICK_HZ * 1000);
#else
    return (uint64_t)tick * 1000 / CONFIG_SYSTICK_HZ;
#endif
  }
}

uint32_t BOOT_TEXT_SRAM_LOC hal_sys_timer_ticks_to_us(uint32_t tick) {
  if (tick <= (~0UL / (1000 * 1000))) {
    return tick * (1000 * 1000) / CONFIG_SYSTICK_HZ;
  } else {
#ifdef TIMER_USE_FPU
    return (uint32_t)((float)tick / CONFIG_SYSTICK_HZ * (1000 * 1000));
#else
    return (uint64_t)tick * (1000 * 1000) / CONFIG_SYSTICK_HZ;
#endif
  }
}
#endif

#ifndef RTOS
int osDelay(uint32_t ms) {
  hal_sys_timer_delay(MS_TO_TICKS(ms));
  return 0;
}
#endif

static void hal_timer00_irq_handler(void) {
  clear_timer_irq(&dual_timer0->timer[0]);
  dual_timer0->timer[0].Control &= ~TIMER_CTRL_INTEN;
}

void hal_timer_setup(enum HAL_TIMER_TYPE_T type,
                     HAL_TIMER_IRQ_HANDLER_T handler) {
  uint32_t mode;

  if (type == HAL_TIMER_TYPE_ONESHOT) {
    mode = TIMER_CTRL_ONESHOT;
  } else if (type == HAL_TIMER_TYPE_PERIODIC) {
    mode = TIMER_CTRL_MODE_PERIODIC;
  } else {
    mode = 0;
  }

  irq_handler = handler;

  clear_timer_irq(&dual_timer0->timer[1]);
#ifdef ELAPSED_TIMER_ENABLED
  dual_timer0->elapsed_timer[1].ElapsedCtrl = TIMER_ELAPSED_CTRL_CLR;
#endif

  if (handler) {
    NVIC_SetVector(TIMER01_IRQn, (uint32_t)hal_timer01_irq_handler);
    NVIC_SetPriority(TIMER01_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(TIMER01_IRQn);
    NVIC_EnableIRQ(TIMER01_IRQn);
  }

  dual_timer0->timer[1].Control = mode | (handler ? TIMER_CTRL_INTEN : 0) |
                                  TIMER_CTRL_PRESCALE_DIV_1 |
                                  TIMER_CTRL_SIZE_32_BIT;
}

void hal_timer_start(uint32_t load) {
  start_time = hal_sys_timer_get();
  hal_timer_reload(load);
  hal_timer_continue();
}

void hal_timer_stop(void) {
  dual_timer0->timer[1].Control &= ~TIMER_CTRL_EN;
#ifdef ELAPSED_TIMER_ENABLED
  dual_timer0->elapsed_timer[1].ElapsedCtrl = TIMER_ELAPSED_CTRL_CLR;
#endif
  clear_timer_irq(&dual_timer0->timer[1]);
  NVIC_ClearPendingIRQ(TIMER01_IRQn);
}

void hal_timer_continue(void) {
#ifdef ELAPSED_TIMER_ENABLED
  dual_timer0->elapsed_timer[1].ElapsedCtrl =
      TIMER_ELAPSED_CTRL_EN | TIMER_ELAPSED_CTRL_CLR;
#endif
  dual_timer0->timer[1].Control |= TIMER_CTRL_EN;
}

int hal_timer_is_enabled(void) {
  return !!(dual_timer0->timer[1].Control & TIMER_CTRL_EN);
}

void hal_timer_reload(uint32_t load) {
  if (load > HAL_TIMER_LOAD_DELTA) {
    // load_value = load;
    load -= HAL_TIMER_LOAD_DELTA;
  } else {
    // load_value = HAL_TIMER_LOAD_DELTA + 1;
    load = 1;
  }
  set_timer_load(&dual_timer0->timer[1], load, SLOW_TIMER_VAL_DELTA);
}

uint32_t hal_timer_get(void) {
  return get_timer_value(&dual_timer0->timer[1], SLOW_TIMER_VAL_DELTA);
}

int hal_timer_irq_active(void) { return NVIC_GetActive(TIMER01_IRQn); }

int hal_timer_irq_pending(void) {
  // Or NVIC_GetPendingIRQ(TIMER2_IRQn) ?
  return (dual_timer0->timer[1].MIS & TIMER_MIS_MIS);
}

uint32_t hal_timer_get_overrun_time(void) {
#ifdef ELAPSED_TIMER_ENABLED
  uint32_t extra;

  if (dual_timer0->elapsed_timer[1].ElapsedCtrl & TIMER_ELAPSED_CTRL_EN) {
    extra = dual_timer0->elapsed_timer[1].ElapsedVal;
  } else {
    extra = 0;
  }

  return extra;
#else
  return 0;
#endif
}

uint32_t hal_timer_get_elapsed_time(void) {
  // return load_value + hal_timer_get_overrun_time();
  return hal_sys_timer_get() - start_time;
}

static void hal_timer01_irq_handler(void) {
  uint32_t elapsed;

  clear_timer_irq(&dual_timer0->timer[1]);
  if (irq_handler) {
    elapsed = hal_timer_get_elapsed_time();
    irq_handler(elapsed);
  } else {
    dual_timer0->timer[1].Control &= ~TIMER_CTRL_INTEN;
  }
}

uint32_t hal_timer_get_passed_ticks(uint32_t curr_ticks, uint32_t prev_ticks) {
  if (curr_ticks < prev_ticks)
    return ((0xffffffff - prev_ticks + 1) + curr_ticks);
  else
    return (curr_ticks - prev_ticks);
}
