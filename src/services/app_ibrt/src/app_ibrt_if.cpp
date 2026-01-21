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
#include "app_ibrt_if.h"
#include "a2dp_decoder.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_bt_func.h"
#include "app_bt_media_manager.h"
#include "app_bt_stream.h"
#include "app_ibrt_a2dp.h"
#include "app_ibrt_hf.h"
#include "app_ibrt_if_internal.h"
#include "app_ibrt_nvrecord.h"
#include "app_ibrt_ui_test.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "app_tws_ibrt_trace.h"
#include "app_tws_if.h"
#include "besaud_api.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "btapp.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "hal_location.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "nvrecord.h"
#include <string.h>
#ifdef BLE_ENABLE
#include "app_ble_include.h"
#endif

#ifdef IBRT_OTA
#include "ota_control.h"
#endif

#if defined(TOTA) || defined(TEST_OVER_THE_AIR_ENANBLED)
#include "app_spp_tota.h"
#endif

extern struct BT_DEVICE_T app_bt_device;

extern uint8_t avrcp_get_media_status(void);

#if defined(IBRT)

static uint32_t tws_sniff_block_start = 0;
static uint32_t tws_sniff_block_next_sec = 0;
void app_ibrt_if_register_tws_switch_needed_ind(
    APP_IBRT_IF_TWS_SWITCH_NEEDED_IND tws_switch_need_ind) {
  app_ibrt_ui_register_tws_switch_needed_ind(tws_switch_need_ind);
}

void app_ibrt_if_register_connect_mobile_needed_ind(
    APP_IBRT_IF_CONNECT_MOBILE_NEEDED_IND connect_moible_need_ind) {
  app_ibrt_ui_register_connect_mobile_needed_ind(connect_moible_need_ind);
}

void app_ibrt_if_register_vender_handler_ind(
    APP_IBRT_IF_VENDER_EVENT_HANDLER_IND handler) {
  app_ibrt_ui_register_vender_event_update_ind(handler);
}

void app_ibrt_if_register_global_handler_ind(
    APP_IBRT_IF_GLOBAL_HANDLER_IND handler) {
  app_ibrt_ui_register_global_handler_ind(handler);
}

void app_ibrt_if_register_global_event_update_ind(
    APP_IBRT_IF_GLOBAL_EVENT_UPDATE_IND handler) {
  app_ibrt_ui_register_global_event_update_ind(handler);
}

void app_ibrt_if_register_pairing_mode_ind(
    APP_IBRT_UI_PAIRING_MODE_HANDLER_IND set_callback,
    APP_IBRT_UI_PAIRING_MODE_HANDLER_IND clear_callback) {
  app_ibrt_ui_register_pairing_mode_ind(set_callback, clear_callback);
}

void app_ibrt_if_register_cmd_complete_ind(
    APP_IBRT_UI_CMD_COMPLETE_HANDLER_IND cmd_complete_ind) {
  app_ibrt_ui_register_cmd_complete_ind(cmd_complete_ind);
}

void app_ibrt_if_register_link_connected_ind(
    APP_IBRT_UI_CONNECTED_HANDLER_IND mobile_connected_ind,
    APP_IBRT_UI_CONNECTED_HANDLER_IND ibrt_connected_ind,
    APP_IBRT_UI_CONNECTED_HANDLER_IND tws_connected_ind) {
  app_ibrt_ui_register_link_connected_ind(
      mobile_connected_ind, ibrt_connected_ind, tws_connected_ind);
}

void app_ibrt_if_register_profile_state_change_ind(
    APP_IBRT_IF_PROFILE_STATE_CHANGE_IND handler) {
  app_ibrt_ui_register_profile_state_change_ind(handler);
}

typedef struct {
  ibrt_config_t ibrt_config;
  nvrec_btdevicerecord rec_mobile;
  nvrec_btdevicerecord rec_peer;
  uint8_t reserved __attribute__((aligned(4)));
  uint32_t crc;
} ibrt_config_ram_bak_t;

ibrt_config_ram_bak_t REBOOT_CUSTOM_PARAM_LOC ibrt_config_ram_bak;

int app_ibrt_if_config_keeper_clear(void) {
  memset(&ibrt_config_ram_bak, 0, sizeof(ibrt_config_ram_bak));
  return 0;
}

void app_ibrt_if_set_silent_mode(void) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  p_ibrt_ui->silent_mode = true;
  app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
}

void app_ibrt_if_clear_silent_mode(void) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  p_ibrt_ui->silent_mode = false;
  app_ibrt_ui_judge_scan_type(IBRT_RESTORE_SCAN, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

int app_ibrt_if_config_keeper_flush(void) {
  ibrt_config_ram_bak.crc =
      crc32(0, (uint8_t *)(&ibrt_config_ram_bak),
            (sizeof(ibrt_config_ram_bak_t) - sizeof(uint32_t)));
  TRACE(2, "%s crc:%08x", __func__, ibrt_config_ram_bak.crc);
  return 0;
}

int app_ibrt_if_volume_ptr_update(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  btif_remote_device_t *remDev = NULL;

  if (app_tws_ibrt_mobile_link_connected()) {
    remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->ibrt_conhandle);
  }

  if (remDev) {
    app_bt_stream_volume_ptr_update(
        (uint8_t *)btif_me_get_remote_device_bdaddr(remDev));
  }

  return 0;
}

