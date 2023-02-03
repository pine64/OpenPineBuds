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

#include "analog.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_status_ind.h"
#include "audioflinger.h"
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
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_bt_media_manager.h"
#include "app_fp_rfcomm.h"
#include "app_hfp.h"
#include "apps.h"
#include "besbt.h"
#include "bt_if.h"
#include "btapp.h"
#include "cqueue.h"
#include "hfp_api.h"
#include "os_api.h"
#include "resources.h"
#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#endif
#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif

#include "bt_drv_interface.h"

#ifdef __IAG_BLE_INCLUDE__
#include "app_ble_mode_switch.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif
#include "app.h"
#ifdef __AI_VOICE__
#include "ai_control.h"
#endif

#if defined(IBRT)
#include "app_ibrt_hf.h"
#include "app_ibrt_if.h"
#include "app_tws_ibrt_cmd_sync_hfp_status.h"
#include "besaud_api.h"
#endif

#include "bt_drv_reg_op.h"

#ifdef BISTO_ENABLED
#include "gsound_custom_bt.h"
#endif

#if defined(__BTMAP_ENABLE__)
#include "app_btmap_sms.h"
#endif

#ifdef __INTERCONNECTION__
#define HF_COMMAND_HUAWEI_BATTERY_HEAD "AT+HUAWEIBATTERY="
#define BATTERY_REPORT_NUM 2
#define BATTERY_REPORT_TYPE_BATTERY_LEVEL 1
#define BATTERY_REPORT_KEY_LEFT_BATTERY_LEVEL 2
#define BATTERY_REPORT_KEY_LEFT_CHARGE_STATE 3
char ATCommand[42];
const char *huawei_self_defined_command_response = "+HUAWEIBATTERY=OK";
#endif

/* hfp */
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);

int store_voicecvsd_buffer(unsigned char *buf, unsigned int len);
int store_voicemsbc_buffer(unsigned char *buf, unsigned int len);

void btapp_hfp_mic_need_skip_frame_set(int32_t skip_frame);
void a2dp_dual_slave_handling_refresh(void);
void a2dp_dual_slave_setup_during_sco(enum BT_DEVICE_ID_T currentId);

extern "C" bool bt_media_cur_is_bt_stream_media(void);
bool bt_is_sco_media_open();
extern bool app_audio_list_playback_exist(void);
#ifdef GFPS_ENABLED
extern "C" void app_exit_fastpairing_mode(void);
#endif

extern void app_bt_profile_connect_manager_hf(enum BT_DEVICE_ID_T id,
                                              hf_chan_handle_t Chan,
                                              struct hfp_context *ctx);
#ifdef __BT_ONE_BRING_TWO__
extern void hfcall_next_sta_handler(hf_event_t event);
#endif
#ifndef _SCO_BTPCM_CHANNEL_
struct hf_sendbuff_control hf_sendbuff_ctrl;
#endif
#ifdef __INTERACTION__
const char *oppo_self_defined_command_response = "+VDSF:7";
#endif
#if defined(SCO_LOOP)
#define HF_LOOP_CNT (20)
#define HF_LOOP_SIZE (360)

static uint8_t hf_loop_buffer[HF_LOOP_CNT * HF_LOOP_SIZE];
static uint32_t hf_loop_buffer_len[HF_LOOP_CNT];
static uint32_t hf_loop_buffer_valid = 1;
static uint32_t hf_loop_buffer_size = 0;
static char hf_loop_buffer_w_idx = 0;
#endif

static void app_hfp_set_starting_media_pending_flag(bool isEnabled,
                                                    uint8_t devId);
void app_hfp_resume_pending_voice_media(void);

static void app_hfp_mediaplay_delay_resume_timer_cb(void const *n) {
  app_hfp_resume_pending_voice_media();
}

#define HFP_MEDIAPLAY_DELAY_RESUME_IN_MS                                       \
  3000 // This time must be greater than SOUND_CONNECTED play time.
static void app_hfp_mediaplay_delay_resume_timer_cb(void const *n);
osTimerDef(APP_HFP_MEDIAPLAY_DELAY_RESUME_TIMER,
           app_hfp_mediaplay_delay_resume_timer_cb);
osTimerId app_hfp_mediaplay_delay_resume_timer_id = NULL;

#ifdef __IAG_BLE_INCLUDE__
static void app_hfp_resume_ble_adv(void);
#endif

#define HFP_AUDIO_CLOSED_DELAY_RESUME_ADV_IN_MS 1500
static void app_hfp_audio_closed_delay_resume_ble_adv_timer_cb(void const *n);
osTimerDef(APP_HFP_AUDIO_CLOSED_DELAY_RESUME_BLE_ADV_TIMER,
           app_hfp_audio_closed_delay_resume_ble_adv_timer_cb);
osTimerId app_hfp_audio_closed_delay_resume_ble_adv_timer_id = NULL;

extern struct BT_DEVICE_T app_bt_device;

#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
#ifdef __BT_ONE_BRING_TWO__
static uint8_t battery_level[BT_DEVICE_NUM] = {0xff, 0xff};
#else
static uint8_t battery_level[BT_DEVICE_NUM] = {0xff};
#endif

static uint8_t report_battery_level = 0xff;
void app_hfp_set_battery_level(uint8_t level) {
  osapi_lock_stack();
  if (report_battery_level == 0xff) {
    report_battery_level = level;
    osapi_notify_evm();
  }
  osapi_unlock_stack();
}

int app_hfp_battery_report_reset(uint8_t bt_device_id) {
  ASSERT(bt_device_id < BT_DEVICE_NUM, "bt_device_id error");
  battery_level[bt_device_id] = 0xff;
  return 0;
}

#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
#define HF_COMMAND_BATTERY_HEAD "AT+VDBTY="
#define HF_COMMAND_VERSION_HEAD "AT+VDRV="
#define HF_COMMAND_FEATURE_HEAD "AT+VDSF="
#define REPORT_NUM 3
#define LEFT_UNIT_REPORT 1
#define RIGHT_UNIT_REPORT 2
#define BOX_REPORT 3
char ATCommand[42];

bt_status_t Send_customer_battery_report_AT_command(hf_chan_handle_t chan_h,
                                                    uint8_t level) {
  TRACE(0, "Send battery report at commnad.");
  /// head and keyNumber
  sprintf(ATCommand, "%s%d", HF_COMMAND_BATTERY_HEAD, REPORT_NUM);
  /// keys and corresponding values
  sprintf(ATCommand, "%s,%d,%d,%d,%d,%d,%d\r", ATCommand, LEFT_UNIT_REPORT,
          level, RIGHT_UNIT_REPORT, level, BOX_REPORT, 9);
  /// send AT command
  return btif_hf_send_at_cmd(chan_h, ATCommand);
}

bt_status_t
Send_customer_phone_feature_support_AT_command(hf_chan_handle_t chan_h,
                                               uint8_t val) {
  TRACE(0, "Send_customer_phone_feature_support_AT_command.");
  /// keys and corresponding values
  sprintf(ATCommand, "%s%d\r", HF_COMMAND_FEATURE_HEAD, val);
  /// send AT command
  return btif_hf_send_at_cmd(chan_h, ATCommand);
}

bt_status_t Send_customer_version_report_AT_command(hf_chan_handle_t chan_h) {
  TRACE(0, "Send version report at commnad.");
  /// head and keyNumber
  sprintf(ATCommand, "%s%d", HF_COMMAND_VERSION_HEAD, REPORT_NUM);
  /// keys and corresponding values
  sprintf(ATCommand, "%s,%d,%d,%d,%d,%d,%d\r", ATCommand, LEFT_UNIT_REPORT,
          0x1111, RIGHT_UNIT_REPORT, 0x2222, BOX_REPORT, 0x3333);
  /// send AT command
  return btif_hf_send_at_cmd(chan_h, ATCommand);
}
#endif

#ifdef __INTERCONNECTION__
uint8_t ask_is_selfdefined_battery_report_AT_command_support(void) {
  TRACE(0, "ask if mobile support self-defined at commnad.");
  uint8_t *pSelfDefinedCommandSupport =
      app_battery_get_mobile_support_self_defined_command_p();
  *pSelfDefinedCommandSupport = 0;

  sprintf(ATCommand, "%s?", HF_COMMAND_HUAWEI_BATTERY_HEAD);
  btif_hf_send_at_cmd(
      (hf_chan_handle_t)app_bt_device.hf_channel[BT_DEVICE_ID_1], ATCommand);

  return 0;
}

uint8_t send_selfdefined_battery_report_AT_command(void) {
  uint8_t *pSelfDefinedCommandSupport =
      app_battery_get_mobile_support_self_defined_command_p();
  uint8_t batteryInfo = 0;

  if (*pSelfDefinedCommandSupport) {
    app_battery_get_info(NULL, &batteryInfo, NULL);

    /// head and keyNumber
    sprintf(ATCommand, "%s%d", HF_COMMAND_HUAWEI_BATTERY_HEAD,
            BATTERY_REPORT_NUM);

    /// keys and corresponding values
    sprintf(ATCommand, "%s,%d,%d,%d,%d", ATCommand,
            BATTERY_REPORT_KEY_LEFT_BATTERY_LEVEL, batteryInfo & 0x7f,
            BATTERY_REPORT_KEY_LEFT_CHARGE_STATE, (batteryInfo & 0x80) ? 1 : 0);

    /// send AT command
    btif_hf_send_at_cmd(
        (hf_chan_handle_t)app_bt_device.hf_channel[BT_DEVICE_ID_1], ATCommand);
  }
  return 0;
}
#endif

int app_hfp_battery_report(uint8_t level) {
  // Care: BT_DEVICE_NUM<-->{0xff, 0xff, ...}
  bt_status_t status = BT_STS_LAST_CODE;
  hf_chan_handle_t chan;

  uint8_t i;
  int nRet = 0;

  if (level > 9)
    return -1;

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    chan = app_bt_device.hf_channel[i];
    if (btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN) {
      if (btif_hf_is_hf_indicators_support(chan)) {
        if (battery_level[i] != level) {
          uint8_t assigned_num = 2; /// battery level assigned num:2
          status =
              btif_hf_update_indicators_batt_level(chan, assigned_num, level);
        }
      } else if (btif_hf_is_batt_report_support(chan)) {
        if (battery_level[i] != level) {
#ifdef GFPS_ENABLED
          app_fp_msg_send_battery_levels();
#endif
#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
          status = Send_customer_battery_report_AT_command(chan, level);
#endif
          status = btif_hf_batt_report(chan, level);
        }
      }
      if (BT_STS_PENDING == status) {
        battery_level[i] = level;
      } else {
        nRet = -1;
      }
    } else {
      battery_level[i] = 0xff;
      nRet = -1;
    }
  }
  return nRet;
}

