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
 * @addtogroup TOTATASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TOTA)
#include "gap.h"
#include "gattc_task.h"
#include "gapc_task.h"
#include "attm.h"
#include "tota_ble.h"
#include "tota_task.h"

#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
									  struct gapc_disconnect_ind const *param,
									  ke_task_id_t const dest_id,
									  ke_task_id_t const src_id)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	uint8_t conidx = KE_IDX_GET(src_id);
	struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);
	tota_env->isNotificationEnabled[conidx] = false;
	return KE_MSG_CONSUMED;
}

void app_tota_send_rx_cfm(uint8_t conidx)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	  struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

	  struct gattc_write_cfm *cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM,
	  	KE_BUILD_ID(TASK_GATTC, conidx),
	  prf_src_task_get(&tota_env->prf_env, conidx),
	  	gattc_write_cfm);
	  cfm->handle = tota_env->shdl + TOTA_IDX_VAL;
	  cfm->status = ATT_ERR_NO_ERROR;
	  ke_msg_send(cfm);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{	
    // Get the address of the environment
    struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

	BLE_GATT_DBG("gattc_write_req_ind_handler tota_env 0x%x write handle %d shdl %d", 
		tota_env, param->handle, tota_env->shdl);

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    if (tota_env != NULL)
    {
		if (param->handle == (tota_env->shdl + TOTA_IDX_NTF_CFG))
		{
			uint16_t value = 0x0000;
			
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND)
            {
                tota_env->isNotificationEnabled[conidx] = false;
            }
			else if (value == PRF_CLI_START_NTF)
			{
				tota_env->isNotificationEnabled[conidx] = true;
			}
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of TX ccc change
                struct ble_tota_tx_notif_config_t * ind = KE_MSG_ALLOC(TOTA_CCC_CHANGED,
                        prf_dst_task_get(&tota_env->prf_env, conidx),
                        prf_src_task_get(&tota_env->prf_env, conidx),
                        ble_tota_tx_notif_config_t);

                ind->isNotificationEnabled = tota_env->isNotificationEnabled[conidx];

                ke_msg_send(ind);
            }
		}
		else if (param->handle == (tota_env->shdl + TOTA_IDX_VAL))
		{
			//inform APP of data
            struct ble_tota_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(TOTA_DATA_RECEIVED,
                    prf_dst_task_get(&tota_env->prf_env, conidx),
                    prf_src_task_get(&tota_env->prf_env, conidx),
                    ble_tota_rx_data_ind_t,
                    sizeof(struct ble_tota_rx_data_ind_t) + param->length);

			ind->length = param->length;
			memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);

            return KE_MSG_CONSUMED;
		}
		else
		{
			status = PRF_APP_ERROR;
		}
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
	// Get the address of the environment
    struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

	uint8_t conidx = KE_IDX_GET(dest_id);
		
	// notification or write request has been sent out
    if ((GATTC_NOTIFY == param->operation) || (GATTC_INDICATE == param->operation))
    {
		
        struct ble_tota_tx_sent_ind_t * ind = KE_MSG_ALLOC(TOTA_TX_DATA_SENT,
                prf_dst_task_get(&tota_env->prf_env, conidx),
                prf_src_task_get(&tota_env->prf_env, conidx),
                ble_tota_tx_sent_ind_t);

		ind->status = param->status;

        ke_msg_send(ind);

    }

	ke_state_set(dest_id, TOTA_IDLE);
	
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the read request from peer device
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{	

    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
    // Get the address of the environment
    struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

	struct gattc_read_cfm* cfm;

	uint8_t status = GAP_ERR_NO_ERROR;
	uint8_t conidx = KE_IDX_GET(src_id);

	BLE_GATT_DBG("gattc_read_req_ind_handler read handle %d shdl %d", param->handle, tota_env->shdl);

    if (param->handle == (tota_env->shdl + TOTA_IDX_NTF_CFG))
	{
		uint16_t notify_ccc;
		cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(notify_ccc));
		
		if (tota_env->isNotificationEnabled[conidx])
		{
			notify_ccc = 1;
		}
		else
		{
			notify_ccc = 0;
		}
		cfm->length = sizeof(notify_ccc);
		memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
	}
	else
	{
		cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
		cfm->length = 0;
		status = ATT_ERR_REQUEST_NOT_SUPPORTED;
	}

	cfm->handle = param->handle;

	cfm->status = status;

	ke_msg_send(cfm);

	ke_state_set(dest_id, TOTA_IDLE);

    return (KE_MSG_CONSUMED);
}

static void send_notifiction(uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
	uint16_t handle_offset = 0xFFFF;
	struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

	if (tota_env->isNotificationEnabled[conidx])
	{
		handle_offset = TOTA_IDX_VAL;
	}

	TRACE(1,"Send tota notificationto handle offset %d:", handle_offset);

	if (0xFFFF != handle_offset)
	{
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&tota_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = tota_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
	}
}

static void send_indication(uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
	uint16_t handle_offset = 0xFFFF;
	struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);
	if (tota_env->isNotificationEnabled[conidx])
	{
		handle_offset = TOTA_IDX_VAL;
	}

	TRACE(1,"Send tota indicationto handle offset %d:", handle_offset);

	if (0xFFFF != handle_offset)
	{
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&tota_env->prf_env, conidx),
                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_INDICATE;
        report_ntf->handle    = tota_env->shdl + handle_offset;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
	}
}

__STATIC int send_notification_handler(ke_msg_id_t const msgid,
                                      struct ble_tota_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_notifiction(param->connecionIndex, param->value, param->length);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_indication_handler(ke_msg_id_t const msgid,
                                      struct ble_tota_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_indication(param->connecionIndex, param->value, param->length);
	return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the attribute info request message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    TOTA_LOG_DBG(1,"[%s]TOTA",__func__);
    // Get the address of the environment
    struct gattc_att_info_cfm * cfm;

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

	struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

	if (param->handle == (tota_env->shdl + TOTA_IDX_NTF_CFG))
	{
		// CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
	}
	else if (param->handle == (tota_env->shdl + TOTA_IDX_VAL))
	{
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;			
	}
	else
	{
		cfm->length = 0;
    	cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
	}

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(tota)
{
	{GAPC_DISCONNECT_IND,		(ke_msg_func_t) gapc_disconnect_ind_handler},
    {GATTC_WRITE_REQ_IND,    	(ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,          	(ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,     	(ke_msg_func_t) gattc_read_req_ind_handler},
    {TOTA_SEND_NOTIFICATION,   	(ke_msg_func_t) send_notification_handler},
	{TOTA_SEND_INDICATION,   	(ke_msg_func_t) send_indication_handler},		
	{GATTC_ATT_INFO_REQ_IND,    (ke_msg_func_t) gattc_att_info_req_ind_handler },
};

void tota_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct tota_env_tag *tota_env = PRF_ENV_GET(TOTA, tota);

    task_desc->msg_handler_tab = tota_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(tota_msg_handler_tab);
    task_desc->state           = &(tota_env->state);
    task_desc->idx_max         = BLE_CONNECTION_MAX;
}

#endif /* #if (BLE_TOTA) */

/// @} TOTATASK
