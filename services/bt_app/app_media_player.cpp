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
#include "app_bt_trace.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "tgt_hardware.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WL_DET
#include "app_mic_alg.h"
#endif

#ifdef MEDIA_PLAYER_SUPPORT

#include "analog.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_utils.h"
#include "audioflinger.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "lockcqueue.h"

#include "app_bt.h"
#include "app_media_player.h"
#include "audio_prompt_sbc.h"
#include "besbt.h"
#include "res_audio_data.h"
#include "res_audio_ring.h"
#include "resources.h"

#include "app_bt_media_manager.h"
#include "btapp.h"
#include "cqueue.h"
#ifdef VOICE_DATAPATH
#include "app_ai_if.h"
#include "app_voicepath.h"
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
#include "anc_process.h"
#include "hal_codec.h"
#endif

#ifdef __THIRDPARTY
#include "app_thirdparty.h"
#endif
#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_ibrt_voice_report.h"
#include "app_tws_ibrt.h"
#endif
#ifdef __INTERACTION__
uint8_t g_findme_fadein_vol = TGT_VOLUME_LEVEL_0;
#endif
static char need_init_decoder = 1;
static btif_sbc_decoder_t *media_sbc_decoder = NULL;

#define SBC_TEMP_BUFFER_SIZE 64
#define SBC_QUEUE_SIZE (SBC_TEMP_BUFFER_SIZE * 16)
CQueue media_sbc_queue;

static float *media_sbc_eq_band_gain = NULL;

#ifdef __BT_ANC__
#define APP_AUDIO_PLAYBACK_BUFF_SIZE (1024 * 3)
#else
#define APP_AUDIO_PLAYBACK_BUFF_SIZE (1024 * 4)
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
static enum AUD_BITS_T sample_size_play_bt;
static enum AUD_SAMPRATE_T sample_rate_play_bt;
static uint32_t data_size_play_bt;

static uint8_t *playback_buf_bt;
static uint32_t playback_size_bt;
static int32_t playback_samplerate_ratio_bt;

static uint8_t *playback_buf_mc;
static uint32_t playback_size_mc;
static enum AUD_CHANNEL_NUM_T playback_ch_num_bt;
#endif

#define SBC_FRAME_LEN 64 // 0x5c   /* pcm 512 bytes*/
static U8 *g_app_audio_data = NULL;
static uint32_t g_app_audio_length = 0;
static uint32_t g_app_audio_read = 0;

static uint32_t g_play_continue_mark = 0;

static uint8_t app_play_sbc_stop_proc_cnt = 0;

static uint16_t g_prompt_chnlsel = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;

// for continue play

#define MAX_SOUND_NUMBER 10

typedef struct tMediaSoundMap {
  U8 *data;       // total files
  uint32_t fsize; // file index

} _tMediaSoundMap;

const tMediaSoundMap media_sound_map[MAX_SOUND_NUMBER] = {
    {(U8 *)SOUND_ZERO, SOUND_ZERO_len},   {(U8 *)SOUND_ONE, SOUND_ONE_len},
    {(U8 *)SOUND_TWO, SOUND_TWO_len},     {(U8 *)SOUND_THREE, SOUND_THREE_len},
    {(U8 *)SOUND_FOUR, SOUND_FOUR_len},   {(U8 *)SOUND_FIVE, SOUND_FIVE_len},
    {(U8 *)SOUND_SIX, SOUND_SIX_len},     {(U8 *)SOUND_SEVEN, SOUND_SEVEN_len},
    {(U8 *)SOUND_EIGHT, SOUND_EIGHT_len}, {(U8 *)SOUND_NINE, SOUND_NINE_len},
};

extern const uint8_t SOUND_MUTE[];
extern const unsigned SOUND_MUTE_len;

char Media_player_number[MAX_PHB_NUMBER];

typedef struct tPlayContContext {
  uint32_t g_play_continue_total; // total files
  uint32_t g_play_continue_n;     // file index

  uint32_t g_play_continue_fread; // per file have readed

  U8 g_play_continue_array[MAX_PHB_NUMBER];

} _tPlayContContext;

tPlayContContext pCont_context;

APP_AUDIO_STATUS MSG_PLAYBACK_STATUS;
APP_AUDIO_STATUS *ptr_msg_playback = &MSG_PLAYBACK_STATUS;

static int g_language = MEDIA_DEFAULT_LANGUAGE;
#ifdef AUDIO_LINEIN
static enum AUD_SAMPRATE_T app_play_audio_sample_rate = AUD_SAMPRATE_16000;
#endif

#define PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index)                        \
  ((PROMPT_MIX_PROPERTY_T *)((uintptr_t)__mixprompt_property_table_start +     \
                             (index) * sizeof(PROMPT_MIX_PROPERTY_T)))

