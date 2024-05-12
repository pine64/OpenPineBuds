/*
 * Copyright (c) 2013-2019 Arm Limited. All rights reserved.
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
 * Project:     CMSIS-RTOS RTX
 * Title:       Thread functions
 *
 * -----------------------------------------------------------------------------
 */

#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "rtx_lib.h"

#define RTX_DUMP_VERBOSE

struct IRQ_STACK_FRAME_T {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t pc;
  uint32_t xpsr;
};

extern uint32_t __StackTop[];

static inline uint32_t get_IPSR(void) {
  uint32_t result;

  asm volatile("MRS %0, ipsr" : "=r"(result));
  return (result);
}

static inline uint32_t get_PSP(void) {
  uint32_t result;

  asm volatile("MRS %0, psp" : "=r"(result));
  return (result);
}

static inline struct IRQ_STACK_FRAME_T *
_rtx_get_irq_stack_frame(const os_thread_t *thread) {
  uint32_t sp;

  if (thread == NULL) {
    return NULL;
  }
  if (thread == osRtxThreadGetRunning() && get_IPSR() == 0) {
    return NULL;
  }
  if (thread == osRtxThreadGetRunning()) {
    sp = get_PSP();
  } else {
    sp = thread->sp;
  }
  if ((sp & 3) || !hal_trace_address_writable(sp)) {
    return NULL;
  }

  if (thread != osRtxThreadGetRunning()) {
    // r4-r11
    sp += 4 * 8;
    if ((thread->stack_frame & 0x10) == 0) {
      // s16-s31
      sp += 4 * 16;
    }
  }

  return (struct IRQ_STACK_FRAME_T *)sp;
}

FLASH_TEXT_LOC
void rtx_thread_show(const os_thread_t *thread) {
  const char *thread_st_str;
  struct IRQ_STACK_FRAME_T *frame;

  if (thread) {
    switch (thread->state) {
    case osRtxThreadInactive:
      thread_st_str = "INACTIVE";
      break;
    case osRtxThreadReady:
      thread_st_str = "READY";
      break;
    case osRtxThreadRunning:
      thread_st_str = "RUNNING";
      break;
    case osRtxThreadTerminated:
      thread_st_str = "TERMINAT";
      break;
    case osRtxThreadWaitingDelay:
      thread_st_str = "WAIT_DLY";
      break;
    case osRtxThreadWaitingJoin:
      thread_st_str = "WAIT_JOIN";
      break;
    case osRtxThreadWaitingThreadFlags:
      thread_st_str = "WAIT_FLAG";
      break;
    case osRtxThreadWaitingEventFlags:
      thread_st_str = "WAIT_EVE";
      break;
    case osRtxThreadWaitingMutex:
      thread_st_str = "WAIT_MUT";
      break;
    case osRtxThreadWaitingSemaphore:
      thread_st_str = "WAIT_SEM";
      break;
    case osRtxThreadWaitingMemoryPool:
      thread_st_str = "WAIT_MEM";
      break;
    case osRtxThreadWaitingMessageGet:
      thread_st_str = "WAIT_MGET";
      break;
    case osRtxThreadWaitingMessagePut:
      thread_st_str = "WAIT_MPUT";
      break;
    default:
      thread_st_str = "BAD";
      break;
    }
    REL_TRACE_NOCRLF_NOTS(0, "--- Thread ");
    REL_TRACE_NOCRLF_NOTS(1, "name=%s", thread->name ? thread->name : "NULL");

    REL_TRACE_NOTS(4, " thread=0x%x, prio=%u state=%-9s thread_addr=0x%08X",
                   (uint32_t)thread, thread->priority, thread_st_str,
                   thread->thread_addr);
#ifdef RTX_DUMP_VERBOSE
    REL_TRACE_NOTS(4,
                   "    thread_next=0x%08X thread_prev=0x%08X "
                   "delay_next=0x%08X delay_prev=0x%08X",
                   (uint32_t)thread->thread_next, (uint32_t)thread->thread_prev,
                   (uint32_t)thread->delay_next, (uint32_t)thread->delay_prev);
    REL_TRACE_NOTS(
        4,
        "    thread_join=0x%08X flags_options=%u wait_flags=%u thread_flags=%u",
        (uint32_t)thread->thread_join, thread->flags_options,
        thread->wait_flags, thread->thread_flags);
    REL_TRACE_NOTS(3, "    stack_mem=0x%08X stack_size=%u sp:0x%04x",
                   (uint32_t)thread->stack_mem, thread->stack_size, thread->sp);
#ifdef __RTX_CPU_STATISTICS__
    REL_TRACE_NOTS(2, "    swap_in_time=%u swap_out_time=%u",
                   thread->swap_in_time, thread->swap_out_time);
    REL_TRACE_NOCRLF_NOTS(0, "    after last switch ");
    if (thread->swap_in_time <= thread->swap_out_time)
      REL_TRACE_NOTS(1, "thread runtime %ums",
                     thread->swap_out_time - thread->swap_in_time);
    else
      REL_TRACE_NOTS(1, "thread still runing, now %d",
                     HWTICKS_TO_MS(rtx_get_hwticks()));
#endif
#endif /*RTX_DUMP_VERBOSE*/

    frame = _rtx_get_irq_stack_frame(thread);
    if (frame) {
      uint32_t stack_end;
      uint32_t search_cnt, print_cnt;

      REL_TRACE_NOTS(1, "    frame:0x%08X", (uint32_t)frame);
      REL_TRACE_NOTS(4, "    R0 =0x%08X R1=0x%08X R2=0x%08X R3  =0x%08X",
                     frame->r0, frame->r1, frame->r2, frame->r3);
      REL_TRACE_NOTS(4, "    R12=0x%08X LR=0x%08X PC=0x%08X XPSR=0x%08X",
                     frame->r12, frame->lr, frame->pc, frame->xpsr);

      stack_end = (uint32_t)thread->stack_mem + thread->stack_size;
      if (stack_end > thread->sp) {
        search_cnt = (stack_end - thread->sp) / 4;
        if (search_cnt > 512) {
          search_cnt = 512;
        }
        print_cnt = 10;
        hal_trace_print_backtrace(thread->sp, search_cnt, print_cnt);
      }
    }
  } else {
    REL_TRACE_NOTS(0, "--- Thread NONE");
  }

  REL_TRACE_IMM_NOTS(0, " ");
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
  if (hal_trace_crash_dump_onprocess()) {
    hal_sys_timer_delay(MS_TO_TICKS(500));
  }
#endif
}

