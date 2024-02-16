/***************************************************************************
 *
 *Copyright 2015-2019 BES.
 *All rights reserved. All unpublished rights reserved.
 *
 *No part of this work may be used or reproduced in any form or by any
 *means, or stored in a database or retrieval system, without prior written
 *permission of BES.
 *
 *Use of this work is governed by a license granted by BES.
 *This work contains confidential and proprietary information of
 *BES. which is protected by copyright, trade secret,
 *trademark and other intellectual property rights.
 *
 ****************************************************************************/

/*****************************header include********************************/
#include "app.h"
#include "app_ble_include.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_hfp.h"
#include "app_sec.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"
#include "ble_app_dbg.h"
#include "cmsis_os.h"
#include "co_math.h" // Common Maths Definition
#include "hal_timer.h"
#include "nvrecord.h"
#include "nvrecord_ble.h"
#include "rwprf_config.h"
#include "stdbool.h"
#include "string.h"

/************************private macro defination***************************/
#define DEBUG_BLE_STATE_MACHINE true

#if DEBUG_BLE_STATE_MACHINE
#define SET_BLE_STATE(newState)                                                \
  do {                                                                         \
    LOG_I("[STATE]%s->%s at line %d", ble_state2str(bleModeEnv.state),         \
          ble_state2str(newState), __LINE__);                                  \
    bleModeEnv.state = (newState);                                             \
  } while (0);

#define SET_BLE_OP(newOp)                                                      \
  do {                                                                         \
    LOG_I("[OP]%s->%s at line %d", ble_op2str(bleModeEnv.op),                  \
          ble_op2str(newOp), __LINE__);                                        \
    bleModeEnv.op = (newOp);                                                   \
  } while (0);
#else
#define SET_BLE_STATE(newState)                                                \
  do {                                                                         \
    bleModeEnv.state = (newState);                                             \
  } while (0);

#define SET_BLE_OP(newOp)                                                      \
  do {                                                                         \
    bleModeEnv.op = (newOp);                                                   \
  } while (0);
#endif

extern void bt_drv_reg_op_set_rand_seed(uint32_t seed);

/************************private type defination****************************/

/************************extern function declearation***********************/
extern uint8_t bt_addr[6];

/**********************private function declearation************************/
/*---------------------------------------------------------------------------
 *            ble_state2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the string of the ble state
 *
 * Parameters:
 *    state - state to get string
 *
 * Return:
 *    the string of the BLE state
 */
static char *ble_state2str(uint8_t state);

/*---------------------------------------------------------------------------
 *            ble_op2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the string of the ble operation
 *
 * Parameters:
 *    op - operation to get string
 *
 * Return:
 *    the string of the BLE operation
 */
static char *ble_op2str(uint8_t op);

/*---------------------------------------------------------------------------
 *            ble_adv_user2str
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get the string of the ble_adv_user
 *
 * Parameters:
 *    op - operation to get string
 *
 * Return:
 *    the string of the ble_adv_user
 */
static char *ble_adv_user2str(enum BLE_ADV_USER_E user);

/*---------------------------------------------------------------------------
 *            ble_adv_config_param
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    configure BLE adv related parameter in @advParam for further use
 *
 * Parameters:
 *    advType - advertisment mode, see @ for more info
 *    advInterval - advertisement interval in MS
 *
 * Return:
 *    void
 */
static void ble_adv_config_param(uint8_t advType, uint16_t advInterval);

/*---------------------------------------------------------------------------
 *            ble_adv_is_allowed
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if BLE advertisment is allowed or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    true - if advertisement is allowed
 *    flase -  if adverstisement is not allowed
 */
static bool ble_adv_is_allowed(void);