void app_ibrt_if_pairing_mode_refresh(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (p_ibrt_ctrl->nv_role == IBRT_MASTER) {
    app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
  } else {
    if (!btif_besaud_is_connected()) {
      app_ibrt_ui_set_enter_pairing_mode(IBRT_TWS_PAIR_MODE_ENTER);
      app_ibrt_ui_judge_scan_type(IBRT_ENTER_PAIRING_MODE_TRIGGER, NO_LINK_TYPE,
                                  0);
    } else {
      tws_ctrl_send_cmd(APP_TWS_CMD_PAIRING_MODE_REFRESH, NULL, 0);
    }
  }
}

int app_ibrt_if_config_keeper_mobile_update(bt_bdaddr_t *addr) {
  nvrec_btdevicerecord *nv_record = NULL;
  nvrec_btdevicerecord *rambak_record = NULL;

  rambak_record = &ibrt_config_ram_bak.rec_mobile;

  if (!nv_record_btdevicerecord_find(addr, &nv_record)) {
    TRACE(1, "%s success", __func__);
    DUMP8("%02x ", nv_record->record.bdAddr.address,
          sizeof(nv_record->record.bdAddr.address));
    DUMP8("%02x ", nv_record->record.linkKey,
          sizeof(nv_record->record.linkKey));
    ibrt_config_ram_bak.ibrt_config.mobile_addr = *addr;
    *rambak_record = *nv_record;
    app_ibrt_if_config_keeper_flush();
    app_ibrt_if_volume_ptr_update();
  } else {
    TRACE(1, "%s failure", __func__);
  }

  return 0;
}

int app_ibrt_if_config_keeper_tws_update(bt_bdaddr_t *addr) {
  nvrec_btdevicerecord *nv_record = NULL;
  nvrec_btdevicerecord *rambak_record = NULL;

  rambak_record = &ibrt_config_ram_bak.rec_peer;

  if (!nv_record_btdevicerecord_find(addr, &nv_record)) {
    TRACE(1, "%s success", __func__);
    DUMP8("%02x ", nv_record->record.bdAddr.address,
          sizeof(nv_record->record.bdAddr.address));
    DUMP8("%02x ", nv_record->record.linkKey,
          sizeof(nv_record->record.linkKey));
    ibrt_config_ram_bak.ibrt_config.peer_addr = *addr;
    *rambak_record = *nv_record;
    app_ibrt_if_config_keeper_flush();
    app_ibrt_if_volume_ptr_update();
  } else {
    TRACE(1, "%s failure", __func__);
  }

  return 0;
}
/*
 * tws preparation before tws switch if needed
 */
#ifdef IBRT_OTA
extern void ota_control_send_start_role_switch(void);
#endif
bool app_ibrt_if_tws_switch_prepare_needed(uint32_t *wait_ms) {
  __attribute__((unused)) ibrt_ctrl_t *p_ibrt_ctrl =
      app_tws_ibrt_get_bt_ctrl_ctx();
  bool ret = false;

#ifdef BLE_ENABLE
  ble_callback_evnet_t event;
  event.evt_type = BLE_CALLBACK_RS_START;
  app_ble_core_global_callback_event(&event, NULL);
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
  p_ibrt_ctrl->ibrt_ai_role_switch_handle =
      app_ai_tws_role_switch_prepare(wait_ms);
  if (p_ibrt_ctrl->ibrt_ai_role_switch_handle) {
    p_ibrt_ctrl->ibrt_role_switch_handle_user |=
        (1 << IBRT_ROLE_SWITCH_USER_AI);
  }
#endif

#ifdef IBRT_OTA
  if (app_get_bes_ota_state()) {
    p_ibrt_ctrl->ibrt_role_switch_handle_user |=
        (1 << IBRT_ROLE_SWITCH_USER_OTA);
    *wait_ms = 800;
  }
#endif

  if (p_ibrt_ctrl->ibrt_role_switch_handle_user) {
    ret = true;
  }

  app_tws_if_role_switch_started_handler();

  TRACE(2, "tws_switch_prepare_needed=%d, wait_ms=%d", ret, *wait_ms);
  return ret;
}
/*
 * tws preparation before tws switch
 */
void app_ibrt_if_tws_swtich_prepare(void) {
#if defined(BISTO_ENABLED)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (p_ibrt_ctrl->ibrt_ai_role_switch_handle & (1 << AI_SPEC_GSOUND)) {
    app_ai_tws_role_switch();
  }
#endif

#ifdef IBRT_OTA
  ibrt_ctrl_t *ota_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if ((app_get_bes_ota_state()) &&
      (ota_ibrt_ctrl->ibrt_role_switch_handle_user &
       IBRT_ROLE_SWITCH_USER_OTA)) {
    app_set_ota_role_switch_initiator(true);
    bes_ota_send_role_switch_req();
  }
#endif
}
/*
 * notify UI SM tws preparation done
 */
extern osTimerId ibrt_ui_tws_switch_prepare_timer_id;
static void app_ibrt_if_tws_switch_prepare_done(IBRT_ROLE_SWITCH_USER_E user,
                                                uint32_t role) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (p_ibrt_ctrl->ibrt_role_switch_handle_user) {
#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    TRACE(4, "%s ai_handle 0x%x role %d%s", __func__,
          p_ibrt_ctrl->ibrt_ai_role_switch_handle, role,
          ai_spec_type2str((AI_SPEC_TYPE_E)role));

    if (user == IBRT_ROLE_SWITCH_USER_AI) {
      if (role >= AI_SPEC_COUNT) {
        TRACE(1, "%s role error", __func__);
        return;
      }
      if (!p_ibrt_ctrl->ibrt_ai_role_switch_handle) {
        TRACE(1, "%s ai_handle is 0", __func__);
        return;
      }

      p_ibrt_ctrl->ibrt_ai_role_switch_handle &= ~(1 << role);
      if (!p_ibrt_ctrl->ibrt_ai_role_switch_handle) {
        p_ibrt_ctrl->ibrt_role_switch_handle_user &= ~IBRT_ROLE_SWITCH_USER_AI;
      }
    } else
