#ifdef AI_ALGORITHM_ENABLE

#include "cmsis_os.h"
#include "hal_aud.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "app_ai_algorithm.h"
#include "cqueue.h"
#include "heap_api.h"
#include "audio_dump.h"
#include "norflash_api.h"

#ifdef BISTO_ENABLED
#include "app_voicepath.h"
#endif

#ifdef AI_CAPTURE_DATA_AEC
#include "echo_canceller.h"
#endif

#ifdef AI_AEC_CP_ACCEL
#include "cp_accel.h"
#define AI_TEXT_SRAM_LOC CP_TEXT_SRAM_LOC
#define AI_CP_BSS_LOC CP_BSS_LOC
#else
#define AI_TEXT_SRAM_LOC
#define AI_CP_BSS_LOC
#endif

#define AI_CP_CACHE_ATTR   ALIGNED(4) AI_CP_BSS_LOC

#define AI_ABS(x) (x>=0?x:(-x))

#define FRAME_LEN                   (256)
#if AI_CAPTURE_CHANNEL_NUM == 2
#define CAPTURE_CHANNEL_NUM         (2)
#else
#define CAPTURE_CHANNEL_NUM         (1)
#endif
#ifdef AI_CAPTURE_DATA_AEC
#define AUDIO_CHANNEL_NUM           (2)
#else
#define AUDIO_CHANNEL_NUM           (0)
#endif
#define CAPTURE_AUDIO_CHANNEL_NUM   (CAPTURE_CHANNEL_NUM+AUDIO_CHANNEL_NUM)
#define MIC_DATA_PROCESS_SIZE       (FRAME_LEN * CAPTURE_AUDIO_CHANNEL_NUM * sizeof(short))

#define AI_AEC_CACHE_NUM 6
typedef struct
{
    uint8_t                 cp_data_cache[AI_AEC_CACHE_NUM][MIC_DATA_PROCESS_SIZE];
    uint8_t                 cp_data_in_cache_num;
    uint8_t                 cp_data_process_cache_num;
    uint8_t                 cp_data_out_cache_num;
    uint8_t                 cp_data_use_cache_num;
    uint8_t                 cp_data_use_max_cache_num;
    AI_AEC_CACHE_STATE_E    cache_state[AI_AEC_CACHE_NUM];
} AI_AEC_struct ;

#define MED_MEM_POOL_SIZE  (1024*136)
static uint8_t g_medMemPool[MED_MEM_POOL_SIZE] __attribute__((aligned(4)));

osMutexId mic_data_handle_mutex_id = NULL;
osMutexDef(mic_data_handle_queue_mutex);

#define LOCK_MIC_DATA_HANDLE_QUEUE() \
    osMutexWait(mic_data_handle_mutex_id, osWaitForever);

#define UNLOCK_MIC_DATA_HANDLE_QUEUE() \
    osMutexRelease(mic_data_handle_mutex_id);

CQueue mic_data_handle_queue;
unsigned char mic_data_handle_queue_buff[MIC_DATA_PROCESS_SIZE * 6] __attribute__((aligned(4)));
unsigned char mic_data_handle_temp_buff[MIC_DATA_PROCESS_SIZE] __attribute__((aligned(4)));

#ifdef AI_CAPTURE_DATA_AEC
static bool ai_aec_inited = false;
Ec2FloatState *speech_aec2float_st1 = NULL;

static AI_CP_BSS_LOC short data_mic1[FRAME_LEN] = {0};
static AI_CP_BSS_LOC short echo_ref1[FRAME_LEN] = {0};
static AI_CP_BSS_LOC short echo_ref2[FRAME_LEN] = {0};
static AI_CP_BSS_LOC short aec2_out_buf1[FRAME_LEN] = {0};

#if AI_CAPTURE_CHANNEL_NUM == 2
Ec2FloatState *speech_aec2float_st2 = NULL;
static AI_CP_BSS_LOC short data_mic2[FRAME_LEN] = {0};
static AI_CP_BSS_LOC short aec2_out_buf2[FRAME_LEN] = {0};
#endif

