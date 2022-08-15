/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:    RT_TASK.C
 *      Purpose: Task functions and system start up.
 *      Rev.:    V4.60
 *----------------------------------------------------------------------------
 *
 * Copyright (c) 1999-2009 KEIL, 2009-2012 ARM Germany GmbH
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of ARM  nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/

#include "rt_TypeDef.h"
#include "RTX_Conf.h"
#include "rt_System.h"
#include "rt_Task.h"
#include "rt_List.h"
#include "rt_MemBox.h"
#include "rt_Robin.h"
#include "rt_HAL_CM.h"

#include "hal_timer.h"

/*----------------------------------------------------------------------------
 *      Global Variables
 *---------------------------------------------------------------------------*/

/* Running and next task info. */
struct OS_TSK os_tsk;

/* Task Control Blocks of idle demon */
struct OS_TCB os_idle_TCB;

static U32 rtx_get_hwticks(void)
{
    return hal_sys_timer_get();
}


/*----------------------------------------------------------------------------
 *      Local Functions
 *---------------------------------------------------------------------------*/

OS_TID rt_get_TID (void) {
  U32 tid;

  for (tid = 1; tid <= os_maxtaskrun; tid++) {
    if (os_active_TCB[tid-1] == NULL) {
      return ((OS_TID)tid);
    }
  }
  return (0);
}

#if defined (__CC_ARM) && !defined (__MICROLIB)
/*--------------------------- __user_perthread_libspace ---------------------*/
extern void  *__libspace_start;

void *__user_perthread_libspace (void) {
  /* Provide a separate libspace for each task. */
  if (os_tsk.run == NULL) {
    /* RTX not running yet. */
    return (&__libspace_start);
  }
  return (void *)(os_tsk.run->std_libspace);
}
#endif

/*--------------------------- rt_init_context -------------------------------*/

void rt_init_context (P_TCB p_TCB, U8 priority, FUNCP task_body) {
  /* Initialize general part of the Task Control Block. */
  p_TCB->cb_type = TCB;
  p_TCB->state   = READY;
  p_TCB->prio    = priority;
  p_TCB->p_lnk   = NULL;
  p_TCB->p_rlnk  = NULL;
  p_TCB->p_dlnk  = NULL;
  p_TCB->p_blnk  = NULL;
  p_TCB->delta_time    = 0;
  p_TCB->interval_time = 0;
  p_TCB->events  = 0;
  p_TCB->waits   = 0;
  p_TCB->stack_frame = 0;

  rt_init_stack (p_TCB, task_body);
}


/*--------------------------- rt_switch_req ---------------------------------*/

void rt_switch_req (P_TCB p_new) {
  /* Switch to next task (identified by "p_new"). */
  os_tsk.new_tsk   = p_new;
#if __RTX_CPU_STATISTICS__
  if (os_tsk.run != p_new) {
      os_tsk.run->swap_out_time = HWTICKS_TO_MS(rtx_get_hwticks());
      p_new->swap_in_time = HWTICKS_TO_MS(rtx_get_hwticks());
  }
#endif
  p_new->state = RUNNING;
  DBG_TASK_SWITCH(p_new->task_id);
}


/*--------------------------- rt_dispatch -----------------------------------*/

void rt_dispatch (P_TCB next_TCB) {
  /* Dispatch next task if any identified or dispatch highest ready task    */
  /* "next_TCB" identifies a task to run or has value NULL (=no next task)  */
  if (next_TCB == NULL) {
    /* Running task was blocked: continue with highest ready task */
    next_TCB = rt_get_first (&os_rdy);
    rt_switch_req (next_TCB);
  }
  else {
    /* Check which task continues */
    if (next_TCB->prio > os_tsk.run->prio) {
      /* preempt running task */
      rt_put_rdy_first (os_tsk.run);
      os_tsk.run->state = READY;
      rt_switch_req (next_TCB);
    }
    else {
      /* put next task into ready list, no task switch takes place */
      next_TCB->state = READY;
      rt_put_prio (&os_rdy, next_TCB);
    }
  }
}


/*--------------------------- rt_block --------------------------------------*/

