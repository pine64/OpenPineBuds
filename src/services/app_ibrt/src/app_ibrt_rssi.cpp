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
//#include "mbed.h"
#if defined(IBRT)
#include <assert.h>
#include <stdio.h>

#include "a2dp_decoder.h"
#include "app_ibrt_if.h"
#include "app_ibrt_rssi.h"
#include "app_tws_ctrl_thread.h"
#include "hal_trace.h"

#define RSSI_WINDOW_SIZE 30

static tota_stutter_t g_stutter = {
    0,
};
extern float a2dp_audio_get_sample_reference(void);
extern int8_t a2dp_audio_get_current_buf_size(void);
extern diff_us_t *app_tws_ibrt_audio_sync_get_diff_us(void);

struct rssi_window_struct {
  int8_t buf[RSSI_WINDOW_SIZE];
  uint8_t index;
};

extern osTimerId ibrt_ui_check_roleswitch_timer_id;
static struct rssi_window_struct tws_rssi_window = {0};
static struct rssi_window_struct mobile_rssi_window = {0};
static bool need_roleswitch_with_rssi_bak = false;

static void rssi_window_push(struct rssi_window_struct *p, int8_t data) {
  if (p == NULL) {
    return;
  }

  if (p->index < RSSI_WINDOW_SIZE) {
    for (uint8_t i = p->index; i > 0; i--) {
      p->buf[i] = p->buf[i - 1];
    }
    p->buf[0] = data;
    p->index++;
  } else {
    for (uint8_t i = p->index - 1; i > 0; i--) {
      p->buf[i] = p->buf[i - 1];
    }
    p->buf[0] = data;
  }
}