/*---------------------------------------------------------------------------
 *            ble_start_adv
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE advertisement
 *
 * Parameters:
 *    param - see @BLE_ADV_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_adv(void *param);

/*---------------------------------------------------------------------------
 *            ble_start_adv_failed_cb
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    callback function of start BLE advertisement failed
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_start_adv_failed_cb(void);

/*---------------------------------------------------------------------------
 *            ble_start_scan
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE scan api
 *
 * Parameters:
 *    param - see @BLE_SCAN_PARAM_T to get more info
 *
 * Return:
 *    void
 */
static void ble_start_scan(void *param);

/*---------------------------------------------------------------------------
 *            ble_start_connect
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    start BLE connection
 *
 * Parameters:
 *    bleBdAddr - address of BLE device to connect
 *
 * Return:
 *    void
 */
static void ble_start_connect(uint8_t *bleBdAddr);

/*---------------------------------------------------------------------------
 *            ble_stop_all_activities
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop all BLE pending operations, stop ongoing adv and scan
 *    NOTE: will not disconnect BLE connections
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_stop_all_activities(void);

/*---------------------------------------------------------------------------
 *            ble_execute_pending_op
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    execute pended BLE op
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_execute_pending_op(void);

/*---------------------------------------------------------------------------
 *            ble_switch_activities
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    switch BLE activities after last state complete
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
static void ble_switch_activities(void);

/************************private variable defination************************/
static BLE_MODE_ENV_T bleModeEnv;
static BLE_ADV_PARAM_T advParam;
static BLE_SCAN_PARAM_T scanParam;

/****************************function defination****************************/
// common used function
#ifdef USE_LOG_I_ID
static inline char *ble_state2str(uint8_t state) {
  return (char *)(uint32_t)state;
}
static inline char *ble_op2str(uint8_t op) { return (char *)(uint32_t)op; }
static inline char *ble_adv_user2str(enum BLE_ADV_USER_E user) {
  return (char *)(uint32_t)user;
}

#else
static char *ble_state2str(uint8_t state) {
  char *str = NULL;

#define CASES(state)                                                           \
  case state:                                                                  \
    str = "[" #state "]";                                                      \
    break

  switch (state) {
    CASES(STATE_IDLE);
    CASES(ADVERTISING);
    CASES(STARTING_ADV);
    CASES(STOPPING_ADV);
    CASES(SCANNING);
    CASES(STARTING_SCAN);
    CASES(STOPPING_SCAN);
    CASES(CONNECTING);
    CASES(STARTING_CONNECT);
    CASES(STOPPING_CONNECT);

  default:
    str = "[INVALID]";
    break;
  }

  return str;
}

static char *ble_op2str(uint8_t op) {
  char *str = NULL;

#define CASEO(op)                                                              \
  case op:                                                                     \
    str = "[" #op "]";                                                         \
    break

  switch (op) {
    CASEO(OP_IDLE);
    CASEO(START_ADV);
    CASEO(START_SCAN);
    CASEO(START_CONNECT);
    CASEO(STOP_ADV);
    CASEO(STOP_SCAN);
    CASEO(STOP_CONNECT);

  default:
    str = "[INVALID]";
    break;
  }

  return str;
}

static char *ble_adv_user2str(enum BLE_ADV_USER_E user) {
#define CASE_S(s)                                                              \
  case s:                                                                      \
    return "[" #s "]";
#define CASE_D()                                                               \
  default:                                                                     \
    return "[INVALID]";

  switch (user) {
    CASE_S(USER_STUB)
    CASE_S(USER_GFPS)
    CASE_S(USER_GSOUND)
    CASE_S(USER_AI)
    CASE_S(USER_INTERCONNECTION)
    CASE_S(USER_TILE)
    CASE_S(USER_OTA)
    CASE_D()
  }
}
#endif

void app_ble_mode_init(void) {
  LOG_I("%s", __func__);

  memset(&bleModeEnv, 0, sizeof(bleModeEnv));
  SET_BLE_STATE(STATE_IDLE);
  SET_BLE_OP(OP_IDLE);

  bleModeEnv.bleEnv = &app_env;
}

