#ifndef __APP_VOICE_DETECTOR_H__
#define __APP_VOICE_DETECTOR_H__
#include "hal_aud.h"
#include "audioflinger.h"
#include "voice_detector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* event of voice detector */
enum voice_detector_evt {
    VOICE_DET_EVT_IDLE,
    VOICE_DET_EVT_OPEN,
    VOICE_DET_EVT_VAD_START,
    VOICE_DET_EVT_AUD_CAP_START,
    VOICE_DET_EVT_CLOSE,
};

/*
 * Initialize voice detector.
 * This function should be called by main thread.
 */
void app_voice_detector_init();

/*
 * Open voice detector specified by a unique id.
 * This function must be callbed firstly.
 * return: 0(success) !0(fail)
 */
int app_voice_detector_open(enum voice_detector_id id, enum AUD_VAD_TYPE_T vad_type);

/*
 * Configure VAD by specified structure conf.
 * This function is optional to be called because of VAD also
 * can be initialized by default parameter.
 * return: 0(success) !0(fail)
 */
int app_voice_detector_setup_vad(enum voice_detector_id id,
                                struct AUD_VAD_CONFIG_T *conf);
/*
 * Setup capture(or playback) stream to voice detector.
 * The capture stream will be used to open or start audio device.
 * This function must be called at least once;
 * return: 0(success) !0(fail)
 */
int app_voice_detector_setup_stream(enum voice_detector_id id,
                                    enum AUD_STREAM_T stream_id,
                                    struct AF_STREAM_CONFIG_T *stream);
/*
 * Setup callback function to voice detector.
 * voice detector has several callbacks and normally we use func_id to specifiy
 * it's type.
 *
 * parameters:
 *     id:      voice detector identify number
 *     func_id: callback type identify number
 *
 *        VOICE_DET_CB_PREVIOUS:
 *           This callback will be invoked before any command executed;
 *        VOICE_DET_CB_RUN_WAIT:
 *           This callback will be invoked while command queue is empty;
 *        VOICE_DET_CB_RUN_DONE:
 *           This callback will be invoked while one command executed done;
 *        VOICE_DET_CB_POST:
 *           This callback will be invoked after all commands finished;
 *        VOICE_DET_CB_ERROR:
 *           This callback will be invoked while one command executed error;
 *        VOICE_DET_CB_APP:
 *           This callback will be invoked while VAD interrupt comes;
 *
 *     func:    callback subrotine
 *     param:   arguments of callback subrotine.
 * return: 0(success) !0(fail)
 */
int app_voice_detector_setup_callback(enum voice_detector_id id,
                                      enum voice_detector_cb_id func_id,
                                      voice_detector_cb_t func,
                                      void *param);
/*
 * Send events to voice detector.
 * This function maybe invoked in any thread.
 * return: 0(success) !0(fail)
 */
int app_voice_detector_send_event(enum voice_detector_id id,
                                enum voice_detector_evt evt);

/*
 * Close a voice detector specified by a unique id.
 */
void app_voice_detector_close(enum voice_detector_id id);

/*
 * change a voice detector specified by a unique id to capture
 */
void app_voice_detector_capture_start(enum voice_detector_id id);

/*
 * get vad data info by a unique id.
 */
void app_voice_detector_get_vad_data_info(enum voice_detector_id id, struct CODEC_VAD_BUF_INFO_T* vad_buf_info);

#ifdef __cplusplus
}
#endif

#endif /* __APP_VOICE_DETECTOR_H__ */
