#include "bt_sco_chain.h"
#include "speech_memory.h"
#include "speech_utils.h"
#include "hal_trace.h"
#include "audio_dump.h"
#include "vcp-api.h"
#include "spf-postapi.h"

#define ALANGO_TRACE(s, ...) TRACE(1,"%s: " s, __FUNCTION__, ## __VA_ARGS__)

short *aec_echo_buf = NULL;

// Use to free buffer
static short *aec_echo_buf_ptr;

static void *mem;

extern void *voicebtpcm_get_ext_buff(int size);

extern char* vcp_errorv(err_t err);

extern PROFILE_TYPE(t) alango_profile;

static vcp_profile_t *curr_profile = NULL;

mem_reg_t reg[NUM_MEM_REGIONS];

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM >= 2
static int16_t *deinterleaved_buf = NULL;

static void deinterleave_audio(int16_t *dst, int16_t *src, uint32_t len, uint32_t ch_num)
{
    uint32_t samples_per_channel = len / ch_num;

    for (uint32_t i = 0; i < samples_per_channel; i++) {
        for (uint32_t j = 0; j < ch_num; j++) {
            dst[samples_per_channel * j + i] = src[ch_num * i + j];
        }
    }
}
#endif

int speech_init(int tx_sample_rate, int rx_sample_rate,
                     int tx_frame_ms, int rx_frame_ms,
                     int sco_frame_ms,
                     uint8_t *buf, int len)
{
    speech_heap_init(buf, len);

    int frame_len = SPEECH_FRAME_MS_TO_LEN(tx_sample_rate, tx_frame_ms);

    aec_echo_buf = (short *)speech_calloc(frame_len, sizeof(short));
    aec_echo_buf_ptr = aec_echo_buf;

    // init alango
    // check profile
    curr_profile = &alango_profile;
    err_t err = vcp_check_profile(curr_profile);
    if (err.err) {
        if (err.err == ERR_INVALID_CRC)
            ALANGO_TRACE(0,"Profile error: Invalid CRC!");
        else
            ALANGO_TRACE(1,"Profile error: %d", err.err);
    }

    ASSERT(frame_len % curr_profile->p_gen->frlen == 0, "Profile error: frame_len(%d) should be divided by frlen(%d)", frame_len, curr_profile->p_gen->frlen);

    unsigned int smem = vcp_get_hook_size();
    mem = speech_malloc(smem);

    vcp_get_mem_size(curr_profile, reg, mem);

    ALANGO_TRACE(0,"Hello, I am VCP8!");

    for (int i = 0; i < NUM_MEM_REGIONS; i++) {
        reg[i].mem = (void *)speech_malloc(reg[i].size);
        ALANGO_TRACE(2,"I need %d bytes of memory in memory region %d to work.\n", reg[i].size, i + 1);
    }

    err = vcp_init_debug(curr_profile, reg);
    if (err.err == ERR_NOT_ENOUGH_MEMORY) {
        ALANGO_TRACE(2,"%d more bytes needed in region %d!\n", -reg[err.pid].size, err.pid);
    } else if (err.err == ERR_UNKNOWN) {
        ALANGO_TRACE(0,"vcp_init_debug() returns UNKNOWN error\n!");
    } else if (err.err != ERR_NO_ERROR) {
        ALANGO_TRACE(2,"vcp_init_debug() returns error %d, pid %d!\n", err.err, err.pid);
    }

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM >= 2
    deinterleaved_buf = speech_malloc(curr_profile->p_gen->frlen * SPEECH_CODEC_CAPTURE_CHANNEL_NUM * sizeof(int16_t));
#endif

    audio_dump_init(frame_len, sizeof(int16_t), 3);

    return 0;
}

int speech_deinit(void)
{
    speech_free(aec_echo_buf_ptr);
    speech_free(mem);

    for (int i = 0; i < NUM_MEM_REGIONS; i++)
        speech_free(reg[i].mem);

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM >= 2
    speech_free(deinterleaved_buf);
#endif

    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(3,"SPEECH MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);

    return 0;
}

#if defined(BONE_SENSOR_TDM)
extern void bt_sco_get_tdm_buffer(uint8_t **buf, uint32_t *len);
#endif

int speech_tx_process(void *pcm_buf, void *ref_buf, int *pcm_len)
{
    int16_t *pcm16_buf = (int16_t *)pcm_buf;
    int16_t *ref16_buf = (int16_t *)ref_buf;
    int pcm16_len = *pcm_len;

#if defined(BONE_SENSOR_TDM)
    uint8_t *bone_buf = NULL;
    uint32_t bone_len = 0;
    bt_sco_get_tdm_buffer(&bone_buf, &bone_len);
#endif

    audio_dump_clear_up();
    audio_dump_add_channel_data(0, ref_buf, pcm16_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
    audio_dump_add_channel_data_from_multi_channels(1, pcm16_buf, pcm16_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SPEECH_CODEC_CAPTURE_CHANNEL_NUM, 0);
    audio_dump_add_channel_data_from_multi_channels(2, pcm16_buf, pcm16_len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SPEECH_CODEC_CAPTURE_CHANNEL_NUM, 1);

#if defined(BONE_SENSOR_TDM)
    audio_dump_add_channel_data(3, bone_buf, pcm16_len/SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
#endif

    audio_dump_run();

    // Add your algrithm here and disable #if macro
#if 0
    for (int i = 0, j = 0; i < pcm16_len; i += SPEECH_CODEC_CAPTURE_CHANNEL_NUM, j++) {
        // choose main microphone data
        pcm16_buf[j] = pcm16_buf[i];
        // choose reference data, i.e. loopback
        //pcm16_buf[j] = ref16_buf[j];
    }
    pcm16_len /= SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
#else
    for (int i = 0, j = 0; i < pcm16_len; i += curr_profile->p_gen->frlen * SPEECH_CODEC_CAPTURE_CHANNEL_NUM, j += curr_profile->p_gen->frlen) {
#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM >= 2
        deinterleave_audio(deinterleaved_buf, &pcm16_buf[i], curr_profile->p_gen->frlen * SPEECH_CODEC_CAPTURE_CHANNEL_NUM, SPEECH_CODEC_CAPTURE_CHANNEL_NUM);
        err_t err = vcp_process_tx(reg, deinterleaved_buf, &ref16_buf[j], &pcm16_buf[j]);
        //memcpy(&pcm16_buf[j], deinterleaved_buf, curr_profile->p_gen->frlen * sizeof(int16_t));
#else
        err_t err = vcp_process_tx(reg, &pcm16_buf[i], &ref16_buf[j], &pcm16_buf[j]);
#endif
        if (err.err != ERR_NO_ERROR) {
            ALANGO_TRACE(1,"vcp_process_tx error: %d", err.err);
        }
    }
    pcm16_len /= SPEECH_CODEC_CAPTURE_CHANNEL_NUM;
#endif

#if defined(BONE_SENSOR_TDM)
    memcpy(pcm_buf, bone_buf, bone_len);
#endif

    *pcm_len = pcm16_len;

    return 0;
}

int speech_rx_process(void *pcm_buf, int *pcm_len)
{
    int16_t *pcm16_buf = (int16_t *)pcm_buf;
    int pcm16_len = *pcm_len;

    for (int i = 0; i < pcm16_len; i += curr_profile->p_gen->frlen) {
        err_t err = vcp_process_rx(reg, &pcm16_buf[i], &pcm16_buf[i]);
        if (err.err != ERR_NO_ERROR) {
            ALANGO_TRACE(1,"vcp_process_tx error: %d", err.err);
        }
    }

    return 0;
}