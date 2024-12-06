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
#if !defined(NEW_NV_RECORD_ENABLED)

#ifndef NVRECORD_ENV_H
#define NVRECORD_ENV_H

#ifdef __cplusplus
extern "C" {
#endif
#include "me_api.h"

#define NVRAM_ENV_INVALID (0xdead0000)
#define NVRAM_ENV_MEDIA_LANGUAGE_DEFAULT (0)
#define NVRAM_ENV_STREAM_VOLUME_A2DP_VOL_DEFAULT (AUDIO_OUTPUT_VOLUME_DEFAULT)
#define NVRAM_ENV_STREAM_VOLUME_HFP_VOL_DEFAULT (AUDIO_OUTPUT_VOLUME_DEFAULT)
#define NVRAM_ENV_TWS_MODE_DEFAULT (0xff)
#define NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT (0xaabbccdd)
#define NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS (0xffffaa55)

struct media_language_t
{
    int8_t language;
};

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
struct src_snk_t
{
    int8_t src_snk_mode;
};
#endif

struct ibrt_mode_t
{
    uint32_t mode;
    btif_device_record_t record;
    bool    tws_connect_success;
};
struct factory_tester_status_t
{
    uint32_t status;
};

typedef struct
{
    bool     voice_key_enable;
    uint8_t  setedCurrentAi;
    uint8_t  currentAiSpec;
    uint8_t  aiStatusDisableFlag;
    uint8_t  amaAssistantEnableStatus; //one bit for one AI assistant
} AI_MANAGER_INFO_T;

struct nvrecord_env_t
{
    struct media_language_t media_language;
#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
    struct src_snk_t src_snk_flag;
#endif
    struct ibrt_mode_t ibrt_mode;
    struct factory_tester_status_t factory_tester_status;

    uint8_t  flag_value[8];
    AI_MANAGER_INFO_T   aiManagerInfo;
};

int nv_record_env_init(void);

int nv_record_env_get(struct nvrecord_env_t **nvrecord_env);

int nv_record_env_set(struct nvrecord_env_t *nvrecord_env);

void nv_record_update_ibrt_info(uint32_t newMode,bt_bdaddr_t *ibrtPeerAddr);

void nv_record_update_factory_tester_status(uint32_t status);

#ifdef __cplusplus
}
#endif
#endif

#endif // #if !defined(NEW_NV_RECORD_ENABLED)