void rtx_show_all_threads(void) {
  int i;
  const os_thread_t *thread;
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
  if (hal_trace_crash_dump_onprocess()) {
    for (i = 0; i < 10; i++) {
      REL_TRACE_IMM_NOTS(0, "                                                  "
                            "                      ");
      REL_TRACE_IMM_NOTS(0, "                                                  "
                            "                      \n");
      hal_sys_timer_delay(MS_TO_TICKS(200));
    }
  }
#endif

  REL_TRACE_NOTS(0, "Thread List:");

  // Current List
  REL_TRACE_NOTS(0, "Current List");
  rtx_thread_show(osRtxInfo.thread.run.curr);

  // Next List
  if (osRtxInfo.thread.run.next != osRtxInfo.thread.run.curr) {
    REL_TRACE_NOTS(0, "Next List");
    rtx_thread_show(osRtxInfo.thread.run.next);
  }

  // Ready List
  REL_TRACE_NOTS(0, "Ready List");
  for (thread = osRtxInfo.thread.ready.thread_list; thread != NULL;
       thread = thread->thread_next) {
    rtx_thread_show(thread);
  }

  // Delay List
  REL_TRACE_NOTS(0, "Delay List");
  for (thread = osRtxInfo.thread.delay_list; thread != NULL;
       thread = thread->delay_next) {
    rtx_thread_show(thread);
  }

  // Wait List
  REL_TRACE_NOTS(0, "Wait List");
  for (thread = osRtxInfo.thread.wait_list; thread != NULL;
       thread = thread->delay_next) {
    rtx_thread_show(thread);
  }

  // Terminate List
  REL_TRACE_NOTS(0, "Terminate List");
  for (thread = osRtxInfo.thread.terminate_list; thread != NULL;
       thread = thread->thread_next) {
    rtx_thread_show(thread);
  }

  REL_TRACE_IMM_NOTS(0, " ");
}

#if __RTX_CPU_STATISTICS__

#if TASK_HUNG_CHECK_ENABLED
FLASH_TEXT_LOC NOINLINE static void print_hung_task(const os_thread_t *thread,
                                                    U32 curr_time) {
  REL_TRACE_IMM_NOTS(2, "Thread \"%s\" blocked for %dms",
                     thread->name == NULL ? "NULL" : (char *)thread->name,
                     curr_time - thread->swap_out_time);
  ASSERT(0, "Find thread hung ");
}

