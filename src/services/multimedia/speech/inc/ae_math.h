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
#ifndef AE_MATH_H
#define AE_MATH_H

#include <stdint.h>
#include <math.h>

#ifdef __arm__
#include "arm_math.h"
#endif

#define AE_PI       3.14159265358979323846f

#define EPS (1e-7f)

#define AE_CLAMP(x,lo,hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#ifdef __arm__
#define AE_SSAT16(x) __SSAT((int32_t)(x), 16)
#define AE_SSAT24(x) __SSAT((int32_t)(x), 24)
#else
#define AE_SSAT16(x) AE_CLAMP(x,-32768,32767)
#define AE_SSAT24(x) AE_CLAMP(x,-16777216,16777215)
#endif

#define AE_ABS(x) ((x) > 0 ? (x) : (-(x)))

#define AE_FLOOR(x) (floorf(x))

#define AE_ROUND(x) (roundf(x))

#define AE_INT(x) ((int)(x))

// deal with x > -1 && x < 0
#define AE_SIGN(x) ((AE_INT(x) == 0 && (x) < 0) ? "-" : "")

#define AE_FRAC(x) ((int)(((x) > 0 ? ((x) - AE_INT(x)) : (AE_INT(x) - (x))) * 1000000))

#define AE_MIN(a,b) ((a) < (b) ? (a) : (b))

#define AE_MAX(a,b) ((a) > (b) ? (a) : (b))

#define SQUARE(x) ((x) * (x))

#define DB2LIN(x) (powf(10.f, (x) / 20.f))

#ifdef VQE_SIMULATE
#define AE_SIN(x) sinf(x)
#define AE_COS(x) cosf(x)
#else
#define AE_SIN(x) arm_sin_f32(x)
#define AE_COS(x) arm_cos_f32(x)
#endif

int ae_gcd(int u, int v);

int ipow(int base, int exp);

float ipowf(float base, int exp);

float pow_int(float base, int exp);

float expint(int n, float x);

float sqrt_approx(float z);

#define AE_RAND_MAX (32767)

void ae_srand(unsigned int init);

int ae_rand(void);

void speech_conv(float *x, float *y, short len1,int len2, float *out);

void scale_int16(int16_t *pDst, int16_t *pSrc, float scale, uint32_t blockSize);

int32_t nextpow2(int32_t N);

#endif