Ec2FloatConfig speech_aec2float_cfg = {
    .bypass         = 0,
    .hpf_enabled    = false,
    .af_enabled     = true,
    .nlp_enabled    = false,
    .clip_enabled   = false,
    .stsupp_enabled = false,
    .ns_enabled     = false,
    .cng_enabled    = false,
    .blocks         = 6,
    .delay          = 0,
    .min_ovrd       = 2,
    .target_supp    = -40,
    .noise_supp     = -15,
    .cng_type       = 1,
    .cng_level      = -60,
    .clip_threshold = -20.f,
    .banks          = 32,
};
#endif

#ifdef AI_AEC_CP_ACCEL
static AI_CP_CACHE_ATTR AI_AEC_struct ai_aec_struct;
#endif

#define IGNORE_ECHO_VOL 60
CP_TEXT_SRAM_LOC
void app_ai_aec_process(uint8_t *buf, uint32_t length)
{
    uint16_t i = 0;
    short *tmp_in = (short *)buf;
    float echo_vol=0;

    for (i = 0; i < FRAME_LEN; i++)
    {
        data_mic1[i] = tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM];
        echo_ref1[i] = tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM+2];
        echo_ref2[i] = tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM+3];
        echo_vol += AI_ABS((float)echo_ref1[i])/FRAME_LEN;
    }

    //TRACE(3, "%s len %d echo_vol %d", __func__, length, (uint16_t)echo_vol);

    if (echo_vol < IGNORE_ECHO_VOL)
    {
        //TRACE(1, "%s echo_vol is too small", __func__);
        return;
    }

    ec2float_stereo(speech_aec2float_st1, data_mic1, echo_ref1, echo_ref2,FRAME_LEN, aec2_out_buf1);
    for (i = 0; i < FRAME_LEN; i++)
    {
        tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM] = aec2_out_buf1[i];
    }

#if AI_CAPTURE_CHANNEL_NUM == 2
    for (i = 0; i < FRAME_LEN; i++)
    {
        data_mic2[i] = tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM+1];
        echo_ref1[i] = tmp_in[i+FRAME_LEN*2];
        echo_ref2[i] = tmp_in[i+FRAME_LEN*3];
    }
    ec2float_stereo(speech_aec2float_st2, data_mic2, echo_ref1, echo_ref2,FRAME_LEN, aec2_out_buf2);

    for (i = 0; i < FRAME_LEN; i++)
    {
        tmp_in[i*CAPTURE_AUDIO_CHANNEL_NUM+1] = aec2_out_buf2[i];
    }
#endif

#if defined(AUDIO_DEBUG)
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, data_mic1, FRAME_LEN);
    audio_dump_add_channel_data(1, aec2_out_buf1, FRAME_LEN);
#if AI_CAPTURE_CHANNEL_NUM == 2
    audio_dump_add_channel_data(2, data_mic2, FRAME_LEN);
    audio_dump_add_channel_data(3, aec2_out_buf2, FRAME_LEN);
#endif
    audio_dump_run();
#endif
}

bool app_ai_aec_get_data_from_cache(uint8_t *buf, uint32_t length)
{
    bool ret = false;
    uint8_t out_cache_num = ai_aec_struct.cp_data_out_cache_num;
    uint8_t *out_data_cache = ai_aec_struct.cp_data_cache[out_cache_num];
    AI_AEC_CACHE_STATE_E out_cache_state = ai_aec_struct.cache_state[out_cache_num];

#ifdef AI_AEC_CP_ACCEL
    //TRACE(2, "%s cache_num %d state %d", __func__, ai_aec_struct.cp_data_out_cache_num, out_cache_state);

    if (AI_AEC_CACHE_PROCESSED == out_cache_state)
    {
        memcpy(buf, out_data_cache, length);
        ai_aec_struct.cache_state[out_cache_num] = AI_AEC_CACHE_IDLE;
        if (++ai_aec_struct.cp_data_out_cache_num >= AI_AEC_CACHE_NUM)
        {
            ai_aec_struct.cp_data_out_cache_num = 0;
        }
        ai_aec_struct.cp_data_use_cache_num--;
        ret = true;
    }
    else
    {
        ret = false;
    }
#else
    ret = true;
#endif

    return ret;
}

