
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

#include "cmsis_os.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "osif.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern osMessageQId evm_queue_id;
static void __timer_wrapper_callback(void const *n);

static osif_timer_callback __timer_wrapper_notify_func = NULL;
static osTimerId __timer_wrapper_id = NULL;
osTimerDef(__timer_wrapper__, __timer_wrapper_callback);

osMutexId __mutex_wrapper_id = NULL;
osMutexDef(__mutex_wrapper);

osMutexId __mutex_stophardware_id = NULL;
osMutexDef(__mutex_stophardware);

bool osif_init(void) {
  if (__mutex_wrapper_id == NULL)
    __mutex_wrapper_id = osMutexCreate((osMutex(__mutex_wrapper)));

  if (__mutex_stophardware_id == NULL)
    __mutex_stophardware_id = osMutexCreate((osMutex(__mutex_stophardware)));

  if (__timer_wrapper_id == NULL)
    __timer_wrapper_id =
        osTimerCreate(osTimer(__timer_wrapper__), osTimerOnce, (void *)0);

  return true;
}

uint32_t osif_get_sys_time(void) { return hal_sys_timer_get(); }

uint16_t osif_rand(void) { return rand(); }

void osif_stop_hardware(void) {
  osMutexWait(__mutex_stophardware_id, osWaitForever);
}

void osif_resume_hardware(void) { osMutexRelease(__mutex_stophardware_id); }

void osif_memcopy(uint8_t *dest, const uint8_t *source, uint32_t numBytes) {
  memcpy(dest, source, numBytes);
}

bool osif_memcmp(const uint8_t *buffer1, uint16_t len1, const uint8_t *buffer2,
                 uint16_t len2) {
  int r = 0;
  r = memcmp(buffer1, buffer2, len1 > len2 ? len2 : len1);
  return (r == 0 ? 1 : 0);
}

void osif_memset(uint8_t *dest, uint8_t byte, uint32_t len) {
  memset(dest, byte, len);
}

uint8_t osif_strcmp(const char *Str1, const char *Str2) {
  int r = 0;
  r = strcmp(Str1, Str2);
  return (r == 0 ? 0 : 1);
}

uint16_t osif_strlen(const char *Str) { return strlen(Str); }

void osif_assert(const char *expression, const char *file, uint16_t line) {
  ASSERT(0, "OS_Assert exp: %s, func: %s, line %d\r\n", expression, file, line);
  while (1)
    ;
}
extern void OS_LockStack_Info_Print(void);
void osif_lock_stack(void) {
  osThreadId request = 0;
#if (osCMSIS < 0x20000U)
  osThreadId hold = 0;
#endif
  unsigned int t = 0;
  osStatus status;
  bool success;

  if (NULL == __mutex_wrapper_id)
    return;

  request = osThreadGetId();
#if (osCMSIS < 0x20000U)
  hold = osMutexGetOwner(__mutex_wrapper_id);
#endif
  t = hal_sys_timer_get();
  status = osMutexWait(__mutex_wrapper_id, 8000);
  t = TICKS_TO_MS(hal_sys_timer_get() - t);

  if ((t > 10) || (status != osOK)) {
    TRACE(2, "stack lock wait %d ms, status=0x%x", t, status);
    TRACE(1, "request thread=%p", request);
#if (osCMSIS < 0x20000U)
    TRACE(1, "hold thread=%p", hold);
#endif
  }

  success = (status == osOK);
  if (!success) {
    TRACE(1, "request thread=%p", request);
#if (osCMSIS < 0x20000U)
    osThreadShow(request);
    TRACE(1, "hold thread=%p", hold);
    osThreadShow(hold);
#endif
    OS_LockStack_Info_Print();
    ASSERT(0, "cannot lock stack %d\n", status);
    return;
  }
}

void osif_unlock_stack(void) {
  if (NULL == __mutex_wrapper_id)
    return;

  osMutexRelease(__mutex_wrapper_id);
}

extern uint32_t rt_mbx_check(void *mailbox);

void osif_notify_evm(void) {
  if (osMessageGetSpace(evm_queue_id) > 5) {
    osMessagePut(evm_queue_id, 0xFF, 0);
  }
}

static void __timer_wrapper_callback(void const *n) {
  if (__timer_wrapper_notify_func)
    __timer_wrapper_notify_func();
}

void osif_start_timer(osif_timer_t t, osif_timer_callback func) {
  __timer_wrapper_notify_func = func;

  osTimerStart(__timer_wrapper_id, (t >> 4) + 0x5);
}

void osif_cancel_timer(void) { osTimerStop(__timer_wrapper_id); }

uint8_t osif_lock_is_exist(void) {
  if (NULL == __mutex_wrapper_id)
    return 0;
  else
    return 1;
}
