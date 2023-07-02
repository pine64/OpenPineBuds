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
#include "apps.h"
#include "common_apps_imports.h"
#include "ibrt.h"
#include "key_handler.h"
#include "led_control.h"
#include "rb_codec.h"

#ifdef BTIF_BLE_APP_DATAPATH_SERVER
#include "app_ble_cmd_handler.h"
#endif

#ifdef ANC_APP
#include "app_anc.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#ifdef OTA_ENABLED
#include "nvrecord_ota.h"
#include "ota_common.h"
#endif

#ifdef WL_DET
#include "app_mic_alg.h"
#endif

#ifdef AUDIO_DEBUG_V0_1_0
extern "C" int speech_tuning_init(void);
#endif

#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
extern "C" bool app_usbaudio_mode_on(void);
#endif

#ifdef BES_OTA_BASIC
extern "C" void ota_flash_init(void);
#endif

#define APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD (1)
#define POWERON_PRESSMAXTIME_THRESHOLD_MS (5000)

enum APP_POWERON_CASE_T {
  APP_POWERON_CASE_NORMAL = 0,
  APP_POWERON_CASE_DITHERING,
  APP_POWERON_CASE_REBOOT,
  APP_POWERON_CASE_ALARM,
  APP_POWERON_CASE_CALIB,
  APP_POWERON_CASE_BOTHSCAN,
  APP_POWERON_CASE_CHARGING,
  APP_POWERON_CASE_FACTORY,
  APP_POWERON_CASE_TEST,
  APP_POWERON_CASE_LINKLOSE_REBOOT,
  APP_POWERON_CASE_INVALID,

  APP_POWERON_CASE_NUM
};

#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
extern bool app_pwr_key_monitor_get_val(void);
static bool anc_single_mode_on = false;
extern "C" bool anc_single_mode_is_on(void) { return anc_single_mode_on; }
#endif

#ifdef __ANC_STICK_SWITCH_USE_GPIO__
extern bool app_anc_switch_get_val(void);
#endif

#ifdef GFPS_ENABLED
extern "C" void app_fast_pairing_timeout_timehandler(void);
#endif

uint8_t app_poweroff_flag = 0;
static enum APP_POWERON_CASE_T g_pwron_case = APP_POWERON_CASE_INVALID;

#ifndef APP_TEST_MODE
static uint8_t app_status_indication_init(void) {
  struct APP_PWL_CFG_T cfg;
  hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl,
                 sizeof(cfg_hw_pinmux_pwl) /
                     sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
  memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
  app_pwl_open();
  app_pwl_setup(APP_PWL_ID_0, &cfg);
  app_pwl_setup(APP_PWL_ID_1, &cfg);
  return 0;
}
#endif

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)

void PairingTransferToConnectable(void);

typedef void (*APP_10_SECOND_TIMER_CB_T)(void);

void app_pair_timerout(void);
void app_poweroff_timerout(void);
void CloseEarphone(void);

typedef struct {
  uint8_t timer_id;
  uint8_t timer_en;
  uint8_t timer_count;
  uint8_t timer_period;
  APP_10_SECOND_TIMER_CB_T cb;
} APP_10_SECOND_TIMER_STRUCT;

#define INIT_APP_TIMER(_id, _en, _count, _period, _cb)                         \
  {                                                                            \
    .timer_id = _id, .timer_en = _en, .timer_count = _count,                   \
    .timer_period = _period, .cb = _cb,                                        \
  }

APP_10_SECOND_TIMER_STRUCT app_10_second_array[] = {
    INIT_APP_TIMER(APP_PAIR_TIMER_ID, 0, 0, 6, PairingTransferToConnectable),
    INIT_APP_TIMER(APP_POWEROFF_TIMER_ID, 0, 0, 90, CloseEarphone),
#ifdef GFPS_ENABLED
    INIT_APP_TIMER(APP_FASTPAIR_LASTING_TIMER_ID, 0, 0,
                   APP_FAST_PAIRING_TIMEOUT_IN_SECOND / 10,
                   app_fast_pairing_timeout_timehandler),
#endif
};

void app_stop_10_second_timer(uint8_t timer_id) {
  APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

  timer->timer_en = 0;
  timer->timer_count = 0;
}

void app_start_10_second_timer(uint8_t timer_id) {
  APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

  timer->timer_en = 0;
  timer->timer_count = 0;
}

void app_set_10_second_timer(uint8_t timer_id, uint8_t enable, uint8_t period) {
  APP_10_SECOND_TIMER_STRUCT *timer = &app_10_second_array[timer_id];

  timer->timer_en = enable;
  timer->timer_count = period;
}

void app_10_second_timer_check(void) {
  APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(app_10_second_array); i++) {
    if (timer->timer_en) {
      timer->timer_count++;
      if (timer->timer_count >= timer->timer_period) {
        timer->timer_en = 0;
        if (timer->cb)
          timer->cb();
      }
    }
    timer++;
  }
}

void CloseEarphone(void) {
  int activeCons;
  osapi_lock_stack();
  activeCons = btif_me_get_activeCons();
  osapi_unlock_stack();

#ifdef ANC_APP
  if (app_anc_work_status()) {
    app_set_10_second_timer(APP_POWEROFF_TIMER_ID, 1, 30);
    return;
  }
#endif /* ANC_APP */
  if (activeCons == 0) {
    TRACE(0, "!!!CloseEarphone\n");
    app_shutdown();
  }
}
#endif /* #if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__) */

int signal_send_to_main_thread(uint32_t signals);
uint8_t stack_ready_flag = 0;
void app_notify_stack_ready(uint8_t ready_flag) {
  TRACE(2, "app_notify_stack_ready %d %d", stack_ready_flag, ready_flag);

  stack_ready_flag |= ready_flag;

#ifdef __IAG_BLE_INCLUDE__
  if (stack_ready_flag == (STACK_READY_BT | STACK_READY_BLE))
#endif
  {
    signal_send_to_main_thread(0x3);
  }
}

bool app_is_stack_ready(void) {
  bool ret = false;

  if (stack_ready_flag == (STACK_READY_BT
#ifdef __IAG_BLE_INCLUDE__
                           | STACK_READY_BLE
#endif
                           )) {
    ret = true;
  }

  return ret;
}

static void app_stack_ready_cb(void) {
  TRACE(0, "stack init done");
#ifdef BLE_ENABLE
  app_ble_stub_user_init();
  app_ble_start_connectable_adv(BLE_ADVERTISING_INTERVAL);
#endif
}

// #if (HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT) ||
//(HF_SDK_FEATURES & HF_FEATURE_HF_INDICATORS)
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
extern void app_hfp_set_battery_level(uint8_t level);
#endif

int app_status_battery_report(uint8_t level) {
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  if (anc_single_mode_on) // anc power on,anc only mode
    return 0;
#endif
#if defined(__BTIF_EARPHONE__)
  if (app_is_stack_ready()) {
    app_bt_state_checker();
  }
  app_10_second_timer_check();
#endif
  if (level <= APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD) {
    // add something
  }

  if (app_is_stack_ready()) {
// #if (HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT) ||
// (HF_SDK_FEATURES & HF_FEATURE_HF_INDICATORS)
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
#if defined(IBRT)
    if (app_tws_ibrt_mobile_link_connected()) {
      app_hfp_set_battery_level(level);
    }
#else
    app_hfp_set_battery_level(level);
#endif
#else
    TRACE(1, "[%s] Can not enable SUPPORT_BATTERY_REPORT", __func__);
#endif
    osapi_notify_evm();
  }
  return 0;
}

void stopAuto_Shutdowm_Timer(void);
void app_bt_power_off_customize() {
#if (TWS_Sync_Shutdowm)
  app_ibrt_poweroff_notify_force();
#endif
  app_shutdown();
}

#ifdef MEDIA_PLAYER_SUPPORT

void app_status_set_num(const char *p) { media_Set_IncomingNumber(p); }