void app_ai_aec_mic_data_handle(uint8_t *buf, uint32_t length)
{
    uint8_t in_cache_num = ai_aec_struct.cp_data_in_cache_num;
    uint8_t *in_data_cache = ai_aec_struct.cp_data_cache[in_cache_num];
    AI_AEC_CACHE_STATE_E in_cache_state = ai_aec_struct.cache_state[in_cache_num];

#ifdef AI_AEC_CP_ACCEL
    //TRACE(2, "%s cache_num %d state %d max_num %d", __func__, ai_aec_struct.cp_data_in_cache_num, in_cache_state, ai_aec_struct.cp_data_use_max_cache_num);
    if (AI_AEC_CACHE_IDLE == in_cache_state)
    {
        memcpy(in_data_cache, buf, length);
        ai_aec_struct.cache_state[in_cache_num] = AI_AEC_CACHE_CACHED;
        if (++ai_aec_struct.cp_data_in_cache_num >= AI_AEC_CACHE_NUM)
        {
            ai_aec_struct.cp_data_in_cache_num = 0;
        }
        ai_aec_struct.cp_data_use_cache_num++;
        if (ai_aec_struct.cp_data_use_cache_num > ai_aec_struct.cp_data_use_max_cache_num)
        {
            ai_aec_struct.cp_data_use_max_cache_num = ai_aec_struct.cp_data_use_cache_num;
        }
        cp_accel_send_event_mcu2cp(CP_BUILD_ID(CP_TASK_AEC, CP_EVENT_AEC_PROCESSING));
    }
    else
    {
        TRACE(3, "%s cache %d state error %d", __func__, in_cache_num, in_cache_state);
    }
#else
    app_ai_aec_process(buf, length);
#endif

    return;
}

void app_ai_algorithm_mic_data_handle(uint8_t *buf, uint32_t length)
{
    bool ret = false;
    uint16_t i = 0;
    uint32_t queue_len = 0;
    short *tmp_out = (short *)mic_data_handle_temp_buff;

    //TRACE(2, "%s len %d", __func__, length);
    LOCK_MIC_DATA_HANDLE_QUEUE();
    if (EnCQueue(&mic_data_handle_queue,(CQItemType *) buf, length) == CQ_ERR)
    {
        TRACE(2, "###far_field_queue overflow, want %d bytes, has %d bytes\n",
            length, AvailableOfCQueue(&mic_data_handle_queue));
    }
    queue_len = LengthOfCQueue(&mic_data_handle_queue);
    UNLOCK_MIC_DATA_HANDLE_QUEUE();

    while (queue_len >= MIC_DATA_PROCESS_SIZE)
    {
        LOCK_MIC_DATA_HANDLE_QUEUE();
        ret = (DeCQueue(&mic_data_handle_queue, mic_data_handle_temp_buff, MIC_DATA_PROCESS_SIZE)==CQ_OK)?true:false;
        queue_len = LengthOfCQueue(&mic_data_handle_queue);
        UNLOCK_MIC_DATA_HANDLE_QUEUE();

#ifdef AI_CAPTURE_DATA_AEC
        if (ai_aec_inited && ret)
        {
            app_ai_aec_mic_data_handle(mic_data_handle_temp_buff, MIC_DATA_PROCESS_SIZE);
            ret = app_ai_aec_get_data_from_cache(mic_data_handle_temp_buff, MIC_DATA_PROCESS_SIZE);
        }
#endif

        while (ret)
        {
            //ret = other algorithm handle result

            if (ret)
            {
                for (i = 0; i < FRAME_LEN; i++)
                {
                    tmp_out[i] = tmp_out[i*CAPTURE_AUDIO_CHANNEL_NUM];
                }
                app_voicepath_enqueue_pcm_data(mic_data_handle_temp_buff, FRAME_LEN*2);
            }

            if (ai_aec_inited)
            {
                ret = app_ai_aec_get_data_from_cache(mic_data_handle_temp_buff, MIC_DATA_PROCESS_SIZE);
            }
            else
            {
                ret = false;
            }
        }
    }
}

