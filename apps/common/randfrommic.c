#ifdef __RAND_FROM_MIC__
#include "audioflinger.h"
#include "hal_trace.h"
#include "app_utils.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "app_bt_stream.h"
#include "randfrommic.h"
#include "hal_timer.h"
#include "cmsis_os.h"
#include "cmsis_gcc.h"
#include "app_audio.h"

#if BT_DRV_DEBUG
#define RAND_TRACE(n, fmt, ...)     TRACE(n, fmt, ##__VA_ARGS__)
#define RAND_DUMP(s,buff,len)       DUMP8(s,buff,len)
#else
#define RAND_TRACE(n, fmt, ...)
#define RAND_DUMP(s,buff,len)
#endif

static void generateRand(bool on);
static uint32_t rand_data_handle(uint8_t *buf, uint32_t len);

static uint8_t deviceId = 0;
static uint8_t *captureBuffer = NULL;
static uint32_t randSeed = 1;
static bool randInitialised = false;

// 4 bytes aligned
#define RAND_GRAB_BITS_PER_SAMPLE       4
#define RAND_GRAB_BITS_MASK_PER_SAMPLE  ((1 << RAND_GRAB_BITS_PER_SAMPLE)-1)

RAND_NUMBER_T randomBuffer =
{
    25,
    RAND_STATUS_CLOSE,
};

/**
 * Description: parse mic data according to the stream cfg(bit mode and channel number)
 *              only the lowest byte of each frame is taken
 * ADC format:
 *      16bit mode -> [15:0] is valid
 *      24bit mode -> [23:4] is valid
 *      32bit mode -> [31:12] is valid
 *
 */
static int randDataParse(uint8_t *buf, uint32_t len, enum AUD_BITS_T bits,
    enum AUD_CHANNEL_NUM_T ch_num)
{
    uint8_t index = 0;

    union {
        uint32_t    seedValue;
        uint8_t     value[4];
    }seedData;

    if ((NULL == buf) ||
        ((RANDOM_CAPTURE_BUFFER_SIZE/2) > len)) // ping-pong buffer
    {
        return -1;
    }

    RAND_TRACE(1, "%s", __func__);
    RAND_DUMP("%x ",buf, 16);

    switch (bits)
    {
        case AUD_BITS_16:
        {
            uint16_t* content = (uint16_t *)buf;

            for (index = 0;index < 4; index++)
            {
                seedData.value[index] = ((*content) & RAND_GRAB_BITS_MASK_PER_SAMPLE) |
                    (((*(content+ch_num)) & RAND_GRAB_BITS_MASK_PER_SAMPLE) << RAND_GRAB_BITS_PER_SAMPLE);
                content += ((8/RAND_GRAB_BITS_PER_SAMPLE)*ch_num);
            }
            break;
        }
        case AUD_BITS_24:
        {
            uint32_t* content = (uint32_t *)buf;
            for (index = 0;index < 4; index++)
            {
                // bit 23:4 are valid
                seedData.value[index] = (((*content) >> 4) & RAND_GRAB_BITS_MASK_PER_SAMPLE) |
                    ((((*(content+ch_num)) >> 4)&RAND_GRAB_BITS_MASK_PER_SAMPLE) << RAND_GRAB_BITS_PER_SAMPLE);
                content += ((8/RAND_GRAB_BITS_PER_SAMPLE)*ch_num);
            }
            break;
        }
        case AUD_BITS_32:
        {
            uint32_t* content = (uint32_t *)buf;
            for (index = 0;index < 4; index++)
            {
                // bit 31:12 are valid
                seedData.value[index] = (((*content) >> 12) & RAND_GRAB_BITS_MASK_PER_SAMPLE) |
                    ((((*(content+ch_num)) >> 12) & RAND_GRAB_BITS_MASK_PER_SAMPLE) << RAND_GRAB_BITS_PER_SAMPLE);
                content += ((8/RAND_GRAB_BITS_PER_SAMPLE)*ch_num);
            }
            break;
        }
        default:
        {
            return -1;
        }
        break;
    }

    randSeed = seedData.seedValue;

    return 0;
}

