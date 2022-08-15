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



#ifndef _OTA_TASK_H_
#define _OTA_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup OTATASK Task
 * @ingroup OTA
 * @brief OTA Profile Task.
 *
 * The OTATASK is responsible for handling the messages coming in and out of the
 * @ref OTA collector block of the BLE Host.
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

/*
 * DEFINES
 ****************************************************************************************
 */
#define OTA_CONTENT_TYPE_COMMAND		0 
#define OTA_CONTENT_TYPE_DATA		1

/// Messages for OTA Profile 
enum ota_msg_id
{
	OTA_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_OTA),

	OTA_TX_DATA_SENT,
	
	OTA_DATA_RECEIVED,

	OTA_SEND_NOTIFICATION,

	OTA_SEND_INDICATION,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ble_ota_tx_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct ble_ota_rx_data_ind_t
{
	uint16_t	length;
	uint8_t		data[0];
};

struct ble_ota_tx_sent_ind_t
{
	uint8_t 	status;
};

struct ble_ota_send_data_req_t
{
	uint8_t 	connecionIndex;
	uint32_t 	length;
	uint8_t  	value[__ARRAY_EMPTY];
};


/// @} OTATASK

#endif /* _OTA_TASK_H_ */

