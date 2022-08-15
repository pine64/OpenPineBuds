/**
 ****************************************************************************************
 * @addtogroup RSCPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_RSC_COLLECTOR)
#include "rscp_common.h"
#include "rscpc_task.h"
#include "rscpc.h"

#include "gap.h"
#include "attm.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "compiler.h"
#include "ke_timer.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Running Speed and Cadence service characteristics information
const struct prf_char_def rscpc_rscs_char[RSCP_RSCS_CHAR_MAX] =
{
    /// RSC Measurement
    [RSCP_RSCS_RSC_MEAS_CHAR]      = {ATT_CHAR_RSC_MEAS,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_NTF},
    /// RSC Feature
    [RSCP_RSCS_RSC_FEAT_CHAR]      = {ATT_CHAR_RSC_FEAT,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD},
    /// Sensor Location
    [RSCP_RSCS_SENSOR_LOC_CHAR]    = {ATT_CHAR_SENSOR_LOC,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_RD},
    /// SC Control Point
    [RSCP_RSCS_SC_CTNL_PT_CHAR]    = {ATT_CHAR_SC_CNTL_PT,
                                      ATT_OPTIONAL,
                                      ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
};

/// State machine used to retrieve Running Speed and Cadence service characteristic descriptor information
const struct prf_char_desc_def rscpc_rscs_char_desc[RSCPC_DESC_MAX] =
{
    /// RSC Measurement Char. - Client Characteristic Configuration
    [RSCPC_DESC_RSC_MEAS_CL_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_MANDATORY,
                                       RSCP_RSCS_RSC_MEAS_CHAR},
    /// SC Control Point Char. - Client Characteristic Configuration
    [RSCPC_DESC_SC_CTNL_PT_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                       ATT_OPTIONAL,
                                       RSCP_RSCS_SC_CTNL_PT_CHAR},
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

    if(state == RSCPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);

        ASSERT_INFO(rscpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(rscpc_env->env[conidx] != NULL, dest_id, src_id);

        if(rscpc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve RSCS characteristics and descriptors
            prf_extract_svc_info(ind, RSCP_RSCS_CHAR_MAX, &rscpc_rscs_char[0],  &rscpc_env->env[conidx]->rscs.chars[0],
                                      RSCPC_DESC_MAX, &rscpc_rscs_char_desc[0], &rscpc_env->env[conidx]->rscs.descs[0]);

            //Even if we get multiple responses we only store 1 range
            rscpc_env->env[conidx]->rscs.svc.shdl = ind->start_hdl;
            rscpc_env->env[conidx]->rscs.svc.ehdl = ind->end_hdl;
        }

        rscpc_env->env[conidx]->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscpc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct rscpc_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = GAP_ERR_NO_ERROR;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    uint8_t state = ke_state_get(dest_id);
    // Running Speed and Cadence Profile Collector Role Task Environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);

    ASSERT_INFO(rscpc_env != NULL, dest_id, src_id);
    if ((state == RSCPC_IDLE) && (rscpc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        rscpc_env->env[conidx] = (struct rscpc_cnx_env*) ke_malloc(sizeof(struct rscpc_cnx_env), KE_MEM_ATT_DB);
        memset(rscpc_env->env[conidx], 0, sizeof(struct rscpc_cnx_env));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            prf_disc_svc_send(&(rscpc_env->prf_env), conidx, ATT_SVC_RUNNING_SPEED_CADENCE);

            // Go to DISCOVERING state
            ke_state_set(dest_id, RSCPC_DISCOVERING);
        }
        // Bond information is provided
        else
        {
            rscpc_env->env[conidx]->rscs = param->rscs;

            //send APP confirmation that can start normal connection to TH
            rscpc_enable_rsp_send(rscpc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }
    else if (state != RSCPC_FREE)
    {
        // The message will be forwarded towards the good task instance
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if(status != GAP_ERR_NO_ERROR)
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        rscpc_enable_rsp_send(rscpc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscpc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct rscpc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (state == RSCPC_IDLE)
    {
        // State is Connected or Busy
        ASSERT_INFO(rscpc_env != NULL, dest_id, src_id);

        // Check the current state
        if (rscpc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else    // State is RSCPC_CONNECTED
        {
            // Attribute Handle
            uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;

            switch (param->read_code)
            {
                // Read RSC Feature
                case (RSCPC_RD_RSC_FEAT):
                {
                    handle = rscpc_env->env[conidx]->rscs.chars[RSCP_RSCS_RSC_FEAT_CHAR].val_hdl;
                } break;

                // Read Sensor Location
                case (RSCPC_RD_SENSOR_LOC):
                {
                    handle = rscpc_env->env[conidx]->rscs.chars[RSCP_RSCS_SENSOR_LOC_CHAR].val_hdl;
                } break;

                // Read RSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_RSC_MEAS_CFG):
                {
                    handle = rscpc_env->env[conidx]->rscs.descs[RSCPC_DESC_RSC_MEAS_CL_CFG].desc_hdl;
                } break;

                // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
                {
                    handle = rscpc_env->env[conidx]->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;
                } break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                } break;
            }

            // Check if handle is viable
            if (handle != ATT_INVALID_SEARCH_HANDLE)
            {
                // Force the operation value
                param->operation       = RSCPC_READ_OP_CODE;

                // Store the command structure
                rscpc_env->env[conidx]->operation  = (void *)param;
                msg_status            = KE_MSG_NO_FREE;

                // Send the read request
                prf_read_char_send(&(rscpc_env->prf_env), conidx,
                            rscpc_env->env[conidx]->rscs.svc.shdl,
                            rscpc_env->env[conidx]->rscs.svc.ehdl,
                            handle);

                // Go to the Busy state
                ke_state_set(dest_id, RSCPC_BUSY);

                status = ATT_ERR_NO_ERROR;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
    }
    else if (state == RSCPC_FREE)
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
        // Send the complete event message to the task id stored in the environment
        rscpc_send_cmp_evt(rscpc_env, conidx, RSCPC_READ_OP_CODE, status);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscpc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct rscpc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    if (rscpc_env != NULL)
    {
        // Status
        uint8_t status     = PRF_ERR_REQ_DISALLOWED;
        // Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Get connection index
        uint8_t conidx = KE_IDX_GET(dest_id);

        do
        {
            if (ke_state_get(dest_id) != RSCPC_IDLE)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is GAP_ERR_NO_ERROR, no message will be sent to the application
                break;
            }

            ASSERT_ERR(rscpc_env->env[conidx] != NULL);

            switch(param->desc_code)
            {
                // Write RSC Measurement Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_RSC_MEAS_CFG):
                {
                    if (param->ntfind_cfg <= PRF_CLI_START_NTF)
                    {
                        handle = rscpc_env->env[conidx]->rscs.descs[RSCPC_DESC_RSC_MEAS_CL_CFG].desc_hdl;

                        status = GAP_ERR_NO_ERROR;
                        // The characteristic is mandatory and the descriptor is mandatory
                        ASSERT_ERR(handle != ATT_INVALID_SEARCH_HANDLE);
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write SC Control Point Characteristic Client Char. Cfg. Descriptor Value
                case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
                {
                    if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                        (param->ntfind_cfg == PRF_CLI_START_IND))
                    {
                        handle = rscpc_env->env[conidx]->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl;

                        status = GAP_ERR_NO_ERROR;
                        if (handle == ATT_INVALID_SEARCH_HANDLE)
                        {
                            // The descriptor has not been found.
                            status = PRF_ERR_INEXISTENT_HDL;
                        }
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                } break;
            }
        } while (0);


        if ((status == GAP_ERR_NO_ERROR) && (handle != ATT_INVALID_SEARCH_HANDLE))
        {
            // Set the operation code
            param->operation = RSCPC_CFG_NTF_IND_OP_CODE;

            // Store the command structure
            rscpc_env->env[conidx]->operation   = param;
            msg_status = KE_MSG_NO_FREE;

            // Go to the Busy state
            ke_state_set(dest_id, RSCPC_BUSY);

            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&rscpc_env->prf_env, conidx, handle, param->ntfind_cfg);
        }
    }
    else
    {
        // No connection
        rscps_send_no_conn_cmp_evt(dest_id, src_id, RSCPC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscpc_ctnl_pt_cfg_req_handler(ke_msg_id_t const msgid,
                                         struct rscpc_ctnl_pt_cfg_req *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (rscpc_env != NULL)
    {
        // Status
        uint8_t status     = GAP_ERR_NO_ERROR;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > RSCPC_FREE);

            // Check the provided connection handle
            if (rscpc_env->env[conidx] == NULL)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) != RSCPC_IDLE)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is GAP_ERR_NO_ERROR, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (rscpc_env->env[conidx]->rscs.descs[RSCPC_DESC_SC_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Packed request
                uint8_t req[RSCP_SC_CNTL_PT_REQ_MAX_LEN];
                // Request Length
                uint8_t req_len = RSCP_SC_CNTL_PT_REQ_MIN_LEN;

                // Set the operation code
                req[0] = param->sc_ctnl_pt.op_code;

                // Fulfill the message according to the operation code
                switch (param->sc_ctnl_pt.op_code)
                {
                    case (RSCP_CTNL_PT_OP_RESERVED):
                    {
                        // Do nothing, used to generate an "Operation Code not supported" error.
                    } break;

                    case (RSCP_CTNL_PT_OP_SET_CUMUL_VAL):
                    {
                        // Set the cumulative value
                        co_write32p(&req[req_len], param->sc_ctnl_pt.value.cumul_val);
                        // Update length
                        req_len += 4;
                    } break;

                    case (RSCP_CTNL_PT_OP_UPD_LOC):
                    {
                        // Set the sensor location
                        req[req_len] = param->sc_ctnl_pt.value.sensor_loc;
                        // Update length
                        req_len++;
                    } break;

                    case (RSCP_CTNL_PT_OP_START_CALIB):
                    case (RSCP_CTNL_PT_OP_REQ_SUPP_LOC):
                    {
                        // Nothing more to do
                    } break;

                    default:
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    } break;
                }

                if (status == GAP_ERR_NO_ERROR)
                {
                    // Set the operation code
                    param->operation = RSCPC_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    rscpc_env->env[conidx]->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, RSCPC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(rscpc_env->prf_env), conidx, rscpc_env->env[conidx]->rscs.chars[RSCP_RSCS_SC_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], req_len, GATTC_WRITE);
                }
            }
            else
            {
                // The SC Control Point Characteristic has not been found
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != GAP_ERR_NO_ERROR)
        {
            // Send a complete event status to the application
            rscpc_send_cmp_evt(rscpc_env, conidx, RSCPC_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        rscps_send_no_conn_cmp_evt(dest_id, src_id, RSCPC_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscpc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if ((rscpc_env != NULL) && (rscpc_env->env[conidx] != NULL))
    {
        ASSERT_ERR(rscpc_env->env[conidx]->operation != NULL);
        ASSERT_ERR(((struct rscpc_cmd *)rscpc_env->env[conidx]->operation)->operation == RSCPC_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        rscpc_send_cmp_evt(rscpc_env, conidx, RSCPC_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
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
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);
    // Status
    uint8_t status;

    if (rscpc_env != NULL)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        uint8_t state = ke_state_get(dest_id);

        if (state == RSCPC_DISCOVERING)
        {
            status = param->status;

            if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (status == ATT_ERR_NO_ERROR))
            {
                // Discovery
                // check characteristic validity
                if(rscpc_env->env[conidx]->nb_svc == 1)
                {
                    status = prf_check_svc_char_validity(RSCP_RSCS_CHAR_MAX,
                            rscpc_env->env[conidx]->rscs.chars,
                            rscpc_rscs_char);
                }
                // too much services
                else if (rscpc_env->env[conidx]->nb_svc > 1)
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
                    status = prf_check_svc_char_desc_validity(RSCPC_DESC_MAX,
                            rscpc_env->env[conidx]->rscs.descs,
                            rscpc_rscs_char_desc,
                            rscpc_env->env[conidx]->rscs.chars);
                }
            }

            rscpc_enable_rsp_send(rscpc_env, conidx, status);
        }

        else if (state == RSCPC_BUSY)
        {
            switch (param->operation)
            {
                case GATTC_READ:
                {
                    // Send the complete event status
                    rscpc_send_cmp_evt(rscpc_env, conidx, RSCPC_READ_OP_CODE, param->status);
                } break;

                case GATTC_WRITE:
                case GATTC_WRITE_NO_RESPONSE:
                {
                    uint8_t operation = ((struct rscpc_cmd *)rscpc_env->env[conidx]->operation)->operation;

                    if (operation == RSCPC_CFG_NTF_IND_OP_CODE)
                    {
                        // Send the complete event status
                        rscpc_send_cmp_evt(rscpc_env, conidx, operation, param->status);
                    }

                    else if (operation == RSCPC_CTNL_PT_CFG_WR_OP_CODE)
                    {
                        if (param->status == GAP_ERR_NO_ERROR)
                        {
                            // Start Timeout Procedure
                            ke_timer_set(RSCPC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                            // Wait for the response indication
                            ((struct rscpc_cmd *)rscpc_env->env[conidx]->operation)->operation = RSCPC_CTNL_PT_CFG_IND_OP_CODE;
                        }
                        else
                        {
                            // Send the complete event status
                            rscpc_send_cmp_evt(rscpc_env, conidx, operation, param->status);
                        }
                    }
                } break;

                case GATTC_REGISTER:
                case GATTC_UNREGISTER:
                {
                    // Do nothing
                } break;

                default:
                {
                    ASSERT_ERR(0);
                } break;
            }
        }
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
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
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);

    if (ke_state_get(dest_id) == RSCPC_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        ASSERT_INFO(rscpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(rscpc_env->env[conidx] != NULL, dest_id, src_id);

        // Send the read value to the HL
        struct rscpc_value_ind *ind = KE_MSG_ALLOC(RSCPC_VALUE_IND,
                prf_dst_task_get(&(rscpc_env->prf_env), conidx),
                dest_id,
                rscpc_value_ind);

        switch (((struct rscpc_read_cmd *)rscpc_env->env[conidx]->operation)->read_code)
        {
            // Read RSC Feature Characteristic value
            case (RSCPC_RD_RSC_FEAT):
            {
                ind->value.sensor_feat = co_read16p(param->value);

                // Mask the unused bits
//                ind->value.sensor_feat &= RSCP_FEAT_ALL_SUPP;
            } break;

            // Read Sensor Location Characteristic value
            case (RSCPC_RD_SENSOR_LOC):
            {
                ind->value.sensor_loc = param->value[0];
            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (RSCPC_RD_WR_RSC_MEAS_CFG):
            case (RSCPC_RD_WR_SC_CTNL_PT_CFG):
            {
                co_write16p(&ind->value.ntf_cfg, param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct rscpc_read_cmd *)rscpc_env->env[conidx]->operation)->read_code;

        // Send the message to the application
        ke_msg_send(ind);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
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
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);

    if (rscpc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {
                // Offset
                uint8_t offset = RSCP_RSC_MEAS_MIN_LEN;

                // RSC Measurement value has been received
                struct rscpc_value_ind *ind = KE_MSG_ALLOC(RSCPC_VALUE_IND,
                        prf_dst_task_get(&(rscpc_env->prf_env), conidx),
                        prf_src_task_get(&(rscpc_env->prf_env), conidx),
                        rscpc_value_ind);

                // Attribute code
                ind->att_code = RSCPC_NTF_RSC_MEAS;

                /*----------------------------------------------------
                 * Unpack Measurement --------------------------------
                 *----------------------------------------------------*/

                // Flags
                ind->value.rsc_meas.flags      = param->value[0];
                // Instantaneous Speed
                ind->value.rsc_meas.inst_speed = co_read16p((uint16_t *)&param->value[1]);
                // Instantaneous Cadence
                ind->value.rsc_meas.inst_cad   = param->value[3];

                // Instantaneous Stride Length
                if (param->value[0] & RSCP_MEAS_INST_STRIDE_LEN_PRESENT)
                {
                    ind->value.rsc_meas.inst_stride_len = co_read16p(&param->value[offset]);
                    offset += 2;
                }

                // Total Distance
                if (param->value[0] & RSCP_MEAS_TOTAL_DST_MEAS_PRESENT)
                {
                    ind->value.rsc_meas.total_dist = co_read32p(&param->value[offset]);
                }

                ke_msg_send(ind);
            } break;

            case (GATTC_INDICATE):
            {
                // confirm that indication has been correctly received
                struct gattc_event_cfm * cfm = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
                cfm->handle = param->handle;
                ke_msg_send(cfm);

                // Check if we were waiting for the indication
                if (rscpc_env->env[conidx]->operation != NULL)
                {
                    if (((struct rscpc_cmd *)rscpc_env->env[conidx]->operation)->operation == RSCPC_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(RSCPC_TIMEOUT_TIMER_IND, dest_id);

                        // RSC Measurement value has been received
                        struct rscpc_ctnl_pt_cfg_rsp *ind = KE_MSG_ALLOC(RSCPC_CTNL_PT_CFG_RSP,
                                prf_dst_task_get(&(rscpc_env->prf_env), conidx),
                                prf_src_task_get(&(rscpc_env->prf_env), conidx),
                                rscpc_ctnl_pt_cfg_rsp);

                        // Requested operation code
                        ind->ctnl_pt_rsp.req_op_code = param->value[1];
                        // Response value
                        ind->ctnl_pt_rsp.resp_value  = param->value[2];

                        // Get the list of supported sensor locations if needed
                        if ((ind->ctnl_pt_rsp.req_op_code == RSCP_CTNL_PT_OP_REQ_SUPP_LOC) &&
                            (ind->ctnl_pt_rsp.resp_value == RSCP_CTNL_PT_RESP_SUCCESS) &&
                            (param->length > 3))
                        {
                            // Get the number of supported locations that have been received
                            uint8_t nb_supp_loc = (param->length - 3);
                            // Counter
                            uint8_t counter;
                            // Location
                            uint8_t loc;

                            for (counter = 0; counter < nb_supp_loc; counter++)
                            {
                                loc = param->value[counter + 3];

                                // Check if valid
                                if (loc < RSCP_LOC_MAX)
                                {
                                    ind->ctnl_pt_rsp.supp_loc |= (1 << loc);
                                }
                            }
                        }

                        // Send the message
                        ke_msg_send(ind);

                        // Send the complete event message
                        rscpc_send_cmp_evt(rscpc_env, conidx, RSCPC_CTNL_PT_CFG_WR_OP_CODE, GAP_ERR_NO_ERROR);
                    }
                    // else drop the message
                }
                // else drop the message
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(rscpc)
{
    {GATTC_SDP_SVC_IND,              (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {RSCPC_ENABLE_REQ,               (ke_msg_func_t)rscpc_enable_req_handler},
    {RSCPC_READ_CMD,                 (ke_msg_func_t)rscpc_read_cmd_handler},
    {GATTC_READ_IND,                 (ke_msg_func_t)gattc_read_ind_handler},
    {RSCPC_CFG_NTFIND_CMD,           (ke_msg_func_t)rscpc_cfg_ntfind_cmd_handler},
    {RSCPC_CTNL_PT_CFG_REQ,          (ke_msg_func_t)rscpc_ctnl_pt_cfg_req_handler},
    {RSCPC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)rscpc_timeout_timer_ind_handler},
    {GATTC_EVENT_IND,                (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_EVENT_REQ_IND,            (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                  (ke_msg_func_t)gattc_cmp_evt_handler},
};

void rscpc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct rscpc_env_tag *rscpc_env = PRF_ENV_GET(RSCPC, rscpc);

    task_desc->msg_handler_tab = rscpc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(rscpc_msg_handler_tab);
    task_desc->state           = rscpc_env->state;
    task_desc->idx_max         = RSCPC_IDX_MAX;
}

#endif //(BLE_RSC_COLLECTOR)

/// @} RSCPCTASK
