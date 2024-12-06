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
#include "app_ibrt_peripheral_manager.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui.h"
#include "app_tws_besaud.h"
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt_trace.h"
#include "app_vendor_cmd_evt.h"
#include "bluetooth.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "bt_if.h"
#include "cmsis_os.h"
#include "conmgr_api.h"
#include "crc16.h"
#include "hci_api.h"
#include "heap_api.h"
#include "l2cap_api.h"
#include "me_api.h"
#include "rfcomm_api.h"
#include "tws_role_switch.h"
#include <string.h>
#if defined(IBRT)
osThreadId app_ibrt_peripheral_tid;
static void app_ibrt_peripheral_thread(void const *argument);
osThreadDef(app_ibrt_peripheral_thread, osPriorityHigh, 1, 2048,
            "app_ibrt_peripheral_thread");

#define TWS_PERIPHERAL_DEVICE_MAILBOX_MAX (10)
osMailQDef(app_ibrt_peripheral_mailbox, TWS_PERIPHERAL_DEVICE_MAILBOX_MAX,
           TWS_PD_MSG_BLOCK);
static osMailQId app_ibrt_peripheral_mailbox = NULL;
static uint8_t app_ibrt_peripheral_mailbox_cnt = 0;

static int app_ibrt_peripheral_mailbox_init(void) {
  app_ibrt_peripheral_mailbox =
      osMailCreate(osMailQ(app_ibrt_peripheral_mailbox), NULL);
  if (app_ibrt_peripheral_mailbox == NULL) {
    TRACE(0, "Failed to Create app_ibrt_peripheral_mailbox\n");
    return -1;
  }
  app_ibrt_peripheral_mailbox_cnt = 0;
  return 0;
}

int app_ibrt_peripheral_mailbox_put(TWS_PD_MSG_BLOCK *msg_src) {
  if (!msg_src) {
    TRACE(0, "msg_src is a null pointer in app_ibrt_peripheral_mailbox_put!");
    return -1;
  }

  osStatus status;
  TWS_PD_MSG_BLOCK *msg_p = NULL;
  msg_p = (TWS_PD_MSG_BLOCK *)osMailAlloc(app_ibrt_peripheral_mailbox, 0);
  ASSERT(msg_p, "osMailAlloc error");
  msg_p->msg_body.message_id = msg_src->msg_body.message_id;
  msg_p->msg_body.message_ptr = msg_src->msg_body.message_ptr;
  msg_p->msg_body.message_Param0 = msg_src->msg_body.message_Param0;
  msg_p->msg_body.message_Param1 = msg_src->msg_body.message_Param1;
  msg_p->msg_body.message_Param2 = msg_src->msg_body.message_Param2;

  status = osMailPut(app_ibrt_peripheral_mailbox, msg_p);
  if (osOK == status)
    app_ibrt_peripheral_mailbox_cnt++;
  return (int)status;
}

int app_ibrt_peripheral_mailbox_free(TWS_PD_MSG_BLOCK *msg_p) {
  if (!msg_p) {
    TRACE(0, "msg_p is a null pointer in app_ibrt_peripheral_mailbox_free!");
    return -1;
  }
  osStatus status;

  status = osMailFree(app_ibrt_peripheral_mailbox, msg_p);
  if (osOK == status)
    app_ibrt_peripheral_mailbox_cnt--;

  return (int)status;
}

int app_ibrt_peripheral_mailbox_get(TWS_PD_MSG_BLOCK **msg_p) {
  if (!msg_p) {
    TRACE(0, "msg_p is a null pointer in app_ibrt_peripheral_mailbox_get!");
    return -1;
  }

  osEvent evt;
  evt = osMailGet(app_ibrt_peripheral_mailbox, osWaitForever);
  if (evt.status == osEventMail) {
    *msg_p = (TWS_PD_MSG_BLOCK *)evt.value.p;
    return 0;
  }
  return -1;
}

#ifdef BES_AUTOMATE_TEST
#define APP_IBRT_PERIPHERAL_BUF_SIZE 2048
static heap_handle_t app_ibrt_peripheral_heap;
uint8_t app_ibrt_peripheral_buf[APP_IBRT_PERIPHERAL_BUF_SIZE];
bool app_ibrt_auto_test_started = false;
void app_ibrt_peripheral_heap_init(void) {
  app_ibrt_auto_test_started = true;
  app_ibrt_peripheral_heap =
      heap_register(app_ibrt_peripheral_buf, APP_IBRT_PERIPHERAL_BUF_SIZE);
}