int media_audio_init(void) {
  const float EQLevel[25] = {
      0.0630957, 0.0794328, 0.1,       0.1258925, 0.1584893,
      0.1995262, 0.2511886, 0.3162278, 0.398107,  0.5011872,
      0.6309573, 0.794328,  1,         1.258925,  1.584893,
      1.995262,  2.5118864, 3.1622776, 3.9810717, 5.011872,
      6.309573,  7.943282,  10,        12.589254, 15.848932}; //-12~12
  uint8_t *buff = NULL;
  uint8_t i;

  app_audio_mempool_get_buff((uint8_t **)&media_sbc_eq_band_gain,
                             CFG_HW_AUD_EQ_NUM_BANDS * sizeof(float));

  for (i = 0; i < CFG_HW_AUD_EQ_NUM_BANDS; i++) {
    media_sbc_eq_band_gain[i] = EQLevel[12];
  }

  app_audio_mempool_get_buff(&buff, SBC_QUEUE_SIZE);
  memset(buff, 0, SBC_QUEUE_SIZE);

  LOCK_APP_AUDIO_QUEUE();
  APP_AUDIO_InitCQueue(&media_sbc_queue, SBC_QUEUE_SIZE, buff);
  UNLOCK_APP_AUDIO_QUEUE();

  app_audio_mempool_get_buff((uint8_t **)&media_sbc_decoder,
                             sizeof(btif_sbc_decoder_t) + 4);

  need_init_decoder = 1;

  app_play_sbc_stop_proc_cnt = 0;

  return 0;
}
static int decode_sbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len) {
  uint8_t underflow = 0;

  int r = 0;
  unsigned char *e1 = NULL, *e2 = NULL;
  unsigned int len1 = 0, len2 = 0;

  static btif_sbc_pcm_data_t pcm_data;
  bt_status_t ret = BT_STS_SUCCESS;
  unsigned short byte_decode = 0;

  pcm_data.data = (unsigned char *)pcm_buffer;

  LOCK_APP_AUDIO_QUEUE();
again:
  if (need_init_decoder) {
    pcm_data.data = (unsigned char *)pcm_buffer;
    pcm_data.dataLen = 0;
    btif_sbc_init_decoder(media_sbc_decoder);
  }

get_again:
  len1 = len2 = 0;
  r = APP_AUDIO_PeekCQueue(&media_sbc_queue, SBC_TEMP_BUFFER_SIZE, &e1, &len1,
                           &e2, &len2);

  if (r == CQ_ERR) {
    need_init_decoder = 1;
    underflow = 1;
    r = pcm_data.dataLen;
    TRACE_MEDIA_PLAYESTREAM_I("[DECODE_SBC][END]");
    goto exit;
  }
  if (!len1) {
    TRACE_MEDIA_PLAYESTREAM_I("[DECODE_SBC][LOOP] len1 underflow %d/%d\n", len1,
                              len2);
    goto get_again;
  }

  ret = btif_sbc_decode_frames(media_sbc_decoder, (unsigned char *)e1, len1,
                               &byte_decode, &pcm_data, pcm_len,
                               media_sbc_eq_band_gain);

  if (ret == BT_STS_CONTINUE) {
    need_init_decoder = 0;
    APP_AUDIO_DeCQueue(&media_sbc_queue, 0, len1);
    goto again;

    /* back again */
  } else if (ret == BT_STS_SUCCESS) {
    need_init_decoder = 0;
    r = pcm_data.dataLen;
    pcm_data.dataLen = 0;

    APP_AUDIO_DeCQueue(&media_sbc_queue, 0, byte_decode);

    // TRACE_MEDIA_PLAYESTREAM_I("p %d\n", pcm_data.sampleFreq);

    /* leave */
  } else if (ret == BT_STS_FAILED) {
    need_init_decoder = 1;
    r = pcm_data.dataLen;
    TRACE_MEDIA_PLAYESTREAM_E("[DECODE_SBC] err\n");

    APP_AUDIO_DeCQueue(&media_sbc_queue, 0, byte_decode);

    /* leave */
  } else if (ret == BT_STS_NO_RESOURCES) {
    need_init_decoder = 0;

    TRACE_MEDIA_PLAYESTREAM_I("[DECODE_SBC] no\n");

    /* leav */
    r = 0;
  }

exit:
  if (underflow) {
    TRACE_MEDIA_PLAYESTREAM_I("[DECODE_SBC][END] len:%d\n ", pcm_len);
  }
  UNLOCK_APP_AUDIO_QUEUE();
  return r;
}

static int store_sbc_buffer(unsigned char *buf, unsigned int len) {
  int nRet;

  LOCK_APP_AUDIO_QUEUE();
  nRet = APP_AUDIO_EnCQueue(&media_sbc_queue, buf, len);
  UNLOCK_APP_AUDIO_QUEUE();

  return nRet;
}

#if defined(IBRT)

#define PENDING_SYNC_PROMPT_BUFFER_CNT 8
// cleared when tws is disconnected
static uint16_t pendingSyncPromptId[PENDING_SYNC_PROMPT_BUFFER_CNT];
static uint8_t pending_sync_prompt_in_index = 0;
static uint8_t pending_sync_prompt_out_index = 0;
static uint8_t pending_sync_prompt_cnt = 0;

void app_tws_sync_prompt_manager_reset(void) {
  pending_sync_prompt_in_index = 0;
  pending_sync_prompt_out_index = 0;
  pending_sync_prompt_cnt = 0;
}

void app_tws_sync_prompt_check(void) {
  if (0 == pending_sync_prompt_cnt) {
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_SYNC_VOICE_PROMPT,
                             UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
  }

  if (IBRT_ACTIVE_MODE != app_ibrt_if_get_bt_ctrl_ctx()->tws_mode) {
    return;
  }

  bool isPlayPendingPrompt = false;
  uint16_t promptIdToPlay = 0;

  uint32_t lock = int_lock_global();
  if (pending_sync_prompt_cnt > 0) {
    isPlayPendingPrompt = true;
    promptIdToPlay = pendingSyncPromptId[pending_sync_prompt_out_index];
    pending_sync_prompt_out_index++;
    if (PENDING_SYNC_PROMPT_BUFFER_CNT == pending_sync_prompt_out_index) {
      pending_sync_prompt_out_index = 0;
    }
    pending_sync_prompt_cnt--;
  }
  int_unlock_global(lock);

  if (isPlayPendingPrompt) {
    TRACE_MEDIA_PLAYESTREAM_I("[POP_PENDING_PROMPT] 0x%x to play",
                              promptIdToPlay);
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_MEDIA,
                                  0, promptIdToPlay);
  }
}
#endif

void trigger_media_play(AUD_ID_ENUM id, uint8_t device_id, uint16_t aud_pram) {
  uint16_t convertedId = (uint8_t)id;
  convertedId |= aud_pram;
  app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START, BT_STREAM_MEDIA,
                                device_id, convertedId);
}
void trigger_media_stop(AUD_ID_ENUM id, uint8_t device_id) {
  /* Only the stop loop mode is supported */
  if (id == AUDIO_ID_FIND_MY_BUDS)
    app_play_sbc_stop_proc_cnt = 1;
}

uint32_t media_playAudioSideSelect(AUD_ID_ENUM id, uint8_t device_id,
                                   uint16_t side_select) {
  trigger_media_play(id, device_id, PROMOT_ID_BIT_MASK_MERGING | side_select);
  return 0;
}

uint32_t media_PlayAudio(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id,
                     PROMOT_ID_BIT_MASK_MERGING |
                         PROMOT_ID_BIT_MASK_CHNLSEl_ALL);
  return 0;
}

void media_PlayAudio_standalone(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id, false);
}

void media_PlayAudio_locally(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id,
                     PROMOT_ID_BIT_MASK_MERGING |
                         PROMOT_ID_BIT_MASK_CHNLSEl_ALL);
}

void media_PlayAudio_standalone_locally(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id, PROMOT_ID_BIT_MASK_CHNLSEl_ALL);
}

void media_PlayAudio_remotely(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id,
                     PROMOT_ID_BIT_MASK_MERGING |
                         PROMOT_ID_BIT_MASK_CHNLSEl_ALL);
}

void media_PlayAudio_standalone_remotely(AUD_ID_ENUM id, uint8_t device_id) {
  trigger_media_play(id, device_id, PROMOT_ID_BIT_MASK_CHNLSEl_ALL);
}

