#ifndef LIMITER_H
#define LIMITER_H

#include <stdint.h>

typedef struct
{
    int32_t knee;
    int32_t look_ahead_time;
    int32_t threshold;
    float makeup_gain;
    int32_t ratio;              // reserved
    int32_t attack_time;
    int32_t release_time;
} LimiterConfig;

struct LimiterState_;

typedef struct LimiterState_ LimiterState;

LimiterState *limiter_create(int32_t sample_rate, int32_t frame_size, int32_t sample_bit, int32_t channel_number, const LimiterConfig *config);

int32_t limiter_destroy(LimiterState *st);

int32_t limiter_set_config(LimiterState *st, const LimiterConfig *config);

int32_t limiter_process(LimiterState *st, uint8_t *buf, int32_t len);

#endif