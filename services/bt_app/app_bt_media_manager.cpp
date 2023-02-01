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
#include <assert.h>
#include <stdio.h>

#include "analog.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_bt_trace.h"
#include "app_overlay.h"
#include "app_utils.h"
#include "audioflinger.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "nvrecord.h"
#if defined(IBRT)
#include "app_ibrt_if.h"
#else
#include "app_media_player.h"
#endif

#include "resources.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#endif

#include "app_bt_func.h"
#include "apps.h"
#include "bt_drv.h"

#include "besbt.h"

#include "app_hfp.h"
#include "btapp.h"
#include "cqueue.h"

#include "app_bt_media_manager.h"
#include "app_thread.h"

#include "app_ai_if.h"
#include "app_ring_merge.h"
#include "audio_prompt_sbc.h"
#include "bt_if.h"

int bt_sco_player_forcemute(bool mic_mute, bool spk_mute);
int bt_sco_player_get_codetype(void);
extern struct BT_DEVICE_T app_bt_device;
extern enum AUD_SAMPRATE_T a2dp_sample_rate;

struct bt_media_manager {
  uint16_t media_active[BT_DEVICE_NUM];
  uint8_t media_current_call_state[BT_DEVICE_NUM];
  uint8_t media_curr_sbc;
  uint8_t media_curr_sco;
  uint16_t curr_active_media; // low 8 bits are out direciton, while high 8 bits
                              // are in direction
};

static char _strm_type_str[168];
static char *_catstr(char *dst, const char *src) {
  while (*dst)
    dst++;
  while ((*dst++ = *src++))
    ;
  return --dst;
}
static const char *strmtype2str(uint16_t stream_type) {
  const char *s = NULL;
  char _cat = 0, first = 1, *d = NULL;
  _strm_type_str[0] = '\0';
  d = _strm_type_str;
  d = _catstr(d, "[");
  if (stream_type != 0) {
    for (int i = 15; i >= 0; i--) {
      _cat = 1;
      // TRACE_AUD_MGR_I("i=%d,stream_type=0x%d,stream_type&(1<<i)=0x%x", i,
      // stream_type, stream_type&(1<<i));
      switch (stream_type & (1 << i)) {
      case 0:
        _cat = 0;
        break;
      case BT_STREAM_SBC:
        s = "SBC";
        break;
      case BT_STREAM_MEDIA:
        s = "MEDIA";
        break;
      case BT_STREAM_VOICE:
        s = "VOICE";
        break;
#ifdef RB_CODEC
      case BT_STREAM_RBCODEC:
        s = "RB_CODEC";
        break;
#endif
        // direction is in
#ifdef VOICE_DATAPATH
      case BT_STREAM_CAPTURE:
        s = "CAPTURE";
        break;
#endif
#ifdef __AI_VOICE__
      case BT_STREAM_AI_VOICE:
        s = "AI_VOICE";
        break;
#endif
#ifdef __THIRDPARTY
      case BT_STREAM_THIRDPARTY_VOICE:
        s = "THIRDPARTY";
        break;
#endif
#ifdef AUDIO_LINEIN
      case BT_STREAM_LINEIN:
        s = "LINEIN";
        break;
#endif
      default:
        s = "UNKNOWN";
        break;
      }
      if (_cat) {
        if (!first)
          d = _catstr(d, "|");
        // TRACE_AUD_MGR_I("d=%s,s=%s", d, s);
        d = _catstr(d, s);
        first = 0;
      }
    }
  }

  _catstr(d, "]");

  return _strm_type_str;
}

static const char *handleId2str(uint8_t id) {
#define CASE_M(s)                                                              \
  case APP_BT_STREAM_MANAGER_##s:                                              \
    return "[" #s "]";

  switch (id) {
    CASE_M(START)
    CASE_M(STOP)
    CASE_M(SWITCHTO_SCO)
    CASE_M(STOP_MEDIA)
    CASE_M(UPDATE_MEDIA)
    CASE_M(SWAP_SCO)
    CASE_M(CTRL_VOLUME)
    CASE_M(TUNE_SAMPLERATE_RATIO)
  }
  return "[]";
}

extern enum AUD_SAMPRATE_T sco_sample_rate;
static struct bt_media_manager bt_meida;

uint16_t bt_media_get_media_active(enum BT_DEVICE_ID_T device_id) {
  return bt_meida.media_active[device_id];
}

uint8_t bt_media_is_media_active_by_type(uint16_t media_type) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_meida.media_active[i] & media_type)
      return 1;
  }
  return 0;
}

bool bt_media_is_media_idle(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_meida.media_active[i] != 0)
      return false;
  }

  return true;
}
void bt_media_clean_up(void) {
  bt_meida.curr_active_media = 0;
  for (uint8_t index = 0; index < BT_DEVICE_NUM; index++) {
    bt_meida.media_active[index] = 0;
  }
}

bool bt_media_is_media_active_by_sbc(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_meida.media_active[i] & BT_STREAM_SBC)
      return 1;
  }
  return 0;
}
bool bt_is_sco_media_open(void) {
  return (bt_meida.curr_active_media == BT_STREAM_VOICE) ? (true) : (false);
}

static enum BT_DEVICE_ID_T
bt_media_get_active_device_by_type(uint16_t media_type) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if (bt_meida.media_active[i] & media_type)
      return (enum BT_DEVICE_ID_T)i;
  }
  return BT_DEVICE_NUM;
}

uint8_t bt_media_is_media_active_by_device(uint16_t media_type,
                                           enum BT_DEVICE_ID_T device_id) {
  if (bt_meida.media_active[device_id] & media_type)
    return 1;
  return 0;
}

uint16_t bt_media_get_current_media(void) { return bt_meida.curr_active_media; }

bool bt_media_cur_is_bt_stream_media(void) {
  return (BT_STREAM_MEDIA & bt_meida.curr_active_media) ? (true) : (false);
}

bool bt_media_is_sbc_media_active(void) {
  return (bt_media_is_media_active_by_type(BT_STREAM_SBC) == 1) ? (true)
                                                                : (false);
}

void bt_media_current_sbc_set(uint8_t id) {
  TRACE_AUD_MGR_I("current sbc %d->%d", bt_meida.media_curr_sbc, id);
  bt_meida.media_curr_sbc = id;
}

static void bt_media_set_current_media(uint16_t media_type) {
  // out direction
  if (media_type < 0x100) {
    bt_meida.curr_active_media &= (~0xFF);
    bt_meida.curr_active_media |= media_type;
  } else {
    // bt_meida.curr_active_media &= (~0xFF00);
    bt_meida.curr_active_media |= media_type;
  }

  TRACE_AUD_MGR_I("curr_active_media is set to 0x%x->:%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
}

static void bt_media_clear_current_media(uint16_t media_type) {
  if (media_type < 0x100) {
    bt_meida.curr_active_media &= (~0xFF);
  } else {
    bt_meida.curr_active_media &= (~media_type);
  }
  TRACE_AUD_MGR_I("clear media 0x%x curr media 0x%x", media_type,
                  bt_meida.curr_active_media);
}
#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
static void bt_media_clear_all_media_type(void) {
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    bt_meida.media_active[i] &= (~BT_STREAM_MEDIA);
  }
}
#endif
#endif

static uint8_t bt_media_set_media_type(uint16_t media_type,
                                       enum BT_DEVICE_ID_T device_id) {
  TRACE_AUD_MGR_I("[MEDIA_TYPE][SET] 0x%x active_now 0x%x id %d", media_type,
                  bt_meida.media_active[0], device_id);
  if (device_id < BT_DEVICE_NUM) {
    bt_meida.media_active[device_id] |= media_type;
  } else {
    TRACE_AUD_MGR_W("[MEDIA_TYPE][SET] invalid devcie_id:%d", device_id);
  }
  return 0;
}

void bt_media_clear_media_type(uint16_t media_type,
                               enum BT_DEVICE_ID_T device_id) {
  TRACE_AUD_MGR_I("[MEDIA_TYPE][CLR] 0x%x active_now 0x%x id %d", media_type,
                  bt_meida.media_active[0], device_id);
  if (media_type == BT_STREAM_MEDIA) {
    for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
      bt_meida.media_active[i] &= (~media_type);
    }
  } else {
    bt_meida.media_active[device_id] &= (~media_type);
  }
}

static enum BT_DEVICE_ID_T bt_media_get_active_sbc_device(void) {
  enum BT_DEVICE_ID_T device = BT_DEVICE_NUM;
  uint8_t i;
  for (i = 0; i < BT_DEVICE_NUM; i++) {
    if ((bt_meida.media_active[i] & BT_STREAM_SBC) &&
        (i == bt_meida.media_curr_sbc))
      device = (enum BT_DEVICE_ID_T)i;
  }
  return device;
}

#ifdef RB_CODEC
bool bt_media_rbcodec_start_process(uint16_t stream_type,
                                    enum BT_DEVICE_ID_T device_id,
                                    AUD_ID_ENUM media_id, uint32_t param,
                                    uint32_t ptr) {
  int ret_SendReq2AudioThread = -1;
  bt_meida.media_active[device_id] |= stream_type;

  ret_SendReq2AudioThread = app_audio_sendrequest(
      APP_BT_STREAM_RBCODEC, (uint8_t)APP_BT_SETTING_OPEN, media_id);
  bt_media_set_current_media(BT_STREAM_RBCODEC);
  return true;
exit:
  return false;
}
#endif

#ifdef VOICE_DATAPATH
static bool bt_media_voicepath_start_process(uint16_t stream_type,
                                             enum BT_DEVICE_ID_T device_id,
                                             AUD_ID_ENUM media_id,
                                             uint32_t param, uint32_t ptr) {
  bt_meida.media_active[device_id] |= stream_type;
  app_audio_sendrequest(APP_BT_STREAM_VOICEPATH, (uint8_t)APP_BT_SETTING_OPEN,
                        media_id);

  bt_media_set_current_media(BT_STREAM_CAPTURE);
  return true;
}
#endif

#ifdef __AI_VOICE__
static bool bt_media_ai_voice_start_process(uint16_t stream_type,
                                            enum BT_DEVICE_ID_T device_id,
                                            AUD_ID_ENUM media_id,
                                            uint32_t param, uint32_t ptr) {
  bt_meida.media_active[device_id] |= stream_type;
  app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_OPEN,
                        media_id);

  bt_media_set_current_media(BT_STREAM_AI_VOICE);
  return true;
}
#endif

