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
#ifndef __APP_A2DP_H__
#define __APP_A2DP_H__
#include "btapp.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define A2DP_NON_CODEC_TYPE_NON         0
#define A2DP_NON_CODEC_TYPE_LHDC        1
#define A2DP_NON_CODEC_TYPE_LDAC        2
#define A2DP_NON_CODEC_TYPE_SCALABLE    3

uint8_t a2dp_get_current_codec_type(uint8_t *elements);

bool a2dp_is_music_ongoing(void);
int a2dp_volume_set(enum BT_DEVICE_ID_T id, uint8_t vol);
#if defined(A2DP_LDAC_ON)
void app_ibrt_restore_ldac_info(uint8_t sample_freq);
#endif
#ifdef __cplusplus
}
#endif

#endif