void app_hfp_battery_report_proc(void) {
  osapi_lock_stack();

  if (report_battery_level != 0xff) {
    app_hfp_battery_report(report_battery_level);
    report_battery_level = 0xff;
  }
  osapi_unlock_stack();
}

bt_status_t app_hfp_send_at_command(const char *cmd) {
  bt_status_t ret = 0;
  // send AT command
  ret = btif_hf_send_at_cmd(
      (hf_chan_handle_t)app_bt_device.hf_channel[BT_DEVICE_ID_1], cmd);

  return ret;
}

#endif

void a2dp_get_curStream_remDev(btif_remote_device_t **p_remDev);

bool app_hfp_curr_audio_up(hf_chan_handle_t hfp_chnl) {
  int i = 0;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (app_bt_device.hf_channel[i] == hfp_chnl) {
      return app_bt_device.hf_conn_flag[i] &&
             app_bt_device.hf_audio_state[i] == BTIF_HF_AUDIO_CON;
    }
  }
  return false;
}

uint8_t app_hfp_get_chnl_via_remDev(hf_chan_handle_t *p_hfp_chnl) {
  uint8_t i = 0;

#ifdef __BT_ONE_BRING_TWO__
  btif_remote_device_t *p_a2dp_remDev;
  btif_remote_device_t *p_hfp_remDev;

  a2dp_get_curStream_remDev(&p_a2dp_remDev);
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    p_hfp_remDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
        app_bt_device.hf_channel[i]);
    if (p_hfp_remDev == p_a2dp_remDev)
      break;
  }
  if (i != BT_DEVICE_NUM)
    *p_hfp_chnl = app_bt_device.hf_channel[i];
#else
  i = BT_DEVICE_ID_1;
  *p_hfp_chnl = app_bt_device.hf_channel[i];
#endif
  return i;
}

#ifdef SUPPORT_SIRI
int app_hfp_siri_report() {
  uint8_t i;
  bt_status_t status = BT_STS_LAST_CODE;
  hf_chan_handle_t chan;

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    chan = app_bt_device.hf_channel[i];
    if (btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN) {
      status = btif_hf_siri_report(chan);
      if (status == BT_STS_PENDING) {
        return 0;
      } else {
        return -1;
      }
    }
  }
  return 0;
}

extern int open_siri_flag;
int app_hfp_siri_voice(bool en) {
  static enum BT_DEVICE_ID_T hf_id = BT_DEVICE_ID_1;
  bt_status_t res = BT_STS_LAST_CODE;

  hf_chan_handle_t POSSIBLY_UNUSED hf_siri_chnl = NULL;
  if (open_siri_flag == 1) {
    if (btif_hf_is_voice_rec_active(app_bt_device.hf_channel[hf_id]) == false) {
      open_siri_flag = 0;
      TRACE(0, "end auto");
    } else {
      TRACE(0, "need close");
      en = false;
    }
  }
  if (open_siri_flag == 0) {
    if (btif_hf_is_voice_rec_active(app_bt_device.hf_channel[BT_DEVICE_ID_1]) ==
        true) {
      TRACE(0, "1 ->close");
      hf_id = BT_DEVICE_ID_1;
      en = false;
      hf_siri_chnl = app_bt_device.hf_channel[BT_DEVICE_ID_1];
    }
#ifdef __BT_ONE_BRING_TWO__
    else if (btif_hf_is_voice_rec_active(
                 app_bt_device.hf_channel[BT_DEVICE_ID_2]) == true) {
      TRACE(0, "2->close");
      hf_id = BT_DEVICE_ID_2;
      en = false;
      hf_siri_chnl = app_bt_device.hf_channel[BT_DEVICE_ID_2];
    }
#endif
    else {
      open_siri_flag = 1;
      en = true;
#ifdef __BT_ONE_BRING_TWO__
      hf_id = (enum BT_DEVICE_ID_T)app_hfp_get_chnl_via_remDev(&hf_siri_chnl);
#else
      hf_id = BT_DEVICE_ID_1;
#endif
      TRACE(1, "a2dp id = %d", hf_id);
    }
  }
  TRACE(4, "[%s]id =%d/%d/%d", __func__, hf_id, open_siri_flag, en);
  if (hf_id == BT_DEVICE_NUM)
    hf_id = BT_DEVICE_ID_1;

  if ((btif_get_hf_chan_state(app_bt_device.hf_channel[hf_id]) ==
       BTIF_HF_STATE_OPEN)) {
    res = btif_hf_enable_voice_recognition(app_bt_device.hf_channel[hf_id], en);
  }

  TRACE(3, "[%s] Line =%d, res = %d", __func__, __LINE__, res);

  return 0;
}
#endif

#define _THREE_WAY_ONE_CALL_COUNT__ 1
#ifdef _THREE_WAY_ONE_CALL_COUNT__
static enum BT_DEVICE_ID_T hfp_cur_call_chnl = BT_DEVICE_NUM;
static int8_t cur_chnl_call_on_active[BT_DEVICE_NUM] = {0};
int app_bt_get_audio_up_id(void);

void app_hfp_3_way_call_counter_set(enum BT_DEVICE_ID_T id, uint8_t set) {
  if (set > 0) {
    cur_chnl_call_on_active[id]++;
    if (cur_chnl_call_on_active[id] > BT_DEVICE_NUM)
      cur_chnl_call_on_active[id] = 2;
  } else {
    cur_chnl_call_on_active[id]--;
    if (cur_chnl_call_on_active[id] < 0)
      cur_chnl_call_on_active[id] = 0;
    if (app_bt_device.hfchan_call[id] == 1)
      cur_chnl_call_on_active[id] = 1;
  }
  TRACE(1, "call_on_active = %d", cur_chnl_call_on_active[id]);
}

void app_hfp_set_cur_chnl_id(uint8_t id) {
  if (hfp_cur_call_chnl == BT_DEVICE_NUM) {
    hfp_cur_call_chnl = (enum BT_DEVICE_ID_T)id;
    cur_chnl_call_on_active[id] = 1;
  }
  TRACE(3, "%s    hfp_cur_call_chnl = %d    id=%d", __func__, hfp_cur_call_chnl,
        id);
}

uint8_t app_hfp_get_cur_call_chnl(void **chnl) {
  TRACE(2, "%s    hfp_cur_call_chnl = %d", __func__, hfp_cur_call_chnl);
  if (hfp_cur_call_chnl != BT_DEVICE_NUM) {
    return hfp_cur_call_chnl;
  }
  return BT_DEVICE_NUM;
}

void app_hfp_clear_cur_call_chnl(enum BT_DEVICE_ID_T id) {
  hfp_cur_call_chnl = BT_DEVICE_NUM;
  cur_chnl_call_on_active[id] = 0;
  TRACE(2, "%s    id= %d", __func__, id);
}

void app_hfp_cur_call_chnl_reset(enum BT_DEVICE_ID_T id) {
  if (id == hfp_cur_call_chnl)
    hfp_cur_call_chnl = BT_DEVICE_NUM;
  TRACE(3, "%s hfp_cur_call_chnl = %d    id=%d", __func__, hfp_cur_call_chnl,
        id);
}

bool app_hfp_cur_chnl_is_on_3_way_calling(void) {
  uint8_t i = 0;
  TRACE(1, "hfp_cur_call_chnl = %d", hfp_cur_call_chnl);
#ifdef __BT_ONE_BRING_TWO__
  TRACE(2, "cur_chnl_call_on_active[0] = %d  [1] = %d",
        cur_chnl_call_on_active[0], cur_chnl_call_on_active[1]);
#else
  TRACE(1, "cur_chnl_call_on_active[0] = %d", cur_chnl_call_on_active[0]);
#endif
  if (hfp_cur_call_chnl == BT_DEVICE_NUM)
    return false;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (cur_chnl_call_on_active[i] > 1) {
      break;
    }
  }
  if (i == BT_DEVICE_NUM)
    return false;
  return true;
}
#endif
#if defined(__BTIF_EARPHONE__)
static void hfp_app_status_indication(enum BT_DEVICE_ID_T chan_id,
                                      struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T chan_id_other =
      (chan_id == BT_DEVICE_ID_1) ? (BT_DEVICE_ID_2) : (BT_DEVICE_ID_1);
#else
  enum BT_DEVICE_ID_T chan_id_other = BT_DEVICE_ID_1;
#endif
  switch (ctx->event) {
  /*
  case HF_EVENT_SERVICE_CONNECTED:
      break;
  case HF_EVENT_SERVICE_DISCONNECTED:
      break;
      */
  case BTIF_HF_EVENT_CURRENT_CALL_STATE:
    TRACE(2, "!!!HF_EVENT_CURRENT_CALL_STATE  chan_id:%d, call_number:%s\n",
          chan_id, ctx->call_number);
    if (app_bt_device.hfchan_callSetup[chan_id] == BTIF_HF_CALL_SETUP_IN) {
      //////report incoming call number
      // app_status_set_num(ctx->call_number);
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
      app_voice_report(APP_STATUS_RING_WARNING, chan_id);
#endif
    }
    break;
  case BTIF_HF_EVENT_CALL_IND:
    if (ctx->call == BTIF_HF_CALL_NONE &&
        app_bt_device.hfchan_call[chan_id] == BTIF_HF_CALL_ACTIVE) {
      //////report call hangup voice
      TRACE(1,
            "!!!HF_EVENT_CALL_IND  APP_STATUS_INDICATION_HANGUPCALL  "
            "chan_id:%d\n",
            chan_id);
      app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,
                                    BT_STREAM_VOICE, chan_id, 0);
      /// disable media prompt
      if (app_bt_device.hf_endcall_dis[chan_id] == false) {
        TRACE(0, "HANGUPCALL PROMPT");
        // app_voice_report(APP_STATUS_INDICATION_HANGUPCALL,chan_id);
      }
#if defined(_THREE_WAY_ONE_CALL_COUNT__)
      if (app_hfp_get_cur_call_chnl(NULL) == chan_id) {
        app_hfp_clear_cur_call_chnl(chan_id);
      }
#endif
    }
#if defined(_THREE_WAY_ONE_CALL_COUNT__)
    else if ((ctx->call == BTIF_HF_CALL_ACTIVE) &&
             (app_bt_get_audio_up_id() == chan_id)) {
      app_hfp_set_cur_chnl_id(chan_id);
    }
#endif
    break;
  case BTIF_HF_EVENT_CALLSETUP_IND:
    if (ctx->call_setup == BTIF_HF_CALL_SETUP_NONE &&
        (app_bt_device.hfchan_call[chan_id] != BTIF_HF_CALL_ACTIVE) &&
        (app_bt_device.hfchan_callSetup[chan_id] != BTIF_HF_CALL_SETUP_NONE)) {
      ////check the call refuse and stop media of (ring and call number)
      TRACE(1,
            "!!!HF_EVENT_CALLSETUP_IND  APP_STATUS_INDICATION_REFUSECALL  "
            "chan_id:%d\n",
            chan_id);
#if 0 // def __BT_ONE_BRING_TWO__
                if (app_bt_device.hf_audio_state[chan_id_other] == BTIF_HF_AUDIO_DISCON){
                    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,BT_STREAM_VOICE,chan_id,0);
                    app_voice_report(APP_STATUS_INDICATION_REFUSECALL,chan_id);
                }
#else
      app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,
                                    BT_STREAM_VOICE, chan_id, 0);
