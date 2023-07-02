#ifndef APP_AI_MANAGER_API_H_
#define APP_AI_MANAGER_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_ai_if.h"


#define MAI_TYPE_REBOOT_WITHOUT_OEM_APP

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ai_manager_is_in_multi_ai_mode
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to know ai manager is in multi ai mode or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true is in multi ai mode
 */
bool app_ai_manager_is_in_multi_ai_mode(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_voicekey_is_enable
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get the voicekey is enable or not
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true: voice key is enable
 */
bool app_ai_manager_voicekey_is_enable(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_voicekey_save_status
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager save voice key status
 *
 * Parameters:
 *    state -- the voice key status
 *
 * Return:
 *    void
 */
void app_ai_manager_voicekey_save_status(bool state);

/*---------------------------------------------------------------------------
 *            app_ai_manager_switch_spec
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager switch ai spec
 *
 * Parameters:
 *    ai_spec -- the ai spec want to switch
 *
 * Return:
 *    void
 */
void app_ai_manager_switch_spec(AI_SPEC_TYPE_E ai_spec);

/*---------------------------------------------------------------------------
 *            app_ai_manager_set_current_spec
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager set current ai spec
 *
 * Parameters:
 *    ai_spec -- the ai spec need to set
 *
 * Return:
 *    void
 */
void app_ai_manager_set_current_spec(AI_SPEC_TYPE_E ai_spec);

/*---------------------------------------------------------------------------
 *            app_ai_manager_get_current_spec
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager get current ai spec
 *
 * Parameters:
 *    bool
 *
 * Return:
 *    ai_spec -- the current ai spec
 */
uint8_t app_ai_manager_get_current_spec(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_is_need_reboot
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    check if ai manager need to reboot
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true is need to reboot
 */
bool app_ai_manager_is_need_reboot(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_enable
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager set a ai_spec enable or disable
 *
 * Parameters:
 *    isEnabled -- true is set the spec enable; false is disable
 *    ai_spec -- the ai spec you want to operate
 *
 * Return:
 *    void
 */
void app_ai_manager_enable(bool isEnabled, AI_SPEC_TYPE_E ai_spec);

/*---------------------------------------------------------------------------
 *            app_ai_manager_set_spec_connected_status
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager set a ai_spec connect status
 *
 * Parameters:
 *    ai_spec -- the ai spec you want to operate
 *    connected -- true is set the spec connected; false is disconnected
 *
 * Return:
 *    void
 */
void app_ai_manager_set_spec_connected_status(AI_SPEC_TYPE_E ai_spec, uint8_t connected);

/*---------------------------------------------------------------------------
 *            app_ai_manager_get_spec_connected_status
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager get a ai_spec connect status
 *
 * Parameters:
 *    ai_spec -- the ai spec you want to operate
 *
 * Return:
 *    int8_t -- 1 is the spec connected; 0 is disconnected; -1 is invalue
 */
int8_t app_ai_manager_get_spec_connected_status(uint8_t ai_spec);

/*---------------------------------------------------------------------------
 *            app_ai_manager_spec_get_status_is_in_invalid
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager get a ai_spec invalue status
 *
 * Parameters:
 *    ai_spec -- the ai spec you want to operate
 *
 * Return:
 *    bool -- true is the spec invalue; false is value
 */
bool app_ai_manager_spec_get_status_is_in_invalid(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_set_spec_update_flag
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager set spec update flag, when this flag seted, AI entry switch spec status.
 *    And then reboot
 *
 * Parameters:
 *    onOff -- true is AI entry switch spec status
 *
 * Return:
 *    void
 */
void app_ai_manager_set_spec_update_flag(bool onOff);

/*---------------------------------------------------------------------------
 *            app_ai_manager_get_spec_update_flag
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager get spec update flag
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true is AI entry switch spec status
 */
bool app_ai_manager_get_spec_update_flag(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_spec_update_start_reboot
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager update spec, start reboot now
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_manager_spec_update_start_reboot(void);

/*---------------------------------------------------------------------------
 *            app_ai_manager_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager init
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool
 */
void app_ai_manager_init(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_MANAGER_API_H_

