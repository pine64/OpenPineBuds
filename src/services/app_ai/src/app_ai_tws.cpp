#include "app_ai_tws.h"
#include "app_ai_if.h"
#include "app_ai_if_thirdparty.h"
#include "app_ble_mode_switch.h"
#include "cmsis_os.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "spp_api.h"

#ifdef __AI_VOICE__
#include "ai_control.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_ibrt_ui.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_if.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_bt.h"
#include "gsound_custom_tws.h"
#endif

#define CASE_S(s)                                                              \
  case s:                                                                      \
    return "[" #s "]";
#define CASE_D()                                                               \
  default:                                                                     \
    return "[INVALID]";
#define APP_AI_ROLE_SWITCH_TIME_IN_MS (500)

APP_AI_TWS_REBOOT_T REBOOT_CUSTOM_PARAM_LOC app_ai_tws_reboot = {false, 0xFF};

#if defined(IBRT) && defined(__AI_VOICE__)
static void app_ai_role_switch_timeout_cb(void const *n);
osTimerDef(APP_AI_ROLE_SWITCH_TIMER, app_ai_role_switch_timeout_cb);
osTimerId app_ai_role_switch_timer_id = NULL;

static void app_ai_role_switch_timeout_cb(void const *n) {
  app_ai_tws_role_switch_dis_ble();
}

#define APP_AI_BLE_DISC_TIME_IN_MS (100)
static void app_ai_ble_disc_timeout_cb(void const *n);
osTimerDef(APP_AI_BLE_DISC_TIMER, app_ai_ble_disc_timeout_cb);
osTimerId app_ai_ble_disc_timer_id = NULL;

extern "C" bool app_ai_ble_is_connected(void);
extern "C" uint16_t app_ai_ble_get_conhdl(void);

static void app_ai_ble_disc_timeout_cb(void const *n) {
  TRACE(1, "%s", __func__);

  app_ai_tws_role_switch_direct();
}
#endif

void app_ai_let_slave_continue_roleswitch(void) {
#if defined(IBRT) && defined(__AI_VOICE__)
  uint8_t role = app_ai_get_ai_spec();
  tws_ctrl_send_cmd(APP_TWS_CMD_LET_SLAVE_CONTINUE_RS, &role, 1);
#endif
}

bool app_ai_tws_role_switch_direct(void) {
  bool ret = false;
#if defined(IBRT) && defined(__AI_VOICE__)
  if (!app_ai_is_role_switching()) {
    TRACE(1, "%s isn't switching now", __func__);
    return ret;
  }

  TRACE(2, "%s initiative %d", __func__, app_ai_is_initiative_switch());
  if (app_ai_is_role_switch_direct()) {
    TRACE(1, "%s has done", __func__);
    return ret;
  }
  app_ai_set_role_switch_direct(true);

  osTimerStop(app_ai_role_switch_timer_id);
  osTimerStop(app_ai_ble_disc_timer_id);

  if (app_ai_is_initiative_switch()) {
    app_ibrt_if_tws_switch_prepare_done_in_bt_thread(IBRT_ROLE_SWITCH_USER_AI,
                                                     app_ai_get_ai_spec());
    return ret;
  } else {
    app_ai_let_slave_continue_roleswitch();
    // ret = true;
  }
#endif

  return ret;
}

bool app_ai_tws_role_switch_dis_ble(void) {
  bool ret = false;
#if defined(IBRT) && defined(__AI_VOICE__)
  uint16_t ble_Conhandle = 0;
  bool ble_connected = app_ai_ble_is_connected();
  TRACE(2, "%s ble_connected %d", __func__, ble_connected);

  osTimerStop(app_ai_role_switch_timer_id);

  if (ble_connected) {
    // disconnect ble connection if existing
    ble_Conhandle = app_ai_ble_get_conhdl();
    bt_drv_reg_op_ble_sup_timeout_set(
        ble_Conhandle,
        15); // fix ble disconnection takes long time:connSupervisionTime=150ms
    app_ble_disconnect_all();
    osTimerStart(app_ai_ble_disc_timer_id, APP_AI_BLE_DISC_TIME_IN_MS);
    ret = true;
  } else {
    ret = app_ai_tws_role_switch_direct();
  }
#endif

  return ret;
}