#ifdef __THIRDPARTY
static bool bt_media_thirdparty_voice_start_process(
    uint16_t stream_type, enum BT_DEVICE_ID_T device_id, AUD_ID_ENUM media_id,
    uint32_t param, uint32_t ptr) {
  bt_meida.media_active[device_id] |= stream_type;
  app_audio_sendrequest(APP_BT_STREAM_THIRDPARTY_VOICE,
                        (uint8_t)APP_BT_SETTING_OPEN, media_id);

  bt_media_set_current_media(BT_STREAM_THIRDPARTY_VOICE);
  return true;
}
#endif

void app_stop_a2dp_media_stream(uint8_t devId) {
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_SBC,
                                devId, 0);
}

void app_stop_sco_media_stream(uint8_t devId) {
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                devId, 0);
}

// only used in iamain thread ,can't used in other thread or interrupt
void bt_media_start(uint16_t stream_type, enum BT_DEVICE_ID_T device_id,
                    uint16_t media_id) {
#ifdef __BT_ONE_BRING_TWO__
  enum BT_DEVICE_ID_T other_device_id =
      (device_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  bool isMergingPrompt = true;
#endif
#endif

  bt_media_set_media_type(stream_type, device_id);

  TRACE_AUD_MGR_I(
      "[HANDLE_START] type= 0x%x->:%s,device id = 0x%x,media_id = 0x%x",
      stream_type, strmtype2str(stream_type), device_id, media_id);

#ifdef __BT_ONE_BRING_TWO__
  TRACE_AUD_MGR_I("[HANDLE_START] media_active[0] = 0x%x->:%s",
                  bt_meida.media_active[0],
                  strmtype2str(bt_meida.media_active[0]));
  TRACE_AUD_MGR_I("[HANDLE_START] media_active[1] = 0x%x->:%s",
                  bt_meida.media_active[1],
                  strmtype2str(bt_meida.media_active[1]));
  TRACE_AUD_MGR_I("[HANDLE_START] curr_active_media = 0x%x->:%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
#else
  TRACE_AUD_MGR_I("[HANDLE_START] media_active = 0x%x->:%s",
                  bt_meida.media_active[0],
                  strmtype2str(bt_meida.media_active[0]));
  TRACE_AUD_MGR_I("[HANDLE_START] curr_active_media = 0x%x->:%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
#endif

  switch (stream_type) {
#ifdef RB_CODEC
  case BT_STREAM_RBCODEC:
    if (!bt_media_rbcodec_start_process(stream_type, device_id,
                                        (AUD_ID_ENUM)media_id, NULL, NULL))
      goto exit;
    break;
#endif

#ifdef VOICE_DATAPATH
  case BT_STREAM_CAPTURE:
    if (bt_media_get_current_media() & BT_STREAM_CAPTURE) {
      TRACE_AUD_MGR_I("[HANDLE_START] capture stream exist ,do nothing");
      return;
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      goto exit;
    }

#ifdef MEDIA_PLAYER_SUPPORT
#if !ISOLATED_AUDIO_STREAM_ENABLED
    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      return;
    }
#endif
#endif
    if (!bt_media_voicepath_start_process(BT_STREAM_CAPTURE, device_id,
                                          (AUD_ID_ENUM)media_id, (uint32_t)NULL,
                                          (uint32_t)NULL))
      goto exit;
    break;
#endif

#ifdef __AI_VOICE__
  case BT_STREAM_AI_VOICE:
    if (bt_media_get_current_media() & BT_STREAM_AI_VOICE) {
      TRACE_AUD_MGR_I("[HANDLE_START] ai voice stream exist ,do nothing");
      return;
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      TRACE_AUD_MGR_I("[HANDLE_START] SCO stream exist ,do nothing");
      goto exit;
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      if (bt_media_get_current_media() & BT_STREAM_SBC) {
        TRACE_AUD_MGR_I("[HANDLE_START] CLOSE SBC.");
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_SBC);
      }
    }

    if (!bt_media_ai_voice_start_process(BT_STREAM_AI_VOICE, device_id,
                                         (AUD_ID_ENUM)media_id, (uint32_t)NULL,
                                         (uint32_t)NULL))
      goto exit;
    break;
#endif
#ifdef __THIRDPARTY
  case BT_STREAM_THIRDPARTY_VOICE:
    if (bt_media_get_current_media() & BT_STREAM_THIRDPARTY_VOICE) {
      TRACE_AUD_MGR_I(
          "[HANDLE_START]there is a thirdparty voice stream exist ,do nothing");
      return;
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      TRACE_AUD_MGR_I("[HANDLE_START]there is a SCO stream exist ,do nothing");
      goto exit;
    }

    if (!bt_media_thirdparty_voice_start_process(
            BT_STREAM_THIRDPARTY_VOICE, device_id, (AUD_ID_ENUM)media_id,
            (uint32_t)NULL, (uint32_t)NULL))
      goto exit;
    break;
#endif

  case BT_STREAM_SBC: {
    uint8_t media_pre_sbc = bt_meida.media_curr_sbc;
    ////because voice is the highest priority and media report will stop soon
    //// so just store the sbc type
    if (bt_meida.media_curr_sbc == BT_DEVICE_NUM)
      bt_meida.media_curr_sbc = device_id;
    TRACE_AUD_MGR_I("[HANDLE_START] pre/cur_sbc = %d/%d", media_pre_sbc,
                    bt_meida.media_curr_sbc);

#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    /// clear the pending stop flag if it is set
    audio_prompt_clear_pending_stream(PENDING_TO_STOP_A2DP_STREAMING);
#endif

    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      goto exit;
    }
#endif
#ifdef RB_CODEC
    if (bt_media_is_media_active_by_type(BT_STREAM_RBCODEC)) {
      goto exit;
    }
#endif

#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
    if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      goto exit;
    }
#endif
#endif

#ifdef __AI_VOICE__
    if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) ||
        ai_if_is_ai_stream_mic_open()) {
      TRACE_AUD_MGR_I("[HANDLE_START] ai voice stream exist");
      goto exit;
    }
#endif

#ifdef AUDIO_LINEIN
    if (bt_media_is_media_active_by_type(BT_STREAM_LINEIN)) {
      if (bt_media_get_current_media() & BT_STREAM_LINEIN) {
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,
                                      (uint8_t)BT_STREAM_LINEIN, 0, 0);
        app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                      BT_STREAM_SBC, device_id, 0);
        return;
      }
    }
#endif

    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      ////sbc and voice is all on so set sys freq to 104m
      app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
      return;
    }
#ifdef __BT_ONE_BRING_TWO__
    else if (btapp_hfp_is_call_active()) {
      // do nothing
      TRACE_AUD_MGR_I("[HANDLE_START] skip sbc start, because HF_CALL_ACTIVE");
    } else if (bt_media_is_media_active_by_device(BT_STREAM_SBC,
                                                  other_device_id)) {
#if !defined(__MULTIPOINT_A2DP_PREEMPT__)
      // if another device is the active stream do nothing
      if (bt_meida.media_curr_sbc == other_device_id) {

        /// 2 device is play sbc,so set sys freq to 104m
        app_audio_manager_switch_a2dp(
            (enum BT_DEVICE_ID_T)bt_meida.media_curr_sbc);
        app_audio_sendrequest_param((uint8_t)APP_BT_STREAM_A2DP_SBC,
                                    (uint8_t)APP_BT_SETTING_RESTART, 0,
                                    APP_SYSFREQ_104M);
        return;
      }
#else
      if (bt_meida.media_curr_sbc == other_device_id) {
        app_audio_manager_switch_a2dp(device_id);
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)(APP_BT_SETTING_SETUP),
                              (uint32_t)(app_bt_device.sample_rate[device_id] &
                                         A2D_STREAM_SAMP_FREQ_MSK));
        app_audio_sendrequest_param(APP_BT_STREAM_A2DP_SBC,
                                    (uint8_t)APP_BT_SETTING_RESTART, 0,
                                    APP_SYSFREQ_104M);
        return;
      }
#endif
      ////if curr active media is not sbc,wrong~~
      if (0 == (bt_media_get_current_media() & BT_STREAM_SBC)) {
        ASSERT(0, "curr_active_media is wrong!");
      }
      /// stop the old audio sbc and start the new audio sbc
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
    }
#endif
    else {
      // start audio sbc stream
      if (media_pre_sbc != bt_meida.media_curr_sbc) {
        app_audio_manager_switch_a2dp(device_id);
        bt_media_set_current_media(BT_STREAM_SBC);
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)(APP_BT_SETTING_SETUP),
                              (uint32_t)(app_bt_device.sample_rate[device_id] &
                                         A2D_STREAM_SAMP_FREQ_MSK));
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
      }
    }
  } break;

#ifdef MEDIA_PLAYER_SUPPORT
  case BT_STREAM_MEDIA:
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    isMergingPrompt = IS_PROMPT_NEED_MERGING(media_id);
#endif

    if (
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        audio_prompt_is_playing_ongoing() ||
#endif
        app_audio_list_playback_exist()) {
      if (!bt_media_cur_is_bt_stream_media()) {
        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
      }
      media_id = PROMPT_ID_FROM_ID_VALUE(media_id);
      APP_AUDIO_STATUS aud_status;
      aud_status.id = APP_PLAY_BACK_AUDIO;
      aud_status.aud_id = media_id;

      app_audio_list_append(&aud_status);
      break;
    }

#ifdef AUDIO_LINEIN
    if (bt_media_is_media_active_by_type(BT_STREAM_LINEIN)) {
      if (bt_media_get_current_media() & BT_STREAM_LINEIN) {
        APP_AUDIO_STATUS aud_status;
        aud_status.id = media_id;
        app_play_audio_lineinmode_start(&aud_status);
        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
      }
    } else
