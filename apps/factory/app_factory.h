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
#ifndef __APP_FACTORY_H__
#define __APP_FACTORY_H__

#define APP_FACTORY_TRACE(s,...) TRACE(s, ##__VA_ARGS__)

void app_factorymode_result_set(bool result);

void app_factorymode_result_clean(void);

bool app_factorymode_result_wait(void);

void app_factorymode_enter(void);

void app_factorymode_key_init(void);

int app_factorymode_init(uint32_t factorymode);

int app_factorymode_calib_only(void);

int app_factorymode_languageswitch_proc(void);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __USB_COMM__
int app_factorymode_cdc_comm(void);
#endif

bool app_factorymode_get(void);

void app_factorymode_set(bool set);

#ifdef __cplusplus
}
#endif

#endif