void rt_block (U16 timeout, U8 block_state) {
  /* Block running task and choose next ready task.                         */
  /* "timeout" sets a time-out value or is 0xffff (=no time-out).           */
  /* "block_state" defines the appropriate task state */
  P_TCB next_TCB;

  if (timeout) {
    if (timeout < 0xffff) {
      rt_put_dly (os_tsk.run, timeout);
    }
    os_tsk.run->state = block_state;
    next_TCB = rt_get_first (&os_rdy);
    rt_switch_req (next_TCB);
  }
}


/*--------------------------- rt_tsk_pass -----------------------------------*/

void rt_tsk_pass (void) {
  /* Allow tasks of same priority level to run cooperatively.*/
  P_TCB p_new;

  p_new = rt_get_same_rdy_prio();
  if (p_new != NULL) {
    rt_put_prio ((P_XCB)&os_rdy, os_tsk.run);
    os_tsk.run->state = READY;
    rt_switch_req (p_new);
  }
}


/*--------------------------- rt_tsk_self -----------------------------------*/

OS_TID rt_tsk_self (void) {
  /* Return own task identifier value. */
  if (os_tsk.run == NULL) {
    return (0);
  }
  return (os_tsk.run->task_id);
}


/*--------------------------- rt_tsk_prio -----------------------------------*/

OS_RESULT rt_tsk_prio (OS_TID task_id, U8 new_prio) {
  /* Change execution priority of a task to "new_prio". */
  P_TCB p_task;

  if (task_id == 0) {
    /* Change execution priority of calling task. */
    os_tsk.run->prio = new_prio;
run:if (rt_rdy_prio() > new_prio) {
      rt_put_prio (&os_rdy, os_tsk.run);
      os_tsk.run->state   = READY;
      rt_dispatch (NULL);
    }
    return (OS_R_OK);
  }

  /* Find the task in the "os_active_TCB" array. */
  if (task_id > os_maxtaskrun || os_active_TCB[task_id-1] == NULL) {
    /* Task with "task_id" not found or not started. */
    return (OS_R_NOK);
  }
  p_task = os_active_TCB[task_id-1];
  p_task->prio = new_prio;
  if (p_task == os_tsk.run) {
    goto run;
  }
  rt_resort_prio (p_task);
  if (p_task->state == READY) {
    /* Task enqueued in a ready list. */
    p_task = rt_get_first (&os_rdy);
    rt_dispatch (p_task);
  }
  return (OS_R_OK);
}

/*--------------------------- rt_tsk_delete ---------------------------------*/

OS_RESULT rt_tsk_delete (OS_TID task_id) {
  /* Terminate the task identified with "task_id". */
  P_TCB task_context;

  if (task_id == 0 || task_id == os_tsk.run->task_id) {
    /* Terminate itself. */
    os_tsk.run->state     = INACTIVE;
    os_tsk.run->tsk_stack = rt_get_PSP ();
    rt_stk_check ();
    os_active_TCB[os_tsk.run->task_id-1] = NULL;

    os_tsk.run->stack = NULL;
    DBG_TASK_NOTIFY(os_tsk.run, __FALSE);
    os_tsk.run = NULL;
    rt_dispatch (NULL);
    /* The program should never come to this point. */
  }
  else {
    /* Find the task in the "os_active_TCB" array. */
    if (task_id > os_maxtaskrun || os_active_TCB[task_id-1] == NULL) {
      /* Task with "task_id" not found or not started. */
      return (OS_R_NOK);
    }
    task_context = os_active_TCB[task_id-1];
    rt_rmv_list (task_context);
    rt_rmv_dly (task_context);
    os_active_TCB[task_id-1] = NULL;

    task_context->stack = NULL;
    DBG_TASK_NOTIFY(task_context, __FALSE);
  }
  return (OS_R_OK);
}


/*--------------------------- rt_sys_init -----------------------------------*/

