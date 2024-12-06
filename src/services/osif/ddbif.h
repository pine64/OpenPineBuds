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
#ifndef __DDBIF_H__
#define __DDBIF_H__

#include "bluetooth.h"
#include "me_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void ddbif_list_saved_flash(void);

bt_status_t ddbif_close(void);

bt_status_t ddbif_add_record(btif_device_record_t *record);

bt_status_t ddbif_open(const bt_bdaddr_t *bdAddr);

bt_status_t ddbif_find_record(const bt_bdaddr_t *bdAddr, btif_device_record_t *record);

bt_status_t ddbif_delete_record(const bt_bdaddr_t *bdAddr);

bt_status_t ddbif_enum_device_records(I16 index, btif_device_record_t *record);

#ifdef __cplusplus
}
#endif
#endif /*__DDBIF_H__*/