// ble advertisement used functions
static void ble_adv_config_param(uint8_t advType, uint16_t advInterval) {
  uint8_t avail_space;
  memset(&advParam, 0, sizeof(advParam));

  advParam.advType = advType;
  advParam.advInterval = advInterval;
  advParam.withFlag = true;

  // connectable adv is not allowed if max connection reaches
  if (app_is_arrive_at_max_ble_connections() &&
      (GAPM_ADV_UNDIRECT == advType)) {
    LOG_W("will change adv type to none-connectable because max ble connection "
          "reaches");
    advParam.advType = GAPM_ADV_NON_CONN;
  }

  for (uint8_t user = 0; user < BLE_ADV_USER_NUM; user++) {
    if (bleModeEnv.bleDataFillFunc[user]) {
      bleModeEnv.bleDataFillFunc[user]((void *)&advParam);

      // check if the adv/scan_rsp data length is legal
      if (advParam.withFlag) {
        ASSERT(BLE_ADV_DATA_WITH_FLAG_LEN >= advParam.advDataLen,
               "[BLE][ADV]adv data exceed");
      } else {
        ASSERT(BLE_ADV_DATA_WITHOUT_FLAG_LEN >= advParam.advDataLen,
               "[BLE][ADV]adv data exceed");
      }
      ASSERT(SCAN_RSP_DATA_LEN >= advParam.scanRspDataLen,
             "[BLE][ADV]scan response data exceed");
    }
  }

  if (advParam.withFlag) {
    avail_space = BLE_ADV_DATA_WITH_FLAG_LEN - advParam.advDataLen -
                  BLE_ADV_DATA_STRUCT_HEADER_LEN;
  } else {
    avail_space = BLE_ADV_DATA_WITHOUT_FLAG_LEN - advParam.advDataLen -
                  BLE_ADV_DATA_STRUCT_HEADER_LEN;
  }

  // Check if data can be added to the adv Data
  if (avail_space > 2) {
    avail_space = co_min(avail_space, bleModeEnv.bleEnv->dev_name_len);
    advParam.advData[advParam.advDataLen++] = avail_space + 1;
    // Fill Device Name Flag
    advParam.advData[advParam.advDataLen++] =
        (avail_space == bleModeEnv.bleEnv->dev_name_len) ? '\x08' : '\x09';
    // Copy device name
    memcpy(&advParam.advData[advParam.advDataLen], bleModeEnv.bleEnv->dev_name,
           avail_space);
    // Update adv Data Length
    advParam.advDataLen += avail_space;
  }
}

static bool ble_adv_is_allowed(void) {
  bool allowed_adv = true;
  if (!app_is_stack_ready()) {
    LOG_I("reason: stack not ready");
    allowed_adv = false;
  }

  if (app_is_power_off_in_progress()) {
    LOG_I("reason: in power off mode");
    allowed_adv = false;
  }

  if (bleModeEnv.advSwitch) {
    LOG_I("adv switched off:%d", bleModeEnv.advSwitch);
    allowed_adv = false;
  }

  if (btapp_hfp_is_sco_active()) {
    LOG_I("SCO ongoing");
    allowed_adv = false;
  }

  if (false == allowed_adv) {
    app_ble_stop_activities();
  }

  return allowed_adv;
}