#if !defined(IBRT) && defined(MEDIA_PLAYER_SUPPORT)
      app_voice_report(APP_STATUS_INDICATION_REFUSECALL,
                       chan_id); /////////////du����
#endif
#endif
      if ((app_bt_device.hfchan_call[chan_id_other] == BTIF_HF_CALL_ACTIVE) &&
          (app_bt_device.hf_audio_state[chan_id_other] == BTIF_HF_AUDIO_CON)) {
        app_bt_device.curr_hf_channel_id = chan_id_other;
      }
    } else if (ctx->call_setup == BTIF_HF_CALL_SETUP_NONE &&
               (app_bt_device.hfchan_callSetup[chan_id] !=
                BTIF_HF_CALL_SETUP_NONE) &&
               (app_bt_device.hfchan_call[chan_id] == BTIF_HF_CALL_ACTIVE)) {
      TRACE(1,
            "!!!HF_EVENT_CALLSETUP_IND  APP_STATUS_INDICATION_ANSWERCALL but "
            "noneed sco chan_id:%d\n",
            chan_id);
#ifdef _THREE_WAY_ONE_CALL_COUNT__
      if (app_bt_device.hf_callheld[chan_id] == BTIF_HF_CALL_HELD_NONE) {
        if (app_hfp_get_cur_call_chnl(NULL) == chan_id) {
          app_hfp_3_way_call_counter_set(chan_id, 0);
        }
      }
#endif /* _THREE_WAY_ONE_CALL_COUNT__ */

#ifdef MEDIA_PLAYER_SUPPORT
      app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP_MEDIA,
                                    BT_STREAM_MEDIA, chan_id, 0);
#endif
    }
#if defined(_THREE_WAY_ONE_CALL_COUNT__)
    else if ((ctx->call_setup != BTIF_HF_CALL_SETUP_NONE) &&
             (app_bt_device.hfchan_call[chan_id] == BTIF_HF_CALL_ACTIVE)) {
      if (app_hfp_get_cur_call_chnl(NULL) == chan_id) {
        app_hfp_3_way_call_counter_set(chan_id, 1);
      }
    }
#endif
    break;
    /*
case HF_EVENT_AUDIO_CONNECTED:
    TRACE(1,"!!!HF_EVENT_AUDIO_CONNECTED  APP_STATUS_INDICATION_ANSWERCALL
chan_id:%d\n",chan_id);
  //
app_voice_report(APP_STATUS_INDICATION_ANSWERCALL,chan_id);//////////////duһ��
    break;
    */
  case BTIF_HF_EVENT_RING_IND:
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL, chan_id);
#endif
    break;
  default:
    break;
  }
}
#endif

struct BT_DEVICE_ID_DIFF chan_id_flag;
#ifdef __BT_ONE_BRING_TWO__
void hfp_chan_id_distinguish(hf_chan_handle_t chan) {
  if (chan == app_bt_device.hf_channel[BT_DEVICE_ID_1]) {
    chan_id_flag.id = BT_DEVICE_ID_1;
  } else if (chan == app_bt_device.hf_channel[BT_DEVICE_ID_2]) {
    chan_id_flag.id = BT_DEVICE_ID_2;
  }
}
#endif

int hfp_volume_get(enum BT_DEVICE_ID_T id) {
  int vol = TGT_VOLUME_LEVEL_15;

  nvrec_btdevicerecord *record = NULL;
  bt_bdaddr_t bdAdd;
  if (btif_hf_get_remote_bdaddr(app_bt_device.hf_channel[id], &bdAdd) &&
      !nv_record_btdevicerecord_find(&bdAdd, &record)) {
    vol = record->device_vol.hfp_vol - 2;
  } else if (app_audio_manager_hfp_is_active(id)) {
    vol = app_bt_stream_hfpvolume_get() - 2;
  } else {
    vol = TGT_VOLUME_LEVEL_15;
  }

  if (vol > 15)
    vol = 15;
  if (vol < 0)
    vol = 0;

#ifndef BES_AUTOMATE_TEST
  TRACE(2, "hfp get vol raw:%d loc:%d", vol, vol + 2);
#endif
  return (vol);
}

void hfp_volume_local_set(enum BT_DEVICE_ID_T id, int8_t vol) {
  nvrec_btdevicerecord *record = NULL;
  bt_bdaddr_t bdAdd;
  if (btif_hf_get_remote_bdaddr(app_bt_device.hf_channel[id], &bdAdd)) {
    if (!nv_record_btdevicerecord_find(&bdAdd, &record)) {
      nv_record_btdevicerecord_set_hfp_vol(record, vol);
    }
  }

  if (app_bt_stream_volume_get_ptr()->hfp_vol != vol) {
#if defined(NEW_NV_RECORD_ENABLED)
    nv_record_btdevicevolume_set_hfp_vol(app_bt_stream_volume_get_ptr(), vol);
#endif
    nv_record_touch_cause_flush();
  }
}

int hfp_volume_set(enum BT_DEVICE_ID_T id, int vol) {
  if (vol > 15)
    vol = 15;
  if (vol < 0)
    vol = 0;

  hfp_volume_local_set(id, vol + 2);
  if (app_audio_manager_hfp_is_active(id)) {
    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET, vol + 2);
  }

  TRACE(2, "hfp put vol raw:%d loc:%d", vol, vol + 2);
  return 0;
}

static uint8_t call_setup_running_on = 0;
void hfp_call_setup_running_on_set(uint8_t set) { call_setup_running_on = set; }

void hfp_call_setup_running_on_clr(void) { call_setup_running_on = 0; }

uint8_t hfp_get_call_setup_running_on_state(void) {
  TRACE(2, "%s state = %d", __func__, call_setup_running_on);
  return call_setup_running_on;
}

static void hfp_connected_ind_handler(hf_chan_handle_t chan,
                                      struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

#ifdef GFPS_ENABLED
  app_exit_fastpairing_mode();
#endif

  app_bt_clear_connecting_profiles_state(chan_id_flag.id);

  TRACE(1, "::HF_EVENT_SERVICE_CONNECTED  Chan_id:%d\n", chan_id_flag.id);
  app_bt_device.phone_earphone_mark = 1;

#if defined(__BTIF_EARPHONE__)
  if (ctx->state == BTIF_HF_STATE_OPEN) {
    ////report connected voice
    app_bt_device.hf_conn_flag[chan_id_flag.id] = 1;
  }
#endif
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
  uint8_t battery_level;
  app_hfp_battery_report_reset(chan_id_flag.id);
  app_battery_get_info(NULL, &battery_level, NULL);
  app_hfp_set_battery_level(battery_level);
#endif
  //        app_bt_stream_hfpvolume_reset();
  btif_hf_report_speaker_volume(chan, hfp_volume_get(chan_id_flag.id));

#if defined(HFP_DISABLE_NREC)
  btif_hf_disable_nrec(chan);
#endif

#ifdef __BT_ONE_BRING_TWO__
  ////if a call is active and start bt open reconnect procedure, process the
  /// curr_hf_channel_id
  if ((app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) ||
      (app_bt_device.hfchan_callSetup[anotherDevice] ==
       BTIF_HF_CALL_SETUP_IN)) {
    app_bt_device.curr_hf_channel_id = anotherDevice;
  } else {
    app_bt_device.curr_hf_channel_id = chan_id_flag.id;
  }
#endif

#if defined(__BTMAP_ENABLE__) && defined(BTIF_DIP_DEVICE)
  if ((btif_dip_get_process_status(app_bt_get_remoteDev(chan_id_flag.id))) &&
      (app_btmap_check_is_idle(chan_id_flag.id))) {
    app_btmap_sms_open(chan_id_flag.id, &ctx->remote_dev_bdaddr);
  }
#endif

  app_bt_profile_connect_manager_hf(chan_id_flag.id, chan, ctx);
#ifdef __INTERCONNECTION__
  ask_is_selfdefined_battery_report_AT_command_support();
#endif
#ifdef __INTERACTION_CUSTOMER_AT_COMMAND__
  Send_customer_phone_feature_support_AT_command(chan, 7);
  Send_customer_battery_report_AT_command(chan, battery_level);
#endif
}

static void hfp_disconnected_ind_handler(hf_chan_handle_t chan,
                                         struct hfp_context *ctx) {
  TRACE(2, "::HF_EVENT_SERVICE_DISCONNECTED Chan_id:%d, reason=%x\n",
        chan_id_flag.id, ctx->disc_reason);
#if defined(HFP_1_6_ENABLE)
  btif_hf_set_negotiated_codec(chan, BTIF_HF_SCO_CODEC_CVSD);
#endif
#if defined(__BTIF_EARPHONE__)
  if (app_bt_device.hf_conn_flag[chan_id_flag.id]) {
    ////report device disconnected voice
    app_bt_device.hf_conn_flag[chan_id_flag.id] = 0;
  }
#endif
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                chan_id_flag.id, MAX_RECORD_NUM);
  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    if (chan == app_bt_device.hf_channel[i]) {
      app_bt_device.hfchan_call[i] = 0;
      app_bt_device.hfchan_callSetup[i] = 0;
      app_bt_device.hf_audio_state[i] = BTIF_HF_AUDIO_DISCON;
      app_bt_device.hf_conn_flag[i] = 0;
      app_bt_device.hf_voice_en[i] = 0;
    }
  }

  app_bt_profile_connect_manager_hf(chan_id_flag.id, chan, ctx);
}
static void hfp_audio_data_sent_handler(hf_chan_handle_t chan,
                                        struct hfp_context *ctx) {
#if defined(SCO_LOOP)
  hf_loop_buffer_valid = 1;
#endif
}

