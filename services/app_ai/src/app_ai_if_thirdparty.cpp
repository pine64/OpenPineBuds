#include "cmsis_os.h"
#include "spp_api.h"
#include "hal_trace.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"
#include "app_ai_if_thirdparty.h"
#include "app_thirdparty.h"
#include "app_bt_media_manager.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_control.h"
#include "ai_thread.h"
#include "app_ai_voice.h"
#endif

uint8_t app_ai_if_thirdparty_mempool_buf[APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE];
THIRDPARTY_AI_MEMPOOL_BUFFER_T app_ai_if_thirdparty_mempool_buf_t;

POSSIBLY_UNUSED void app_ai_if_thirdparty_mempool_init(void)
{
    memset((uint8_t *)app_ai_if_thirdparty_mempool_buf, 0, APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE);

    app_ai_if_thirdparty_mempool_buf_t.buff = app_ai_if_thirdparty_mempool_buf;
    app_ai_if_thirdparty_mempool_buf_t.buff_size_total = APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE;
    app_ai_if_thirdparty_mempool_buf_t.buff_size_used = 0;
    app_ai_if_thirdparty_mempool_buf_t.buff_size_free = APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE;

    TRACE(3, "%s buf %p size %d", __func__, app_ai_if_thirdparty_mempool_buf, APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE);
}

void app_ai_if_thirdparty_mempool_deinit(void)
{
    TRACE(2, "%s size %d", __func__, app_ai_if_thirdparty_mempool_buf_t.buff_size_total);

    memset((uint8_t *)app_ai_if_thirdparty_mempool_buf_t.buff, 0, app_ai_if_thirdparty_mempool_buf_t.buff_size_total);

    app_ai_if_thirdparty_mempool_buf_t.buff_size_used = 0;
    app_ai_if_thirdparty_mempool_buf_t.buff_size_free = app_ai_if_thirdparty_mempool_buf_t.buff_size_total;
}

void app_ai_if_thirdparty_mempool_get_buff(uint8_t **buff, uint32_t size)
{
    uint32_t buff_size_free;

    buff_size_free = app_ai_if_thirdparty_mempool_buf_t.buff_size_free;

    if (size % 4){
        size = size + (4 - size % 4);
    }

    TRACE(3,"%s free %d to allocate %d", __func__, buff_size_free, size);

    ASSERT(size <= buff_size_free, "[%s] size = %d > free size = %d", __func__, size, buff_size_free);

    *buff = app_ai_if_thirdparty_mempool_buf_t.buff + app_ai_if_thirdparty_mempool_buf_t.buff_size_used;

    app_ai_if_thirdparty_mempool_buf_t.buff_size_used += size;
    app_ai_if_thirdparty_mempool_buf_t.buff_size_free -= size;

    TRACE(3,"thirdparty allocate %d, now used %d left %d", size,
                    app_ai_if_thirdparty_mempool_buf_t.buff_size_used,
                    app_ai_if_thirdparty_mempool_buf_t.buff_size_free);
}

void app_ai_if_thirdparty_start_stream_by_app_audio_manager(void)
{
#ifdef __THIRDPARTY
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_START,
                                  BT_STREAM_THIRDPARTY_VOICE,
                                  0,
                                  0);
#endif
}

void app_ai_if_thirdparty_stop_stream_by_app_audio_manager(void)
{
#ifdef __THIRDPARTY
    app_audio_manager_sendrequest(APP_BT_STREAM_MANAGER_STOP,
                                  BT_STREAM_THIRDPARTY_VOICE,
                                  0,
                                  0);
#endif
}

int app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_ID funcId, THIRDPARTY_EVENT_TYPE event_type)
{
    int ret = 0;
#ifdef __THIRDPARTY
    ret = app_thirdparty_specific_lib_event_handle(funcId, event_type);
#endif
    return ret;
}

void app_ai_if_thirdparty_init(void)
{
#if defined(__THIRDPARTY) && defined(__AI_VOICE__)
    app_ai_set_use_thirdparty(true);
    app_ai_if_thirdparty_mempool_init();
    app_thirdparty_callback_init(THIRDPARTY_DATA_COME_CALLBACK, app_ai_voice_thirdparty_data_come_callback);
    app_thirdparty_callback_init(THIRDPARTY_WAKE_UP_CALLBACK, app_ai_voice_thirdparty_wake_up_callback);
    app_thirdparty_callback_init(THIRDPARTY_START_SPEECH_CALLBACK, app_ai_voice_thirdparty_start_speech_callback);
    app_thirdparty_callback_init(THIRDPARTY_STOP_SPEECH_CALLBACK, app_ai_voice_thirdparty_stop_speech_callback);
    app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_KWS, THIRDPARTY_INIT);
#endif
}

