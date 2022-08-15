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
#include "math.h"
#include "peak_detector.h"

// #define PKD_FACTOR_UP   (0.6)
// #define PKD_FACTOR_DOWN (2.0)
// #define PKD_REDUCE_RATE (0.0335)    // -30dB

static enum AUD_BITS_T pkd_samp_bits;
static float pkd_alphaR = 0.0f;
static float pkd_alphaA = 0.0f;
static float pkd_factor1 = 0.0f;
static float pkd_factor2 = 0.0f;
static float pkd_reduce_rate = 1.0f;

#define FABS(x)  ( (x) >= 0.f  ?  (x)  : -(x) )
#define Max(a,b) ((a)>(b) ? (a):(b))

// Depend on codec_dac_vol
// const float pkd_vol_multiple[18] = {0.089125, 0.0, 0.005623, 0.007943, 0.011220, 0.015849, 0.022387, 0.031623, 0.044668, 0.063096, 0.089125, 0.125893, 0.177828, 0.251189, 0.354813, 0.501187, 0.707946, 1.000000};

// static uint32_t test_num = 0;

// int app_bt_stream_local_volume_get(void);

// y = 20log(x)
static inline float convert_multiple_to_db(float multiple)
{
    return 20*(float)log10(multiple);
}

// x = 10^(y/20)
static  inline float convert_db_to_multiple(float db)
{
    return (float)pow(10, db/20);
}

void peak_detector_init(void)
{
    pkd_alphaR = 0.0f;
    pkd_alphaA = 0.0f;
    pkd_factor1 = 0.0f;
    pkd_factor2 = 0.0f;
    pkd_reduce_rate = 1.0f;
    // TRACE(3,"[%s] pkd_alphaR = %f, pkd_alphaA = %f", __func__, (double)pkd_alphaR, (double)pkd_alphaA);
}

void peak_detector_setup(PEAK_DETECTOR_CFG_T *cfg)
{
    pkd_samp_bits = cfg->bits;
    pkd_alphaR = (float)exp(-1/(cfg->factor_down * cfg->fs));
    pkd_alphaA = (float)exp(-1/(cfg->factor_up * cfg->fs));
    pkd_reduce_rate = convert_db_to_multiple(cfg->reduce_dB);
}

static void peak_detector_run_16bits(int16_t *buf, uint32_t len, float vol_multiple)
{
    float normal_rate = 1.0;
    float tgt_rate = 1.0;

    for(uint32_t i = 0; i < len; i++)
    {   
        pkd_factor1 = Max(buf[i], pkd_alphaR * pkd_factor1);
        pkd_factor2 = pkd_alphaA * pkd_factor2 + (1 - pkd_alphaA) * pkd_factor1;

        normal_rate = pkd_factor2/32768;

        tgt_rate = pkd_reduce_rate / normal_rate / vol_multiple;

        if(tgt_rate > 1.0)
        {
            tgt_rate = 1.0;
        }

        // rate += (tgt_rate - rate) / 10000.0;

        // if(pkd_factor2>)
        // {
        //  normal_rate = 0.25;
        // }
        // else
        // {
        //  normal_rate = 0.25;
        // }
        // normal_rate *= 1.0 - pkd_factor2/32768;

        buf[i] = (int16_t)(buf[i] * tgt_rate);
        // buf[i] = 0;
        // 
        // TRACE(2,"%d, %d", buf[i], pkd_factor2);
    }

    // if(test_num == 500)
    // {
    //  test_num = 0;
    //  TRACE(0,"START>>>");
    //  TRACE(2,"vol_level = %d, pkd_vol_multiple = %f", vol_level, pkd_vol_multiple[vol_level]);
    //  TRACE(3,"buf = %d, pkd_alphaR = %f, pkd_alphaA = %f", buf[len-1], pkd_alphaR, pkd_alphaA);
    //  TRACE(4,"pkd_factor1 = %f, pkd_factor2 = %f, normal_rate = %f, tgt_rate = %f", pkd_factor1, pkd_factor2, normal_rate, tgt_rate);
    //  TRACE(0,"END<<<");
    //  // TRACE(7,"[%s] buf = %d, pkd_alphaR = %f, pkd_alphaA = %f, pkd_factor1 = %f, pkd_factor2 = %f, normal_rate = %f", __func__, buf[len-1], pkd_alphaR, pkd_alphaA, pkd_factor1, pkd_factor2, (1.0 - pkd_factor2/32768));
    // }

#if 0
    short sample;
    short sample_max = 0;
    short sample_min = 0;

    for (uint32_t i = 0; i < len; i++)
    {
        sample = buf[i];
        if(sample > sample_max)
        {
            sample_max = sample;
        }

        if(sample < sample_min)
        {
            sample_min = sample;
        }
    } 


    TRACE(2,"Max = %10d, Min = %10d",sample_max, sample_min);
#endif
}

static void peak_detector_run_24bits(int32_t *buf, uint32_t len, float vol_multiple)
{
    float normal_rate = 1.0;
    float tgt_rate = 1.0;

    for(uint32_t i = 0; i < len; i++)
    {   
        pkd_factor1 = Max(buf[i], pkd_alphaR * pkd_factor1);
        pkd_factor2 = pkd_alphaA * pkd_factor2 + (1 - pkd_alphaA) * pkd_factor1;

        normal_rate = pkd_factor2/32768;

        tgt_rate = pkd_reduce_rate / normal_rate / vol_multiple;

        if(tgt_rate > 1.0)
        {
            tgt_rate = 1.0;
        }

        // rate += (tgt_rate - rate) / 10000.0;

        // if(pkd_factor2>)
        // {
        //  normal_rate = 0.25;
        // }
        // else
        // {
        //  normal_rate = 0.25;
        // }
        // normal_rate *= 1.0 - pkd_factor2/32768;

        buf[i] = (int32_t)(buf[i] * tgt_rate);
        // buf[i] = 0;
        // 
        // TRACE(2,"%d, %d", buf[i], pkd_factor2);
    }
}

void peak_detector_run(uint8_t *buf, uint32_t len, float vol_multiple)
{
    // int vol_level = 0;

    if (pkd_samp_bits <= AUD_BITS_16) {
        len = len / sizeof(int16_t);
        peak_detector_run_16bits((int16_t *)buf, len, vol_multiple);
    } else {
        len = len / sizeof(int32_t);
        peak_detector_run_24bits((int32_t *)buf, len, vol_multiple);
    }

    // test_num++;

    // vol_level = app_bt_stream_local_volume_get();
}

