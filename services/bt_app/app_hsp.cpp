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

#include "besbt.h"

#include "app_bt.h"
#include "btapp.h"
#include "cqueue.h"

#include "nvrecord.h"

#include "apps.h"
#include "resources.h"

#include "app_bt_media_manager.h"

#if defined(__HSP_ENABLE__) && defined(__BT_ONE_BRING_TWO__)
#error can not defined at the same time.
#endif

#if defined(__HSP_ENABLE__)

#define MODIFY_HS_CALL_SETUP_IN HF_CALL_SETUP_IN
#define MODIFY_HS_CALL_ACTIVE HF_CALL_ACTIVE
#define MODIFY_HS_CALL_NONE HF_CALL_NONE

extern struct BT_DEVICE_ID_DIFF chan_id_flag;
extern struct BT_DEVICE_T app_bt_device;

#if defined(SCO_LOOP)
#define HF_LOOP_CNT (20)
#define HF_LOOP_SIZE (360)

extern uint8_t hf_loop_buffer[HF_LOOP_CNT * HF_LOOP_SIZE];
extern uint32_t hf_loop_buffer_len[HF_LOOP_CNT];
extern uint32_t hf_loop_buffer_valid;
extern uint32_t hf_loop_buffer_size;
extern char hf_loop_buffer_w_idx;

#endif

#ifndef _SCO_BTPCM_CHANNEL_
extern struct hf_sendbuff_control hf_sendbuff_ctrl;
#endif

#ifdef __BT_ONE_BRING_TWO__
extern void hfp_chan_id_distinguish(HfChannel *Chan);
#endif

extern int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len);
extern int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len);
extern int store_voicecvsd_buffer(unsigned char *buf, unsigned int len);
extern int store_voicemsbc_buffer(unsigned char *buf, unsigned int len);
extern int hfp_volume_get(enum BT_DEVICE_ID_T id);
extern void hfp_volume_local_set(int8_t vol);
extern int hfp_volume_set(enum BT_DEVICE_ID_T id, int vol);
extern void app_bt_profile_connect_manager_hs(enum BT_DEVICE_ID_T id,
                                              HsChannel *Chan,
                                              HsCallbackParms *Info);
;

#ifdef __BT_ONE_BRING_TWO__
void hsp_chan_id_distinguish(HsChannel *Chan) {
  if (Chan == &app_bt_device.hs_channel[BT_DEVICE_ID_1]) {
    chan_id_flag.id = BT_DEVICE_ID_1;
    chan_id_flag.id_other = BT_DEVICE_ID_2;
  } else if (Chan == &app_bt_device.hsf_channel[BT_DEVICE_ID_2]) {
    chan_id_flag.id = BT_DEVICE_ID_2;
    chan_id_flag.id_other = BT_DEVICE_ID_1;
  }
}
#endif

int app_hsp_hscommand_mempool_init(void) {
  app_hfp_hfcommand_mempool_init();
  return 0;
}

int app_hsp_hscommand_mempool_calloc(HsCommand **hs_cmd_p) {
  app_hfp_hfcommand_mempool_calloc(hs_cmd_p);
  return 0;
}

int app_hsp_hscommand_mempool_free(HsCommand *hs_cmd_p) {
  app_hfp_hfcommand_mempool_free(hs_cmd_p);
  return 0;
}
#if 0
#define app_hsp_hscommand_mempool app_hfp_hfcommand_mempool
#define app_hsp_hscommand_mempool_init app_hfp_hfcommand_mempool_init
#define app_hsp_hscommand_mempool_calloc app_hfp_hfcommand_mempool_calloc
#define app_hsp_hscommand_mempool_free app_hfp_hfcommand_mempool_free
#endif

