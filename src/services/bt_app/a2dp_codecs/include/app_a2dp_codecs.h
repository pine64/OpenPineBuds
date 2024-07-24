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
#ifndef __APP_A2DP_CODEC_H__
#define __APP_A2DP_CODEC_H__

#include "a2dp_codec_sbc.h"
#include "a2dp_codec_aac.h"
#include "a2dp_codec_ldac.h"
#include "a2dp_codec_lhdc.h"
#include "a2dp_codec_opus.h"
#include "a2dp_codec_scalable.h"

#if defined(__cplusplus)
extern "C" {
#endif

int a2dp_codec_init(void);
uint8_t a2dp_codec_confirm_stream_state(uint8_t index, uint8_t old_state, uint8_t new_state);

#if defined(__cplusplus)
}
#endif

#endif /* __APP_A2DP_CODEC_H__ */