void app_ibrt_ui_rssi_reset(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  memset(&tws_rssi_window, 0, sizeof(rssi_window_struct));
  memset(&mobile_rssi_window, 0, sizeof(rssi_window_struct));
  need_roleswitch_with_rssi_bak = false;
  p_ibrt_ctrl->raw_rssi.agc_idx0 = 0;
  p_ibrt_ctrl->raw_rssi.rssi0 = 0;
  p_ibrt_ctrl->raw_rssi.rssi0_max = 0x7f;
  p_ibrt_ctrl->raw_rssi.rssi0_min = 0x80;
  p_ibrt_ctrl->raw_rssi.agc_idx1 = 0;
  p_ibrt_ctrl->raw_rssi.rssi1 = 0;
  p_ibrt_ctrl->raw_rssi.rssi1_max = 0x7f;
  p_ibrt_ctrl->raw_rssi.rssi1_min = 0x80;
  p_ibrt_ctrl->raw_rssi.rssi2 = 0;
  p_ibrt_ctrl->raw_rssi.rssi2_max = 0x7f;
  p_ibrt_ctrl->raw_rssi.rssi2_min = 0x80;
  p_ibrt_ctrl->raw_rssi.ser = 0;
  p_ibrt_ctrl->raw_rssi.rx_data_sum = 0;

  p_ibrt_ctrl->peer_raw_rssi.agc_idx0 = 0;
  p_ibrt_ctrl->peer_raw_rssi.rssi0 = 0;
  p_ibrt_ctrl->peer_raw_rssi.rssi0_max = 0x7f;
  p_ibrt_ctrl->peer_raw_rssi.rssi0_min = 0x80;
  p_ibrt_ctrl->peer_raw_rssi.agc_idx1 = 0;
  p_ibrt_ctrl->peer_raw_rssi.rssi1 = 0;
  p_ibrt_ctrl->peer_raw_rssi.rssi1_max = 0x7f;
  p_ibrt_ctrl->peer_raw_rssi.rssi1_min = 0x80;
  p_ibrt_ctrl->peer_raw_rssi.rssi2 = 0;
  p_ibrt_ctrl->peer_raw_rssi.rssi2_max = 0x7f;
  p_ibrt_ctrl->peer_raw_rssi.rssi2_min = 0x80;
  p_ibrt_ctrl->peer_raw_rssi.ser = 0;
  p_ibrt_ctrl->peer_raw_rssi.rx_data_sum = 0;
  p_ibrt_ctrl->role_switch_debonce_time = 0;
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_check_roleswitch_timer_cb
 Description  : close bt scan when timeout
 Input        : current_evt
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/17
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_rssi_process(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_tws_link_connected()) {
    rx_agc_t tws_agc = {0};
    bt_drv_reg_op_read_rssi_in_dbm(p_ibrt_ctrl->tws_conhandle, &tws_agc);
    rssi_window_push(&tws_rssi_window, tws_agc.rssi);
    if (tws_rssi_window.index >= RSSI_WINDOW_SIZE) {
      int32_t tws_rssi_sum = 0;
      for (uint8_t i = 0; i < RSSI_WINDOW_SIZE; i++) {
        tws_rssi_sum += tws_rssi_window.buf[i];
      }

      p_ibrt_ctrl->raw_rssi.rssi1 = tws_rssi_sum / RSSI_WINDOW_SIZE;
      p_ibrt_ctrl->raw_rssi.agc_idx1 = tws_agc.rxgain;

      if (p_ibrt_ctrl->raw_rssi.rssi1 >= p_ibrt_ctrl->raw_rssi.rssi1_max)
        p_ibrt_ctrl->raw_rssi.rssi1_max = p_ibrt_ctrl->raw_rssi.rssi1;
      if (p_ibrt_ctrl->raw_rssi.rssi1 <= p_ibrt_ctrl->raw_rssi.rssi1_min)
        p_ibrt_ctrl->raw_rssi.rssi1_min = p_ibrt_ctrl->raw_rssi.rssi1;
    } else {
      p_ibrt_ctrl->raw_rssi.rssi1 = 0;
      p_ibrt_ctrl->raw_rssi.rssi1_max = 0x80;
      p_ibrt_ctrl->raw_rssi.rssi1_min = 0x7f;
      p_ibrt_ctrl->raw_rssi.agc_idx1 = 0;
    }
  }

  bool mobile_link_flag = app_tws_ibrt_mobile_link_connected();
  bool slave_ibrt_link_flag = app_tws_ibrt_slave_ibrt_link_connected();

  if (mobile_link_flag || slave_ibrt_link_flag) {
    rx_agc_t mobile_agc = {0};
    if (mobile_link_flag) {
      bt_drv_reg_op_read_rssi_in_dbm(p_ibrt_ctrl->mobile_conhandle,
                                     &mobile_agc);
    } else if (slave_ibrt_link_flag) {
      bt_drv_reg_op_read_rssi_in_dbm(p_ibrt_ctrl->ibrt_conhandle, &mobile_agc);
    }
    rssi_window_push(&mobile_rssi_window, mobile_agc.rssi);
    if (mobile_rssi_window.index >= RSSI_WINDOW_SIZE) {
      int32_t mobile_rssi_sum = 0;
      for (uint8_t i = 0; i < RSSI_WINDOW_SIZE; i++) {
        mobile_rssi_sum += mobile_rssi_window.buf[i];
      }

      p_ibrt_ctrl->raw_rssi.rssi0 = mobile_rssi_sum / RSSI_WINDOW_SIZE;
      p_ibrt_ctrl->raw_rssi.agc_idx0 = mobile_agc.rxgain;

      if (p_ibrt_ctrl->raw_rssi.rssi0 >= p_ibrt_ctrl->raw_rssi.rssi0_max)
        p_ibrt_ctrl->raw_rssi.rssi0_max = p_ibrt_ctrl->raw_rssi.rssi0;
      if (p_ibrt_ctrl->raw_rssi.rssi0 <= p_ibrt_ctrl->raw_rssi.rssi0_min)
        p_ibrt_ctrl->raw_rssi.rssi0_min = p_ibrt_ctrl->raw_rssi.rssi0;
    } else {
      p_ibrt_ctrl->raw_rssi.rssi0 = 0;
      p_ibrt_ctrl->raw_rssi.rssi0_max = 0x80;
      p_ibrt_ctrl->raw_rssi.rssi0_min = 0x7f;
      p_ibrt_ctrl->raw_rssi.agc_idx0 = 0;
    }
  }
  // TRACE(5,"mobile -> %d:%d tws -> %d:%d SER:%d/100",
  // p_ibrt_ctrl->raw_rssi.rssi0, p_ibrt_ctrl->raw_rssi.agc_idx0,
  // p_ibrt_ctrl->raw_rssi.rssi1,
  // p_ibrt_ctrl->raw_rssi.agc_idx1,p_ibrt_ctrl->raw_rssi.ser);
}
/*****************************************************************************
 Prototype    : app_ibrt_ui_tws_switch_according_rssi_needed
 Description  :
 Input        : void
 Output       : bool
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/5
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
bool app_ibrt_ui_tws_switch_according_rssi_needed(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  if ((p_ibrt_ctrl->raw_rssi.rssi0 != IBRT_UI_INVALID_RSSI) &&
      (p_ibrt_ctrl->peer_raw_rssi.rssi0 != IBRT_UI_INVALID_RSSI) &&
      (p_ibrt_ctrl->peer_raw_rssi.rssi0 >= IBRT_UI_MIN_RSSI) &&
      (p_ibrt_ctrl->raw_rssi.rssi0 >= IBRT_UI_MIN_RSSI)) {
    int8_t rssi_d_value_diff =
        p_ibrt_ctrl->peer_raw_rssi.rssi0 - p_ibrt_ctrl->raw_rssi.rssi0;

    if (rssi_d_value_diff >= p_ibrt_ui->config.rssi_threshold) {
      // local RSSI is stronger than peer and local role is SLAVE
      return true;
    }
  }
  return false;
}

/*****************************************************************************
 Prototype    : app_ibrt_ui_check_roleswitch_timer_cb
 Description  : close bt scan when timeout
 Input        : current_evt
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/17
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ui_check_roleswitch_timer_cb(void const *current_evt) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  // app_ibrt_ui_rssi_process();

  if (app_tws_ibrt_mobile_link_connected() &&
      app_ibrt_ui_is_profile_exchanged() &&
      (p_ibrt_ctrl->tws_mode == BTIF_BLM_ACTIVE_MODE)) {
    tws_ctrl_send_cmd(APP_TWS_CMD_GET_PEER_MOBILE_RSSI, NULL, 0);
  }

  osTimerStart(ibrt_ui_check_roleswitch_timer_id,
               p_ibrt_ui->config.rssi_monitor_timeout);
}

/*****************************************************************************
 Prototype    : app_ibrt_get_peer_mobile_rssi
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/4
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_get_peer_mobile_rssi(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_GET_PEER_MOBILE_RSSI, p_buff,
                                length);
}

/*****************************************************************************
 Prototype    : app_ibrt_get_peer_mobile_rssi_handler
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/4
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_get_peer_mobile_rssi_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                           uint16_t length) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  diff_us_t *p_diff_us = app_tws_ibrt_audio_sync_get_diff_us();

  if (app_tws_ibrt_slave_ibrt_link_connected() &&
      app_ibrt_ui_is_profile_exchanged() &&
      (p_ibrt_ctrl->tws_mode == BTIF_BLM_ACTIVE_MODE)) {
    app_ui_rssi_battery_info_t rssi_battery_buffer;
    rssi_battery_buffer.battery_volt = p_ibrt_ctrl->local_battery_volt;
    rssi_battery_buffer.raw_rssi = p_ibrt_ctrl->raw_rssi;
    rssi_battery_buffer.mobile_conhandle = p_ibrt_ctrl->ibrt_conhandle;
    rssi_battery_buffer.tws_conhandle = p_ibrt_ctrl->tws_conhandle;
    rssi_battery_buffer.mobile_diff_us = p_diff_us->mobile_diff_us;
    rssi_battery_buffer.tws_diff_us = p_diff_us->tws_diff_us;
    rssi_battery_buffer.cur_buf_size = a2dp_audio_get_current_buf_size();

    tws_ctrl_send_rsp(APP_TWS_CMD_GET_PEER_MOBILE_RSSI, rsp_seq,
                      (uint8_t *)&rssi_battery_buffer,
                      sizeof(app_ui_rssi_battery_info_t));
  }
}

/*****************************************************************************
 Prototype    : app_ibrt_get_peer_mobile_rssi_rsp_handler
 Description  :
 Input        : uint8_t *p_buff
                uint16_t length
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/6/4
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
static void app_ibrt_print_rssi_info(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  TRACE(0, "RSSI local mobile : local tws <> peer mobile : peer tws");
  TRACE(19,
        "RSSI %d %d %d %d : %d %d %d %d %d/100 <> %d %d %d %d : %d %d %d %d "
        "%d/100 %d",
        p_ibrt_ctrl->raw_rssi.agc_idx0, p_ibrt_ctrl->raw_rssi.rssi0,
        p_ibrt_ctrl->raw_rssi.rssi0_max, p_ibrt_ctrl->raw_rssi.rssi0_min,
        p_ibrt_ctrl->raw_rssi.agc_idx1, p_ibrt_ctrl->raw_rssi.rssi1,
        p_ibrt_ctrl->raw_rssi.rssi1_max, p_ibrt_ctrl->raw_rssi.rssi1_min,
        p_ibrt_ctrl->raw_rssi.ser, p_ibrt_ctrl->peer_raw_rssi.agc_idx0,
        p_ibrt_ctrl->peer_raw_rssi.rssi0, p_ibrt_ctrl->peer_raw_rssi.rssi0_max,
        p_ibrt_ctrl->peer_raw_rssi.rssi0_min,
        p_ibrt_ctrl->peer_raw_rssi.agc_idx1, p_ibrt_ctrl->peer_raw_rssi.rssi1,
        p_ibrt_ctrl->peer_raw_rssi.rssi1_max,
        p_ibrt_ctrl->peer_raw_rssi.rssi1_min, p_ibrt_ctrl->peer_raw_rssi.ser,
        p_ibrt_ctrl->current_role);
}

void app_ibrt_get_peer_mobile_rssi_rsp_handler(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_mobile_link_connected() &&
      app_ibrt_ui_is_profile_exchanged() &&
      (p_ibrt_ctrl->tws_mode == BTIF_BLM_ACTIVE_MODE)) {
    if (p_ibrt_ctrl->role_switch_debonce_time == 0) {
      app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
      app_ui_rssi_battery_info_t rssi_battery_info =
          *(app_ui_rssi_battery_info_t *)p_buff;
      p_ibrt_ctrl->peer_mobile_conhandle = rssi_battery_info.mobile_conhandle;
      p_ibrt_ctrl->peer_tws_conhandle = rssi_battery_info.tws_conhandle;
      p_ibrt_ctrl->peer_battery_volt = rssi_battery_info.battery_volt;
      p_ibrt_ctrl->peer_raw_rssi = rssi_battery_info.raw_rssi;
      p_ibrt_ctrl->mobile_diff_us = rssi_battery_info.mobile_diff_us;
      p_ibrt_ctrl->tws_diff_us = rssi_battery_info.tws_diff_us;
      p_ibrt_ctrl->cur_buf_size = rssi_battery_info.cur_buf_size;

      app_ibrt_print_rssi_info();

      if (p_ibrt_ui->config.tws_switch_according_to_rssi_value) {
        bool need_roleswitch_with_rssi =
            app_ibrt_ui_tws_switch_according_rssi_needed();

        if (need_roleswitch_with_rssi != need_roleswitch_with_rssi_bak) {
          if (need_roleswitch_with_rssi) {
            TRACE(2,
                  "ibrt_ui_log:one headset far away cause tws switch used rssi "
                  "%d %d",
                  p_ibrt_ctrl->raw_rssi.rssi0,
                  p_ibrt_ctrl->peer_raw_rssi.rssi0);
            app_ibrt_ui_tws_switch();
            p_ibrt_ctrl->role_switch_debonce_time =
                p_ibrt_ui->config.role_switch_timer_threshold;
          }
          need_roleswitch_with_rssi_bak = need_roleswitch_with_rssi;
        }
      }
    } else {
      p_ibrt_ctrl->role_switch_debonce_time--;
    }
  }
}

void app_ibrt_rssi_get_stutter(uint8_t *data, uint32_t *data_len) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint8_t temp;

  data[0] = p_ibrt_ctrl->raw_rssi.agc_idx0;
  data[1] = p_ibrt_ctrl->raw_rssi.rssi0;
  data[2] = p_ibrt_ctrl->raw_rssi.rssi0_max;
  data[3] = p_ibrt_ctrl->raw_rssi.rssi0_min;

  data[4] = p_ibrt_ctrl->raw_rssi.agc_idx1;
  data[5] = p_ibrt_ctrl->raw_rssi.rssi1;
  data[6] = p_ibrt_ctrl->raw_rssi.rssi1_max;
  data[7] = p_ibrt_ctrl->raw_rssi.rssi1_min;
  data[8] = p_ibrt_ctrl->raw_rssi.ser;

  data[9] = p_ibrt_ctrl->peer_raw_rssi.agc_idx0;
  data[10] = p_ibrt_ctrl->peer_raw_rssi.rssi0;
  data[11] = p_ibrt_ctrl->peer_raw_rssi.rssi0_max;
  data[12] = p_ibrt_ctrl->peer_raw_rssi.rssi0_min;

  data[13] = p_ibrt_ctrl->peer_raw_rssi.agc_idx1;
  data[14] = p_ibrt_ctrl->peer_raw_rssi.rssi1;
  data[15] = p_ibrt_ctrl->peer_raw_rssi.rssi1_max;
  data[16] = p_ibrt_ctrl->peer_raw_rssi.rssi1_min;
  data[17] = p_ibrt_ctrl->peer_raw_rssi.ser;

  g_stutter.sample_ref = a2dp_audio_get_sample_reference();
  g_stutter.cur_buf_size_l = a2dp_audio_get_current_buf_size();

  TRACE(2, "diff us: mobile:%d,tws:%d.", p_ibrt_ctrl->mobile_diff_us,
        p_ibrt_ctrl->tws_diff_us);

  TRACE(
      19,
      "%d %d %d %d : %d %d %d %d %d/100 <> %d %d %d %d : %d %d %d %d %d/100 %d",
      p_ibrt_ctrl->raw_rssi.agc_idx0, p_ibrt_ctrl->raw_rssi.rssi0,
      p_ibrt_ctrl->raw_rssi.rssi0_max, p_ibrt_ctrl->raw_rssi.rssi0_min,
      p_ibrt_ctrl->raw_rssi.agc_idx1, p_ibrt_ctrl->raw_rssi.rssi1,
      p_ibrt_ctrl->raw_rssi.rssi1_max, p_ibrt_ctrl->raw_rssi.rssi1_min,
      p_ibrt_ctrl->raw_rssi.ser, p_ibrt_ctrl->peer_raw_rssi.agc_idx0,
      p_ibrt_ctrl->peer_raw_rssi.rssi0, p_ibrt_ctrl->peer_raw_rssi.rssi0_max,
      p_ibrt_ctrl->peer_raw_rssi.rssi0_min, p_ibrt_ctrl->peer_raw_rssi.agc_idx1,
      p_ibrt_ctrl->peer_raw_rssi.rssi1, p_ibrt_ctrl->peer_raw_rssi.rssi1_max,
      p_ibrt_ctrl->peer_raw_rssi.rssi1_min, p_ibrt_ctrl->peer_raw_rssi.ser,
      p_ibrt_ctrl->current_role);

  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    for (int i = 0; i < 9; i++) {
      temp = data[i + 9];
      data[i + 9] = data[i];
      data[i] = temp;
    }
  }
  data[18] = p_ibrt_ctrl->current_role;

  data[19] = bt_drv_reg_op_fa_gain_direct_get();
  data[20] = g_stutter.cur_buf_size_l;  // L buffer size
  data[21] = p_ibrt_ctrl->cur_buf_size; // R buffer size
  // sample_reference
  data[22] = *((int8_t *)&g_stutter.sample_ref);
  data[23] = *((int8_t *)&g_stutter.sample_ref + 1);
  data[24] = *((int8_t *)&g_stutter.sample_ref + 2);
  data[25] = *((int8_t *)&g_stutter.sample_ref + 3);
  // mobile diff / us
  data[26] = *((int8_t *)&p_ibrt_ctrl->mobile_diff_us);
  data[27] = *((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 1);
  data[28] = *((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 2);
  data[29] = *((int8_t *)&p_ibrt_ctrl->mobile_diff_us + 3);
  // tws diff / us
  data[30] = *((int8_t *)&p_ibrt_ctrl->tws_diff_us);
  data[31] = *((int8_t *)&p_ibrt_ctrl->tws_diff_us + 1);
  data[32] = *((int8_t *)&p_ibrt_ctrl->tws_diff_us + 2);
  data[33] = *((int8_t *)&p_ibrt_ctrl->tws_diff_us + 3);

  // L error_sum
  data[34] = *((int8_t *)&p_ibrt_ctrl->raw_rssi.ser);
  data[35] = *((int8_t *)&p_ibrt_ctrl->raw_rssi.ser + 1);
  // L Total_sum
  data[36] = *((int8_t *)&p_ibrt_ctrl->raw_rssi.rx_data_sum);
  data[37] = *((int8_t *)&p_ibrt_ctrl->raw_rssi.rx_data_sum + 1);
  // R error_sum
  data[38] = *((int8_t *)&p_ibrt_ctrl->peer_raw_rssi.ser);
  data[39] = *((int8_t *)&p_ibrt_ctrl->peer_raw_rssi.ser + 1);
  // R Total_sum
  data[40] = *((int8_t *)&p_ibrt_ctrl->peer_raw_rssi.rx_data_sum);
  data[41] = *((int8_t *)&p_ibrt_ctrl->peer_raw_rssi.rx_data_sum + 1);

  TRACE(1, "0x%08x", p_ibrt_ctrl->mobile_diff_us);
  TRACE(1, "0x%08x", p_ibrt_ctrl->tws_diff_us);
  TRACE(1, "0x%08x", p_ibrt_ctrl->raw_rssi.ser);
  TRACE(1, "0x%08x", p_ibrt_ctrl->raw_rssi.rx_data_sum);
  TRACE(1, "0x%08x", p_ibrt_ctrl->peer_raw_rssi.ser);
  TRACE(1, "0x%08x", p_ibrt_ctrl->peer_raw_rssi.rx_data_sum);

  TRACE(23,
        "%d %d %d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
        "%02x %02x %02x %02x %02x %02x %02x %02x",
        data[19], data[20], data[21], data[22], data[23], data[24], data[25],
        data[26], data[27], data[28], data[29], data[30], data[31], data[32],
        data[33], data[34], data[35], data[36], data[37], data[38], data[39],
        data[40], data[41]);
  *data_len = 42;
}

void app_ibrt_debug_parse(uint8_t *data, uint32_t data_len) {
  bool do_it = false;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint8_t *parse_p = data;

  TRACE(2, "%s len:%d", __func__, data_len);
  DUMP8("%02x ", data, data_len);

  if (p_ibrt_ctrl->audio_chnl_sel == A2DP_AUDIO_CHANNEL_SELECT_LCHNL) {
    if (*parse_p & 0x01) {
      do_it = true;
    }
  }
  if (p_ibrt_ctrl->audio_chnl_sel == A2DP_AUDIO_CHANNEL_SELECT_RCHNL) {
    if (*parse_p & 0x02) {
      do_it = true;
    }
  }
  parse_p++;
  if (do_it) {
    uint8_t funcCode = *parse_p;
    parse_p++;
    switch (funcCode) {
    case 1:
      if (app_tws_ibrt_mobile_link_connected()) {
        bt_drv_reg_op_dgb_link_gain_ctrl_set(p_ibrt_ctrl->mobile_conhandle, 0,
                                             *parse_p,
                                             *parse_p == 0xff ? 0 : 1);
      } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
        bt_drv_reg_op_dgb_link_gain_ctrl_set(
            p_ibrt_ctrl->ibrt_conhandle, 0, *parse_p, *parse_p == 0xff ? 0 : 1);
      }
      break;
    case 2:
      bt_drv_reg_op_dgb_link_gain_ctrl_set(p_ibrt_ctrl->tws_conhandle, 0,
                                           *parse_p, *parse_p == 0xff ? 0 : 1);
      break;
    case 3:
      bt_drv_reg_op_fa_gain_direct_set(*parse_p);
      break;
    case 4: {
      uint32_t data_format = 0;
      data_format = be_to_host32(parse_p);
      TRACE(1, "lowlayer_monitor len:%d", data_format);
      if (app_tws_ibrt_mobile_link_connected()) {
        btif_me_set_link_lowlayer_monitor(
            btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle),
            FLAG_START_DATA, REP_FORMAT_PACKET, data_format, 0);
      } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
        btif_me_set_link_lowlayer_monitor(
            btif_me_get_remote_device_by_handle(p_ibrt_ctrl->ibrt_conhandle),
            FLAG_START_DATA, REP_FORMAT_PACKET, data_format, 0);
      }
    } break;
    default:
      TRACE(1, "wrong cmd 0x%x", funcCode);
      break;
    }
  }
}
#endif
