/**
 ****************************************************************************************
 * @addtogroup DATAPATHPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_DATAPATH_SERVER)
#include "gap.h"
#include "gattc_task.h"
#include "gapc_task.h"
#include "attm.h"
#include "datapathps.h"
#include "datapathps_task.h"

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
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);
	uint8_t conidx = KE_IDX_GET(src_id);
    datapathps_env->isNotificationEnabled[conidx] = false;
    return KE_MSG_CONSUMED;
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
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = GAP_ERR_NO_ERROR;

	BLE_GATT_DBG("gattc_write_req_ind_handler datapathps_env 0x%x write handle %d shdl %d", 
		datapathps_env, param->handle, datapathps_env->shdl);


    if (datapathps_env != NULL)
    {
        // TX ccc
        if (param->handle == (datapathps_env->shdl + DATAPATHPS_IDX_TX_NTF_CFG))
        {
			uint16_t value = 0x0000;
			
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if (value == PRF_CLI_STOP_NTFIND) {
                datapathps_env->isNotificationEnabled[conidx] = false;
            } else if (value == PRF_CLI_START_NTF) {
                datapathps_env->isNotificationEnabled[conidx] = true;
            } else {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR) {
                //Inform APP of TX ccc change
                struct ble_datapath_tx_notif_config_t * ind = KE_MSG_ALLOC(DATAPATHPS_TX_CCC_CHANGED,
                        prf_dst_task_get(&datapathps_env->prf_env, conidx),
                        prf_src_task_get(&datapathps_env->prf_env, conidx),
                        ble_datapath_tx_notif_config_t);

                ind->isNotificationEnabled = datapathps_env->isNotificationEnabled[conidx];

                ke_msg_send(ind);
            }
        }
        // RX data
        else if (param->handle == (datapathps_env->shdl + DATAPATHPS_IDX_RX_VAL))
        {
			//inform APP of data
            struct ble_datapath_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(DATAPATHPS_RX_DATA_RECEIVED,
                    prf_dst_task_get(&datapathps_env->prf_env, conidx),
                    prf_src_task_get(&datapathps_env->prf_env, conidx),
                    ble_datapath_rx_data_ind_t,
                    param->length);

			ind->length = param->length;
			memcpy((uint8_t *)(ind->data), &(param->value), param->length);

            ke_msg_send(ind);
        }
		else
		{
			status = PRF_APP_ERROR;
		}
    }

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

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
	// Get the address of the environment
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

	uint8_t conidx = KE_IDX_GET(dest_id);
		
	// notification has been sent out
    if ((GATTC_NOTIFY == param->operation) || (GATTC_WRITE_NO_RESPONSE == param->operation))
    {
		
        struct ble_datapath_tx_sent_ind_t * ind = KE_MSG_ALLOC(DATAPATHPS_TX_DATA_SENT,
                prf_dst_task_get(&datapathps_env->prf_env, conidx),
                prf_src_task_get(&datapathps_env->prf_env, conidx),
                ble_datapath_tx_sent_ind_t);

		ind->status = param->status;

        ke_msg_send(ind);

    }

	ke_state_set(dest_id, DATAPATHPS_IDLE);
	
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
    // Get the address of the environment
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

	struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, 
		gattc_read_cfm, BLE_MAXIMUM_CHARACTERISTIC_DESCRIPTION);

	uint8_t conidx = KE_IDX_GET(src_id);
    uint8_t status = GAP_ERR_NO_ERROR;

	BLE_GATT_DBG("gattc_read_req_ind_handler read handle %d shdl %d", param->handle, datapathps_env->shdl);

    if (param->handle == (datapathps_env->shdl + DATAPATHPS_IDX_TX_DESC)) {
        cfm->length = sizeof(custom_tx_desc);
        memcpy(cfm->value, custom_tx_desc, cfm->length);
    } else if (param->handle == (datapathps_env->shdl + DATAPATHPS_IDX_RX_DESC)) {
        cfm->length = sizeof(custom_rx_desc);
        memcpy(cfm->value, custom_rx_desc, cfm->length);
    } else if (param->handle == (datapathps_env->shdl + DATAPATHPS_IDX_TX_NTF_CFG)) {
        uint16_t notify_ccc;
        if (datapathps_env->isNotificationEnabled[conidx]) {
            notify_ccc = 1;
        } else {
            notify_ccc = 0;
        }
        cfm->length = sizeof(notify_ccc);
        memcpy(cfm->value, (uint8_t *)&notify_ccc, cfm->length);
    }

    else {
        cfm->length = 0;
        status = ATT_ERR_REQUEST_NOT_SUPPORTED;
    }

	cfm->handle = param->handle;

	cfm->status = status;

	ke_msg_send(cfm);

	ke_state_set(dest_id, DATAPATHPS_IDLE);

    return (KE_MSG_CONSUMED);
}

static void send_notifiction(uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

    if (datapathps_env->isNotificationEnabled[conidx]) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                                                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&datapathps_env->prf_env, conidx),
                                                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_NOTIFY;
        report_ntf->handle    = datapathps_env->shdl + DATAPATHPS_IDX_TX_VAL;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
    }
}

__STATIC int send_data_via_notification_handler(ke_msg_id_t const msgid,
        struct ble_datapath_send_data_req_t const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    send_notifiction(param->connecionIndex, param->value, param->length);
    return (KE_MSG_CONSUMED);
}

static void send_indication(uint8_t conidx, const uint8_t* ptrData, uint32_t length)
{
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

    if (datapathps_env->isNotificationEnabled[conidx]) {
        // Allocate the GATT notification message
        struct gattc_send_evt_cmd *report_ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                                                KE_BUILD_ID(TASK_GATTC, conidx), prf_src_task_get(&datapathps_env->prf_env, conidx),
                                                gattc_send_evt_cmd, length);

        // Fill in the parameter structure
        report_ntf->operation = GATTC_INDICATE;
        report_ntf->handle    = datapathps_env->shdl + DATAPATHPS_IDX_TX_VAL;
        // pack measured value in database
        report_ntf->length    = length;
        memcpy(report_ntf->value, ptrData, length);
        // send notification to peer device
        ke_msg_send(report_ntf);
	}
}

__STATIC int send_data_via_indication_handler(ke_msg_id_t const msgid,
                                      struct ble_datapath_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	send_indication(param->connecionIndex, param->value, param->length);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_data_via_write_command_handler(ke_msg_id_t const msgid,
                                      struct ble_datapath_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

	prf_gatt_write(&(datapathps_env->prf_env), KE_IDX_GET(dest_id), datapathps_env->shdl + DATAPATHPS_IDX_RX_VAL,
                                   (uint8_t *)&param->value, param->length, GATTC_WRITE_NO_RESPONSE);

	ke_state_set(dest_id, DATAPATHPS_BUSY);
	return (KE_MSG_CONSUMED);
}

__STATIC int send_data_via_write_request_handler(ke_msg_id_t const msgid,
                                      struct ble_datapath_send_data_req_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

	prf_gatt_write(&(datapathps_env->prf_env), KE_IDX_GET(dest_id), datapathps_env->shdl + DATAPATHPS_IDX_RX_VAL,
                                   (uint8_t *)&param->value, param->length, GATTC_WRITE);

	ke_state_set(dest_id, DATAPATHPS_BUSY);
	return (KE_MSG_CONSUMED);
}

__STATIC int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                        struct gattc_event_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

    switch(param->type)
    {
        case GATTC_INDICATE:
        {
            // confirm that indication has been correctly received
            struct gattc_event_cfm * cfm = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
            cfm->handle = param->handle;
            ke_msg_send(cfm);
        }
        /* no break */

        case GATTC_NOTIFY:
        {
            if (param->handle == datapathps_env->shdl + DATAPATHPS_IDX_TX_VAL)
            {
				//inform APP of data
				struct ble_datapath_rx_data_ind_t * ind = KE_MSG_ALLOC_DYN(DATAPATHPS_NOTIFICATION_RECEIVED,
						prf_dst_task_get(&datapathps_env->prf_env, conidx),
						prf_src_task_get(&datapathps_env->prf_env, conidx),
						ble_datapath_rx_data_ind_t,
						param->length);
				
				ind->length = param->length;
				memcpy((uint8_t *)(ind->data), &(param->value), param->length);
				
				ke_msg_send(ind);

            }
        } break;

        default:
            break;
    }

    return (KE_MSG_CONSUMED);
}