#endif
    {
      p_ibrt_ctrl->ibrt_role_switch_handle_user &= ~user;
    }

    if (!p_ibrt_ctrl->ibrt_role_switch_handle_user) {
      osTimerStop(ibrt_ui_tws_switch_prepare_timer_id);
      app_ibrt_ui_event_handler(IBRT_ACTION_TWS_SWITCH_PREPARE_COMPLETE,
                                IBRT_UI_NO_ERROR);
    }
  }
}

void app_ibrt_if_tws_switch_prepare_done_in_bt_thread(
    IBRT_ROLE_SWITCH_USER_E user, uint32_t role) {
  app_bt_start_custom_function_in_bt_thread(
      user, role, (uint32_t)app_ibrt_if_tws_switch_prepare_done);
}

int app_ibrt_if_config_keeper_resume(ibrt_config_t *config) {
  uint32_t crc;
  nvrec_btdevicerecord *nv_record = NULL;
  nvrec_btdevicerecord *rambak_record = NULL;
  bool mobile_check_ok = false;
  bool peer_check_ok = false;
  bool flash_need_flush = false;
  bt_bdaddr_t zeroAddr = {0, 0, 0, 0, 0, 0};

  crc = crc32(0, (uint8_t *)(&ibrt_config_ram_bak),
              (sizeof(ibrt_config_ram_bak_t) - sizeof(uint32_t)));

  TRACE(3, "%s start crc:%08x/%08x", __func__, ibrt_config_ram_bak.crc, crc);
  if (crc == ibrt_config_ram_bak.crc) {
    TRACE_IMM(1, "%s success", __func__);
    TRACE(1, "%s config loc", __func__);
    DUMP8("%02x ", config->local_addr.address, 6);
    TRACE(1, "%s config mobile", __func__);
    DUMP8("%02x ", config->mobile_addr.address, 6);
    TRACE(1, "%s config peer", __func__);
    DUMP8("%02x ", config->peer_addr.address, 6);

    rambak_record = &ibrt_config_ram_bak.rec_mobile;
    if (!nv_record_btdevicerecord_find(&config->mobile_addr, &nv_record)) {
      TRACE(1, "%s  find mobile", __func__);
      DUMP8("%02x ", nv_record->record.bdAddr.address,
            sizeof(nv_record->record.bdAddr.address));
      DUMP8("%02x ", nv_record->record.linkKey,
            sizeof(nv_record->record.linkKey));
      if (!memcmp(rambak_record->record.linkKey, nv_record->record.linkKey,
                  sizeof(nv_record->record.linkKey))) {
        TRACE(1, "%s  check mobile success", __func__);
        mobile_check_ok = true;
      }
    }
    if (!mobile_check_ok) {
      TRACE(1, "%s  check mobile failure", __func__);
      DUMP8("%02x ", rambak_record->record.bdAddr.address, sizeof(zeroAddr));
      DUMP8("%02x ", rambak_record->record.linkKey,
            sizeof(rambak_record->record.linkKey));
      if (memcmp(rambak_record->record.bdAddr.address, zeroAddr.address,
                 sizeof(zeroAddr))) {
        nv_record_add(section_usrdata_ddbrecord, rambak_record);
        config->mobile_addr = rambak_record->record.bdAddr;
        flash_need_flush = true;
        TRACE(1, "%s resume mobile", __func__);
      }
    }

    rambak_record = &ibrt_config_ram_bak.rec_peer;
    if (!nv_record_btdevicerecord_find(&config->peer_addr, &nv_record)) {
      TRACE(1, "%s  find tws peer", __func__);
      DUMP8("%02x ", nv_record->record.bdAddr.address,
            sizeof(nv_record->record.bdAddr.address));
      DUMP8("%02x ", nv_record->record.linkKey,
            sizeof(nv_record->record.linkKey));
      if (!memcmp(rambak_record->record.linkKey, nv_record->record.linkKey,
                  sizeof(nv_record->record.linkKey))) {
        TRACE(1, "%s  check tws peer success", __func__);
        peer_check_ok = true;
      }
    }
    if (!peer_check_ok) {
      TRACE(1, "%s  check tws peer failure", __func__);
      DUMP8("%02x ", rambak_record->record.bdAddr.address,
            sizeof(rambak_record->record.bdAddr));
      DUMP8("%02x ", rambak_record->record.linkKey,
            sizeof(rambak_record->record.linkKey));
      if (memcmp(rambak_record->record.bdAddr.address, zeroAddr.address,
                 sizeof(zeroAddr))) {
        nv_record_add(section_usrdata_ddbrecord, rambak_record);
        config->peer_addr = rambak_record->record.bdAddr;
        flash_need_flush = true;
        TRACE(1, "%s resume tws peer", __func__);
      }
    }
  }

  ibrt_config_ram_bak.ibrt_config = *config;
  rambak_record = &ibrt_config_ram_bak.rec_mobile;
  if (!nv_record_btdevicerecord_find(&config->mobile_addr, &nv_record)) {
    *rambak_record = *nv_record;
  } else {
    memset(rambak_record, 0, sizeof(nvrec_btdevicerecord));
  }
  rambak_record = &ibrt_config_ram_bak.rec_peer;
  if (!nv_record_btdevicerecord_find(&config->peer_addr, &nv_record)) {
    *rambak_record = *nv_record;
  } else {
    memset(rambak_record, 0, sizeof(nvrec_btdevicerecord));
  }
  app_ibrt_if_config_keeper_flush();
  if (flash_need_flush) {
    nv_record_flash_flush();
  }
  TRACE_IMM(2, "%s end crc:%08x", __func__, ibrt_config_ram_bak.crc);

  TRACE(1, "%s mobile", __func__);
  DUMP8("%02x ", ibrt_config_ram_bak.rec_mobile.record.bdAddr.address,
        sizeof(ibrt_config_ram_bak.rec_mobile.record.bdAddr));
  DUMP8("%02x ", ibrt_config_ram_bak.rec_mobile.record.linkKey,
        sizeof(ibrt_config_ram_bak.rec_mobile.record.linkKey));
  TRACE(1, "%s peer", __func__);
  DUMP8("%02x ", ibrt_config_ram_bak.rec_peer.record.bdAddr.address,
        sizeof(ibrt_config_ram_bak.rec_peer.record.bdAddr));
  DUMP8("%02x ", ibrt_config_ram_bak.rec_peer.record.linkKey,
        sizeof(ibrt_config_ram_bak.rec_peer.record.linkKey));
  return 0;
}

