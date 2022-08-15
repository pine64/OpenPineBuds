/**
 ****************************************************************************************
 * @addtogroup PASPCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PAS_CLIENT)
#include "pasp_common.h"
#include "paspc_task.h"
#include "paspc.h"

#include "gap.h"
#include "attm.h"
#include "gattc_task.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Phone Alert Status service characteristics information
const struct prf_char_def paspc_pass_char[PASPC_CHAR_MAX] =
{
    /// Alert Status
    [PASPC_CHAR_ALERT_STATUS]      = {ATT_CHAR_ALERT_STATUS,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
    /// Ringer Setting
    [PASPC_CHAR_RINGER_SETTING]    = {ATT_CHAR_RINGER_SETTING,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF},
    /// Ringer Control Point
    [PASPC_CHAR_RINGER_CTNL_PT]    = {ATT_CHAR_RINGER_CNTL_POINT,
                                      ATT_MANDATORY,
                                      ATT_CHAR_PROP_WR_NO_RESP},
};

/// State machine used to retrieve Phone Alert Status service characteristic descriptor information
const struct prf_char_desc_def paspc_pass_char_desc[PASPC_DESC_MAX] =
{
    /// Alert Status Client Characteristic Configuration
    [PASPC_DESC_ALERT_STATUS_CL_CFG]   = {ATT_DESC_CLIENT_CHAR_CFG,
                                          ATT_MANDATORY,
                                          PASPC_CHAR_ALERT_STATUS},
    /// Ringer Setting Client Characteristic Configuration
    [PASPC_DESC_RINGER_SETTING_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                          ATT_MANDATORY,
                                          PASPC_CHAR_RINGER_SETTING},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int paspc_enable_req_handler(ke_msg_id_t const msgid,
                                    struct paspc_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Phone Alert Status Profile Client Role Task Environment
    struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state = ke_state_get(dest_id);

    if ((state == PASPC_IDLE) && (paspc_env->env[conidx] == NULL))
    {
        // allocate environment variable for task instance
        paspc_env->env[conidx] = (struct paspc_cnx_env*) ke_malloc(sizeof(struct paspc_cnx_env),KE_MEM_ATT_DB);
        memset(paspc_env->env[conidx], 0, sizeof(struct paspc_cnx_env));

        paspc_env->env[conidx]->operation     = PASPC_ENABLE_OP_CODE;

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            prf_disc_svc_send(&(paspc_env->prf_env), conidx, ATT_SVC_PHONE_ALERT_STATUS);

            // Go to DISCOVERING state
            ke_state_set(dest_id, PASPC_DISCOVERING);

            // Configure the environment for a discovery procedure
            paspc_env->env[conidx]->last_uuid_req = ATT_SVC_PHONE_ALERT_STATUS;
        }
        // Bond information is provided
        else
        {
            // Keep the provided database content
            memcpy(&paspc_env->env[conidx]->pass, &param->pass, sizeof(struct paspc_pass_content));

            //send APP confirmation that can start normal connection to TH
            paspc_enable_rsp_send(paspc_env, conidx, GAP_ERR_NO_ERROR);

            /* --------------------------------------------------------------------------------
             * After connection establishment, once the discovery procedure is successful,
             * the client shall read the Alert Status
             *  -------------------------------------------------------------------------------- */
            // Configure the environment for a read procedure
            paspc_env->env[conidx]->last_char_code = PASPC_RD_ALERT_STATUS;

            // Send the read request
            prf_read_char_send(&(paspc_env->prf_env),
                    conidx, paspc_env->env[conidx]->pass.svc.shdl,
                    paspc_env->env[conidx]->pass.svc.ehdl,
                    paspc_env->env[conidx]->pass.chars[PASPC_CHAR_ALERT_STATUS].val_hdl);

            // Go to BUSY state
            ke_state_set(dest_id, PASPC_BUSY);
        }
    }
    else if (state != PASPC_FREE)
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // send an error if request fails
    if(status != GAP_ERR_NO_ERROR)
    {
        paspc_enable_rsp_send(paspc_env, conidx, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int paspc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct paspc_read_cmd const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);
    // Get the address of the environment
    struct paspc_env_tag *paspc_idx_env = PRF_ENV_GET(PASPC, paspc);

    if (paspc_idx_env != NULL)
    {
        // Attribute Handle
        uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
        // Status
        uint8_t status     = GAP_ERR_NO_ERROR;

        // Check the provided connection handle
        do
        {
            // Check the current state
            if (ke_state_get(dest_id) == PASPC_BUSY)
            {
                // Keep the request for later, state is GAP_ERR_NO_ERROR
                msg_status = KE_MSG_SAVED;
                break;
            }

            switch (param->read_code)
            {
                // Read Alert Status Characteristic Value
                case (PASPC_RD_ALERT_STATUS):
                {
                    handle = paspc_idx_env->env[conidx]->pass.chars[PASPC_CHAR_ALERT_STATUS].val_hdl;
                } break;

                // Read Ringer Setting Characteristic Value
                case (PASPC_RD_RINGER_SETTING):
                {
                    handle = paspc_idx_env->env[conidx]->pass.chars[PASPC_CHAR_RINGER_SETTING].val_hdl;
                } break;

                // Read Alert Status Characteristic Client Char. Cfg. Descriptor Value
                case (PASPC_RD_WR_ALERT_STATUS_CFG):
                {
                    handle = paspc_idx_env->env[conidx]->pass.descs[PASPC_DESC_ALERT_STATUS_CL_CFG].desc_hdl;
                } break;

                // Read Ringer Setting Characteristic Client Char. Cfg. Descriptor Value
                case (PASPC_RD_WR_RINGER_SETTING_CFG):
                {
                    handle = paspc_idx_env->env[conidx]->pass.descs[PASPC_DESC_RINGER_SETTING_CL_CFG].desc_hdl;
                } break;

                default:
                {
                    // Handle is ATT_INVALID_SEARCH_HANDLE
                    status = PRF_ERR_INVALID_PARAM;
                } break;
            }

                if (status == GAP_ERR_NO_ERROR)
                {
                    // Check if handle is viable
                    if (handle != ATT_INVALID_SEARCH_HANDLE)
                    {
                        // Configure the environment for a read operation
                        paspc_idx_env->env[conidx]->operation      = PASPC_READ_OP_CODE;
                        paspc_idx_env->env[conidx]->last_char_code = param->read_code;

                        // Send the read request
                        prf_read_char_send(&(paspc_idx_env->prf_env), conidx,
                                paspc_idx_env->env[conidx]->pass.svc.shdl,
                                paspc_idx_env->env[conidx]->pass.svc.ehdl,
                                handle);

                        // Go to the Busy state
                        ke_state_set(dest_id, PASPC_BUSY);
                    }
                    else
                    {
                        status = PRF_ERR_INEXISTENT_HDL;
                    }
                }
        } while(0);

        if (status != GAP_ERR_NO_ERROR)
        {
            paspc_send_cmp_evt(paspc_idx_env, conidx, PASPC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists
        paspc_send_cmp_evt(paspc_idx_env, conidx, PASPC_READ_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int paspc_write_cmd_handler(ke_msg_id_t const msgid,
                                   struct paspc_write_cmd const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = GAP_ERR_NO_ERROR;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct paspc_env_tag *paspc_idx_env = PRF_ENV_GET(PASPC, paspc);
    // Get connection index
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (paspc_idx_env != NULL)
    {
        do
        {
            // Check the current state
            if (ke_state_get(dest_id) != PASPC_IDLE)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                break;
            }

            // Attribute handle
            uint16_t handle    = ATT_INVALID_SEARCH_HANDLE;
            // Write type
            uint8_t wr_type;
            // Length
            uint8_t length;

            switch (param->write_code)
            {
                // Write Ringer Control Point Characteristic Value
                case (PASPC_WR_RINGER_CTNL_PT):
                {
                    if ((param->value.ringer_ctnl_pt >= PASP_SILENT_MODE) &&
                        (param->value.ringer_ctnl_pt <= PASP_CANCEL_SILENT_MODE))
                    {
                        handle  = paspc_idx_env->env[conidx]->pass.chars[PASPC_CHAR_RINGER_CTNL_PT].val_hdl;
                        length  = sizeof(uint8_t);
                        wr_type = GATTC_WRITE_NO_RESPONSE;
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write Alert Status Characteristic Client Char. Cfg. Descriptor Value
                case (PASPC_RD_WR_ALERT_STATUS_CFG):
                {
                    if (param->value.alert_status_ntf_cfg <= PRF_CLI_START_NTF)
                    {
                        handle  = paspc_idx_env->env[conidx]->pass.descs[PASPC_DESC_ALERT_STATUS_CL_CFG].desc_hdl;
                        length  = sizeof(uint16_t);
                        wr_type = GATTC_WRITE;
                    }
                    else
                    {
                        status = PRF_ERR_INVALID_PARAM;
                    }
                } break;

                // Write Ringer Setting Characteristic Client Char. Cfg. Descriptor Value
                case (PASPC_RD_WR_RINGER_SETTING_CFG):
                {
                    if (param->value.ringer_setting_ntf_cfg <= PRF_CLI_START_NTF)
                    {
                        handle  = paspc_idx_env->env[conidx]->pass.descs[PASPC_DESC_RINGER_SETTING_CL_CFG].desc_hdl;
                        length  = sizeof(uint16_t);
                        wr_type = GATTC_WRITE;
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
                    prf_gatt_write(&(paspc_idx_env->prf_env), conidx, handle, (uint8_t *)&param->value, length, wr_type);

                    // Configure the environment for a write operation
                    paspc_idx_env->env[conidx]->operation      = PASPC_WRITE_OP_CODE;
                    paspc_idx_env->env[conidx]->last_char_code = param->write_code;

                    // Go to the Busy state
                    ke_state_set(dest_id, PASPC_BUSY);
                }
                else
                {
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
        } while (0);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        paspc_send_cmp_evt(paspc_idx_env, conidx, PASPC_WRITE_OP_CODE, status);
    }

    return (int)msg_status;
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

    if(state == PASPC_DISCOVERING)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);

        struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);

        ASSERT_INFO(paspc_env != NULL, dest_id, src_id);
        ASSERT_INFO(paspc_env->env[conidx] != NULL, dest_id, src_id);

        if(paspc_env->env[conidx]->nb_svc == 0)
        {
            // Retrieve PASS characteristics and descriptors
            prf_extract_svc_info(ind, PASPC_CHAR_MAX, &paspc_pass_char[0],  &paspc_env->env[conidx]->pass.chars[0],
                                      PASPC_DESC_MAX, &paspc_pass_char_desc[0], &paspc_env->env[conidx]->pass.descs[0]);

            //Even if we get multiple responses we only store 1 range
            paspc_env->env[conidx]->pass.svc.shdl = ind->start_hdl;
            paspc_env->env[conidx]->pass.svc.ehdl = ind->end_hdl;
        }

        paspc_env->env[conidx]->nb_svc++;
    }

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
    struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);
    // Status
    uint8_t status;


    if (paspc_env != NULL)
    {
        uint8_t conidx = KE_IDX_GET(dest_id);
        uint8_t state = ke_state_get(dest_id);

        if (state == PASPC_DISCOVERING)
        {
            status = param->status;

            if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (status == ATT_ERR_NO_ERROR))
            {
                // Discovery
                // check characteristic validity
                if(paspc_env->env[conidx]->nb_svc == 1)
                {
                    status = prf_check_svc_char_validity(PASPC_CHAR_MAX,
                            paspc_env->env[conidx]->pass.chars,
                            paspc_pass_char);
                }
                // too much services
                else if (paspc_env->env[conidx]->nb_svc > 1)
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
                    status = prf_check_svc_char_desc_validity(PASPC_DESC_MAX,
                            paspc_env->env[conidx]->pass.descs,
                            paspc_pass_char_desc,
                            paspc_env->env[conidx]->pass.chars);
                }
            }

            paspc_enable_rsp_send(paspc_env, conidx, status);
        }

        else if (state == PASPC_BUSY)
        {
            switch (param->operation)
            {
                case GATTC_READ:
                {
                    // Send the complete event status
                    paspc_send_cmp_evt(paspc_env, conidx, PASPC_READ_OP_CODE, param->status);
                } break;

                case GATTC_WRITE:
                case GATTC_WRITE_NO_RESPONSE:
                {
                    if (paspc_env->env[conidx]->operation == PASPC_WRITE_OP_CODE)
                    {
                        // Send the complete event status
                        paspc_send_cmp_evt(paspc_env, conidx, PASPC_WRITE_OP_CODE, param->status);
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
    struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (paspc_env != NULL)
    {
        ASSERT_ERR(ke_state_get(dest_id) == PASPC_BUSY);

        // Prepare the indication message for the HL
        struct paspc_value_ind *ind = KE_MSG_ALLOC(PASPC_VALUE_IND,
                prf_dst_task_get(&(paspc_env->prf_env), conidx),
                dest_id,
                paspc_value_ind);

        switch (paspc_env->env[conidx]->last_char_code)
        {
            // Read Alert Status Characteristic Value
            case (PASPC_RD_ALERT_STATUS):
            {
                ind->value.alert_status = param->value[0];
            } break;

            // Read Ringer Setting Characteristic Value
            case (PASPC_RD_RINGER_SETTING):
            {
                ind->value.ringer_setting = param->value[0];
            } break;

            // Read Alert Status Characteristic Client Char. Cfg. Descriptor Value
            case (PASPC_RD_WR_ALERT_STATUS_CFG):
            {
                ind->value.alert_status_ntf_cfg = co_read16p(&param->value[0]);
            } break;

            // Read Ringer Setting Characteristic Client Char. Cfg. Descriptor Value
            case (PASPC_RD_WR_RINGER_SETTING_CFG):
            {
                ind->value.ringer_setting_ntf_cfg = co_read16p(&param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = paspc_env->env[conidx]->last_char_code;

        // Send the message
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
    struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);
    uint8_t conidx = KE_IDX_GET(dest_id);

    if (paspc_env != NULL)
    {
        ASSERT_ERR((param->handle == paspc_env->env[conidx]->pass.chars[PASPC_CHAR_ALERT_STATUS].val_hdl) ||
                   (param->handle == paspc_env->env[conidx]->pass.chars[PASPC_CHAR_RINGER_SETTING].val_hdl));

        // Prepare the indication message for the HL
        struct paspc_value_ind *ind = KE_MSG_ALLOC(PASPC_VALUE_IND,
                prf_dst_task_get(&paspc_env->prf_env, conidx),
                prf_src_task_get(&paspc_env->prf_env, conidx),
                paspc_value_ind);

        // Alert Status Characteristic Value
        if (param->handle == paspc_env->env[conidx]->pass.chars[PASPC_CHAR_ALERT_STATUS].val_hdl)
        {
            ind->value.alert_status = param->value[0];
            ind->att_code = PASPC_RD_ALERT_STATUS;
        }
        // Ringer Setting Characteristic Value
        else
        {
            ind->value.ringer_setting = param->value[0];
            ind->att_code = PASPC_RD_RINGER_SETTING;
        }

        // Send the message
        ke_msg_send(ind);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(paspc)
{
    {PASPC_ENABLE_REQ,              (ke_msg_func_t)paspc_enable_req_handler},
    {PASPC_READ_CMD,                (ke_msg_func_t)paspc_read_cmd_handler},
    {PASPC_WRITE_CMD,               (ke_msg_func_t)paspc_write_cmd_handler},
    {GATTC_SDP_SVC_IND,             (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GATTC_READ_IND,                (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,               (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void paspc_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct paspc_env_tag *paspc_env = PRF_ENV_GET(PASPC, paspc);

    task_desc->msg_handler_tab = paspc_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(paspc_msg_handler_tab);
    task_desc->state           = paspc_env->state;
    task_desc->idx_max         = PASPC_IDX_MAX;
}

#endif //(BLE_PAS_CLIENT)

/// @} PASPCTASK
