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
#if (defined(__GNUC__) && !defined(__ARMCC_VERSION))

#if !defined(NOSTD) && !defined(MBED)
#define LIBC_HOOKS
#endif

#ifdef  LIBC_HOOKS

#include <errno.h>
#include "hal_trace.h"

#ifndef FILEHANDLE
typedef int FILEHANDLE;
#endif

#define WEAK     __attribute__((weak))
#define PACKED   __attribute__((packed))

#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/syslimits.h>
#define PREFIX(x)    x

extern "C" int PREFIX(_write)(FILEHANDLE fh, const unsigned char *buffer,
                                    unsigned int length, int mode)
{
    int n = 0; // n is the number of bytes written
    if (fh < 3) {
        hal_trace_output(buffer, length);
        n = length;
    }
    return n;
}

extern "C" int PREFIX(_read)(FILEHANDLE fh, unsigned char *buffer,
                                     unsigned int length, int mode)
{
    int n = 0; // n is the number of bytes read
    if (fh < 3) {
        // only read a character at a time from stdin
        // TODO: Read from trace uart input
        n = 1;
    }
    return n;
}

#if defined(__GNUC__)
/* prevents the exception handling name demangling code getting pulled in */
namespace __gnu_cxx {
    void __verbose_terminate_handler() {
        ASSERT(0, "Exception");
    }
}
extern "C" WEAK void __cxa_pure_virtual(void);
extern "C" WEAK void __cxa_pure_virtual(void)
{
    _exit(1);
}

#endif

// Provide implementation of _sbrk (low-level dynamic memory allocation
// routine) for GCC_ARM which compares new heap pointer with MSP instead of
// SP.  This make it compatible with RTX RTOS thread stacks.
#if defined(__GNUC__) && defined(__arm__)
// Linker defined symbol used by _sbrk to indicate where heap should start.
extern "C" int __end__;
extern "C" uint32_t  __HeapLimit;

// Turn off the errno macro and use actual global variable instead.
#undef errno
extern "C" int errno;

// For ARM7 only
register unsigned char * stack_ptr __asm ("sp");

// Dynamic memory allocation related syscall.
extern "C" caddr_t _sbrk(int incr)
{
    static unsigned char* heap = (unsigned char*)&__end__;
    unsigned char*        prev_heap = heap;
    unsigned char*        new_heap = heap + incr;

#if defined(TARGET_ARM7)
    if (new_heap >= stack_ptr) {
#elif defined(TARGET_CORTEX_A)
    if (new_heap >= (unsigned char*)&__HeapLimit) {     /* __HeapLimit is end of heap section */
#else
    if (new_heap >= (unsigned char*)&__HeapLimit) {     /* __HeapLimit is end of heap section */
#endif
        ASSERT(false, "_sbrk:Heap overflowed: start=%p end=%p cur=%p incr=%d",
            (unsigned char*)&__end__, (unsigned char*)&__HeapLimit, heap, incr);
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    TRACE(2,"_sbrk: incr %d cur=%p\n", incr, heap);

    heap = new_heap;
    return (caddr_t) prev_heap;
}
#endif


#if defined(__GNUC__) && defined(__arm__)
extern "C" void _exit(int return_code)
{
#else
namespace std {
extern "C" void exit(int return_code)
{
#endif
    if (return_code) {
        ASSERT(false, "system die: %d", return_code);
    }

    do { volatile int i = 0; i++; } while (1);
}

#if !(defined(__GNUC__) && defined(__arm__)) && !defined(TOOLCHAIN_GCC_CW)
} //namespace std
#endif

#endif /*LIBC_HOOKS*/

#endif // __GNUC__ && !__ARMCC_VERSION