AUD_ID_ENUM media_GetCurrentPrompt(uint8_t device_id) {
  AUD_ID_ENUM currentPromptId = AUD_ID_INVALID;
  if (app_bt_stream_isrun(APP_PLAY_BACK_AUDIO)) {
    currentPromptId = app_get_current_standalone_promptId();
  }
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  else if (audio_prompt_is_playing_ongoing()) {
    currentPromptId = (AUD_ID_ENUM)audio_prompt_get_prompt_id();
  }
#endif
  return currentPromptId;
}

#define IsDigit(c) (((c) >= '0') && ((c) <= '9'))
void media_Set_IncomingNumber(const char *pNumber) {
  char *p_num = Media_player_number;
  uint8_t cnt = 0;
  for (uint8_t idx = 0; idx < MAX_PHB_NUMBER; idx++) {
    if (*(pNumber + idx) == 0)
      break;

    if (IsDigit(*(pNumber + idx))) {
      *(p_num + cnt) = *(pNumber + idx);
      TRACE_MEDIA_PLAYESTREAM_I("[INCOMINGNUMBER] cnt %d ,p_num  %d", cnt,
                                *(p_num + cnt));
      cnt++;
    }
  }
}

PROMPT_MIX_PROPERTY_T *get_prompt_mix_property(uint16_t promptId) {
  for (uint32_t index = 0;
       index < ((uint32_t)(uintptr_t)__mixprompt_property_table_end -
                (uint32_t)(uintptr_t)__mixprompt_property_table_start) /
                   sizeof(PROMPT_MIX_PROPERTY_T);
       index++) {
    if (PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index)->promptId == promptId) {
      return PROMPT_MIX_PROPERTY_PTR_FROM_ENTRY_INDEX(index);
    }
  }

  return NULL;
}
/*
Reference information for how to pass
parameters into PROMPT_MIX_PROPERTY_TO_ADD:

PROMPT_MIX_PROPERTY_TO_ADD(
promptId,
volume_level_override,
coeff_for_mix_prompt_for_music,
coeff_for_mix_music_for_music,
coeff_for_mix_prompt_for_call,
coeff_for_mix_call_for_call)
*/
void media_runtime_audio_prompt_update(uint16_t id, uint8_t **ptr,
                                       uint32_t *len) {
  switch (id) {
  case AUD_ID_POWER_ON:
    g_app_audio_data = (U8 *)SOUND_POWER_ON; // aud_get_reouce((AUD_ID_ENUM)id,
                                             // &g_app_audio_length, &type);
    g_app_audio_length = SOUND_POWER_ON_len;
    break;
  case AUD_ID_POWER_OFF:
    g_app_audio_data = (U8 *)SOUND_POWER_OFF;
    g_app_audio_length = SOUND_POWER_OFF_len;
    break;
  case AUD_ID_BT_PAIR_ENABLE:
    g_app_audio_data = (U8 *)SOUND_PAIR_ENABLE;
    g_app_audio_length = SOUND_PAIR_ENABLE_len;
    break;
  case AUD_ID_BT_PAIRING:
    g_app_audio_data = (U8 *)SOUND_PAIRING;
    g_app_audio_length = SOUND_PAIRING_len;
    break;
  case AUD_ID_BT_PAIRING_SUC:
    g_app_audio_data = (U8 *)SOUND_PAIRING_SUCCESS;
    g_app_audio_length = SOUND_PAIRING_SUCCESS_len;
    break;
  case AUD_ID_BT_PAIRING_FAIL:
    g_app_audio_data = (U8 *)SOUND_PAIRING_FAIL;
    g_app_audio_length = SOUND_PAIRING_FAIL_len;
    break;
  case AUD_ID_BT_CALL_REFUSE:
    g_app_audio_data = (U8 *)SOUND_REFUSE;
    g_app_audio_length = SOUND_REFUSE_len;
    break;
  case AUD_ID_BT_CALL_OVER:
    g_app_audio_data = (U8 *)SOUND_OVER;
    g_app_audio_length = SOUND_OVER_len;
    break;
  case AUD_ID_BT_CALL_ANSWER:
    g_app_audio_data = (U8 *)SOUND_ANSWER;
    g_app_audio_length = SOUND_ANSWER_len;
    break;
  case AUD_ID_BT_CALL_HUNG_UP:
    g_app_audio_data = (U8 *)SOUND_HUNG_UP;
    g_app_audio_length = SOUND_HUNG_UP_len;
    break;
  case AUD_ID_BT_CALL_INCOMING_CALL:
    g_app_audio_data = (U8 *)SOUND_INCOMING_CALL;
    g_app_audio_length = SOUND_INCOMING_CALL_len;
    break;
  case AUD_ID_BT_CHARGE_PLEASE:
    g_app_audio_data = (U8 *)SOUND_CHARGE_PLEASE;
    g_app_audio_length = SOUND_CHARGE_PLEASE_len;
    break;
  case AUD_ID_BT_CHARGE_FINISH:
    g_app_audio_data = (U8 *)SOUND_CHARGE_FINISH;
    g_app_audio_length = SOUND_CHARGE_FINISH_len;
    break;
  case AUD_ID_BT_CONNECTED:
    g_app_audio_data = (U8 *)SOUND_CONNECTED;
    g_app_audio_length = SOUND_CONNECTED_len;
    break;
  case AUD_ID_BT_DIS_CONNECT:
    g_app_audio_data = (U8 *)SOUND_DIS_CONNECT;
    g_app_audio_length = SOUND_DIS_CONNECT_len;
    break;
  case AUD_ID_BT_WARNING:
    g_app_audio_data = (U8 *)SOUND_WARNING;
    g_app_audio_length = SOUND_WARNING_len;
    break;
  case AUDIO_ID_BT_ALEXA_START:
    g_app_audio_data = (U8 *)SOUND_ALEXA_START;
    g_app_audio_length = SOUND_ALEXA_START_len;
    break;
  case AUDIO_ID_BT_ALEXA_STOP:
  case AUDIO_ID_FIND_MY_BUDS:
  case AUDIO_ID_FIND_TILE:
    g_app_audio_data = (U8 *)SOUND_ALEXA_STOP;
    g_app_audio_length = SOUND_ALEXA_STOP_len;
    break;
  case AUDIO_ID_BT_GSOUND_MIC_OPEN:
    g_app_audio_data = (U8 *)SOUND_GSOUND_MIC_OPEN;
    g_app_audio_length = SOUND_GSOUND_MIC_OPEN_len;
    break;
  case AUDIO_ID_BT_GSOUND_MIC_CLOSE:
    g_app_audio_data = (U8 *)SOUND_GSOUND_MIC_CLOSE;
    g_app_audio_length = SOUND_GSOUND_MIC_CLOSE_len;
    break;
  case AUDIO_ID_BT_GSOUND_NC:
    g_app_audio_data = (U8 *)SOUND_GSOUND_NC;
    g_app_audio_length = SOUND_GSOUND_NC_len;
    break;
  case AUD_ID_LANGUAGE_SWITCH:
    g_app_audio_data = (U8 *)SOUND_LANGUAGE_SWITCH;
    g_app_audio_length = SOUND_LANGUAGE_SWITCH_len;
    break;
  case AUDIO_ID_BT_MUTE:
    g_app_audio_data = (U8 *)SOUND_MUTE;
    g_app_audio_length = SOUND_MUTE_len;
    break;
  case AUD_ID_NUM_0:
    g_app_audio_data = (U8 *)SOUND_ZERO;
    g_app_audio_length = SOUND_ZERO_len;
    break;
  case AUD_ID_NUM_1:
    g_app_audio_data = (U8 *)SOUND_ONE;
    g_app_audio_length = SOUND_ONE_len;
    break;
  case AUD_ID_NUM_2:
    g_app_audio_data = (U8 *)SOUND_TWO;
    g_app_audio_length = SOUND_TWO_len;
    break;
  case AUD_ID_NUM_3:
    g_app_audio_data = (U8 *)SOUND_THREE;
    g_app_audio_length = SOUND_THREE_len;
    break;
  case AUD_ID_NUM_4:
    g_app_audio_data = (U8 *)SOUND_FOUR;
    g_app_audio_length = SOUND_FOUR_len;
    break;
  case AUD_ID_NUM_5:
    g_app_audio_data = (U8 *)SOUND_FIVE;
    g_app_audio_length = SOUND_FIVE_len;
    break;
  case AUD_ID_NUM_6:
    g_app_audio_data = (U8 *)SOUND_SIX;
    g_app_audio_length = SOUND_SIX_len;
    break;
  case AUD_ID_NUM_7:
    g_app_audio_data = (U8 *)SOUND_SEVEN;
    g_app_audio_length = SOUND_SEVEN_len;
    break;
  case AUD_ID_NUM_8:
    g_app_audio_data = (U8 *)SOUND_EIGHT;
    g_app_audio_length = SOUND_EIGHT_len;
    break;
  case AUD_ID_NUM_9:
    g_app_audio_data = (U8 *)SOUND_NINE;
    g_app_audio_length = SOUND_NINE_len;
    break;
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
  case AUD_ID_RING_WARNING:
    g_app_audio_data = (U8 *)RES_AUD_RING_SAMPRATE_16000;
    g_app_audio_length = RES_AUD_RING_SAMPRATE_16000;
    break;
#endif
#ifdef __INTERACTION__
  case AUD_ID_BT_FINDME:
    g_app_audio_data = (U8 *)SOUND_FINDME;
    g_app_audio_length = SOUND_FINDME_len;
    break;
#endif
  case AUDIO_ID_BT_DUDU:
    g_app_audio_data = (U8 *)DUDU;
    g_app_audio_length = DUDU_len;
    break;
  case AUDIO_ID_BT_DU:
    g_app_audio_data = (U8 *)DUDU;
    g_app_audio_length = DUDU_len;
    break;
  default:
    g_app_audio_length = 0;
    break;
  }

  *ptr = g_app_audio_data;
  *len = g_app_audio_length;
}

