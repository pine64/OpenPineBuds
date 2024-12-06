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

#include "app_tota_anc.h"
#include "anc_parse_data.h"
#include "app_spp_tota.h"
#include "app_tota_cmd_code.h"
#include "cmsis_os.h"
#include <stdio.h>

osTimerId app_check_send_synccmd_timer = NULL;
static void app_synccmd_timehandler(void const *param);
osTimerDef(APP_SYNCCMD, app_synccmd_timehandler);

static void app_synccmd_timehandler(void const *param) {
  send_sync_cmd_to_tool();
}

APP_TOTA_CMD_RET_STATUS_E app_anc_tota_cmd_received(uint8_t *ptrData,
                                                    uint32_t dataLength) {
  TOTA_LOG_DBG(2, "[%s] length:%d", __func__, dataLength);
  TOTA_LOG_DBG(1, "[%s] data:", __func__);
  TOTA_LOG_DUMP("0x%02x ", ptrData, dataLength);

  anc_handle_received_data(ptrData, dataLength);

  if (get_send_sync_flag() == 1) {
    osTimerStop(app_check_send_synccmd_timer);
  }
  return TOTA_NO_ERROR;
}

static bool is_connected = false;

/*-----------------------------------------------------------------------------*/

static void _tota_spp_connected(void);
static void _tota_spp_disconnected(void);
static void _tota_spp_tx_done(void);
static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len);

static tota_callback_func_t s_func = {_tota_spp_connected,
                                      _tota_spp_disconnected, _tota_spp_tx_done,
                                      _tota_spp_data_receive_handle};

static APP_TOTA_MODULE_E s_module = APP_TOTA_ANC;

void app_tota_anc_init() {
  tota_callback_module_register(s_module, s_func);
  reset_programmer_state(&g_buf, &g_len);

  if (app_check_send_synccmd_timer == NULL)
    app_check_send_synccmd_timer =
        osTimerCreate(osTimer(APP_SYNCCMD), osTimerPeriodic, NULL);
}

static void _tota_spp_connected(void) {
  anc_data_buff_init();
  // add a send sync timer
  osTimerStop(app_check_send_synccmd_timer);
  osTimerStart(app_check_send_synccmd_timer, 2000);
  is_connected = true;
}

static void _tota_spp_disconnected(void) {
  anc_data_buff_deinit();
  osTimerStop(app_check_send_synccmd_timer);
  is_connected = false;
}

static void _tota_spp_tx_done(void) {
  if (is_connected)
    // TODO:
    ; // bulk_read_done();
}

static void _tota_spp_data_receive_handle(uint8_t *buf, uint32_t len) {
  app_anc_tota_cmd_received(buf, len);
}

/*-----------------------------------------------------------------------------*/
