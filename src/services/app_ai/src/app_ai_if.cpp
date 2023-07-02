#include "app_ai_if.h"
#include "app_ai_ble.h"
#include "app_ai_if_config.h"
#include "app_ai_if_custom_ui.h"
#include "app_ai_if_thirdparty.h"
#include "app_ai_manager_api.h"
#include "app_ai_tws.h"
#include "app_audio.h"
#include "app_dip.h"
#include "app_through_put.h"
#include "cmsis_os.h"
#include "dip_api.h"
#include "hal_trace.h"
#include "nvrecord_bt.h"

#ifdef BLE_ENABLE
#include "app_ble_mode_switch.h"
#endif

#ifdef __AI_VOICE__
#include "ai_control.h"
#include "ai_manager.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#endif

#ifdef BISTO_ENABLED
#include "gsound_custom.h"
#include "gsound_custom_bt.h"
#endif

#ifdef ANC_APP
#include "app_anc.h"
#endif

#define CASE_S(s)                                                              \
  case s:                                                                      \
    return "[" #s "]";
#define CASE_D()                                                               \
  default:                                                                     \
    return "[INVALID]";

AI_CAPTURE_BUFFER_T app_ai_if_capture_buf;

const char *ai_spec_type2str(AI_SPEC_TYPE_E ai_spec) {
  switch (ai_spec) {
    CASE_S(AI_SPEC_INIT)
    CASE_S(AI_SPEC_GSOUND)
    CASE_S(AI_SPEC_AMA)
    CASE_S(AI_SPEC_BES)
    CASE_S(AI_SPEC_BAIDU)
    CASE_S(AI_SPEC_TENCENT)
    CASE_S(AI_SPEC_ALI)
    CASE_S(AI_SPEC_COMMON)
    CASE_D()
  }
}

void app_ai_key_event_handle(void *param1, uint32_t param2) {
#if defined(__AI_VOICE__)

  if (app_ai_dont_have_mobile_connect()) {
    TRACE(1, "%s no mobile connected", __func__);
    return;
  }

#ifdef PUSH_AND_HOLD_ENABLED
  app_ai_set_wake_up_type(TYPE__PRESS_AND_HOLD);
#else
  app_ai_set_wake_up_type(TYPE__TAP);
#endif

  if (0 == ai_function_handle(CALLBACK_KEY_EVENT_HANDLE, param1, param2)) {
    if (app_ai_is_use_thirdparty()) {
      app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS,
                                        THIRDPARTY_AI_PROVIDE_SPEECH);
    }
  }
#endif
}

bool ai_if_is_ai_stream_mic_open(void) {
  bool mic_open = false;

#ifdef __AI_VOICE__
  mic_open = app_ai_is_stream_mic_open();
#endif

  return mic_open;
}

uint8_t app_ai_if_get_ble_connection_index(void) {
  uint8_t ble_connection_index = 0xFF;

#ifdef __AI_VOICE__
  ble_connection_index = app_ai_ble_get_connection_index();
#endif

  return ble_connection_index;
}

uint32_t app_ai_if_get_ai_spec(void) {
  uint32_t ai_spec = AI_SPEC_INIT;

#ifdef __AMA_VOICE__
  ai_spec = AI_SPEC_AMA;
#elif defined(__DMA_VOICE__)
  ai_spec = AI_SPEC_BAIDU;
#elif defined(__SMART_VOICE__)
  ai_spec = AI_SPEC_BES;
#elif defined(__TENCENT_VOICE__)
  ai_spec = AI_SPEC_TENCENT;
#elif defined(__GMA_VOICE__)
  ai_spec = AI_SPEC_ALI;
#elif defined(__CUSTOMIZE_VOICE__)
  ai_spec = AI_SPEC_COMMON;
#elif defined(__DUAL_MIC_RECORDING__)
  ai_spec = AI_SPEC_RECORDING;
#endif

  return ai_spec;
}

static POSSIBLY_UNUSED void app_ai_if_mempool_init(void) {
  uint8_t *buf = NULL;
  app_capture_audio_mempool_get_buff(&buf, APP_AI_IF_CAPTURE_BUFFER_SIZE);

  memset((uint8_t *)buf, 0, APP_AI_IF_CAPTURE_BUFFER_SIZE);

  app_ai_if_capture_buf.buff = buf;
  app_ai_if_capture_buf.buff_size_total = APP_AI_IF_CAPTURE_BUFFER_SIZE;
  app_ai_if_capture_buf.buff_size_used = 0;
  app_ai_if_capture_buf.buff_size_free = APP_AI_IF_CAPTURE_BUFFER_SIZE;

  TRACE(3, "%s buf %p size %d", __func__, buf, APP_AI_IF_CAPTURE_BUFFER_SIZE);
}

