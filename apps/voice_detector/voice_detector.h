#ifndef __VOICE_DETECTOR_H__
#define __VOICE_DETECTOR_H__
#include "hal_aud.h"
#include "audioflinger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* identify number of voice detector */
enum voice_detector_id {
    VOICE_DETECTOR_ID_0,
    VOICE_DETECTOR_QTY,
};

#if defined(CHIP_BEST2300P)
#define VAD_BUFFER_LEN 8192
#else
#define VAD_BUFFER_LEN 2048 
#endif

/* commands of voice detector */
enum voice_detector_cmd {
    VOICE_DET_CMD_IDLE,
    VOICE_DET_CMD_EXIT,
    VOICE_DET_CMD_VAD_OPEN,
    VOICE_DET_CMD_VAD_START,
    VOICE_DET_CMD_VAD_STOP,
    VOICE_DET_CMD_VAD_CLOSE,
    VOICE_DET_CMD_AUD_CAP_START,
    VOICE_DET_CMD_AUD_CAP_STOP,
    VOICE_DET_CMD_AUD_CAP_OPEN,
    VOICE_DET_CMD_AUD_CAP_CLOSE,
    VOICE_DET_CMD_SYS_CLK_32K,
    VOICE_DET_CMD_SYS_CLK_26M,
    VOICE_DET_CMD_SYS_CLK_52M,
    VOICE_DET_CMD_SYS_CLK_104M,
};

/* state of voice detector */
enum voice_detector_state {
    VOICE_DET_STATE_IDLE          = VOICE_DET_CMD_IDLE,
    VOICE_DET_STATE_EXIT          = VOICE_DET_CMD_EXIT,
    VOICE_DET_STATE_VAD_OPEN      = VOICE_DET_CMD_VAD_OPEN,
    VOICE_DET_STATE_VAD_START     = VOICE_DET_CMD_VAD_START,
    VOICE_DET_STATE_VAD_STOP      = VOICE_DET_CMD_VAD_STOP,
    VOICE_DET_STATE_VAD_CLOSE     = VOICE_DET_CMD_VAD_CLOSE,
    VOICE_DET_STATE_AUD_CAP_START = VOICE_DET_CMD_AUD_CAP_START,
    VOICE_DET_STATE_AUD_CAP_STOP  = VOICE_DET_CMD_AUD_CAP_STOP,
    VOICE_DET_STATE_AUD_CAP_OPEN  = VOICE_DET_CMD_AUD_CAP_OPEN,
    VOICE_DET_STATE_AUD_CAP_CLOSE = VOICE_DET_CMD_AUD_CAP_CLOSE,
    VOICE_DET_STATE_SYS_CLK_32K     = VOICE_DET_CMD_SYS_CLK_32K,
    VOICE_DET_STATE_SYS_CLK_26M     = VOICE_DET_CMD_SYS_CLK_26M,
    VOICE_DET_STATE_SYS_CLK_52M     = VOICE_DET_CMD_SYS_CLK_52M,
    VOICE_DET_STATE_SYS_CLK_104M    = VOICE_DET_CMD_SYS_CLK_104M,
};

/* running mode of voice detector */
enum voice_detector_mode {
    VOICE_DET_MODE_ONESHOT,
    VOICE_DET_MODE_EXEC_CMD,
    VOICE_DET_MODE_LOOP,
};

/* callback type of voice detector */
enum voice_detector_cb_id {
    VOICE_DET_CB_PREVIOUS,
    VOICE_DET_CB_RUN_WAIT,
    VOICE_DET_CB_RUN_DONE,
    VOICE_DET_CB_POST,
    VOICE_DET_CB_ERROR,
    VOICE_DET_CB_APP,
    VOICE_DET_FIND_APP,
    VOICE_DET_NOT_FIND_APP,
    VOICE_DET_CB_QTY,
};

/* callback function definition */
typedef void (*voice_detector_cb_t)(int current_state, void *arguments);