#endif
        // first,if the voice is active so  mix "dudu" to the stream
        if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      if (bt_media_get_current_media() & BT_STREAM_VOICE) {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        // if the playback is not triggered yet, we just use the stand-alone
        // prompt playing
        if (!bt_is_playback_triggered()) {
          isMergingPrompt = false;
        }
#endif
        // if call is not active so do media report
        if ((btapp_hfp_is_call_active() && !btapp_hfp_incoming_calls()) ||
            (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM))) {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
          bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
          if (isMergingPrompt) {
            audio_prompt_start_playing(media_id, sco_sample_rate);
            goto exit;
          } else {
            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                  (uint8_t)APP_BT_SETTING_CLOSE, 0);
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                  (uint8_t)APP_BT_SETTING_OPEN, media_id);
            bt_media_set_current_media(BT_STREAM_MEDIA);
          }
#else
          // in three way call merge "dudu"
          TRACE_AUD_MGR_I(
              "[HANDLE_START] BT_STREAM_VOICE-->app_ring_merge_start\n");
          app_ring_merge_start();
          // meida is done here
          bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
#endif
        } else {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
          bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
          if (isMergingPrompt) {
            audio_prompt_start_playing(media_id, sco_sample_rate);
            goto exit;
          } else {
            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                  (uint8_t)APP_BT_SETTING_CLOSE, 0);
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                  (uint8_t)APP_BT_SETTING_OPEN, media_id);
            bt_media_set_current_media(BT_STREAM_MEDIA);
          }
#else
          TRACE_AUD_MGR_I("[HANDLE_START] stop sco and do media report\n");
          bt_media_set_current_media(BT_STREAM_MEDIA);
          app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                (uint8_t)APP_BT_SETTING_CLOSE, 0);
          app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                (uint8_t)APP_BT_SETTING_OPEN, media_id);
#endif
        }
      } else if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
        bt_media_set_current_media(BT_STREAM_MEDIA);
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                              media_id);
      } else {
        /// if voice is active but current is not voice something is unkown
        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
#ifdef __BT_ONE_BRING_TWO__
        TRACE_AUD_MGR_I(
            "[HANDLE_START] voice_act media_act = %x,%x,curr_media = %x",
            bt_meida.media_active[0], bt_meida.media_active[1],
            bt_meida.curr_active_media);
#else
        TRACE_AUD_MGR_I(
            "[HANDLE_START] voice_act media_act = %x,curr_media = %x",
            bt_meida.media_active[0], bt_meida.curr_active_media);

#endif
      }
    } else if (btapp_hfp_is_call_active()) {
      bt_media_set_current_media(BT_STREAM_MEDIA);
      app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                            media_id);
    }
#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
    else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                              media_id);
      } else {
        goto exit;
      }
    }
#endif
#endif
    ////if sbc active so
    else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      if (bt_media_get_current_media() & BT_STREAM_SBC) {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
        // if the playback is not triggered yet, we just use the stand-alone
        // prompt playing
        if (!bt_is_playback_triggered()) {
          isMergingPrompt = false;
        }

        bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
        if (isMergingPrompt) {
          audio_prompt_start_playing(media_id, a2dp_sample_rate);
          goto exit;
        } else {
          TRACE_AUD_MGR_I("[HANDLE_START] START prompt.");
          app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)APP_BT_SETTING_CLOSE, 0);
          bt_media_set_current_media(BT_STREAM_MEDIA);
          app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                (uint8_t)APP_BT_SETTING_OPEN, media_id);
        }
#else
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
        if (PROMPT_ID_FROM_ID_VALUE(media_id) == AUD_ID_BT_WARNING) {
          TRACE_AUD_MGR_I(
              "[HANDLE_START] BT_STREAM_SBC-->app_ring_merge_start\n");
          app_ring_merge_start();
          // meida is done here
          bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
        } else
#endif
        {
          app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)APP_BT_SETTING_CLOSE, 0);
          bt_media_set_current_media(BT_STREAM_MEDIA);
          app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                (uint8_t)APP_BT_SETTING_OPEN, media_id);
        }
#endif // #ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
      } else if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                              media_id);
      } else if ((bt_media_get_current_media() & 0xFF) == 0) {
        app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                              media_id);
      } else {
        ASSERT(0, "media in sbc  current wrong");
      }
    }
    /// just play the media
    else {
      bt_media_set_current_media(BT_STREAM_MEDIA);
      app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                            media_id);
    }
    break;
#endif
  case BT_STREAM_VOICE:
#ifdef HFP_NO_PRERMPT
    if (app_audio_manager_get_active_sco_num() != BT_DEVICE_NUM)
      return;
#endif
    app_audio_manager_set_active_sco_num(device_id);

#ifdef VOICE_DATAPATH
    if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      if (bt_media_get_current_media() & BT_STREAM_CAPTURE) {
        app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_CAPTURE);
      }
    }
#endif
#ifdef __AI_VOICE__
    if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE)) {
      if (bt_media_get_current_media() & BT_STREAM_AI_VOICE) {
        app_audio_sendrequest(APP_BT_STREAM_AI_VOICE,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_AI_VOICE);
      }
    }
#endif
#ifdef __THIRDPARTY
    if (bt_media_is_media_active_by_type(BT_STREAM_THIRDPARTY_VOICE)) {
      if (bt_media_get_current_media() & BT_STREAM_THIRDPARTY_VOICE) {
        app_audio_sendrequest(APP_BT_STREAM_THIRDPARTY_VOICE,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
        bt_media_clear_current_media(BT_STREAM_THIRDPARTY_VOICE);
      }
    }
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    /// clear the pending stop flag if it is set
    audio_prompt_clear_pending_stream(PENDING_TO_STOP_A2DP_STREAMING);
#endif

    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      // if call is active ,so disable media report
      if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
        if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
          if (app_play_audio_get_aud_id() == AUD_ID_BT_CALL_INCOMING_NUMBER) {
            // if meida is open ,close media clear all media type
            TRACE_AUD_MGR_I("[HANDLE_START] call active so start sco and stop "
                            "media report\n");
#ifdef __AUDIO_QUEUE_SUPPORT__
            app_audio_list_clear();
#endif
            app_audio_sendrequest(APP_PLAY_BACK_AUDIO,
                                  (uint8_t)APP_BT_SETTING_CLOSE, 0);
            bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
            bt_media_set_current_media(BT_STREAM_VOICE);
            app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                  (uint8_t)APP_BT_SETTING_OPEN, 0);
          }
        }
      } else {
        ////call is not active so media report continue
      }
    } else
#endif
        if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      /// if sbc is open  stop sbc
      if (bt_media_get_current_media() & BT_STREAM_SBC) {
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
      }
      ////start voice stream
      bt_media_set_current_media(BT_STREAM_VOICE);
      app_audio_sendrequest_param(APP_BT_STREAM_HFP_PCM,
                                  (uint8_t)APP_BT_SETTING_OPEN, 0,
                                  APP_SYSFREQ_104M);
    } else {
      // voice is open already so do nothing
      if (bt_media_get_current_media() & BT_STREAM_VOICE) {
#if defined(__BT_ONE_BRING_TWO__)
        if (bt_get_sco_number() > 1
#ifdef CHIP_BEST1000
            && hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2
#endif
        ) {
          app_audio_manager_swap_sco(device_id);
#if defined(__HF_KEEP_ONE_ALIVE__)
          if (btif_hf_check_AudioConnect_status(
                  app_bt_device.hf_channel[other_device_id])) {
            TRACE_AUD_MGR_I("[MEDIA_START] Disconnect another AudioLink");
            btif_hf_disc_audio_link(app_bt_device.hf_channel[other_device_id]);
          }
#endif
        }
#endif
      } else {
        bt_media_set_current_media(BT_STREAM_VOICE);
        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
      }
    }

    break;
#ifdef AUDIO_LINEIN
  case BT_STREAM_LINEIN:
    if (!bt_media_is_media_active_by_type(BT_STREAM_SBC | BT_STREAM_MEDIA |
                                          BT_STREAM_VOICE)) {
      app_audio_sendrequest(APP_PLAY_LINEIN_AUDIO, (uint8_t)APP_BT_SETTING_OPEN,
                            0);
      bt_media_set_current_media(BT_STREAM_LINEIN);
    }
    break;
#endif

  default:
    ASSERT(0, "bt_media_open ERROR TYPE");
    break;
  }

#if defined(RB_CODEC) || defined(VOICE_DATAPATH) ||                            \
    defined(MEDIA_PLAYER_SUPPORT) || defined(__AI_VOICE__)
exit:
  return;
#endif
}

#ifdef RB_CODEC

static bool bt_media_rbcodec_stop_process(uint16_t stream_type,
                                          enum BT_DEVICE_ID_T device_id,
                                          uint32_t ptr) {
  int ret_SendReq2AudioThread = -1;
  bt_media_clear_media_type(stream_type, device_id);
  // if current stream is the stop one ,so stop it
  if (bt_media_get_current_media() & BT_STREAM_RBCODEC) {
    ret_SendReq2AudioThread = app_audio_sendrequest(
        APP_BT_STREAM_RBCODEC, (uint8_t)APP_BT_SETTING_CLOSE, ptr);
    bt_media_clear_current_media(BT_STREAM_RBCODEC);
    TRACE_AUD_MGR_I("[RBCODEC][STOPED]");
  }

  if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
    enum BT_DEVICE_ID_T sbc_id =
        bt_media_get_active_device_by_type(BT_STREAM_SBC);
    TRACE_AUD_MGR_I("[RBCODEC][STOPED] sbc_id %d", sbc_id);
    if (sbc_id < BT_DEVICE_NUM) {
      bt_meida.media_curr_sbc = sbc_id;
    }
  } else {
    bt_meida.media_curr_sbc = BT_DEVICE_NUM;
  }

  TRACE_AUD_MGR_I("[RBCODEC][STOPED] bt_meida.media_curr_sbc %d",
                  bt_meida.media_curr_sbc);

  if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
  } else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
    enum BT_DEVICE_ID_T sbc_id =
        bt_media_get_active_device_by_type(BT_STREAM_SBC);
    if (sbc_id < BT_DEVICE_NUM) {
#ifdef __TWS__
      bt_media_clear_media_type(BT_STREAM_SBC, sbc_id);
      bt_media_clear_current_media(BT_STREAM_SBC);
      notify_tws_player_status(APP_BT_SETTING_OPEN);
#else
      bt_parse_store_sbc_sample_rate(app_bt_device.sample_rate[sbc_id]);
      ret_SendReq2AudioThread = app_audio_sendrequest(
          APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_SBC);
#endif
    }
  } else if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
    // do nothing
  }
}
#endif