static u8 last_voice_waring = APP_STATUS_INDICATION_NUM;
int app_voice_report_handler(APP_STATUS_INDICATION_T status, uint8_t device_id,
                             uint8_t isMerging) {
#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
  if (app_usbaudio_mode_on())
    return 0;
#endif
  TRACE(3, "%s %d", __func__, status);
  AUD_ID_ENUM id = MAX_RECORD_NUM;
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
  if (anc_single_mode_on)
    return 0;
#endif
  if (((last_voice_waring == APP_STATUS_INDICATION_POWERON) &&
       (status == APP_STATUS_INDICATION_POWERON)) ||
      ((last_voice_waring == APP_STATUS_INDICATION_BOTHSCAN) &&
       (status == APP_STATUS_INDICATION_BOTHSCAN)) ||
      ((last_voice_waring == APP_STATUS_INDICATION_BOTHSCAN) &&
       (status == APP_STATUS_INDICATION_DISCONNECTED))) {
    last_voice_waring = status;
    return 0;
  }
  if (app_poweroff_flag == 1) {
    switch (status) {
    case APP_STATUS_INDICATION_POWEROFF:
      id = AUD_ID_POWER_OFF;
      break;
    default:
      return 0;
      break;
    }
  } else {
    switch (status) {
    case APP_STATUS_INDICATION_POWERON:
      id = AUD_ID_POWER_ON;
      break;
    case APP_STATUS_INDICATION_POWEROFF:
      id = AUD_ID_POWER_OFF;
      break;
    case APP_STATUS_INDICATION_CONNECTED:
      id = AUD_ID_BT_CONNECTED;
      break;
    case APP_STATUS_INDICATION_DISCONNECTED:
      id = AUD_ID_BT_DIS_CONNECT;
      break;
    case APP_STATUS_INDICATION_CALLNUMBER:
      id = AUD_ID_BT_CALL_INCOMING_NUMBER;
      break;
    case APP_STATUS_INDICATION_CHARGENEED:
      id = AUD_ID_BT_CHARGE_PLEASE;
      break;
    case APP_STATUS_INDICATION_FULLCHARGE:
      id = AUD_ID_BT_CHARGE_FINISH;
      break;
    case APP_STATUS_INDICATION_PAIRSUCCEED:
      id = AUD_ID_BT_PAIRING_SUC;
      break;
    case APP_STATUS_INDICATION_PAIRFAIL:
      id = AUD_ID_BT_PAIRING_FAIL;
      break;

    case APP_STATUS_INDICATION_HANGUPCALL:
      id = AUD_ID_BT_CALL_HUNG_UP;
      break;

    case APP_STATUS_INDICATION_REFUSECALL:
      id = AUD_ID_BT_CALL_REFUSE;
      isMerging = false;
      break;

    case APP_STATUS_INDICATION_ANSWERCALL:
      id = AUD_ID_BT_CALL_ANSWER;
      break;

    case APP_STATUS_INDICATION_CLEARSUCCEED:
      id = AUD_ID_BT_CLEAR_SUCCESS;
      break;

    case APP_STATUS_INDICATION_CLEARFAIL:
      id = AUD_ID_BT_CLEAR_FAIL;
      break;
    case APP_STATUS_INDICATION_INCOMINGCALL:
      id = AUD_ID_BT_CALL_INCOMING_CALL;
      break;
    case APP_STATUS_INDICATION_BOTHSCAN:
      id = AUD_ID_BT_PAIR_ENABLE;
      break;
    case APP_STATUS_INDICATION_WARNING:
      id = AUD_ID_BT_WARNING;
      break;
    case APP_STATUS_INDICATION_ALEXA_START:
      id = AUDIO_ID_BT_ALEXA_START;
      break;
    case APP_STATUS_INDICATION_ALEXA_STOP:
      id = AUDIO_ID_BT_ALEXA_STOP;
      break;
    case APP_STATUS_INDICATION_GSOUND_MIC_OPEN:
      id = AUDIO_ID_BT_GSOUND_MIC_OPEN;
      break;
    case APP_STATUS_INDICATION_GSOUND_MIC_CLOSE:
      id = AUDIO_ID_BT_GSOUND_MIC_CLOSE;
      break;
    case APP_STATUS_INDICATION_GSOUND_NC:
      id = AUDIO_ID_BT_GSOUND_NC;
      break;
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
    case APP_STATUS_RING_WARNING:
      id = AUD_ID_RING_WARNING;
      break;
#endif
    case APP_STATUS_INDICATION_MUTE:
      id = AUDIO_ID_BT_MUTE;
      break;
#ifdef __INTERACTION__
    case APP_STATUS_INDICATION_FINDME:
      id = AUD_ID_BT_FINDME;
      break;
#endif
    case APP_STATUS_INDICATION_FIND_MY_BUDS:
      id = AUDIO_ID_FIND_MY_BUDS;
      break;
    case APP_STATUS_INDICATION_TILE_FIND:
      id = AUDIO_ID_FIND_TILE;
      break;
    case APP_STATUS_INDICATION_DUDU:
      id = AUDIO_ID_BT_DUDU;
      break;
    case APP_STATUS_INDICATION_DU:
      id = AUDIO_ID_BT_DU;
      break;
    default:
      break;
    }
  }

  uint16_t aud_pram = 0;
  if (isMerging) {
    aud_pram |= PROMOT_ID_BIT_MASK_MERGING;
  }
#ifdef BT_USB_AUDIO_DUAL_MODE
  if (!btusb_is_usb_mode()) {
#if defined(IBRT)
    app_ibrt_if_voice_report_handler(id, aud_pram);
#else
    trigger_media_play(id, device_id, aud_pram);
#endif
  }

#else
#if defined(IBRT)
  aud_pram |= PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
  app_ibrt_if_voice_report_handler(id, aud_pram);
#else
  trigger_media_play(id, device_id, aud_pram);
#endif
#endif

  return 0;
}

extern "C" int app_voice_report(APP_STATUS_INDICATION_T status,
                                uint8_t device_id) {
  return app_voice_report_handler(status, device_id, true);
}

extern "C" int app_voice_report_generic(APP_STATUS_INDICATION_T status,
                                        uint8_t device_id, uint8_t isMerging) {
  return app_voice_report_handler(status, device_id, isMerging);
}

extern "C" int app_voice_stop(APP_STATUS_INDICATION_T status,
                              uint8_t device_id) {
  AUD_ID_ENUM id = MAX_RECORD_NUM;

  TRACE(2, "%s %d", __func__, status);

  if (status == APP_STATUS_INDICATION_FIND_MY_BUDS)
    id = AUDIO_ID_FIND_MY_BUDS;

  if (id != MAX_RECORD_NUM)
    trigger_media_stop(id, device_id);

  return 0;
}

#endif
#if 1 //! defined(BLE_ONLY_ENABLED)

static void app_poweron_normal(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  g_pwron_case = APP_POWERON_CASE_NORMAL;

  signal_send_to_main_thread(0x2);
}
static void app_poweron_scan(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  g_pwron_case = APP_POWERON_CASE_BOTHSCAN;

  signal_send_to_main_thread(0x2);
}
#endif

#if 0 // def __ENGINEER_MODE_SUPPORT__
#if !defined(BLE_ONLY_ENABLED)
static void app_poweron_factorymode(APP_KEY_STATUS *status, void *param)
{
    TRACE(3,"%s %d,%d",__func__, status->code, status->event);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_factorymode_enter();
}
#endif
#endif

#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
static bool g_pwron_finished = false;
static void app_poweron_finished(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  g_pwron_finished = true;
  signal_send_to_main_thread(0x2);
}
#endif

void app_poweron_wait_finished(void) {
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
  if (!g_pwron_finished)
#endif
  {
    osSignalWait(0x2, osWaitForever);
  }
}

#if defined(__POWERKEY_CTRL_ONOFF_ONLY__)
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
const APP_KEY_HANDLE pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_UP},
     "power on: shutdown",
     app_bt_key_shutdown,
     NULL},
};
#elif defined(__ENGINEER_MODE_SUPPORT__)
const APP_KEY_HANDLE pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITUP},
     "power on: normal",
     app_poweron_normal,
     NULL},
#if !defined(BLE_ONLY_ENABLED)
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITLONGPRESS},
     "power on: both scan",
     app_poweron_scan,
     NULL},
//    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGLONGPRESS},"power on: factory
//    mode", app_poweron_factorymode  , NULL},
#endif
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITFINISHED},
     "power on: finished",
     app_poweron_finished,
     NULL},
};
#else
const APP_KEY_HANDLE pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITUP},
     "power on: normal",
     app_poweron_normal,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITLONGPRESS},
     "power on: both scan",
     app_poweron_scan,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_INITFINISHED},
     "power on: finished",
     app_poweron_finished,
     NULL},
};
#endif

#ifndef APP_TEST_MODE
static void app_poweron_key_init(void) {
  uint8_t i = 0;
  TRACE(1, "%s", __func__);

  for (i = 0; i < (sizeof(pwron_key_handle_cfg) / sizeof(APP_KEY_HANDLE));
       i++) {
    app_key_handle_registration(&pwron_key_handle_cfg[i]);
  }
}

static uint8_t app_poweron_wait_case(void) {

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
  g_pwron_case = APP_POWERON_CASE_NORMAL;
#else
  uint32_t stime = 0, etime = 0;

  TRACE(1, "poweron_wait_case enter:%d", g_pwron_case);
  if (g_pwron_case == APP_POWERON_CASE_INVALID) {
    stime = hal_sys_timer_get();
    osSignalWait(0x2, POWERON_PRESSMAXTIME_THRESHOLD_MS);
    etime = hal_sys_timer_get();
  }
  TRACE(2, "powon raw case:%d time:%d", g_pwron_case,
        TICKS_TO_MS(etime - stime));
#endif
  return g_pwron_case;
}
#endif

static void app_wait_stack_ready(void) {
  uint32_t stime, etime;
  stime = hal_sys_timer_get();
  osSignalWait(0x3, 1000);
  etime = hal_sys_timer_get();
  TRACE(1, "app_wait_stack_ready: wait:%d ms", TICKS_TO_MS(etime - stime));

  app_stack_ready_cb();
}

extern "C" int system_shutdown(void);
int app_shutdown(void) {
  system_shutdown();
  return 0;
}

int system_reset(void);
int app_reset(void) {
  system_reset();
  return 0;
}

static void app_postponed_reset_timer_handler(void const *param);
osTimerDef(APP_POSTPONED_RESET_TIMER, app_postponed_reset_timer_handler);
static osTimerId app_postponed_reset_timer = NULL;
#define APP_RESET_PONTPONED_TIME_IN_MS 2000
static void app_postponed_reset_timer_handler(void const *param) {
  //    hal_cmu_sys_reboot();
  app_reset();
}

void app_start_postponed_reset(void) {
  if (NULL == app_postponed_reset_timer) {
    app_postponed_reset_timer =
        osTimerCreate(osTimer(APP_POSTPONED_RESET_TIMER), osTimerOnce, NULL);
  }

  hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);

  osTimerStart(app_postponed_reset_timer, APP_RESET_PONTPONED_TIME_IN_MS);
}

void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
  hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
  app_reset();
#else
  app_shutdown();
#endif
}

void app_bt_key_enter_testmode(APP_KEY_STATUS *status, void *param) {
  TRACE(1, "%s\n", __FUNCTION__);

  if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN) {
#ifdef __FACTORY_MODE_SUPPORT__
    app_factorymode_bt_signalingtest(status, param);
#endif
  }
}

void app_bt_key_enter_nosignal_mode(APP_KEY_STATUS *status, void *param) {
  TRACE(1, "%s\n", __FUNCTION__);
  if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN) {
#ifdef __FACTORY_MODE_SUPPORT__
    app_factorymode_bt_nosignalingtest(status, param);
#endif
  }
}

extern "C" void OS_NotifyEvm(void);

