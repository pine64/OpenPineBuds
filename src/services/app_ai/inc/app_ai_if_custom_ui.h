#ifndef APP_AI_IF_CUSTOM_UI_H_
#define APP_AI_IF_CUSTOM_UI_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ai_if_custom_ui_send_cmd
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for custom to send cmd
 *
 * Parameters:
 *    cmd_buf -- cmd data buff
 *    cmd_len -- cmd data length
 *
 * Return:
 *    void
 */
bool app_ai_if_custom_ui_send_cmd(uint8_t *cmd_buf,uint16_t cmd_len);

/*---------------------------------------------------------------------------
 *            app_ai_if_custom_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init custom
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_if_custom_init(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_IF_CUSTOM_UI_H_