XaStatus app_hs_handle_cmd(HsChannel *Chan, uint8_t cmd_type) {
  HsCommand *hs_cmd_p;
  int8_t ret = 0;
  switch (cmd_type) {
  case APP_REPORT_SPEAKER_VOL_CMD:
    app_hsp_hscommand_mempool_calloc(&hs_cmd_p);
    if (hs_cmd_p) {
      if (HS_ReportSpeakerVolume(Chan, hfp_volume_get(chan_id_flag.id),
                                 hs_cmd_p) != BT_STATUS_PENDING) {
        app_hsp_hscommand_mempool_free(hs_cmd_p);
        ret = -1;
      }
    }
    break;
  case APP_CPKD_CMD:
    app_hsp_hscommand_mempool_calloc(&hs_cmd_p);
    if (hs_cmd_p) {
      if (HS_CKPD_CONTROL(Chan, hs_cmd_p) != BT_STATUS_PENDING) {
        app_hsp_hscommand_mempool_free(hs_cmd_p);
        ret = -1;
      }
    }
    break;
  default:
    break;
  }

  return ret;
}

// because hfp and hsp can not exist simultaneously , so we do not need to alloc
// 2 cmd pool!
void app_hsp_init(void) {

  app_hsp_hscommand_mempool_init();

  app_bt_device.curr_hs_channel_id = BT_DEVICE_ID_1;
  app_bt_device.hs_mute_flag = 0;

  for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
    app_bt_device.hschan_call[i] = 0;
    app_bt_device.hs_audio_state[i] = 0;
    app_bt_device.hs_conn_flag[i] = 0;
    app_bt_device.hs_voice_en[i] = 0;
  }
}

/*
Differ with HFP in HF_EVENT_RING_IND:
    Because hsp lack of some state, like         HF_EVENT_CALL_IND
                                                                    HF_EVENT_CALLSETUP_IND
                                                                    (DONE)
    And for the least code modify purpose, meanwhile  keep the state runs ok.
    i put all the state setting in HF_EVENT_RING_IND case
*/
void hsp_callback(HsChannel *Chan, HsCallbackParms *Info) {
  uint8_t ret = 0;

#ifdef __BT_ONE_BRING_TWO__
  hsp_chan_id_distinguish(Chan);
#else
  chan_id_flag.id = BT_DEVICE_ID_1;
#endif

  TRACE(2, "[%s] event = %d", __func__, Info->event);

  switch (Info->event) {
  case HS_EVENT_SERVICE_CONNECTED:
    TRACE(1, "::HS_EVENT_SERVICE_CONNECTED  Chan_id:%d\n", chan_id_flag.id);
    app_bt_profile_connect_manager_hs(chan_id_flag.id, Chan, Info);
#if defined(__BTIF_EARPHONE__)
    if (Chan->state == HF_STATE_OPEN) {
      ////report connected voice
      app_bt_device.hs_conn_flag[chan_id_flag.id] = 1;
    }
#endif
    app_bt_stream_volume_ptr_update((uint8_t *)Info->p.remDev->bdAddr.addr);
    if (app_hs_handle_cmd(Chan, APP_REPORT_SPEAKER_VOL_CMD) != 0)
      TRACE(0, "app_hs_handle_cmd err");

    break;
  case HS_EVENT_AUDIO_DATA_SENT:
    TRACE(1, "::HF_EVENT_AUDIO_DATA_SENT %d\n", Info->event);
#if defined(SCO_LOOP)
    hf_loop_buffer_valid = 1;
#endif
    break;
  case HS_EVENT_AUDIO_DATA:
    TRACE(0, "HF_EVENT_AUDIO_DATA");
    {

#ifndef _SCO_BTPCM_CHANNEL_
      uint32_t idx = 0;
      if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
        store_voicebtpcm_m2p_buffer(Info->p.audioData->data,
                                    Info->p.audioData->len);

        idx = hf_sendbuff_ctrl.index % HF_SENDBUFF_MEMPOOL_NUM;
        get_voicebtpcm_p2m_frame(&(hf_sendbuff_ctrl.mempool[idx].buffer[0]),
                                 Info->p.audioData->len);
        hf_sendbuff_ctrl.mempool[idx].packet.data =
            &(hf_sendbuff_ctrl.mempool[idx].buffer[0]);
        hf_sendbuff_ctrl.mempool[idx].packet.dataLen = Info->p.audioData->len;
        hf_sendbuff_ctrl.mempool[idx].packet.flags = BTP_FLAG_NONE;
        if (!app_bt_device.hf_mute_flag) {
          HF_SendAudioData(Chan, &hf_sendbuff_ctrl.mempool[idx].packet);
        }
        hf_sendbuff_ctrl.index++;
      }
#endif
    }

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
    break;
  case HS_EVENT_SERVICE_DISCONNECTED:
    TRACE(2, "::HS_EVENT_SERVICE_DISCONNECTED Chan_id:%d, reason=%x\n",
          chan_id_flag.id, Info->p.remDev->discReason);
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                  chan_id_flag.id, MAX_RECORD_NUM);