#define PRESS_KEY_TO_ENTER_OTA_INTERVEL (15000) // press key 15s enter to ota
#define PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT                                      \
  ((PRESS_KEY_TO_ENTER_OTA_INTERVEL - 2000) / 500)
void app_otaMode_enter(APP_KEY_STATUS *status, void *param) {
  TRACE(1, "%s", __func__);

  hal_norflash_disable_protection(HAL_NORFLASH_ID_0);

  hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
#ifdef __KMATE106__
  app_status_indication_set(APP_STATUS_INDICATION_OTA);
  app_voice_report(APP_STATUS_INDICATION_WARNING, 0);
  osDelay(1200);
#endif
  hal_cmu_sys_reboot();
}

#ifdef __USB_COMM__
void app_usb_cdc_comm_key_handler(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_CDC_COMM);
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#endif

#if 0
void app_dfu_key_handler(APP_KEY_STATUS *status, void *param)
{
    TRACE(1,"%s ",__func__);
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
    pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
    hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#else
void app_dfu_key_handler(APP_KEY_STATUS *status, void *param) {
  TRACE(1, "%s ", __func__);
  hal_sw_bootmode_clear(0xffffffff);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD |
                      HAL_SW_BOOTMODE_SKIP_FLASH_BOOT);
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  hal_cmu_reset_set(HAL_CMU_MOD_GLOBAL);
}
#endif

void app_ota_key_handler(APP_KEY_STATUS *status, void *param) {
  static uint32_t time = hal_sys_timer_get();
  static uint16_t cnt = 0;

  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  if (TICKS_TO_MS(hal_sys_timer_get() - time) >
      600) // 600 = (repeat key intervel)500 + (margin)100
    cnt = 0;
  else
    cnt++;

  if (cnt == PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT) {
    app_otaMode_enter(NULL, NULL);
  }

  time = hal_sys_timer_get();
}
extern "C" void app_bt_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
#define DEBUG_CODE_USE 0
  switch (status->event) {
  case APP_KEY_EVENT_CLICK:
    TRACE(0, "first blood!");
#if DEBUG_CODE_USE
    if (status->code == APP_KEY_CODE_PWR) {
#ifdef __INTERCONNECTION__
      // add favorite music
      // app_interconnection_handle_favorite_music_through_ccmp(1);

      // ask for ota update
      ota_update_request();
      return;
#else
      static int m = 0;
      if (m == 0) {
        m = 1;
        hal_iomux_set_analog_i2c();
      } else {
        m = 0;
        hal_iomux_set_uart0();
      }
#endif
    }
#endif
    break;
  case APP_KEY_EVENT_DOUBLECLICK:
    TRACE(0, "double kill");
#if DEBUG_CODE_USE
    if (status->code == APP_KEY_CODE_PWR) {
#ifdef __INTERCONNECTION__
      // play favorite music
      app_interconnection_handle_favorite_music_through_ccmp(2);
#else
      app_otaMode_enter(NULL, NULL);
#endif
      return;
    }
#endif
    break;
  case APP_KEY_EVENT_TRIPLECLICK:
    TRACE(0, "triple kill");
    if (status->code == APP_KEY_CODE_PWR) {

#ifndef __BT_ONE_BRING_TWO__
      if (btif_me_get_activeCons() < 1) {
#else
      if (btif_me_get_activeCons() < 2) {
#endif
        app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#ifdef __INTERCONNECTION__
        app_interceonnection_start_discoverable_adv(
            INTERCONNECTION_BLE_FAST_ADVERTISING_INTERVAL,
            APP_INTERCONNECTION_FAST_ADV_TIMEOUT_IN_MS);
        return;
#endif
#ifdef GFPS_ENABLED
        app_enter_fastpairing_mode();
#endif
        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
      }
      return;
    }
    break;
  case APP_KEY_EVENT_ULTRACLICK:
    TRACE(0, "ultra kill");
    break;
  case APP_KEY_EVENT_RAMPAGECLICK:
    TRACE(0, "rampage kill!you are crazy!");
    break;

  case APP_KEY_EVENT_UP:
    break;
  }
#if 0 // def __FACTORY_MODE_SUPPORT__
    if (app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN && (status->event == APP_KEY_EVENT_DOUBLECLICK)){
        app_factorymode_languageswitch_proc();
    }else
#endif
  {
#ifdef __SUPPORT_ANC_SINGLE_MODE_WITHOUT_BT__
    if (!anc_single_mode_on)
#endif
      bt_key_send(status);
  }
}

void app_voice_assistant_key(APP_KEY_STATUS *status, void *param) {
  TRACE(2, "%s event %d", __func__, status->event);
#if defined(BISTO_ENABLED) || defined(__AI_VOICE__)
  if (app_ai_manager_is_in_multi_ai_mode()) {
    if (app_ai_manager_spec_get_status_is_in_invalid()) {
      TRACE(0, "AI feature has been diabled");
      return;
    }

#ifdef MAI_TYPE_REBOOT_WITHOUT_OEM_APP
    if (app_ai_manager_get_spec_update_flag()) {
      TRACE(0, "device reboot is ongoing...");
      return;
    }
#endif

    if (app_ai_manager_is_need_reboot()) {
      TRACE(1, "%s ai need to reboot", __func__);
      return;
    }

    if (app_ai_manager_voicekey_is_enable()) {
      if (AI_SPEC_GSOUND == app_ai_manager_get_current_spec()) {
#ifdef BISTO_ENABLED
        gsound_custom_actions_handle_key(status, param);
#endif
      } else if (AI_SPEC_INIT != app_ai_manager_get_current_spec()) {
        app_ai_key_event_handle(status, 0);
      }
    }
  } else {
    app_ai_key_event_handle(status, 0);
#if defined(BISTO_ENABLED)
    gsound_custom_actions_handle_key(status, param);
#endif
  }
#endif
}

#ifdef IS_MULTI_AI_ENABLED
void app_voice_gva_onoff_key(APP_KEY_STATUS *status, void *param) {
  uint8_t current_ai_spec = app_ai_manager_get_current_spec();

  TRACE(2, "%s current_ai_spec %d", __func__, current_ai_spec);
  if (current_ai_spec == AI_SPEC_INIT) {
    app_ai_manager_enable(true, AI_SPEC_GSOUND);
  } else if (current_ai_spec == AI_SPEC_GSOUND) {
    app_ai_manager_enable(false, AI_SPEC_GSOUND);
  } else if (current_ai_spec == AI_SPEC_AMA) {
    app_ai_manager_switch_spec(AI_SPEC_GSOUND);
  }
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}

void app_voice_ama_onoff_key(APP_KEY_STATUS *status, void *param) {
  uint8_t current_ai_spec = app_ai_manager_get_current_spec();

  TRACE(2, "%s current_ai_spec %d", __func__, current_ai_spec);
  if (current_ai_spec == AI_SPEC_INIT) {
    app_ai_manager_enable(true, AI_SPEC_AMA);
  } else if (current_ai_spec == AI_SPEC_AMA) {
    app_ai_manager_enable(false, AI_SPEC_AMA);
  } else if (current_ai_spec == AI_SPEC_GSOUND) {
    app_ai_manager_switch_spec(AI_SPEC_AMA);
  }
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
}
#endif

#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
extern "C" void test_btusb_switch(void);
void app_btusb_audio_dual_mode_test(APP_KEY_STATUS *status, void *param) {
  TRACE(0, "test_btusb_switch");
  test_btusb_switch();
}
#endif

extern void switch_dualmic_status(void);

void app_switch_dualmic_key(APP_KEY_STATUS *status, void *param) {
  switch_dualmic_status();
}

#ifdef POWERKEY_I2C_SWITCH
extern void app_factorymode_i2c_switch(APP_KEY_STATUS *status, void *param);
#endif

#ifdef TILE_DATAPATH
extern "C" void app_tile_key_handler(APP_KEY_STATUS *status, void *param);
#endif

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#if defined(__APP_KEY_FN_STYLE_A__)
const APP_KEY_HANDLE app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_UP},
     "bt function key",
     app_bt_key_shutdown,
     NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_LONGPRESS},
     "bt function key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_UP}, "bt function key", app_bt_key, NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_DOUBLECLICK},
     "bt function key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP},
     "bt volume up key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_LONGPRESS},
     "bt play backward key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_UP},
     "bt volume down key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_LONGPRESS},
     "bt play forward key",
     app_bt_key,
     NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE, APP_KEY_EVENT_NONE},
     "none function key",
     app_bt_key,
     NULL},
#endif

};
#else // #elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_UP},
     "bt function key",
     app_bt_key_shutdown,
     NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_LONGPRESS},
     "bt function key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_UP}, "bt function key", app_bt_key, NULL},
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_DOUBLECLICK},
     "bt function key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_REPEAT},
     "bt volume up key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP},
     "bt play backward key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_REPEAT},
     "bt volume down key",
     app_bt_key,
     NULL},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_UP},
     "bt play forward key",
     app_bt_key,
     NULL},
#ifdef SUPPORT_SIRI
    {{APP_KEY_CODE_NONE, APP_KEY_EVENT_NONE},
     "none function key",
     app_bt_key,
     NULL},
#endif

};
#endif
#else
#if defined(__APP_KEY_FN_STYLE_A__)
//--
const APP_KEY_HANDLE app_key_handle_cfg[] = {
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGLONGPRESS},
   "bt function key",
   app_bt_key_shutdown,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGPRESS},
   "bt function key",
   app_bt_key,
   NULL},
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
   "bt function key",
   app_bt_key,
   NULL},
#ifdef RB_CODEC
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
   "bt function key",
   app_switch_player_key,
   NULL},
#else
//{{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"btusb mode switch
// key.",app_btusb_audio_dual_mode_test, NULL},
#endif
#endif
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_DOUBLECLICK},
   "bt function key",
   app_bt_key,
   NULL},
#ifdef TILE_DATAPATH
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_TRIPLECLICK},
   "bt function key",
   app_tile_key_handler,
   NULL},
#else
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_TRIPLECLICK},
   "bt function key",
   app_bt_key,
   NULL},
