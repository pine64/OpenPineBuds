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
#ifndef __SPEECH_SSAT_H__
#define __SPEECH_SSAT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && defined(__arm__)
#include "cmsis.h"
static inline int16_t speech_ssat_int16(int32_t in)
{
    int16_t out;

    out = __SSAT(in,16);

    return out;
}

static inline int32_t speech_ssat_int24(int32_t in)
{
    int32_t out;

    out = __SSAT(in, 24);

    return out;
}
#else
static inline int16_t speech_ssat_int16(int32_t in)
{
    int16_t out;

    if (in>32767)
    {
        in = 32767;
    }
    else if (in<-32768)
    {
        in = -32768;
    }
    out = (int16_t)in;
    return out;
}

static inline int32_t speech_ssat_int24(int32_t in)
{
    int32_t out;

    if (in > 0x7fffff) {
        in = 0x7fffff;
    } else if (in < -0x800000) {
        in = -0x800000;
    }
    out = (int32_t)in;
    return out;
}
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template<typename DataType>
static inline DataType speech_ssat(int in)
{
    if (sizeof(DataType) == sizeof(int16_t))
        return speech_ssat_int16(in);
    else
        return speech_ssat_int24(in);
}
#endif

#endif