static void hfp_audio_data_handler(hf_chan_handle_t chan,
                                   struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  if (app_bt_device.hf_voice_en[chan_id_flag.id]) {
#endif

#ifndef _SCO_BTPCM_CHANNEL_
    uint32_t idx = 0;
    if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
      store_voicebtpcm_m2p_buffer(ctx->audio_data, ctx->audio_data_len);

      idx = hf_sendbuff_ctrl.index % HF_SENDBUFF_MEMPOOL_NUM;
      get_voicebtpcm_p2m_frame(&(hf_sendbuff_ctrl.mempool[idx].buffer[0]),
                               ctx->audio_data_len);
      hf_sendbuff_ctrl.mempool[idx].packet.data =
          &(hf_sendbuff_ctrl.mempool[idx].buffer[0]);
      hf_sendbuff_ctrl.mempool[idx].packet.dataLen = ctx->audio_data_len;
      hf_sendbuff_ctrl.mempool[idx].packet.flags = BTIF_BTP_FLAG_NONE;
      if (!app_bt_device.hf_mute_flag) {
        btif_hf_send_audio_data(chan, &hf_sendbuff_ctrl.mempool[idx].packet);
      }
      hf_sendbuff_ctrl.index++;
    }
#endif

#ifdef __BT_ONE_BRING_TWO__
  }
#endif

#if defined(SCO_LOOP)
  memcpy(hf_loop_buffer + hf_loop_buffer_w_idx * HF_LOOP_SIZE,
         Info->p.audioData->data, Info->p.audioData->len);
  hf_loop_buffer_len[hf_loop_buffer_w_idx] = Info->p.audioData->len;
  hf_loop_buffer_w_idx = (hf_loop_buffer_w_idx + 1) % HF_LOOP_CNT;
  ++hf_loop_buffer_size;

  if (hf_loop_buffer_size >= 18 && hf_loop_buffer_valid == 1) {
    hf_loop_buffer_valid = 0;
    idx = hf_loop_buffer_w_idx - 17 < 0
              ? (HF_LOOP_CNT - (17 - hf_loop_buffer_w_idx))
              : hf_loop_buffer_w_idx - 17;
    pkt.flags = BTP_FLAG_NONE;
    pkt.dataLen = hf_loop_buffer_len[idx];
    pkt.data = hf_loop_buffer + idx * HF_LOOP_SIZE;
    HF_SendAudioData(Chan, &pkt);
  }
#endif
}

static void hfp_call_ind_handler(hf_chan_handle_t chan,
                                 struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

#ifdef __BT_ONE_BRING_TWO__
  TRACE(8,
        "::HF_EVENT_CALL_IND %d chan_id %dx call %d %d held %d %d audio_state "
        "%d %d\n",
        ctx->call, chan_id_flag.id, app_bt_device.hfchan_call[BT_DEVICE_ID_1],
        app_bt_device.hfchan_call[BT_DEVICE_ID_2],
        app_bt_device.hf_callheld[BT_DEVICE_ID_1],
        app_bt_device.hf_callheld[BT_DEVICE_ID_2],
        app_bt_device.hf_audio_state[BT_DEVICE_ID_1],
        app_bt_device.hf_audio_state[BT_DEVICE_ID_2]);
#else
  TRACE(2, "::HF_EVENT_CALL_IND  chan_id:%d, call:%d\n", chan_id_flag.id,
        ctx->call);
#endif
  if (ctx->call == BTIF_HF_CALL_NONE) {
    hfp_call_setup_running_on_clr();
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("Call hangup ok.");
#endif
    app_bt_device.hf_callheld[chan_id_flag.id] = BTIF_HF_CALL_HELD_NONE;
  } else if (ctx->call == BTIF_HF_CALL_ACTIVE) {
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("Call setup ok.");
#endif

    /// call is active so check if it's a outgoing call
    if (app_bt_device.hfchan_callSetup[chan_id_flag.id] ==
        BTIF_HF_CALL_SETUP_ALERT) {
      TRACE(1, "HF CALLACTIVE TIME=%d", hal_sys_timer_get());
      if (TICKS_TO_MS(hal_sys_timer_get() -
                      app_bt_device.hf_callsetup_time[chan_id_flag.id]) <
          1000) {
        TRACE(0, "DISABLE HANGUPCALL PROMPT");
        app_bt_device.hf_endcall_dis[chan_id_flag.id] = true;
      }
    }
    /////stop media of (ring and call number) and switch to sco
#if defined(HFP_NO_PRERMPT)
    if (app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) {
    } else
#endif
    {
#ifdef __BT_ONE_BRING_TWO__
      TRACE(1, "%s another %d,hf_callheld %d", __func__, anotherDevice,
            app_bt_device.hf_callheld[anotherDevice]);
      if ((btapp_hfp_get_call_state()) &&
          (app_bt_device.hf_callheld[anotherDevice] == BTIF_HF_CALL_HELD_NONE))
#else
      if (btapp_hfp_get_call_state())
#endif
      {
        TRACE(0, "DON'T SIWTCH_TO_SCO");
        app_bt_device.hfchan_call[chan_id_flag.id] = ctx->call;
        return;
      } else {
#ifndef ENABLE_HFP_AUDIO_PENDING_FOR_MEDIA
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWITCHTO_SCO,
                                      BT_STREAM_VOICE, chan_id_flag.id, 0);
#endif
      }
    }
  } else {
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("Call hangup ok.");
#endif
  }

#if defined(__BTIF_EARPHONE__)
  hfp_app_status_indication(chan_id_flag.id, ctx);
#endif

  if (ctx->call == BTIF_HF_CALL_ACTIVE) {
#if defined(HFP_NO_PRERMPT)
#else
    app_bt_device.curr_hf_channel_id = chan_id_flag.id;
#endif
  }
#ifdef __BT_ONE_BRING_TWO__
  else if ((ctx->call == BTIF_HF_CALL_NONE) &&
           ((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) ||
            (app_bt_device.hfchan_callSetup[anotherDevice] ==
             BTIF_HF_CALL_SETUP_IN) ||
            (app_bt_device.hfchan_callSetup[anotherDevice] ==
             BTIF_HF_CALL_SETUP_OUT) ||
            (app_bt_device.hfchan_callSetup[anotherDevice] ==
             BTIF_HF_CALL_SETUP_ALERT))) {
    app_bt_device.curr_hf_channel_id = anotherDevice;
  }
#endif
  TRACE(1, "!!!HF_EVENT_CALL_IND  curr_hf_channel_id:%d\n",
        app_bt_device.curr_hf_channel_id);
  app_bt_device.hfchan_call[chan_id_flag.id] = ctx->call;
  if (ctx->call == BTIF_HF_CALL_NONE) {
    app_bt_device.hf_endcall_dis[chan_id_flag.id] = false;
  }
#if defined(__BT_ONE_BRING_TWO__)
  if (bt_get_sco_number() > 1
#ifdef CHIP_BEST1000
      && hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2
#endif
  ) {
    ////a call is active:
    if (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_ACTIVE) {
#if !defined(HFP_NO_PRERMPT)
      if (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) {
        app_bt_device.curr_hf_channel_id = chan_id_flag.id;
#ifdef __HF_KEEP_ONE_ALIVE__
#ifdef ENABLE_HFP_AUDIO_PENDING_FOR_MEDIA
        if (bt_media_cur_is_bt_stream_media()) {
          app_hfp_set_starting_media_pending_flag(true, chan_id_flag.id);
        } else
#endif
        {
          app_hfp_start_voice_media(chan_id_flag.id);
        }
#else
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWAP_SCO,
                                      BT_STREAM_SBC, chan_id_flag.id, 0);
#endif
      }
      app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
      app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_DISABLE;
#endif
    } else {
      ////a call is hung up:
      /// if one device  setup a sco connect so get the other device's sco
      /// state, if both connect mute the earlier one
      if (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) {
        app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_ENABLE;
        app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
      }
    }
  }
#endif
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_UPDATE_MEDIA,
                                BT_STREAM_VOICE, chan_id_flag.id,
                                MAX_RECORD_NUM);
}