/* VAD default parameters */
#define VAD_DEFAULT_UDC         0x1//0xa
#define VAD_DEFAULT_UPRE        0x4
#define VAD_DEFAULT_SENSOR_FRAME_LEN    8       // 1.6K sample rate
#define VAD_DEFAULT_MIC_FRAME_LEN       0x50    // 16K sample rate
#define VAD_DEFAULT_MVAD        0x7
#define VAD_DEFAULT_DIG_MODE    0x1
#define VAD_DEFAULT_PRE_GAIN    0x4
#define VAD_DEFAULT_STH         0x10
#define VAD_DEFAULT_DC_BYPASS   0x0
#define VAD_DEFAULT_FRAME_TH0   0x32
#define VAD_DEFAULT_FRAME_TH1   0x1f4
#define VAD_DEFAULT_FRAME_TH2   0x1388
#define VAD_DEFAULT_RANGE0      0xf
#define VAD_DEFAULT_RANGE1      0x32
#define VAD_DEFAULT_RANGE2      0x96
#define VAD_DEFAULT_RANGE3      0x12c
#define VAD_DEFAULT_PSD_TH0     0x0
#define VAD_DEFAULT_PSD_TH1     0x07ffffff

/*
 * open voice detector specified by a unique id
 * must be callbed firstly
 * return: 0(success) !0(fail)
 */
int voice_detector_open(enum voice_detector_id id, enum AUD_VAD_TYPE_T vad_type);

/*
 * configure VAD by specified structure conf
 * Optional be called because of voice detector will initial vad by default parameter
 * return: 0(success) !0(fail)
 */
int voice_detector_setup_vad(enum voice_detector_id id, struct AUD_VAD_CONFIG_T *conf);

/*
 * setup capture(or playback) stream to voice detector. The capture stream will
 * be used to open or start audio device.
 * must be called before voice_detector_run();
 * return: 0(success) !0(fail)
 */
int voice_detector_setup_stream(enum voice_detector_id id,
        enum AUD_STREAM_T stream_id, struct AF_STREAM_CONFIG_T *stream);

/*
 * setup callback function to voice detector. This callback function will be called
 * when voice_detector_run() starts.
 * parameters:
 *     func_id: callback type id
 *     func:    callback fuction
 *     param:   the arguments of callback function.
 * return: 0(success) !0(fail)
 */

int voice_detector_setup_callback(enum voice_detector_id id,
        enum voice_detector_cb_id func_id, voice_detector_cb_t func, void *param);

/*
 * query the status of voice detector. Before send any command to voice detector
 * by invoke voice_detector_send_cmd(), the status should be queried by this
 * function.
 * return: the state of voice detector
 */
enum voice_detector_state voice_detector_query_status(enum voice_detector_id id);

/*
 * receive data from voice detector's VAD buffer.
 * return: The number of bytes received from VAD private buffer
 */
int voice_detector_recv_vad_data(enum voice_detector_id id,
                            uint8_t *pbuf, uint32_t buf_size);
/*
 * send commands to voice detector. These commands will be saved in a queue
 * before they are executed exactly.
 * return: 0(success) !0(fail)
 */
int voice_detector_send_cmd(enum voice_detector_id id, enum voice_detector_cmd cmd);

/*
 * send commands array to voice detector. These commands will be saved in a queue
 * before they are executed exactly.
 * return: 0(success) !0(fail)
 */
int voice_detector_send_cmd_array(enum voice_detector_id id, int *cmd_array, int num);

int voice_detector_enhance_perform(enum voice_detector_id id);

/*
 * This function is a special subrotine which should be invoked by a thread
 * It will execute these commands stored in a queue.
 * parameters:
 *     id: voice detector identify number
 *     continous:
 *       2: This function will not return until the VOICE_DET_EXIT_CMD be sent
 *       1: This function will not return until the cmd queue is empty
 *       0: This function just execute only once.
 * return: 0(success) !0(error)
 */
int voice_detector_run(enum voice_detector_id id, int continous);

/*
 * close a voice detector specified by a unique id.
 */
void voice_detector_close(enum voice_detector_id id);

/*
 * get vad data info by a unique id.
 */
void voice_detector_get_vad_data_info(enum voice_detector_id id, struct CODEC_VAD_BUF_INFO_T* vad_buf_info);

#ifdef VD_TEST
void voice_detector_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
