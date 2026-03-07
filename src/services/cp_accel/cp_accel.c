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
#ifdef CHIP_HAS_CP

#include "cp_accel.h"
#include "app_utils.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_cmu.h"
#include "hal_location.h"
#include "hal_mcu2cp.h"
#include "hal_memsc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "mpu.h"
#include "stdarg.h"
#include "string.h"
#include "system_cp.h"

#ifdef CP_ACCEL_DEBUG
#define CP_ACCEL_TRACE(s, ...) TRACE(s, ##__VA_ARGS__)
#else
#define CP_ACCEL_TRACE(s, ...)
#endif

#define CP_NO_FLASH_ACCESS

#define CP_CRASH_START_TIMEOUT MS_TO_TICKS(100)
#define CP_TRACE_FLUSH_TIMEOUT MS_TO_TICKS(200)
#define CP_CRASH_DUMP_TIMEOUT MS_TO_TICKS(500)
#define CP_TRACE_BUF_FULL_INTVL MS_TO_TICKS(50)

enum CP_SYS_EVENT_T {
  CP_SYS_EVENT_NONE = 0,
  CP_SYS_EVENT_CRASH_START,
  CP_SYS_EVENT_CRASH_END,
  CP_SYS_EVENT_TRACE_FLUSH,
  CP_SYS_EVENT_TRACE_BUF_FULL,
};

static bool ram_inited;
static bool cp_accel_inited = false;
static struct cp_task_env_tag cp_task_env;
static CP_BSS_LOC volatile struct cp_env_tag cp_env;

static CP_BSS_LOC volatile enum CP_SYS_EVENT_T cp_sys_evt;
static CP_BSS_LOC bool cp_in_crash;
static CP_BSS_LOC volatile uint8_t cp_in_sleep;
static CP_BSS_LOC uint32_t cp_buf_full_time;
static CP_BSS_LOC uint8_t req_event = 0, pending_event = 0;

static CP_TEXT_SRAM_LOC int send_sys_ctrl_cp2mcu(uint32_t event) {
  return hal_mcu2cp_send_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0,
                            (unsigned char *)event, 0);
}

static CP_TEXT_SRAM_LOC void
cp_trace_crash_notify(enum HAL_TRACE_STATE_T state) {
  uint32_t time;

  if (state == HAL_TRACE_STATE_CRASH_ASSERT_START ||
      state == HAL_TRACE_STATE_CRASH_FAULT_START) {
    cp_in_crash = true;
    cp_sys_evt = CP_SYS_EVENT_CRASH_START;
    mpu_close();
    send_sys_ctrl_cp2mcu(0);

    time = hal_sys_timer_get();
    while (cp_sys_evt == CP_SYS_EVENT_CRASH_START &&
           hal_sys_timer_get() - time < CP_CRASH_START_TIMEOUT)
      ;
  } else {
    cp_sys_evt = CP_SYS_EVENT_CRASH_END;
  }
}

static CP_TEXT_SRAM_LOC void
cp_trace_buffer_ctrl(enum HAL_TRACE_BUF_STATE_T buf_state) {
  uint32_t time;

  if (cp_sys_evt != CP_SYS_EVENT_NONE) {
    return;
  }

  time = hal_sys_timer_get();

  if (buf_state == HAL_TRACE_BUF_STATE_FLUSH) {
    cp_sys_evt = CP_SYS_EVENT_TRACE_FLUSH;
    if (!cp_in_crash) {
      send_sys_ctrl_cp2mcu(0);
    }

    while (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH &&
           hal_sys_timer_get() - time < CP_TRACE_FLUSH_TIMEOUT)
      ;
  } else if (buf_state == HAL_TRACE_BUF_STATE_FULL ||
             buf_state == HAL_TRACE_BUF_STATE_NEAR_FULL) {
    if (time - cp_buf_full_time >= CP_TRACE_BUF_FULL_INTVL) {
      cp_buf_full_time = time;
      if (!cp_in_crash) {
        cp_sys_evt = CP_SYS_EVENT_TRACE_BUF_FULL;
        send_sys_ctrl_cp2mcu(0);
      }
    }
  }
}