extern uint8_t once_event_case;
extern void startonce_delay_event_Timer_(int ms);
extern void bt_drv_clear_skip_flag();
static void hfp_callsetup_ind_handler(hf_chan_handle_t chan,
                                      struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  // clear flag_skip_resv and flag_skip_retx
  bt_drv_clear_skip_flag();
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

  TRACE(2, "::HF_EVENT_CALLSETUP_IND chan_id:%d, callSetup =%d\n",
        chan_id_flag.id, ctx->call_setup);
  if (ctx->call_setup == 0x03) {
    once_event_case = 8;
    startonce_delay_event_Timer_(1000);
  }
  if ((ctx->call_setup & 0x03) != 0) {
    hfp_call_setup_running_on_set(1);
  }
#if defined(__BTIF_EARPHONE__)
  hfp_app_status_indication(chan_id_flag.id, ctx);
#endif

  if (BTIF_HF_CALL_SETUP_NONE != ctx->call_setup) {
    // exit sniff mode and stay active
    app_bt_active_mode_set(ACTIVE_MODE_KEEPEER_SCO_STREAMING,
                           UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
  } else {
    // resume sniff mode
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPEER_SCO_STREAMING,
                             UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
  }

#ifdef __BT_ONE_BRING_TWO__
  TRACE(2, "call [0]/[1] =%d / %d", app_bt_device.hfchan_call[BT_DEVICE_ID_1],
        app_bt_device.hfchan_call[BT_DEVICE_ID_2]);
  TRACE(2, "audio [0]/[1] =%d / %d",
        app_bt_device.hf_audio_state[BT_DEVICE_ID_1],
        app_bt_device.hf_audio_state[BT_DEVICE_ID_2]);

  app_bt_device.callSetupBitRec |= (1 << ctx->call_setup);
  if (ctx->call_setup == 0) {
    // do nothing
  } else {

#ifdef BT_USB_AUDIO_DUAL_MODE
    if (!btusb_is_bt_mode()) {
      TRACE(0, "btusb_usbaudio_close doing.");
      btusb_usbaudio_close();
    }
#endif
#if defined(HFP_NO_PRERMPT)
    if (((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) &&
         (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_NONE)) ||
        ((app_bt_device.hfchan_callSetup[anotherDevice] ==
          BTIF_HF_CALL_SETUP_IN) &&
         (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_NONE))) {
      app_bt_device.curr_hf_channel_id = anotherDevice;
    } else if ((app_bt_device.hfchan_call[chan_id_flag.id] ==
                BTIF_HF_CALL_ACTIVE) &&
               (app_bt_device.hfchan_call[anotherDevice] ==
                BTIF_HF_CALL_ACTIVE)) {
    } else {
      app_bt_device.curr_hf_channel_id = chan_id_flag.id;
    }
#else
    if ((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) ||
        ((app_bt_device.hfchan_callSetup[anotherDevice] ==
          BTIF_HF_CALL_SETUP_IN) &&
         (app_bt_device.hfchan_call[chan_id_flag.id] != BTIF_HF_CALL_ACTIVE))) {
      app_bt_device.curr_hf_channel_id = anotherDevice;
    } else {
      app_bt_device.curr_hf_channel_id = chan_id_flag.id;
    }
#endif
  }
  TRACE(1, "!!!HF_EVENT_CALLSETUP_IND curr_hf_channel_id:%d\n",
        app_bt_device.curr_hf_channel_id);
#endif
  app_bt_device.hfchan_callSetup[chan_id_flag.id] = ctx->call_setup;
  /////call is alert so remember this time
  if (app_bt_device.hfchan_callSetup[chan_id_flag.id] ==
      BTIF_HF_CALL_SETUP_ALERT) {
    TRACE(1, "HF CALLSETUP TIME=%d", hal_sys_timer_get());
    app_bt_device.hf_callsetup_time[chan_id_flag.id] = hal_sys_timer_get();
  }
  if (app_bt_device.hfchan_callSetup[chan_id_flag.id] ==
      BTIF_HF_CALL_SETUP_IN) {
    btif_hf_list_current_calls(chan);
  }

  if ((app_bt_device.hfchan_callSetup[chan_id_flag.id] == 0) &&
      (app_bt_device.hfchan_call[chan_id_flag.id] == 0)) {
    hfp_call_setup_running_on_clr();
  }
}

static void hfp_current_call_state_handler(hf_chan_handle_t chan,
                                           struct hfp_context *ctx) {
  TRACE(1, "::HF_EVENT_CURRENT_CALL_STATE  chan_id:%d\n", chan_id_flag.id);
#if defined(__BTIF_EARPHONE__)
  hfp_app_status_indication(chan_id_flag.id, ctx);
#endif
}
void app_hfp_mute_upstream(uint8_t devId, bool isMute);

static void hfp_audio_connected_handler(hf_chan_handle_t chan,
                                        struct hfp_context *ctx) {

#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif
#if defined(HFP_1_6_ENABLE)
  hf_chan_handle_t chan_tmp;
#endif
#ifdef __AI_VOICE__
  ai_function_handle(CALLBACK_STOP_SPEECH, NULL, 0);
  ai_function_handle(CALLBACK_AI_APP_KEEPALIVE_POST_HANDLER, NULL, 0); // check
#endif
  if (ctx->status != BT_STS_SUCCESS)
    return;
#if defined(IBRT)
  app_ibrt_if_sniff_checker_start(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);
#endif

  btdrv_set_powerctrl_rssi_low(0xffff);
  btapp_hfp_mic_need_skip_frame_set(2);

#ifdef __BT_ONE_BRING_TWO__
#ifdef SUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING
  if (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) {
    TRACE(0, "::HF_EVENT_AUDIO_CONNECTED no need to update state");
    return;
  }
  hfp_suspend_another_device_a2dp();
#endif
#endif
#ifdef __BT_ONE_BRING_TWO__
  a2dp_dual_slave_setup_during_sco(chan_id_flag.id);
#endif

#if defined(HFP_1_6_ENABLE)
  chan_tmp = app_bt_device.hf_channel[chan_id_flag.id];
  uint16_t codec_id;
#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE) {
    codec_id = p_ibrt_ctrl->ibrt_sco_codec;
  } else
#endif
  {
    codec_id = btif_hf_get_negotiated_codec(chan_tmp);
  }
  TRACE(2, "::HF_EVENT_AUDIO_CONNECTED  chan_id:%d, codec_id:%d\n",
        chan_id_flag.id, codec_id);
  app_audio_manager_set_scocodecid(chan_id_flag.id, codec_id);

  //  bt_drv_reg_op_sco_txfifo_reset(codec_id);
#else
  TRACE(1, "::HF_EVENT_AUDIO_CONNECTED  chan_id:%d\n", chan_id_flag.id);
  // bt_drv_reg_op_sco_txfifo_reset(1);
#endif
#if defined(HFP_NO_PRERMPT)
  if (((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) &&
       (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_DISCON)) &&
      ((app_bt_device.hf_audio_state[chan_id_flag.id] ==
        BTIF_HF_AUDIO_DISCON) &&
       (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_ACTIVE))) {
    app_bt_device.phone_earphone_mark = 0;
    app_bt_device.hf_mute_flag = 0;
  } else if ((app_bt_device.hf_audio_state[chan_id_flag.id] ==
              BTIF_HF_AUDIO_CON) ||
             (app_bt_device.hf_audio_state[anotherDevice] ==
              BTIF_HF_AUDIO_CON)) {
  } else
#endif
  {
    app_bt_device.phone_earphone_mark = 0;
    app_bt_device.hf_mute_flag = 0;
  }
  app_bt_device.hf_audio_state[chan_id_flag.id] = BTIF_HF_AUDIO_CON;

#if defined(__FORCE_REPORTVOLUME_SOCON__)
  btif_hf_report_speaker_volume(chan, hfp_volume_get(chan_id_flag.id));
#endif

#ifdef __BT_ONE_BRING_TWO__
  if (bt_get_sco_number() > 1
#ifdef CHIP_BEST1000
      && hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2
#endif
  ) {
    if (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_ACTIVE) {
#if !defined(HFP_NO_PRERMPT)
#ifndef __HF_KEEP_ONE_ALIVE__
      app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWAP_SCO,
                                    BT_STREAM_SBC, chan_id_flag.id, 0);
#endif
      app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
      app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_DISABLE;
#else
      if ((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_NONE) &&
          (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_DISCON))
        app_bt_device.curr_hf_channel_id = chan_id_flag.id;
#ifdef _THREE_WAY_ONE_CALL_COUNT__
      app_hfp_set_cur_chnl_id(chan_id_flag.id);
      TRACE(4, "%s :%d : app_bt_device.hf_callheld[%d]: %d\n", __func__,
            __LINE__, chan_id_flag.id,
            app_bt_device.hf_callheld[chan_id_flag.id]);
      if (app_bt_device.hf_callheld[chan_id_flag.id] ==
          BTIF_HF_CALL_HELD_ACTIVE) {
        if (app_hfp_get_cur_call_chnl(NULL) == chan_id_flag.id)
          app_hfp_3_way_call_counter_set(chan_id_flag.id, 1);
      } else if ((app_bt_device.hf_callheld[chan_id_flag.id] ==
                  BTIF_HF_CALL_HELD_NONE) ||
                 (app_bt_device.hf_callheld[chan_id_flag.id] ==
                  BTIF_HF_CALL_HELD_NO_ACTIVE)) {
        if (app_hfp_get_cur_call_chnl(NULL) == chan_id_flag.id)
          app_hfp_3_way_call_counter_set(chan_id_flag.id, 0);
      }
#endif
#endif
    } else if (app_bt_device.hf_audio_state[anotherDevice] ==
               BTIF_HF_AUDIO_CON) {
      app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
      app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_ENABLE;
    }
  } else {
    /// if one device  setup a sco connect so get the other device's sco state,
    /// if both connect mute the earlier one
    if (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) {
      app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_DISABLE;
    }
    app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_ENABLE;
  }
#ifndef __HF_KEEP_ONE_ALIVE__
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_VOICE,
                                chan_id_flag.id, MAX_RECORD_NUM);
#endif
#if defined(HFP_NO_PRERMPT)
  TRACE(2, "call[id_other] =%d audio[id_other] = %d",
        app_bt_device.hfchan_call[anotherDevice],
        app_bt_device.hf_audio_state[anotherDevice]);
  if (/*(app_bt_device.hfchan_call[anotherDevice] == HF_CALL_ACTIVE)&&*/
      (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON)) {
  } else
#endif
  {
    if (bt_media_cur_is_bt_stream_media()) {
      app_hfp_start_voice_media(chan_id_flag.id);
      app_bt_device.curr_hf_channel_id = chan_id_flag.id;
    } else {
#ifdef __BT_ONE_BRING_TWO__
      TRACE(1, "%s another[%d] hf_audio_state %d,", __func__, anotherDevice,
            app_bt_device.hf_audio_state[anotherDevice]);

      if (BTIF_HF_AUDIO_CON == app_bt_device.hf_audio_state[anotherDevice]) {
        TRACE(1,
              "disconnect current audio link and don't swith to current sco");
        btif_hf_disc_audio_link(app_bt_device.hf_channel[chan_id_flag.id]);
        app_bt_device.curr_hf_channel_id = anotherDevice;
      }
#else
      if (bt_is_sco_media_open()) {
        TRACE(0,
              "disconnect current audio link and don't swith to current sco");
        app_bt_HF_DisconnectAudioLink(
            app_bt_device.hf_channel[chan_id_flag.id]);
      }
#endif
      else {
        app_hfp_start_voice_media(chan_id_flag.id);
        app_bt_device.curr_hf_channel_id = chan_id_flag.id;
      }
    }
  }
#else
#ifdef _THREE_WAY_ONE_CALL_COUNT__
  app_hfp_set_cur_chnl_id(chan_id_flag.id);
#endif
#ifdef ENABLE_HFP_AUDIO_PENDING_FOR_MEDIA
  if (bt_media_cur_is_bt_stream_media()) {
    app_hfp_set_starting_media_pending_flag(true, BT_DEVICE_ID_1);
  } else
#endif
  {
    app_hfp_start_voice_media(BT_DEVICE_ID_1);
  }
#endif

#ifdef __IAG_BLE_INCLUDE__
  app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_HFP_ON, true);
#endif
}

static void hfp_audio_disconnected_handler(hf_chan_handle_t chan,
                                           struct hfp_context *ctx) {
#ifdef __BT_ONE_BRING_TWO__
  // clear flag_skip_resv and flag_skip_retx
  bt_drv_clear_skip_flag();
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
  if (!btusb_is_bt_mode()) {
    TRACE(0, "btusb_usbaudio_open doing.");
    btusb_usbaudio_open();
  }
#endif
  app_hfp_set_starting_media_pending_flag(false, 0);
#if defined(IBRT)
  app_ibrt_if_sniff_checker_stop(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);
#endif

#ifdef __BT_ONE_BRING_TWO__
#ifdef SUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING
  if (app_bt_is_to_resume_music_player(anotherDevice)) {
    app_bt_resume_music_player(anotherDevice);
  }
#endif
#endif

#ifdef __BT_ONE_BRING_TWO__
  a2dp_dual_slave_handling_refresh();
#endif
  TRACE(1, "::HF_EVENT_AUDIO_DISCONNECTED  chan_id:%d\n", chan_id_flag.id);
  if (app_bt_device.hfchan_call[chan_id_flag.id] == BTIF_HF_CALL_ACTIVE) {
    app_bt_device.phone_earphone_mark = 1;
  }

  app_bt_device.hf_audio_state[chan_id_flag.id] = BTIF_HF_AUDIO_DISCON;

  /* Dont clear callsetup status when audio disc: press iphone volume button
     will disc audio link, but the iphone incoming call is still exist. The
     callsetup status will be reported after call rejected or answered. */
  // app_bt_device.hfchan_callSetup[chan_id_flag.id] = BTIF_HF_CALL_SETUP_NONE;

#ifdef __IAG_BLE_INCLUDE__
  if (!app_bt_is_in_reconnecting()) {
    app_hfp_resume_ble_adv();
  }

  app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_HFP_ON, false);
