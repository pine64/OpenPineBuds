/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:    RTX_Conf_CM.C
 *      Purpose: Configuration of CMSIS RTX Kernel for Cortex-M
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

#include  "stdarg.h"
#include  "stdio.h"
#include "cmsis_os.h"
#include "rt_System.h"
#include "rt_Time.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sleep.h"
#include "cmsis.h"
#include "hwtimer_list.h"

#define WEAK __attribute__((weak))

void WEAK sleep(void)
{
    hal_sleep_enter_sleep();
}

/*----------------------------------------------------------------------------
 *      RTX User configuration part BEGIN
 *---------------------------------------------------------------------------*/

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
// <h>Thread Configuration
// =======================
//
//   <o>Number of concurrent running threads <0-250>
//   <i> Defines max. number of threads that will run at the same time.
//       counting "main", but not counting "osTimerThread"
//   <i> Default: 6
#ifndef OS_TASKCNT
#    define OS_TASKCNT         __BEST_D_OS_TASKCNT
#endif

//   <o>Scheduler (+ interrupts) stack size [bytes] <64-4096:8><#/4>
#ifndef OS_SCHEDULERSTKSIZE
#      define OS_SCHEDULERSTKSIZE    __BEST_D_OS_SCHEDULERSTKSIZE
#endif

//   <o>Idle stack size [bytes] <64-4096:8><#/4>
//   <i> Defines default stack size for the Idle thread.
#ifndef OS_IDLESTKSIZE
 #define OS_IDLESTKSIZE         256
#endif

//   <o>Timer Thread stack size [bytes] <64-4096:8><#/4>
//   <i> Defines stack size for Timer thread.
//   <i> Default: 200
#ifndef OS_TIMERSTKSZ
 #define OS_TIMERSTKSZ  WORDS_STACK_SIZE
#endif

// <q>Check for stack overflow
// <i> Includes the stack checking code for stack overflow.
// <i> Note that additional code reduces the Kernel performance.
#ifndef OS_STKCHECK
 #define OS_STKCHECK    1
#endif

// <o>Processor mode for thread execution
//   <0=> Unprivileged mode
//   <1=> Privileged mode
// <i> Default: Privileged mode
#ifndef OS_RUNPRIV
 #define OS_RUNPRIV     1
#endif

//   <o>Timer tick value [us] <1-1000000>
//   <i> Defines the timer tick value.
//   <i> Default: 1000  (1ms)
#ifndef OS_TICK
 #define OS_TICK        1000
#endif

// </h>

// <h>System Configuration
// =======================
//
// <e>Round-Robin Thread switching
// ===============================
//
// <i> Enables Round-Robin Thread switching.
#ifndef OS_ROBIN
 #define OS_ROBIN       1
#endif

//   <o>Round-Robin Timeout [ticks] <1-1000>
//   <i> Defines how long a thread will execute before a thread switch.
//   <i> Default: 5
#ifndef OS_ROBINTOUT
 #define OS_ROBINTOUT   5
#endif

// </e>

// <e>User Timers
// ==============
//   <i> Enables user Timers
#ifndef OS_TIMERS
 #define OS_TIMERS      1
#endif

//   <o>Timer Thread Priority
//                        <1=> Low
//                        <2=> Below Normal
//                        <3=> Normal
//                        <4=> Above Normal
//                        <5=> High
//                        <6=> Realtime (highest)
//   <i> Defines priority for Timer Thread
//   <i> Default: High
#ifndef OS_TIMERPRIO
 #define OS_TIMERPRIO   5
#endif

//   <o>Timer Callback Queue size <1-32>
//   <i> Number of concurrent active timer callback functions.
//   <i> Default: 4
#ifndef OS_TIMERCBQSZ
 #define OS_TIMERCBQS   16
#endif

// </e>

//   <o>ISR FIFO Queue size<4=>   4 entries  <8=>   8 entries
//                         <12=> 12 entries  <16=> 16 entries
//                         <24=> 24 entries  <32=> 32 entries
//                         <48=> 48 entries  <64=> 64 entries
//                         <96=> 96 entries
//   <i> ISR functions store requests to this buffer,
//   <i> when they are called from the interrupt handler.
//   <i> Default: 16 entries
#ifndef OS_FIFOSZ
 #define OS_FIFOSZ      16