static void ble_start_adv(void *param) {
  switch (bleModeEnv.state) {
  case ADVERTISING:
    SET_BLE_STATE(STOPPING_ADV);
    SET_BLE_OP(START_ADV);
    appm_stop_advertising();
    break;

  case SCANNING:
    SET_BLE_STATE(STOPPING_SCAN);
    SET_BLE_OP(START_ADV);
    appm_stop_scanning();
    break;

  case CONNECTING:
    SET_BLE_STATE(STOPPING_CONNECT);
    SET_BLE_OP(START_ADV);
    appm_stop_connecting();
    break;

  case STARTING_ADV:
  case STARTING_SCAN:
  case STARTING_CONNECT:
  case STOPPING_ADV:
  case STOPPING_SCAN:
  case STOPPING_CONNECT:
    SET_BLE_OP(START_ADV);
    break;

  case STATE_IDLE:
    if (!ble_adv_is_allowed()) {
      LOG_I("[ADV] not allowed.");
      if (START_ADV == bleModeEnv.op) {
        SET_BLE_OP(OP_IDLE);
      }
      break;
    }

    memcpy(&bleModeEnv.advInfo, param, sizeof(bleModeEnv.advInfo));
    appm_start_advertising(&bleModeEnv.advInfo);

    SET_BLE_STATE(STARTING_ADV);
    break;

  default:
    break;
  }
}

static void ble_start_adv_failed_cb(void) {
  if (STARTING_ADV == bleModeEnv.state) {
    SET_BLE_STATE(STATE_IDLE);
  }

  // start pending op(start adv again)
}

static void ble_start_scan(void *param) {
  switch (bleModeEnv.state) {
  case ADVERTISING:
    SET_BLE_STATE(STOPPING_ADV);
    SET_BLE_OP(START_SCAN);
    appm_stop_advertising();
    break;

  case SCANNING:
    SET_BLE_STATE(STOPPING_SCAN);
    SET_BLE_OP(START_SCAN);
    appm_stop_scanning();
    break;

  case CONNECTING:
    SET_BLE_STATE(STOPPING_CONNECT);
    SET_BLE_OP(START_SCAN);
    appm_stop_connecting();
    break;

  case STARTING_ADV:
  case STARTING_SCAN:
  case STARTING_CONNECT:
  case STOPPING_ADV:
  case STOPPING_SCAN:
  case STOPPING_CONNECT:
    SET_BLE_OP(START_SCAN);
    break;

  case STATE_IDLE:

    SET_BLE_STATE(STARTING_SCAN);
    memcpy(&bleModeEnv.scanInfo, param, sizeof(BLE_SCAN_PARAM_T));
    appm_start_scanning(bleModeEnv.scanInfo.scanInterval,
                        bleModeEnv.scanInfo.scanWindow,
                        bleModeEnv.scanInfo.scanType);
    break;

  default:
    break;
  }
}

static void ble_start_connect(uint8_t *bleBdAddr) {
  SET_BLE_OP(START_CONNECT);

  if ((CONNECTING != bleModeEnv.state) &&
      (STARTING_CONNECT != bleModeEnv.state)) {
    switch (bleModeEnv.state) {
    case ADVERTISING:
      SET_BLE_STATE(STOPPING_ADV);
      appm_stop_advertising();
      break;

    case SCANNING:
      SET_BLE_STATE(STOPPING_SCAN);
      appm_stop_scanning();
      break;

    case STATE_IDLE:
      SET_BLE_STATE(STARTING_CONNECT);
      struct gap_bdaddr bdAddr;
      memcpy(bdAddr.addr.addr, bleBdAddr, BTIF_BD_ADDR_SIZE);
      bdAddr.addr_type = 0;
      LOG_I("Master paired with mobile dev is scanned, connect it via BLE.");
      appm_start_connecting(&bdAddr);
      break;

    default:
      break;
    }
  }
}

static void ble_stop_all_activities(void) {
  switch (bleModeEnv.state) {
  case ADVERTISING:
    SET_BLE_OP(OP_IDLE);
    SET_BLE_STATE(STOPPING_ADV);
    appm_stop_advertising();
    break;

  case SCANNING:
    SET_BLE_OP(OP_IDLE);
    SET_BLE_STATE(STOPPING_SCAN);
    appm_stop_scanning();
    break;

  case CONNECTING:
    SET_BLE_OP(OP_IDLE);
    SET_BLE_STATE(STOPPING_CONNECT);
    appm_stop_connecting();
    break;

  case STARTING_ADV:
    SET_BLE_OP(STOP_ADV);
    break;

  case STARTING_SCAN:
    SET_BLE_OP(STOP_SCAN);
    break;

  case STARTING_CONNECT:
    SET_BLE_OP(STOP_CONNECT);
    break;

  case STOPPING_ADV:
  case STOPPING_SCAN:
  case STOPPING_CONNECT:
    SET_BLE_OP(OP_IDLE);
    break;

  default:
    break;
  }
}

