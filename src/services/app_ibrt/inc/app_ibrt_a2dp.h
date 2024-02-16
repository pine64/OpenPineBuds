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
#ifndef __APP_IBRT_A2DP__
#define __APP_IBRT_A2DP__
#include "a2dp_api.h"
#include "app_tws_ibrt_cmd_sync_a2dp_status.h"
#include <stdint.h>

a2dp_stream_t *app_bt_get_mobile_a2dp_stream(uint32_t deviceId);
int a2dp_ibrt_sync_set_status(ibrt_a2dp_status_t *a2dp_status);
int a2dp_ibrt_sync_get_status(ibrt_a2dp_status_t *a2dp_status);
int a2dp_ibrt_stream_open_mock(void);
int a2dp_ibrt_session_reset(void);
int a2dp_ibrt_session_new(void);
uint32_t a2dp_ibrt_session_get(void);
int a2dp_ibrt_session_set(uint8_t session);
int a2dp_ibrt_stream_need_autotrigger_set_flag(void);
int a2dp_ibrt_stream_need_autotrigger_getandclean_flag(void);

#endif