#endif
#if RAMPAGECLICK_TEST_MODE
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_ULTRACLICK},
   "bt function key",
   app_bt_key_enter_nosignal_mode,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_RAMPAGECLICK},
   "bt function key",
   app_bt_key_enter_testmode,
   NULL},
#endif
#ifdef POWERKEY_I2C_SWITCH
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_RAMPAGECLICK},
   "bt i2c key",
   app_factorymode_i2c_switch,
   NULL},
#endif
//{{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
//{{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key,
// NULL},
#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
  {{APP_KEY_CODE_FN1, APP_KEY_EVENT_DOUBLECLICK},
   "bt mode src snk key",
   app_bt_key,
   NULL},
#endif
  //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
  //{{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play forward
  // key",app_bt_key, NULL},
  //{{APP_KEY_CODE_FN15,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key,
  // NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
   "bt function key",
   app_bt_key,
   NULL},

#ifdef SUPPORT_SIRI
  {{APP_KEY_CODE_NONE, APP_KEY_EVENT_NONE},
   "none function key",
   app_bt_key,
   NULL},
#endif
#if defined(__BT_ANC_KEY__) && defined(ANC_APP)
#if defined(IBRT)
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK}, "bt anc key", app_anc_key, NULL},
#else
  {{APP_KEY_CODE_FN2, APP_KEY_EVENT_CLICK}, "bt anc key", app_anc_key, NULL},
#endif
#endif
#ifdef TILE_DATAPATH
  {{APP_KEY_CODE_TILE, APP_KEY_EVENT_DOWN},
   "tile function key",
   app_tile_key_handler,
   NULL},
  {{APP_KEY_CODE_TILE, APP_KEY_EVENT_UP},
   "tile function key",
   app_tile_key_handler,
   NULL},
#endif

#if defined(VOICE_DATAPATH) || defined(__AI_VOICE__)
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
#if defined(IS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED) ||                   \
    defined(PUSH_AND_HOLD_ENABLED) || defined(__TENCENT_VOICE__)
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
#endif
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP_AFTER_LONGPRESS},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
#endif
#ifdef IS_MULTI_AI_ENABLED
  {{APP_KEY_CODE_FN13, APP_KEY_EVENT_CLICK},
   "gva on-off key",
   app_voice_gva_onoff_key,
   NULL},
  {{APP_KEY_CODE_FN14, APP_KEY_EVENT_CLICK},
   "ama on-off key",
   app_voice_ama_onoff_key,
   NULL},
#endif
#if defined(BT_USB_AUDIO_DUAL_MODE_TEST) && defined(BT_USB_AUDIO_DUAL_MODE)
  {{APP_KEY_CODE_FN15, APP_KEY_EVENT_CLICK},
   "btusb mode switch key.",
   app_btusb_audio_dual_mode_test,
   NULL},
#endif
};
#else // #elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE app_key_handle_cfg[] = {
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGLONGPRESS},
   "bt function key",
   app_bt_key_shutdown,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGPRESS},
   "bt function key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
   "bt function key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_DOUBLECLICK},
   "bt function key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_TRIPLECLICK},
   "bt function key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_ULTRACLICK},
   "bt function key",
   app_bt_key_enter_nosignal_mode,
   NULL},
  {{APP_KEY_CODE_PWR, APP_KEY_EVENT_RAMPAGECLICK},
   "bt function key",
   app_bt_key_enter_testmode,
   NULL},
  {{APP_KEY_CODE_FN1, APP_KEY_EVENT_REPEAT},
   "bt volume up key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_FN1, APP_KEY_EVENT_UP},
   "bt play backward key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_FN2, APP_KEY_EVENT_REPEAT},
   "bt volume down key",
   app_bt_key,
   NULL},
  {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP},
   "bt play forward key",
   app_bt_key,
   NULL},
#ifdef SUPPORT_SIRI
  {{APP_KEY_CODE_NONE, APP_KEY_EVENT_NONE},
   "none function key",
   app_bt_key,
   NULL},
#endif

  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_FIRST_DOWN},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
#if defined(IS_GSOUND_BUTTION_HANDLER_WORKAROUND_ENABLED) ||                   \
    defined(PUSH_AND_HOLD_ENABLED)
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
#endif
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_UP_AFTER_LONGPRESS},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_LONGPRESS},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_CLICK},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
  {{APP_KEY_CODE_GOOGLE, APP_KEY_EVENT_DOUBLECLICK},
   "google assistant key",
   app_voice_assistant_key,
   NULL},
};
#endif
#endif

extern struct BT_DEVICE_T app_bt_device;
extern uint8_t avrcp_get_media_status(void);
/******************************delay_report_tone_timer*********************************************************/
APP_STATUS_INDICATION_T delay_report_tone_num = APP_STATUS_INDICATION_NUM;
osTimerId delay_report_toneid = NULL;
void startdelay_report_tone(int ms, APP_STATUS_INDICATION_T status);
void stopdelay_report_tone(void);
static void delay_report_tonefun(const void *);
osTimerDef(defdelay_report_tone, delay_report_tonefun);
void delay_report_toneinit(void) {
  delay_report_toneid =
      osTimerCreate(osTimer(defdelay_report_tone), osTimerOnce, (void *)0);
}
static void delay_report_tonefun(const void *) {
  TRACE(3, "\n\n!!!!!!enter %s,delay_report_tone_num = %d \n\n", __func__,
        delay_report_tone_num);
  if (MobileLinkLose_reboot) {
    return;
  }
  // static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  // uint8_t default_mobileaddr[6] = {0x0,0x0,0x0,0x0,0x0,0x0};
  // DUMP8("%02x ", p_ibrt_ctrl->mobile_addr.address, 6);
  // app_status_indication_set(APP_STATUS_INDICATION_TWS_CONNECTED);
  if (Curr_Is_Master()) {
    app_ibrt_sync_volume_info();
  }
  if (delay_report_tone_num == APP_STATUS_INDICATION_DUDU) {
    if (Curr_Is_Master() && (avrcp_get_media_status() != 1) &&
        (app_bt_device.hfchan_call[BT_DEVICE_ID_1] == BTIF_HF_CALL_NONE)) {
      app_voice_report(delay_report_tone_num, 0);
      /*if((!memcmp(default_mobileaddr,p_ibrt_ctrl->mobile_addr.address,6))&&(p_ibrt_ctrl->access_mode
      == 0x3)){
              TRACE(0,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
              app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);
      }*/
      if ((nv_record_get_paired_dev_count() == 1)) {
        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
      }
    }
  }
  delay_report_tone_num = APP_STATUS_INDICATION_NUM;
}

void startdelay_report_tone(int ms, APP_STATUS_INDICATION_T status) {
  TRACE(3, "\n\n !!!!!!!!!!start %s\n\n", __func__);
  delay_report_tone_num = status;
  osTimerStart(delay_report_toneid, ms);
}

/********************************delay_report_tone_timer*******************************************************/

/******************************low_latlatency_delay_switch_timer*********************************************************/
uint8_t latency_mode_is_open = false;

osTimerId low_latlatency_delay_switchid = NULL;
void startlow_latlatency_delay_switch(int ms);
void stoplow_latlatency_delay_switch(void);
static void low_latlatency_delay_switchfun(const void *);
osTimerDef(deflow_latlatency_delay_switch, low_latlatency_delay_switchfun);
void low_latlatency_delay_switchinit(void) {
  low_latlatency_delay_switchid = osTimerCreate(
      osTimer(deflow_latlatency_delay_switch), osTimerOnce, (void *)0);
}

void app_ibrt_ui_test_mtu_change_sync_notify(void) {
  extern volatile uint8_t avdtp_playback_delay_sbc_mtu;
  extern volatile uint8_t avdtp_playback_delay_aac_mtu;

  TRACE(3, "SWITC GAME MODE !!!latency_mode_is_open = %d###",
        latency_mode_is_open);
  if (!latency_mode_is_open) {
    avdtp_playback_delay_sbc_mtu = 32;
    avdtp_playback_delay_aac_mtu = 3;
    app_ibrt_if_force_audio_retrigger_slave_sync();
  } else {
    avdtp_playback_delay_sbc_mtu = 50;
    avdtp_playback_delay_aac_mtu = 6;
    app_ibrt_if_force_audio_retrigger_slave_sync();
  }
  latency_mode_is_open = !latency_mode_is_open;
  TRACE(3, "set SBC_MTU=%d AAC_MTU=%d\n", avdtp_playback_delay_sbc_mtu,
        avdtp_playback_delay_aac_mtu);
}

static void low_latlatency_delay_switchfun(const void *) {
  extern volatile uint8_t avdtp_playback_delay_sbc_mtu;
  extern volatile uint8_t avdtp_playback_delay_aac_mtu;

  latency_mode_is_open = !latency_mode_is_open;
  if (latency_mode_is_open) {
    avdtp_playback_delay_sbc_mtu = 32;
    avdtp_playback_delay_aac_mtu = 3;
    TRACE(3, "%s latency_mode_is_open!!", __func__);
  } else {
    avdtp_playback_delay_sbc_mtu = 50;
    avdtp_playback_delay_aac_mtu = 6;
    TRACE(3, "%s latency_mode_is_close!!", __func__);
  }
  app_ibrt_if_force_audio_retrigger();
  app_ibrt_customif_test3_cmd_send(&latency_mode_is_open, 1);
}

void startlow_latlatency_delay_switch(int ms) {
  osTimerStart(low_latlatency_delay_switchid, ms);
}

void stoplow_latlatency_delay_switch(void) {
  osTimerStop(low_latlatency_delay_switchid);
}

/********************************low_latlatency_delay_switch_timer*******************************************************/