static void ble_execute_pending_op(void) {
  LOG_I("%s", __func__);
  uint8_t op = bleModeEnv.op;
  SET_BLE_OP(OP_IDLE);

  switch (op) {
  case START_ADV:
    ble_start_adv(&advParam);
    break;

  case START_SCAN:
    ble_start_scan(&scanParam);
    break;

  case START_CONNECT:
    ble_start_connect(bleModeEnv.bleAddrToConnect);
    break;

  case STOP_ADV:
  case STOP_SCAN:
  case STOP_CONNECT:
    ble_stop_all_activities();
    break;

  default:
    break;
  }
}

static void ble_switch_activities(void) {
  switch (bleModeEnv.state) {
  case STARTING_ADV:
    SET_BLE_STATE(ADVERTISING);
    break;

  case STARTING_SCAN:
    SET_BLE_STATE(SCANNING);
    break;

  case STARTING_CONNECT:
    SET_BLE_STATE(CONNECTING);
    break;

  case STOPPING_ADV:
    SET_BLE_STATE(STATE_IDLE);
    break;

  case STOPPING_SCAN:
    SET_BLE_STATE(STATE_IDLE);
    break;

  case STOPPING_CONNECT:
    SET_BLE_STATE(STATE_IDLE);
    break;

  default:
    break;
  }

  ble_execute_pending_op();
}

// BLE advertisement event callbacks
void app_advertising_started(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

void app_advertising_starting_failed(void) {
  memset(&bleModeEnv.advInfo, 0, sizeof(bleModeEnv.advInfo));
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_start_adv_failed_cb);
}

void app_advertising_stopped(void) {
  memset(&bleModeEnv.advInfo, 0, sizeof(bleModeEnv.advInfo));
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

// BLE adv data updated event callback
void app_adv_data_updated(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

// BLE scan event callbacks
void app_scanning_started(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

void app_scanning_stopped(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

// BLE connect event callbacks
void app_connecting_started(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

void app_connecting_stopped(void) {
  app_bt_start_custom_function_in_bt_thread(0, 0,
                                            (uint32_t)ble_switch_activities);
}

/**
 * @brief : callback function of BLE connect failed
 *
 */
static void app_ble_connecting_failed_handler(void) {
  if ((CONNECTING == bleModeEnv.state) ||
      (STOPPING_CONNECT == bleModeEnv.state) ||
      (STARTING_CONNECT == bleModeEnv.state)) {
    SET_BLE_STATE(STATE_IDLE);
  }
}

void app_connecting_failed(void) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)app_ble_connecting_failed_handler);
}
void app_ble_connected_evt_handler(uint8_t conidx,
                                   const uint8_t *pPeerBdAddress) {
  ble_evnet_t event;

  if ((ADVERTISING == bleModeEnv.state) || (STARTING_ADV == bleModeEnv.state) ||
      (CONNECTING == bleModeEnv.state)) {
    SET_BLE_STATE(STATE_IDLE);
  }

  if (START_CONNECT == bleModeEnv.op) {
    SET_BLE_OP(OP_IDLE);
  }

  event.evt_type = BLE_CONNECT_EVENT;
  event.p.connect_handled.conidx = conidx;
  event.p.connect_handled.peer_bdaddr = pPeerBdAddress;
  app_ble_core_global_handle(&event, NULL);

  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  app_stop_fast_connectable_ble_adv_timer();
}

void app_ble_disconnected_evt_handler(uint8_t conidx) {
  ble_evnet_t event;

  event.evt_type = BLE_DISCONNECT_EVENT;
  event.p.disconnect_handled.conidx = conidx;
  app_ble_core_global_handle(&event, NULL);

  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

// BLE APIs for external use
void app_ble_data_fill_enable(enum BLE_ADV_USER_E user, bool enable) {
  LOG_I("%s user %d%s enable %d", __func__, user, ble_adv_user2str(user),
        enable);

  ASSERT(user < BLE_ADV_USER_NUM, "%s user %d", __func__, user);
  if (enable) {
    bleModeEnv.adv_user_enable |= (1 << user);
  } else {
    bleModeEnv.adv_user_enable &= ~(1 << user);
  }
}

bool app_ble_get_data_fill_enable(enum BLE_ADV_USER_E user) {
  bool enable = bleModeEnv.adv_user_enable & (1 << user);
  LOG_I("%s user %d enable %d", __func__, user, enable);
  return enable;
}

/**
 * @brief : callback function of BLE scan starting failed
 *
 */
static void app_ble_scanning_starting_failed_handler(void) {
  if (STARTING_SCAN == bleModeEnv.state) {
    SET_BLE_STATE(STATE_IDLE);
  }
}

void app_scanning_starting_failed(void) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)app_ble_scanning_starting_failed_handler);
}