static SRAM_TEXT_LOC unsigned int cp2mcu_sys_arrived(const unsigned char *data,
                                                     unsigned int len) {
  uint32_t time;
  uint8_t task_id = 0;

  if (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH) {
    TRACE_FLUSH();
    cp_sys_evt = CP_SYS_EVENT_NONE;
  } else if (cp_sys_evt == CP_SYS_EVENT_TRACE_BUF_FULL) {
    TRACE(0, " ");
    cp_sys_evt = CP_SYS_EVENT_NONE;
  } else if (cp_sys_evt == CP_SYS_EVENT_CRASH_START) {
    cp_sys_evt = CP_SYS_EVENT_NONE;

    TRACE(0, " ");
    TRACE(0, "CP Crash starts ...");
    UNLOCK_CP_PROCESS(); // Forced release lock

    // Wait CP crash dump finishes in interrupt context
    time = hal_sys_timer_get();
    while (cp_sys_evt != CP_SYS_EVENT_CRASH_END &&
           hal_sys_timer_get() - time < CP_CRASH_DUMP_TIMEOUT) {
      if (cp_sys_evt == CP_SYS_EVENT_TRACE_FLUSH) {
        TRACE_FLUSH();
        cp_sys_evt = CP_SYS_EVENT_NONE;
      }
    }

    for (task_id = 0; task_id < CP_TASK_MAX; task_id++) {
      if (cp_task_env.p_desc[task_id].mcu_sys_ctrl_hdlr) {
        cp_task_env.p_desc[task_id].mcu_sys_ctrl_hdlr(CP_SYS_EVENT_CRASH_END);
      }
    }

    TRACE(0, "CP Crash ends ...");
    TRACE(0, " ");
  }

  return len;
}

static CP_TEXT_SRAM_LOC unsigned int
mcu2cp_msg_arrived(const unsigned char *data, unsigned int len) {
  uint8_t task_id = CP_TASK_ID_GET(*data);
  uint8_t event_type = CP_EVENT_GET(*data);

  cp_env.cp_msg[task_id][event_type] = 1;
  cp_env.cp_msg_recv = true;

  if (cp_task_env.p_desc[task_id].cp_evt_hdlr) {
    cp_task_env.p_desc[task_id].cp_evt_hdlr((uint32_t)data);
  }

  return len;
}

static CP_TEXT_SRAM_LOC void mcu2cp_msg_sent(const unsigned char *data,
                                             unsigned int len) {
  // TRACE(1, "mcu2cp_msg_sent,pending count = %d", cp_env.mcu2cp_tx_count);

  if (cp_env.mcu2cp_tx_count > 1) {
    cp_env.mcu2cp_tx_count--;
    pending_event = cp_env.mcu2cp_tx_pending[0];
    hal_mcu2cp_send_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, &pending_event,
                        1);

    for (uint8_t index = 0; index < cp_env.mcu2cp_tx_count - 1; index++) {
      cp_env.mcu2cp_tx_pending[index] = cp_env.mcu2cp_tx_pending[index + 1];
    }
  } else {
    cp_env.mcu2cp_tx_count = 0;
  }
}

#if defined(__ARM_ARCH_8M_MAIN__)

#define CP_CODE_MAP_BASE (ROM_BASE + 0x800)
#define CP_CODE_MAP_SIZE (RAMX_BASE + RAM_TOTAL_SIZE - CP_CODE_MAP_BASE)

static CP_DATA_LOC const mpu_regions_t mpu_table_cp[] = {
    {CP_CODE_MAP_BASE, CP_CODE_MAP_SIZE, MPU_ATTR_EXEC, MAIR_ATTR_INT_SRAM},
    {RAM_BASE, RAM_TOTAL_SIZE, MPU_ATTR_READ_WRITE, MAIR_ATTR_INT_SRAM},
    {CMU_BASE, 0x01000000, MPU_ATTR_READ_WRITE, MAIR_ATTR_DEVICE},
};
#else
static CP_DATA_LOC const mpu_regions_t mpu_table_cp[] = {
    {0, 0x800, MPU_ATTR_NO_ACCESS},
    {FLASHX_BASE, 0x4000000, MPU_ATTR_NO_ACCESS},
    {FLASH_BASE, 0x4000000, MPU_ATTR_NO_ACCESS},
    {FLASH_NC_BASE, 0x4000000, MPU_ATTR_NO_ACCESS},
};
#endif