#ifdef VOICE_DATAPATH
bool bt_media_voicepath_stop_process(uint16_t stream_type,
                                     enum BT_DEVICE_ID_T device_id) {
  int ret_SendReq2AudioThread __attribute__((unused));
  ret_SendReq2AudioThread = -1;
  bt_media_clear_media_type(stream_type, device_id);
  // if current stream is the stop one ,so stop it
  if (bt_media_get_current_media() & BT_STREAM_CAPTURE) {
    ret_SendReq2AudioThread = app_audio_sendrequest(
        APP_BT_STREAM_VOICEPATH, (uint8_t)APP_BT_SETTING_CLOSE, 0);
    bt_media_clear_current_media(BT_STREAM_CAPTURE);
    TRACE_AUD_MGR_I("[VOICEPATH][STOPED] Voice Path STOPED! ");
  }

  if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
    enum BT_DEVICE_ID_T sbc_id =
        bt_media_get_active_device_by_type(BT_STREAM_SBC);
    TRACE_AUD_MGR_I("[VOICEPATH][STOPED] sbc_id %d", sbc_id);
    if (sbc_id < BT_DEVICE_NUM) {
      bt_meida.media_curr_sbc = sbc_id;
    }
  } else {
    bt_meida.media_curr_sbc = BT_DEVICE_NUM;
  }

  TRACE_AUD_MGR_I("[VOICEPATH][STOPED] bt_meida.media_curr_sbc %d",
                  bt_meida.media_curr_sbc);

  if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
  }
#if !ISOLATED_AUDIO_STREAM_ENABLED
  else if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
    app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, 0);
    bt_media_set_current_media(BT_STREAM_MEDIA);
  } else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
    enum BT_DEVICE_ID_T sbc_id =
        bt_media_get_active_device_by_type(BT_STREAM_SBC);
    if (sbc_id < BT_DEVICE_NUM) {
#ifdef __TWS__
      bt_media_clear_media_type(BT_STREAM_SBC, sbc_id);
      bt_media_clear_current_media(BT_STREAM_SBC);
      notify_tws_player_status(APP_BT_SETTING_OPEN);
#else
      // bt_parse_store_sbc_sample_rate(app_bt_device.sample_rate[sbc_id]);
      ret_SendReq2AudioThread = app_audio_sendrequest(
          APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_SBC);
#endif
    }
  }
#endif
  return true;
}
#endif

#ifdef __AI_VOICE__
bool bt_media_ai_voice_stop_process(uint16_t stream_type,
                                    enum BT_DEVICE_ID_T device_id) {
  bt_media_clear_media_type(BT_STREAM_AI_VOICE, device_id);
  // if current stream is the stop one ,stop it
  if (bt_media_get_current_media() & BT_STREAM_AI_VOICE) {
    app_audio_sendrequest(APP_BT_STREAM_AI_VOICE, (uint8_t)APP_BT_SETTING_CLOSE,
                          0);
    bt_media_clear_current_media(BT_STREAM_AI_VOICE);
    TRACE_AUD_MGR_I("[AI_VOICE][STOPED]");

#ifdef IBRT
    if (!bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      app_ibrt_if_force_audio_retrigger();
    }
#endif
  }

#ifndef IBRT
  enum BT_DEVICE_ID_T sbc_id = BT_DEVICE_NUM;
  sbc_id = bt_media_get_active_device_by_type(BT_STREAM_SBC);
  TRACE_AUD_MGR_I("[AI_VOICE][STOPED] sbc_id %d", sbc_id);
  bt_meida.media_curr_sbc = sbc_id;

  TRACE_AUD_MGR_I("[AI_VOICE][STOPED] bt_meida.media_curr_sbc %d",
                  bt_meida.media_curr_sbc);

  if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
  } else if (sbc_id < BT_DEVICE_NUM) {
    if (!(bt_media_get_current_media() & BT_STREAM_SBC)) {
      // bt_parse_store_sbc_sample_rate(app_bt_device.sample_rate[sbc_id]);
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)(APP_BT_SETTING_SETUP),
                            (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                       A2D_STREAM_SAMP_FREQ_MSK));
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_SBC);
    }
  }
#endif
  return true;
}
#endif

#ifdef __THIRDPARTY
bool bt_media_thirdparty_voice_stop_process(uint16_t stream_type,
                                            enum BT_DEVICE_ID_T device_id) {
  bt_media_clear_media_type(BT_STREAM_THIRDPARTY_VOICE, device_id);
  // if current stream is the stop one ,stop it
  if (bt_media_get_current_media() & BT_STREAM_THIRDPARTY_VOICE) {
    app_audio_sendrequest(APP_BT_STREAM_THIRDPARTY_VOICE,
                          (uint8_t)APP_BT_SETTING_CLOSE, 0);
    bt_media_clear_current_media(BT_STREAM_THIRDPARTY_VOICE);
    TRACE_AUD_MGR_I("THIRDPARTY VOICE STOPED! ");
  }
  return true;
}
#endif

/*
   bt_media_stop function is called to stop media by app or media play callback
   sbc is just stop by a2dp stream suspend or close
   voice is just stop by hfp audio disconnect
   media is stop by media player finished call back

*/
void bt_media_stop(uint16_t stream_type, enum BT_DEVICE_ID_T device_id) {
  TRACE_AUD_MGR_I("[HANDLE_STOP] type= 0x%x->:%s, device id = 0x%x",
                  stream_type, strmtype2str(stream_type), device_id);
#ifdef __BT_ONE_BRING_TWO__
  TRACE_AUD_MGR_I("[HANDLE_STOP] media_active[0] = 0x%x->:%s",
                  bt_meida.media_active[0],
                  strmtype2str(bt_meida.media_active[0]));
  TRACE_AUD_MGR_I("[HANDLE_STOP] media_active[1] = 0x%x->:%s",
                  bt_meida.media_active[1],
                  strmtype2str(bt_meida.media_active[1]));
  TRACE_AUD_MGR_I("[HANDLE_STOP] curr_active_media = 0x%x->:%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
#else
  TRACE_AUD_MGR_I("[HANDLE_STOP] media_active = 0x%x->:%s",
                  bt_meida.media_active[0],
                  strmtype2str(bt_meida.media_active[0]));
  TRACE_AUD_MGR_I("[HANDLE_STOP] curr_active_media = 0x%x->:%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
#endif

  if (!bt_media_is_media_active_by_device(stream_type, device_id) &&
      !(bt_media_get_current_media() & stream_type)) {
    return;
  }

  switch (stream_type) {
#ifdef VOICE_DATAPATH
  case BT_STREAM_CAPTURE:
    bt_media_voicepath_stop_process(stream_type, device_id);
    break;
#endif
#ifdef __AI_VOICE__
  case BT_STREAM_AI_VOICE:
    bt_media_ai_voice_stop_process(stream_type, device_id);
    break;
#endif
#ifdef __THIRDPARTY
  case BT_STREAM_THIRDPARTY_VOICE:
    bt_media_thirdparty_voice_stop_process(stream_type, device_id);
    break;
#endif

  case BT_STREAM_SBC: {
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    if (!audio_prompt_check_on_stopping_stream(PENDING_TO_STOP_A2DP_STREAMING,
                                               device_id)) {
      TRACE_AUD_MGR_I("[HANDLE_STOP] Pending stop BT_STREAM_SBC");
      return;
    }

#else
    if (app_ring_merge_isrun()) {
      TRACE_AUD_MGR_I("[HANDLE_STOP] pending BT_STREAM_SBC");
      app_ring_merge_save_pending_start_stream_op(
          PENDING_TO_STOP_A2DP_STREAMING, device_id);
      return;
    }
#endif

    uint8_t media_pre_sbc = bt_meida.media_curr_sbc;
    TRACE_AUD_MGR_I("[HANDLE_STOP] SBC STOPPING id:%d/%d",
                    bt_meida.media_curr_sbc, device_id);

    ////if current media is sbc ,stop the sbc streaming
    bt_media_clear_media_type(stream_type, device_id);

    // if current stream is the stop one ,so stop it
    if ((bt_media_get_current_media() & BT_STREAM_SBC)
#if !defined(IBRT)
        && bt_meida.media_curr_sbc == device_id
#endif
    ) {
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
      bt_media_clear_current_media(BT_STREAM_SBC);
      TRACE_AUD_MGR_I("[HANDLE_STOP] SBC STOPED!");
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      enum BT_DEVICE_ID_T sbc_id =
          bt_media_get_active_device_by_type(BT_STREAM_SBC);
      if (sbc_id < BT_DEVICE_NUM) {
        bt_meida.media_curr_sbc = sbc_id;
      }
    } else {
      bt_meida.media_curr_sbc = BT_DEVICE_NUM;
    }

    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {

    } else if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      // do nothing
    }
#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
    else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      // do nothing
    }
#endif
#endif
    else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      enum BT_DEVICE_ID_T sbc_id =
          bt_media_get_active_device_by_type(BT_STREAM_SBC);
      if (sbc_id < BT_DEVICE_NUM &&
          (media_pre_sbc != bt_meida.media_curr_sbc)) {
        app_audio_manager_switch_a2dp(sbc_id);
        bt_media_set_current_media(BT_STREAM_SBC);
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)(APP_BT_SETTING_SETUP),
                              (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                         A2D_STREAM_SAMP_FREQ_MSK));
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
      }
    }
  } break;
#ifdef MEDIA_PLAYER_SUPPORT
  case BT_STREAM_MEDIA:
    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);

    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      // also have media report so do nothing
    } else if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      if (bt_media_get_current_media() & BT_STREAM_VOICE) {
        // do nothing
      } else if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
        /// media report is end ,so goto voice
        uint8_t curr_sco_id;
        curr_sco_id = app_audio_manager_get_active_sco_num();
        if (curr_sco_id != BT_DEVICE_NUM) {
          bt_media_set_media_type(BT_STREAM_VOICE,
                                  (enum BT_DEVICE_ID_T)curr_sco_id);
          bt_media_set_current_media(BT_STREAM_VOICE);
#ifdef __BT_ONE_BRING_TWO__
          app_audio_manager_swap_sco((enum BT_DEVICE_ID_T)curr_sco_id);
#endif
          app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                (uint8_t)APP_BT_SETTING_OPEN, 0);
        }
      }

    } else if (btapp_hfp_is_call_active()) {
      // do nothing
    }

