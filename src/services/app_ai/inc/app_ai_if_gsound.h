#ifndef APP_AI_IF_GSOUND_H_
#define APP_AI_IF_GSOUND_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ai_if_gsound_service_enable_switch
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    give the interface to AI to control gsound switch
 *
 * Parameters:
 *    onOff -- control gsound enable or not
 *
 * Return:
 *    void
 */
void app_ai_if_gsound_service_enable_switch(bool onOff);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_IF_GSOUND_H_

