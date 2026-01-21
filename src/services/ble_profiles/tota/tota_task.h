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



#ifndef _TOTA_TASK_H_
#define _TOTA_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup TOTATASK Task
 * @ingroup TOTA
 * @brief TOTA Profile Task.
 *
 * The TOTATASK is responsible for handling the messages coming in and out of the
 * @ref TOTA collector block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "rwip_task.h" // Task definitions
#include "app_tota_cmd_code.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define TOTA_CONTENT_TYPE_COMMAND		0 
#define TOTA_CONTENT_TYPE_DATA		1

/// Messages for TOTA Profile 
enum tota_msg_id
{
	TOTA_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_TOTA),

	TOTA_TX_DATA_SENT,
	
	TOTA_DATA_RECEIVED,

	TOTA_SEND_NOTIFICATION,

	TOTA_SEND_INDICATION,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ble_tota_tx_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct ble_tota_rx_data_ind_t
{
	uint16_t	length;
	uint8_t		data[0];
};

struct ble_tota_tx_sent_ind_t
{
	uint8_t 	status;
};

struct ble_tota_send_data_req_t
{
	uint8_t 	connecionIndex;
	uint32_t 	length;
	uint8_t  	value[__ARRAY_EMPTY];
};


/// @} TOTATASK

#endif /* _TOTA_TASK_H_ */

