#ifndef FFTFILT2_H
#define FFTFILT2_H

#include <stdint.h>

typedef struct
{
    float *filter;
    int filter_length;
    int nfft;
} FftFilt2Config;

struct FftFilt2State_;

typedef struct FftFilt2State_ FftFilt2State;

#ifdef __cplusplus
extern "C" {
#endif

FftFilt2State *fftfilt2_init(int sample_rate, int frame_size, const FftFilt2Config *config);

void fftfilt2_destroy(FftFilt2State *st);

void fftfilt2_process_float(FftFilt2State *st, float *filter,float *buf, int frame_size);

void fftfilt2_process(FftFilt2State *st, float *filter,int16_t *buf, int frame_size);

void fftfilt2_process2_float(FftFilt2State *st, float *filter,float *buf, int frame_size, int stride);

void fftfilt2_process2(FftFilt2State *st, float *filter,int16_t *buf, int frame_size, int stride);

#ifdef __cplusplus
}
#endif

#endif