#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
    else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_CAPTURE);
    }
#endif
#endif
#ifdef __AI_VOICE__
    else if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) ||
             ai_if_is_ai_stream_mic_open()) {
      bt_media_clear_current_media(BT_STREAM_MEDIA);
      if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) &&
          !(bt_media_get_current_media() & BT_STREAM_AI_VOICE)) {
        app_audio_sendrequest(APP_BT_STREAM_AI_VOICE,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
        bt_media_set_current_media(BT_STREAM_AI_VOICE);
      }
    }
#endif
    else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      /// if another device is also in sbc mode
      enum BT_DEVICE_ID_T sbc_id = bt_media_get_active_sbc_device();
      bt_media_set_media_type(BT_STREAM_SBC, sbc_id);
      app_audio_manager_switch_a2dp(sbc_id);
      bt_media_set_current_media(BT_STREAM_SBC);

      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)(APP_BT_SETTING_SETUP),
                            (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                       A2D_STREAM_SAMP_FREQ_MSK));

      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
    } else {
      // have no meida task,so goto idle
      bt_media_set_current_media(0);
    }
    break;
#endif
  case BT_STREAM_VOICE:

    if (!bt_media_is_media_active_by_device(BT_STREAM_VOICE, device_id) ||
        !(bt_media_get_current_media() & BT_STREAM_VOICE)) {
      TRACE_AUD_MGR_I("[HANDLE_STOP] bt_media_stop already stop");
      bt_media_clear_media_type(stream_type, device_id);
      return;
    }

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    if (!audio_prompt_check_on_stopping_stream(PENDING_TO_STOP_SCO_STREAMING,
                                               device_id)) {
      return;
    }

#else
    if (app_ring_merge_isrun()) {
      TRACE_AUD_MGR_I("[HANDLE_STOP] bt_media_stop pending BT_STREAM_VOICE");
      app_ring_merge_save_pending_start_stream_op(PENDING_TO_STOP_SCO_STREAMING,
                                                  device_id);
      return;
    }
#endif

    app_audio_manager_set_active_sco_num(BT_DEVICE_NUM);
    bt_media_clear_media_type(stream_type, device_id);

#ifdef MEDIA_PLAYER_SUPPORT
    if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
      if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
        // do nothing
      }

#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
      else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
        bt_media_clear_all_media_type();
        app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
        bt_media_set_current_media(BT_STREAM_CAPTURE);
      }
#endif
#endif
      else if (bt_media_get_current_media() & BT_STREAM_VOICE) {
        TRACE_AUD_MGR_W("[HANDLE_STOP] voice and media is all on,media should "
                        "be the current media");
        if (!bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
          app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                                (uint8_t)APP_BT_SETTING_CLOSE, 0);
        }
      } else if (bt_media_get_current_media() & BT_STREAM_SBC) {
        TRACE_AUD_MGR_W("[HANDLE_STOP] voice and media is all on,media should "
                        "be the current media");
        if (!bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
          app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)APP_BT_SETTING_CLOSE, 0);
        }
      }
    } else
#endif
        if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
#ifdef __BT_ONE_BRING_TWO__
      if (bt_get_sco_number() > 1
#ifdef CHIP_BEST1000
          && hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2
#endif
      ) {
        TRACE_AUD_MGR_W(
            "[HANDLE_STOP] bt_media_stop, but another soc need connect");
        enum BT_DEVICE_ID_T voice_dev_id =
            bt_media_get_active_device_by_type(BT_STREAM_VOICE);
#ifdef HFP_NO_PRERMPT
        app_audio_manager_set_active_sco_num(voice_dev_id);
#else
        app_audio_manager_swap_sco(voice_dev_id);
#ifdef __HF_KEEP_ONE_ALIVE__
        enum BT_DEVICE_ID_T other_voice_dev_id =
            (voice_dev_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
        ;
        if (btif_hf_check_AudioConnect_status(
                app_bt_device.hf_channel[other_voice_dev_id])) {
          TRACE_AUD_MGR_I("[HANDLE_STOP] Disconnect another AudioLink");
          app_bt_HF_DisconnectAudioLink(
              app_bt_device.hf_channel[other_voice_dev_id]);
        }
#endif
#endif
      }
      bt_media_set_current_media(BT_STREAM_VOICE);
#endif
    } else if (btapp_hfp_is_call_active()) {
      TRACE_AUD_MGR_I("[HANDLE_STOP] stop in HF_CALL_ACTIVE and no sco need");
      bt_media_set_current_media(0);
      app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
      bt_media_clear_media_type(BT_STREAM_VOICE, device_id);
      {
        enum BT_DEVICE_ID_T sbc_id = bt_media_get_active_sbc_device();
        if (sbc_id < BT_DEVICE_NUM) {
          app_audio_manager_switch_a2dp(sbc_id);
          bt_media_set_current_media(BT_STREAM_SBC);
          app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)(APP_BT_SETTING_SETUP),
                                (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                           A2D_STREAM_SAMP_FREQ_MSK));
          app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                                (uint8_t)APP_BT_SETTING_OPEN, 0);
        }
      }
    }
#ifdef VOICE_DATAPATH
#if !ISOLATED_AUDIO_STREAM_ENABLED
    else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
      app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_CAPTURE);
    }
#endif
#endif
    else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      /// if another device is also in sbc mode
      enum BT_DEVICE_ID_T sbc_id = bt_media_get_active_sbc_device();
      if (sbc_id < BT_DEVICE_NUM /*&& device_id == sbc_id*/) {
        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                              (uint8_t)APP_BT_SETTING_CLOSE, 0);
        app_audio_manager_switch_a2dp(sbc_id);
        bt_media_set_current_media(BT_STREAM_SBC);
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)(APP_BT_SETTING_SETUP),
                              (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                         A2D_STREAM_SAMP_FREQ_MSK));
        app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
      }
    } else {
      bt_media_set_current_media(0);
      app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
    }

#if ISOLATED_AUDIO_STREAM_ENABLED
#ifdef VOICE_DATAPATH
    if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_CAPTURE);
    }
#endif
#endif
#ifdef __THIRDPARTY
    if (bt_media_is_media_active_by_type(BT_STREAM_THIRDPARTY_VOICE)) {
      app_audio_sendrequest(APP_BT_STREAM_THIRDPARTY_VOICE,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_THIRDPARTY_VOICE);
    }
#endif
    break;
#ifdef RB_CODEC
  case BT_STREAM_RBCODEC:
    bt_media_rbcodec_stop_process(stream_type, device_id, 0);
    break;
#endif
#ifdef AUDIO_LINEIN
  case BT_STREAM_LINEIN:
    if (bt_media_is_media_active_by_type(BT_STREAM_LINEIN)) {
      app_audio_sendrequest(APP_PLAY_LINEIN_AUDIO,
                            (uint8_t)APP_BT_SETTING_CLOSE, 0);
      if (bt_media_get_current_media() & BT_STREAM_LINEIN)
        bt_media_set_current_media(0);

      bt_media_clear_media_type(stream_type, device_id);
    }
    break;
#endif
  default:
    ASSERT(0, "[HANDLE_STOP] ERROR TYPE: %x", stream_type);
    break;
  }
#ifdef __BT_ONE_BRING_TWO__
  TRACE_AUD_MGR_I(
      "[HANDLE_STOP] end media_active = %x,%x,curr_active_media = %x",
      bt_meida.media_active[0], bt_meida.media_active[1],
      bt_meida.curr_active_media);
#else
  TRACE_AUD_MGR_I("[HANDLE_STOP] end media_active = %x%s",
                  bt_meida.media_active[0],
                  strmtype2str(bt_meida.media_active[0]));
  TRACE_AUD_MGR_I("[HANDLE_STOP] end curr_active_media = %x%s",
                  bt_meida.curr_active_media,
                  strmtype2str(bt_meida.curr_active_media));
#endif
}

void app_media_stop_media(uint16_t stream_type, enum BT_DEVICE_ID_T device_id) {
#ifdef MEDIA_PLAYER_SUPPORT
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  audio_prompt_stop_playing();
#endif

  if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA) ||
      app_audio_list_playback_exist()) {
#ifdef __AUDIO_QUEUE_SUPPORT__
    ////should have no sbc
    app_audio_list_clear();
#endif
    if (bt_media_get_current_media() & BT_STREAM_MEDIA) {
      TRACE_AUD_MGR_I("[STOP_MEDIA]");
      // bt_media_set_current_media(0);
      app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,
                            0);
    }
    bt_media_clear_media_type(BT_STREAM_MEDIA, device_id);
    if (bt_media_is_media_active_by_type(BT_STREAM_VOICE)) {
      enum BT_DEVICE_ID_T currScoId = BT_DEVICE_NUM;
      currScoId = (enum BT_DEVICE_ID_T)app_audio_manager_get_active_sco_num();

      if (currScoId == BT_DEVICE_NUM) {
        for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
          if (bt_media_is_media_active_by_device(BT_STREAM_VOICE,
                                                 (enum BT_DEVICE_ID_T)i)) {
            currScoId = device_id;
            break;
          }
        }
      }

      TRACE_AUD_MGR_I("[STOP_MEDIA] try to resume sco:%d", currScoId);
      if (currScoId != BT_DEVICE_NUM) {
        bt_media_set_media_type(BT_STREAM_VOICE, currScoId);
        bt_media_set_current_media(BT_STREAM_VOICE);
#ifdef __BT_ONE_BRING_TWO__
        app_audio_manager_swap_sco(currScoId);
#endif
        app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
      }
    }
