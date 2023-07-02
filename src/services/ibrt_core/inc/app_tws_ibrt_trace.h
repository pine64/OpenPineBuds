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
#ifndef __APP_TWS_IBRT_TRACE_H__
#define __APP_TWS_IBRT_TRACE_H__

#include "hal_trace.h"

extern const char *g_log_super_state_str[];

extern const char *g_log_action_str[];

extern const char *g_log_event_str[];

extern const char *g_log_status_str[];

extern const char *g_log_box_state_str[];

extern const char *g_log_link_type_str[];

extern const char *g_log_scan_trigger_str[];

extern const char *g_log_policy_trigger_str[];

extern const char *g_log_pairing_state_str[];

extern const char *g_log_error_type_str[];

extern const char *g_log_pair_mode_str[];

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_VOICE_RPT_D(str, ...) LOG_DEBUG(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][VOICE_RPT][DBG]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_I(str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][VOICE_RPT]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_W(str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),     "[AUD][VOICE_RPT][WARN]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_E(str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)),    "[AUD][VOICE_RPT][ERR]"str, ##__VA_ARGS__)
#else
#define TRACE_VOICE_RPT_D(str, ...) LOG_DEBUG(LOG_MOD(AUDFLG),    "[AUD][VOICE_RPT][DBG]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_I(str, ...) LOG_INFO(LOG_MOD(AUDFLG),     "[AUD][VOICE_RPT]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_W(str, ...) LOG_WARN(LOG_MOD(AUDFLG),     "[AUD][VOICE_RPT][WARN]"str, ##__VA_ARGS__)
#define TRACE_VOICE_RPT_E(str, ...) LOG_ERROR(LOG_MOD(AUDFLG),    "[AUD][VOICE_RPT][ERR]"str, ##__VA_ARGS__)
#endif

#endif
