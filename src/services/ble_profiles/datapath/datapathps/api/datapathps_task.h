#ifndef _DATAPATHPS_TASK_H_
#define _DATAPATHPS_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup DATAPATHPSTASK Task
 * @ingroup DATAPATHPS
 * @brief Heart Rate Profile Task.
 *
 * The DATAPATHPSTASK is responsible for handling the messages coming in and out of the
 * @ref DATAPATHPS collector block of the BLE Host.
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
/// Messages for Data Path Server Profile 
enum datapathps_msg_id
{
	DATAPATHPS_TX_CCC_CHANGED = TASK_FIRST_MSG(TASK_ID_DATAPATHPS),

	DATAPATHPS_TX_DATA_SENT,
	
	DATAPATHPS_RX_DATA_RECEIVED,

	DATAPATHPS_NOTIFICATION_RECEIVED,

	DATAPATHPS_SEND_DATA_VIA_NOTIFICATION,

	DATAPATHPS_SEND_DATA_VIA_INDICATION,

	DATAPATHPS_SEND_DATA_VIA_WRITE_COMMAND,

	DATAPATHPS_SEND_DATA_VIA_WRITE_REQUEST,

	DATAPATHPS_CONTROL_NOTIFICATION,

};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct ble_datapath_tx_notif_config_t
{
	bool 		isNotificationEnabled;
};

struct ble_datapath_rx_data_ind_t
{
	uint16_t	length;
	uint8_t		data[0];
};

struct ble_datapath_tx_sent_ind_t
{
	uint8_t 	status;
};

struct ble_datapath_send_data_req_t
{
	uint8_t 	connecionIndex;
	uint32_t 	length;
	uint8_t  	value[__ARRAY_EMPTY];
};

struct ble_datapath_control_notification_t
{
	bool 		isEnable;
    uint8_t     connecionIndex;
};

/// @} DATAPATHPSTASK

#endif /* _DATAPATHPS_TASK_H_ */
