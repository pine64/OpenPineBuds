#include "bt_sco_chain.h"
#include "speech_memory.h"
#include "speech_utils.h"
#include "hal_trace.h"
#include "audio_dump.h"
#include "speech_cfg.h"

#if defined(SPEECH_TX_24BIT)
int32_t *aec_echo_buf = NULL;

// Use to free buffer
static int32_t *aec_echo_buf_ptr = NULL;
#else
short *aec_echo_buf = NULL;

// Use to free buffer
static short *aec_echo_buf_ptr;
#endif

#if defined(MSBC_8K_SAMPLE_RATE)
#define SPEECH_HEAP_RESERVE_SIZE (1024 * 4)
#else
#define SPEECH_HEAP_RESERVE_SIZE (1024 * 4)
#endif

#if defined(SPEECH_TX_DC_FILTER)
SpeechDcFilterState *speech_tx_dc_filter_st = NULL;
#endif

int speech_init(int tx_sample_rate, int rx_sample_rate,
                     int tx_frame_ms, int rx_frame_ms,
                     int sco_frame_ms,
                     uint8_t *buf, int len)
{
    // we shoule keep a minmum buffer for speech heap
    // MSBC_16K_SAMPLE_RATE = 0, 560 bytes
    // MSBC_16K_SAMPLE_RATE = 1, 2568 bytes
    speech_heap_init(buf, SPEECH_HEAP_RESERVE_SIZE);

    uint8_t *free_buf = buf + SPEECH_HEAP_RESERVE_SIZE;
    int free_len = len - SPEECH_HEAP_RESERVE_SIZE;

    // use free_buf for your algorithm
    memset(free_buf, 0, free_len);

    int frame_len = SPEECH_FRAME_MS_TO_LEN(tx_sample_rate, tx_frame_ms);

#if defined(SPEECH_TX_24BIT)
    aec_echo_buf = (int32_t *)speech_calloc(frame_len, sizeof(int32_t));
#else
    aec_echo_buf = (short *)speech_calloc(frame_len, sizeof(short));
#endif
    aec_echo_buf_ptr = aec_echo_buf;

#if defined(SPEECH_TX_DC_FILTER)
    int channel_num = SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
    int data_separation = 0;

    SpeechDcFilterConfig dc_filter_cfg = {
        .bypass = 0,
        .gain = 0.f,
    };

    speech_tx_dc_filter_st = speech_dc_filter_create(tx_sample_rate, frame_len, &dc_filter_cfg);
    speech_dc_filter_ctl(speech_tx_dc_filter_st, SPEECH_DC_FILTER_SET_CHANNEL_NUM, &channel_num);
    speech_dc_filter_ctl(speech_tx_dc_filter_st, SPEECH_DC_FILTER_SET_DATA_SEPARATION, &data_separation);
#endif

    audio_dump_init(frame_len, sizeof(int16_t), 3);

    return 0;
}

int speech_deinit(void)
{
#if defined(SPEECH_TX_DC_FILTER)
    speech_dc_filter_destroy(speech_tx_dc_filter_st);
#endif

    speech_free(aec_echo_buf_ptr);

    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(3,"SPEECH MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);

    return 0;
}

#if defined(BONE_SENSOR_TDM)
extern void bt_sco_get_tdm_buffer(uint8_t **buf, uint32_t *len);
#endif

int speech_tx_process(void *_pcm_buf, void *_ref_buf, int *_pcm_len)
{
#if defined(SPEECH_TX_24BIT)
    int32_t *pcm_buf = (int32_t *)_pcm_buf;
    int32_t *ref_buf = (int32_t *)_ref_buf;
#else
    int16_t *pcm_buf = (int16_t *)_pcm_buf;
    int16_t *ref_buf = (int16_t *)_ref_buf;
#endif

    int pcm_len = *_pcm_len;

#if defined(BONE_SENSOR_TDM)
    uint8_t *bone_buf = NULL;
    uint32_t bone_len = 0;
    bt_sco_get_tdm_buffer(&bone_buf, &bone_len);
#endif

    audio_dump_clear_up();
    audio_dump_add_channel_data(0, ref_buf, pcm_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
    audio_dump_add_channel_data_from_multi_channels(1, pcm_buf, pcm_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SPEECH_CODEC_CAPTURE_CHANNEL_NUM, 0);
    audio_dump_add_channel_data_from_multi_channels(2, pcm_buf, pcm_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SPEECH_CODEC_CAPTURE_CHANNEL_NUM, 1);

#if defined(BONE_SENSOR_TDM)
    audio_dump_add_channel_data(3, bone_buf, pcm16_len/SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
#endif

    audio_dump_run();

#if defined(SPEECH_TX_DC_FILTER)
#if defined(SPEECH_TX_24BIT)
    speech_dc_filter_process_int24(speech_tx_dc_filter_st, pcm_buf, pcm_len);
#else
    speech_dc_filter_process(speech_tx_dc_filter_st, pcm_buf, pcm_len);
#endif
#endif

    // Add your algrithm here and disable #if macro
#if 1
    for (int i = 0, j = 0; i < pcm_len; i += SPEECH_CODEC_CAPTURE_CHANNEL_NUM, j++) {
        // choose main microphone data
        pcm_buf[j] = pcm_buf[i];
        // choose reference data, i.e. loopback
        //pcm16_buf[j] = ref16_buf[j];
    }
    pcm_len /= SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
#endif

#if defined(BONE_SENSOR_TDM)
    memcpy(pcm_buf, bone_buf, bone_len);
#endif

    *_pcm_len = pcm_len;

    return 0;
}

int speech_rx_process(void *pcm_buf, int *pcm_len)
{
    // Add your algorithm here
    return 0;
}