#if !ISOLATED_AUDIO_STREAM_ENABLED
#ifdef VOICE_DATAPATH
    else if (bt_media_is_media_active_by_type(BT_STREAM_CAPTURE)) {
      app_audio_sendrequest(APP_BT_STREAM_VOICEPATH,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
      bt_media_set_current_media(BT_STREAM_CAPTURE);
    }
#endif
#endif
#ifdef __AI_VOICE__
    else if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE)) {
      if ((bt_media_get_current_media() & BT_STREAM_AI_VOICE) == 0) {
        app_audio_sendrequest(APP_BT_STREAM_AI_VOICE,
                              (uint8_t)APP_BT_SETTING_OPEN, 0);
        bt_media_set_current_media(BT_STREAM_AI_VOICE);
      }
    }
#endif
    else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
      enum BT_DEVICE_ID_T sbc_id = bt_media_get_active_sbc_device();
      bt_media_set_media_type(BT_STREAM_SBC, device_id);
      app_audio_manager_switch_a2dp(sbc_id);
      bt_media_set_current_media(BT_STREAM_SBC);
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)(APP_BT_SETTING_SETUP),
                            (uint32_t)(app_bt_device.sample_rate[sbc_id] &
                                       A2D_STREAM_SAMP_FREQ_MSK));
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
    }
  }
#endif
}

void bt_media_switch_to_voice(uint16_t stream_type,
                              enum BT_DEVICE_ID_T device_id) {
  TRACE_AUD_MGR_I("[SW2VOICE] stream_type= 0x%x,device_id=0x%x ", stream_type,
                  device_id);
#ifdef __BT_ONE_BRING_TWO__
  TRACE_AUD_MGR_I(
      "[SW2VOICE] media_active = 0x%x,0x%x,curr_active_media = 0x%x",
      bt_meida.media_active[0], bt_meida.media_active[1],
      bt_meida.curr_active_media);
#else
  TRACE_AUD_MGR_I("[SW2VOICE] media_active = 0x%x,curr_active_media = 0x%x",
                  bt_meida.media_active[0], bt_meida.curr_active_media);

#endif

  /// already in voice ,so return
  if (bt_media_get_current_media() & BT_STREAM_VOICE) {
#ifndef HFP_NO_PRERMPT
#ifdef __BT_ONE_BRING_TWO__
    if (!bt_media_is_media_active_by_device(BT_STREAM_VOICE, device_id)) {
      app_audio_manager_swap_sco(device_id);
    }
#ifdef __HF_KEEP_ONE_ALIVE__
    enum BT_DEVICE_ID_T other_device_id =
        (device_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
    if (btif_hf_check_AudioConnect_status(
            app_bt_device.hf_channel[other_device_id])) {
      TRACE_AUD_MGR_I("[SW2VOICE] Disconnect another AudioLink");
      btif_hf_disc_audio_link(app_bt_device.hf_channel[other_device_id]);
    }
#endif
#endif
#endif
    return;
  }
  app_media_stop_media(stream_type, device_id);
}

void app_media_update_media(uint16_t stream_type,
                            enum BT_DEVICE_ID_T device_id) {
  TRACE_AUD_MGR_I("[UPDATE_MEDIA]");

#ifdef MEDIA_PLAYER_SUPPORT
  if (bt_media_is_media_active_by_type(BT_STREAM_MEDIA)) {
    // do nothing
    TRACE_AUD_MGR_I("[UPDATE_MEDIA] skip BT_STREAM_MEDIA");
  } else
#endif
      if (bt_media_is_media_active_by_type(BT_STREAM_VOICE) ||
          btapp_hfp_is_call_active()) {
    // do nothing
    TRACE_AUD_MGR_I("[UPDATE_MEDIA] skip BT_STREAM_VOICE");
    TRACE_AUD_MGR_I("[UPDATE_MEDIA] DEBUG INFO actByVoc:%d %d %d",
                    bt_media_is_media_active_by_type(BT_STREAM_VOICE),
                    btapp_hfp_is_call_active(), btapp_hfp_incoming_calls());
  } else if (bt_media_is_media_active_by_type(BT_STREAM_SBC)) {
#ifdef __AI_VOICE__
    if (bt_media_is_media_active_by_type(BT_STREAM_AI_VOICE) ||
        ai_if_is_ai_stream_mic_open()) {
      TRACE_AUD_MGR_I("[UPDATE_MEDIA] there is a ai voice stream exist, skip");
      return;
    }
#endif
    /// if another device is also in sbc mode
    TRACE_AUD_MGR_I("[UPDATE_MEDIA] try to resume sbc");
    enum BT_DEVICE_ID_T sbc_id = bt_media_get_active_sbc_device();
    if (0 == (bt_media_get_current_media() & BT_STREAM_SBC)) {
      app_audio_manager_switch_a2dp(sbc_id);
      bt_media_set_current_media(BT_STREAM_SBC);
      app_audio_sendrequest(
          APP_BT_STREAM_A2DP_SBC, (uint8_t)(APP_BT_SETTING_SETUP),
          (uint32_t)(app_bt_device
                         .sample_rate[sbc_id < BT_DEVICE_NUM ? sbc_id
                                                             : BT_DEVICE_ID_1] &
                     A2D_STREAM_SAMP_FREQ_MSK));
      app_audio_sendrequest(APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)APP_BT_SETTING_OPEN, 0);
    }
  } else {
    TRACE_AUD_MGR_I("[UPDATE_MEDIA] skip idle");
  }
}

int app_audio_manager_sco_status_checker(void) {
  btif_cmgr_handler_t *cmgrHandler __attribute__((unused)) = NULL;

  TRACE_AUD_MGR_I("[SCO_CHK] enter");
#if defined(CHIP_BEST1000) || defined(CHIP_BEST2000)
  uint32_t scoTransmissionInterval_reg;
  bt_bdaddr_t bdaddr;
  if (bt_meida.media_curr_sco != BT_DEVICE_NUM) {
    BTDIGITAL_REG_GET_FIELD(0xD0220120, 0xff, 24, scoTransmissionInterval_reg);
    // cmgrHandler =
    // &app_bt_device.hf_channel[bt_meida.media_curr_sco].cmgrHandler;
    cmgrHandler = (btif_cmgr_handler_t *)btif_hf_get_chan_manager_handler(
        app_bt_device.hf_channel[bt_meida.media_curr_sco]);
    if (btif_cmgr_is_audio_up(cmgrHandler)) {
      if (btif_cmgr_get_sco_connect_sco_link_type(cmgrHandler) ==
          BTIF_BLT_ESCO) {
        if (btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(
                cmgrHandler) != scoTransmissionInterval_reg) {
          BTDIGITAL_REG_SET_FIELD(
              0xD0220120, 0xff, 24,
              btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(
                  cmgrHandler));
        }
      }
    }
    TRACE_AUD_MGR_I(
        "[SCO_CHK] curr:%d type:%d Intv:%d Intv_reg:%d",
        bt_meida.media_curr_sco,
        btif_cmgr_get_sco_connect_sco_link_type(cmgrHandler),
        btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(
            cmgrHandler),
        scoTransmissionInterval_reg);

    // DUMP8("%02x ",
    // app_bt_device.hf_channel[bt_meida.media_curr_sco].cmgrHandler.remDev->bdAddr.addr,
    // 6);
    btif_hf_get_remote_bdaddr(app_bt_device.hf_channel[bt_meida.media_curr_sco],
                              &bdaddr);
    DUMP8("%02x ", bdaddr.address, 6);
#if defined(HFP_1_6_ENABLE)
    uint32_t code_type;
    uint32_t code_type_reg;
    code_type = app_audio_manager_get_scocodecid();
    code_type_reg = BTDIGITAL_REG(0xD0222000);
    if (code_type == BTIF_HF_SCO_CODEC_MSBC) {
      BTDIGITAL_REG(0xD0222000) = (code_type_reg & (~(7 << 1))) | (3 << 1);
      TRACE_AUD_MGR_I("[SCO_CHK] MSBC REG:0xD0222000=0x%08x B:%d",
                      BTDIGITAL_REG(0xD0222000),
                      (BTDIGITAL_REG(0xD0222000) >> 15) & 1);

    } else {
      BTDIGITAL_REG(0xD0222000) = (code_type_reg & (~(7 << 1))) | (2 << 1);
      TRACE_AUD_MGR_I("[SCO_CHK] CVSD REG:0xD0222000=0x%08x B:%d",
                      BTDIGITAL_REG(0xD0222000),
                      (BTDIGITAL_REG(0xD0222000) >> 15) & 1);
    }
#else
    uint32_t code_type_reg;
    code_type_reg = BTDIGITAL_REG(0xD0222000);
    BTDIGITAL_REG(0xD0222000) = (code_type_reg & (~(7 << 1))) | (2 << 1);
    TRACE_AUD_MGR_I("[SCO_CHK] CVSD REG:0xD0222000=0x%08x B:%d",
                    BTDIGITAL_REG(0xD0222000),
                    (BTDIGITAL_REG(0xD0222000) >> 15) & 1);
#endif
  }
#else
#if defined(DEBUG)
#if defined(IBRT)
  if (app_tws_ibrt_mobile_link_connected())
#endif
  {
    hf_chan_handle_t hf_chan =
        app_bt_device.hf_channel[bt_meida.media_curr_sco];
    if (hf_chan && (cmgrHandler = btif_hf_get_chan_manager_handler(hf_chan))) {
      TRACE_AUD_MGR_I(
          "[SCO_CHK] curSco:%d type:%d Interval:%d", bt_meida.media_curr_sco,
          btif_cmgr_get_sco_connect_sco_link_type(cmgrHandler),
          btif_cmgr_get_sco_connect_sco_rx_parms_sco_transmission_interval(
              cmgrHandler));
    }
  }
#endif
#endif
  TRACE_AUD_MGR_I("[SCO_CHK] exit");
  return 0;
}

