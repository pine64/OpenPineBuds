/**
 ****************************************************************************************
 * @addtogroup HRPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#include "hrp_common.h"

#if (BLE_HR_COLLECTOR)
#include "co_utils.h"
#include "gap.h"
#include "attm.h"
#include "hrpc_task.h"
#include "hrpc.h"
#include "gattc_task.h"

#include "ke_mem.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// State machine used to retrieve heart rate service characteristics information
const struct prf_char_def hrpc_hrs_char[HRPC_CHAR_MAX] =
{
    /// Heart Rate Measurement
    [HRPC_CHAR_HR_MEAS]              = {ATT_CHAR_HEART_RATE_MEAS,
                                        ATT_MANDATORY,
                                        ATT_CHAR_PROP_NTF},
    /// Body Sensor Location
    [HRPC_CHAR_BODY_SENSOR_LOCATION] = {ATT_CHAR_BODY_SENSOR_LOCATION,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_RD},
    /// Heart Rate Control Point
    [HRPC_CHAR_HR_CNTL_POINT]        = {ATT_CHAR_HEART_RATE_CNTL_POINT,
                                        ATT_OPTIONAL,
                                        ATT_CHAR_PROP_WR},
};

/// State machine used to retrieve heart rate service characteristic description information
const struct prf_char_desc_def hrpc_hrs_char_desc[HRPC_DESC_MAX] =
{
    /// Heart Rate Measurement client config
    [HRPC_DESC_HR_MEAS_CLI_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY, HRPC_CHAR_HR_MEAS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_SDP_SVC_IND_HANDLER message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_sdp_svc_ind_handler(ke_msg_id_t const msgid,
                                             struct gattc_sdp_svc_ind const *ind,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if(state == HRPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);

        ASSERT_INFO(hrpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(hrpc_env->env[conidx] != NULL, dest_id, src_id);

        if(hrpc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve HRS characteristics and descriptors
            prf_extract_svc_info(ind, HRPC_CHAR_MAX, &hrpc_hrs_char[0],  &hrpc_env->env[conidx]->hrs.chars[0],
                                      HRPC_DESC_MAX, &hrpc_hrs_char_desc[0], &hrpc_env->env[conidx]->hrs.descs[0]);

            //Even if we get multiple responses we only store 1 range
            hrpc_env->env[conidx]->hrs.svc.shdl = ind->start_hdl;
            hrpc_env->env[conidx]->hrs.svc.ehdl = ind->end_hdl;
        }

        hrpc_env->env[conidx]->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPC_ENABLE_REQ message.
 * The handler enables the Heart Rate Profile Collector Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hrpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hrpc_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    uint8_t state = ke_state_get(dest_id);
    // Heart Rate Profile Client Role Task Environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);;

    ASSERT_INFO(hrpc_env != NULL, dest_id, src_id);
    // Config connection, start discovering
    if((state == HRPC_IDLE) && (hrpc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        hrpc_env->env[conidx] = (struct hrpc_cnx_env*) ke_malloc(sizeof(struct hrpc_cnx_env),KE_MEM_ATT_DB);
        memset(hrpc_env->env[conidx], 0, sizeof(struct hrpc_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering HRS on peer
            prf_disc_svc_send(&(hrpc_env->prf_env), conidx,  ATT_SVC_HEART_RATE);

            hrpc_env->env[conidx]->last_uuid_req = ATT_SVC_HEART_RATE;

            // Go to DISCOVERING state
            ke_state_set(dest_id, HRPC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            hrpc_env->env[conidx]->hrs = param->hrs;
            //send APP confirmation that can start normal connection to TH
            hrpc_enable_rsp_send(hrpc_env, conidx, GAP_ERR_NO_ERROR);
        }

        status = GAP_ERR_NO_ERROR;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        hrpc_enable_rsp_send(hrpc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * This generic event is received for different requests, so need to keep track.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);
    // Status
    uint8_t status;

    if (hrpc_env != NULL)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        uint8_t state = ke_state_get(dest_id);

        if(state == HRPC_DISCOVERING)
        {
            status = param->status;

            if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||
                (status == ATT_ERR_NO_ERROR))
            {
                // Discovery
                // check characteristic validity
                if(hrpc_env->env[conidx]->nb_svc == 1)
                {
                    status = prf_check_svc_char_validity(HRPC_CHAR_MAX,
                            hrpc_env->env[conidx]->hrs.chars,
                            hrpc_hrs_char);
                }
                // too much services
                else if (hrpc_env->env[conidx]->nb_svc > 1)
                {
                    status = PRF_ERR_MULTIPLE_SVC;
                }
                // no services found
                else
                {
                    status = PRF_ERR_STOP_DISC_CHAR_MISSING;
                }

                // check descriptor validity
                if (status == GAP_ERR_NO_ERROR)
                {
                    status = prf_check_svc_char_desc_validity(HRPC_DESC_MAX,
                            hrpc_env->env[conidx]->hrs.descs,
                            hrpc_hrs_char_desc,
                            hrpc_env->env[conidx]->hrs.chars);
                }
            }

            hrpc_enable_rsp_send(hrpc_env, conidx, status);
        }

        else if(state == HRPC_BUSY)
        {
            switch(param->operation)
            {
                case GATTC_WRITE:
                case GATTC_WRITE_NO_RESPONSE:
                {
                    uint16_t rsp_msg_id = HRPC_CFG_INDNTF_RSP;
                    if(hrpc_env->env[conidx]->last_char_code != HRPC_DESC_HR_MEAS_CLI_CFG)
                    {
                        rsp_msg_id = HRPC_WR_CNTL_POINT_RSP;
                    }

                    struct hrpc_cfg_indntf_rsp *rsp = KE_MSG_ALLOC(rsp_msg_id,
                            prf_dst_task_get(&(hrpc_env->prf_env), conidx), dest_id,
                            hrpc_cfg_indntf_rsp);
                    rsp->status    = param->status;
                    // Send the message
                    ke_msg_send(rsp);
                }
                break;

                case GATTC_READ:
                {
                    if(param->status != GAP_ERR_NO_ERROR)
                    {
                        // an error occurs while reading peer device attribute
                        prf_client_att_info_rsp(
                                (prf_env_t*) hrpc_env->env[conidx],
                                conidx,
                                HRPC_RD_CHAR_RSP,
                                param->status,
                                NULL);
                    }
                }
                break;

                default: break;
            }

            ke_state_set(prf_src_task_get(&hrpc_env->prf_env, conidx), HRPC_IDLE);
        }
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPC_RD_DATETIME_REQ message.
 * Check if the handle exists in profile(already discovered) and send request, otherwise
 * error to APP.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hrpc_rd_char_req_handler(ke_msg_id_t const msgid,
                                        struct hrpc_rd_char_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if(state == HRPC_IDLE)
    {
        ASSERT_INFO(hrpc_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(hrpc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else
        {
            uint16_t search_hdl = ATT_INVALID_SEARCH_HANDLE;

            if(((param->char_code & HRPC_DESC_MASK) == HRPC_DESC_MASK) &&
               ((param->char_code & ~HRPC_DESC_MASK) < HRPC_DESC_MAX))
            {
                search_hdl = hrpc_env->env[conidx]->hrs.descs[param->char_code & ~HRPC_DESC_MASK].desc_hdl;
            }
            else if (param->char_code < HRPC_CHAR_MAX)
            {
                search_hdl = hrpc_env->env[conidx]->hrs.chars[param->char_code].val_hdl;
            }

            //check if handle is viable
            if (search_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Store the command
                hrpc_env->env[conidx]->last_char_code = param->char_code;
                // Send the read request
                prf_read_char_send(&(hrpc_env->prf_env), conidx,
                        hrpc_env->env[conidx]->hrs.svc.shdl,
                        hrpc_env->env[conidx]->hrs.svc.ehdl,
                        search_hdl);

                // Go to the Busy state
                ke_state_set(dest_id, HRPC_BUSY);

                status = ATT_ERR_NO_ERROR;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
    }
    else if (state == HRPC_FREE)
    {
        status = GAP_ERR_DISCONNECTED;
    }
    else
    {
        // Another procedure is pending, keep the command for later
        msg_status = KE_MSG_SAVED;
        status = GAP_ERR_NO_ERROR;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        prf_client_att_info_rsp(&hrpc_env->prf_env, conidx, HRPC_RD_CHAR_RSP,
                status, NULL);
    }

    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    prf_client_att_info_rsp(&hrpc_env->prf_env, conidx, HRPC_RD_CHAR_RSP,
            GAP_ERR_NO_ERROR, param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPC_CFG_INDNTF_REQ message.
 * It allows configuration of the peer ind/ntf/stop characteristic for a specified characteristic.
 * Will return an error code if that cfg char does not exist.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hrpc_cfg_indntf_req_handler(ke_msg_id_t const msgid,
                                       struct hrpc_cfg_indntf_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);

    if(state == HRPC_IDLE)
    {
        if(hrpc_env->env[conidx] != NULL)
        {

            // Status
            status = PRF_ERR_INVALID_PARAM;
            // Check if parameter is OK
            if((param->cfg_val == PRF_CLI_STOP_NTFIND) ||
                    (param->cfg_val == PRF_CLI_START_NTF))
            {
                // Status
                status = PRF_ERR_INEXISTENT_HDL;
                // Get handle of the client configuration
                uint16_t cfg_hdl =
                        hrpc_env->env[conidx]->hrs.descs[HRPC_DESC_HR_MEAS_CLI_CFG].desc_hdl;
                //check if the handle value exists
                if (cfg_hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    hrpc_env->env[conidx]->last_char_code = HRPC_DESC_HR_MEAS_CLI_CFG;

                    status = GAP_ERR_NO_ERROR;
                    // Go to the Busy state
                    ke_state_set(dest_id, HRPC_BUSY);
                    // Send GATT Write Request
                    prf_gatt_write_ntf_ind(&hrpc_env->prf_env, conidx, cfg_hdl, param->cfg_val);
                }
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }
    else if (state == HRPC_FREE)
    {
        status = GAP_ERR_DISCONNECTED;
    }
    else
    {
        // Another procedure is pending, keep the command for later
        msg_status = KE_MSG_SAVED;
        status = GAP_ERR_NO_ERROR;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        struct hrpc_cfg_indntf_rsp *rsp = KE_MSG_ALLOC(HRPC_CFG_INDNTF_RSP, src_id, dest_id, hrpc_cfg_indntf_rsp);
        rsp->status    = status;
        // Send the message
        ke_msg_send(rsp);
    }

    return (msg_status);
}

/**
****************************************************************************************
* @brief Handles reception of the @ref HRPC_WR_CNTL_POINT_REQ message.
* Check if the handle exists in profile(already discovered) and send request, otherwise
* error to APP.
* @param[in] msgid Id of the message received (probably unused).
* @param[in] param Pointer to the parameters of the message.
* @param[in] dest_id ID of the receiving task instance (probably unused).
* @param[in] src_id ID of the sending task instance.
* @return If the message was consumed or not.
****************************************************************************************
*/
__STATIC int hrpc_wr_cntl_point_req_handler(ke_msg_id_t const msgid,
                                       struct hrpc_wr_cntl_point_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t status = GAP_ERR_NO_ERROR;

    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    uint8_t state = ke_state_get(dest_id);

    if(state == HRPC_IDLE)
    {
        //this is mandatory readable if it is included in the peer's DB
        if(hrpc_env->env[conidx] != NULL)
        {
            //this is mandatory readable if it is included in the peer's DB
            if (hrpc_env->env[conidx]->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                if ((hrpc_env->env[conidx]->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].prop & ATT_CHAR_PROP_WR) == ATT_CHAR_PROP_WR)
                {
                    hrpc_env->env[conidx]->last_char_code = HRPC_CHAR_HR_CNTL_POINT;

                    // Send GATT Write Request
                    prf_gatt_write(&hrpc_env->prf_env, conidx, hrpc_env->env[conidx]->hrs.chars[HRPC_CHAR_HR_CNTL_POINT].val_hdl,
                            (uint8_t *)&param->val, sizeof(uint8_t), GATTC_WRITE);
                    // Go to the Busy state
                    ke_state_set(dest_id, HRPC_BUSY);
                }
                //write not allowed, so no point in continuing
                else
                {
                    status = PRF_ERR_NOT_WRITABLE;
                }
            }
            //send app error indication for inexistent handle for this characteristic
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }
    else if (state == HRPC_FREE)
    {
        status = GAP_ERR_DISCONNECTED;
    }
    else
    {
        // Another procedure is pending, keep the command for later
        msg_status = KE_MSG_SAVED;
        status = GAP_ERR_NO_ERROR;
    }

    if(status != GAP_ERR_NO_ERROR)
    {
        struct hrpc_wr_cntl_point_rsp *rsp = KE_MSG_ALLOC(HRPC_WR_CNTL_POINT_RSP, src_id, dest_id, hrpc_wr_cntl_point_rsp);
        rsp->status    = status;
        // Send the message
        ke_msg_send(rsp);
    }

    return (msg_status);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                        struct gattc_event_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);

    if(hrpc_env != NULL)
    {
        if((param->handle == hrpc_env->env[conidx]->hrs.chars[HRPC_CHAR_HR_MEAS].val_hdl) &&
                (param->type == GATTC_NOTIFY))
        {
            //build a HRPC_HR_MEAS_IND message with stable heart rate value code.
            struct hrpc_hr_meas_ind * ind = KE_MSG_ALLOC(
                    HRPC_HR_MEAS_IND,
                    prf_dst_task_get(&(hrpc_env->prf_env), conidx),
                    prf_src_task_get(&(hrpc_env->prf_env), conidx),
                    hrpc_hr_meas_ind);

            // unpack heart rate measurement.
            hrpc_unpack_meas_value(&(ind->meas_val), (uint8_t*) param->value, param->length);

            ke_msg_send(ind);
        }
    }
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(hrpc)
{
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {HRPC_ENABLE_REQ,               (ke_msg_func_t)hrpc_enable_req_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
    {HRPC_CFG_INDNTF_REQ,           (ke_msg_func_t)hrpc_cfg_indntf_req_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {HRPC_RD_CHAR_REQ,              (ke_msg_func_t)hrpc_rd_char_req_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {HRPC_WR_CNTL_POINT_REQ,        (ke_msg_func_t)hrpc_wr_cntl_point_req_handler},
};

void hrpc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct hrpc_env_tag *hrpc_env = PRF_ENV_GET(HRPC, hrpc);

    task_desc->msg_handler_tab = hrpc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(hrpc_msg_handler_tab);
    task_desc->state           = hrpc_env->state;
    task_desc->idx_max         = HRPC_IDX_MAX;
}

#endif /* (BLE_HR_COLLECTOR) */
/// @} HRPCTASK
