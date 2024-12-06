/*
 * Copyright (c) 2013-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -----------------------------------------------------------------------------
 *
 * $Revision:   V5.1.0
 *
 * Project:     CMSIS-RTOS RTX
 * Title:       RTX Configuration
 *
 * -----------------------------------------------------------------------------
 */

#include "RTE_Components.h"
#include "cmsis_compiler.h"
#include CMSIS_device_header
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "rtx_config.h"
#include "rtx_os.h"

#include "cmsis_os2.h"
#include "hwtimer_list.h"

void WEAK sleep(void) { hal_sleep_enter_sleep(); }

extern void rtx_show_all_threads(void);
#if TASK_HUNG_CHECK_ENABLED
extern void check_hung_threads(void);
#endif

// OS Idle Thread
__NO_RETURN void osRtxIdleThread(void *argument) {
  unsigned int os_ticks;
  HWTIMER_ID timer;
  int ret;
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
  unsigned int start_time;
  unsigned int start_os_time;
  unsigned int start_tick;
#endif
#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
  ret = hal_trace_crash_dump_register(HAL_TRACE_CRASH_DUMP_MODULE_SYS,
                                      rtx_show_all_threads);
  ASSERT(ret == 0, "IdleTask: Failed to register crash dump callback");
#endif
  timer = hwtimer_alloc(NULL, NULL);
  ASSERT(timer, "IdleTask: Failed to alloc sleep timer");

  for (;;) {
#if TASK_HUNG_CHECK_ENABLED
    check_hung_threads();
#endif

    if (hal_sleep_light_sleep() == HAL_SLEEP_STATUS_DEEP) {
      os_ticks = osKernelSuspend();
      if (os_ticks) {
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
        __disable_irq();
#endif
        ret =
            hwtimer_start(timer, MS_TO_HWTICKS(os_ticks * OS_TICK_FREQ / 1000));
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
        start_time = hal_sys_timer_get();
        start_tick = SysTick->VAL;
        start_os_time = osRtxInfo.kernel.tick;
        __enable_irq();
#endif
        if (ret == 0) {
          sleep();
          ret = hwtimer_stop(timer);
        }
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
        if (hal_sys_timer_get() - start_time >= MS_TO_HWTICKS(1)) {
          TRACE(5, "[%u/0x%X][%2u/%u] os_idle_demon start timer. tick:%u",
                TICKS_TO_MS(start_time), start_time, start_tick, start_os_time,
                os_ticks);
        }
#endif
      }
      osKernelResume(os_ticks);
    }
  }
}

// OS Error Callback function
__WEAK uint32_t osRtxErrorNotify(uint32_t code, void *object_id) {
  (void)object_id;

  switch (code) {
  case osRtxErrorStackUnderflow:
    // Stack overflow detected for thread (thread_id=object_id)
    break;
  case osRtxErrorISRQueueOverflow:
    // ISR Queue overflow detected when inserting object (object_id)
    break;
  case osRtxErrorTimerQueueOverflow:
    // User Timer Callback Queue overflow detected for timer
    // (timer_id=object_id)
    break;
  case osRtxErrorClibSpace:
    // Standard C/C++ library libspace not available: increase
    // OS_THREAD_LIBSPACE_NUM
    break;
  case osRtxErrorClibMutex:
    // Standard C/C++ library mutex initialization failed
    break;
  default:
    // Reserved
    break;
  }
  ASSERT(0, "osRtxErrorNotify, code: %08x\n", code);
  // return 0U;
}