void app_latency_switch_key_handler(void) {

  if ((btif_me_get_activeCons() > 0) &&
      (app_bt_device.hfchan_call[BT_DEVICE_ID_1] == BTIF_HF_CALL_NONE) &&
      (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] ==
       BTIF_HF_CALL_SETUP_NONE)) {
    if (latency_mode_is_open) {
      app_voice_report(APP_STATUS_INDICATION_ALEXA_STOP,
                       0); // close latlatency mode
    } else {
      app_voice_report(APP_STATUS_INDICATION_ALEXA_START,
                       0); // close latlatency mode
    }
    startlow_latlatency_delay_switch(1500);
  }
}

void stopAuto_Shutdowm_Timer(void);

bool MobileLinkLose_reboot = false;
bool IsMobileLinkLossing = FALSE;
bool IsTwsLinkLossing = FALSE;
bool enterpairing_flag = false;
bool IsTwsLinkdiscon = false;
bool reconnect_fail_fail_flag = false;
static void app_enterpairing_timehandler(void const *param);
osTimerDef(APP_ENTERPAIRING_TIMER, app_enterpairing_timehandler);
static osTimerId enterpairing_timer = NULL;
static void app_enterpairing_timehandler(void const *param) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  static app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();
  TRACE(3, "xqd log --%s\n", __func__);
  TRACE(3, "MyLog: is_tws_con = %d, nv_role = 0x%x",
        app_tws_ibrt_tws_link_connected(), p_ibrt_ctrl->nv_role);
  reconnect_fail_fail_flag = true;
  enterpairing_flag = false;
  if (IsMobileLinkLossing) {
    TRACE(3, "xqd---log:return evt 0!!!\n");
    return;
  }
  if (p_ui_ctrl->box_state == IBRT_IN_BOX_CLOSED) {
    TRACE(3, "xqd---log:return evt 1!!!\n");
    return;
  }
  if ((app_battery_is_charging()) || ((app_device_bt_is_connected()) &&
                                      (app_tws_ibrt_mobile_link_connected()) &&
                                      (app_tws_ibrt_mobile_link_connected())))
    return;

  if (app_tws_ibrt_mobile_link_connected() ||
      app_tws_ibrt_slave_ibrt_link_connected()) {
    TRACE(3, "xqd---log:return evt 2!!!\n");
    return;
  }

  if (app_ibrt_ui_get_enter_pairing_mode()) {
    TRACE(3, "xqd---log:return evt 3!!!\n");
    if ((IBRT_UNKNOW != p_ibrt_ctrl->nv_role) &&
        (nv_record_get_paired_dev_count() >= 2)) {
      app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
    }
    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
    return;
  }

  if (app_tws_ibrt_tws_link_connected() &&
      (p_ibrt_ctrl->current_role == IBRT_MASTER)) {
    TRACE(3, "xqd---log:return evt 4!!!\n");

    // p_ui_ctrl->config.enter_pairing_on_reconnect_mobile_failed = true;
    // p_ui_ctrl->config.nv_slave_enter_pairing_on_mobile_disconnect = true;
    // app_ibrt_if_enter_freeman_pairing();
    // app_ibrt_if_enter_pairing_after_tws_connected();
    app_ibrt_ui_set_enter_pairing_mode(IBRT_CONNECT_MOBILE_FAILED);
    app_ibrt_ui_judge_scan_type(IBRT_CONNECTE_TRIGGER, MOBILE_LINK, 0);
    app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
  } else if ((!app_tws_ibrt_tws_link_connected()) &&
             (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
             (p_ibrt_ctrl->access_mode != 0x3)) {
    TRACE(3, "xqd---log:return evt 5!!!\n");
    app_ibrt_ui_set_enter_pairing_mode(IBRT_CONNECT_MOBILE_FAILED);
    app_ibrt_ui_judge_scan_type(IBRT_CONNECTE_TRIGGER, MOBILE_LINK, 0);
    app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
  } else if ((!app_tws_ibrt_tws_link_connected()) &&
             (p_ibrt_ctrl->nv_role == IBRT_SLAVE)) {
    app_ibrt_ui_set_enter_pairing_mode(IBRT_CONNECT_MOBILE_FAILED);
    app_ibrt_ui_judge_scan_type(IBRT_CONNECTE_TRIGGER, MOBILE_LINK, 0);
    app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
  }
}
void app_enterpairing_timer_start(void) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  TRACE(3,
        "xqd log --app_enterpairing_timer_start, enterpairing_flag = "
        "%d,nv_role = %d\n",
        enterpairing_flag, p_ibrt_ctrl->nv_role);
  if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW)
    return;

  enterpairing_flag = true;

  osTimerStop(enterpairing_timer);
  osTimerStart(enterpairing_timer, 11000);
}
void app_enterpairing_timer_stop(void) {
  TRACE(3, "xqd log --app_enterpairing_timer_stop\n");
  if (enterpairing_flag)
    osTimerStop(enterpairing_timer);
}
void app_enterpairing_timer_open(void) {
  if (NULL == enterpairing_timer) {
    enterpairing_timer =
        osTimerCreate(osTimer(APP_ENTERPAIRING_TIMER), osTimerOnce, (void *)0);
  }
}
extern bool IsMobileLinkLossing;
extern bool IsTwsLinkLossing;
bool LINKLOSE_REBOOT_ENABLE = false;

osTimerId Auto_Shutdowm_Timerid = NULL;
void startAuto_Shutdowm_Timer(int ms);

static void Auto_Shutdowm_Timerfun(const void *);
osTimerDef(defAuto_Shutdowm_Timer, Auto_Shutdowm_Timerfun);
void Auto_Shutdowm_Timerinit(void) {
  Auto_Shutdowm_Timerid = osTimerCreate(osTimer(defAuto_Shutdowm_Timer),
                                        osTimerPeriodic, (void *)0);
  startAuto_Shutdowm_Timer(5000);
}
extern struct btdevice_volume *btdevice_volume_p;

static void Auto_Shutdowm_Timerfun(const void *) {
  static uint32_t auto_shutdown_cnt = 0;
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  static app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();
  struct nvrecord_env_t *nvrecord_env;
  nv_record_env_get(&nvrecord_env);
  static uint8_t trace_cnt = 0;
  APP_BATTERY_MV_T currvolt = 0;
  // static uint8_t charge_full_timeout_cnt = 0;
  trace_cnt++;
  if (trace_cnt >= 2) {
    trace_cnt = 0;
    /*
    TRACE(0,"xqd---log:curr_role = %d,nv_role = %d,box_state = %d,access_mode =
    %d,channel =
    %d",p_ibrt_ctrl->current_role,p_ibrt_ctrl->nv_role,p_ui_ctrl->box_state,p_ibrt_ctrl->access_mode,tgt_tws_get_channel_is_right());
    TRACE(0,"curr_avrcp_status = %d£¬localbat = %d,slave_bat =
    %d",avrcp_get_media_status(),p_ibrt_ctrl->local_battery_volt,p_ibrt_ctrl->peer_battery_volt);
    TRACE(0,"a2dp vol : %d", btdevice_volume_p->a2dp_vol);
    //  TRACE(0,"hfp vol: %d", btdevice_volume_p->hfp_vol);
    TRACE(0,"qxw---nv_ibrt_mode addr:");
    DUMP8("0x%x ", nvrecord_env->ibrt_mode.record.bdAddr.address, 6);
    TRACE(0,"qxw---peer addr:");
    DUMP8("%02x ", p_ibrt_ctrl->peer_addr.address, 6);
    TRACE(0,"qxw---local addr:");
    DUMP8("%02x ", p_ibrt_ctrl->local_addr.address, 6);
    TRACE(0,"qxw---nv_master addr:");
    DUMP8("%02x ", nvrecord_env->master_bdaddr.address, 6);
    TRACE(0,"qxw---nv_slave addr:");
    DUMP8("%02x ", nvrecord_env->slave_bdaddr.address, 6);*/
    // TRACE(2,"charge_gpio statu:%d,chargefull_gpio statu:%d,curr_charge status
    // = %d!",hal_gpio_pin_get_val((enum
    // HAL_GPIO_PIN_T)cfg_hw_charge_indication_cfg.pin),hal_gpio_pin_get_val((enum
    // HAL_GPIO_PIN_T)cfg_hw_charge_full_indication_cfg.pin),app_battery_is_charging());
  }

  // TRACE(3,"GIOI25 = %d,charge sta = %d!!!!!",hal_gpio_pin_get_val((enum
  // HAL_GPIO_PIN_T)app_battery_charger_full_indicator_cfg.pin),app_battery_is_charging());
  if (app_battery_is_charging()) {
    if ((p_ui_ctrl->box_state != IBRT_IN_BOX_CLOSED)) {
      app_ibrt_ui_event_entry(IBRT_CLOSE_BOX_EVENT);
    }
  }
  app_battery_get_info(&currvolt, NULL, NULL);
  if (p_ui_ctrl->box_state == IBRT_OUT_BOX) {
    if (app_device_bt_is_connected()) {
      IsMobileLinkLossing = FALSE;
      IsTwsLinkLossing = FALSE;
      MobileLinkLose_reboot = FALSE;
    }
    if ((!app_device_bt_is_connected()) &&
        p_ibrt_ctrl->current_role != BTIF_BCR_SLAVE) {
      auto_shutdown_cnt++;
      if (auto_shutdown_cnt == Auto_Shutdowm_TIME / 5) {
        TRACE(0, "xqd---shutdown!!@@@@@@!!");
        auto_shutdown_cnt = 0;
        app_bt_power_off_customize();
      }
    } else {
      auto_shutdown_cnt = 0;
    }
    /*if((auto_shutdown_cnt % 4 ==
    0)&&(MobileLinkLose_reboot)&&(!app_qxw_bt_is_connected())&&(p_ibrt_ctrl->current_role
    != BTIF_BCR_SLAVE)){ app_ibrt_if_event_entry(IBRT_PHONE_CONNECT_EVENT);
    }*/
  } else {
    auto_shutdown_cnt = 0;
  }

#if (QXW_TOUCH_INEAR_DET)
  last_tws_con_status = app_tws_ibrt_tws_link_connected();
#endif
}