void media_Play_init_audio(uint16_t aud_id) {

  if (aud_id == AUD_ID_BT_CALL_INCOMING_NUMBER) {
    g_play_continue_mark = 1;

    memset(&pCont_context, 0x0, sizeof(pCont_context));

    pCont_context.g_play_continue_total =
        strlen((const char *)Media_player_number);

    for (uint32_t i = 0;
         (i < pCont_context.g_play_continue_total) && (i < MAX_PHB_NUMBER);
         i++) {
      pCont_context.g_play_continue_array[i] = Media_player_number[i] - '0';

      TRACE_MEDIA_PLAYESTREAM_I("[INIT_AUDIO] array[%d] = %d, total =%d", i,
                                pCont_context.g_play_continue_array[i],
                                pCont_context.g_play_continue_total);
    }
  } else {
    g_app_audio_read = 0;
    g_play_continue_mark = 0;

    media_runtime_audio_prompt_update(aud_id, &g_app_audio_data,
                                      &g_app_audio_length);
  }
}
uint32_t app_play_sbc_more_data_fadeout(int16_t *buf, uint32_t len) {
  uint32_t i;
  uint32_t j = 0;

  for (i = len; i > 0; i--) {
    *(buf + j) = *(buf + j) * i / len;
    j++;
  }

  return len;
}

static uint32_t need_fadein_len = 0;
static uint32_t need_fadein_len_processed = 0;

int app_play_sbc_more_data_fadein_config(uint32_t len) {
  TRACE_MEDIA_PLAYESTREAM_I("[FADEIN] config l:%d", len);
  need_fadein_len = len;
  need_fadein_len_processed = 0;
  return 0;
}
uint32_t app_play_sbc_more_data_fadein(int16_t *buf, uint32_t len) {
  uint32_t i;
  uint32_t j = 0;
  uint32_t base;
  uint32_t dest;

  base = need_fadein_len_processed;
  dest = need_fadein_len_processed + len < need_fadein_len
             ? need_fadein_len_processed + len
             : need_fadein_len_processed;

  if (base >= dest) {
    //        TRACE_MEDIA_PLAYESTREAM_I("skip fadein");
    return len;
  }
  //    TRACE_MEDIA_PLAYESTREAM_I("fadein l:%d base:%d dest:%d", len, base,
  //    dest); DUMP16("%5d ", buf, 20); DUMP16("%5d ", buf+len-19, 20);

  for (i = base; i < dest; i++) {
    *(buf + j) = *(buf + j) * i / need_fadein_len;
    j++;
  }

  need_fadein_len_processed += j;
  //    DUMP16("%05d ", buf, 20);
  //    DUMP16("%5d ", buf+len-19, 20);
  return len;
}

