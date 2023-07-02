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
#ifndef __HAL_BTPCM_H__
#define __HAL_BTPCM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_aud.h"

enum HAL_BTPCM_ID_T {
	HAL_BTPCM_ID_0 = 0,
	HAL_BTPCM_ID_NUM,
};

struct HAL_BTPCM_CONFIG_T {
    uint32_t bits;
    uint32_t channel_num;
    uint32_t sample_rate;

    uint32_t use_dma;
};

int hal_btpcm_open(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream);
int hal_btpcm_close(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream);
int hal_btpcm_start_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream);
int hal_btpcm_stop_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream);
int hal_btpcm_setup_stream(enum HAL_BTPCM_ID_T id, enum AUD_STREAM_T stream, struct HAL_BTPCM_CONFIG_T *cfg);
int hal_btpcm_send(enum HAL_BTPCM_ID_T id, uint8_t *value, uint32_t value_len);
uint8_t hal_btpcm_recv(enum HAL_BTPCM_ID_T id, uint8_t *value, uint32_t value_len);

#ifdef __cplusplus
}
#endif

#endif