int app_ibrt_if_config_load(ibrt_config_t *config) {
  memset(config->mobile_addr.address, 0, BD_ADDR_LEN);
#ifdef IBRT_SEARCH_UI
  app_ibrt_nvrecord_config_load((void *)config);
  config->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
#else
  app_ibrt_ui_test_config_load((void *)config);
#endif
  return 0;
}

int app_ibrt_if_reconfig(ibrt_config_t *config) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  p_ibrt_ctrl->nv_role = config->nv_role;
  memcpy(p_ibrt_ctrl->peer_addr.address, config->peer_addr.address, 6);
  return 0;
}

int app_ibrt_if_ui_reconfig(ibrt_ui_config_t *config) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  p_ibrt_ui->config = *config;

  return 0;
}

int app_ibrt_core_if_reconfig(ibrt_core_config_t *config) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  p_ibrt_ctrl->config = *config;

  return 0;
}

int app_ibrt_if_config(ibrt_ui_config_t *ui_config) {
  app_ibrt_if_ui_reconfig(ui_config);

  ibrt_core_config_t core_config;
  core_config.lowlayer_monitor_enable = ui_config->lowlayer_monitor_enable;
  core_config.long_private_poll_interval =
      ui_config->long_private_poll_interval;
  core_config.default_private_poll_interval =
      ui_config->default_private_poll_interval;
  core_config.short_private_poll_interval =
      ui_config->short_private_poll_interval;
  core_config.default_private_poll_interval_in_sco =
      ui_config->default_private_poll_interval_in_sco;
  core_config.short_private_poll_interval_in_sco =
      ui_config->short_private_poll_interval_in_sco;
  core_config.default_bt_tpoll = ui_config->default_bt_tpoll;
  core_config.tws_switch_tx_data_protect =
      ui_config->tws_switch_tx_data_protect;
  core_config.tws_cmd_send_timeout = ui_config->tws_cmd_send_timeout;
  core_config.tws_cmd_send_counter_threshold =
      ui_config->tws_cmd_send_counter_threshold;
  core_config.tws_switch_stable_timeout = ui_config->tws_switch_stable_timeout;
  core_config.mobile_page_timeout = ui_config->mobile_page_timeout;
  core_config.tws_connection_timeout = ui_config->tws_connection_timeout;
  core_config.delay_exit_sniff = ui_config->delay_exit_sniff;
  core_config.delay_ms_exit_sniff = ui_config->delay_ms_exit_sniff;
  core_config.audio_sync_mismatch_resume_version =
      ui_config->audio_sync_mismatch_resume_version;
  app_ibrt_core_if_reconfig(&core_config);

  return 0;
}

int app_ibrt_if_event_entry(ibrt_event_type event) {
  TRACE(1, "ibrt_ui_log:custom event entry enter=%s", g_log_event_str[event]);
  // int status = app_ibrt_ui_event_entry(event);

  app_bt_start_custom_function_in_bt_thread((uint32_t)event, 0,
                                            (uint32_t)app_ibrt_ui_event_entry);

#ifdef BLE_ENABLE
  ble_callback_evnet_t ble_callback_event;
  ble_callback_event.evt_type = BLE_CALLBACK_IBRT_EVENT_ENTRY;
  ble_callback_event.p.ibrt_event_entry_handled.event = event;
  app_ble_core_global_callback_event(&ble_callback_event, NULL);
#endif

  return 0;
}

void app_ibrt_if_choice_connect_second_mobile(void) {
  TRACE(0, "ibrt_ui_log:app_ibrt_if_choice_connect_second_mobile");
  app_ibrt_ui_choice_connect_second_mobile();
}

void app_ibrt_if_choice_mobile_connect(uint8_t index) {
  TRACE(0, "ibrt_ui_log:app_ibrt_if_choice_mobile_connect");
  app_ibrt_ui_choice_mobile_connect(index);
}

void app_ibrt_if_disconnect_mobile(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_tws_ibrt_mobile_link_connected()) {
    app_tws_ibrt_disconnect_connection(
        btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle));
  } else if (btif_besaud_is_connected()) {
    tws_ctrl_send_cmd(APP_TWS_CMD_DISC_MOBILE, NULL, 0);
  }
}

