/**
 ****************************************************************************************
 * @addtogroup LANCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_LN_COLLECTOR)
#include "lan_common.h"

#include "gap.h"
#include "attm.h"
#include "lanc_task.h"
#include "lanc.h"
#include "gattc_task.h"
#include "ke_timer.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Location and Navigation service characteristics information
const struct prf_char_def lanc_lns_char[LANP_LANS_CHAR_MAX] =
{
    /// LN Feature
    [LANP_LANS_LN_FEAT_CHAR]    = {ATT_CHAR_LN_FEAT,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_RD},
    /// LN Measurement
    [LANP_LANS_LOC_SPEED_CHAR]  = {ATT_CHAR_LOC_SPEED,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_NTF},
    /// Position Quality
    [LANP_LANS_POS_Q_CHAR]      = {ATT_CHAR_POS_QUALITY,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_RD},
    /// SC Control Point
    [LANP_LANS_LN_CTNL_PT_CHAR] = {ATT_CHAR_LN_CNTL_PT,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
    /// Navigation
    [LANP_LANS_NAVIG_CHAR]  = {ATT_CHAR_NAVIGATION,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_NTF},
};

/// State machine used to retrieve Location and Navigation service characteristic descriptor information
const struct prf_char_desc_def lanc_lns_char_desc[LANC_DESC_MAX] =
{
    /// Location and Speed Char. - Client Characteristic Configuration
    [LANC_DESC_LOC_SPEED_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_MANDATORY,
                                        LANP_LANS_LOC_SPEED_CHAR},

    /// Control Point Char. - Client Characteristic Configuration
    [LANC_DESC_LN_CTNL_PT_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        LANP_LANS_LN_CTNL_PT_CHAR},

    /// Navigation Char. - Client Characteristic Configuration
    [LANC_DESC_NAVIGATION_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        LANP_LANS_NAVIG_CHAR},
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

    if(state == LANC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

        ASSERT_INFO(lanc_env != NULL, dest_id, src_id);
        ASSERT_INFO(lanc_env->env[conidx] != NULL, dest_id, src_id);

        if(lanc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve CPS characteristics and descriptors
            prf_extract_svc_info(ind, LANP_LANS_CHAR_MAX, &lanc_lns_char[0],  &lanc_env->env[conidx]->lans.chars[0],
                                        LANC_DESC_MAX, &lanc_lns_char_desc[0], &lanc_env->env[conidx]->lans.descs[0]);

            //Even if we get multiple responses we only store 1 range
            lanc_env->env[conidx]->lans.svc.shdl = ind->start_hdl;
            lanc_env->env[conidx]->lans.svc.ehdl = ind->end_hdl;
        }

        lanc_env->env[conidx]->nb_svc++;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int lanc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct lanc_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = GAP_ERR_NO_ERROR;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    uint8_t state = ke_state_get(dest_id);
    // Location and Navigation Profile Collector Role Task Environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

    ASSERT_INFO(lanc_env != NULL, dest_id, src_id);
    if((state == LANC_IDLE) && (lanc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        lanc_env->env[conidx] = (struct lanc_cnx_env*) ke_malloc(sizeof(struct lanc_cnx_env),KE_MEM_ATT_DB);
        memset(lanc_env->env[conidx], 0, sizeof(struct lanc_cnx_env));

        //Config connection, start discovering
        if(param->con_type == PRF_CON_DISCOVERY)
        {
            //start discovering CPS on peer
            prf_disc_svc_send(&(lanc_env->prf_env), conidx, ATT_SVC_LOCATION_AND_NAVIGATION);

            // Go to DISCOVERING state
            ke_state_set(dest_id, LANC_DISCOVERING);
        }
        //normal connection, get saved att details
        else
        {
            lanc_env->env[conidx]->lans = param->lans;

            //send APP confirmation that can start normal connection to TH
            lanc_enable_rsp_send(lanc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }

    else if(state != LANC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        lanc_enable_rsp_send(lanc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int lanc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct lanc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    uint8_t status = PRF_ERR_REQ_DISALLOWED;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (state == LANC_IDLE)
    {
        ASSERT_INFO(lanc_env != NULL, dest_id, src_id);
        // environment variable not ready
        if(lanc_env->env[conidx] == NULL)
        {
            status = PRF_APP_ERROR;
        }
        else
        {
            // Get the handler
            uint16_t hdl = lanc_get_read_handle_req (lanc_env, conidx, param);

            // Check if handle is viable
            if (hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Force the operation value
                param->operation = LANC_READ_OP_CODE;

                // Store the command structure
                lanc_env->env[conidx]->operation = param;
                msg_status = KE_MSG_NO_FREE;

                // Send the read request
                prf_read_char_send(&(lanc_env->prf_env), conidx,
                        lanc_env->env[conidx]->lans.svc.shdl,
                        lanc_env->env[conidx]->lans.svc.ehdl,
                        hdl);

                // Go to the Busy state
                ke_state_set(dest_id, LANC_BUSY);

                status = ATT_ERR_NO_ERROR;
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }
    }
    else if(state == LANC_FREE)
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
        lanc_send_cmp_evt(lanc_env, conidx, LANC_READ_OP_CODE, status);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int lanc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct lanc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

    uint8_t state = ke_state_get(dest_id);
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    if (lanc_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_REQ_DISALLOWED;
        // Handle
        uint16_t handle = ATT_INVALID_SEARCH_HANDLE;
        // Get connection index
        uint8_t conidx = KE_IDX_GET(dest_id);

        do
        {
            if (state != LANC_IDLE)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(lanc_env->env[conidx] != NULL);

            // Get handle
            status = lanc_get_write_desc_handle_req (conidx, param, lanc_env, &handle);
        } while (0);

        if ((status == GAP_ERR_NO_ERROR) && (handle != ATT_INVALID_SEARCH_HANDLE))
        {
            // Set the operation code
            param->operation = LANC_CFG_NTF_IND_OP_CODE;

            // Store the command structure
            lanc_env->env[conidx]->operation = param;
            msg_status = KE_MSG_NO_FREE;

            // Go to the Busy state
            ke_state_set(dest_id, LANC_BUSY);

            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&lanc_env->prf_env, conidx, handle, param->ntfind_cfg);
        }
    }
    else
    {
        lanc_send_no_conn_cmp_evt(dest_id, src_id, LANC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_LN_CTNL_PT_CFG_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int lanc_ln_ctnl_pt_cfg_req_handler(ke_msg_id_t const msgid,
                                         struct lanc_ln_ctnl_pt_cfg_req *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (lanc_env != NULL)
    {
        // Status
        uint8_t status = GAP_ERR_NO_ERROR;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > LANC_FREE);

            // Check the provided connection handle
            if (lanc_env->env[conidx] == NULL)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) != LANC_IDLE)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (lanc_env->env[conidx]->lans.descs[LANC_DESC_LN_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Request array declaration
                uint8_t req[LANP_LAN_LN_CNTL_PT_REQ_MAX_LEN];
                // Pack request
                uint8_t nb = lanc_pack_ln_ctnl_pt_req (param, req, &status);

                if (status == GAP_ERR_NO_ERROR)
                {
                    // Set the operation code
                    param->operation = LANC_LN_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    lanc_env->env[conidx]->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, LANC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(lanc_env->prf_env), conidx, lanc_env->env[conidx]->lans.chars[LANP_LANS_LN_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], nb, GATTC_WRITE);
                }
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != GAP_ERR_NO_ERROR)
        {
            // Send a complete event status to the application
            lanc_send_cmp_evt(lanc_env, conidx, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        lanc_send_no_conn_cmp_evt(dest_id, src_id, LANC_LN_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int lanc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->env[conidx]->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->env[conidx]->operation)->operation == LANC_LN_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        lanc_send_cmp_evt(lanc_env, conidx, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
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
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);
    // Status
    uint8_t status;

    if (lanc_env != NULL)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        uint8_t state = ke_state_get(dest_id);

        if (state == LANC_DISCOVERING)
        {
            status = param->status;

            if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (status == ATT_ERR_NO_ERROR))
            {
                // Discovery
                // check characteristic validity
                if(lanc_env->env[conidx]->nb_svc == 1)
                {
                    status = prf_check_svc_char_validity(LANP_LANS_CHAR_MAX,
                            lanc_env->env[conidx]->lans.chars,
                            lanc_lns_char);
                }
                // too much services
                else if (lanc_env->env[conidx]->nb_svc > 1)
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
                    status = prf_check_svc_char_desc_validity(LANC_DESC_MAX,
                            lanc_env->env[conidx]->lans.descs,
                            lanc_lns_char_desc,
                            lanc_env->env[conidx]->lans.chars);
                }
            }

            lanc_enable_rsp_send(lanc_env, conidx, status);
        }

        else if (state == LANC_BUSY)
        {
            switch (param->operation)
            {
                case GATTC_READ:
                {
                    // Send the complete event status
                    lanc_send_cmp_evt(lanc_env, conidx, LANC_READ_OP_CODE, param->status);
                } break;

                case GATTC_WRITE:
                case GATTC_WRITE_NO_RESPONSE:
                {
                    uint8_t operation = ((struct lanc_cmd *)lanc_env->env[conidx]->operation)->operation;

                    if (operation == LANC_CFG_NTF_IND_OP_CODE)
                    {
                        // Send the complete event status
                        lanc_send_cmp_evt(lanc_env, conidx, operation, param->status);
                    }

                    else if (operation == LANC_LN_CTNL_PT_CFG_WR_OP_CODE)
                    {
                        if (param->status == GAP_ERR_NO_ERROR)
                        {
                            // Start Timeout Procedure
                            ke_timer_set(LANC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                            // Wait for the response indication
                            ((struct lanc_cmd *)lanc_env->env[conidx]->operation)->operation = LANC_LN_CTNL_PT_CFG_IND_OP_CODE;
                        }
                        else
                        {
                            // Send the complete event status
                            lanc_send_cmp_evt(lanc_env, conidx, operation, param->status);
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
    uint8_t state = ke_state_get(dest_id);

    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

    if (state == LANC_BUSY)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        ASSERT_INFO(lanc_env != NULL, dest_id, src_id);
        ASSERT_INFO(lanc_env->env[conidx] != NULL, dest_id, src_id);

        // Send the read value to the HL
        struct lanc_value_ind *ind = KE_MSG_ALLOC(LANC_VALUE_IND,
                prf_dst_task_get(&(lanc_env->prf_env), conidx),
                dest_id,
                lanc_value_ind);

        switch (((struct lanc_read_cmd *)lanc_env->env[conidx]->operation)->read_code)
        {
            // Read LN Feature Characteristic value
            case (LANC_RD_LN_FEAT):
            {
                ind->value.LN_feat = co_read32p(&param->value[0]);

                // Mask the reserved bits
//                ind->value.LN_feat &= LANP_FEAT_ALL_SUPP;
            } break;

            // Read Position Quality
            case (LANC_RD_POS_Q):
            {
                // Read position quality
                lanc_unpack_pos_q_ind (param, ind);

            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (LANC_RD_WR_LOC_SPEED_CL_CFG):
            case (LANC_RD_WR_LN_CTNL_PT_CFG):
            case (LANC_RD_WR_NAVIGATION_CFG):
            {
                ind->value.ntf_cfg = co_read16p(&param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct lanc_read_cmd *)lanc_env->env[conidx]->operation)->read_code;

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
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

    if (lanc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {
                if (param->handle == lanc_env->env[conidx]->lans.chars[LANP_LANS_LOC_SPEED_CHAR].val_hdl)
                {
                    //Unpack location and speed
                    lanc_unpack_loc_speed_ind(conidx, param, lanc_env);
                }
                else if (param->handle == lanc_env->env[conidx]->lans.chars[LANP_LANS_NAVIG_CHAR].val_hdl)
                {
                    // Unpack navigation
                    lanc_unpack_navigation_ind(conidx, param, lanc_env);
                }
                else
                {
                    ASSERT_ERR(0);
                }
            } break;

            case (GATTC_INDICATE):
            {
                // confirm that indication has been correctly received
                struct gattc_event_cfm * cfm = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
                cfm->handle = param->handle;
                ke_msg_send(cfm);

                // Check if we were waiting for the indication
                if (lanc_env->env[conidx]->operation != NULL)
                {
                    if (((struct lanc_cmd *)lanc_env->env[conidx]->operation)->operation == LANC_LN_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(LANC_TIMEOUT_TIMER_IND, dest_id);

                        //Unpack control point
                        lanc_unpack_ln_ctln_pt_ind(param, prf_src_task_get(&(lanc_env->prf_env), conidx), prf_dst_task_get(&(lanc_env->prf_env), conidx));

                        // Send the complete event message
                        lanc_send_cmp_evt(lanc_env, conidx, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, GAP_ERR_NO_ERROR);
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
KE_MSG_HANDLER_TAB(lanc)
{
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {LANC_ENABLE_REQ,               (ke_msg_func_t)lanc_enable_req_handler},
    {LANC_READ_CMD,                 (ke_msg_func_t)lanc_read_cmd_handler},
    {LANC_CFG_NTFIND_CMD,           (ke_msg_func_t)lanc_cfg_ntfind_cmd_handler},
    {LANC_LN_CTNL_PT_CFG_REQ,       (ke_msg_func_t)lanc_ln_ctnl_pt_cfg_req_handler},
    {LANC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)lanc_timeout_timer_ind_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_EVENT_REQ_IND,           (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void lanc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_ENV_GET(LANC, lanc);

    task_desc->msg_handler_tab = lanc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(lanc_msg_handler_tab);
    task_desc->state           = lanc_env->state;
    task_desc->idx_max         = LANC_IDX_MAX;
}

#endif //(BLE_LN_COLLECTOR)

/// @} LANCTASK