void startAuto_Shutdowm_Timer(int ms) {
  osTimerStart(Auto_Shutdowm_Timerid, ms);
}
void stopAuto_Shutdowm_Timer(void) {
  TRACE(0, "%s", __func__);
  osTimerStop(Auto_Shutdowm_Timerid);
}

/********************************Auto_Shutdowm_Timer_timer*******************************************************/
extern void touch_reset(void);
/*******************************once_delay_event_Timer__timer*********************************************************/
uint8_t once_event_case = 0;
osTimerId once_delay_event_Timer_id = NULL;
void startonce_delay_event_Timer_(int ms);
void stoponce_delay_event_Timer_(void);
static void once_delay_event_Timer_fun(const void *);
osTimerDef(defonce_delay_event_Timer_, once_delay_event_Timer_fun);
void once_delay_event_Timer_init(void) {
  TRACE(3, "%s", __func__);
  once_delay_event_Timer_id = osTimerCreate(osTimer(defonce_delay_event_Timer_),
                                            osTimerOnce, (void *)0);
}
static void once_delay_event_Timer_fun(const void *) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if ((p_ibrt_ctrl->current_role == IBRT_SLAVE)) {
    once_event_case = 0;
    TRACE(3, "delay_report_tonefun RETURN!!!");
    return;
  }
  TRACE(3, "\n\n!!!!!!enter %s,mobile_link = %d,event = %d\n\n", __func__,
        app_tws_ibrt_mobile_link_connected(), once_event_case);
  switch (once_event_case) {
  case 1:
    if (/*(avrcp_get_media_status() != 1)&&*/ (
            app_bt_device.a2dp_state[BT_DEVICE_ID_1] == 1) &&
        (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == 0) &&
        (p_ibrt_ctrl->current_role == IBRT_MASTER))
      app_voice_report(APP_STATUS_INDICATION_CONNECTED, 0);
    break;
  case 2:
    if ((IsMobileLinkLossing || app_device_bt_is_connected()))
      break;
    app_voice_report(APP_STATUS_INDICATION_DISCONNECTED, 0);
    app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
    // once_event_case = 3;
    //        startonce_delay_event_Timer_(2000);
    break;
  case 3:
    if (IsMobileLinkLossing) {
      return;
    }
    if ((app_device_bt_is_connected()))
      return;
    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
    app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
    if (p_ibrt_ctrl->nv_role == IBRT_MASTER) {
      app_ibrt_if_enter_pairing_after_tws_connected();
    } else if (p_ibrt_ctrl->nv_role == IBRT_SLAVE) {
      app_ibrt_ui_set_enter_pairing_mode(IBRT_CONNECT_MOBILE_FAILED);
      app_ibrt_ui_judge_scan_type(IBRT_CONNECTE_TRIGGER, MOBILE_LINK, 0);
    } else if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW) {
      app_ibrt_ui_judge_scan_type(IBRT_FREEMAN_PAIR_TRIGGER, NO_LINK_TYPE,
                                  IBRT_UI_NO_ERROR);
      app_ibrt_ui_set_freeman_enable();
    }
    break;

  case 4:
    if (!app_device_bt_is_connected()) {
      app_voice_report_generic(APP_STATUS_INDICATION_DISCONNECTED, 0, 0);
    }
    LINKLOSE_REBOOT_ENABLE = true;
    break;
  case 5:
    if (!app_device_bt_is_connected() && !app_tws_ibrt_tws_link_connected()) {
      app_voice_report_generic(APP_STATUS_INDICATION_DISCONNECTED, 0, 0);
    }
    LINKLOSE_REBOOT_ENABLE = true;
    break;
  case 8:
    app_ibrt_sync_volume_info();
    break;
  case 9:
    break;
  default:
    break;
  }
  once_event_case = 0;
}

void startonce_delay_event_Timer_(int ms) {
  TRACE(3, "\n\n !!!!!!!!!!start %s\n\n", __func__);
  osTimerStart(once_delay_event_Timer_id, ms);
}
void stoponce_delay_event_Timer_(void) {
  TRACE(3, "\n\n!!!!!!!!!!  stop %s\n\n", __func__);
  osTimerStop(once_delay_event_Timer_id);
}

/********************************once_delay_event_Timer__timer*******************************************************/
/***********************************************************************************************/

extern bt_status_t LinkDisconnectDirectly(bool PowerOffFlag);
void a2dp_suspend_music_force(void);

bool app_is_power_off_in_progress(void) {
  return app_poweroff_flag ? TRUE : FALSE;
}

#if GFPS_ENABLED
#define APP_GFPS_BATTERY_TIMEROUT_VALUE (10000)
static void app_gfps_battery_show_timeout_timer_cb(void const *n);
osTimerDef(GFPS_BATTERY_SHOW_TIMEOUT_TIMER,
           app_gfps_battery_show_timeout_timer_cb);
static osTimerId app_gfps_battery_show_timer_id = NULL;
#include "app_gfps.h"
static void app_gfps_battery_show_timeout_timer_cb(void const *n) {
  TRACE(1, "%s", __func__);
  app_gfps_set_battery_datatype(HIDE_UI_INDICATION);
}

void app_gfps_battery_show_timer_start() {
  if (app_gfps_battery_show_timer_id == NULL)
    app_gfps_battery_show_timer_id = osTimerCreate(
        osTimer(GFPS_BATTERY_SHOW_TIMEOUT_TIMER), osTimerOnce, NULL);
  osTimerStart(app_gfps_battery_show_timer_id, APP_GFPS_BATTERY_TIMEROUT_VALUE);
}

void app_gfps_battery_show_timer_stop() {
  if (app_gfps_battery_show_timer_id)
    osTimerStop(app_gfps_battery_show_timer_id);
}
#endif

int app_deinit(int deinit_case) {
  int nRet = 0;
  TRACE(2, "%s case:%d", __func__, deinit_case);
  app_tws_if_trigger_role_switch();
  osDelay(200); // This is a hack; a hackkitttyy hack. To wait for the tws
                // exchange to occur
#ifdef WL_DET
  app_mic_alg_audioloop(false, APP_SYSFREQ_78M);
#endif

#ifdef __PC_CMD_UART__
  app_cmd_close();
#endif
#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
  if (app_usbaudio_mode_on())
    return 0;
#endif
  if (!deinit_case) {
    app_poweroff_flag = 1;
#if defined(APP_LINEIN_A2DP_SOURCE)
    app_audio_sendrequest(APP_A2DP_SOURCE_LINEIN_AUDIO,
                          (uint8_t)APP_BT_SETTING_CLOSE, 0);
#endif
#if defined(APP_I2S_A2DP_SOURCE)
    app_audio_sendrequest(APP_A2DP_SOURCE_I2S_AUDIO,
                          (uint8_t)APP_BT_SETTING_CLOSE, 0);
#endif
    app_status_indication_filter_set(APP_STATUS_INDICATION_BOTHSCAN);
    app_audio_sendrequest(APP_BT_STREAM_INVALID,
                          (uint8_t)APP_BT_SETTING_CLOSEALL, 0);
    osDelay(500);
    LinkDisconnectDirectly(true);
    osDelay(500);
    app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
#endif
#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_DEINIT);
#endif
    nv_record_flash_flush();
    norflash_api_flush_all();
#if defined(DUMP_LOG_ENABLE)
    log_dump_flush_all();
#endif
    osDelay(1000);
    af_close();
  }

  return nRet;
}

int app_bt_connect2tester_init(void) {
  btif_device_record_t rec;
  bt_bdaddr_t tester_addr;
  uint8_t i;
  bool find_tester = false;
  struct nvrecord_env_t *nvrecord_env;
  btdevice_profile *btdevice_plf_p;
  nv_record_env_get(&nvrecord_env);

  if (nvrecord_env->factory_tester_status.status !=
      NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT)
    return 0;

  if (!nvrec_dev_get_dongleaddr(&tester_addr)) {
    nv_record_open(section_usrdata_ddbrecord);
    for (i = 0; nv_record_enum_dev_records(i, &rec) == BT_STS_SUCCESS; i++) {
      if (!memcmp(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE)) {
        find_tester = true;
      }
    }
    if (i == 0 && !find_tester) {
      memset(&rec, 0, sizeof(btif_device_record_t));
      memcpy(rec.bdAddr.address, tester_addr.address, BTIF_BD_ADDR_SIZE);
      nv_record_add(section_usrdata_ddbrecord, &rec);
      btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(
          rec.bdAddr.address);
      nv_record_btdevicerecord_set_hfp_profile_active_state(btdevice_plf_p,
                                                            true);
      nv_record_btdevicerecord_set_a2dp_profile_active_state(btdevice_plf_p,
                                                             true);
    }
    if (find_tester && i > 2) {
      nv_record_ddbrec_delete(&tester_addr);
      nvrecord_env->factory_tester_status.status =
          NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
      nv_record_env_set(nvrecord_env);
    }
  }

  return 0;
}

int app_nvrecord_rebuild(void) {
  struct nvrecord_env_t *nvrecord_env;
  nv_record_env_get(&nvrecord_env);

  nv_record_sector_clear();
  nv_record_env_init();
  nv_record_update_factory_tester_status(
      NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS);
  nv_record_env_set(nvrecord_env);
  nv_record_flash_flush();

  return 0;
}

#if (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE))
#include "app_audio.h"
#include "usb_audio_app.h"
#include "usb_audio_frm_defs.h"

static bool app_usbaudio_mode = false;

extern "C" void btusbaudio_entry(void);
void app_usbaudio_entry(void) {
  btusbaudio_entry();
  app_usbaudio_mode = true;
}

bool app_usbaudio_mode_on(void) { return app_usbaudio_mode; }

void app_usb_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
}

const APP_KEY_HANDLE app_usb_handle_cfg[] = {
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_UP},
     "USB HID FN1 UP key",
     app_usb_key,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP},
     "USB HID FN2 UP key",
     app_usb_key,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_UP},
     "USB HID PWR UP key",
     app_usb_key,
     NULL},
};