#endif

#if defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) ||                      \
    defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
  bt_drv_reg_op_clean_flags_of_ble_and_sco();
#endif

#ifdef __BT_ONE_BRING_TWO__
  if (btif_get_hf_chan_state(app_bt_device.hf_channel[anotherDevice]) !=
      BTIF_HF_STATE_OPEN) {
    TRACE(2, "!!!HF_EVENT_AUDIO_DISCONNECTED  hfchan_call[%d]:%d\n",
          anotherDevice, app_bt_device.hfchan_call[anotherDevice]);
  }
  if ((app_bt_device.hfchan_call[anotherDevice] == BTIF_HF_CALL_ACTIVE) ||
      (app_bt_device.hfchan_callSetup[anotherDevice] ==
       BTIF_HF_CALL_SETUP_IN)) {
    //            app_bt_device.curr_hf_channel_id = chan_id_flag.id_other;
    TRACE(
        1,
        "!!!HF_EVENT_AUDIO_DISCONNECTED  app_bt_device.curr_hf_channel_id:%d\n",
        app_bt_device.curr_hf_channel_id);
  } else {
    app_bt_device.curr_hf_channel_id = chan_id_flag.id;
  }
#if defined(_THREE_WAY_ONE_CALL_COUNT__)
  if (chan_id_flag.id == app_hfp_get_cur_call_chnl(NULL)) {
    app_hfp_cur_call_chnl_reset(chan_id_flag.id);
  }
#endif
  app_bt_device.hf_voice_en[chan_id_flag.id] = HF_VOICE_DISABLE;
  if (app_bt_device.hf_audio_state[anotherDevice] == BTIF_HF_AUDIO_CON) {
    app_bt_device.hf_voice_en[anotherDevice] = HF_VOICE_ENABLE;
    TRACE(1, "chan_id:%d AUDIO_DISCONNECTED, then enable id_other voice",
          chan_id_flag.id);
  }
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                chan_id_flag.id, MAX_RECORD_NUM);
#else
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                BT_DEVICE_ID_1, MAX_RECORD_NUM);

#endif
  app_bt_device.callSetupBitRec = 0;
  // app_hfp_mute_upstream(chan_id_flag.id, true);
#if defined(HFP_1_6_ENABLE)
  uint16_t codec_id =
      btif_hf_get_negotiated_codec(app_bt_device.hf_channel[chan_id_flag.id]);
  bt_drv_reg_op_sco_txfifo_reset(codec_id);
#else
  bt_drv_reg_op_sco_txfifo_reset(1);
#endif

  app_bt_active_mode_clear(ACTIVE_MODE_KEEPEER_SCO_STREAMING,
                           UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
}

static void hfp_ring_ind_handler(hf_chan_handle_t chan,
                                 struct hfp_context *ctx) {
#if defined(__BTIF_EARPHONE__) && defined(MEDIA_PLAYER_SUPPORT)
#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T anotherDevice =
      (BT_DEVICE_ID_1 == chan_id_flag.id) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif
  TRACE(1, "::HF_EVENT_RING_IND  chan_id:%d\n", chan_id_flag.id);
  TRACE(1, "btif_hf_is_inbandring_enabled:%d",
        btif_hf_is_inbandring_enabled(chan));
#if defined(__BT_ONE_BRING_TWO__)
  if ((app_bt_device.hf_audio_state[chan_id_flag.id] != BTIF_HF_AUDIO_CON) &&
      (app_bt_device.hf_audio_state[anotherDevice] != BTIF_HF_AUDIO_CON))
    app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL, chan_id_flag.id);
#else

#if defined(IBRT)
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE)
    return;
#endif
  if (app_bt_device.hf_audio_state[chan_id_flag.id] != BTIF_HF_AUDIO_CON)
    app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL, chan_id_flag.id);
#endif
#endif
}

static void hfp_speak_volume_handler(hf_chan_handle_t chan,
                                     struct hfp_context *ctx) {
  TRACE(2, "::HF_EVENT_SPEAKER_VOLUME  chan_id:%d,speaker gain = %d\n",
        chan_id_flag.id, ctx->speaker_volume);
  hfp_volume_set(chan_id_flag.id, (int)ctx->speaker_volume);
}

static void hfp_voice_rec_state_ind_handler(hf_chan_handle_t chan,
                                            struct hfp_context *ctx) {
  TRACE(2, "::HF_EVENT_VOICE_REC_STATE  chan_id:%d,voice_rec_state = %d\n",
        chan_id_flag.id, ctx->voice_rec_state);
}

static void hfp_bes_test_handler(hf_chan_handle_t chan,
                                 struct hfp_context *ctx) {
  // TRACE(0,"HF_EVENT_BES_TEST content =d", Info->p.ptr);
}

static void hfp_read_ag_ind_status_handler(hf_chan_handle_t chan,
                                           struct hfp_context *ctx) {
  TRACE(1, "HF_EVENT_READ_AG_INDICATORS_STATUS %s\n", __func__);
}

static void hfp_call_held_ind_handler(hf_chan_handle_t chan,
                                      struct hfp_context *ctx) {
#if defined(__BT_ONE_BRING_TWO__)
  TRACE(8,
        "::HF_EVENT_CALLHELD_IND %d chan_id %d call %d %d held %d %d "
        "audio_state %d %d\n",
        ctx->call_held, chan_id_flag.id,
        app_bt_device.hfchan_call[BT_DEVICE_ID_1],
        app_bt_device.hfchan_call[BT_DEVICE_ID_2],
        app_bt_device.hf_callheld[BT_DEVICE_ID_1],
        app_bt_device.hf_callheld[BT_DEVICE_ID_2],
        app_bt_device.hf_audio_state[BT_DEVICE_ID_1],
        app_bt_device.hf_audio_state[BT_DEVICE_ID_2]);
#else
  TRACE(2, "::HF_EVENT_CALLHELD_IND  chan_id:%d HELD_STATUS = %d \n",
        chan_id_flag.id, ctx->call_held);
#endif
#if defined(_THREE_WAY_ONE_CALL_COUNT__)
#if defined(__BT_ONE_BRING_TWO__)
  if (app_bt_device.hf_audio_state[chan_id_flag.id] == BTIF_HF_AUDIO_CON &&
      (ctx->call_held == BTIF_HF_CALL_HELD_NONE ||
       ctx->call_held == BTIF_HF_CALL_HELD_ACTIVE)) {
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_SWITCHTO_SCO,
                                  BT_STREAM_VOICE, chan_id_flag.id, 0);
  }
#endif
  app_bt_device.hf_callheld[chan_id_flag.id] = ctx->call_held;
  if (ctx->call_held == BTIF_HF_CALL_HELD_ACTIVE) {
    if (app_hfp_get_cur_call_chnl(NULL) == chan_id_flag.id)
      app_hfp_3_way_call_counter_set(chan_id_flag.id, 1);
  } else if ((ctx->call_held == BTIF_HF_CALL_HELD_NONE) ||
             (ctx->call_held == BTIF_HF_CALL_HELD_NO_ACTIVE)) {
    if (app_hfp_get_cur_call_chnl(NULL) == chan_id_flag.id)
      app_hfp_3_way_call_counter_set(chan_id_flag.id, 0);
  } else {
    TRACE(0, "UNKNOWN CMD.IGNORE");
  }
#endif
}

static uint8_t skip_frame_cnt = 0;
void app_hfp_set_skip_frame(uint8_t frames) { skip_frame_cnt = frames; }
uint8_t app_hfp_run_skip_frame(void) {
  if (skip_frame_cnt > 0) {
    skip_frame_cnt--;
    return 1;
  }
  return 0;
}
uint8_t hfp_is_service_connected(uint8_t device_id) {
  if (device_id >= BT_DEVICE_NUM)
    return 0;
  return app_bt_device.hf_conn_flag[device_id];
}
#if HF_VERSION_1_6 == XA_ENABLED
// HfCommand hf_codec_sel_command;
#endif

static uint8_t app_hfp_is_starting_media_pending_flag = false;
static uint8_t app_hfp_pending_dev_id;
bool app_hfp_is_starting_media_pending(void) {
  return app_hfp_is_starting_media_pending_flag;
}
static uint8_t upstreamMute = 0xff;
void app_hfp_mute_upstream(uint8_t devId, bool isMute) {
  if (upstreamMute != isMute) {
    TRACE(3, "%s devId %d isMute %d", __func__, devId, isMute);
    upstreamMute = isMute;
    if (isMute) {
      btdrv_set_bt_pcm_en(0);
    } else {
      btdrv_set_bt_pcm_en(1);
    }
  }
}

static void app_hfp_set_starting_media_pending_flag(bool isEnabled,
                                                    uint8_t devId) {
  TRACE(1, "%s %d.Current state %d toEnable %d", __func__, __LINE__,
        app_hfp_is_starting_media_pending_flag, isEnabled);
  if ((app_hfp_is_starting_media_pending_flag && isEnabled) ||
      (!app_hfp_is_starting_media_pending_flag && !isEnabled)) {
    return;
  }

  app_hfp_is_starting_media_pending_flag = isEnabled;

  app_hfp_pending_dev_id = devId;
#if 0
    if (isEnabled)
    {
        if (!app_hfp_mediaplay_delay_resume_timer_id ) {
            app_hfp_mediaplay_delay_resume_timer_id  =
                osTimerCreate(osTimer(APP_HFP_MEDIAPLAY_DELAY_RESUME_TIMER),
                osTimerOnce, NULL);
        }
        osTimerStart(app_hfp_mediaplay_delay_resume_timer_id ,
            HFP_MEDIAPLAY_DELAY_RESUME_IN_MS);
    }
#endif
}

void app_hfp_start_voice_media(uint8_t devId) {
  app_hfp_set_starting_media_pending_flag(false, 0);
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_VOICE,
                                devId, MAX_RECORD_NUM);
}

void app_hfp_resume_pending_voice_media(void) {
  if (btapp_hfp_is_dev_sco_connected(app_hfp_pending_dev_id)) {
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_VOICE,
                                  app_hfp_pending_dev_id, MAX_RECORD_NUM);
    app_hfp_set_starting_media_pending_flag(false, 0);
  }
}

