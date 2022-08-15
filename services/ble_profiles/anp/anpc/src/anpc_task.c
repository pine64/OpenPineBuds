/**
 ****************************************************************************************
 * @addtogroup ANPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_AN_CLIENT)
#include "anp_common.h"

#include "gap.h"
#include "anpc.h"
#include "anpc_task.h"
#include "gattm_task.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Alert Notification service characteristics information
const struct prf_char_def anpc_ans_char[ANPC_CHAR_MAX] =
{
    /// Supported New Alert Category
    [ANPC_CHAR_SUP_NEW_ALERT_CAT]      = {ATT_CHAR_SUP_NEW_ALERT_CAT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_RD},
    /// New Alert
    [ANPC_CHAR_NEW_ALERT]              = {ATT_CHAR_NEW_ALERT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_NTF},
    /// Supported Unread Alert Category
    [ANPC_CHAR_SUP_UNREAD_ALERT_CAT]   = {ATT_CHAR_SUP_UNREAD_ALERT_CAT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_RD},
    /// Unread Alert Status
    [ANPC_CHAR_UNREAD_ALERT_STATUS]    = {ATT_CHAR_UNREAD_ALERT_STATUS,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_NTF},
    /// Alert Notification Control Point
    [ANPC_CHAR_ALERT_NTF_CTNL_PT]      = {ATT_CHAR_ALERT_NTF_CTNL_PT,
                                          ATT_MANDATORY,
                                          ATT_CHAR_PROP_WR},
};

/// State machine used to retrieve Phone Alert Status service characteristic descriptor information
const struct prf_char_desc_def anpc_ans_char_desc[ANPC_DESC_MAX] =
{
    /// New Alert Char. - Client Characteristic Configuration
    [ANPC_DESC_NEW_ALERT_CL_CFG]            = {ATT_DESC_CLIENT_CHAR_CFG,
                                               ATT_MANDATORY,
                                               ANPC_CHAR_NEW_ALERT},
    /// Unread Alert Status Char. - Client Characteristic Configuration
    [ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                               ATT_MANDATORY,
                                               ANPC_CHAR_UNREAD_ALERT_STATUS},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int anpc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct anpc_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Phone Alert Status Profile Client Role Task Environment
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state = ke_state_get(dest_id);

    if ((state == ANPC_IDLE) && (anpc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        anpc_env->env[conidx] = (struct anpc_cnx_env*) ke_malloc(sizeof(struct anpc_cnx_env),KE_MEM_ATT_DB);
        memset(anpc_env->env[conidx], 0, sizeof(struct anpc_cnx_env));

        anpc_env->env[conidx]->last_char_code     = ANPC_ENABLE_OP_CODE;

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            prf_disc_svc_send(&(anpc_env->prf_env), conidx, ATT_SVC_ALERT_NTF);

            // Go to DISCOVERING state
            ke_state_set(dest_id, ANPC_DISCOVERING);

            // Configure the environment for a discovery procedure
            anpc_env->env[conidx]->last_uuid_req = ATT_SVC_ALERT_NTF;
        }
        // Bond information is provided
        else
        {
            // Keep the provided database content
            memcpy(&anpc_env->env[conidx]->ans, &param->ans, sizeof(struct anpc_ans_content));

            //send APP confirmation that can start normal connection to TH
            anpc_enable_rsp_send(anpc_env, conidx, GAP_ERR_NO_ERROR);
        }
    }

    else if (state != ANPC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        anpc_enable_rsp_send(anpc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int anpc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct anpc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);

    if (anpc_env->env[conidx] != NULL)
    {
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Status
        uint8_t status     = GAP_ERR_NO_ERROR;

        // Check the provided connection handle
        do
        {
            // Check the current state
            if (ke_state_get(dest_id) == ANPC_BUSY)
            {
                // Keep the request for later, status is GAP_ERR_NO_ERROR
                msg_status = KE_MSG_SAVED;
                break;
            }

            switch (param->read_code)
            {
                // Read Supported New Alert
                case (ANPC_RD_SUP_NEW_ALERT_CAT):
                {
                    handle = anpc_env->env[conidx]->ans.chars[ANPC_CHAR_SUP_NEW_ALERT_CAT].val_hdl;
                } break;

                // Read Supported Unread Alert
                case (ANPC_RD_SUP_UNREAD_ALERT_CAT):
                {
                    handle = anpc_env->env[conidx]->ans.chars[ANPC_CHAR_SUP_UNREAD_ALERT_CAT].val_hdl;
                } break;

                // Read New Alert Characteristic Client Char. Cfg. Descriptor Value
                case (ANPC_RD_WR_NEW_ALERT_CFG):
                {
                    handle = anpc_env->env[conidx]->ans.descs[ANPC_DESC_NEW_ALERT_CL_CFG].desc_hdl;
                } break;

                // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
                case (ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG):
                {
                    handle = anpc_env->env[conidx]->ans.descs[ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG].desc_hdl;
                } break;

                default:
                {
                    status = PRF_ERR_INVALID_PARAM;
                } break;
            }

            // Check if handle is viable
            if ((handle != ATT_INVALID_SEARCH_HANDLE) &&
                (status == GAP_ERR_NO_ERROR))
            {
                // Force the operation value
                param->operation      = ANPC_READ_OP_CODE;

                // Store the command structure
                anpc_env->env[conidx]->operation   = param;
                msg_status = KE_MSG_NO_FREE;

                // Send the read request
                prf_read_char_send(&(anpc_env->prf_env), conidx,
                        anpc_env->env[conidx]->ans.svc.shdl,
                        anpc_env->env[conidx]->ans.svc.ehdl,
                        handle);

                // Go to the Busy state
                ke_state_set(dest_id, ANPC_BUSY);
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        }while(0);

        if (status != GAP_ERR_NO_ERROR)
        {
            anpc_send_cmp_evt(anpc_env, conidx, ANPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists
        anpc_send_cmp_evt(anpc_env, conidx, ANPC_READ_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int anpc_write_cmd_handler(ke_msg_id_t const msgid,
                                   struct anpc_write_cmd *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = GAP_ERR_NO_ERROR;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (anpc_env->env[conidx] != NULL)
    {
        do
        {
            // Check the current state
            if (ke_state_get(dest_id) != ANPC_IDLE)
            {
                msg_status = KE_MSG_SAVED;
                break;
            }
            // Attribute handle
            uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
            // Length
            uint8_t length;

            switch (param->write_code)
            {
                // Write Alert Notification Control Point Characteristic Value
                case (ANPC_WR_ALERT_NTF_CTNL_PT):
                {
                    // Check the Category ID and the Command ID
                    if ((param->value.ctnl_pt.cmd_id < CMD_ID_NB) &&
                        ((param->value.ctnl_pt.cat_id < CAT_ID_NB) || (param->value.ctnl_pt.cat_id == CAT_ID_ALL_SUPPORTED_CAT)))
                    {
                        handle  = anpc_env->env[conidx]->ans.chars[ANPC_CHAR_ALERT_NTF_CTNL_PT].val_hdl;
                        length  = sizeof(struct anp_ctnl_pt);
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write New Alert Characteristic Client Char. Cfg. Descriptor Value
                case (ANPC_RD_WR_NEW_ALERT_CFG):
                {
                    if (param->value.new_alert_ntf_cfg <= PRF_CLI_START_NTF)
                    {
                        handle  = anpc_env->env[conidx]->ans.descs[ANPC_DESC_NEW_ALERT_CL_CFG].desc_hdl;
                        length  = sizeof(uint16_t);
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write Unread Alert Status Characteristic Client Char. Cfg. Descriptor Value
                case (ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG):
                {
                    if (param->value.unread_alert_status_ntf_cfg <= PRF_CLI_START_NTF)
                    {
                        handle  = anpc_env->env[conidx]->ans.descs[ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG].desc_hdl;
                        length  = sizeof(uint16_t);
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

            if (status == GAP_ERR_NO_ERROR)
            {
                // Check if handle is viable
                if (handle != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Send the write request
                    prf_gatt_write(&(anpc_env->prf_env), conidx, handle,
                                   (uint8_t *)&param->value, length, GATTC_WRITE);

                    // Force the operation value
                    param->operation      = ANPC_WRITE_OP_CODE;
                    // Store the command structure
                    anpc_env->env[conidx]->operation   = param;

                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, ANPC_BUSY);
                }
                else
                {
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
        }while(0);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        anpc_send_cmp_evt(anpc_env, conidx, ANPC_WRITE_OP_CODE, status);
    }

    return (int)msg_status;
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
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Status
    uint8_t status;


    if (anpc_env->env[conidx] != NULL)
    {
        uint8_t state = ke_state_get(dest_id);

        if (state == ANPC_DISCOVERING)
        {
            status = param->status;

            if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (status == ATT_ERR_NO_ERROR))
            {
                // Discovery
                // check characteristic validity
                if(anpc_env->env[conidx]->nb_svc == 1)
                {
                    status = prf_check_svc_char_validity(ANPC_CHAR_MAX,
                            anpc_env->env[conidx]->ans.chars,
                            anpc_ans_char);
                }
                // too much services
                else if (anpc_env->env[conidx]->nb_svc > 1)
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
                    status = prf_check_svc_char_desc_validity(ANPC_DESC_MAX,
                            anpc_env->env[conidx]->ans.descs,
                            anpc_ans_char_desc,
                            anpc_env->env[conidx]->ans.chars);
                }
            }

            anpc_enable_rsp_send(anpc_env, conidx, status);
        }

        else if (state == ANPC_BUSY)
        {
            switch (param->operation)
            {
                case GATTC_READ:
                {
                    switch (((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation)
                    {
                        // Read Supported New Alert Category Characteristic value
                        case (ANPC_ENABLE_RD_NEW_ALERT_OP_CODE):
                        {
                            if (param->status == GAP_ERR_NO_ERROR)
                            {
                                // Force the operation value
                                ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE;
                                // Check Supported Unread Alert Category
                                prf_read_char_send(&(anpc_env->prf_env), conidx,
                                                   anpc_env->env[conidx]->ans.svc.shdl, anpc_env->env[conidx]->ans.svc.ehdl,
                                                   anpc_env->env[conidx]->ans.chars[ANPC_CHAR_SUP_UNREAD_ALERT_CAT].val_hdl);
                            }
                            else
                            {
                                anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, param->status);
                            }
                        } break;

                        case (ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE):
                        {
                            if (param->status == GAP_ERR_NO_ERROR)
                            {
                                // If peer device was unknown, stop the procedure
                                if (((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->con_type == PRF_CON_DISCOVERY)
                                {
                                    anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, GAP_ERR_NO_ERROR);
                                }
                                // If peer device was already known (bonded), start Recovery from Connection Loss procedure
                                else
                                {
                                    // Reset the environment
                                    anpc_env->env[conidx]->last_uuid_req = CAT_ID_SPL_ALERT;

                                    if (anpc_found_next_alert_cat(anpc_env, conidx, ((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->new_alert_enable))
                                    {
                                        // Force the operation value
                                        ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_WR_NEW_ALERT_OP_CODE;
                                        // Enable sending of notifications for the found category ID
                                        anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_EN_NEW_IN_ALERT_NTF, anpc_env->env[conidx]->last_uuid_req - 1);
                                    }
                                    else
                                    {
                                        // Reset the environment
                                        anpc_env->env[conidx]->last_uuid_req = CAT_ID_SPL_ALERT;

                                        if (anpc_found_next_alert_cat(anpc_env, conidx, ((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->unread_alert_enable))
                                        {
                                            // Force the operation value
                                            ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE;
                                            // Enable sending of notifications for the found category ID
                                            anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->env[conidx]->last_uuid_req - 1);
                                        }
                                        else
                                        {
                                            anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, GAP_ERR_NO_ERROR);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, param->status);
                            }
                        } break;

                        case (ANPC_READ_OP_CODE):
                        {
                            // Inform the requester that the read procedure is over
                            anpc_send_cmp_evt(anpc_env, conidx, ANPC_READ_OP_CODE, param->status);
                        } break;

                        default:
                        {
                            ASSERT_ERR(0);
                        }
                    }
                } break;

                case GATTC_WRITE:
                case GATTC_WRITE_NO_RESPONSE:
                {
                    // Retrieve the operation currently performed
                    uint8_t operation = ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation;

                    switch (operation)
                    {
                        case (ANPC_WRITE_OP_CODE):
                        {
                            uint8_t wr_code = ((struct anpc_write_cmd *)anpc_env->env[conidx]->operation)->write_code;

                            if ((wr_code == ANPC_RD_WR_NEW_ALERT_CFG) ||
                                (wr_code == ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG) ||
                                (wr_code == ANPC_WR_ALERT_NTF_CTNL_PT))
                            {
                                anpc_send_cmp_evt(anpc_env, conidx, ANPC_WRITE_OP_CODE, param->status);
                            }
                            else
                            {
                                ASSERT_ERR(0);
                            }
                        } break;

                        case (ANPC_ENABLE_WR_NEW_ALERT_OP_CODE):
                        {
                            // Look for the next category to enable
                            if (anpc_found_next_alert_cat(anpc_env, conidx, ((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->new_alert_enable))
                            {
                                // Enable sending of notifications for the found category ID
                                anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_EN_NEW_IN_ALERT_NTF, anpc_env->env[conidx]->last_uuid_req - 1);
                            }
                            else
                            {
                                // Force the operation value
                                ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_WR_NTF_NEW_ALERT_OP_CODE;
                                // Send a "Notify New Alert Immediately" command with the Category ID field set to 0xFF
                                anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_NTF_NEW_IN_ALERT_IMM, CAT_ID_ALL_SUPPORTED_CAT);
                            }
                        } break;

                        case (ANPC_ENABLE_WR_NTF_NEW_ALERT_OP_CODE):
                        {
                            // Reset the environment
                            anpc_env->env[conidx]->last_uuid_req = CAT_ID_SPL_ALERT;

                            if (anpc_found_next_alert_cat(anpc_env, conidx, ((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->unread_alert_enable))
                            {
                                // Force the operation value
                                ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE;
                                // Enable sending of notifications for the found category ID
                                anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->env[conidx]->last_uuid_req - 1);
                            }
                            else
                            {
                                anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, GAP_ERR_NO_ERROR);
                            }
                        } break;

                        case (ANPC_ENABLE_WR_UNREAD_ALERT_OP_CODE):
                        {
                            // Look for the next category to enable
                            if (anpc_found_next_alert_cat(anpc_env, conidx, ((struct anpc_enable_req *)anpc_env->env[conidx]->operation)->unread_alert_enable))
                            {
                                // Enable sending of notifications for the found category ID
                                anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_EN_UNREAD_CAT_STATUS_NTF, anpc_env->env[conidx]->last_uuid_req - 1);
                            }
                            else
                            {
                                // Force the operation value
                                ((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation = ANPC_ENABLE_WR_NTF_UNREAD_ALERT_OP_CODE;
                                // Send a "Notify New Alert Immediately" command with the Category ID field set to 0xFF
                                anpc_write_alert_ntf_ctnl_pt(anpc_env, conidx, CMD_ID_NTF_UNREAD_CAT_STATUS_IMM, CAT_ID_ALL_SUPPORTED_CAT);
                            }
                        } break;

                        case (ANPC_ENABLE_WR_NTF_UNREAD_ALERT_OP_CODE):
                        {
                            // The discovery procedure is over
                            anpc_send_cmp_evt(anpc_env, conidx, ANPC_ENABLE_OP_CODE, param->status);
                        } break;

                        default:
                        {
                            ASSERT_ERR(0);
                        } break;
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

    if(state == ANPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);

        ASSERT_INFO(anpc_env != NULL, dest_id, src_id);
        ASSERT_INFO(anpc_env->env[conidx] != NULL, dest_id, src_id);

        if(anpc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve ANS characteristics and descriptors
            prf_extract_svc_info(ind, ANPC_CHAR_MAX, &anpc_ans_char[0],  &anpc_env->env[conidx]->ans.chars[0],
                                      ANPC_DESC_MAX, &anpc_ans_char_desc[0], &anpc_env->env[conidx]->ans.descs[0]);

            //Even if we get multiple responses we only store 1 range
            anpc_env->env[conidx]->ans.svc.shdl = ind->start_hdl;
            anpc_env->env[conidx]->ans.svc.ehdl = ind->end_hdl;
        }

        anpc_env->env[conidx]->nb_svc++;
    }

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
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (anpc_env->env[conidx] != NULL)
    {
        ASSERT_ERR(ke_state_get(dest_id) == ANPC_BUSY);

        // Prepare the indication to send to the application
        struct anpc_value_ind *ind = KE_MSG_ALLOC(ANPC_VALUE_IND,
                prf_dst_task_get(&(anpc_env->prf_env), conidx),
                dest_id,
                anpc_value_ind);

        switch (((struct anpc_cmd *)anpc_env->env[conidx]->operation)->operation)
        {
            // Read Supported New Alert Category Characteristic value
            case (ANPC_ENABLE_RD_NEW_ALERT_OP_CODE):
            {
                ind->att_code = ANPC_RD_SUP_NEW_ALERT_CAT;
                ind->value.supp_cat.cat_id_mask_0 = param->value[0];
                // If cat_id_mask_1 not present, shall be considered as 0
                ind->value.supp_cat.cat_id_mask_1 = (param->length > 1)
                                                    ? param->value[1] : 0x00;
            } break;

            case (ANPC_ENABLE_RD_UNREAD_ALERT_OP_CODE):
            {
                ind->att_code = ANPC_RD_SUP_UNREAD_ALERT_CAT;
                ind->value.supp_cat.cat_id_mask_0 = param->value[0];
                // If cat_id_mask_1 not present, shall be considered as 0
                ind->value.supp_cat.cat_id_mask_1 = (param->length > 1)
                                                    ? param->value[1] : 0;
            } break;

            case (ANPC_READ_OP_CODE):
            {
                ind->att_code      = ((struct anpc_read_cmd *)anpc_env->env[conidx]->operation)->read_code;
                ind->value.ntf_cfg = co_read16(&param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        // Send the indication
        ke_msg_send(ind);
    }
    // else ignore the message

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
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (anpc_env->env[conidx] != NULL)
    {
        // Check the category ID
        if (param->value[0] < CAT_ID_NB)
        {
            // New Alert Characteristic Value
            if (param->handle == anpc_env->env[conidx]->ans.chars[ANPC_CHAR_NEW_ALERT].val_hdl)
            {
                // Text String information Length
                uint8_t length = param->length - 2;

                if (length > ANS_NEW_ALERT_STRING_INFO_MAX_LEN)
                {
                    length = ANS_NEW_ALERT_STRING_INFO_MAX_LEN;
                }

                // Send an indication to the application
                struct anpc_value_ind *new_alert_ind = KE_MSG_ALLOC_DYN(ANPC_VALUE_IND,
                        prf_dst_task_get(&anpc_env->prf_env, conidx),
                        prf_src_task_get(&anpc_env->prf_env, conidx),
                        anpc_value_ind,
                        length);

                new_alert_ind->att_code = ANPC_NTF_NEW_ALERT;

                // Fulfill the characteristic value
                new_alert_ind->value.new_alert.cat_id          = param->value[0];
                new_alert_ind->value.new_alert.nb_new_alert    = param->value[1];
                new_alert_ind->value.new_alert.info_str_len    = length;
                memcpy(&new_alert_ind->value.new_alert.str_info[0], &param->value[2], length);

                // Send the message
                ke_msg_send(new_alert_ind);
            }
            // Unread Alert Status Characteristic Value
            else if (param->handle == anpc_env->env[conidx]->ans.chars[ANPC_CHAR_UNREAD_ALERT_STATUS].val_hdl)
            {
                // Send an indication to the application
                struct anpc_value_ind *unrd_alert_ind = KE_MSG_ALLOC(ANPC_VALUE_IND,
                        prf_dst_task_get(&anpc_env->prf_env, conidx),
                        prf_src_task_get(&anpc_env->prf_env, conidx),
                        anpc_value_ind);

                unrd_alert_ind->att_code = ANPC_NTF_UNREAD_ALERT;

                // Fulfill the characteristic value
                unrd_alert_ind->value.unread_alert.cat_id          = param->value[0];
                unrd_alert_ind->value.unread_alert.nb_unread_alert = param->value[1];

                // Send the message
                ke_msg_send(unrd_alert_ind);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        // else ignore the message
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(anpc)
{
    {ANPC_ENABLE_REQ,               (ke_msg_func_t)anpc_enable_req_handler},
    {ANPC_READ_CMD,                 (ke_msg_func_t)anpc_read_cmd_handler},
    {ANPC_WRITE_CMD,                (ke_msg_func_t)anpc_write_cmd_handler},

    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},

    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void anpc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct anpc_env_tag *anpc_env = PRF_ENV_GET(ANPC, anpc);

    task_desc->msg_handler_tab = anpc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(anpc_msg_handler_tab);
    task_desc->state           = anpc_env->state;
    task_desc->idx_max         = ANPC_IDX_MAX;
}


#endif //(BLE_AN_CLIENT)

/// @} ANPCTASK
