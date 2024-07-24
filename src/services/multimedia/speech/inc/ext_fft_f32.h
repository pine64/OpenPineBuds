/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "arm_math.h"

#ifndef VQE_SIMULATE
#include "hal_trace.h"
#else
#define ASSERT(cond, str, ...)      { if (!(cond)) { fprintf(stderr, str, ##__VA_ARGS__); while(1); } }
#define TRACE(str, ...)             do { fprintf(stdout, str, ##__VA_ARGS__); fprintf(stdout, "\n"); } while (0)
#endif

// Enable this MACRO, Get same data with speex fft
// Some case can disable this MACRO to reduce MIPS
// #define _FFT_PRECISION

struct ext_f32_config {
   float *in;
   float *out;
   arm_rfft_fast_instance_f32 fft;
   int32_t N;
   int32_t mode;
};

static void *ext_f32_fft_init(int32_t size, int32_t mode, void *ext_malloc(int))
{
    arm_status status;
    struct ext_f32_config *st = ext_malloc(1 * sizeof(struct ext_f32_config));
    st->mode = mode;

    st->in = ext_malloc(size * sizeof(float));

    if (st->mode)
    {
        st->out = ext_malloc(size * sizeof(float));
    }
    else
    {
        st->out = NULL;
    }

    status = arm_rfft_fast_init_f32(&(st->fft), size);
    ASSERT(status == ARM_MATH_SUCCESS, "FFTWRAP: arm cmsis f32 fft init error %d", status);
    st->N = size;

    return st;
}

static void ext_f32_fft_destroy(void *table, void ext_free(void *))
{
    struct ext_f32_config *st = (struct ext_f32_config *)table;

    ext_free(st->in);

    if (st->mode) {
        ext_free(st->out);
    }

    ext_free(st);
}

static POSSIBLY_UNUSED void ext_f32_fft(void *table, float *in, float *out)
{
    struct ext_f32_config *st = (struct ext_f32_config *)table;
    const int32_t N = st->N;
    float *iptr = st->in;
    float *optr = NULL;

    if (st->mode)
    { 
        optr = st->out;
    }
    else
    {
        optr = out;
    }

    /* We need to make a copy as fft modify input data */
    for (int32_t i = 0; i < N; i++)
      iptr[i] = in[i];

    arm_rfft_fast_f32(&(st->fft), iptr, optr, 0);

    if (st->mode)
    {    
        out[0] = optr[0];
        for (int32_t i = 1; i < N - 1; i++) {
            out[i] = optr[i + 1];
        }
        out[N - 1] = optr[1];
    }

#if defined(_FFT_PRECISION)
    for (int32_t i = 0; i < N; i++)
        out[i] = out[i] / N;
#endif
}

static POSSIBLY_UNUSED void ext_f32_ifft(void *table, float *in, float *out)
{
    struct ext_f32_config *st = (struct ext_f32_config *)table;
    const int32_t N = st->N;
    float *iptr = st->in;

#if defined(_FFT_PRECISION)
    for (int32_t i = 0; i < N; i++) {
        in[i] = in[i] * N;
    }
#endif

    if (st->mode)
    {   
        iptr[0] = in[0];
        iptr[1] = in[N - 1];
        for (int32_t i = 1; i < N - 1; i++) {
            iptr[i + 1] = in[i];
        }
    }
    else
    {
        iptr = in;
    }

    arm_rfft_fast_f32(&(st->fft), iptr, out, 1);
}