static void check_hung_thread(const os_thread_t *thread) {
  uint32_t curr_hwticks, curr_time;

  if (!thread->hung_check)
    return;

  curr_hwticks = hal_sys_timer_get();
  curr_time = HWTICKS_TO_MS(curr_hwticks);
  if ((curr_time - thread->swap_out_time) > thread->hung_check_timeout) {
    print_hung_task(thread, curr_time);
  }
}

void check_hung_threads(void) {
  const os_thread_t *thread;
  // Current List
  check_hung_thread(osRtxInfo.thread.run.curr);

  // Next List
  if (osRtxInfo.thread.run.next != osRtxInfo.thread.run.curr)
    check_hung_thread(osRtxInfo.thread.run.next);

  // Ready List
  for (thread = osRtxInfo.thread.ready.thread_list; thread != NULL;
       thread = thread->thread_next) {
    check_hung_thread(thread);
  }

  // Delay List
  for (thread = osRtxInfo.thread.delay_list; thread != NULL;
       thread = thread->delay_next) {
    check_hung_thread(thread);
  }

  // Wait List
  for (thread = osRtxInfo.thread.wait_list; thread != NULL;
       thread = thread->delay_next) {
    check_hung_thread(thread);
  }

  // Terminate List
  for (thread = osRtxInfo.thread.terminate_list; thread != NULL;
       thread = thread->thread_next) {
    check_hung_thread(thread);
  }
}
#endif

static inline void print_thread_sw_statitics(const os_thread_t *thread) {
  /*
  REL_TRACE_NOTS(3,"--- Thread swap in:%d  out=%d runings %d",
          thread->swap_in_time,
          thread->swap_out_time,
          thread->rtime);
   */
}

FLASH_TEXT_LOC
static void _rtx_show_thread_usage(const os_thread_t *thread,
                                   uint32_t sample_time) {
  if (thread) {
    if (thread->thread_addr && thread->stack_mem) {
      REL_TRACE_NOTS(4, "--- Thread name=%s cpu=%%%d",
                     thread->name == NULL ? "null" : (char *)thread->name,
                     sample_time != 0 ? ((thread->rtime - thread->step_rtime) *
                                         100 / sample_time)
                                      : 0);
      print_thread_sw_statitics(thread);
      ((os_thread_t *)thread)->step_rtime = thread->rtime;
    } else {
      REL_TRACE_NOTS(0, "--- Thread BAD");
    }
  } else {
    REL_TRACE_NOTS(0, "--- Thread NONE");
  }
}

FLASH_TEXT_LOC
void rtx_show_all_threads_usage(void) {
  const os_thread_t *thread;
  static bool first_time = 1;
  uint32_t sample_time;
  static uint32_t start_sample_time = 0;

  if (first_time) {
    start_sample_time = rtx_get_hwticks();
    first_time = 0;
    return;
  }

  sample_time = HWTICKS_TO_MS(rtx_get_hwticks() - start_sample_time);
  REL_TRACE_IMM_NOTS(0, " ");
  REL_TRACE_NOTS(0, "Thread List:");

  // Current List
  _rtx_show_thread_usage(osRtxInfo.thread.run.curr, sample_time);

  // Next List
  if (osRtxInfo.thread.run.next != osRtxInfo.thread.run.curr)
    _rtx_show_thread_usage(osRtxInfo.thread.run.next, sample_time);

  // Ready List
  for (thread = osRtxInfo.thread.ready.thread_list; thread != NULL;
       thread = thread->thread_next) {
    _rtx_show_thread_usage(thread, sample_time);
  }

  // Delay List
  for (thread = osRtxInfo.thread.delay_list; thread != NULL;
       thread = thread->delay_next) {
    _rtx_show_thread_usage(thread, sample_time);
  }

  // Wait List
  for (thread = osRtxInfo.thread.wait_list; thread != NULL;
       thread = thread->delay_next) {
    _rtx_show_thread_usage(thread, sample_time);
  }

  // Terminate List
  for (thread = osRtxInfo.thread.terminate_list; thread != NULL;
       thread = thread->thread_next) {
    _rtx_show_thread_usage(thread, sample_time);
  }
  start_sample_time = rtx_get_hwticks();
  REL_TRACE_IMM_NOTS(0, " ");
}

#endif