#if defined(__BTIF_EARPHONE__)
    if (app_bt_device.hs_conn_flag[chan_id_flag.id]) {
      ////report device disconnected voice
      app_bt_device.hs_conn_flag[chan_id_flag.id] = 0;
    }
#endif
    app_bt_stream_volume_ptr_update(NULL);

    app_bt_profile_connect_manager_hs(chan_id_flag.id, Chan, Info);
    for (uint8_t i = 0; i < BT_DEVICE_NUM; i++) {
      if (Chan == &(app_bt_device.hs_channel[i])) {
        app_bt_device.hschan_call[i] = 0;
        app_bt_device.hs_audio_state[i] = 0;
        app_bt_device.hs_conn_flag[i] = 0;
        app_bt_device.hs_voice_en[i] = 0;
      }
    }
    break;

  case HS_EVENT_AUDIO_CONNECTED:

    if (Info->status == BT_STATUS_SUCCESS) {
      TRACE(1, "::HS_EVENT_AUDIO_CONNECTED  chan_id:%d\n", chan_id_flag.id);
      if ((Chan->state == HF_STATE_OPEN) &&
          (app_bt_device.hs_conn_flag[chan_id_flag.id] == 1)) {
        app_bt_device.hschan_call[BT_DEVICE_ID_1] = MODIFY_HS_CALL_ACTIVE;
      }

      app_bt_device.curr_hs_channel_id = chan_id_flag.id;

      app_bt_device.phone_earphone_mark = 0;
      app_bt_device.hs_mute_flag = 0;

      app_bt_device.hs_audio_state[chan_id_flag.id] = HF_AUDIO_CON;

#if defined(__FORCE_REPORTVOLUME_SOCON__)

      app_hs_handle_cmd(Chan, APP_REPORT_SPEAKER_VOL_CMD);

#endif

      if (bt_media_cur_is_bt_stream_media()) {
        app_hfp_set_starting_media_pending_flag(true, BT_DEVICE_ID_1);
      } else {
        app_hfp_start_voice_media(BT_DEVICE_ID_1);
      }
    }
    break;
  case HS_EVENT_AUDIO_DISCONNECTED:
    TRACE(1, "::HS_EVENT_AUDIO_DISCONNECTED  chan_id:%d\n", chan_id_flag.id);

    if (app_bt_device.hschan_call[chan_id_flag.id] == HF_CALL_ACTIVE) {
      app_bt_device.phone_earphone_mark = 1;
    }

    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP, BT_STREAM_VOICE,
                                  BT_DEVICE_ID_1, MAX_RECORD_NUM);

    break;

  case HS_EVENT_RING_IND:
    TRACE(1, "::HS_EVENT_RING_IND  chan_id:%d\n", chan_id_flag.id);
#if defined(__BTIF_EARPHONE__)
    //        if(app_bt_device.hs_audio_state[chan_id_flag.id] != HF_AUDIO_CON)
    app_voice_report(APP_STATUS_INDICATION_INCOMINGCALL, chan_id_flag.id);
#endif

    break;
  case HS_EVENT_SPEAKER_VOLUME:
    TRACE(2, "::HS_EVENT_SPEAKER_VOLUME  chan_id:%d,speaker gain = %x\n",
          chan_id_flag.id, Info->p.ptr);
    hfp_volume_set(chan_id_flag.id, (int)(uint32_t)Info->p.ptr);
    break;

  case HS_EVENT_COMMAND_COMPLETE:
    TRACE(2, "::EVENT_HS_COMMAND_COMPLETE  chan_id:%d %x\n", chan_id_flag.id,
          (HsCommand *)Info->p.ptr);
    if (Info->p.ptr)
      app_hsp_hscommand_mempool_free((HsCommand *)Info->p.ptr);
    break;
  default:
    break;
  }
}

#endif