int app_audio_manager_swap_sco(enum BT_DEVICE_ID_T id) {
  if (bt_get_sco_number() <= 1)
    return 0;
#if defined(HFP_1_6_ENABLE)
  uint8_t curr_sco_id;
  curr_sco_id = app_audio_manager_get_active_sco_num();
#endif
  bt_bdaddr_t bdAdd;
  uint16_t scohandle;
  // if (HF_GetRemoteBDAddr(&app_bt_device.hf_channel[id], &bdAdd)){
  if (btif_hf_get_remote_bdaddr(app_bt_device.hf_channel[id], &bdAdd)) {
    // TRACE_AUD_MGR_I("[SCO SWAP]switch_sco to id:%d sco:%x", id,
    // app_bt_device.hf_channel[id].cmgrHandler.scoConnect);

    // DUMP8("%02x ",
    // app_bt_device.hf_channel[id].cmgrHandler.remDev->bdAddr.addr, 6);
    // DUMP8("%02x ", bdAdd.address, 6);

    /*
    TRACE_AUD_MGR_I("[SCO_SWAP] state:%d type:%d hdl:%x ",
    app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scostate,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoLinkType,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoHciHandle);
    TRACE_AUD_MGR_I("[SCO_SWAP] tx[bw:%d rbw:%d ml:%d vs:%d rt:%d pktyp:%d]",
    app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.transmitBandwidth,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.receiveBandwidth,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.maxLatency,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.voiceSetting,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.retransmissionEffort,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoTxParms.eScoPktType);

    TRACE_AUD_MGR_I("[SCO_SWAP] rx[itv:%d ret:%d rxl:%d txl:%d]",
    app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoRxParms.scoTransmissionInterval,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoRxParms.scoRetransmissionWindow,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoRxParms.scoRxPacketLen,
            app_bt_device.hf_channel[id].cmgrHandler.scoConnect->scoRxParms.scoTxPacketLen);
            */

    app_audio_manager_set_active_sco_num(id);
    scohandle = btif_hf_get_sco_hcihandle(app_bt_device.hf_channel[id]);
    if (scohandle != BTIF_HCI_INVALID_HANDLE) {
      app_bt_Me_switch_sco(scohandle);
    }
    app_bt_stream_volume_ptr_update(bdAdd.address);
    app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_SET,
                                  app_bt_stream_volume_get_ptr()->hfp_vol);
#if defined(HFP_1_6_ENABLE)
    if (curr_sco_id != id) {
      TRACE_AUD_MGR_I("[SCO_SWAP] try restart");
      bt_sco_player_forcemute(true, true);
      app_audio_sendrequest(APP_BT_STREAM_HFP_PCM,
                            (uint8_t)APP_BT_SETTING_RESTART, 0);
    }
#endif
    app_audio_manager_sco_status_checker();
  }
  return 0;
}

int app_audio_manager_ctrl_volume_handle(APP_MESSAGE_BODY *msg_body) {
  enum APP_AUDIO_MANAGER_VOLUME_CTRL_T volume_ctrl;
  uint16_t volume_level = 0;

  volume_ctrl = (enum APP_AUDIO_MANAGER_VOLUME_CTRL_T)msg_body->message_ptr;
  volume_level = (uint16_t)msg_body->message_Param0;

  switch (volume_ctrl) {
  case APP_AUDIO_MANAGER_VOLUME_CTRL_SET:
    app_bt_stream_volumeset(volume_level);
    break;
  case APP_AUDIO_MANAGER_VOLUME_CTRL_UP:
    app_bt_stream_volumeup();
    btapp_hfp_report_speak_gain();
    btapp_a2dp_report_speak_gain();
    break;
  case APP_AUDIO_MANAGER_VOLUME_CTRL_DOWN:
    app_bt_stream_volumedown();
    btapp_hfp_report_speak_gain();
    btapp_a2dp_report_speak_gain();
    break;
  default:
    break;
  }

  return 0;
}

int app_audio_manager_tune_samplerate_ratio_handle(APP_MESSAGE_BODY *msg_body) {
  enum AUD_STREAM_T stream = AUD_STREAM_NUM;
  float ratio = 1.0f;

  stream = (enum AUD_STREAM_T)msg_body->message_ptr;
  ratio = *(float *)&msg_body->message_Param0;

  TRACE_AUD_MGR_I("[TUNE] %d", (int32_t)(ratio * 10000000));
  af_codec_tune(stream, ratio);

  return 0;
}

static bool app_audio_manager_init = false;

int app_audio_manager_sendrequest(uint8_t massage_id, uint16_t stream_type,
                                  uint8_t device_id, uint16_t aud_id) {
  uint32_t audevt;
  uint32_t msg0;
  APP_MESSAGE_BLOCK msg;

  if (app_audio_manager_init == false)
    return -1;

  // only allow prompt playing if powering-off is on-going
  if (app_is_power_off_in_progress()) {
    if ((APP_BT_STREAM_MANAGER_START == massage_id) &&
        (BT_STREAM_MEDIA != stream_type)) {
      return -1;
    }
  }

  msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
  APP_AUDIO_MANAGER_SET_MESSAGE(audevt, massage_id, stream_type);
  APP_AUDIO_MANAGER_SET_MESSAGE0(msg0, device_id, aud_id);
  msg.msg_body.message_id = audevt;
  msg.msg_body.message_ptr = msg0;
  msg.msg_body.message_Param0 = msg0;
  msg.msg_body.message_Param1 = 0;
  msg.msg_body.message_Param2 = 0;
  app_mailbox_put(&msg);

  return 0;
}

int app_audio_manager_sendrequest_need_callback(uint8_t massage_id,
                                                uint16_t stream_type,
                                                uint8_t device_id,
                                                uint16_t aud_id, uint32_t cb,
                                                uint32_t cb_param) {
  uint32_t audevt;
  uint32_t msg0;
  APP_MESSAGE_BLOCK msg;

  if (app_audio_manager_init == false)
    return -1;

  msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
  APP_AUDIO_MANAGER_SET_MESSAGE(audevt, massage_id, stream_type);
  APP_AUDIO_MANAGER_SET_MESSAGE0(msg0, device_id, aud_id);
  msg.msg_body.message_id = audevt;
  msg.msg_body.message_ptr = msg0;
  msg.msg_body.message_Param0 = msg0;
  msg.msg_body.message_Param1 = cb;
  msg.msg_body.message_Param2 = cb_param;
  app_mailbox_put(&msg);

  return 0;
}

int app_audio_manager_ctrl_volume(APP_AUDIO_MANAGER_VOLUME_CTRL_T volume_ctrl,
                                  uint16_t volume_level) {
  uint32_t audevt;
  APP_MESSAGE_BLOCK msg;
  osThreadId currThreadId;

  if (app_audio_manager_init == false)
    return -1;

  msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
  APP_AUDIO_MANAGER_SET_MESSAGE(audevt, APP_BT_STREAM_MANAGER_CTRL_VOLUME, 0);
  msg.msg_body.message_id = audevt;
  msg.msg_body.message_ptr = (uint32_t)volume_ctrl;
  msg.msg_body.message_Param0 = (uint32_t)volume_level;
  msg.msg_body.message_Param1 = 0;
  msg.msg_body.message_Param2 = 0;
  currThreadId = osThreadGetId();
  if (currThreadId == af_thread_tid_get() || currThreadId == app_os_tid_get()) {
    app_audio_manager_ctrl_volume_handle(&msg.msg_body);
  } else {
    app_mailbox_put(&msg);
  }
  return 0;
}

int app_audio_manager_tune_samplerate_ratio(enum AUD_STREAM_T stream,
                                            float ratio) {
  uint32_t audevt;
  APP_MESSAGE_BLOCK msg;
  osThreadId currThreadId;

  if (app_audio_manager_init == false)
    return -1;

  msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
  APP_AUDIO_MANAGER_SET_MESSAGE(audevt,
                                APP_BT_STREAM_MANAGER_TUNE_SAMPLERATE_RATIO, 0);
  msg.msg_body.message_id = audevt;
  msg.msg_body.message_ptr = (uint32_t)stream;
  msg.msg_body.message_Param0 = *(uint32_t *)&ratio;
  msg.msg_body.message_Param1 = 0;
  msg.msg_body.message_Param2 = 0;

  currThreadId = osThreadGetId();
  if (currThreadId == af_thread_tid_get() || currThreadId == app_os_tid_get()) {
    app_audio_manager_tune_samplerate_ratio_handle(&msg.msg_body);
  } else {
    app_mailbox_put(&msg);
  }

  return 0;
}

#if defined(VOICE_DATAPATH) && defined(MIX_MIC_DURING_MUSIC)
static bool app_audio_handle_pre_processing(APP_MESSAGE_BODY *msg_body) {
  uint16_t stream_type;
  APP_AUDIO_MANAGER_GET_STREAM_TYPE(msg_body->message_id, stream_type);

  bool isToResetCaptureStream = false;
  if ((BT_STREAM_SBC == stream_type) || (BT_STREAM_MEDIA == stream_type)) {
    if (app_audio_manager_capture_is_active()) {
      isToResetCaptureStream = true;
    }
  }

  if (isToResetCaptureStream) {
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_CAPTURE,
                                  0, 0);

    APP_MESSAGE_BLOCK msg;
    msg.msg_body = *msg_body;
    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;

    app_mailbox_put(&msg);
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                  BT_STREAM_CAPTURE, 0, 0);

    return false;
  } else {
    return true;
  }
}
#endif