#include "app_bt_func.h"
void hfp_multipoint_audio_manage_a2dp_callback() {
#ifdef SUSPEND_ANOTHER_DEV_A2DP_STREAMING_WHEN_CALL_IS_COMING
  int i = 0;
  int j = 0;
  TRACE(1, "%s", __func__);

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (app_bt_device.a2dp_streamming[i] == 1)
      break;
  }
  if (i == BT_DEVICE_NUM)
    return;

  for (j = 0; j < BT_DEVICE_NUM; j++) {
    if (app_bt_device.hf_audio_state[j] == BTIF_HF_AUDIO_CON)
      break;
  }

  if (j == BT_DEVICE_NUM)
    return;

  btif_remote_device_t *activeA2dpRem =
      A2DP_GetRemoteDevice(app_bt_device.a2dp_connected_stream[i]);

  btif_remote_device_t *activeHfpRem =
      (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
          app_bt_device.hf_channel[j]);

  if (activeA2dpRem == activeHfpRem)
    return;

  TRACE(0, "different profile device");
  if (app_bt_is_music_player_working(i)) {
    bool isPaused = app_bt_pause_music_player(i);
    if (isPaused) {
      app_bt_set_music_player_resume_device(i);
    }
  } else if (app_bt_is_a2dp_streaming(i)) {
    app_bt_suspend_a2dp_streaming(i);
  }
#endif
}

void hfp_suspend_another_device_a2dp(void) {
  app_bt_start_custom_function_in_bt_thread(
      0, 0, (uint32_t)hfp_multipoint_audio_manage_a2dp_callback);
}

static void app_hfp_audio_closed_delay_resume_ble_adv_timer_cb(void const *n) {
#ifdef __IAG_BLE_INCLUDE__
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}
#ifdef __IAG_BLE_INCLUDE__
static void app_hfp_resume_ble_adv(void) {
  if (!app_hfp_audio_closed_delay_resume_ble_adv_timer_id) {
    app_hfp_audio_closed_delay_resume_ble_adv_timer_id =
        osTimerCreate(osTimer(APP_HFP_AUDIO_CLOSED_DELAY_RESUME_BLE_ADV_TIMER),
                      osTimerOnce, NULL);
  }

  osTimerStart(app_hfp_audio_closed_delay_resume_ble_adv_timer_id,
               HFP_AUDIO_CLOSED_DELAY_RESUME_ADV_IN_MS);
}
#endif

#ifdef __BT_ONE_BRING_TWO__
#if defined(__BT_SELECT_PROF_DEVICE_ID__)
static void _app_hfp_select_channel(hf_chan_handle_t chan,
                                    struct hfp_context *ctx) {
  uint32_t i = 0;
  btif_remote_device_t *matching_remdev = NULL, *wanted_remdev = NULL;
  hf_chan_handle_t curr_channel;

  wanted_remdev = ctx->chan_sel_remDev;
  // default channel is NULL
  *(ctx->chan_sel_channel) = NULL;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    // Other profile connected
    curr_channel = app_bt_device.hf_channel[i];
    if (app_bt_is_any_profile_connected(i)) {
      matching_remdev = app_bt_get_connected_profile_remdev(i);
      TRACE(3, "device_id=%d, hfp_select_channel : remdev=0x%x:0x%x.", i,
            wanted_remdev, matching_remdev);
      TRACE(1, "device_id=%d, hfp_select_channel : other_profile_connected.",
            i);
      if (wanted_remdev == matching_remdev) {
        TRACE(2, "device_id=%d, hfp_select_channel : found_same_remdev : 0x%x",
              i, &(app_bt_device.hf_channel[i]));
        if (btif_get_hf_chan_state(curr_channel) == BTIF_HF_STATE_CLOSED) {
          TRACE(1,
                "device_id=%d, hfp_select_channel : current_channel_is_closed, "
                "good.",
                i);
          *(ctx->chan_sel_channel) = (uint32_t *)curr_channel;
        } else {
          TRACE(2,
                "device_id=%d, hfp_select_channel : other_profile_connected: "
                "current_channel_is_not_closed %d, ohno.",
                i, btif_get_hf_chan_state(curr_channel));
          TRACE(1,
                "device_id=%d, hfp_select_channel : other_profile_connected: "
                "missed right channel, found nothing, return",
                i);
        }
        return;
      } else {
        TRACE(1,
              "device_id=%d, hfp_select_channel : different_remdev, see next "
              "device id",
              i);
      }
    } else {
      TRACE(1,
            "device_id=%d, hfp_select_channel : other_profile_not_connected.",
            i);
      // first found idle device id is min device id we want
      // Assume : other profile will use device id ascending
      // TODO to keep other profile use device id ascending
      if (btif_get_hf_chan_state(curr_channel) == BTIF_HF_STATE_CLOSED) {
        TRACE(1,
              "device_id=%d, hfp_select_channel : current_channel_is_closed, "
              "choose this idle channel.",
              i);
        *(ctx->chan_sel_channel) = (uint32_t *)curr_channel;
      } else {
        TRACE(2,
              "device_id=%d, hfp_select_channel : no_other_profile_connected : "
              "current_channel_is_not_closed %d, ohno.",
              i, btif_get_hf_chan_state(curr_channel));
        TRACE(1,
              "device_id=%d, hfp_select_channel : no_other_profile_connected : "
              "missed right channel, found nothing, return",
              i);
      }
      return;
    }
  }
}
#endif
#endif

void app_hfp_event_callback(hf_chan_handle_t chan, struct hfp_context *ctx) {
  struct bt_cb_tag *bt_drv_func_cb = bt_drv_get_func_cb_ptr();

#ifdef __BT_ONE_BRING_TWO__
  if (ctx->event == BTIF_HF_EVENT_SELECT_CHANNEL) {
#if defined(__BT_SELECT_PROF_DEVICE_ID__)
    _app_hfp_select_channel(chan, ctx);
#endif
    return;
  }
  hfp_chan_id_distinguish(chan);
#else
  if (ctx->event == BTIF_HF_EVENT_SELECT_CHANNEL) {
    return;
  }
  chan_id_flag.id = BT_DEVICE_ID_1;
#endif
  switch (ctx->event) {
  case BTIF_HF_EVENT_SERVICE_CONNECTED:
    hfp_connected_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_AUDIO_DATA_SENT:
    hfp_audio_data_sent_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_AUDIO_DATA:
    hfp_audio_data_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_SERVICE_DISCONNECTED:
    hfp_disconnected_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_CALL_IND:
    hfp_call_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_CALLSETUP_IND:
    hfp_callsetup_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_CURRENT_CALL_STATE:
    hfp_current_call_state_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_AUDIO_CONNECTED:
#ifdef IBRT
  {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    bt_drv_reg_op_set_agc_thd(p_ibrt_ctrl->current_role == IBRT_MASTER, true);

    if (!btif_besaud_is_connected()) {
      bt_drv_reg_op_hack_max_slot(p_ibrt_ctrl->mobile_conhandle - 0x80, 1);
    }
  }
#endif

    if (bt_drv_func_cb->bt_switch_agc != NULL) {
      bt_drv_func_cb->bt_switch_agc(BT_HFP_WORK_MODE);
    }

    hfp_audio_connected_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_AUDIO_DISCONNECTED:

    if (bt_drv_func_cb->bt_switch_agc != NULL) {
      bt_drv_func_cb->bt_switch_agc(BT_IDLE_MODE);
    }

    hfp_audio_disconnected_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_RING_IND:
    hfp_ring_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_SPEAKER_VOLUME:
    hfp_speak_volume_handler(chan, ctx);
    break;
#ifdef SUPPORT_SIRI
  case BTIF_HF_EVENT_SIRI_STATUS:
    break;
#endif
  case BTIF_HF_EVENT_BES_TEST:
    hfp_bes_test_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_READ_AG_INDICATORS_STATUS:
    hfp_read_ag_ind_status_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_CALLHELD_IND:
    hfp_call_held_ind_handler(chan, ctx);
    break;
  case BTIF_HF_EVENT_COMMAND_COMPLETE:
    break;
  case BTIF_HF_EVENT_AT_RESULT_DATA:
    TRACE(1, "received AT command: %s", ctx->ptr);
#ifdef __INTERACTION__
    if (!memcmp(oppo_self_defined_command_response, ctx->ptr,
                strlen(oppo_self_defined_command_response))) {
      for (int i = 0; i < BT_DEVICE_NUM; i++) {
        chan = app_bt_device.hf_channel[i];
        {
          TRACE(2, "hf state=%x %d", chan, btif_get_hf_chan_state(chan));
          if (btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN) {
            // char firmwareversion[] = "AT+VDRV=3,1,9,2,9,3,9";
            // sprintf(&firmwareversion[27], "%d", (int)NeonFwVersion[0]);
            // sprintf(&firmwareversion[28], "%d", (int)NeonFwVersion[1]);
            // sprintf(&firmwareversion[29], "%d", (int)NeonFwVersion[2]);
            // btif_hf_send_at_cmd(chan,firmwareversion);
          }
        }
      }
      TRACE(0, "oppo_self_defined_command_response");
    }
#endif
#ifdef __INTERCONNECTION__
    if (!memcmp(huawei_self_defined_command_response, ctx->ptr,
                strlen(huawei_self_defined_command_response) + 1)) {
      uint8_t *pSelfDefinedCommandSupport =
          app_battery_get_mobile_support_self_defined_command_p();
      *pSelfDefinedCommandSupport = 1;

      TRACE(0, "send self defined AT command to mobile.");
      send_selfdefined_battery_report_AT_command();
    }
#endif
    break;

  case BTIF_HF_EVENT_VOICE_REC_STATE:
    hfp_voice_rec_state_ind_handler(chan, ctx);
    break;

  default:
    break;
  }

#ifdef __BT_ONE_BRING_TWO__
  hfcall_next_sta_handler(ctx->event);
#endif

#if defined(IBRT)
  app_tws_ibrt_profile_callback(BTIF_APP_HFP_PROFILE_ID, (void *)chan,
                                (void *)ctx);
#endif
}

uint8_t btapp_hfp_get_call_state(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (app_bt_device.hfchan_call[i] == BTIF_HF_CALL_ACTIVE) {
      return 1;
    }
  }
  return 0;
}

uint8_t btapp_hfp_get_call_setup(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if ((app_bt_device.hfchan_callSetup[i] != BTIF_HF_CALL_SETUP_NONE)) {
      return (app_bt_device.hfchan_callSetup[i]);
    }
  }
  return 0;
}

uint8_t btapp_hfp_incoming_calls(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (app_bt_device.hfchan_callSetup[i] == BTIF_HF_CALL_SETUP_IN) {
      return 1;
    }
  }
  return 0;
}

bool btapp_hfp_is_call_active(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if ((app_bt_device.hfchan_call[i] == BTIF_HF_CALL_ACTIVE) &&
        (app_bt_device.hf_audio_state[i] == BTIF_HF_AUDIO_CON)) {
      return true;
    }
  }
  return false;
}

bool btapp_hfp_is_sco_active(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (app_bt_device.hf_audio_state[i] == BTIF_HF_AUDIO_CON) {
      return true;
    }
  }
  return false;
}

