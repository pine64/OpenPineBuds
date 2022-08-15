#ifndef APP_AI_ALGORITHM_H_
#define APP_AI_ALGORITHM_H_

//the ai aec data cache state
typedef enum {
    AI_AEC_CACHE_IDLE,
    AI_AEC_CACHE_CACHED,
    AI_AEC_CACHE_PROCESSED,
} AI_AEC_CACHE_STATE_E;

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ai_algorithm_mic_data_handle
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    use algorithm to handle the data that cuptrued from mic
 *
 * Parameters:
 *    uint8_t *buf
 *    uint32_t length
 *
 * Return:
 *    void
 */
void app_ai_algorithm_mic_data_handle(uint8_t *buf, uint32_t length);

/*---------------------------------------------------------------------------
 *            app_ai_algorithm_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    initialize AI algorithm
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_algorithm_init(void);

void cp_aec_init(void);
void cp_aec_deinit(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_ALGORITHM_H_