static CP_TEXT_SRAM_LOC NOINLINE void accel_loop(void) {
  uint32_t lock;
  uint8_t task_index = 0, event_index = 0;
  bool msg_flag = false;
  uint8_t msg[CP_TASK_MAX][CP_EVENT_MAX];

  mpu_setup_cp(mpu_table_cp, ARRAY_SIZE(mpu_table_cp));

  while (1) {
    lock = int_lock_global();
    msg_flag = cp_env.cp_msg_recv;
    cp_env.cp_msg_recv = false;
    memcpy(msg, (uint8_t *)cp_env.cp_msg, sizeof(cp_env.cp_msg));
    memset((uint8_t *)cp_env.cp_msg, 0, sizeof(cp_env.cp_msg));
    if (false == msg_flag) {
      cp_in_sleep = true;
      __WFI();
      cp_in_sleep = false;
    }
    int_unlock_global(lock);

    if (msg_flag) {
      for (task_index = 0; task_index < CP_TASK_MAX; task_index++) {
        for (event_index = 0; event_index < CP_EVENT_MAX; event_index++) {
          LOCK_CP_PROCESS();
          if ((msg[task_index][event_index]) &&
              (cp_task_env.p_desc[task_index].cp_work_main)) {
            cp_task_env.p_desc[task_index].cp_work_main(event_index);
          }
          UNLOCK_CP_PROCESS();
        }
      }
    }
  }
}

static void accel_main(void) {
  system_cp_init(!ram_inited);
  TRACE(1, "%s", __func__);

  ram_inited = true;

  memset((uint8_t *)&cp_env, 0, sizeof(cp_env));

  hal_trace_open_cp();

  hal_mcu2cp_open_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, mcu2cp_msg_arrived,
                     NULL, false);
  hal_mcu2cp_open_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0, NULL, NULL, false);

  hal_mcu2cp_start_recv_cp(HAL_MCU2CP_ID_0);
  // hal_mcu2cp_start_recv_cp(HAL_MCU2CP_ID_1);

  cp_accel_inited = true;

  accel_loop();
}

static SRAM_TEXT_LOC unsigned int cp2mcu_msg_arrived(const unsigned char *data,
                                                     unsigned int len) {
  uint8_t task_id = CP_TASK_ID_GET((uint32_t)data);
  // TRACE(2, "%s, task_id = %d", __func__, task_id);

  if (task_id >= CP_TASK_MAX) {
    return -1;
  }

  if (cp_task_env.p_desc[task_id].mcu_evt_hdlr) {
    cp_task_env.p_desc[task_id].mcu_evt_hdlr((uint32_t)data);
  }

  return len;
}

int cp_accel_open(enum CP_TASK_TYPE task_id,
                  struct cp_task_desc const *p_task_desc) {
  TRACE(4, "%s, task id = %d, cp_state = %d init %d", __func__, task_id,
        cp_task_env.p_desc[task_id].cp_accel_state, cp_accel_inited);

  if ((task_id >= CP_TASK_MAX) || (p_task_desc == NULL)) {
    TRACE(1, "%s task id error", __func__);
    return -1;
  }

  if (cp_task_env.p_desc[task_id].cp_accel_state != CP_ACCEL_STATE_CLOSED) {
    TRACE(1, "%s cp_accel_state error", __func__);
    return -1;
  }

  cp_task_env.p_desc[task_id].cp_accel_state = CP_ACCEL_STATE_OPENING;
  cp_task_env.p_desc[task_id].cp_work_main = p_task_desc->cp_work_main;
  cp_task_env.p_desc[task_id].cp_evt_hdlr = p_task_desc->cp_evt_hdlr;
  cp_task_env.p_desc[task_id].mcu_evt_hdlr = p_task_desc->mcu_evt_hdlr;
  cp_task_env.p_desc[task_id].mcu_sys_ctrl_hdlr =
      p_task_desc->mcu_sys_ctrl_hdlr;

  if (false == cp_accel_inited) {
    hal_trace_cp_register(cp_trace_crash_notify, cp_trace_buffer_ctrl);

    hal_cmu_cp_enable(RAMCP_BASE + RAMCP_SIZE, (uint32_t)accel_main);

    hal_mcu2cp_open_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0,
                        cp2mcu_msg_arrived, mcu2cp_msg_sent, false);
    hal_mcu2cp_open_mcu(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0,
                        cp2mcu_sys_arrived, NULL, false);

    hal_mcu2cp_start_recv_mcu(HAL_MCU2CP_ID_0);
    hal_mcu2cp_start_recv_mcu(HAL_MCU2CP_ID_1);
  }
  cp_task_env.p_desc[task_id].cp_accel_state = CP_ACCEL_STATE_OPENED;

  return 0;
}

