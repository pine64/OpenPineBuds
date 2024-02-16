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
#ifndef __A2DP_DECODER_TRACE_H__
#define __A2DP_DECODER_TRACE_H__

#include "hal_trace.h"

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_A2DP_DECODER_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][DECODER][DBG]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][DECODER]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][DECODER][WARN]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][DECODER][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_A2DP_DECODER_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][DECODER][DBG]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][DECODER]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][DECODER][WARN]"str, ##__VA_ARGS__)
#define TRACE_A2DP_DECODER_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][DECODER][ERR]"str, ##__VA_ARGS__)
#endif

#define ASSERT_A2DP_DECODER(cond, str, ...) ASSERT(cond, str, ##__VA_ARGS__) 

#endif