void app_usb_key_init(void) {
  uint8_t i = 0;
  TRACE(1, "%s", __func__);
  for (i = 0; i < (sizeof(app_usb_handle_cfg) / sizeof(APP_KEY_HANDLE)); i++) {
    app_key_handle_registration(&app_usb_handle_cfg[i]);
  }
}
#endif /* (defined(BTUSB_AUDIO_MODE) || defined(BT_USB_AUDIO_DUAL_MODE)) */

// #define OS_HAS_CPU_STAT 1
#if OS_HAS_CPU_STAT
extern "C" void rtx_show_all_threads_usage(void);
#define _CPU_STATISTICS_PEROID_ 6000
#define CPU_USAGE_TIMER_TMO_VALUE (_CPU_STATISTICS_PEROID_ / 3)
static void cpu_usage_timer_handler(void const *param);
osTimerDef(cpu_usage_timer, cpu_usage_timer_handler);
static osTimerId cpu_usage_timer_id = NULL;
static void cpu_usage_timer_handler(void const *param) {
  rtx_show_all_threads_usage();
}
#endif

#ifdef USER_REBOOT_PLAY_MUSIC_AUTO
bool a2dp_need_to_play = false;
#endif
extern void btif_me_write_bt_sleep_enable(uint8_t sleep_en);

int btdrv_tportopen(void);

void app_ibrt_init(void) {
  app_bt_global_handle_init();
#if defined(IBRT)
  ibrt_config_t config;
  app_tws_ibrt_init();
  app_ibrt_ui_init();
  app_ibrt_ui_test_init();
  app_ibrt_if_config_load(&config);
  app_ibrt_customif_ui_start();
#ifdef IBRT_SEARCH_UI
  app_tws_ibrt_start(&config, true);
  app_ibrt_search_ui_init(false, IBRT_NONE_EVENT);
#else
  app_tws_ibrt_start(&config, false);
#endif

#ifdef POWER_ON_ENTER_TWS_PAIRING_ENABLED
  app_ibrt_ui_event_entry(IBRT_TWS_PAIRING_EVENT);
#endif

#endif
}

void user_io_timer_init(void) {
  // app_mute_ctrl_init();
  LED_statusinit();
  // pwrkey_detinit();
  Auto_Shutdowm_Timerinit();
  delay_report_toneinit();
  once_delay_event_Timer_init();
  // app_i2c_demo_init();
  // tou_io_init();
}

extern uint32_t __coredump_section_start[];
extern uint32_t __ota_upgrade_log_start[];
extern uint32_t __log_dump_start[];
extern uint32_t __crash_dump_start[];
extern uint32_t __custom_parameter_start[];
extern uint32_t __lhdc_license_start[];
extern uint32_t __aud_start[];
extern uint32_t __userdata_start[];
extern uint32_t __factory_start[];

int app_init(void) {
  int nRet = 0;
  struct nvrecord_env_t *nvrecord_env;
#ifdef POWER_ON_ENTER_TWS_PAIRING_ENABLED
  bool need_check_key = false;
#else
  bool need_check_key = false;
#endif
  uint8_t pwron_case = APP_POWERON_CASE_INVALID;
#ifdef BT_USB_AUDIO_DUAL_MODE
  uint8_t usb_plugin = 0;
#endif
#ifdef IBRT_SEARCH_UI
  bool is_charging_poweron = false;
#endif
  TRACE(0, "please check all sections sizes and heads is correct ........");
  TRACE(2, "__coredump_section_start: %p length: 0x%x",
        __coredump_section_start, CORE_DUMP_SECTION_SIZE);
  TRACE(2, "__ota_upgrade_log_start: %p length: 0x%x", __ota_upgrade_log_start,
        OTA_UPGRADE_LOG_SIZE);
  TRACE(2, "__log_dump_start: %p length: 0x%x", __log_dump_start,
        LOG_DUMP_SECTION_SIZE);
  TRACE(2, "__crash_dump_start: %p length: 0x%x", __crash_dump_start,
        CRASH_DUMP_SECTION_SIZE);
  TRACE(2, "__custom_parameter_start: %p length: 0x%x",
        __custom_parameter_start, CUSTOM_PARAMETER_SECTION_SIZE);
  TRACE(2, "__lhdc_license_start: %p length: 0x%x", __lhdc_license_start,
        LHDC_LICENSE_SECTION_SIZE);
  TRACE(2, "__userdata_start: %p length: 0x%x", __userdata_start,
        USERDATA_SECTION_SIZE * 2);
  TRACE(2, "__aud_start: %p length: 0x%x", __aud_start, AUD_SECTION_SIZE);
  TRACE(2, "__factory_start: %p length: 0x%x", __factory_start,
        FACTORY_SECTION_SIZE);

  TRACE(0, "app_init\n");
  app_tws_set_side_from_gpio();
#ifdef __RPC_ENABLE__
  extern int rpc_service_setup(void);
  rpc_service_setup();
#endif

#ifdef IBRT
  // init tws interface
  app_tws_if_init();
#endif // #ifdef IBRT

  nv_record_init();
  factory_section_init();

#ifdef ANC_APP
  app_anc_ios_init();
#endif
  app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_104M);
#if defined(MCU_HIGH_PERFORMANCE_MODE)
  TRACE(1, "sys freq calc : %d\n", hal_sys_timer_calc_cpu_freq(5, 0));
#endif
  list_init();
  nRet = app_os_init();
  if (nRet) {
    goto exit;
  }
#if OS_HAS_CPU_STAT
  cpu_usage_timer_id =
      osTimerCreate(osTimer(cpu_usage_timer), osTimerPeriodic, NULL);
  if (cpu_usage_timer_id != NULL) {
    osTimerStart(cpu_usage_timer_id, CPU_USAGE_TIMER_TMO_VALUE);
  }
#endif

  // app_status_indication_init();

#ifdef BTADDR_FOR_DEBUG
  gen_bt_addr_for_debug();
#endif

#ifdef FORCE_SIGNALINGMODE
  hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE |
                      HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
#elif defined FORCE_NOSIGNALINGMODE
  hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE |
                      HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
#endif

  if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT_FROM_CRASH) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT_FROM_CRASH);
    TRACE(0, "Crash happened!!!");
#ifdef VOICE_DATAPATH
    gsound_dump_set_flag(true);
#endif
  }

  if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    pwron_case = APP_POWERON_CASE_REBOOT;
    need_check_key = false;
    TRACE(0, "Initiative REBOOT happens!!!");
#ifdef USER_REBOOT_PLAY_MUSIC_AUTO
    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_LOCAL_PLAYER) {
      hal_sw_bootmode_clear(HAL_SW_BOOTMODE_LOCAL_PLAYER);
      a2dp_need_to_play = true;
      TRACE(0, "a2dp_need_to_play = true");
    }
#endif
  }

  if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
    pwron_case = APP_POWERON_CASE_TEST;
    need_check_key = false;
    TRACE(0, "To enter test mode!!!");
  }

#ifdef BT_USB_AUDIO_DUAL_MODE
  usb_os_init();
#endif
  nRet = app_battery_open();
  TRACE(1, "Yin BATTERY %d", nRet);
  if (pwron_case != APP_POWERON_CASE_TEST) {
#ifdef USER_REBOOT_PLAY_MUSIC_AUTO
    TRACE(0, "hal_sw_bootmode_clear HAL_SW_BOOTMODE_LOCAL_PLAYER!!!!!!");
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_LOCAL_PLAYER);
#endif
    switch (nRet) {
    case APP_BATTERY_OPEN_MODE_NORMAL:
      nRet = 0;
      break;
    case APP_BATTERY_OPEN_MODE_CHARGING:
      app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
      TRACE(0, "CHARGING!");
      app_battery_start();

      app_key_open(false);
      app_key_init_on_charging();
      nRet = 0;
#if defined(BT_USB_AUDIO_DUAL_MODE)
      usb_plugin = 1;
#elif defined(BTUSB_AUDIO_MODE)
      goto exit;
#endif
      goto exit;
      break;
    case APP_BATTERY_OPEN_MODE_CHARGING_PWRON:
      TRACE(0, "CHARGING PWRON!");
#ifdef IBRT_SEARCH_UI
      is_charging_poweron = true;
#endif
      app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
      need_check_key = false;
      nRet = 0;
      break;
    case APP_BATTERY_OPEN_MODE_INVALID:
    default:
      nRet = -1;
      goto exit;
      break;
    }
  }

  if (app_key_open(need_check_key)) {
    TRACE(0, "PWR KEY DITHER!");
    nRet = -1;
    goto exit;
  }

  hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);
  app_poweron_key_init();
#if defined(_AUTO_TEST_)
  AUTO_TEST_SEND("Power on.");
#endif
  app_bt_init();
  af_open();
  app_audio_open();
  app_audio_manager_open();
  app_overlay_open();

  nv_record_env_init();
  nvrec_dev_data_open();
  factory_section_open();

  /*****************************************************************************/
  //    app_bt_connect2tester_init();
  nv_record_env_get(&nvrecord_env);

#ifdef AUDIO_LOOPBACK

#ifdef WL_DET
  app_mic_alg_audioloop(true, APP_SYSFREQ_78M);
#endif

  while (1)
    ;
#endif

#ifdef BISTO_ENABLED
  nv_record_gsound_rec_init();
#endif

#ifdef BLE_ENABLE
  app_ble_mode_init();
  app_ble_customif_init();
#ifdef IBRT
  app_ble_force_switch_adv(BLE_SWITCH_USER_IBRT, false);
#endif // #ifdef IBRT
#endif

  audio_process_init();
#ifdef __PC_CMD_UART__
  app_cmd_open();
#endif
#ifdef AUDIO_DEBUG_V0_1_0
  speech_tuning_init();
#endif
#ifdef ANC_APP
  app_anc_open_module();
#endif

#ifdef MEDIA_PLAYER_SUPPORT
  app_play_audio_set_lang(nvrecord_env->media_language.language);
#endif
  app_bt_stream_volume_ptr_update(NULL);

