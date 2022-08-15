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
#ifndef __APP_HRP_H
#define __APP_HRP_H

#if (BLE_APP_HR)

#include "hrps.h"
#include "hrps_task.h"

#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"

#define INVALID_CONNECTION_INDEX	0xFF

void app_hrps_add_profile(void);

extern const struct ke_state_handler app_hrps_table_handler;

void app_hrps_init(void);

void app_hrps_connected_evt_handler(uint8_t conidx);

void app_hrps_disconnected_evt_handler(void);

/// Messages for HRP Server Profile 
enum hrp_msg_id
{
	HRP_MEASUREMENT_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_HRPS),

    HRP_MEASUREMENT_DATA_SENT,

    HRP_CTRL_POINT_RECEIVED,
	
};

struct ble_hrp_measurement_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct app_hrps_env_tag
{
    uint8_t connectionIndex;
	uint8_t	isNotificationEnabled;
    struct hrps_meas_send_req meas;
};


#endif

#endif