int cp_accel_close(enum CP_TASK_TYPE task_id) {
  uint8_t i = 0;

  TRACE(4, "%s, task id = %d, cp_state = %d init %d", __func__, task_id,
        cp_task_env.p_desc[task_id].cp_accel_state, cp_accel_inited);
  LOCK_CP_PROCESS(); // avoid hangup

  if (cp_task_env.p_desc[task_id].cp_accel_state == CP_ACCEL_STATE_CLOSED) {
    goto accel_close_end;
  }

  cp_task_env.p_desc[task_id].cp_accel_state = CP_ACCEL_STATE_CLOSING;
  cp_task_env.p_desc[task_id].cp_work_main = NULL;
  cp_task_env.p_desc[task_id].cp_evt_hdlr = NULL;
  cp_task_env.p_desc[task_id].mcu_evt_hdlr = NULL;

  for (i = 0; i < CP_TASK_MAX; i++) {
    if (cp_task_env.p_desc[i].cp_accel_state == CP_ACCEL_STATE_OPENED ||
        cp_task_env.p_desc[i].cp_accel_state == CP_ACCEL_STATE_OPENING) {
      goto accel_close_end;
    }
  }

  if (cp_accel_inited) {
    cp_accel_inited = false;

    hal_mcu2cp_close_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);
    hal_mcu2cp_close_mcu(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0);

    hal_cmu_cp_disable();

    system_cp_term();

    hal_mcu2cp_close_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0);
    hal_mcu2cp_close_cp(HAL_MCU2CP_ID_1, HAL_MCU2CP_MSG_TYPE_0);

    hal_trace_cp_register(NULL, NULL);
  }

accel_close_end:
  cp_task_env.p_desc[task_id].cp_accel_state = CP_ACCEL_STATE_CLOSED;
  UNLOCK_CP_PROCESS();
  return 0;
}

int SRAM_TEXT_LOC cp_accel_init_done(void) {
  // TRACE(2, "%s, cp_inited = %d", __func__, cp_accel_inited);
  return cp_accel_inited;
}

int cp_accel_send_event_mcu2cp(uint8_t event) {
  if ((false == cp_accel_inited) || (cp_env.mcu2cp_tx_count > MAX_CP_MSG_NUM)) {
    TRACE(2, "send_evt error, cp_accel_inited = %d, event pending count = %d",
          cp_accel_inited, cp_env.mcu2cp_tx_count);

    TRACE(2, "send evt task_id = %d, event = %d", CP_TASK_ID_GET(event),
          CP_EVENT_GET(event));
    return -1;
  }

  // TRACE(1, "current CP tx count:%d", cp_env.mcu2cp_tx_count);
  if (cp_env.mcu2cp_tx_count > 0) {
    cp_env.mcu2cp_tx_pending[cp_env.mcu2cp_tx_count - 1] = event;
    cp_env.mcu2cp_tx_count++;
  } else {
    req_event = event;
    cp_env.mcu2cp_tx_count = 1;
    hal_mcu2cp_send_mcu(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0, &req_event, 1);
  }

  return 0;
}

int CP_TEXT_SRAM_LOC cp_accel_send_event_cp2mcu(uint8_t event) {
  return hal_mcu2cp_send_cp(HAL_MCU2CP_ID_0, HAL_MCU2CP_MSG_TYPE_0,
                            (unsigned char *)(uint32_t)event, 0);
}

int SRAM_TEXT_LOC cp_accel_busy(enum CP_TASK_TYPE task_id) {
  if (cp_task_env.p_desc[task_id].cp_accel_state != CP_ACCEL_STATE_CLOSED) {
    if (cp_task_env.p_desc[task_id].cp_accel_state == CP_ACCEL_STATE_OPENED &&
        cp_in_sleep && !hal_mcu2cp_local_irq_pending_cp(HAL_MCU2CP_ID_0)) {
      return false;
    }
    return true;
  }

  return false;
}

#endif