#ifdef __THIRDPARTY
  app_thirdparty_init();
  app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                           THIRDPARTY_INIT);
#endif

  // TODO: freddie->unify all of the OTA modules
#if defined(IBRT_OTA)
  ota_flash_init();
#endif

#ifdef OTA_ENABLED
  /// init OTA common module
  ota_common_init_handler();
#endif // OTA_ENABLED

#ifdef IBRT
  // for TWS side decision, the last bit is 1:right, 0:left
  if (app_tws_is_unknown_side()) {
    // app_tws_set_side_from_addr(factory_section_get_bt_address());
    app_tws_set_side_from_gpio();
  }
#endif

  btdrv_start_bt();
#if defined(__GMA_VOICE__) && defined(IBRT_SEARCH_UI)
  app_ibrt_reconfig_btAddr_from_nv();
#endif

  if (pwron_case != APP_POWERON_CASE_TEST) {
    BesbtInit();
    app_wait_stack_ready();
    bt_drv_extra_config_after_init();
    bt_generate_ecdh_key_pair();
    app_bt_start_custom_function_in_bt_thread((uint32_t)0, 0,
                                              (uint32_t)app_ibrt_init);
  }
#if defined(BLE_ENABLE) && defined(IBRT)
  app_ble_force_switch_adv(BLE_SWITCH_USER_IBRT, true);
#endif
  app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_52M);
  TRACE(1, "\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);

  if (pwron_case == APP_POWERON_CASE_REBOOT) {
    app_status_indication_init();
    user_io_timer_init();
    app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
    app_bt_start_custom_function_in_bt_thread(
        (uint32_t)1, 0, (uint32_t)btif_me_write_bt_sleep_enable);
    btdrv_set_lpo_times();

#if defined(IBRT_OTA)
    bes_ota_init();
#endif
    // app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
    if (is_charging_poweron == false) {
      if (IBRT_UNKNOW == nvrecord_env->ibrt_mode.mode) {
        TRACE(0, "ibrt_ui_log:power on unknow mode");
        app_ibrt_enter_limited_mode();
        // if(app_tws_is_right_side())
        if (1) {
          TRACE(0, "app_start_tws_serching_direactly");
          app_start_tws_serching_direactly();
        }
      } else {
        TRACE(1, "ibrt_ui_log:power on %d fetch out",
              nvrecord_env->ibrt_mode.mode);
        app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
      }
      // startLED_status(1000);
      once_event_case = 9;
      startonce_delay_event_Timer_(1000);
      // startpwrkey_det(200);
    }
#elif defined(IS_MULTI_AI_ENABLED)
    // when ama and bisto switch, earphone need reconnect with peer, master need
    // reconnect with phone
    uint8_t box_action = app_ai_tws_reboot_get_box_action();
    if (box_action != 0xFF) {
      TRACE(2, "%s box_actionstate %d", __func__, box_action);
      app_ibrt_ui_event_entry(box_action | IBRT_SKIP_FALSE_TRIGGER_MASK);
    }
#endif
#else
    app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
#endif

    app_key_init();
    app_battery_start();
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
    app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BTIF_BLE_APP_DATAPATH_SERVER)
    BLE_custom_command_init();
#endif
#ifdef __THIRDPARTY
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_INIT);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_START);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                             THIRDPARTY_BT_CONNECTABLE);
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,
                                             THIRDPARTY_START);
#endif
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__)
#if !defined(IBRT)
    app_bt_profile_connect_manager_opening_reconnect();
#endif
#endif
  }
#ifdef __ENGINEER_MODE_SUPPORT__
  else if (pwron_case == APP_POWERON_CASE_TEST) {
    app_status_indication_init();
    app_factorymode_set(true);
    app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
#ifdef __WATCHER_DOG_RESET__
    app_wdt_close();
#endif
    TRACE(0, "!!!!!ENGINEER_MODE!!!!!\n");
    nRet = 0;
    app_nvrecord_rebuild();
    app_factorymode_key_init();
    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE) {
      hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
      app_factorymode_bt_signalingtest(NULL, NULL);
    }
    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE) {
      hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
      app_factorymode_bt_nosignalingtest(NULL, NULL);
    }
  }
#endif
  else {
    user_io_timer_init();
    app_status_indication_init();
    app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
    if (need_check_key) {
      pwron_case = app_poweron_wait_case();
    } else {
      pwron_case = APP_POWERON_CASE_NORMAL;
    }
    if (need_check_key) {
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
      app_poweron_wait_finished();
#endif
    }
    if (pwron_case != APP_POWERON_CASE_INVALID &&
        pwron_case != APP_POWERON_CASE_DITHERING) {
      TRACE(1, "power on case:%d\n", pwron_case);
      nRet = 0;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
      // app_status_indication_set(APP_STATUS_INDICATION_INITIAL);
#endif
      app_bt_start_custom_function_in_bt_thread(
          (uint32_t)1, 0, (uint32_t)btif_me_write_bt_sleep_enable);
      btdrv_set_lpo_times();

#ifdef IBRT_OTA
      bes_ota_init();
#endif

#ifdef __INTERCONNECTION__
      app_interconnection_init();
#endif

#ifdef __INTERACTION__
      app_interaction_init();
#endif

#if defined(__IAG_BLE_INCLUDE__) && defined(BTIF_BLE_APP_DATAPATH_SERVER)
      BLE_custom_command_init();
#endif
#ifdef GFPS_ENABLED
      app_gfps_set_battery_info_acquire_handler(app_tell_battery_info_handler);
      app_gfps_set_battery_datatype(SHOW_UI_INDICATION);
#endif
      osDelay(500);

      switch (pwron_case) {
      case APP_POWERON_CASE_CALIB:
        break;
      case APP_POWERON_CASE_BOTHSCAN:
        app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN, 0);
#endif
#if defined(__BTIF_EARPHONE__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
        if (false == is_charging_poweron)
          app_ibrt_enter_limited_mode();
#endif
#else
        app_bt_accessmode_set(BTIF_BT_DEFAULT_ACCESS_MODE_PAIR);
#endif
#ifdef GFPS_ENABLED
        app_enter_fastpairing_mode();
#endif
#if defined(__BTIF_AUTOPOWEROFF__)
        app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#endif
#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                                 THIRDPARTY_BT_DISCOVERABLE);
#endif
        break;
      case APP_POWERON_CASE_NORMAL:
#if defined(__BTIF_EARPHONE__) && !defined(__EARPHONE_STAY_BOTH_SCAN__)
#if defined(IBRT)
#ifdef IBRT_SEARCH_UI
        app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
        if (is_charging_poweron == false) {
          startLED_status(1000);
          once_event_case = 9;
          startonce_delay_event_Timer_(1000);
          if (IBRT_UNKNOW == nvrecord_env->ibrt_mode.mode) {
            TRACE(0, "ibrt_ui_log:power on unknow mode");
            app_ibrt_enter_limited_mode();
            if (app_tws_is_right_side()) {
              app_start_tws_serching_direactly();
            }
          } else {
            TRACE(1, "ibrt_ui_log:power on %d fetch out",
                  nvrecord_env->ibrt_mode.mode);
            app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
            //			app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
            // break;
          }
          // startpwrkey_det(200);
        }
#elif defined(IS_MULTI_AI_ENABLED)
        // when ama and bisto switch, earphone need reconnect with peer, master
        // need reconnect with phone
        // app_ibrt_ui_event_entry(IBRT_OPEN_BOX_EVENT);
        // TRACE(1,"ibrt_ui_log:power on %d fetch out",
        // nvrecord_env->ibrt_mode.mode);
        // app_ibrt_ui_event_entry(IBRT_FETCH_OUT_EVENT);
#endif
#else
        app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
#endif
#endif
      case APP_POWERON_CASE_REBOOT:
      case APP_POWERON_CASE_ALARM:
      default:
        // app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
#if defined(__BTIF_EARPHONE__) && defined(__BTIF_BT_RECONNECT__) &&            \
    !defined(IBRT)
        app_bt_profile_connect_manager_opening_reconnect();
#endif
#ifdef __THIRDPARTY
        app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO2,
                                                 THIRDPARTY_BT_CONNECTABLE);
#endif

        break;
      }

      app_key_init();
      app_battery_start();

#if defined(A2DP_LHDC_ON)
      lhdc_license_check();
#endif

#if defined(__BTIF_EARPHONE__) && defined(__BTIF_AUTOPOWEROFF__)
      app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
#ifdef __THIRDPARTY
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                               THIRDPARTY_INIT);
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                               THIRDPARTY_START);
      app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO3,
                                               THIRDPARTY_START);
#endif

#ifdef RB_CODEC
      rb_ctl_init();
#endif
    } else {
      af_close();
      app_key_close();
      nRet = -1;
    }
  }
exit:

#ifdef IS_MULTI_AI_ENABLED
  app_ai_tws_clear_reboot_box_state();
#endif

#ifdef __BT_DEBUG_TPORTS__
  {
    extern void bt_enable_tports(void);
    bt_enable_tports();
    // hal_iomux_tportopen();
  }
#endif

#ifdef ANC_APP
  app_anc_set_init_done();
#endif
#ifdef BT_USB_AUDIO_DUAL_MODE
  if (usb_plugin) {
    btusb_switch(BTUSB_MODE_USB);
  } else {
    btusb_switch(BTUSB_MODE_BT);
  }
#else // BT_USB_AUDIO_DUAL_MODE
#if defined(BTUSB_AUDIO_MODE)
  if (pwron_case == APP_POWERON_CASE_CHARGING) {
#ifdef __WATCHER_DOG_RESET__
    app_wdt_close();
#endif
    af_open();
    app_key_handle_clear();
    app_usb_key_init();
    app_usbaudio_entry();
  }

#endif // BTUSB_AUDIO_MODE
#endif // BT_USB_AUDIO_DUAL_MODE
  app_sysfreq_req(APP_SYSFREQ_USER_APP_INIT, APP_SYSFREQ_32K);

  return nRet;
}