#if defined(IBRT) && defined(__AI_VOICE__)
static void app_ai_tws_slave_request_master_role_switch(void) {
  TRACE(3, "%s complete %d switching %d", __func__, app_ai_is_setup_complete(),
        app_ai_is_role_switching());

  uint8_t role = app_ai_get_ai_spec();
  tws_ctrl_send_cmd(APP_TWS_CMD_LET_MASTER_PREPARE_RS, &role, 1);
  osTimerStart(app_ai_ble_disc_timer_id,
               APP_AI_ROLE_SWITCH_TIME_IN_MS + APP_AI_BLE_DISC_TIME_IN_MS);
}
#endif

bool app_ai_tws_master_role_switch(void) {
  bool ret = false;
#if defined(IBRT) && defined(__AI_VOICE__)
  TRACE(3, "%s complete %d switching %d", __func__, app_ai_is_setup_complete(),
        app_ai_is_role_switching());

  if (app_ai_is_use_thirdparty()) {
    app_ai_if_thirdparty_stop_stream_by_app_audio_manager();
    app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS,
                                      THIRDPARTY_AI_DISCONNECT);
  }

  app_ai_set_role_switching(true);
  osTimerStart(app_ai_role_switch_timer_id, APP_AI_ROLE_SWITCH_TIME_IN_MS);
  if (app_ai_is_stream_running() ||
      (app_ai_get_speech_state() != AI_SPEECH_STATE__IDLE)) {
    TRACE(2, "%s stream runing %d", __func__, app_ai_is_stream_running());

    ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
  }

  if (ERROR_RETURN != ai_function_handle(API_AI_ROLE_SWITCH, NULL, 0)) {
    return true;
  }

  ret = app_ai_tws_role_switch_dis_ble();
#endif

  return ret;
}

bool app_ai_role_switch(void) {
  bool ret = false;
#ifdef __AI_VOICE__
  TRACE(3, "%s complete %d switching %d", __func__, app_ai_is_setup_complete(),
        app_ai_is_role_switching());

  if (app_ai_is_role_switching()) {
    TRACE(1, "%s is switching now", __func__);
    return ret;
  }
  app_ai_set_role_switching(true);
  app_ai_set_initiative_switch(true);

  if (!app_ai_is_setup_complete() && !app_ai_is_peer_setup_complete()) {
    TRACE(1, "%s ai not setup complete", __func__);
    return ret;
  }

#if defined(IBRT)
  if (app_ai_tws_get_local_role() == APP_AI_TWS_MASTER) {
    ret = app_ai_tws_master_role_switch();
  } else {
    app_ai_tws_slave_request_master_role_switch();
    ret = true;
  }
#endif
#endif

  return ret;
}

uint32_t app_ai_tws_role_switch_prepare(uint32_t *wait_ms) {
  uint32_t ret = 0;

#ifdef BISTO_ENABLED
  *wait_ms = 800;
  ret |= (1 << AI_SPEC_GSOUND);
#endif

#ifdef __AI_VOICE__
  if (app_ai_role_switch()) {
    *wait_ms = 800;
    ret |= (1 << app_ai_get_ai_spec());
  }
#endif

  TRACE(3, "[%s] ret=%d, wait_ms=%d", __func__, ret, *wait_ms);
  return ret;
}

void app_ai_tws_master_role_switch_prepare(void) {
#ifdef __AI_VOICE__
  app_ai_tws_master_role_switch();
#endif
}