void app_ai_if_mempool_deinit(void) {
  TRACE(2, "%s size %d", __func__, app_ai_if_capture_buf.buff_size_total);

  memset((uint8_t *)app_ai_if_capture_buf.buff, 0,
         app_ai_if_capture_buf.buff_size_total);

  app_ai_if_capture_buf.buff_size_used = 0;
  app_ai_if_capture_buf.buff_size_free = app_ai_if_capture_buf.buff_size_total;
}

void app_ai_if_mempool_get_buff(uint8_t **buff, uint32_t size) {
  uint32_t buff_size_free;

  buff_size_free = app_ai_if_capture_buf.buff_size_free;

  if (size % 4) {
    size = size + (4 - size % 4);
  }

  TRACE(3, "%s free %d to allocate %d", __func__, buff_size_free, size);

  ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__,
         size, buff_size_free);

  *buff = app_ai_if_capture_buf.buff + app_ai_if_capture_buf.buff_size_used;

  app_ai_if_capture_buf.buff_size_used += size;
  app_ai_if_capture_buf.buff_size_free -= size;

  TRACE(3, "AI allocate %d, now used %d left %d", size,
        app_ai_if_capture_buf.buff_size_used,
        app_ai_if_capture_buf.buff_size_free);
}

void app_ai_if_mobile_connect_handle(bt_bdaddr_t *_addr) {
  MOBILE_CONN_TYPE_E type = MOBILE_CONNECT_IDLE;
  uint16_t vend_id = 0;
  uint16_t vend_id_source = 0;

  btif_dip_get_record_vend_id_and_source(_addr, &vend_id, &vend_id_source);
  TRACE(3, "%s vend_id 0x%x vend_id_source 0x%x", __func__, vend_id,
        vend_id_source);
  if (vend_id) {
    type = btif_dip_check_is_ios_by_vend_id(vend_id, vend_id_source)
               ? MOBILE_CONNECT_IOS
               : MOBILE_CONNECT_ANDROID;
#ifdef __AI_VOICE__
    app_ai_mobile_connect_handle(type, _addr);
#endif
#ifdef BISTO_ENABLED
    gsound_custom_bt_link_connected_handler(_addr->address);
    gsound_mobile_type_get_callback(type);
#endif
  }

#ifdef BLE_ENABLE
  app_ble_refresh_adv_state(BLE_ADVERTISING_INTERVAL);
#endif
}

void app_ai_voice_init(void) {
#ifdef __AI_VOICE__
  uint32_t ai_spec = app_ai_if_get_ai_spec();

  app_ai_manager_init();
  if (ai_spec != AI_SPEC_INIT) {
    ai_open(ai_spec);
    app_ai_tws_init();

#ifdef OPUS_IN_OVERLAY
    app_ai_set_opus_in_overlay(true);
#endif

#if VOB_ENCODING_ALGORITHM == ENCODING_ALGORITHM_OPUS
    app_ai_set_encode_type(ENCODING_ALGORITHM_OPUS);
#elif VOB_ENCODING_ALGORITHM == ENCODING_ALGORITHM_SBC
    app_ai_set_encode_type(ENCODING_ALGORITHM_SBC);
#else
    app_ai_set_encode_type(NO_ENCODING);
#endif

#ifdef AI_32KBPS_VOICE
    app_ai_set_use_ai_32kbps_voice(true);
#endif

#ifdef NO_LOCAL_START_TONE
    app_ai_set_local_start_tone(false);
#endif

    app_ai_if_custom_init();

#ifdef __THROUGH_PUT__
    app_throughput_test_init();
#endif
  }

  app_capture_audio_mempool_init();

#ifdef __THIRDPARTY
  app_ai_if_thirdparty_init();
#endif

  app_ai_if_mempool_init();
#endif
}

static bool isMusicOrPromptOnGoing = false;
static uint32_t musicOrPromptSampleRate;

void app_ai_if_inform_music_or_prompt_status(bool isOn, uint32_t sampleRate) {
  isMusicOrPromptOnGoing = isOn;
  musicOrPromptSampleRate = sampleRate;
}

bool app_ai_if_is_music_or_prompt_ongoing(void) {
#ifdef ANC_APP
  return isMusicOrPromptOnGoing || app_anc_is_on();
#else
  return isMusicOrPromptOnGoing;
#endif
}

uint32_t app_ai_if_get_music_or_prompt_sample_rate(void) {
#ifdef ANC_APP
  if (isMusicOrPromptOnGoing) {
    return musicOrPromptSampleRate;
  } else if (app_anc_is_on()) {
    return app_anc_get_sample_rate();
  } else {
    return 0;
  }
#else
  return musicOrPromptSampleRate;
#endif
}

extern void alexa_wwe_pre_music_or_prompt_check(void);
extern "C" void app_voicepath_pre_music_or_prompt_check(void);

void app_ai_if_pre_music_or_prompt_check(void) {
#ifdef __ALEXA_WWE_LITE
  alexa_wwe_pre_music_or_prompt_check();
#endif

#ifdef BISTO_ENABLED
  app_voicepath_pre_music_or_prompt_check();
#endif
}