// BLE APIs for external use
void app_ble_register_data_fill_handle(enum BLE_ADV_USER_E user,
                                       BLE_DATA_FILL_FUNC_T func, bool enable) {
  bool needUpdateAdv = false;

  if (BLE_ADV_USER_NUM <= user) {
    LOG_W("invalid user");
  } else {
    if (func != bleModeEnv.bleDataFillFunc[user] && NULL != func) {
      needUpdateAdv = true;
      bleModeEnv.bleDataFillFunc[user] = func;
    }
  }

  bleModeEnv.adv_user_register |= (1 << user);
  if (needUpdateAdv) {
    app_ble_data_fill_enable(user, enable);
  }
}

void app_ble_system_ready(void) {
  uint32_t generatedSeed = hal_sys_timer_get();

  for (uint8_t index = 0; index < sizeof(bt_addr); index++) {
    generatedSeed ^=
        (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get() & 0xF));
  }

  bt_drv_reg_op_set_rand_seed(generatedSeed);

#if defined(ENHANCED_STACK)
  app_notify_stack_ready(STACK_READY_BLE);
#else
  app_notify_stack_ready(STACK_READY_BLE | STACK_READY_BT);
#endif
}

static void ble_adv_refreshing(void *param) {
  BLE_ADV_PARAM_T *pAdvParam = (BLE_ADV_PARAM_T *)param;
  // four conditions that we just need to update the ble adv data instead of
  // restarting ble adv
  // 1. BLE advertising is on
  // 2. No on-going BLE operation
  // 3. BLE adv type is the same
  // 4. BLE adv interval is the same
  if ((ADVERTISING == bleModeEnv.state) && (OP_IDLE == bleModeEnv.op) &&
      bleModeEnv.advInfo.advType == pAdvParam->advType &&
      bleModeEnv.advInfo.advInterval == pAdvParam->advInterval) {
    memcpy(&bleModeEnv.advInfo, param, sizeof(bleModeEnv.advInfo));
    SET_BLE_STATE(STARTING_ADV);
    appm_update_adv_data(pAdvParam->advData, pAdvParam->advDataLen,
                         pAdvParam->scanRspData, pAdvParam->scanRspDataLen);
  } else {
    // otherwise, restart ble adv
    ble_start_adv(param);
  }
}