void app_ai_tws_role_switch_prepare_done(void) {
#if defined(IBRT) && defined(__AI_VOICE__)
  TRACE(1, "%s", __func__);

  app_ai_tws_role_switch_direct();
#endif
}

void app_ai_tws_role_switch(void) {
  TRACE(1, "[%s]", __func__);

#ifdef BISTO_ENABLED
  gsound_tws_request_roleswitch();
#endif
}

void app_ai_tws_role_switch_complete(void) {
#ifdef __AI_VOICE__
  TRACE(1, "%s", __func__);

  app_ai_set_role_switching(false);
  app_ai_set_initiative_switch(false);
  app_ai_set_role_switch_direct(false);
  app_ai_set_can_role_switch(false);
  app_ai_set_ble_adv();

  if (app_ai_is_setup_complete()) {
    app_ai_set_speech_state(AI_SPEECH_STATE__IDLE);
    app_ai_voice_stream_deinit();
#if defined(IBRT)
    if (app_tws_ibrt_tws_link_connected() &&
        (app_ai_tws_get_local_role() == APP_AI_TWS_MASTER))
#endif
    {
      if (app_ai_is_in_multi_ai_mode()) {
        if (AI_SPEC_GSOUND == ai_manager_get_current_spec() ||
            AI_SPEC_INIT == ai_manager_get_current_spec()) {
          return;
        }
      }

      if (app_ai_is_use_thirdparty()) {
        app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS,
                                          THIRDPARTY_AI_CONNECT);
        app_ai_if_thirdparty_start_stream_by_app_audio_manager();
      }
    }
  }

  app_ai_ui_global_handler_ind(AI_CUSTOM_CODE_AI_ROLE_SWITCH_COMPLETE, NULL, 0);
#endif
}

void app_ai_tws_sync_info_prepare_handler(uint8_t *buf, uint16_t *length) {
#ifdef __AI_VOICE__
  uint16_t buf_len = 0;

  *length = ai_save_ctx(buf, buf_len);
#endif
}

void app_ai_tws_sync_info_received_handler(uint8_t *buf, uint16_t length) {
#ifdef __AI_VOICE__
  ai_restore_ctx(buf, length);
#endif
}

void app_ai_tws_sync_init(void) {
#if defined(IBRT) && defined(__AI_VOICE__)
  TWS_SYNC_USER_T user_app_ai_t = {
      app_ai_tws_sync_info_prepare_handler,
      app_ai_tws_sync_info_received_handler,
      NULL,
      NULL,
      NULL,
  };

  app_tws_if_register_sync_user(TWS_SYNC_USER_AI_INFO, &user_app_ai_t);

  if (NULL == app_ai_role_switch_timer_id) {
    app_ai_role_switch_timer_id =
        osTimerCreate(osTimer(APP_AI_ROLE_SWITCH_TIMER), osTimerOnce, NULL);
  }
  if (NULL == app_ai_ble_disc_timer_id) {
    app_ai_ble_disc_timer_id =
        osTimerCreate(osTimer(APP_AI_BLE_DISC_TIMER), osTimerOnce, NULL);
  }
#endif
}

void app_ai_tws_sync_ai_info(void) {
#if defined(IBRT) && defined(__AI_VOICE__)
  app_tws_if_sync_info(TWS_SYNC_USER_AI_INFO);
#endif
}

void ai_manager_sync_info_prepare_handler(uint8_t *buf, uint16_t *length) {
#ifdef IS_MULTI_AI_ENABLED
  uint16_t buf_len = 0;

  *length = ai_manager_save_ctx(buf, buf_len);
#endif
}

void ai_manager_sync_info_received_handler(uint8_t *buf, uint16_t length) {
#ifdef IS_MULTI_AI_ENABLED
  ai_manager_restore_ctx(buf, length);
#endif
}

void ai_manager_sync_info_received_rsp_handler(uint8_t *buf, uint16_t length) {
#ifdef IS_MULTI_AI_ENABLED
  ai_manager_save_ctx_rsp_handle(buf, length);
#endif
}

