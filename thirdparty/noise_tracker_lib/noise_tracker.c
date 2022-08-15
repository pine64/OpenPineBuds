#include <math.h>
#include "noise_tracker.h"
#include "audio_dump.h"
#include "hal_trace.h"

//#define NT_DUMP_AUDIO_DATA

#define SQUARE(x) ((x) * (x))
#define LIN2DB(x) (10.f * log10f(x))

#define NORMAL_SCALE (1.f/32768/32768)

static NoiseTrackerCallback _callback;
static int _threshold;
static int _ch_num;

static float vec_power(int16_t *buf, uint32_t len, int stride)
{
    float sum = 0.f;
    for (uint32_t i = 0; i < len; i += stride)
        sum += SQUARE(buf[i]);

    return sum;
}

void noise_tracker_init(NoiseTrackerCallback cb, int ch_num, int threshold)
{
    _callback = cb;
    _threshold = threshold;
    _ch_num = ch_num;

#ifdef NT_DUMP_AUDIO_DATA
    audio_dump_init(240 * ANC_NOISE_TRACKER_CHANNEL_NUM, sizeof(int16_t), 1);
#endif
}

void noise_tracker_process(int16_t *buf, uint32_t len)
{
    float scale = 1.f / len * NORMAL_SCALE;
    float maxPowerdB = -90;

#ifdef NT_DUMP_AUDIO_DATA
    audio_dump_clear_up();
    audio_dump_add_channel_data(0, buf, len);
    audio_dump_run();
#endif

    for (int i = 0; i < _ch_num; i++) {
        float power = vec_power(&buf[i], len, _ch_num) * scale;
        float powerdB = LIN2DB(power);
        maxPowerdB = MAX(maxPowerdB, powerdB);

        //TRACE(3,"[%s] powerdB ch[%d] = %d", __FUNCTION__, i, (int)powerdB);
    }

    if (maxPowerdB > _threshold) {
        _callback(maxPowerdB);
    }
}