static bool app_ble_start_adv(uint8_t advType, uint16_t advInterval) {
  uint32_t adv_user_enable = bleModeEnv.adv_user_enable;
  LOG_I("[ADV]type:%d, interval:%d ca:%p", advType, advInterval,
        __builtin_return_address(0));

  if (!ble_adv_is_allowed()) {
    LOG_I("[ADV] not allowed.");
    return false;
  }

  ble_adv_config_param(advType, advInterval);

  LOG_I("%s old_user_enable 0x%x new 0x%x", __func__, adv_user_enable,
        bleModeEnv.adv_user_enable);
  if (!bleModeEnv.adv_user_enable) {
    LOG_I("no adv user enable");
    LOG_I("[ADV] not allowed.");
    app_ble_stop_activities();
    return false;
  }

  // param of adv request is exactly same as current adv
  if (ADVERTISING == bleModeEnv.state &&
      !memcmp(&bleModeEnv.advInfo, &advParam, sizeof(advParam))) {
    LOG_I("reason: adv param not changed");
    LOG_I("[ADV] not allowed.");
    return false;
  }

  LOG_I("[ADV_LEN] %d [DATA]:", advParam.advDataLen);
  DUMP8("%02x ", advParam.advData, advParam.advDataLen);
  LOG_I("[SCAN_RSP_LEN] %d [DATA]:", advParam.scanRspDataLen);
  DUMP8("%02x ", advParam.scanRspData, advParam.scanRspDataLen);

  ble_adv_refreshing(&advParam);

  return true;
}

void app_ble_start_connectable_adv(uint16_t advInterval) {
  LOG_D("%s", __func__);
  app_bt_start_custom_function_in_bt_thread((uint32_t)GAPM_ADV_UNDIRECT,
                                            (uint32_t)advInterval,
                                            (uint32_t)app_ble_start_adv);
}

void app_ble_refresh_adv_state(uint16_t advInterval) {
  LOG_D("%s", __func__);
  app_bt_start_custom_function_in_bt_thread((uint32_t)GAPM_ADV_UNDIRECT,
                                            (uint32_t)advInterval,
                                            (uint32_t)app_ble_start_adv);
}

void app_ble_start_scan(enum BLE_SCAN_FILTER_POLICY scanFilterPolicy,
                        uint16_t scanWindow, uint16_t scanInterval) {
  scanParam.scanWindow = scanWindow;
  scanParam.scanInterval = scanInterval;
  scanParam.scanType = scanFilterPolicy; // BLE_SCAN_ALLOW_ADV_WLST

  app_bt_start_custom_function_in_bt_thread((uint32_t)(&scanParam), 0,
                                            (uint32_t)ble_start_scan);
}

void app_ble_start_connect(uint8_t *bdAddrToConnect) {
  memcpy(bleModeEnv.bleAddrToConnect, bdAddrToConnect, BTIF_BD_ADDR_SIZE);
  app_bt_start_custom_function_in_bt_thread(
      (uint32_t)bleModeEnv.bleAddrToConnect, 0, (uint32_t)ble_start_connect);
}

bool app_ble_is_connection_on(uint8_t index) {
  return (BLE_CONNECTED == bleModeEnv.bleEnv->context[index].connectStatus);
}

bool app_ble_is_any_connection_exist(void) {
  bool ret = false;
  for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++) {
    if (app_ble_is_connection_on(i)) {
      ret = true;
    }
  }

  return ret;
}

void app_ble_start_disconnect(uint8_t conIdx) {
  if (BLE_CONNECTED == bleModeEnv.bleEnv->context[conIdx].connectStatus) {
    LOG_I("will disconnect connection:%d", conIdx);
    bleModeEnv.bleEnv->context[conIdx].connectStatus = BLE_DISCONNECTING;
    app_bt_start_custom_function_in_bt_thread((uint32_t)conIdx, 0,
                                              (uint32_t)appm_disconnect);
  } else {
    LOG_I("will not execute disconnect since state is:%d",
          bleModeEnv.bleEnv->context[conIdx].connectStatus);
  }
}

void app_ble_disconnect_all(void) {
  for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++) {
    app_ble_start_disconnect(i);
  }
}