void ai_manager_sync_init(void) {
#if defined(IBRT) && defined(IS_MULTI_AI_ENABLED)
  TWS_SYNC_USER_T user_ai_manager_t = {
      ai_manager_sync_info_prepare_handler,
      ai_manager_sync_info_received_handler,
      NULL,
      ai_manager_sync_info_received_rsp_handler,
      ai_manager_sync_info_received_rsp_handler,
  };

  app_tws_if_register_sync_user(TWS_SYNC_USER_AI_MANAGER, &user_ai_manager_t);
#endif
}

void app_ai_tws_sync_ai_manager_info(void) {
#if defined(IBRT) && defined(IS_MULTI_AI_ENABLED)
  app_tws_if_sync_info(TWS_SYNC_USER_AI_MANAGER);
#endif
}

#ifdef IBRT
static void gsound_connect_sync_info_prepare_handler(uint8_t *buf,
                                                     uint16_t *length) {
#ifdef BISTO_ENABLED
  *length = 0;

  AI_CONNECTION_STATE_T *pAiInfo = (AI_CONNECTION_STATE_T *)buf;

  pAiInfo->connBtState = gsound_is_bt_connected();
  pAiInfo->connPathType = gsound_custom_get_connection_path();
  memcpy(pAiInfo->connBdAddr, gsound_get_connected_bd_addr(), 6);

  if (CONNECTION_BLE == pAiInfo->connPathType) {
    pAiInfo->connPathState = gsound_get_ble_connect_state();
  } else if (CONNECTION_BT == pAiInfo->connPathType) {
    pAiInfo->connPathState = gsound_get_bt_connect_state();
  } else {
    pAiInfo->connPathState = 0;
  }

  *length = sizeof(AI_CONNECTION_STATE_T);
#endif
}

static void gsound_connect_sync_info_received_handler(uint8_t *buf,
                                                      uint16_t length) {
#ifdef BISTO_ENABLED
  gsound_custom_connection_state_received_handler(buf);
#endif
}
#endif

void app_ai_tws_gsound_sync_init(void) {
#ifdef IBRT
  TWS_SYNC_USER_T userAiConnect = {
      gsound_connect_sync_info_prepare_handler,
      gsound_connect_sync_info_received_handler,
      NULL,
      NULL,
      NULL,
  };

  app_tws_if_register_sync_user(TWS_SYNC_USER_AI_CONNECTION, &userAiConnect);
#endif
}

void app_ai_tws_send_cmd_to_peer(uint8_t *p_buff, uint16_t length) {
#ifdef IBRT
  if (app_tws_ibrt_tws_link_connected()) {
    app_tws_if_ai_send_cmd_to_peer(p_buff, length);
  }
#endif
}

void app_ai_tws_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                     uint16_t length) {
#ifdef __AI_VOICE__
  app_ai_rev_peer_cmd_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_ai_tws_send_cmd_with_rsp_to_peer(uint8_t *p_buff, uint16_t length) {
#ifdef IBRT
  if (app_tws_ibrt_tws_link_connected()) {
    app_tws_if_ai_send_cmd_with_rsp_to_peer(p_buff, length);
  }
#endif
}

void app_ai_tws_send_cmd_rsp_to_peer(uint8_t *p_buff, uint16_t rsp_seq,
                                     uint16_t length) {
#ifdef IBRT
  if (app_tws_ibrt_tws_link_connected()) {
    app_tws_if_ai_send_cmd_rsp_to_peer(p_buff, rsp_seq, length);
  }
#endif
}

void app_ai_tws_rev_peer_cmd_with_rsp_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                              uint16_t length) {
#ifdef __AI_VOICE__
  app_ai_rev_peer_cmd_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_ai_tws_rev_cmd_rsp_from_peer_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                              uint16_t length) {
#ifdef __AI_VOICE__
  app_ai_rev_peer_cmd_hanlder(rsp_seq, p_buff, length);
#endif
}

