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
#include "aud_section.h"
#include "bt_sco_chain_cfg.h"
#include "hal_trace.h"

static bool speech_tuning_status = false;

extern int speech_store_config(const SpeechConfig *cfg);

#ifdef AUDIO_SECTION_ENABLE
typedef struct {
  uint8_t reserved[AUDIO_SECTION_CFG_RESERVED_LEN];
  SpeechConfig cfg;
} AUDIO_SECTION_SPEECH_CFG_T;

static AUDIO_SECTION_SPEECH_CFG_T audio_section_speech_cfg;

int store_speech_cfg_into_audio_section(SpeechConfig *cfg) {
  int res = 0;

  memcpy(&audio_section_speech_cfg.cfg, cfg, sizeof(SpeechConfig));

  res = audio_section_store_cfg(AUDIO_SECTION_DEVICE_SPEECH,
                                (uint8_t *)&audio_section_speech_cfg,
                                sizeof(AUDIO_SECTION_SPEECH_CFG_T));

  if (res) {
    TRACE(2, "[%s] ERROR: res = %d", __func__, res);
  } else {
    TRACE(1, "[%s] Store speech cfg into audio section!!!", __func__);
  }

  return res;
}

void *load_speech_cfg_from_audio_section(void) {
  int res = 0;
  res = audio_section_load_cfg(AUDIO_SECTION_DEVICE_SPEECH,
                               (uint8_t *)&audio_section_speech_cfg,
                               sizeof(AUDIO_SECTION_SPEECH_CFG_T));

  void *res_ptr = NULL;

  if (res) {
    TRACE(2, "[%s] ERROR: res = %d", __func__, res);
    res_ptr = NULL;
  } else {
    TRACE(1, "[%s] Load speech cfg from audio section!!!", __func__);
    res_ptr = (void *)&audio_section_speech_cfg.cfg;
  }

  return res_ptr;
}
#endif

int speech_tuning_set_status(bool en) {
  speech_tuning_status = en;

  return 0;
}

bool speech_tuning_get_status(void) { return speech_tuning_status; }

uint32_t speech_tuning_check(unsigned char *buf, uint32_t len) {
  uint32_t res = 0;

  // Check valid
  uint32_t config_size = sizeof(SpeechConfig);

  if (config_size != len) {
    TRACE(2, "[speech tuning] len(%d) != config_size(%d)", len, config_size);
    res = 1;
  } else {
    TRACE(1, "[speech tuning] len(%d) is OK", len);
    // SpeechConfig POSSIBLY_UNUSED *cfg = (SpeechConfig *)buf;

    // Test parameters
    //#if defined(SPEECH_TX_2MIC_NS2)
    //        TRACE(1,"[speech tuning] TX: delay_taps(x100): %d",
    //        (int)(cfg->tx_2mic_ns2.delay_taps * 100));
    //#endif
    //#if defined(SPEECH_TX_NOISE_GATE)
    //        TRACE(1,"[speech tuning] TX: data_threshold: %d",
    //        cfg->tx_noise_gate.data_threshold);
    //#endif
    //#if defined(SPEECH_TX_EQ)
    //        TRACE(1,"[speech tuning] TX: eq num: %d", cfg->tx_eq.num);
    //#endif
    //#if defined(SPEECH_RX_EQ)
    //        TRACE(1,"[speech tuning] RX: eq num: %d", cfg->rx_eq.num);
    //#endif
  }

  return res;
}

uint32_t speech_tuning_rx_callback(unsigned char *buf, uint32_t len) {
  uint32_t res = 0;

  res = speech_tuning_check(buf, len);

  if (res) {
    TRACE(1, "[speech tuning] ERROR: Send check res = %d", res);
    TRACE(0, "[Speech Tuning] res : 1; info : Send len(%d) != config_size(%d);",
          len, sizeof(SpeechConfig));
  } else {
    // Save cfg
    speech_store_config((SpeechConfig *)buf);

    // Set status
    speech_tuning_set_status(true);

    TRACE(0, "[speech tuning] OK: Send cfg");
    TRACE(0, "[Speech Tuning] res : 0;");
  }

  return res;
}

#ifdef AUDIO_SECTION_ENABLE
uint32_t speech_tuning_burn_rx_callback(unsigned char *buf, uint32_t len) {
  uint32_t res = 0;

  res = speech_tuning_check(buf, len);

  if (res) {
    TRACE(1, "[speech tuning] ERROR: Burn check res = %d", res);
    TRACE(0, "[Speech Tuning] res : 1; info : Burn len(%d) != config_size(%d);",
          len, sizeof(SpeechConfig));
  } else {
    res = store_speech_cfg_into_audio_section((SpeechConfig *)buf);

    if (res) {
      TRACE(1, "[speech tuning] ERROR: Store res = %d", res);
      res += 100;
      TRACE(0, "[Speech Tuning] res : 2; info : Do not enable "
               "AUDIO_SECTION_ENABLE;");
    } else {
      TRACE(0, "[speech tuning] OK: Store cfg");
      TRACE(0, "[Speech Tuning] res : 0;");
    }
  }

  return res;
}
#endif

int speech_tuning_init(void) {
#if defined(HAL_TRACE_RX_ENABLE) && !defined(SPEECH_TX_THIRDPARTY)
  hal_trace_rx_register("Speech Tuning",
                        (HAL_TRACE_RX_CALLBACK_T)speech_tuning_rx_callback);

#ifdef AUDIO_SECTION_ENABLE
  hal_trace_rx_register(
      "Speech Tuning Burn",
      (HAL_TRACE_RX_CALLBACK_T)speech_tuning_burn_rx_callback);
#endif

#endif

  speech_tuning_set_status(false);

  return 0;
}

int speech_tuning_open(void) {
#ifdef AUDIO_SECTION_ENABLE
  SpeechConfig *speech_cfg_load = NULL;

  speech_cfg_load = (SpeechConfig *)load_speech_cfg_from_audio_section();

  if (speech_cfg_load) {
    speech_store_config(speech_cfg_load);
  }
#endif

  speech_tuning_set_status(false);

  return 0;
}

int speech_tuning_close(void) {
  speech_tuning_set_status(false);

  return 0;
}