void app_ble_stop_activities(void) {
  LOG_I("%s %p", __func__, __builtin_return_address(0));

  app_stop_fast_connectable_ble_adv_timer();
  if (bleModeEnv.state != OP_IDLE) {
    app_bt_start_custom_function_in_bt_thread(
        0, 0, (uint32_t)ble_stop_all_activities);
  }
}

void app_ble_force_switch_adv(uint8_t user, bool onOff) {
  ASSERT(user < BLE_SWITCH_USER_NUM, "ble switch user exceed");

  if (onOff) {
    bleModeEnv.advSwitch &= ~(1 << user);
    app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
  } else if ((bleModeEnv.advSwitch & (1 << user)) == 0) {
    bleModeEnv.advSwitch |= (1 << user);
    app_ble_stop_activities();

    // disconnect all of the BLE connections if box is closed
    if (BLE_SWITCH_USER_BOX == user) {
      app_ble_disconnect_all();
    }
  }

  LOG_I("%s user %d onoff %d switch 0x%x", __func__, user, onOff,
        bleModeEnv.advSwitch);
}

bool app_ble_is_in_advertising_state(void) {
  return (ADVERTISING == bleModeEnv.state) ||
         (STARTING_ADV == bleModeEnv.state) ||
         (STOPPING_ADV == bleModeEnv.state);
}

static uint32_t POSSIBLY_UNUSED ble_get_manufacture_data_ptr(
    uint8_t *advData, uint32_t dataLength, uint8_t *manufactureData) {
  uint8_t followingDataLengthOfSection;
  uint8_t rawContentDataLengthOfSection;
  uint8_t flag;
  while (dataLength > 0) {
    followingDataLengthOfSection = *advData++;
    dataLength--;
    if (dataLength < followingDataLengthOfSection) {
      return 0; // wrong adv data format
    }

    if (followingDataLengthOfSection > 0) {
      flag = *advData++;
      dataLength--;

      rawContentDataLengthOfSection = followingDataLengthOfSection - 1;
      if (BLE_ADV_MANU_FLAG == flag) {
        uint32_t lengthToCopy;
        if (dataLength < rawContentDataLengthOfSection) {
          lengthToCopy = dataLength;
        } else {
          lengthToCopy = rawContentDataLengthOfSection;
        }

        memcpy(manufactureData, advData - 2, lengthToCopy + 2);
        return lengthToCopy + 2;
      } else {
        advData += rawContentDataLengthOfSection;
        dataLength -= rawContentDataLengthOfSection;
      }
    }
  }

  return 0;
}

// received adv data
void app_adv_reported_scanned(struct gapm_adv_report_ind *ptInd) {
  /*
  LOG_I("Scanned RSSI %d BD addr:", (int8_t)ptInd->report.rssi);
  DUMP8("0x%02x ", ptInd->report.adv_addr.addr, BTIF_BD_ADDR_SIZE);
  LOG_I("Scanned adv data:");
  DUMP8("0x%02x ", ptInd->report.data, ptInd->report.data_len);
  */

  ble_adv_data_parse(ptInd->report.adv_addr.addr, (int8_t)ptInd->report.rssi,
                     ptInd->report.data, (unsigned char)ptInd->report.data_len);
}

void app_ibrt_ui_disconnect_ble(void) { app_ble_disconnect_all(); }

uint32_t app_ble_get_user_register(void) {
  return bleModeEnv.adv_user_register;
}

enum BLE_STATE_E app_ble_get_current_state(void) { return bleModeEnv.state; }

enum BLE_OP_E app_ble_get_current_operation(void) { return bleModeEnv.op; }

void app_ble_get_runtime_adv_param(uint8_t *pAdvType,
                                   uint16_t *pAdvIntervalMs) {
  *pAdvType = advParam.advType;
  *pAdvIntervalMs = advParam.advInterval;
}