uint32_t app_play_single_sbc_more_data(uint8_t *buf, uint32_t len) {
  // int32_t stime, etime;
  // U16 byte_decode;
  uint32_t l = 0;

  // TRACE_MEDIA_PLAYESTREAM_I("app_play_sbc_more_data : %d, %d",
  // g_app_audio_read, g_app_audio_length);

  if (g_app_audio_read < g_app_audio_length) {
    unsigned int available_len = 0;
    unsigned int store_len = 0;

    available_len = AvailableOfCQueue(&media_sbc_queue);
    store_len = (g_app_audio_length - g_app_audio_read) > available_len
                    ? available_len
                    : (g_app_audio_length - g_app_audio_read);
    store_sbc_buffer((unsigned char *)(g_app_audio_data + g_app_audio_read),
                     store_len);
    g_app_audio_read += store_len;
  }

  l = decode_sbc_frame(buf, len);

  if (l != len) {
    g_app_audio_read = g_app_audio_length;
    // af_stream_stop(AUD_STREAM_PLAYBACK);
    // af_stream_close(AUD_STREAM_PLAYBACK);
    TRACE_MEDIA_PLAYESTREAM_I(
        "[SINGLE_SBC_MORE_DATA][END] length:%d len:%d l:%d", g_app_audio_length,
        len, l);
  }

  return l;
}

/* play continue sound */
uint32_t app_play_continue_sbc_more_data(uint8_t *buf, uint32_t len) {

  uint32_t l, n, fsize = 0;

  U8 *pdata = NULL;

  // store data
  unsigned int available_len = 0;
  unsigned int store_len = 0;

  if (pCont_context.g_play_continue_n < pCont_context.g_play_continue_total) {
    do {
      n = pCont_context.g_play_continue_n;
      pdata = media_sound_map[pCont_context.g_play_continue_array[n]].data;
      fsize = media_sound_map[pCont_context.g_play_continue_array[n]].fsize;

      available_len = AvailableOfCQueue(&media_sbc_queue);
      if (!available_len)
        break;

      store_len = (fsize - pCont_context.g_play_continue_fread) > available_len
                      ? available_len
                      : (fsize - pCont_context.g_play_continue_fread);
      store_sbc_buffer(
          (unsigned char *)(pdata + pCont_context.g_play_continue_fread),
          store_len);
      pCont_context.g_play_continue_fread += store_len;
      if (pCont_context.g_play_continue_fread == fsize) {
        pCont_context.g_play_continue_n++;
        pCont_context.g_play_continue_fread = 0;
      }
    } while (pCont_context.g_play_continue_n <
             pCont_context.g_play_continue_total);
  }

  l = decode_sbc_frame(buf, len);

  if (l != len) {
    TRACE_MEDIA_PLAYESTREAM_I("[CONTINUE_SBC_MORE_DATA][END]");
  }

  return l;
}

uint32_t app_play_sbc_channel_select(int16_t *dst_buf, int16_t *src_buf,
                                     uint32_t src_len) {
#if defined(IBRT)
  if (IS_PROMPT_CHNLSEl_ALL(g_prompt_chnlsel) ||
      app_ibrt_voice_report_is_me(
          PROMPT_CHNLSEl_FROM_ID_VALUE(g_prompt_chnlsel))) {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = src_buf[i];
    }
  } else {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = 0;
    }
  }
#else
  if (IS_PROMPT_CHNLSEl_ALL(g_prompt_chnlsel)) {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = src_buf[i];
    }
  } else if (IS_PROMPT_CHNLSEl_LCHNL(g_prompt_chnlsel)) {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = src_buf[i];
      dst_buf[i * 2 + 1] = 0;
    }
  } else if (IS_PROMPT_CHNLSEl_RCHNL(g_prompt_chnlsel)) {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = 0;
      dst_buf[i * 2 + 1] = src_buf[i];
    }
  } else {
    // Copy from tail so that it works even if dst_buf == src_buf
    for (int i = (int)(src_len - 1); i >= 0; i--) {
      dst_buf[i * 2 + 0] = dst_buf[i * 2 + 1] = 0;
    }
  }
#endif
  return 0;
}

#ifdef __BT_ANC__
extern uint8_t bt_sco_samplerate_ratio;
extern void us_fir_init(void);
extern U32 us_fir_run(short *src_buf, short *dst_buf, U32 in_samp_num);
#endif

uint32_t g_cache_buff_sz = 0;

static int16_t *app_play_sbc_cache = NULL;
uint32_t app_play_sbc_more_data(uint8_t *buf, uint32_t len) {
#if defined(IBRT)
  app_ibrt_voice_report_trigger_checker();
#endif

#ifdef VOICE_DATAPATH
  if (app_voicepath_get_stream_pending_state(VOICEPATH_STREAMING)) {
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#ifdef MIX_MIC_DURING_MUSIC
    app_voicepath_enable_hw_sidetone(0, HW_SIDE_TONE_MAX_ATTENUATION_COEF);
#endif
    app_voicepath_set_stream_state(VOICEPATH_STREAMING, true);
    app_voicepath_set_pending_started_stream(VOICEPATH_STREAMING, false);
  }
#endif
  uint32_t l = 0;

  memset(buf, 0, len);

#if defined(MEDIA_PLAY_24BIT)
  len /= 2;
#endif

#ifdef __BT_ANC__
  uint32_t dec_len = len / bt_sco_samplerate_ratio;
#endif

  if (app_play_sbc_cache)
    memset(app_play_sbc_cache, 0, g_cache_buff_sz);

  if (app_play_sbc_stop_proc_cnt) {
    if (app_play_sbc_stop_proc_cnt == 1) {
      app_play_sbc_stop_proc_cnt = 2;
    } else if (app_play_sbc_stop_proc_cnt == 2) {
      app_play_sbc_stop_proc_cnt = 3;

      // For 8K sample rate data, it takes about 4ms (or 12ms if h/w resample in
      // use) from codec to DAC PA. The playback stream should be stopped after
      // the last data arrives at DAC PA, otherwise there might be some pop
      // sound.
      app_play_audio_stop();
    }
  } else {
    if (app_play_sbc_cache) {
#ifdef __BT_ANC__
      len = dec_len;
#endif
      if (g_play_continue_mark) {
        l = app_play_continue_sbc_more_data((uint8_t *)app_play_sbc_cache,
                                            len / 2);
      } else {
        l = app_play_single_sbc_more_data((uint8_t *)app_play_sbc_cache,
                                          len / 2);
      }
      if (l != len / 2) {
#ifdef __BT_ANC__
        len = dec_len * 3;
#endif
        memset(app_play_sbc_cache + l, 0, len / 2 - l);
        app_play_sbc_stop_proc_cnt = 1;
      }
#ifdef __BT_ANC__
      len = dec_len * 3;
      l = l * 3;
      us_fir_run((short *)app_play_sbc_cache, (short *)buf, dec_len / 2 / 2);
      app_play_sbc_channel_select((int16_t *)buf, (int16_t *)buf, len / 2 / 2);
#else
      app_play_sbc_channel_select((int16_t *)buf, app_play_sbc_cache,
                                  len / 2 / 2);
#endif

#if defined(MEDIA_PLAY_24BIT)
      int32_t *buf32 = (int32_t *)buf;
      int16_t *buf16 = (int16_t *)buf;

      for (int16_t i = len / 2 - 1; i >= 0; i--) {
        buf32[i] = ((int32_t)buf16[i] << 8);
      }
      len *= 2;
#endif
    } else {
#if defined(MEDIA_PLAY_24BIT)
      len *= 2;
#endif
      memset(buf, 0, len);
    }
  }

  return l;
}

