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
#ifdef __PC_CMD_UART__
#include "app_cmd.h"
#include "app_thread.h"
#include "audio_process.h"
#include "cmsis_os.h"
#include "hal_cmd.h"
#include "hal_trace.h"
#include "list.h"
#include "string.h"

#define APP_CMD_TRACE(s, ...) TRACE(s, ##__VA_ARGS__)

void cmd_event_process(hal_cmd_rx_status_t status) {
  APP_CMD_TRACE(1, "%s", __func__);
  APP_MESSAGE_BLOCK msg;
  msg.mod_id = APP_MODUAL_CMD;
  msg.msg_body.message_id = status;
  msg.msg_body.message_ptr = (uint32_t)NULL;
  app_mailbox_put(&msg);
  return;
}

static int app_cmd_handle_process(APP_MESSAGE_BODY *msg_body) {
  hal_cmd_run((hal_cmd_rx_status_t)msg_body->message_id);
  return 0;
}

uint8_t app_cmd_flag = 0;

void app_cmd_open(void) {
  APP_CMD_TRACE(1, "%s", __func__);

  app_cmd_flag = 1;

  app_set_threadhandle(APP_MODUAL_CMD, app_cmd_handle_process);
  hal_cmd_set_callback(cmd_event_process);
  hal_cmd_open();
  return;
}

void app_cmd_close(void) {
  APP_CMD_TRACE(1, "%s", __func__);
  if (app_cmd_flag) {
    app_cmd_flag = 0;
    hal_cmd_close();
    app_set_threadhandle(APP_MODUAL_CMD, NULL);
  }
  return;
}
#endif