__STATIC int control_notification_handler(ke_msg_id_t const msgid,
                                      struct ble_datapath_control_notification_t const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);
	uint16_t ccc_write_val = 0;
	if (param->isEnable)
	{
		ccc_write_val = 1;
	}

	prf_gatt_write(&(datapathps_env->prf_env), param->connecionIndex, datapathps_env->shdl + DATAPATHPS_IDX_TX_NTF_CFG,
                                   (uint8_t *)&ccc_write_val, sizeof(ccc_write_val), GATTC_WRITE);

	ke_state_set(dest_id, DATAPATHPS_BUSY);
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
    // Get the address of the environment
    struct gattc_att_info_cfm * cfm;

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
    cfm->handle = param->handle;

	struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);
    // check if it's a client configuration char
    if (param->handle == datapathps_env->shdl + DATAPATHPS_IDX_TX_NTF_CFG)
    {
        // CCC attribute length = 2
        cfm->length = 2;
        cfm->status = GAP_ERR_NO_ERROR;
    }
    else if (param->handle == datapathps_env->shdl + DATAPATHPS_IDX_RX_VAL)
    {
        // force length to zero to reject any write starting from something != 0
        cfm->length = 0;
        cfm->status = GAP_ERR_NO_ERROR;
    }
    // not expected request
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
KE_MSG_HANDLER_TAB(datapathps)
{
	{GAPC_DISCONNECT_IND,		(ke_msg_func_t) gapc_disconnect_ind_handler},
    {GATTC_WRITE_REQ_IND,    	(ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,          	(ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,     	(ke_msg_func_t) gattc_read_req_ind_handler},
    {DATAPATHPS_SEND_DATA_VIA_NOTIFICATION,   	(ke_msg_func_t) send_data_via_notification_handler},
	{DATAPATHPS_SEND_DATA_VIA_INDICATION,   	(ke_msg_func_t) send_data_via_indication_handler},
	{DATAPATHPS_SEND_DATA_VIA_WRITE_COMMAND,    (ke_msg_func_t) send_data_via_write_command_handler},
	{DATAPATHPS_SEND_DATA_VIA_WRITE_REQUEST,    (ke_msg_func_t) send_data_via_write_request_handler},
	{GATTC_EVENT_IND,               			(ke_msg_func_t) gattc_event_ind_handler},
	{DATAPATHPS_CONTROL_NOTIFICATION,			(ke_msg_func_t) control_notification_handler},
	{GATTC_ATT_INFO_REQ_IND,                    (ke_msg_func_t) gattc_att_info_req_ind_handler },
	
};

void datapathps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct datapathps_env_tag *datapathps_env = PRF_ENV_GET(DATAPATHPS, datapathps);

    task_desc->msg_handler_tab = datapathps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(datapathps_msg_handler_tab);
    task_desc->state           = &(datapathps_env->state);
    task_desc->idx_max         = BLE_CONNECTION_MAX;
}

#endif /* #if (BLE_DATAPATH_SERVER) */

/// @} DATAPATHPSTASK