#ifdef AUDIO_LINEIN
static uint8_t app_play_lineinmode_merge = 0;
static uint8_t app_play_lineinmode_mode = 0;

inline static void app_play_audio_lineinmode_mono_merge(int16_t *aud_buf_mono,
                                                        int16_t *ring_buf_mono,
                                                        uint32_t aud_buf_len) {
  uint32_t i = 0;
  for (i = 0; i < aud_buf_len; i++) {
    aud_buf_mono[i] = (aud_buf_mono[i] >> 1) + (ring_buf_mono[i] >> 1);
  }
}

inline static void app_play_audio_lineinmode_stereo_merge(
    int16_t *aud_buf_stereo, int16_t *ring_buf_mono, uint32_t aud_buf_len) {
  uint32_t aud_buf_stereo_offset = 0;
  uint32_t ring_buf_mono_offset = 0;
  for (aud_buf_stereo_offset = 0; aud_buf_stereo_offset < aud_buf_len;) {
    aud_buf_stereo[aud_buf_stereo_offset] =
        aud_buf_stereo[aud_buf_stereo_offset] +
        (ring_buf_mono[ring_buf_mono_offset] >> 1);
    aud_buf_stereo_offset++;
    aud_buf_stereo[aud_buf_stereo_offset] =
        aud_buf_stereo[aud_buf_stereo_offset] +
        (ring_buf_mono[ring_buf_mono_offset] >> 1);
    aud_buf_stereo_offset++;
    ring_buf_mono_offset++;
  }
}

uint32_t app_play_audio_lineinmode_more_data(uint8_t *buf, uint32_t len) {
  uint32_t l = 0;
  if (app_play_lineinmode_merge && app_play_sbc_cache) {
    TRACE(1, "line in mode:%d ", len);
    if (app_play_lineinmode_mode == 1) {
      if (g_play_continue_mark) {
        l = app_play_continue_sbc_more_data((uint8_t *)app_play_sbc_cache, len);
      } else {
        l = app_play_single_sbc_more_data((uint8_t *)app_play_sbc_cache, len);
      }
      if (l != len) {
        memset(app_play_sbc_cache + l, 0, len - l);
        app_play_lineinmode_merge = 0;
      }
      app_play_audio_lineinmode_mono_merge(
          (int16_t *)buf, (int16_t *)app_play_sbc_cache, len / 2);
    } else if (app_play_lineinmode_mode == 2) {
      if (g_play_continue_mark) {
        l = app_play_continue_sbc_more_data((uint8_t *)app_play_sbc_cache,
                                            len / 2);
      } else {
        l = app_play_single_sbc_more_data((uint8_t *)app_play_sbc_cache,
                                          len / 2);
      }
      if (l != len / 2) {
        memset(app_play_sbc_cache + l, 0, len / 2 - l);
        app_play_lineinmode_merge = 0;
      }
      app_play_audio_lineinmode_stereo_merge(
          (int16_t *)buf, (int16_t *)app_play_sbc_cache, len / 2);
    }
  }

  return l;
}

int app_play_audio_lineinmode_init(uint8_t mode, uint32_t buff_len) {
  TRACE(1, "lapp_play_audio_lineinmode_init:%d ", buff_len);
  app_play_lineinmode_mode = mode;
  app_audio_mempool_get_buff((uint8_t **)&app_play_sbc_cache, buff_len);
  media_audio_init();
  return 0;
}

int app_play_audio_lineinmode_start(APP_AUDIO_STATUS *status) {
  if (app_play_audio_sample_rate == AUD_SAMPRATE_44100) {
    LOCK_APP_AUDIO_QUEUE();
    APP_AUDIO_DeCQueue(&media_sbc_queue, 0,
                       APP_AUDIO_LengthOfCQueue(&media_sbc_queue));
    UNLOCK_APP_AUDIO_QUEUE();
    app_play_lineinmode_merge = 1;
    need_init_decoder = 1;
    media_Play_init_audio(status->aud_id);
  }
  return 0;
}

