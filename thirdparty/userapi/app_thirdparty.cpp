#include "cmsis_os.h"
#include "cmsis.h"
#include "list.h"
#include "hal_trace.h"
#include <string.h>
#include "app_thirdparty.h"


APP_THIRDPARTY_HANDLE_TAB_T app_thirdparty_handle_table;

void app_thirdparty_callback_init(THIRDPARTY_CALLBACK_TYPE type, APP_THIRDPARTY_CUSTOM_CB_T cb)
{
    TRACE(2, "%s type %d", __func__, type);
    switch (type)
    {
        case THIRDPARTY_DATA_COME_CALLBACK:
            app_thirdparty_handle_table._app_thirdparty_data_come_callback = cb;
        break;
        case THIRDPARTY_WAKE_UP_CALLBACK:
            app_thirdparty_handle_table._app_thirdparty_wake_up_callback = cb;
        break;
        case THIRDPARTY_START_SPEECH_CALLBACK:
            app_thirdparty_handle_table._app_thirdparty_start_callback = cb;
        break;
        case THIRDPARTY_STOP_SPEECH_CALLBACK:
            app_thirdparty_handle_table._app_thirdparty_stop_callback = cb;
        break;
        default:
        break;
    }
}

uint32_t app_thirdparty_callback_handble(THIRDPARTY_CALLBACK_TYPE type, void* param1, uint32_t param2)
{
    uint32_t ret = 0xFFFFFFFF;
    //TRACE(2, "%s type %d", __func__, type);

    switch (type)
    {
        case THIRDPARTY_DATA_COME_CALLBACK:
            if (app_thirdparty_handle_table._app_thirdparty_data_come_callback)
            {
                ret = app_thirdparty_handle_table._app_thirdparty_data_come_callback(param1, param2);
            }
        break;
        case THIRDPARTY_WAKE_UP_CALLBACK:
            if (app_thirdparty_handle_table._app_thirdparty_wake_up_callback)
            {
                ret = app_thirdparty_handle_table._app_thirdparty_wake_up_callback(param1, param2);
            }
        break;
        case THIRDPARTY_START_SPEECH_CALLBACK:
            if (app_thirdparty_handle_table._app_thirdparty_start_callback)
            {
                ret = app_thirdparty_handle_table._app_thirdparty_start_callback(param1, param2);
            }
        break;
        case THIRDPARTY_STOP_SPEECH_CALLBACK:
            if (app_thirdparty_handle_table._app_thirdparty_stop_callback)
            {
                ret = app_thirdparty_handle_table._app_thirdparty_stop_callback(param1, param2);
            }
        break;
        default:
        break;
    }

    return ret;
}

int app_thirdparty_specific_lib_event_handle(THIRDPARTY_FUNC_ID funcId,THIRDPARTY_EVENT_TYPE event_type)
{
    TRACE(3, "%s funcId=%d event_type=0x%x", __func__, funcId, event_type);
    const APP_THIRDPARTY_HANDLE *dest_handle;

    if (app_thirdparty_handle_table.thirdparty_func_id == funcId)
    {
        for (uint32_t index = 0;
            index < app_thirdparty_handle_table.thirdparty_handler_cnt;
            index++)
        {
            dest_handle = &app_thirdparty_handle_table.thirdparty_handler_tab[index];
            if (dest_handle->thirdparty_sign.event == event_type)
            {
                TRACE(1,"find index=%d",index);
                if (dest_handle->function)
                {
                    return dest_handle->function(dest_handle->status, dest_handle->param);
                }
            }
        }
    }

    return 0;
}


void app_thirdparty_init(void)
{
#ifdef __ALEXA_WWE
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(ALEXA_WWE_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(ALEXA_WWE_LIB_NAME);
#elif defined(__KWS_ALEXA__)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(KWS_ALEXA_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(KWS_ALEXA_LIB_NAME);
#elif defined(__CYBERON)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(CYBERON_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(CYBERON_LIB_NAME);
#elif defined(__KNOWLES)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(KNOWLES_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(KNOWLES_LIB_NAME);
#elif defined(ANC_NOISE_TRACKER)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(NOISE_TRACKER_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(NOISE_TRACKER_LIB_NAME);
#elif defined(_VOICESPOT_)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(VOICESPOT_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(VOICESPOT_LIB_NAME);
#elif defined(__BIXBY)
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(BIXBY_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(BIXBY_NAME);
#else
    app_thirdparty_handle_table.thirdparty_handler_tab = THIRDPARTY_GET_HANDLER_TAB(DEMO_LIB_NAME);
    app_thirdparty_handle_table.thirdparty_handler_cnt = THIRDPARTY_GET_HANDLER_TAB_SIZE(DEMO_LIB_NAME);
#endif

    app_thirdparty_handle_table.thirdparty_func_id = app_thirdparty_handle_table.thirdparty_handler_tab[0].thirdparty_sign.func_id;
    app_thirdparty_handle_table.thirdparty_lib_id = app_thirdparty_handle_table.thirdparty_handler_tab[0].thirdparty_sign.lib_id;

    TRACE(3, "%s cnt=%d id=%d", __func__, app_thirdparty_handle_table.thirdparty_handler_cnt, app_thirdparty_handle_table.thirdparty_lib_id);
}