void app_ibrt_if_disconnect_mobile_tws_link(void) {
  TRACE(0, "ibrt_ui_log:app_ibrt_if_disconnect_mobile_tws_link");
  app_ibrt_ui_event_entry(IBRT_DISCONNECT_MOBILE_TWS_EVENT);
}

app_ibrt_if_ui_t *app_ibrt_if_ui_get_ctx(void) {
  return (app_ibrt_if_ui_t *)app_ibrt_ui_get_ctx();
}

app_ibrt_if_ctrl_t *app_ibrt_if_get_bt_ctrl_ctx(void) {
  return (app_ibrt_if_ctrl_t *)app_tws_ibrt_get_bt_ctrl_ctx();
}
/*
 * this function is only for tws ibrt pairing mode
 * when tws earphone is in the nearby, tws link will be connected firstly
 * when mobile link exist, this function will disconnect mobile link
 */
void app_ibrt_if_enter_pairing_after_tws_connected(void) {
  app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
}
/*
 * this function is only for freeman pairing
 * no tws link or ibrt link should be connected
 * when mobile link or tws link exist, this function will disconnect mobile link
 * and tws link
 */
void app_ibrt_if_enter_freeman_pairing(void) {
  app_ibrt_ui_event_entry(IBRT_FREEMAN_PAIRING_EVENT);
}
void app_ibrt_if_ctx_checker(void) {
  app_ibrt_if_ui_t *ibrt_if_ui_ctx = app_ibrt_if_ui_get_ctx();
  ibrt_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();

  TRACE(3, "checker: nv_role:%d current_role:%d access_mode:%d",
        p_ibrt_ctrl->nv_role, p_ibrt_ctrl->current_role,
        p_ibrt_ctrl->access_mode);

  DUMP8("%02x ", p_ibrt_ctrl->local_addr.address, BD_ADDR_LEN);
  DUMP8("%02x ", p_ibrt_ctrl->peer_addr.address, BD_ADDR_LEN);

  TRACE(4, "checker: super_state:%s/%s evt:%s plf:%d",
        g_log_super_state_str[ibrt_if_ui_ctx->super_state],
        g_log_super_state_str[ibrt_if_ui_ctx->wait_state],
        g_log_event_str[ibrt_if_ui_ctx->active_event & 0x0F],
        ibrt_if_ui_ctx->profile_exchanged);

  TRACE(4, "checker: box_state:%s/%s sm_run:%d/%d",
        g_log_box_state_str[ibrt_if_ui_ctx->box_state],
        g_log_box_state_str[ibrt_if_ui_ctx->peer_tws_info.box_state],
        ibrt_if_ui_ctx->ibrt_sm_running,
        ibrt_if_ui_ctx->peer_tws_info.ibrt_sm_running);

  if (p_ibrt_ctrl->mobile_mode == IBRT_ACTIVE_MODE ||
      p_ibrt_ctrl->tws_mode == IBRT_ACTIVE_MODE) {
    bt_drv_reg_op_controller_state_checker();
  }
}

int app_ibrt_if_force_audio_retrigger(void) {
  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);
  app_ibrt_if_voice_report_trig_retrigger();
  return 0;
}

int app_ibrt_if_force_audio_retrigger_slave_sync(void) {
  a2dp_audio_detect_next_packet_callback_register(NULL);
  a2dp_audio_detect_store_packet_callback_register(NULL);
  // app_ibrt_if_voice_report_trig_retrigger();
  return 0;
}