void *app_ibrt_peripheral_heap_malloc(uint32_t size) {
  void *ptr = heap_malloc(app_ibrt_peripheral_heap, size);
  ASSERT(ptr, "%s size:%d", __func__, size);
  return ptr;
}

void *app_ibrt_peripheral_heap_cmalloc(uint32_t size) {
  void *ptr = heap_malloc(app_ibrt_peripheral_heap, size);
  ASSERT(ptr, "%s size:%d", __func__, size);
  memset(ptr, 0, size);
  return ptr;
}

void *app_ibrt_peripheral_heap_realloc(void *rmem, uint32_t newsize) {
  void *ptr = heap_realloc(app_ibrt_peripheral_heap, rmem, newsize);
  ASSERT(ptr, "%s rmem:%p size:%d", __func__, rmem, newsize);
  return ptr;
}

void app_ibrt_peripheral_heap_free(void *rmem) {
  ASSERT(rmem, "%s rmem:%p", __func__, rmem);
  heap_free(app_ibrt_peripheral_heap, rmem);
}
#endif

void app_ibrt_peripheral_auto_test_stop(void) {
#ifdef BES_AUTOMATE_TEST
  app_ibrt_auto_test_started = false;
#endif
}

/*****************************************************************************
 Prototype    : app_ibrt_peripheral_thread
 Description  : peripheral manager thread
 Input        : void const *argument
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/18
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
typedef void (*app_ibrt_peripheral_cb0)(void);
typedef void (*app_ibrt_peripheral_cb1)(void *);
typedef void (*app_ibrt_peripheral_cb2)(void *, void *);
void app_ibrt_peripheral_automate_test_handler(uint8_t *cmd_buf,
                                               uint32_t cmd_len);

void app_ibrt_peripheral_thread(void const *argument) {
  while (1) {
    TWS_PD_MSG_BLOCK *msg_p = NULL;
    if ((!app_ibrt_peripheral_mailbox_get(&msg_p)) && (!argument)) {
      switch (msg_p->msg_body.message_id) {
      case 0:
        if (msg_p->msg_body.message_ptr) {
          ((app_ibrt_peripheral_cb0)(msg_p->msg_body.message_ptr))();
        }
        break;
      case 1:
        if (msg_p->msg_body.message_ptr) {
          ((app_ibrt_peripheral_cb1)(msg_p->msg_body.message_ptr))(
              (void *)msg_p->msg_body.message_Param0);
        }
        break;
      case 2:
        if (msg_p->msg_body.message_ptr) {
          ((app_ibrt_peripheral_cb2)(msg_p->msg_body.message_ptr))(
              (void *)msg_p->msg_body.message_Param0,
              (void *)msg_p->msg_body.message_Param1);
        }
        break;
      case 0xfe:
        app_ibrt_peripheral_automate_test_handler(
            (uint8_t *)msg_p->msg_body.message_Param0,
            (uint32_t)msg_p->msg_body.message_Param1);
        break;
      case 0xff: { // ibrt test
        char ibrt_cmd[20] = {0};
        memcpy(ibrt_cmd + 0, &msg_p->msg_body.message_Param0, 4);
        memcpy(ibrt_cmd + 4, &msg_p->msg_body.message_Param1, 4);
        memcpy(ibrt_cmd + 8, &msg_p->msg_body.message_Param2, 4);
        TRACE(1, "ibrt_ui_log: %s\n", ibrt_cmd);
        app_ibrt_ui_test_cmd_handler((unsigned char *)ibrt_cmd,
                                     strlen(ibrt_cmd) + 1);
      } break;
      default:
        break;
      }
      app_ibrt_peripheral_mailbox_free(msg_p);
    }
  }
}

void app_ibrt_peripheral_automate_test_handler(uint8_t *cmd_buf,
                                               uint32_t cmd_len) {
#ifdef BES_AUTOMATE_TEST
  AUTO_TEST_CMD_T *test_cmd = (AUTO_TEST_CMD_T *)cmd_buf;
  static uint8_t last_group_code = 0xFF;
  static uint8_t last_operation_code = 0xFF;

  // TRACE(4, "%s group 0x%x op 0x%x times %d len %d", __func__,
  // test_cmd->group_code, test_cmd->opera_code, test_cmd->test_times,
  // test_cmd->param_len);
  // TRACE(2, "last group 0x%x last op 0x%x", last_group_code,
  // last_operation_code);
  if (last_group_code != test_cmd->group_code ||
      last_operation_code != test_cmd->opera_code) {
    for (uint8_t i = 0; i < test_cmd->test_times; i++) {
      last_group_code = test_cmd->group_code;
      last_operation_code = test_cmd->opera_code;
      app_ibrt_ui_automate_test_cmd_handler(
          test_cmd->group_code, test_cmd->opera_code, test_cmd->param,
          test_cmd->param_len);
    }
  }
  app_ibrt_peripheral_heap_free(cmd_buf);
#endif
}

extern "C" void app_ibrt_peripheral_automate_test(const char *ibrt_cmd,
                                                  uint32_t cmd_len) {
#ifdef BES_AUTOMATE_TEST
  uint16_t crc16_rec = 0;
  uint16_t crc16_result = 0;
  uint8_t *cmd_buf = NULL;
  uint32_t _cmd_data_len = 0;
  uint32_t _cmd_data_min_len =
      sizeof(AUTO_TEST_CMD_T) + AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
  TWS_PD_MSG_BLOCK msg;

  if (ibrt_cmd && cmd_len >= _cmd_data_min_len &&
      cmd_len <= (_cmd_data_min_len + AUTOMATE_TEST_CMD_PARAM_MAX_LEN)) {
    _cmd_data_len = cmd_len - AUTOMATE_TEST_CMD_CRC_RECORD_LEN;
    crc16_rec = *(uint16_t *)(&ibrt_cmd[_cmd_data_len]);
    crc16_result =
        _crc16(crc16_result, (const unsigned char *)ibrt_cmd, _cmd_data_len);
    // DUMP8("0x%x ", ibrt_cmd, cmd_len);
    // TRACE(4, "%s crc16 rec 0x%x result 0x%x buf_len %d", __func__, crc16_rec,
    // crc16_result, cmd_len);
    if (crc16_rec == crc16_result && app_ibrt_auto_test_started) {
      app_ibrt_auto_test_inform_cmd_received(ibrt_cmd[0], ibrt_cmd[1]);
      cmd_buf = (uint8_t *)app_ibrt_peripheral_heap_cmalloc(_cmd_data_len);
      memcpy(cmd_buf, ibrt_cmd, _cmd_data_len);
      msg.msg_body.message_id = 0xfe;
      msg.msg_body.message_Param0 = (uint32_t)cmd_buf;
      msg.msg_body.message_Param1 = _cmd_data_len;
      app_ibrt_peripheral_mailbox_put(&msg);
    }
    return;
  }

  // ASSERT(0, "%s ibrt_cmd %p cmd_len %d", __func__, ibrt_cmd, cmd_len);
#endif
}

extern "C" void app_ibrt_peripheral_perform_test(const char *ibrt_cmd) {
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 0xff;
  memcpy(&msg.msg_body.message_Param0, ibrt_cmd + 0, 4);
  memcpy(&msg.msg_body.message_Param1, ibrt_cmd + 4, 4);
  memcpy(&msg.msg_body.message_Param2, ibrt_cmd + 8, 4);
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run0(uint32_t ptr) {
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 0;
  msg.msg_body.message_ptr = ptr;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run1(uint32_t ptr, uint32_t param0) {
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 1;
  msg.msg_body.message_ptr = ptr;
  msg.msg_body.message_Param0 = param0;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_run2(uint32_t ptr, uint32_t param0, uint32_t param1) {
  TWS_PD_MSG_BLOCK msg;
  msg.msg_body.message_id = 2;
  msg.msg_body.message_ptr = ptr;
  msg.msg_body.message_Param0 = param0;
  msg.msg_body.message_Param1 = param1;
  app_ibrt_peripheral_mailbox_put(&msg);
}

void app_ibrt_peripheral_thread_init(void) {
  if (app_ibrt_peripheral_mailbox_init())
    return;

  app_ibrt_peripheral_tid =
      osThreadCreate(osThread(app_ibrt_peripheral_thread), NULL);
  if (app_ibrt_peripheral_tid == NULL) {
    TRACE(0, "Failed to Create app_ibrt_peripheral_thread\n");
    return;
  }

#ifdef BES_AUTOMATE_TEST
  app_ibrt_peripheral_heap_init();
#endif
  return;
}
#endif