#ifdef AI_AEC_CP_ACCEL
AI_TEXT_SRAM_LOC
void cp_aec_data_process(void)
{
    uint8_t process_cache_num = ai_aec_struct.cp_data_process_cache_num;
    uint8_t *data_cache = ai_aec_struct.cp_data_cache[process_cache_num];
    AI_AEC_CACHE_STATE_E process_cache_state = ai_aec_struct.cache_state[process_cache_num];

    while (AI_AEC_CACHE_CACHED == process_cache_state)
    {
        app_ai_aec_process(data_cache, MIC_DATA_PROCESS_SIZE);
        ai_aec_struct.cache_state[process_cache_num] = AI_AEC_CACHE_PROCESSED;
        if (++ai_aec_struct.cp_data_process_cache_num >= AI_AEC_CACHE_NUM)
        {
            ai_aec_struct.cp_data_process_cache_num = 0;
        }
        process_cache_num = ai_aec_struct.cp_data_process_cache_num;
        data_cache = ai_aec_struct.cp_data_cache[process_cache_num];
        process_cache_state = ai_aec_struct.cache_state[process_cache_num];
    }
    //else
    //{
        //TRACE(3, "%s cache state %d error %d", __func__, process_cache_num, process_cache_state);
    //}
}

AI_TEXT_SRAM_LOC
unsigned int cp_aec_main(uint8_t event)
{
    switch (event)
    {
        case CP_EVENT_AEC_PROCESSING:
            cp_aec_data_process();
            break;
        default:
            break;
    }
    return 0;
}

static struct cp_task_desc TASK_DESC_AEC = {CP_ACCEL_STATE_CLOSED, cp_aec_main, NULL, NULL, NULL};
void cp_aec_init(void)
{
    uint8_t i = 0;
    TRACE(2, "%s ai_aec_inited %d", __func__, ai_aec_inited);

    if (!ai_aec_inited)
    {
        memset(&ai_aec_struct, 0, sizeof(ai_aec_struct));
        ai_aec_struct.cp_data_in_cache_num = 0;
        ai_aec_struct.cp_data_process_cache_num = 0;
        ai_aec_struct.cp_data_out_cache_num = 0;
        for (i=0; i<AI_AEC_CACHE_NUM; i++)
        {
            ai_aec_struct.cache_state[i] = AI_AEC_CACHE_IDLE;
        }
        norflash_api_flush_disable(NORFLASH_API_USER_CP,(uint32_t)cp_accel_init_done);
        cp_accel_open(CP_TASK_AEC, &TASK_DESC_AEC);
        while(cp_accel_init_done() == false) {
            hal_sys_timer_delay_us(100);
        }
        norflash_api_flush_enable(NORFLASH_API_USER_CP);
        ai_aec_inited = true;
    }
}

void cp_aec_deinit(void)
{
    TRACE(2, "%s ai_aec_inited %d", __func__, ai_aec_inited);

    if (ai_aec_inited)
    {
        memset(&ai_aec_struct, 0, sizeof(ai_aec_struct));
        ai_aec_inited = false;
        cp_accel_close(CP_TASK_AEC);
    }
}
#endif

void app_ai_algorithm_init(void)
{
    TRACE(1, "%s", __func__);

    med_heap_init(&g_medMemPool[0], MED_MEM_POOL_SIZE);

#if defined(AUDIO_DEBUG)
    audio_dump_init(FRAME_LEN, sizeof(short), 4);
#endif

#ifdef AI_CAPTURE_DATA_AEC
    //TRACE("init AEC2");
    speech_aec2float_st1 = ec2float_create(AUD_SAMPRATE_16000, FRAME_LEN, &speech_aec2float_cfg);
#if AI_CAPTURE_CHANNEL_NUM == 2
    speech_aec2float_st2 = ec2float_create(AUD_SAMPRATE_16000, FRAME_LEN, &speech_aec2float_cfg);
#endif
#ifdef AI_AEC_CP_ACCEL
    ai_aec_inited = false;
#else
    ai_aec_inited = true;
#endif
#endif

    if (mic_data_handle_mutex_id == NULL)
    {
        mic_data_handle_mutex_id = osMutexCreate((osMutex(mic_data_handle_queue_mutex)));
        if (mic_data_handle_mutex_id == NULL)
        {
            ASSERT(0, "[%s] ERROR: async_enc_queue_mutex_id fail!", __func__);
        }
    }
    //TRACE("line %d", __LINE__);
    InitCQueue(&mic_data_handle_queue, sizeof(mic_data_handle_queue_buff), mic_data_handle_queue_buff);
}

#endif //AI_ALGORITHM_ENABLE

