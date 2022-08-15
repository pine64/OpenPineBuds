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

#ifndef __BTM_DEVICEDB_H__
#define __BTM_DEVICEDB_H__

#include "btlib_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

int8 btm_devicedb_get_latest_device ( struct bdaddr_t *bdaddr );
int8 btm_devicedb_save_latest_device (struct bdaddr_t *bdaddr );
int8 btm_devicedb_save_latest_device_profile (struct bdaddr_t *bdaddr, uint8* profile);

#if defined(__cplusplus)
}
#endif

#endif /* __BTM_DEVICEDB_H__ */