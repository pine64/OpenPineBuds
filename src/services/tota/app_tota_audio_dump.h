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

#ifndef __APP_TOTA_AUDIO_DUMP_H__
#define __APP_TOTA_AUDIO_DUMP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_tota_audio_dump_init();

void app_tota_audio_dump_start();
void app_tota_audio_dump_stop();
void app_tota_audio_dump_flush();
bool app_tota_audio_dump_send(uint8_t * pdata, uint32_t dataLen);

#ifdef __cplusplus
}
#endif

#endif