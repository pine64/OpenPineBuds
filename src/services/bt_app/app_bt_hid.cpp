/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#ifdef BTIF_HID_DEVICE
#include "analog.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_status_ind.h"
#include "bluetooth.h"
#include "cmsis_os.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include <stdio.h>
#if defined(NEW_NV_RECORD_ENABLED)
#include "nvrecord_bt.h"
#endif
#include "app.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_bt_hid.h"
#include "app_tws_ibrt.h"
#include "apps.h"
#include "besbt.h"
#include "besbt_cfg.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "bt_if.h"
#include "btapp.h"
#include "cqueue.h"
#include "hid_api.h"
#include "me_api.h"
#include "os_api.h"
#include "resources.h"

static bool shutter_mode = false;
static bool send_capture_pending = false;

#define APP_BT_HID_DELAY_SEND_CAPTURE_MS 500
osTimerId app_bt_hid_delay_send_timer_id = 0;
static void app_bt_hid_delay_send_timer_handler(void const *param);
osTimerDef(APP_BT_HID_DELAY_SEND_CAPTURE_TIMER,
           app_bt_hid_delay_send_timer_handler);

extern struct BT_DEVICE_T app_bt_device;

static void app_bt_hid_delay_send_timer_handler(void const *param) {
  hid_channel_t chan = (hid_channel_t)param;
  app_bt_start_custom_function_in_bt_thread((uint32_t)&chan, (uint32_t)NULL,
                                            (uint32_t)app_bt_hid_send_capture);
}

static void app_bt_hid_delay_send_capture(hid_channel_t chan) {
  if (app_bt_hid_delay_send_timer_id) {
    osTimerStop(app_bt_hid_delay_send_timer_id);
    osTimerDelete(app_bt_hid_delay_send_timer_id);
    app_bt_hid_delay_send_timer_id = 0;
  }

  app_bt_hid_delay_send_timer_id = osTimerCreate(
      osTimer(APP_BT_HID_DELAY_SEND_CAPTURE_TIMER), osTimerOnce, chan);

  if (!app_bt_hid_delay_send_timer_id) {
    TRACE(1, "%s create timer failed", __func__);
    return;
  }

  TRACE(2, "%s channel %p", __func__, chan);

  osTimerStart(app_bt_hid_delay_send_timer_id,
               APP_BT_HID_DELAY_SEND_CAPTURE_MS);
}

static void app_bt_hid_callback(hid_channel_t chan,
                                const hid_callback_parms_t *param) {
  btif_hid_callback_param_t *info = (btif_hid_callback_param_t *)param;

  TRACE(9, "%s channel %p event %d errno %02x %02x:%02x:%02x:%02x:%02x:%02x",
        __func__, chan, info->event, info->error_code, info->remote.address[0],
        info->remote.address[1], info->remote.address[2],
        info->remote.address[3], info->remote.address[4],
        info->remote.address[5]);

  switch (info->event) {
  case BTIF_HID_EVENT_CONN_OPENED:
    shutter_mode = true;
    if (send_capture_pending) {
      app_bt_hid_delay_send_capture(chan);
      send_capture_pending = false;
    }
    break;
  case BTIF_HID_EVENT_CONN_CLOSED:
    shutter_mode = false;
    break;
  default:
    break;
  }
  app_tws_ibrt_profile_callback(BTIF_APP_HID_PROFILE_ID, (void *)chan,
                                (void *)param);
}

void app_bt_hid_init(void) {
  // TRACE(2, "%s sink_enable %d", __func__, besbt_cfg.sink_enable);
  // if (besbt_cfg.sink_enable)
  {
    btif_hid_init(app_bt_hid_callback, HID_DEVICE_ROLE);

    for (int i = 0; i < BT_DEVICE_NUM; ++i) {
      app_bt_device.hid_channel[i] = btif_hid_channel_alloc();
      TRACE(2, "app_bt_hid_init device: %d hid_channle: %p", i,
            app_bt_device.hid_channel[i]);
    }
  }
}

void app_bt_hid_profile_connect(bt_bdaddr_t *bdaddr) {
  static bt_bdaddr_t remote;

  remote = *bdaddr;

  TRACE(7, "%s address %02x:%02x:%02x:%02x:%02x:%02x", __func__,
        remote.address[0], remote.address[1], remote.address[2],
        remote.address[3], remote.address[4], remote.address[5]);

  app_bt_start_custom_function_in_bt_thread((uint32_t)&remote, (uint32_t)NULL,
                                            (uint32_t)btif_hid_connect);
}

void app_bt_hid_profile_disconnect(hid_channel_t chnl) {
  TRACE(1, "%s channel %p", __func__, chnl);

  app_bt_start_custom_function_in_bt_thread((uint32_t)chnl, (uint32_t)NULL,
                                            (uint32_t)btif_hid_disconnect);
}

int app_bt_nvrecord_get_latest_device_addr(bt_bdaddr_t *addr) {
  btif_device_record_t record;
  int found_addr_count = 0;
  int paired_dev_count = nv_record_get_paired_dev_count();

  if (paired_dev_count > 0 &&
      BT_STS_SUCCESS == nv_record_enum_dev_records(0, &record)) {
    *addr = record.bdAddr;
    found_addr_count = 1;
  }

  return found_addr_count;
}

void app_bt_hid_enter_shutter_mode(void) {
  bt_bdaddr_t remote;

  TRACE(1, "%s", __func__);

  if (!app_bt_nvrecord_get_latest_device_addr(&remote)) {
    TRACE(1, "%s latest device not found", __func__);
    return;
  }

  if (!shutter_mode) {
    app_bt_hid_profile_connect(&remote);
    shutter_mode = true;
  } else {
    TRACE(1, "%s already in shutter mode", __func__);
  }
}

void app_bt_hid_exit_shutter_mode(void) {
  hid_channel_t chnl = NULL;
  int i = 0;

  TRACE(0, "%s", __func__);

  for (; i < BT_DEVICE_NUM; ++i) {
    chnl = app_bt_device.hid_channel[i];
    if (btif_hid_is_connected(chnl)) {
      app_bt_hid_profile_disconnect(chnl);
    }
  }

  shutter_mode = false;
}

void app_bt_hid_send_capture(hid_channel_t chnl) {
  for (int i = 0; i < BT_DEVICE_NUM; ++i) {
    if (btif_hid_is_connected(app_bt_device.hid_channel[i])) {
      chnl = app_bt_device.hid_channel[i];
      break;
    }
  }

  TRACE(3, "%s channel %p state %d", __func__, chnl, btif_hid_get_state(chnl));

  if (btif_hid_is_connected(chnl)) {
    send_capture_pending = false;

    btif_hid_keyboard_input_report(chnl, HID_MOD_KEY_NULL, HID_KEY_CODE_ENTER);
    btif_hid_keyboard_input_report(chnl, HID_MOD_KEY_NULL, HID_KEY_CODE_NULL);

    btif_hid_keyboard_send_ctrl_key(chnl, HID_CTRL_KEY_VOLUME_INC);
    btif_hid_keyboard_send_ctrl_key(chnl, HID_CTRL_KEY_NULL);
  } else {
    send_capture_pending = true;

    app_bt_hid_enter_shutter_mode();
  }
}

#endif /* BTIF_HID_DEVICE */
