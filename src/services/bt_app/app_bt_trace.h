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
#ifndef __APP_BT_TRACE_H__
#define __APP_BT_TRACE_H__

#include "hal_trace.h"

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_AUD_MGR_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][MGR][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][MGR]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][MGR][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][MGR][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_AUD_MGR_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][MGR][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][MGR]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][MGR][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_MGR_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][MGR][ERR]"str, ##__VA_ARGS__)
#endif

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_AUD_HDL_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][HDL][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][HDL]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][HDL][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][HDL][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_AUD_HDL_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][HDL][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][HDL]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][HDL][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_HDL_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][HDL][ERR]"str, ##__VA_ARGS__)
#endif

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_AUD_STREAM_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][STRM][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][STRM]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][STRM][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][STRM][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_AUD_STREAM_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][STRM][DBG]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][STRM]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][STRM][WARN]"str, ##__VA_ARGS__)
#define TRACE_AUD_STREAM_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][STRM][ERR]"str, ##__VA_ARGS__)
#endif

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_MEDIA_PLAYER_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][STRM][DBG]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][STRM]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][STRM][WARN]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][STRM][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_MEDIA_PLAYESTREAM_D(str, ...) hal_trace_dummy(str, ##__VA_ARGS__)
//LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][STRM][DBG]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][STRM]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][STRM][WARN]"str, ##__VA_ARGS__)
#define TRACE_MEDIA_PLAYESTREAM_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][STRM][ERR]"str, ##__VA_ARGS__)
#endif

#endif
