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
#ifdef __GATT_OVER_BR_EDR__
#include "hal_trace.h"
#include "plat_types.h"
#include "app_btgatt.h"
#include "btgatt_api.h"

void app_btgatt_addsdp(uint16_t pServiceUUID, uint16_t startHandle, uint16_t endHandle)
{
    TRACE(1, "%s", __func__);
    btif_btgatt_addsdp(pServiceUUID, startHandle, endHandle);
}

void app_btgatt_init(void)
{
    TRACE(1, "%s", __func__);
    btif_btgatt_init();
}

#endif