bool app_ibrt_if_is_audio_active(void) {
  btif_avdtp_stream_state_t state = btif_a2dp_get_stream_state(
      app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1));
  return ((state == BTIF_AVDTP_STRM_STATE_STREAMING) ||
          btapp_hfp_is_sco_active());
}
bool app_ibrt_if_false_trigger_protect(ibrt_event_type evt_type) {
  app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();
  ibrt_box_state box_state;
  if (app_ibrt_ui_event_queue_empty() ||
      (IBRT_BOX_UNKNOWN == p_ui_ctrl->box_state_future)) {
    box_state = p_ui_ctrl->box_state;
    p_ui_ctrl->box_state_future = IBRT_BOX_UNKNOWN;
  } else {
    TRACE(2,
          "event queue not empty, use future box state to judge, rear=%d, "
          "front=%d",
          p_ui_ctrl->event_q_manager.rear, p_ui_ctrl->event_q_manager.front);

    box_state = p_ui_ctrl->box_state_future;
  }

  TRACE(1, "ibrt_ui_log:current box state=%s", g_log_box_state_str[box_state]);

#ifdef IBRT_SEARCH_UI
  return false;
#else
  switch (evt_type) {
  case IBRT_OPEN_BOX_EVENT:
    return (box_state != IBRT_IN_BOX_CLOSED);

  case IBRT_FETCH_OUT_EVENT:
    return (box_state != IBRT_IN_BOX_OPEN);

  case IBRT_PUT_IN_EVENT:
    return (box_state != IBRT_OUT_BOX);

  case IBRT_CLOSE_BOX_EVENT:
    return (box_state != IBRT_IN_BOX_OPEN);

  case IBRT_WEAR_UP_EVENT:
    return (box_state != IBRT_OUT_BOX);

  case IBRT_WEAR_DOWN_EVENT:
    return (box_state != IBRT_OUT_BOX_WEARED);

  default:
    break;
  }
  return false;
#endif
}
void app_ibrt_if_sco_lowlatency_scan(uint16_t interval, uint16_t window,
                                     uint8_t interlanced) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
  TRACE(3, "ibrt_ui_log:custom pscan int 0x%02x win 0x%02x type %d\n", interval,
        window, interlanced);
  p_ibrt_ui->pscan_cust_interval = interval;
  p_ibrt_ui->pscan_cust_window = window;
  p_ibrt_ui->pscan_cust_type = interlanced;
  app_ibrt_ui_judge_scan_type(IBRT_SCO_PLAYING_TRIGGER, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

void app_ibrt_if_sco_restore_scan(void) {
  TRACE(0, "ibrt_ui_log:restore pscan\n");
  app_ibrt_ui_judge_scan_type(IBRT_SCO_SUSPEND_TRIGGER, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

void app_ibrt_if_a2dp_lowlatency_scan(uint16_t interval, uint16_t window,
                                      uint8_t interlanced) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
  TRACE(3, "ibrt_ui_log:custom pscan int 0x%02x win 0x%02x type %d\n", interval,
        window, interlanced);
  p_ibrt_ui->pscan_cust_interval = interval;
  p_ibrt_ui->pscan_cust_window = window;
  p_ibrt_ui->pscan_cust_type = interlanced;
  app_ibrt_ui_judge_scan_type(IBRT_A2DP_PLAYING_TRIGGER, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

void app_ibrt_if_a2dp_restore_scan(void) {
  TRACE(0, "ibrt_ui_log:restore pscan\n");
  app_ibrt_ui_judge_scan_type(IBRT_A2DP_SUSPEND_TRIGGER, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

/*check if tws link reconnect is allowed*/
bool app_ibrt_if_tws_reconnect_allowed(void) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  if (app_ibrt_ui_is_box_closed()) {
    TRACE(2, " connect tws disallowed_1, box_state=%d, f_box_state=%d",
          p_ibrt_ui->box_state, p_ibrt_ui->box_state_future);
    return false;
  } else if (app_tws_ibrt_mobile_link_connected() &&
             app_ibrt_if_is_audio_active()) {

    TRACE(2, " connect tws disallowed_2, box_state=%d, f_box_state=%d",
          p_ibrt_ui->box_state, p_ibrt_ui->box_state_future);
    return false;
  } else {
    TRACE(0, " connect tws allowed");
    return true;
  }
}

void app_ibrt_if_sniff_event_callback(const btif_event_t *event) {}

void app_ibrt_if_audio_mute(void) {}

void app_ibrt_if_audio_recover(void) {}

int app_ibrt_if_tws_sniff_block(uint32_t block_next_sec) {
  tws_sniff_block_start = hal_sys_timer_get();
  tws_sniff_block_next_sec = block_next_sec;
  return 0;
}

static bool app_ibrt_if_tws_sniff_block_need(void) {
  bool tws_sniff_block = false;
  uint32_t passed_ticks = 0;

  if (tws_sniff_block_start) {
    tws_sniff_block = true;
    passed_ticks =
        hal_timer_get_passed_ticks(hal_sys_timer_get(), tws_sniff_block_start);
    if (TICKS_TO_MS(passed_ticks) > (tws_sniff_block_next_sec * 1000)) {
      tws_sniff_block_start = 0;
      tws_sniff_block_next_sec = 0;
      tws_sniff_block = false;
    }
  } else {
    tws_sniff_block = false;
  }

  return tws_sniff_block;
}

bool app_ibrt_if_tws_sniff_allowed(void) {
  btif_avdtp_stream_state_t a2dp_state = BTIF_AVDTP_STRM_STATE_IDLE;
  if (app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1)) {
    a2dp_state = btif_a2dp_get_stream_state(
        app_bt_get_mobile_a2dp_stream(BTIF_DEVICE_ID_1));
  }
  if (a2dp_state == BTIF_AVDTP_STRM_STATE_STREAMING ||
      btapp_hfp_is_sco_active() || btapp_hfp_get_call_state() ||
      btapp_hfp_get_call_setup() != BTIF_HF_CALL_SETUP_NONE ||
      bt_media_cur_is_bt_stream_media() || app_ibrt_if_tws_sniff_block_need() ||
      app_ibrt_ui_wait_ibrt_connected()
#if defined(IBRT_OTA)
      || ota_is_in_progress()
#endif
#ifdef TEST_OVER_THE_AIR_ENANBLED
      || spp_tota_in_progress()
#endif
  ) {
    TRACE(0, "ibrt_ui_log:sniff not allow\n");
    return false;
  } else {
    TRACE(0, "ibrt_ui_log:sniff allowed\n");
    return true;
  }
}

#define IBRT_IF_SNIFF_CHECKER_INTERVAL_MS (3000)
static void app_ibrt_if_sniff_checker_handler(void const *param);
osTimerDef(APP_IBRT_IF_SNIFF_CHECKER_TIMER, app_ibrt_if_sniff_checker_handler);
static osTimerId app_ibrt_if_sniff_checker_timer = NULL;
static uint32_t app_ibrt_if_sniff_checker_interval_ms =
    IBRT_IF_SNIFF_CHECKER_INTERVAL_MS;
static uint32_t app_ibrt_if_sniff_checker_user_map = 0;

static void app_ibrt_if_sniff_checker_handler(void const *param) {
  app_ibrt_if_ctrl_t *p_ibrt_ctrl = app_ibrt_if_get_bt_ctrl_ctx();
  if (app_ibrt_if_tws_sniff_allowed()) {
    app_ibrt_if_sniff_checker_reset();
  } else {
    if (app_tws_ibrt_mobile_link_connected()) {
      if (p_ibrt_ctrl->mobile_mode == IBRT_SNIFF_MODE) {
        TRACE(0, "ibrt_ui_log:force exit_sniff_with_mobile\n");
        app_tws_ibrt_exit_sniff_with_mobile();
      }
      if (p_ibrt_ctrl->tws_mode == IBRT_SNIFF_MODE) {
        TRACE(0, "ibrt_ui_log:force exit_sniff_with_tws\n");
        app_tws_ibrt_exit_sniff_with_tws();
      }
      osTimerStart(app_ibrt_if_sniff_checker_timer,
                   app_ibrt_if_sniff_checker_interval_ms);
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      osTimerStart(app_ibrt_if_sniff_checker_timer,
                   app_ibrt_if_sniff_checker_interval_ms);
    } else {
      app_ibrt_if_sniff_checker_reset();
    }
  }
}

int app_ibrt_if_sniff_checker_init(uint32_t delay_ms) {
  TRACE(1, "ibrt_ui_log:sniff_checker_init interval:%d \n", delay_ms);
  if (app_ibrt_if_sniff_checker_timer == NULL) {
    app_ibrt_if_sniff_checker_timer = osTimerCreate(
        osTimer(APP_IBRT_IF_SNIFF_CHECKER_TIMER), osTimerOnce, NULL);
  }
  app_ibrt_if_sniff_checker_interval_ms = delay_ms;
  app_ibrt_if_sniff_checker_user_map = 0;
  return 0;
}

int app_ibrt_if_sniff_checker_start(
    enum APP_IBRT_IF_SNIFF_CHECKER_USER_T user) {
  if (app_ibrt_if_sniff_checker_timer) {
    TRACE(1, "ibrt_ui_log:sniff_checker_start usr:%d\n", user);
    app_ibrt_if_sniff_checker_user_map |= (1 << user);
    osTimerStart(app_ibrt_if_sniff_checker_timer,
                 app_ibrt_if_sniff_checker_interval_ms);
  }
  return 0;
}

int app_ibrt_if_sniff_checker_stop(enum APP_IBRT_IF_SNIFF_CHECKER_USER_T user) {
  if (app_ibrt_if_sniff_checker_timer) {
    app_ibrt_if_sniff_checker_user_map &= ~(1 << user);
    if (!app_ibrt_if_sniff_checker_user_map) {
      TRACE(1, "ibrt_ui_log:sniff_checker_stop usr:%d\n", user);
      osTimerStop(app_ibrt_if_sniff_checker_timer);
    }
  }
  return 0;
}

int app_ibrt_if_sniff_checker_reset(void) {
  if (app_ibrt_if_sniff_checker_timer) {
    TRACE(0, "ibrt_ui_log:sniff_checker_reset\n");
    app_ibrt_if_sniff_checker_user_map = 0;
    osTimerStop(app_ibrt_if_sniff_checker_timer);
  }
  return 0;
}

static uint32_t sleep_hook_blocker = 0;
void app_ibrt_if_exec_sleep_hook_blocker_set(
    APP_IBRT_IF_SLEEP_HOOK_BLOCKER_T blocker) {
  sleep_hook_blocker |= (uint32_t)blocker;
}

void app_ibrt_if_exec_sleep_hook_blocker_clr(
    APP_IBRT_IF_SLEEP_HOOK_BLOCKER_T blocker) {
  sleep_hook_blocker &= ~(uint32_t)blocker;
}

bool app_ibrt_if_exec_sleep_hook_blocker_is_hfp_sco(void) {
  return sleep_hook_blocker & APP_IBRT_IF_SLEEP_HOOK_BLOCKER_HFP_SCO ? true
                                                                     : false;
}

bool app_ibrt_if_exec_sleep_hook_blocker_is_a2dp_streaming(void) {
  return sleep_hook_blocker & APP_IBRT_IF_SLEEP_HOOK_BLOCKER_A2DP_STREAMING
             ? true
             : false;
}

bool app_ibrt_if_exec_sleep_hook_allowed(void) {
  if (!sleep_hook_blocker) {
    return true;
  }
  return false;
}

void app_ibrt_ui_adaptive_fa_rx_gain(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (app_tws_ibrt_tws_link_connected()) {
    rx_agc_t tws_agc = {0};
    bt_drv_reg_op_read_rssi_in_dbm(p_ibrt_ctrl->tws_conhandle, &tws_agc);
    static int8_t old_tws_rssi = tws_agc.rssi;
    if (tws_agc.rssi != old_tws_rssi) {
      bt_drv_adaptive_fa_rx_gain(tws_agc.rssi);
      old_tws_rssi = tws_agc.rssi;
    }
  }
}

void app_tws_ibrt_record_sync_id(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint8_t sync_id = bt_drv_reg_op_get_sync_id_op(READ_OP, 0);
  if (is_sco_mode() && sync_id != 0xff) {
    uint16_t sync_id_mask = (1 << 8) | (sync_id & 0xff);
    TRACE(3, "%s:sync id=%d, mask=0x%x", __func__, sync_id, sync_id_mask);
    p_ibrt_ctrl->sync_id_mask = sync_id_mask;
  }
}

void app_tws_ibrt_resume_sync_id(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  uint8_t sync_id_mask = p_ibrt_ctrl->sync_id_mask >> 8;
  uint8_t sync_id = p_ibrt_ctrl->sync_id_mask & 0xff;
  if (sync_id_mask) {
    p_ibrt_ctrl->sync_id_mask = 0;
    if (is_sco_mode() && (sync_id != 0xff)) {
      TRACE(2, "%s: sync id=%d", __func__, sync_id);
      bt_drv_reg_op_get_sync_id_op(WRITE_OP, sync_id);
    }
  }
}

bool app_ibrt_if_start_ibrt_onporcess(void) {
  app_ibrt_if_ui_t *p_ibrt_ui_ctx = app_ibrt_if_ui_get_ctx();
  if (p_ibrt_ui_ctx->super_state == IBRT_UI_W4_SET_ENV_COMPLETE) {
    return true;
  }

  if (p_ibrt_ui_ctx->super_state == IBRT_UI_W4_START_IBRT_COMPLETE) {
    return true;
  }

  if (p_ibrt_ui_ctx->super_state == IBRT_UI_W4_IBRT_DATA_EXCHANGE_COMPLETE) {
    return true;
  }

  return false;
}

bool app_ibrt_if_tws_switch_onporcess(void) {
  app_ibrt_if_ui_t *p_ibrt_ui_ctx = app_ibrt_if_ui_get_ctx();
  if (p_ibrt_ui_ctx->super_state == IBRT_UI_W4_TWS_SWITCH_COMPLETE) {
    return true;
  }

  return false;
}

AppIbrtStatus app_ibrt_if_get_a2dp_state(AppIbrtA2dpState *a2dp_state) {
  if (app_tws_ibrt_mobile_link_connected() ||
      app_tws_ibrt_slave_ibrt_link_connected()) {
#if defined(A2DP_AAC_ON)
    if (btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream) >
        BTIF_AVDTP_STRM_STATE_IDLE) {
      if (a2dp_state != NULL) {
        *a2dp_state = (AppIbrtA2dpState)btif_a2dp_get_stream_state(
            app_bt_device.a2dp_aac_stream[BT_DEVICE_ID_1]->a2dp_stream);
      }
    }
#else
    if (0)
      ;
#endif
    else if (btif_a2dp_get_stream_state(
                 app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream) >
             BTIF_AVDTP_STRM_STATE_IDLE) {
      if (a2dp_state != NULL) {
        *a2dp_state = (AppIbrtA2dpState)btif_a2dp_get_stream_state(
            app_bt_device.a2dp_stream[BT_DEVICE_ID_1]->a2dp_stream);
      }
    } else {
      if (a2dp_state != NULL) {
        *a2dp_state = APP_IBRT_IF_A2DP_IDLE;
      }
    }
  } else {
    if (a2dp_state != NULL) {
      *a2dp_state = APP_IBRT_IF_A2DP_IDLE;
    }
  }

  return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_avrcp_state(AppIbrtAvrcpState *avrcp_state) {
  if (BTIF_AVRCP_STATE_DISCONNECTED !=
      app_bt_device.avrcp_state[BT_DEVICE_ID_1]) {
    if (avrcp_state != NULL) {
      if (avrcp_get_media_status() == BTIF_AVRCP_MEDIA_PLAYING) {
        *avrcp_state = APP_IBRT_IF_AVRCP_PLAYING;
      } else if (avrcp_get_media_status() == BTIF_AVRCP_MEDIA_PAUSED) {
        *avrcp_state = APP_IBRT_IF_AVRCP_PAUSED;
      } else {
        *avrcp_state = APP_IBRT_IF_AVRCP_CONNECTED;
      }
    }
  } else {
    if (avrcp_state != NULL) {
      *avrcp_state = APP_IBRT_IF_AVRCP_DISCONNECTED;
    }
  }
  return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_hfp_state(AppIbrtHfpState *hfp_state) {
  if (btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]) ==
      BTIF_HF_STATE_OPEN) {
    if (app_bt_device.hf_audio_state[BT_DEVICE_ID_1] == BTIF_HF_AUDIO_CON) {
      if (hfp_state != NULL) {
        *hfp_state = APP_IBRT_IF_HFP_SCO_OPEN;
      }
    } else {
      if (hfp_state != NULL) {
        *hfp_state = APP_IBRT_IF_HFP_SCO_CLOSED;
      }
    }
  } else {
    if (hfp_state != NULL) {
      *hfp_state = APP_IBRT_IF_HFP_SLC_DISCONNECTED;
    }
  }
  return APP_IBRT_IF_STATUS_SUCCESS;
}

AppIbrtStatus app_ibrt_if_get_hfp_call_status(AppIbrtCallStatus *call_status) {
  if (call_status == NULL) {
    return APP_IBRT_IF_STATUS_ERROR_INVALID_PARAMETERS;
  }

  if (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == BTIF_HF_CALL_SETUP_IN) {
    *call_status = APP_IBRT_IF_SETUP_INCOMMING;
  } else if (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] ==
             BTIF_HF_CALL_SETUP_OUT) {
    *call_status = APP_IBRT_IF_SETUP_OUTGOING;
  } else if (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] ==
             BTIF_HF_CALL_SETUP_ALERT) {
    *call_status = APP_IBRT_IF_SETUP_ALERT;
  } else if ((app_bt_device.hfchan_call[BT_DEVICE_ID_1] ==
              BTIF_HF_CALL_ACTIVE) &&
             (app_bt_device.hf_audio_state[BT_DEVICE_ID_1] ==
              BTIF_HF_AUDIO_CON)) {
    *call_status = APP_IBRT_IF_CALL_ACTIVE;
  } else if (app_bt_device.hf_callheld[BT_DEVICE_ID_1] ==
             BTIF_HF_CALL_HELD_ACTIVE) {
    *call_status = APP_IBRT_IF_HOLD;
  } else {
    *call_status = APP_IBRT_IF_NO_CALL;
  }

  return APP_IBRT_IF_STATUS_SUCCESS;
}

ibrt_role_e app_ibrt_if_get_ibrt_role() {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  return (p_ibrt_ctrl->current_role);
}

#endif