static void generateRand(bool on)
{
    struct AF_STREAM_CONFIG_T stream_cfg;

    RAND_TRACE(2, "%s op:%d", __func__, on);

    if (on)
    {
        randomBuffer.skipRound = 10;

        randomBuffer.status = random_mic_is_on(&deviceId);
        RAND_TRACE(2, "%s random status = %d", __func__, randomBuffer.status);

        if (RAND_STATUS_CLOSE == randomBuffer.status)
        {
            app_sysfreq_req(APP_SYSFREQ_USER_RANDOM, APP_SYSFREQ_208M);
            app_capture_audio_mempool_init();
            app_capture_audio_mempool_get_buff(&captureBuffer,
                RANDOM_CAPTURE_BUFFER_SIZE);
            memset(&stream_cfg, 0, sizeof(stream_cfg));
            stream_cfg.bits = AUD_BITS_16;
            stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
            stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
            stream_cfg.sample_rate = AUD_SAMPRATE_8000;
            stream_cfg.vol = TGT_VOLUME_LEVEL_15;
            stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
            stream_cfg.handler = rand_data_handle;

            stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(captureBuffer);
            stream_cfg.data_size = RANDOM_CAPTURE_BUFFER_SIZE;
            af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
            af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
            randomBuffer.status = RAND_STATUS_OPEN;
        }
        else if(RAND_STATUS_MIC_OPENED == randomBuffer.status)
        {
            af_stream_start(deviceId, AUD_STREAM_CAPTURE);
        }
    }
    else
    {
        // release the acquired system clock
        app_sysfreq_req(APP_SYSFREQ_USER_RANDOM, APP_SYSFREQ_32K);
        if (RAND_STATUS_MIC_OPENED == randomBuffer.status)
        {
            af_stream_stop(deviceId, AUD_STREAM_CAPTURE);
        }
        else if (RAND_STATUS_OPEN == randomBuffer.status)
        {
            af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
            af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        }
        randomBuffer.status = RAND_STATUS_CLOSE;
    }
}

static uint32_t rand_data_handle(uint8_t *buf, uint32_t len)
{
    if (buf == NULL)
    {
        return len;
    }

    if ((1 == randomBuffer.skipRound) &&
        (!randDataParse(buf, len, AUD_BITS_16, AUD_CHANNEL_NUM_1)))
    {
        generateRand(false);
        randomBuffer.skipRound = 0;
    }
    else if (1 != randomBuffer.skipRound)
    {
        randomBuffer.skipRound--;
    }

    return len;
}

void initSeed(void)
{
    uint8_t count = 100; // avoid deed loop

    RAND_TRACE(2, "%s:+++ initialised = %d", __func__, randInitialised);

    if (randInitialised)
    {
        generateRand(true);

        while ((0 != randomBuffer.skipRound) && (0 != count))
        {
            osDelay(10);
            count --;
        }
    }

    if ((0 == count) || (false == randInitialised))
    {
        RAND_TRACE(1, "%s not ready", __func__);
        randSeed = (uint32_t)hal_sys_timer_get();
        generateRand(false);
    }

    srand(randSeed);
    RAND_TRACE(2, "%s:--- count = %d", __func__, count);
}

void random_status_sync(void)
{
    if (RAND_STATUS_OPEN == randomBuffer.status)
    {
        RAND_TRACE(1, "%s random mic has already on,should be closed", __func__);
        generateRand(false);
    }
}

void random_data_process(uint8_t *buf, uint32_t len,enum AUD_BITS_T bits,
    enum AUD_CHANNEL_NUM_T ch_num)
{
    if (buf == NULL)
    {
        return;
    }

    if ((RAND_STATUS_MIC_STARTED == randomBuffer.status) ||
        (RAND_STATUS_MIC_OPENED == randomBuffer.status))
    {
        if (len >= RANDOM_CAPTURE_BUFFER_SIZE/2)
        {
            RAND_TRACE(4, "%s buf address = 0x%p, bits = %d, channel num = %d", __func__, buf, bits, ch_num);
            RAND_DUMP("%02x ", buf, 32);
            if ((1 == randomBuffer.skipRound) &&
                (!randDataParse(buf, len, bits, ch_num)))
            {
                generateRand(false);
                randomBuffer.skipRound = 0;
            }
            else if (1 != randomBuffer.skipRound)
            {
                randomBuffer.skipRound--;
            }
        }
    }
}

void randInit(void)
{
    randInitialised = true;
}

#endif