int app_play_audio_lineinmode_stop(APP_AUDIO_STATUS *status) {
  app_play_lineinmode_merge = 0;
  return 0;
}
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
#define DELAY_SAMPLE_MC (33 * 2) //  2:ch
static int32_t delay_buf_media[DELAY_SAMPLE_MC];
static uint32_t audio_mc_data_playback_media(uint8_t *buf,
                                             uint32_t mc_len_bytes) {
  uint32_t begin_time;
  // uint32_t end_time;
  begin_time = hal_sys_timer_get();
  TRACE_MEDIA_PLAYESTREAM_I("[PLAYBACK_MEDIA][MC] %d", begin_time);

  float left_gain;
  float right_gain;
  int32_t playback_len_bytes, mc_len_bytes_8;
  int32_t i, j, k;
  int delay_sample;

  mc_len_bytes_8 = mc_len_bytes / 8;

  hal_codec_get_dac_gain(&left_gain, &right_gain);

  // TRACE_MEDIA_PLAYESTREAM_I("playback_samplerate_ratio:
  // %d",playback_samplerate_ratio);

  // TRACE_MEDIA_PLAYESTREAM_I("left_gain:  %d",(int)(left_gain*(1<<12)));
  // TRACE_MEDIA_PLAYESTREAM_I("right_gain: %d",(int)(right_gain*(1<<12)));

  playback_len_bytes = mc_len_bytes / playback_samplerate_ratio_bt;

  if (sample_size_play_bt == AUD_BITS_16) {
    int16_t *sour_p = (int16_t *)(playback_buf_bt + playback_size_bt / 2);
    int16_t *mid_p = (int16_t *)(buf);
    int16_t *mid_p_8 = (int16_t *)(buf + mc_len_bytes - mc_len_bytes_8);
    int16_t *dest_p = (int16_t *)buf;

    if (buf == playback_buf_mc) {
      sour_p = (int16_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC;

    for (i = 0, j = 0; i < delay_sample; i = i + 2) {
      mid_p[j++] = delay_buf_media[i];
      mid_p[j++] = delay_buf_media[i + 1];
    }

    for (i = 0; i < playback_len_bytes / 2 - delay_sample; i = i + 2) {
      mid_p[j++] = sour_p[i];
      mid_p[j++] = sour_p[i + 1];
    }

    for (j = 0; i < playback_len_bytes / 2; i = i + 2) {
      delay_buf_media[j++] = sour_p[i];
      delay_buf_media[j++] = sour_p[i + 1];
    }

    if (playback_samplerate_ratio_bt <= 8) {
      for (i = 0, j = 0; i < playback_len_bytes / 2;
           i = i + 2 * (8 / playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
        mid_p_8[j++] = mid_p[i + 1];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 2; i = i + 2) {
        for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      }
    }

    anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain, right_gain,
                      AUD_BITS_16);

    for (i = 0, j = 0; i < (mc_len_bytes_8) / 2; i = i + 2) {
      for (k = 0; k < 8; k++) {
        dest_p[j++] = mid_p_8[i];
        dest_p[j++] = mid_p_8[i + 1];
      }
    }

  } else if (sample_size_play_bt == AUD_BITS_24) {
    int32_t *sour_p = (int32_t *)(playback_buf_bt + playback_size_bt / 2);
    int32_t *mid_p = (int32_t *)(buf);
    int32_t *mid_p_8 = (int32_t *)(buf + mc_len_bytes - mc_len_bytes_8);
    int32_t *dest_p = (int32_t *)buf;

    if (buf == (playback_buf_mc)) {
      sour_p = (int32_t *)playback_buf_bt;
    }

    delay_sample = DELAY_SAMPLE_MC;

    for (i = 0, j = 0; i < delay_sample; i = i + 2) {
      mid_p[j++] = delay_buf_media[i];
      mid_p[j++] = delay_buf_media[i + 1];
    }

    for (i = 0; i < playback_len_bytes / 4 - delay_sample; i = i + 2) {
      mid_p[j++] = sour_p[i];
      mid_p[j++] = sour_p[i + 1];
    }

    for (j = 0; i < playback_len_bytes / 4; i = i + 2) {
      delay_buf_media[j++] = sour_p[i];
      delay_buf_media[j++] = sour_p[i + 1];
    }

    if (playback_samplerate_ratio_bt <= 8) {
      for (i = 0, j = 0; i < playback_len_bytes / 4;
           i = i + 2 * (8 / playback_samplerate_ratio_bt)) {
        mid_p_8[j++] = mid_p[i];
        mid_p_8[j++] = mid_p[i + 1];
      }
    } else {
      for (i = 0, j = 0; i < playback_len_bytes / 4; i = i + 2) {
        for (k = 0; k < playback_samplerate_ratio_bt / 8; k++) {
          mid_p_8[j++] = mid_p[i];
          mid_p_8[j++] = mid_p[i + 1];
        }
      }
    }

    anc_mc_run_stereo((uint8_t *)mid_p_8, mc_len_bytes_8, left_gain, right_gain,
                      AUD_BITS_24);

    for (i = 0, j = 0; i < (mc_len_bytes_8) / 4; i = i + 2) {
      for (k = 0; k < 8; k++) {
        dest_p[j++] = mid_p_8[i];
        dest_p[j++] = mid_p_8[i + 1];
      }
    }
  }

  //  end_time = hal_sys_timer_get();

  //   TRACE_MEDIA_PLAYESTREAM_I("%s:run time: %d", __FUNCTION__,
  //   end_time-begin_time);

  return 0;
}
#endif

void app_audio_playback_done(void) {
#if defined(IBRT)
  app_tws_sync_prompt_check();
#endif
}

int app_play_audio_stop(void) {
  TRACE_MEDIA_PLAYESTREAM_I("[STOP]");

  app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
  return 0;
}

uint32_t g_active_aud_id = MAX_RECORD_NUM;

int app_play_audio_set_aud_id(uint32_t aud_id) {
  g_active_aud_id = aud_id;
  return 0;
}

int app_play_audio_get_aud_id(void) { return g_active_aud_id; }

void app_play_audio_set_lang(int L) { g_language = L; }

int app_play_audio_get_lang() { return g_language; }
extern struct BT_DEVICE_T app_bt_device;
int app_play_audio_onoff(bool onoff, APP_AUDIO_STATUS *status) {
  static bool isRun = false;

  struct AF_STREAM_CONFIG_T stream_cfg;
  uint8_t *bt_audio_buff = NULL;
  uint16_t bytes_parsed = 0;
  enum AUD_SAMPRATE_T sample_rate POSSIBLY_UNUSED = AUD_SAMPRATE_16000;
  uint16_t aud_id = 0;
  uint16_t aud_pram = 0;

  TRACE_MEDIA_PLAYESTREAM_I("[MEDIA_ONOFF] %s, to %s it",
                            isRun ? "Running" : "Idle",
                            onoff ? "start" : "stop");

  if (isRun == onoff) {
    return 0;
  }
  if (onoff) {

#ifdef WL_DET
    app_mic_alg_audioloop(false, APP_SYSFREQ_78M);
#endif

#if defined(__AI_VOICE__) || defined(BISTO_ENABLED)
    app_ai_if_inform_music_or_prompt_status(true, AUD_SAMPRATE_16000);
    app_ai_if_pre_music_or_prompt_check();
#endif

#if defined(__THIRDPARTY)
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_STOP);
#endif
    aud_id = PROMPT_ID_FROM_ID_VALUE(status->aud_id);
    aud_pram = PROMPT_PRAM_FROM_ID_VALUE(status->aud_id);
    g_prompt_chnlsel = PROMPT_CHNLSEl_FROM_ID_VALUE(aud_pram);
    TRACE_MEDIA_PLAYESTREAM_I(
        "[MEDIA_ONOFF] aud_id:%04x %s aud_pram:%04x chnlsel:%d", status->aud_id,
        aud_id2str(aud_id), aud_pram, g_prompt_chnlsel);

    media_Play_init_audio(aud_id);
    app_play_audio_set_aud_id(aud_id);
    if (!g_app_audio_length) {
      app_audio_sendrequest(APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE,
                            0);
      return 0;
    }

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
#else
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
#endif
    af_set_priority(AF_USER_AUDIO, osPriorityHigh);
    app_audio_mempool_init_with_specific_size(APP_AUDIO_BUFFER_SIZE);
    media_audio_init();

#if (defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)) &&       \
    defined(ENHANCED_STACK)
    if (app_bt_device.src_or_snk == BT_DEVICE_SRC) {
      app_a2dp_source_init();
    }
#endif

    btif_sbc_init_decoder(media_sbc_decoder);
    btif_sbc_decode_frames(media_sbc_decoder, g_app_audio_data,
                           g_app_audio_length, &bytes_parsed, NULL, 0, NULL);
    switch (media_sbc_decoder->streamInfo.sampleFreq) {
    case BTIF_SBC_CHNL_SAMPLE_FREQ_16:
      sample_rate = AUD_SAMPRATE_16000;
      break;
    case BTIF_SBC_CHNL_SAMPLE_FREQ_32:
      sample_rate = AUD_SAMPRATE_32000;
      break;
    case BTIF_SBC_CHNL_SAMPLE_FREQ_44_1:
      sample_rate = AUD_SAMPRATE_44100;
      break;
    case BTIF_SBC_CHNL_SAMPLE_FREQ_48:
      sample_rate = AUD_SAMPRATE_48000;
      break;
    default:
      sample_rate = AUD_SAMPRATE_16000;
      break;
    }

#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_disable();
#endif

    memset(&stream_cfg, 0, sizeof(stream_cfg));

#if defined(MEDIA_PLAY_24BIT)
    stream_cfg.bits = AUD_BITS_24;
#else
    stream_cfg.bits = AUD_BITS_16;
#endif
    stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#ifdef __BT_ANC__
    stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#else
    stream_cfg.sample_rate = AUD_SAMPRATE_16000;
#endif
#ifdef PLAYBACK_USE_I2S
    stream_cfg.device = AUD_STREAM_USE_I2S0_MASTER;
    stream_cfg.io_path = AUD_IO_PATH_NULL;
#else
    stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
#endif
#ifdef __INTERACTION__
    if (aud_id == AUD_ID_BT_FINDME) {
      stream_cfg.vol = g_findme_fadein_vol;
    } else
#endif
    {
      stream_cfg.vol = TGT_VOLUME_LEVEL_WARNINGTONE;
    }
    stream_cfg.handler = app_play_sbc_more_data;

    stream_cfg.data_size = APP_AUDIO_PLAYBACK_BUFF_SIZE;

#if defined(MEDIA_PLAY_24BIT)
    stream_cfg.data_size *= 2;
#endif

    g_cache_buff_sz = stream_cfg.data_size / 2 / 2;

    app_audio_mempool_get_buff((uint8_t **)&app_play_sbc_cache,
                               g_cache_buff_sz);
    app_audio_mempool_get_buff(&bt_audio_buff, stream_cfg.data_size);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);

#ifdef __BT_ANC__
    bt_sco_samplerate_ratio = 3;
    us_fir_init();
#endif

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
    sample_size_play_bt = stream_cfg.bits;
    sample_rate_play_bt = stream_cfg.sample_rate;
    data_size_play_bt = stream_cfg.data_size;
    playback_buf_bt = stream_cfg.data_ptr;
    playback_size_bt = stream_cfg.data_size;
#ifdef __BT_ANC__
    playback_samplerate_ratio_bt = 8;
#else
    playback_samplerate_ratio_bt = 8 * 3;
#endif
    playback_ch_num_bt = stream_cfg.channel_num;
#endif
    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
    stream_cfg.bits = sample_size_play_bt;
    stream_cfg.channel_num = playback_ch_num_bt;
    stream_cfg.sample_rate = sample_rate_play_bt;
    stream_cfg.device = AUD_STREAM_USE_MC;
    stream_cfg.vol = 0;
    stream_cfg.handler = audio_mc_data_playback_media;
    stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

    app_audio_mempool_get_buff(
        &bt_audio_buff, data_size_play_bt * playback_samplerate_ratio_bt);
    stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(bt_audio_buff);
    stream_cfg.data_size = data_size_play_bt * playback_samplerate_ratio_bt;

    playback_buf_mc = stream_cfg.data_ptr;
    playback_size_mc = stream_cfg.data_size;

    anc_mc_run_init(hal_codec_anc_convert_rate(sample_rate_play_bt));
    af_stream_open(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK, &stream_cfg);
#endif

#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_NUM, 0);
#endif

#if defined(IBRT)
    app_ibrt_voice_report_trigger_init(aud_id, aud_pram);
#endif
    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
    af_stream_start(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif

  } else {

#ifdef VOICE_DATAPATH
    app_voicepath_set_stream_state(AUDIO_OUTPUT_STREAMING, false);
    app_voicepath_set_pending_started_stream(AUDIO_OUTPUT_STREAMING, false);
#endif
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_PLAYBACK_RESAMPLE))
    af_codec_tune(AUD_STREAM_PLAYBACK, 0);
#endif

    af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

#if defined(AUDIO_ANC_FB_MC_MEDIA) && defined(ANC_APP) &&                      \
    !defined(__AUDIO_RESAMPLE__)
    af_stream_stop(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
    af_stream_close(AUD_STREAM_ID_2, AUD_STREAM_PLAYBACK);
#endif
#if defined(IBRT)
    app_ibrt_voice_report_trigger_deinit();
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
    audio_prompt_stop_playing();
#endif
#endif
#ifdef PLAYBACK_USE_I2S
    hal_cmu_audio_resample_enable();
#endif

    app_play_sbc_cache = NULL;
    g_cache_buff_sz = 0;
    g_prompt_chnlsel = PROMOT_ID_BIT_MASK_CHNLSEl_ALL;
    app_play_audio_set_aud_id(MAX_RECORD_NUM);
    af_set_priority(AF_USER_AUDIO, osPriorityAboveNormal);
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);

    app_audio_playback_done();
#if defined(__THIRDPARTY)
    app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_NO1,
                                             THIRDPARTY_START);
#endif

#ifdef WL_DET
    app_mic_alg_audioloop(true, APP_SYSFREQ_78M);
#endif
  }
  isRun = onoff;

  return 0;
}

static void app_stop_local_prompt_playing(void) {
  app_audio_sendrequest(APP_PLAY_BACK_AUDIO, APP_BT_SETTING_CLOSE, 0);
#ifdef MIX_AUDIO_PROMPT_WITH_A2DP_MEDIA_ENABLED
  audio_prompt_stop_playing();
#endif
}

void app_stop_both_prompt_playing(void) {
  app_stop_local_prompt_playing();
  app_tws_stop_peer_prompt();
}

void app_tws_cmd_stop_prompt_handler(uint8_t *ptrParam, uint16_t paramLen) {
  TRACE_MEDIA_PLAYESTREAM_I("[STOP_PROMPT_HANDLER]");
  app_stop_local_prompt_playing();
}
#endif