#ifdef __CMSIS_RTOS
void rt_sys_init (void) {
#else
void rt_sys_init (FUNCP first_task, U32 prio_stksz, void *stk) {
#endif
  /* Initialize system and start up task declared with "first_task". */
  U32 i;

  DBG_INIT();

  /* Initialize dynamic memory and task TCB pointers to NULL. */
  for (i = 0; i < os_maxtaskrun; i++) {
    os_active_TCB[i] = NULL;
  }

  /* Set up TCB of idle demon */
  os_idle_TCB.task_id = 255;
  os_idle_TCB.priv_stack = idle_task_stack_size;
  os_idle_TCB.stack = idle_task_stack;
#if __RTX_CPU_STATISTICS__
  os_idle_TCB.name = (U8 *)"os_idle";
#endif
  rt_init_context (&os_idle_TCB, 0, os_idle_demon);

  /* Set up ready list: initially empty */
  os_rdy.cb_type = HCB;
  os_rdy.p_lnk   = NULL;
  /* Set up delay list: initially empty */
  os_dly.cb_type = HCB;
  os_dly.p_dlnk  = NULL;
  os_dly.p_blnk  = NULL;
  os_dly.delta_time = 0;

  /* Fix SP and systemvariables to assume idle task is running  */
  /* Transform main program into idle task by assuming idle TCB */
#ifndef __CMSIS_RTOS
  rt_set_PSP (os_idle_TCB.tsk_stack+32);
#endif
  os_tsk.run = &os_idle_TCB;
  os_tsk.run->state = RUNNING;

  /* Initialize ps queue */
  os_psq->first = 0;
  os_psq->last  = 0;
  os_psq->size  = os_fifo_size;
#if __RTX_CPU_STATISTICS__
  os_tsk.run->swap_in_time = HWTICKS_TO_MS(rtx_get_hwticks());
#endif
  rt_init_robin ();

  /* Intitialize SVC and PendSV */
  rt_svc_init ();

#ifndef __CMSIS_RTOS
  /* Intitialize and start system clock timer */
  os_tick_irqn = os_tick_init ();
  if (os_tick_irqn >= 0) {
    OS_X_INIT(os_tick_irqn);
  }

  /* Start up first user task before entering the endless loop */
  rt_tsk_create (first_task, prio_stksz, stk, NULL);
#endif
}


/*--------------------------- rt_sys_start ----------------------------------*/

#ifdef __CMSIS_RTOS
void rt_sys_start (void) {
  /* Start system */

  /* Intitialize and start system clock timer */
  os_tick_irqn = os_tick_init ();
  if (os_tick_irqn >= 0) {
    OS_X_INIT(os_tick_irqn);
  }
}
#endif

//------------------------------------------------------------------------------
// Debug functions
//------------------------------------------------------------------------------

#include "plat_addr_map.h"
#include "hal_location.h"
#include "hal_trace.h"

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

static inline uint32_t get_IPSR(void)
{
  uint32_t result;

  asm volatile ("MRS %0, ipsr" : "=r" (result) );
  return(result);
}

static inline uint32_t get_PSP(void)
{
  uint32_t result;

  asm volatile ("MRS %0, psp" : "=r" (result) );
  return(result);
}

static inline struct IRQ_STACK_FRAME_T *_rtx_get_irq_stack_frame(P_TCB tcb)
{
    uint32_t sp;

    if (tcb == NULL) {
        return NULL;
    }
    if (tcb == os_tsk.run && get_IPSR() == 0) {
        return NULL;
    }
    if (tcb == os_tsk.run) {
        sp = get_PSP();
    } else {
        sp = tcb->tsk_stack;
    }
    if ((sp & 3) || !hal_trace_address_writable(sp)) {
        return NULL;
    }

    if (tcb != os_tsk.run) {
        // r4-r11
        sp += 4 * 8;
        if (tcb->stack_frame) {
            // s16-s31
            sp += 4 * 16;
        }
    }

    return (struct IRQ_STACK_FRAME_T *)sp;
}

FLASH_TEXT_LOC
void rt_tsk_show(P_TCB tcb)
{
    static const char * const state_list[] = {
        "INACTIVE", "READY", "RUNNING", "WAIT_DLY", "WAIT_ITV", "WAIT_OR", "WAIT_AND", "WAIT_SEM", "WAIT_MBX", "WAIT_MUT", "BAD",
    };
    const char *task_st_str;
    uint32_t idx;
    struct IRQ_STACK_FRAME_T *frame;

    if (tcb) {
        if (tcb->cb_type == TCB && ((tcb->task_id >= 1 && tcb->task_id <= os_maxtaskrun) || tcb->task_id == 255) &&
                tcb->ptask && tcb->stack) {
            if (tcb->state < ARRAY_SIZE(state_list)) {
                idx = tcb->state;
            } else {
                idx = ARRAY_SIZE(state_list) - 1;
            }
            task_st_str = state_list[idx];
            REL_TRACE_NOCRLF_NOTS(1,"--- Task %3u", tcb->task_id);

            REL_TRACE_NOTS(4," tcb=0x%08X prio=%u state=%-8s ptask=0x%08X", (uint32_t)tcb, tcb->prio, task_st_str, (uint32_t)tcb->ptask);
#if __RTX_CPU_STATISTICS__
            if(tcb->name || tcb->task_id == 1) {
                REL_TRACE_NOTS(1,"    name=%s", tcb->name ? (const char *)tcb->name : "main");
            }
#endif
#ifdef RTX_DUMP_VERBOSE
            REL_TRACE_NOTS(3,"    p_lnk=0x%08X p_rlnk=0x%08X p_dlnk=0x%08X",
                 (uint32_t)tcb->p_lnk, (uint32_t)tcb->p_rlnk, (uint32_t)tcb->p_dlnk);
            REL_TRACE_NOTS(3,"    p_blnk=0x%08X delta_time=%u interval_time=%u",
                 (uint32_t)tcb->p_blnk, tcb->delta_time, tcb->interval_time);
            REL_TRACE_NOTS(3,"    events=0x%04X waits=0x%04X msg=0x%08X",
                 tcb->events, tcb->waits, (uint32_t)tcb->msg);
            REL_TRACE_NOTS(2,"    priv_stack(stack_size)=%4u tsk_stack(sp)=0x%08X",
                 tcb->priv_stack, tcb->tsk_stack);
            REL_TRACE_NOTS(3,"    stack(top)=0x%08X stack_frame=%u stk_msk:0x%04x",
                (uint32_t)tcb->stack, tcb->stack_frame, tcb->stack[0]);
#ifdef __RTX_CPU_STATISTICS__
            REL_TRACE_NOTS(2,"    swap_in_time=%u swap_out_time=%u",
                tcb->swap_in_time, tcb->swap_out_time);
            REL_TRACE_NOCRLF_NOTS(0,"    after last switch ");
            if (tcb->swap_in_time <= tcb->swap_out_time)
                REL_TRACE_NOTS(1,"task runtime %u ms", tcb->swap_out_time - tcb->swap_in_time);
            else
                REL_TRACE_NOTS(1,"task still runing, now %d", HWTICKS_TO_MS(rtx_get_hwticks()));
#endif
#endif /*RTX_DUMP_VERBOSE*/

            frame = _rtx_get_irq_stack_frame(tcb);
            if (frame) {
                uint32_t stack_end;
                uint32_t search_cnt, print_cnt;

                REL_TRACE_NOTS(0," ");
                REL_TRACE_NOTS(4,"    R0 =0x%08X R1=0x%08X R2=0x%08X R3  =0x%08X", frame->r0, frame->r1, frame->r2, frame->r3);
                REL_TRACE_NOTS(4,"    R12=0x%08X LR=0x%08X PC=0x%08X XPSR=0x%08X", frame->r12, frame->lr, frame->pc, frame->xpsr);

                stack_end = (uint32_t)tcb->stack + tcb->priv_stack;
                if (stack_end > tcb->tsk_stack) {
                    search_cnt = (stack_end - tcb->tsk_stack) / 4;
                    if (search_cnt > 512) {
                        search_cnt = 512;
                    }
                    print_cnt = 10;
                    hal_trace_print_backtrace(tcb->tsk_stack, search_cnt, print_cnt);
                }
            }
        } else {
            REL_TRACE_NOTS(0,"--- Task BAD");
        }
    } else {
        REL_TRACE_NOTS(0,"--- Task NONE");
    }
}

FLASH_TEXT_LOC
void rtx_show_current_thread(void)
{
    REL_TRACE_NOTS(1,"Current Task    : %u", os_tsk.run ? os_tsk.run->task_id : 0);
    REL_TRACE_NOTS(1,"New Running Task: %u", os_tsk.new_tsk ? os_tsk.new_tsk->task_id : 0);
    REL_TRACE_IMM_NOTS(0," ");
}

FLASH_TEXT_LOC
void rtx_show_ready_threads(void)
{
    P_TCB tcb;
    uint32_t i;

    REL_TRACE_NOTS(0,"Ready Tasks:");
    if (os_rdy.p_lnk) {
        tcb = os_rdy.p_lnk;
        i = 0;
        do {
            REL_TRACE_NOCRLF_NOTS(1,"%u ", tcb->task_id);
            tcb = tcb->p_lnk;
            i++;
        } while (tcb && i < os_maxtaskrun);
        REL_TRACE_NOTS(0," ");
        if (tcb) {
            REL_TRACE_NOTS(2,"*** Error: List corrupted? count=%u next=0x%08X\n", i, (uint32_t)tcb);
        }
    } else {
        REL_TRACE_NOTS(0,"<NONE>");
    }
    REL_TRACE_IMM_NOTS(0," ");
}

FLASH_TEXT_LOC
void rtx_show_delay_threads(void)
{
    P_TCB tcb;
    uint32_t i;

    REL_TRACE_NOTS(0,"Delay Tasks:");
    if (os_dly.p_dlnk) {
        tcb = os_dly.p_dlnk;
        i = 0;
        do {
            REL_TRACE_NOCRLF_NOTS(1,"%u ", tcb->task_id);
            tcb = tcb->p_dlnk;
            i++;
        } while (tcb && i < os_maxtaskrun);
        REL_TRACE_NOTS(0," ");
        if (tcb) {
            REL_TRACE_NOTS(2,"*** Error: List corrupted? count=%u next=0x%08X\n", i, (uint32_t)tcb);
        }
    } else {
        REL_TRACE_NOTS(0,"<NONE>");
    }
    REL_TRACE_IMM_NOTS(0," ");
}

FLASH_TEXT_LOC
void rtx_show_all_threads(void)
{
    int i;
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
    if (hal_trace_crash_dump_onprocess()){
        for (i = 0; i < 10; i++){
            REL_TRACE_IMM_NOTS(0,"                                                                        ");
            REL_TRACE_IMM_NOTS(0,"                                                                        \n");
            hal_sys_timer_delay(MS_TO_TICKS(200));
        }
    }
#endif

    REL_TRACE_NOTS(0,"Task List:");
    for (i = 0; i < os_maxtaskrun; i++) {
        if (os_active_TCB[i]) {
            rt_tsk_show(os_active_TCB[i]);
            REL_TRACE_IMM_NOTS(0," ");
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
            if (hal_trace_crash_dump_onprocess()){
                hal_sys_timer_delay(MS_TO_TICKS(500));
            }
#endif
        }
    }
    rt_tsk_show(&os_idle_TCB);
    REL_TRACE_IMM_NOTS(0," ");

    rtx_show_current_thread();
    rtx_show_ready_threads();
    rtx_show_delay_threads();
}

#if __RTX_CPU_STATISTICS__

#if TASK_HUNG_CHECK_ENABLED
FLASH_TEXT_LOC NOINLINE
static void print_hung_task(const P_TCB tcb, U32 curr_time)
{
    REL_TRACE_IMM_NOTS(2,"Task \"%s\" blocked for %dms",
            tcb->name==NULL ? (tcb->task_id == 1 ? "main" : "null") : (char *)tcb->name,
            curr_time - tcb->swap_out_time);
    ASSERT(0, "Find task hung");
}

static void check_hung_task(const P_TCB tcb)
{
    uint32_t curr_hwticks, curr_time;

    if (!tcb->hung_check)
        return;

    curr_hwticks = hal_sys_timer_get();
    curr_time = HWTICKS_TO_MS(curr_hwticks);
    if((curr_time - tcb->swap_out_time) > tcb->hung_check_timeout) {
        print_hung_task(tcb, curr_time);
    }
}

void check_hung_tasks(void)
{
    int i;

    for (i = 0; i < os_maxtaskrun; i++) {
        if (os_active_TCB[i]) {
            P_TCB tcb = os_active_TCB[i];
            check_hung_task(tcb);
        }
    }
}
#endif

static inline void print_task_sw_statitics(P_TCB tcb)
{
    /*
    REL_TRACE_NOTS(3,"--- Task swap in:%d  out=%d runings %d",
            tcb->swap_in_time,
            tcb->swap_out_time,
            tcb->rtime);
     */
}

FLASH_TEXT_LOC
static void _rtx_show_thread_usage(P_TCB tcb, U32 sample_time)
{
    if (tcb) {
        if (tcb->cb_type == TCB && ((tcb->task_id >= 1 && tcb->task_id <= os_maxtaskrun)) &&
                tcb->ptask && tcb->stack) {
            REL_TRACE_NOTS(5,"--- Task id:%d task_name=%s cpu=%%%d",
                    tcb->task_id,
                    tcb->name==NULL ? (tcb->task_id == 1 ? "main" : "null") : (char *)tcb->name,
                    sample_time != 0 ? ((tcb->rtime - task_rtime[tcb->task_id]) * 100 / sample_time) : 0);
            print_task_sw_statitics(tcb);
            task_rtime[tcb->task_id] = tcb->rtime;
        } else if (tcb->task_id == 255 ) {
            REL_TRACE_NOTS(5,"--- Task id:%d task_name=%s cpu=%%%d",
                    tcb->task_id,
                    "idle",
                    sample_time != 0 ? (tcb->rtime - task_rtime[0]) * 100 / sample_time : 0);
            print_task_sw_statitics(tcb);
            task_rtime[0] = tcb->rtime;
        } else {
            REL_TRACE_NOTS(0,"--- Task BAD");
        }
    } else {
        REL_TRACE_NOTS(0,"--- Task NONE");
    }
}

FLASH_TEXT_LOC
void rtx_show_all_threads_usage(void)
{
    int i;
    static BOOL first_time  = 1;
    uint32_t sample_time;
    static U32 start_sample_time  = 0;

    if (first_time) {
        for (i = 0; i < os_maxtaskrun; i++) {
            if (os_active_TCB[i]) {
                P_TCB tcb = os_active_TCB[i];
                task_rtime[tcb->task_id] = tcb->rtime;
            }
        }
        task_rtime[0] =  os_idle_TCB.rtime;
        start_sample_time  = rtx_get_hwticks();
        first_time = 0;
        return;
    }

    sample_time = HWTICKS_TO_MS(rtx_get_hwticks() - start_sample_time);
    REL_TRACE_IMM_NOTS(0," ");
    REL_TRACE_NOTS(0,"Task List:");
    for (i = 0; i < os_maxtaskrun; i++) {
        if (os_active_TCB[i]) {
            _rtx_show_thread_usage(os_active_TCB[i], sample_time);
        }
    }
    _rtx_show_thread_usage(&os_idle_TCB, sample_time);
    start_sample_time  = rtx_get_hwticks();
    REL_TRACE_IMM_NOTS(0," ");
}

#endif

/*
 * health time period for idle thread scheduled
 */
#define TASK_IDLE_HEALTH_PERIOD  (60 * 1000)

FLASH_TEXT_LOC
int rtx_task_idle_health_check(void)
{
    P_TCB tcb = &os_idle_TCB;
    uint32_t now, period;

    now = HWTICKS_TO_MS(rtx_get_hwticks());

    if (now >= tcb->swap_in_time) {
        period = now - tcb->swap_in_time;
    } else {
        uint64_t temp;
        temp = now + HWTICKS_TO_MS((-1u));
        period = temp - tcb->swap_in_time;
    }

    if ( period > TASK_IDLE_HEALTH_PERIOD) {
        REL_TRACE_NOTS(1,"--- Task idle hung %d seconds", period / 1000);
        return -1;
    }
    return 0;
}

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
