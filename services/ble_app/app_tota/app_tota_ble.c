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


/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_TOTA)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "string.h"
#include "arch.h"                    // Platform Definitions
#include "co_bt.h"
#include "prf.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "app_ble_rx_handler.h"
#include "tota_task.h"              
#include "app_tota_ble.h"                  // TOTA Application Definitions
#include "app_tota_data_handler.h"
#include "app_tota_cmd_handler.h"
#include "app_tota.h"


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct app_tota_env_tag app_tota_env;

static app_tota_tx_done_t tota_data_tx_done_callback = NULL;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void app_tota_connected_evt_handler(uint8_t conidx)
{	
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	app_tota_env.connectionIndex = conidx;
    app_tota_update_datapath(APP_TOTA_VIA_NOTIFICATION);
	//tota_register_transmitter(app_tota_send_notification);
}

void app_tota_disconnected_evt_handler(uint8_t conidx)
{
    TOTA_LOG_DBG(3,"[%s], condix = %d Index = %d",__func__, conidx, app_tota_env.connectionIndex);
	if (conidx == app_tota_env.connectionIndex)
	{
		app_tota_env.connectionIndex = BLE_INVALID_CONNECTION_INDEX;
		app_tota_env.isNotificationEnabled = false;
		app_tota_env.mtu[conidx] = 0;
        app_tota_update_datapath(APP_TOTA_PATH_IDLE);
		//tota_disconnection_handler();
	}
}

void app_tota_ble_init(void)
{
	app_tota_env.connectionIndex =  BLE_INVALID_CONNECTION_INDEX;
	app_tota_env.isNotificationEnabled = false;
	memset((uint8_t *)&(app_tota_env.mtu), 0, sizeof(app_tota_env.mtu));	
}

void app_tota_add_tota(void)
{
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, 0);
    
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX>1
	req->sec_lvl = PERM(SVC_AUTH, ENABLE)|PERM(SVC_MI, ENABLE);
#else
	req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif  

    req->prf_task_id = TASK_ID_TOTA;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Send the message
    ke_msg_send(req);
}

void app_tota_send_notification(uint8_t* ptrData, uint32_t length)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	
	struct ble_tota_send_data_req_t * req = KE_MSG_ALLOC_DYN(TOTA_SEND_NOTIFICATION,
                                                KE_BUILD_ID(prf_get_task_from_id(TASK_ID_TOTA), app_tota_env.connectionIndex),
                                                TASK_APP,
                                                ble_tota_send_data_req_t,
                                                length);
	req->connecionIndex = app_tota_env.connectionIndex;
	req->length = length;
	memcpy(req->value, ptrData, length);

	ke_msg_send(req);
}

void app_tota_send_indication(uint8_t* ptrData, uint32_t length)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	struct ble_tota_send_data_req_t * req = KE_MSG_ALLOC_DYN(TOTA_SEND_INDICATION,
                                                KE_BUILD_ID(prf_get_task_from_id(TASK_ID_TOTA), app_tota_env.connectionIndex),
                                                TASK_APP,
                                                ble_tota_send_data_req_t,
                                                length);
	req->connecionIndex = app_tota_env.connectionIndex;
	req->length = length;
	memcpy(req->value, ptrData, length);

	ke_msg_send(req);
}

static int app_tota_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
    return (KE_MSG_CONSUMED);
}

void tota_update_MTU(uint16_t mtu)
{
    // remove the 3 bytes of overhead
    app_tota_env.mtu[app_tota_env.connectionIndex] = mtu;
    TRACE(1,"updated data packet size is %d", app_tota_env.mtu[app_tota_env.connectionIndex]);
}

static int app_tota_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct ble_tota_tx_notif_config_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
	TOTA_LOG_DBG(1,"tota data ccc changed to %d", param->isNotificationEnabled);
    app_tota_env.isNotificationEnabled = param->isNotificationEnabled;

	if (app_tota_env.isNotificationEnabled)
	{
		if (BLE_INVALID_CONNECTION_INDEX == app_tota_env.connectionIndex)
		{
			uint8_t conidx = KE_IDX_GET(src_id);
			app_tota_connected_evt_handler(conidx);

			if (app_tota_env.mtu[conidx])
			{
				// tota_update_MTU(app_tota_env.mtu[conidx]);
			}
		}
	}
    else
    {
        app_tota_update_datapath(APP_TOTA_VIA_INDICATION);
    }
    
	
    return (KE_MSG_CONSUMED);
}

static int app_tota_tx_data_sent_handler(ke_msg_id_t const msgid,
                              struct ble_tota_tx_sent_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    if (NULL != tota_data_tx_done_callback)
    {
		tota_data_tx_done_callback();
    }

	//tota_data_transmission_done_callback();

    return (KE_MSG_CONSUMED);
}

static int app_tota_data_received_handler(ke_msg_id_t const msgid,
                              struct ble_tota_rx_data_ind_t *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
	app_ble_push_rx_data(BLE_RX_DATA_SELF_TOTA, app_tota_env.connectionIndex, param->data, param->length);
    return (KE_MSG_CONSUMED);
}

void app_tota_register_tx_done(app_tota_tx_done_t callback)
{
	tota_data_tx_done_callback = callback;
}

void app_tota_mtu_exchanged_handler(uint8_t conidx, uint16_t MTU)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	if (conidx == app_tota_env.connectionIndex)
	{
		tota_update_MTU(MTU);
	}
	else
    {
	    app_tota_env.mtu[conidx] = MTU;
    }
}

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_tota_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)app_tota_msg_handler},

    {TOTA_CCC_CHANGED,     		(ke_msg_func_t)app_tota_ccc_changed_handler},
    {TOTA_TX_DATA_SENT,    		(ke_msg_func_t)app_tota_tx_data_sent_handler},
    {TOTA_DATA_RECEIVED, 		(ke_msg_func_t)app_tota_data_received_handler},
};

const struct ke_state_handler app_tota_table_handler =
    {&app_tota_msg_handler_list[0], (sizeof(app_tota_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_APP_TOTA

/// @} APP
