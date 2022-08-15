#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_ai_if.h"
#include "app_ai_tws.h"
#include "app_ai_manager_api.h"

#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "ai_thread.h"
#endif

bool app_ai_manager_is_in_multi_ai_mode(void)
{
    bool ret = false;
#ifdef IS_MULTI_AI_ENABLED
    ret = true;
#endif
    return ret;
}

bool app_ai_manager_voicekey_is_enable(void)
{
    bool ret = true;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_voicekey_is_enable();
#endif
    return ret;
}

void app_ai_manager_voicekey_save_status(bool state)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_voicekey_save_status(state);
#endif
}

void app_ai_manager_switch_spec(AI_SPEC_TYPE_E ai_spec)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_switch_spec(ai_spec);
#endif
}

void app_ai_manager_set_current_spec(AI_SPEC_TYPE_E ai_spec)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_set_current_spec(ai_spec);
#endif
}

uint8_t app_ai_manager_get_current_spec(void)
{
    uint8_t ret = -1;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_manager_get_current_spec();
#endif
    return ret;
}

bool app_ai_manager_is_need_reboot(void)
{
    bool ret = false;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_manager_is_need_reboot();
#endif
    return ret;
}

void app_ai_manager_enable(bool isEnabled, AI_SPEC_TYPE_E ai_spec)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_enable(isEnabled, ai_spec);
#endif
}

void app_ai_manager_set_spec_connected_status(AI_SPEC_TYPE_E ai_spec, uint8_t connected)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_set_spec_connected_status(ai_spec, connected);
#endif
}

int8_t app_ai_manager_get_spec_connected_status(uint8_t ai_spec)
{
    uint8_t ret = -1;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_manager_get_spec_connected_status(ai_spec);
#endif
    return ret;
}

bool app_ai_manager_spec_get_status_is_in_invalid(void)
{
    bool ret = true;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_manager_spec_get_status_is_in_invalid();
#endif
    return ret;
}

void app_ai_manager_set_spec_update_flag(bool onOff)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_set_spec_update_flag(onOff);
#endif
}

bool app_ai_manager_get_spec_update_flag(void)
{
    bool ret = false;
#ifdef IS_MULTI_AI_ENABLED
    ret = ai_manager_get_spec_update_flag();
#endif
    return ret;
}

void app_ai_manager_spec_update_start_reboot(void)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_spec_update_start_reboot();
#endif
}

void app_ai_manager_init(void)
{
#ifdef IS_MULTI_AI_ENABLED
    ai_manager_init();
#endif
}