void app_ai_tws_rev_cmd_rsp_timeout_hanlder(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length) {
#ifdef __AI_VOICE__
  app_ai_rev_peer_cmd_rsp_timeout_hanlder(rsp_seq, p_buff, length);
#endif
}

bool app_ai_tws_init_done(void) {
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  return (p_ibrt_ctrl->init_done != 0);
#endif
  return false;
}

bool app_ai_tws_link_connected(void) {
#if defined(IBRT)
  return app_tws_ibrt_tws_link_connected();
#endif
  return false;
}

uint8_t *app_ai_tws_local_address(void) {
  uint8_t *local_addr = NULL;
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  local_addr = p_ibrt_ctrl->local_addr.address;
#endif
  return local_addr;
}

uint8_t *app_ai_tws_peer_address(void) {
  uint8_t *peer_addr = NULL;
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  peer_addr = p_ibrt_ctrl->local_addr.address;
#endif
  return peer_addr;
}

void app_ai_tws_reboot_record_box_state(void) {
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
  app_ai_tws_reboot.is_ai_reboot = true;
  app_ai_tws_reboot.box_state = p_ibrt_ui->box_state;
  TRACE(2, "%s box state %d", __func__, p_ibrt_ui->box_state);
}

uint8_t app_ai_tws_reboot_get_box_action(void) {
  uint8_t box_state = 0xFF;

  if (app_ai_tws_reboot.is_ai_reboot) {
    box_state = app_ai_tws_reboot.box_state;
  }

  TRACE(2, "%s box state %d", __func__, box_state);
  if (box_state == IBRT_IN_BOX_OPEN) {
    return IBRT_OPEN_BOX_EVENT;
  } else if (box_state == IBRT_OUT_BOX) {
    return IBRT_FETCH_OUT_EVENT;
  } else if (box_state == IBRT_OUT_BOX_WEARED) {
    return IBRT_WEAR_UP_EVENT;
  }

  return 0xFF;
}

void app_ai_tws_clear_reboot_box_state(void) {
  TRACE(2, "%s box state %d", __func__, app_ai_tws_reboot.box_state);
  app_ai_tws_reboot.is_ai_reboot = false;
  app_ai_tws_reboot.box_state = 0xFF;
}

void app_ai_tws_disconnect_all_bt_connection(void) {
#if defined(IBRT)
  app_tws_ibrt_disconnect_all_connection();
#endif
}

uint8_t app_ai_tws_get_local_role(void) {
  uint8_t local_role = APP_AI_TWS_UNKNOW;
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  if (app_ibrt_ui_is_profile_exchanged()) {
    if (app_tws_ibrt_mobile_link_connected()) {
      local_role = APP_AI_TWS_MASTER;
    } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
      local_role = APP_AI_TWS_SLAVE;
    } else {
      local_role = APP_AI_TWS_UNKNOW;
    }
  } else if (app_tws_ibrt_tws_link_connected()) {
    if (app_tws_ibrt_mobile_link_connected()) {
      local_role = APP_AI_TWS_MASTER;
    } else if (p_ibrt_ctrl->current_role == IBRT_MASTER) {
      local_role = APP_AI_TWS_MASTER;
    } else {
      local_role = APP_AI_TWS_SLAVE;
    }

  } else {
    local_role = APP_AI_TWS_UNKNOW;
  }

  TRACE(5, "[%s] %d exchanged %d tws %d mobile %d", __func__, local_role,
        app_ibrt_ui_is_profile_exchanged(), app_tws_ibrt_tws_link_connected(),
        app_tws_ibrt_mobile_link_connected());
#endif
  return local_role;
}

void app_ai_tws_init(void) {
#if defined(IBRT) && defined(__AI_VOICE__)
  app_ai_set_in_tws_mode(true);
#endif
}