static int app_audio_manager_handle_process(APP_MESSAGE_BODY *msg_body) {
  int nRet = 0;

  APP_AUDIO_MANAGER_MSG_STRUCT aud_manager_msg;
  APP_AUDIO_MANAGER_CALLBACK_T callback_fn = NULL;
  uint32_t callback_param = 0;

  if (app_audio_manager_init == false)
    return -1;

#if defined(VOICE_DATAPATH) && defined(MIX_MIC_DURING_MUSIC)
  bool isContinue = app_audio_handle_pre_processing(msg_body);
  if (!isContinue) {
    return -1;
  }
#endif

  APP_AUDIO_MANAGER_GET_ID(msg_body->message_id, aud_manager_msg.id);
  APP_AUDIO_MANAGER_GET_STREAM_TYPE(msg_body->message_id,
                                    aud_manager_msg.stream_type);
  APP_AUDIO_MANAGER_GET_DEVICE_ID(msg_body->message_Param0,
                                  aud_manager_msg.device_id);
  APP_AUDIO_MANAGER_GET_AUD_ID(msg_body->message_Param0,
                               aud_manager_msg.aud_id);
  APP_AUDIO_MANAGER_GET_CALLBACK((uintptr_t)msg_body->message_Param1,
                                 callback_fn);
  APP_AUDIO_MANAGER_GET_CALLBACK_PARAM(msg_body->message_Param2,
                                       callback_param);

  TRACE_AUD_MGR_I("[handle_process] id %d %s type 0x%x %s", aud_manager_msg.id,
                  handleId2str(aud_manager_msg.id), aud_manager_msg.stream_type,
                  strmtype2str(aud_manager_msg.stream_type));

  switch (aud_manager_msg.id) {
  case APP_BT_STREAM_MANAGER_START:
    bt_media_start(aud_manager_msg.stream_type,
                   (enum BT_DEVICE_ID_T)aud_manager_msg.device_id,
                   aud_manager_msg.aud_id);
    break;
  case APP_BT_STREAM_MANAGER_STOP:
    bt_media_stop(aud_manager_msg.stream_type,
                  (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
    break;
  case APP_BT_STREAM_MANAGER_SWITCHTO_SCO:
    bt_media_switch_to_voice(aud_manager_msg.stream_type,
                             (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
    break;
  case APP_BT_STREAM_MANAGER_STOP_MEDIA:
    app_media_stop_media(aud_manager_msg.stream_type,
                         (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
    break;
  case APP_BT_STREAM_MANAGER_UPDATE_MEDIA:
    app_media_update_media(aud_manager_msg.stream_type,
                           (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
    break;
  case APP_BT_STREAM_MANAGER_SWAP_SCO:
    app_audio_manager_swap_sco((enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
    break;
  case APP_BT_STREAM_MANAGER_CTRL_VOLUME:
    app_audio_manager_ctrl_volume_handle(msg_body);
    break;
  case APP_BT_STREAM_MANAGER_TUNE_SAMPLERATE_RATIO:
    app_audio_manager_tune_samplerate_ratio_handle(msg_body);
    break;
  default:
    break;
  }
  if (callback_fn) {
    callback_fn(aud_manager_msg.id, callback_param);
  }
  return nRet;
}

void bt_media_volume_ptr_update_by_mediatype(uint16_t stream_type) {
  bt_bdaddr_t *bdAdd = NULL;
  bt_bdaddr_t temp;
  btif_remote_device_t *remDev = NULL;
  uint8_t id;

  TRACE_AUD_MGR_I("[VOLUME_PTR] enter");
  if (stream_type & bt_media_get_current_media()) {
    switch (stream_type) {
    case BT_STREAM_SBC:
      id = bt_meida.media_curr_sbc;
      ASSERT(id < BT_DEVICE_NUM, "INVALID_BT_DEVICE_NUM");
      remDev =
          btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[id]);
      if (remDev) {
        bdAdd = btif_me_get_remote_device_bdaddr(remDev);
        app_bt_stream_volume_ptr_update(bdAdd->address);
      }
      break;
    case BT_STREAM_VOICE:
      id = app_audio_manager_get_active_sco_num();
      ASSERT(id < BT_DEVICE_NUM, "INVALID_BT_DEVICE_NUM");
      // if (HF_GetRemoteBDAddr(&app_bt_device.hf_channel[id], &bdAdd)){
      bdAdd = &temp;
      if (btif_hf_get_remote_bdaddr(app_bt_device.hf_channel[id], bdAdd)) {
        app_bt_stream_volume_ptr_update(bdAdd->address);
      }
      break;
    case BT_STREAM_MEDIA:
    default:
      break;
    }
  }
  TRACE_AUD_MGR_I("[VOLUME_PTR] exit");
}

int app_audio_manager_set_active_sco_num(enum BT_DEVICE_ID_T id) {
  bt_meida.media_curr_sco = id;
  return 0;
}

int app_audio_manager_get_active_sco_num(void) {
  return bt_meida.media_curr_sco;
}

hf_chan_handle_t *app_audio_manager_get_active_sco_chnl(void) {
  int curr_sco;

  curr_sco = app_audio_manager_get_active_sco_num();
  if (curr_sco != BT_DEVICE_NUM) {
    return &app_bt_device.hf_channel[curr_sco];
  }
  return NULL;
}

#if defined(HFP_1_6_ENABLE)
#ifdef __BT_ONE_BRING_TWO__
static uint16_t app_audio_manage_scocodecid[BT_DEVICE_NUM] = {
    BTIF_HF_SCO_CODEC_CVSD, BTIF_HF_SCO_CODEC_CVSD};
#else
static uint16_t app_audio_manage_scocodecid[BT_DEVICE_NUM] = {
    BTIF_HF_SCO_CODEC_CVSD};
#endif

int app_audio_manager_set_scocodecid(enum BT_DEVICE_ID_T dev_id,
                                     uint16_t codec_id) {
  app_audio_manage_scocodecid[dev_id] = codec_id;
  return 0;
}

int app_audio_manager_get_scocodecid(void) {
  int scocodecid = BTIF_HF_SCO_CODEC_NONE;
  if (bt_meida.media_curr_sco != BT_DEVICE_NUM) {
    scocodecid = app_audio_manage_scocodecid[bt_meida.media_curr_sco];
  }
  return scocodecid;
}
#endif

int app_audio_manager_switch_a2dp(enum BT_DEVICE_ID_T id) {
  bt_bdaddr_t *bdAdd = NULL;
  btif_remote_device_t *remDev = NULL;

#ifndef __MULTIPOINT_A2DP_PREEMPT__
  if (bt_meida.media_curr_sbc == id) {
    TRACE_AUD_MGR_I("[A2DP_SWITCH]the disconnected dev not working");
    return 0;
  }
#endif

  remDev = btif_a2dp_get_remote_device(app_bt_device.a2dp_connected_stream[id]);
  if (remDev) {
    TRACE_AUD_MGR_I("[A2DP_SWITCH] switch_a2dp to id:%d", id);
    bdAdd = btif_me_get_remote_device_bdaddr(remDev);
    app_bt_stream_volume_ptr_update(bdAdd->address);
    bt_meida.media_curr_sbc = id;
  }
  return 0;
}

bool app_audio_manager_a2dp_is_active(enum BT_DEVICE_ID_T id) {
  uint16_t media_type;
  bool nRet = false;

  media_type = bt_media_get_current_media();
  if (media_type & BT_STREAM_SBC) {
    if (bt_meida.media_curr_sbc == id) {
      nRet = true;
    }
  }

#ifndef BES_AUTOMATE_TEST
  TRACE_AUD_MGR_I("[A2DP_ACTIVE] nRet:%d type:%d %d/%d", nRet, media_type, id,
                  bt_meida.media_curr_sbc);
#endif
  return nRet;
}

bool app_audio_manager_hfp_is_active(enum BT_DEVICE_ID_T id) {
  uint16_t media_type;
  bool nRet = false;

  media_type = bt_media_get_current_media();
  if (media_type & BT_STREAM_VOICE) {
    if (bt_meida.media_curr_sco == id) {
      nRet = true;
    }
  }

#ifndef BES_AUTOMATE_TEST
  TRACE_AUD_MGR_I("[SCO_ACTIVE] nRet:%d type:%d %d/%d", nRet, media_type, id,
                  bt_meida.media_curr_sco);
#endif
  return nRet;
}

#ifdef VOICE_DATAPATH
bool app_audio_manager_capture_is_active(void) {
  uint16_t media_type;
  bool nRet = false;

  media_type = bt_media_get_current_media();
  if (media_type & BT_STREAM_CAPTURE) {
    nRet = true;
  }

  return nRet;
}
#endif

bool app_audio_manager_media_is_active(void) {
  uint16_t media_type;
  bool nRet = false;

  media_type = bt_media_get_current_media();
  if (media_type & BT_STREAM_MEDIA) {
    nRet = true;
  }

  return nRet;
}

void app_audio_manager_open(void) {
  if (app_audio_manager_init) {
    return;
  }
  bt_meida.media_curr_sbc = BT_DEVICE_NUM;
  bt_meida.media_curr_sco = BT_DEVICE_NUM;
  bt_meida.curr_active_media = 0;
  app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE,
                       app_audio_manager_handle_process);
  app_audio_manager_init = true;

#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  audio_prompt_init_handler();
#endif
}

void app_audio_manager_close(void) {
  app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE, NULL);
  app_audio_manager_init = false;
}

#ifdef RB_CODEC

static bool app_rbcodec_play_status = false;

static bool app_rbplay_player_mode = false;

bool app_rbplay_is_localplayer_mode(void) { return app_rbplay_player_mode; }

bool app_rbplay_mode_switch(void) {
  return (app_rbplay_player_mode = !app_rbplay_player_mode);
}

void app_rbplay_set_player_mode(bool isInPlayerMode) {
  app_rbplay_player_mode = isInPlayerMode;
}

void app_rbcodec_ctr_play_onoff(bool on) {
  TRACE_AUD_MGR_I("[RBCODEC] onoff %d ,turnon:%d ", app_rbcodec_play_status,
                  on);

  if (app_rbcodec_play_status == on)
    return;
  app_rbcodec_play_status = on;
  if (on)
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                  BT_STREAM_RBCODEC, 0, 0);
  else
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_RBCODEC,
                                  0, 0);
}

void app_rbcodec_ctl_set_play_status(bool st) {
  app_rbcodec_play_status = st;
  TRACE_AUD_MGR_I("[RBCODEC] curr_status%d", app_rbcodec_play_status);
}

bool app_rbcodec_get_play_status(void) {
  TRACE_AUD_MGR_I("[RBCODEC] play_status:%d", app_rbcodec_play_status);
  return app_rbcodec_play_status;
}

void app_rbcodec_toggle_play_stop(void) {
  if (app_rbcodec_get_play_status()) {
    app_rbcodec_ctr_play_onoff(false);
  } else {
    app_rbcodec_ctr_play_onoff(true);
  }
}

bool app_rbcodec_check_hfp_active(void) {
  return (bool)bt_media_is_media_active_by_type(BT_STREAM_VOICE);
}
#endif

void app_ibrt_sync_mix_prompt_req_handler(uint8_t *ptrParam,
                                          uint16_t paramLen) {
#ifdef TWS_PROMPT_SYNC
  app_tws_cmd_sync_mix_prompt_req_handler(ptrParam, paramLen);
#endif
}

void app_audio_decode_err_force_trigger(void) {
#ifndef IBRT
  trigger_media_play((AUD_ID_ENUM)AUDIO_ID_BT_MUTE, 0, false);
#endif
}
