#ifndef CROSSFADE_H
#define CROSSFADE_H

#include <stdint.h>

enum CROSSFADE_STATUS_T {
    CROSSFADE_STATUS_OK = 0,
    CROSSFADE_STATUS_ERROR,
    CROSSFADE_STATUS_NO_BUF,
    CROSSFADE_STATUS_BUF_MISALIGN,
    CROSSFADE_STATUS_BUF_TOO_SMALL,
};

typedef struct
{
    float overlap_time;
    float coef1;
    float coef2;

    void *buf;
    uint32_t size;
} CrossFadeConfig;

typedef void *CrossFadeId;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crossfade_get_buffer_size(void);

enum CROSSFADE_STATUS_T crossfade_init(int32_t sample_rate, int32_t sample_bits, CrossFadeConfig *cfg, CrossFadeId *id);

void crossfade_destroy(CrossFadeId id);

/*
 * outbuf = (1 - weight) * inbuf1 + weight * (coef1 * inbuf1 + coef2 * inbuf2)
 *        = (1 - weight + weight * coef1) * inbuf1 + weight * coef2 * inbuf2
 *        = (1 - (1 - coef1) * weight) * inbuf1 + weight * coef2 * inbuf2;
 * out_ch_num = max(ch_num1, ch_num2);
 * final data is outbuf = inbuf1 * coef1 + inbuf2 * coef2;
 *
 * fadein:
 *  crossfade_process(st, NULL, 1, inbuf, ch_num, outbuf, frame_size);
 *
 * fadeout:
 *  crossfade_process(st, inbuf, ch_num, NULL, 1, frame_size);
 *
 * crossfade: (ch_num1 == ch_num2 || ch_num1 == 1 || ch_num2 == 1)
 *  crossfade_process(st, inbuf1, ch_num1, inbuf2, ch_num2, outbuf, frame_size);
 */
bool crossfade_process(CrossFadeId id, void *inbuf1, int32_t ch_num1, void *inbuf2, int32_t ch_num2, void *outbuf, int32_t frame_size);

#ifdef __cplusplus
}
#endif

#endif