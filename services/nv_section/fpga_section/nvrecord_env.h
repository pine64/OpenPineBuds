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
#if defined(NEW_NV_RECORD_ENABLED)

#ifndef NVRECORD_ENV_H
#define NVRECORD_ENV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "me_api.h"
#include "nvrecord_extension.h"

#define NVRAM_ENV_MEDIA_LANGUAGE_DEFAULT (0)
#define NVRAM_ENV_TWS_MODE_DEFAULT (0xff)
#define NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT (0xaabbccdd)
#define NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS (0xffffaa55)

int nv_record_env_init(void);
int nv_record_env_get(struct nvrecord_env_t **nvrecord_env);
int nv_record_env_set(struct nvrecord_env_t *nvrecord_env);
void nv_record_update_ibrt_info(uint32_t newMode,bt_bdaddr_t *ibrtPeerAddr);
void nvrecord_rebuild_system_env(struct nvrecord_env_t* pSystemEnv);
void nv_record_update_factory_tester_status(uint32_t status);

#ifdef __cplusplus
}
#endif
#endif
#endif // #if defined(NEW_NV_RECORD_ENABLED)