bool btapp_hfp_is_dev_call_active(uint8_t devId) {
  return ((app_bt_device.hfchan_call[devId] == BTIF_HF_CALL_ACTIVE) &&
          (app_bt_device.hf_audio_state[devId] == BTIF_HF_AUDIO_CON));
}

bool btapp_hfp_is_dev_sco_connected(uint8_t devId) {
  return (app_bt_device.hf_audio_state[devId] == BTIF_HF_AUDIO_CON);
}

uint8_t btapp_hfp_get_call_active(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if ((app_bt_device.hfchan_call[i] == BTIF_HF_CALL_ACTIVE) ||
        (app_bt_device.hfchan_callSetup[i] == BTIF_HF_CALL_SETUP_ALERT)) {

      return 1;
    }
  }
  return 0;
}

void btapp_hfp_report_speak_gain(void) {
  uint8_t i;
  btif_remote_device_t *remDev = NULL;
  btif_link_mode_t mode = BTIF_BLM_SNIFF_MODE;
  hf_chan_handle_t chan;

  for (i = 0; i < BT_DEVICE_NUM; i++) {
    osapi_lock_stack();
    remDev = (btif_remote_device_t *)btif_hf_cmgr_get_remote_device(
        app_bt_device.hf_channel[i]);
    if (remDev) {
      mode = btif_me_get_current_mode(remDev);
    } else {
      mode = BTIF_BLM_SNIFF_MODE;
    }
    chan = app_bt_device.hf_channel[i];
    if ((btif_get_hf_chan_state(chan) == BTIF_HF_STATE_OPEN) &&
        (mode == BTIF_BLM_ACTIVE_MODE)) {
      btif_hf_report_speaker_volume(chan,
                                    hfp_volume_get((enum BT_DEVICE_ID_T)i));
    }
    osapi_unlock_stack();
  }
}

uint8_t btapp_hfp_need_mute(void) { return app_bt_device.hf_mute_flag; }

int32_t hfp_mic_need_skip_frame_cnt = 0;

bool btapp_hfp_mic_need_skip_frame(void) {
  bool nRet;

  if (hfp_mic_need_skip_frame_cnt > 0) {
    hfp_mic_need_skip_frame_cnt--;
    nRet = true;
  } else {
    app_hfp_mute_upstream(0, false);
    nRet = false;
  }
  return nRet;
}

void btapp_hfp_mic_need_skip_frame_set(int32_t skip_frame) {
  hfp_mic_need_skip_frame_cnt = skip_frame;
}

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
typedef void (*btapp_set_sco_switch_cmd_callback)(void);

btapp_set_sco_switch_cmd_callback set_sco_switch_cmd_callback;

void btapp_sco_switch_set_pcm(void) {
  TRACE(0, "btapp_sco_switch_set_pcm\n");
  TRACE(1, "0xd02201b0 = 0x%x before\n", *(volatile uint32_t *)(0xd02201b0));
  osDelay(20);
  btdrv_pcm_enable();
  TRACE(1, "0xd02201b0 = 0x%x after\n", *(volatile uint32_t *)(0xd02201b0));
}
#endif

void app_hfp_init(void) {
  hfp_hfcommand_mempool_init();
#if defined(ENHANCED_STACK)
  btif_hfp_initialize();
#endif /* ENHANCED_STACK */
  app_bt_device.curr_hf_channel_id = BT_DEVICE_ID_1;
  app_bt_device.hf_mute_flag = 0;

  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    app_bt_device.hf_channel[i] = btif_hf_create_channel();
    if (!app_bt_device.hf_channel[i]) {
      ASSERT(0, "Serious error: cannot create hf channel\n");
    }
    btif_hf_init_channel(app_bt_device.hf_channel[i]);
    app_bt_device.hfchan_call[i] = 0;
    app_bt_device.hfchan_callSetup[i] = 0;
    app_bt_device.hf_audio_state[i] = BTIF_HF_AUDIO_DISCON,
    app_bt_device.hf_conn_flag[i] = false;
    app_bt_device.hf_voice_en[i] = 0;
  }
  btif_hf_register_callback(app_hfp_event_callback);
#if defined(SUPPORT_BATTERY_REPORT) || defined(SUPPORT_HF_INDICATORS)
  Besbt_hook_handler_set(BESBT_HOOK_USER_3, app_hfp_battery_report_proc);
#endif

#if defined(CHIP_BEST2300) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
  set_sco_switch_cmd_callback = btapp_sco_switch_set_pcm;
#endif
  app_hfp_mute_upstream(chan_id_flag.id, true);
}

void app_hfp_enable_audio_link(bool isEnable) { return; }

#if defined(IBRT)
int hfp_ibrt_service_connected_mock(void) {
  if (btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]) ==
      BTIF_HF_STATE_OPEN) {
    TRACE(0, "::HF_EVENT_SERVICE_CONNECTED mock");
    app_bt_device.hf_conn_flag[chan_id_flag.id] = 1;
  } else {
    TRACE(1, "::HF_EVENT_SERVICE_CONNECTED mock need check chan_state:%d",
          btif_get_hf_chan_state(app_bt_device.hf_channel[BT_DEVICE_ID_1]));
  }

  return 0;
}

int hfp_ibrt_sync_get_status(ibrt_hfp_status_t *hfp_status) {
  hfp_status->audio_state =
      (uint8_t)app_bt_device.hf_audio_state[BT_DEVICE_ID_1];
  hfp_status->volume = hfp_volume_get(BT_DEVICE_ID_1);
  hfp_status->lmp_sco_hdl = 0;

  if (hfp_status->audio_state == BTIF_HF_AUDIO_CON &&
      app_tws_ibrt_mobile_link_connected()) {
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    uint16_t sco_connhdl =
        btif_me_get_scohdl_by_connhdl(p_ibrt_ctrl->mobile_conhandle);
    hfp_status->lmp_sco_hdl = bt_drv_reg_op_lmp_sco_hdl_get(sco_connhdl);
    TRACE(2, "ibrt_ui_log:get sco lmp hdl %04x %02x\n", sco_connhdl,
          hfp_status->lmp_sco_hdl);
  }

  return 0;
}

int hfp_ibrt_sync_set_status(ibrt_hfp_status_t *hfp_status) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  btif_remote_device_t *remDev = NULL;

  TRACE(4, "%s audio_state:%d volume:%d lmp_scohdl:%02x", __func__,
        hfp_status->audio_state, hfp_status->volume, hfp_status->lmp_sco_hdl);

  app_bt_device.hf_audio_state[BT_DEVICE_ID_1] =
      (btif_audio_state_t)hfp_status->audio_state;

  if (app_tws_ibrt_mobile_link_connected()) {
    remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->mobile_conhandle);
  } else if (app_tws_ibrt_slave_ibrt_link_connected()) {
    remDev = btif_me_get_remote_device_by_handle(p_ibrt_ctrl->ibrt_conhandle);
  }

  if (remDev) {
    app_bt_stream_volume_ptr_update(
        (uint8_t *)btif_me_get_remote_device_bdaddr(remDev));
  } else {
    app_bt_stream_volume_ptr_update(NULL);
  }

  hfp_volume_set(BT_DEVICE_ID_1, hfp_status->volume);

  p_ibrt_ctrl->ibrt_sco_lmphdl = 0;

  if (hfp_status->audio_state == BTIF_HF_AUDIO_CON &&
      hfp_status->lmp_sco_hdl != 0 &&
      app_tws_ibrt_slave_ibrt_link_connected()) {
    uint16_t sco_connhdl = 0x0100; // SYNC_HFP_STATUS arrive before mock
                                   // sniffer_sco, so use 0x0100 directly
    if (bt_drv_reg_op_lmp_sco_hdl_set(sco_connhdl, hfp_status->lmp_sco_hdl)) {
      TRACE(2, "ibrt_ui_log:set sco %04x lmp hdl %02x\n", sco_connhdl,
            hfp_status->lmp_sco_hdl);
    } else {
      // SYNC_HFP_STATUS may so much faster lead bt_drv_reg_op_lmp_sco_hdl_set
      // fail, backup the value
      p_ibrt_ctrl->ibrt_sco_lmphdl = hfp_status->lmp_sco_hdl;
    }
  }

  return 0;
}

int hfp_ibrt_sco_audio_connected(hfp_sco_codec_t codec, uint16_t sco_connhdl) {
  int ret = BT_STS_SUCCESS;
  ibrt_ctrl_t *p_ibrt_ctrl = NULL;

  TRACE(0, "::HF_EVENT_AUDIO_CONNECTED mock");

  app_bt_device.hf_audio_state[BT_DEVICE_ID_1] = BTIF_HF_AUDIO_CON;
  btif_hf_set_negotiated_codec(app_bt_device.hf_channel[BT_DEVICE_ID_1], codec);
  app_audio_manager_set_scocodecid(BT_DEVICE_ID_1, codec);
#ifdef ENABLE_HFP_AUDIO_PENDING_FOR_MEDIA
  if (bt_media_cur_is_bt_stream_media()) {
    app_hfp_set_starting_media_pending_flag(true, BT_DEVICE_ID_1);
  } else
#endif
  {
    app_hfp_start_voice_media(BT_DEVICE_ID_1);
  }
  app_ibrt_if_sniff_checker_start(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);

  struct bt_cb_tag *bt_drv_func_cb = bt_drv_get_func_cb_ptr();
  if (bt_drv_func_cb->bt_switch_agc != NULL) {
    bt_drv_func_cb->bt_switch_agc(BT_HFP_WORK_MODE);
  }

  p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->ibrt_sco_lmphdl != 0) {
    // set backuped lmphdl if it is failed in hfp_ibrt_sync_set_status
    TRACE(2, "ibrt_ui_log:set sco %04x lmp hdl %02x\n", sco_connhdl,
          p_ibrt_ctrl->ibrt_sco_lmphdl);
    bt_drv_reg_op_lmp_sco_hdl_set(sco_connhdl, p_ibrt_ctrl->ibrt_sco_lmphdl);
    p_ibrt_ctrl->ibrt_sco_lmphdl = 0;
  }

  return ret;
}

int hfp_ibrt_sco_audio_disconnected(void) {
  int ret = BT_STS_SUCCESS;
  TRACE(0, "::HF_EVENT_AUDIO_DISCONNECTED mock");
  app_bt_device.hf_audio_state[BT_DEVICE_ID_1] = BTIF_HF_AUDIO_DISCON;
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                BT_DEVICE_ID_1, 0);
  app_ibrt_if_sniff_checker_stop(APP_IBRT_IF_SNIFF_CHECKER_USER_HFP);

  struct bt_cb_tag *bt_drv_func_cb = bt_drv_get_func_cb_ptr();
  if (bt_drv_func_cb->bt_switch_agc != NULL) {
    bt_drv_func_cb->bt_switch_agc(BT_IDLE_MODE);
  }
  return ret;
}

#endif