#endif

// </h>

//------------- <<< end of configuration section >>> -----------------------

// Standard library system mutexes
// ===============================
//  Define max. number system mutexes that are used to protect
//  the arm standard runtime library. For microlib they are not used.
#ifndef OS_MUTEXCNT
 #define OS_MUTEXCNT    12
#endif

/*----------------------------------------------------------------------------
 *      RTX User configuration part END
 *---------------------------------------------------------------------------*/

#define OS_TRV          ((uint32_t)((((float)OS_CLOCK*(float)OS_TICK))/(float)1E6+0.5f)-1)

U32 os_get_trv       (void)
{
    return OS_TRV;
}

extern void rtx_show_all_threads(void);

#if TASK_HUNG_CHECK_ENABLED
extern void check_hung_tasks(void);
#endif

/*----------------------------------------------------------------------------
 *      OS Idle daemon
 *---------------------------------------------------------------------------*/
void os_idle_demon (void) {
    /* The idle demon is a system thread, running when no other thread is      */
    /* ready to run.                                                           */

    unsigned int os_ticks;
    HWTIMER_ID timer;
    int ret;
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
    unsigned int start_time;
    unsigned int start_os_time;
    unsigned int start_tick;
#endif
#if defined(FPGA) || !(defined(ROM_BUILD) || defined(PROGRAMMER))
    ret = hal_trace_crash_dump_register(HAL_TRACE_CRASH_DUMP_MODULE_SYS, rtx_show_all_threads);
    ASSERT(ret == 0, "IdleTask: Failed to register crash dump callback");
#endif
    timer = hwtimer_alloc((HWTIMER_CALLBACK_T)rt_psh_req, NULL);
    ASSERT(timer, "IdleTask: Failed to alloc sleep timer");
    /* Sleep: ideally, we should put the chip to sleep.
     Unfortunately, this usually requires disconnecting the interface chip (debugger).
     This can be done, but it would break the local file system.
    */
    for (;;) {
#if TASK_HUNG_CHECK_ENABLED
        check_hung_tasks();
#endif

        if (hal_sleep_light_sleep() == HAL_SLEEP_STATUS_DEEP) {
            os_ticks = rt_suspend();
            if (os_ticks) {
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
                __disable_irq();
#endif
                ret = hwtimer_start(timer, MS_TO_HWTICKS(os_ticks * OS_TICK / 1000));
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
                start_time = hal_sys_timer_get();
                start_tick = SysTick->VAL;
                start_os_time = os_time;
                __enable_irq();
#endif
                if (ret == 0) {
                    sleep();
                    ret = hwtimer_stop(timer);
                }
#if defined(DEBUG_SLEEP) && (DEBUG_SLEEP >= 2)
                if (hal_sys_timer_get() - start_time >= MS_TO_HWTICKS(1)) {
                    TRACE(4,"[%u/0x%X][%2u/%u] os_idle_demon start timer",
                        TICKS_TO_MS(start_time), start_time, start_tick, start_os_time);
                }
#endif
            }
            rt_resume(os_ticks);
        }
    }
}

/*----------------------------------------------------------------------------
 *      RTX Errors
 *---------------------------------------------------------------------------*/
extern void rtx_show_current_thread(void);
void os_error (uint32_t err_code) {
    /* This function is called when a runtime error is detected. Parameter     */
    /* 'err_code' holds the runtime error code (defined in RTX_Conf.h).      */

    rtx_show_current_thread();
    ASSERT(0, "os_error: %d ThreadId:%d\n", err_code, osGetThreadIntId());

    //mbed_die();
}

void os_error_str (const char *str, ...) {
    va_list ap;
    static char buf[50];

    va_start(ap, str);
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);

    ASSERT(0, "%s\n", buf);
}

void sysThreadError(osStatus status) {
    if (status != osOK) {
        TRACE_IMM(1,"osStatus: %08x\n", status);
        rtx_show_current_thread();
        ASSERT(0, "sysThreadError ThreadId:%d\n", osGetThreadIntId());
    }
}

/*----------------------------------------------------------------------------
 *      RTX Configuration Functions
 *---------------------------------------------------------------------------*/

#include "RTX_CM_lib.h"

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

