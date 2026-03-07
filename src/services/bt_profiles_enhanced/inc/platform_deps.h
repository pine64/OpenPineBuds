
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
#ifndef __PLATFORM_DEPS_H__
#define __PLATFORM_DEPS_H__

#if defined(_WIN32)
#include "win32_os.h"
#else
#include "bes_os.h"
#endif

// ------ HCI ------ //
#if defined(__cplusplus)
extern "C" {
#endif
void Plt_HciInit(void);
void Plt_HciSendData(unsigned char type, unsigned short cmd_conn, unsigned short len, unsigned char *buffer);
void Plt_HciSendBuffer(unsigned char type, unsigned char *buff, int len);
void Plt_NotifyScheduler(void);
void Plt_LockHCIBuffer(void);
void Plt_UNLockHCIBuffer(void);

// ------ OS ------ //
//typedef enum {
//    OS_STK_EVT_STACK_READY = 0,
//    OS_STK_EVT_NOTIFY_EVM, // 1
//    OS_STK_EVT_EXIT_MAINLOOP, // 2
//}OS_STACK_EVENT_T;

char OS_Init(void);
void OS_LockStack(void);
void OS_UnlockStack(void);
unsigned char OS_LockIsExist(void);
void OS_StopHardware(void);
void OS_ResumeHardware(void);
//OS_STACK_EVENT_T OS_WaitEvent(unsigned int timeout_ms);
//char OS_SendEvent(OS_STACK_EVENT_T event);
void OS_NotifyEvm(void);
bool OS_WaitEvm(unsigned int timeout_ms);

// ------ TIMER ------ //
void Plt_TimerWrapperInit(void);
void Plt_TimerWrapperStart(unsigned int ms);
void Plt_TimerWrapperStop(void);
void Plt_UNLockTimerWrapper(void);
void Plt_LockTimerWrapper(void);
unsigned int Plt_GetTicks(void);
unsigned int Plt_GetTicksMax(void);

#if defined(__cplusplus)
}
#endif
#endif /* __PLATFORM_DEPS_H__ */