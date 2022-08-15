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
#ifndef __APP_FACTORY_BT_H__
#define __APP_FACTORY__BTH__

#include "app_key.h"


void app_factorymode_bt_create_connect(void);

void app_factorymode_bt_init_connect(void);

int app_factorymode_bt_xtalcalib_proc(void);

void app_factorymode_bt_xtalrangetest(APP_KEY_STATUS *status, void *param);

void app_factorymode_bt_signalingtest(APP_KEY_STATUS *status, void *param);

void app_factorymode_bt_nosignalingtest(APP_KEY_STATUS *status, void *param);

void app_factorymode_bt_xtalcalib(APP_KEY_STATUS *status, void *param);

#endif
