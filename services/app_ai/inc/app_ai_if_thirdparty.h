#ifndef APP_AI_IF_THIRDPARTY_H_
#define APP_AI_IF_THIRDPARTY_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_thirdparty.h"

#if defined(__ALEXA_WWE_LITE)
#ifdef VOICE_DETECTOR_EN
#if defined(CHIP_BEST2300A)
#define APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE   (127*1024)
#else
#define APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE   (111*1024)
#endif
#else
#define APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE   (99*1024)
#endif
#elif defined(GSOUND_HOTWORD_EXTERNAL)
#define APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE   (68*1024)
#else
#define APP_AI_IF_THIRDPARTY_MEMPOOL_BUFFER_SIZE   (0)
#endif

typedef struct{
    uint8_t *buff;
    uint32_t buff_size_total;
    uint32_t buff_size_used;
    uint32_t buff_size_free;
}THIRDPARTY_AI_MEMPOOL_BUFFER_T;

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
*            app_ai_if_thirdparty_mempool_init
*---------------------------------------------------------------------------
*
*Synopsis:
*    init ai voice thirdparty mempool
*
* Parameters:
*    void
*
* Return:
*    void
*/
POSSIBLY_UNUSED void app_ai_if_thirdparty_mempool_init(void);

/*---------------------------------------------------------------------------
*            app_ai_if_thirdparty_mempool_deinit
*---------------------------------------------------------------------------
*
*Synopsis:
*    init ai voice thirdparty mempool
*
* Parameters:
*    void
*
* Return:
*    void
*/
void app_ai_if_thirdparty_mempool_deinit(void);

/*---------------------------------------------------------------------------
*            app_ai_if_thirdparty_mempool_get_buff
*---------------------------------------------------------------------------
*
*Synopsis:
*    get buf form ai voice thirdparty mempool
*
* Parameters:
*    buff -- the pointer of buf that get from mempool
*    size -- the size of buf that get from mempool
*
* Return:
*    void
*/
void app_ai_if_thirdparty_mempool_get_buff(uint8_t **buff, uint32_t size);

/*---------------------------------------------------------------------------
*            app_ai_if_thirdparty_start_stream_by_app_audio_manager
*---------------------------------------------------------------------------
*
*Synopsis:
*    start mic stream of thirdparty
*
* Parameters:
*    void
*
* Return:
*    void
*/
void app_ai_if_thirdparty_start_stream_by_app_audio_manager(void);

/*---------------------------------------------------------------------------
*            app_ai_if_thirdparty_stop_stream_by_app_audio_manager
*---------------------------------------------------------------------------
*
*Synopsis:
*    stop mic stream of thirdparty
*
* Parameters:
*    void
*
* Return:
*    void
*/
void app_ai_if_thirdparty_stop_stream_by_app_audio_manager(void);

/*---------------------------------------------------------------------------
 *            app_ai_if_thirdparty_event_handle
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for AI use thirdparty event handle
 *
 * Parameters:
 *    funcId -- function ID
 *    event_type event type ID
 *
 * Return:
 *    void
 */
int app_ai_if_thirdparty_event_handle(THIRDPARTY_FUNC_ID funcId, THIRDPARTY_EVENT_TYPE event_type);

/*---------------------------------------------------------------------------
 *            app_ai_if_thirdparty_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for AI init thirdparty
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_if_thirdparty_init(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_IF_THIRDPARTY_H_

