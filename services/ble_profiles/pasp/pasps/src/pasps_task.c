/**
 ****************************************************************************************
 * @addtogroup PASPSTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_PAS_SERVER)
#include "pasp_common.h"

#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "pasps.h"
#include "pasps_task.h"
#include "prf_utils.h"

#include "co_utils.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int pasps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct pasps_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);
    uint8_t conidx = KE_IDX_GET(src_id);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(ke_state_get(dest_id) == PASPS_IDLE)
    {
        // Bonded data was not used before
        if (!(pasps_env->env[conidx]->ntf_state & PASPS_FLAG_CFG_PERFORMED_OK))
        {
            status = GAP_ERR_NO_ERROR;
            if (param->alert_status_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->alert_status_ntf_cfg = PRF_CLI_START_NTF;

                PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_ALERT_STATUS_CFG);
            }

            if (param->ringer_setting_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                // Force to PRF_CLI_START_NTF
                param->ringer_setting_ntf_cfg = PRF_CLI_START_NTF;

                PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_RINGER_SETTING_CFG);
            }
        }
        // Enable Bonded Data
        PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_CFG_PERFORMED_OK);
    }

    // send completed information to APP task that contains error status
    struct pasps_enable_rsp *cmp_evt = KE_MSG_ALLOC(PASPS_ENABLE_RSP, src_id, dest_id, pasps_enable_rsp);
    cmp_evt->status     = status;

    ke_msg_send(cmp_evt);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref PASPS_UPDATE_CHAR_VAL_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int pasps_update_char_val_cmd_handler(ke_msg_id_t const msgid,
                                             struct pasps_update_char_val_cmd *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state = ke_state_get(dest_id);

    if (state == PASPS_IDLE)
    {
        // Get the address of the environment
        struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);
        // Handle
        uint16_t handle = ATT_ERR_INVALID_HANDLE;
        // Notification status flag
        uint8_t flag = 0;

        ASSERT_ERR(pasps_env != NULL);
        ASSERT_ERR(pasps_env->env[conidx] != NULL);

        // Check the connection handle
        switch (param->operation)
        {
            // Alert Status Characteristic
            case (PASPS_UPD_ALERT_STATUS_OP_CODE):
            {
                // Check the provided value
                if (param->value <= PASP_ALERT_STATUS_VAL_MAX)
                {
                    // Set the handle value
                    handle = pasps_env->shdl + PASS_IDX_ALERT_STATUS_VAL;
                    // Set the flag
                    flag   = PASPS_FLAG_ALERT_STATUS_CFG;
                    // Update the ringer state value
                    pasps_env->alert_status = param->value;

                    status = GAP_ERR_NO_ERROR;
                }
                // else status is PRF_ERR_INVALID_PARAM
            } break;

            // Ringer Setting Characteristic
            case (PASPS_UPD_RINGER_SETTING_OP_CODE):
            {
                // Check the provided value
                if (param->value <= PASP_RINGER_NORMAL)
                {
                    // Set the handle value
                    handle = pasps_env->shdl + PASS_IDX_RINGER_SETTING_VAL;
                    // Set the flag
                    flag   = PASPS_FLAG_RINGER_SETTING_CFG;
                    // Update the ringer state value
                    pasps_env->ringer_setting = param->value;

                    status = GAP_ERR_NO_ERROR;
                }
                // else status is PRF_ERR_INVALID_PARAM
            } break;

            default:
            {
                // Nothing more to do, status is PRF_ERR_INVALID_PARAM
            } break;
        }

        if (status == GAP_ERR_NO_ERROR)
        {
            // Check if sending of notifications is enabled for this connection
            if (PASPS_IS_NTF_ENABLED(conidx, pasps_env, flag))
            {
                // Configure the environment for the operation
                pasps_env->operation = param->operation;
                // Go to the Busy state
                ke_state_set(dest_id, PASPS_BUSY);

                // Allocate the GATT notification message
                struct gattc_send_evt_cmd *ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                        KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                        gattc_send_evt_cmd, sizeof(uint8_t));

                // Fill in the parameter structure
                ntf->operation = GATTC_NOTIFY;
                ntf->handle = handle;
                // pack measured value in database
                ntf->length = sizeof(uint8_t);
                // Fill data
                ntf->value[0] = param->value;

                // The notification can be sent, send the notification
                ke_msg_send(ntf);
            }
            else
            {
                status = PRF_ERR_NTF_DISABLED;
            }
        }
    }
    else if (state == PASPS_BUSY)
    {
        // Save it for later
        msg_status = KE_MSG_SAVED;
        status = GAP_ERR_NO_ERROR;
    }
    // no connection
    else
    {
        status = GAP_ERR_DISCONNECTED;
    }

    if(status != GAP_ERR_NO_ERROR)
    {
        // Send the message to the application
        struct pasps_cmp_evt *evt = KE_MSG_ALLOC(PASPS_CMP_EVT, src_id, dest_id, pasps_cmp_evt);

        evt->operation   = param->operation;
        evt->status      = status;

        ke_msg_send(evt);
    }

    return (int)msg_status;
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
__STATIC int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
        if(ke_state_get(dest_id) == PASPS_IDLE)
        {
            // Get the address of the environment
            struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);
            uint8_t att_idx = param->handle - pasps_env->shdl;
            struct gattc_att_info_cfm * cfm;

            //Send write response
            cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
            cfm->handle = param->handle;

            // check if it's a client configuration char
            if((att_idx == PASS_IDX_ALERT_STATUS_CFG)
                    || (att_idx == PASS_IDX_RINGER_SETTING_CFG))
            {
                // CCC attribute length = 2
                cfm->length = 2;
                cfm->status = GAP_ERR_NO_ERROR;
            }
            else if (att_idx == PASS_IDX_RINGER_CTNL_PT_VAL)
            {
                // attribute length = 1
                cfm->length = 1;
                cfm->status = GAP_ERR_NO_ERROR;
            }
            // not expected request
            else
            {
                cfm->length = 0;
                cfm->status = ATT_ERR_WRITE_NOT_PERMITTED;
            }
            ke_msg_send(cfm);
        }

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
    if(ke_state_get(dest_id) == PASPS_IDLE)
    {
        // Get the address of the environment
        struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = param->handle - pasps_env->shdl;

        uint8_t value[2];
        uint8_t value_size = 0;
        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case PASS_IDX_ALERT_STATUS_VAL:
            {
                // Fill data
                value_size = sizeof(uint8_t);
                value[0] = pasps_env->alert_status;
            } break;

            case PASS_IDX_RINGER_SETTING_VAL:
            {
                // Fill data
                value_size = sizeof(uint8_t);
                value[0] = pasps_env->ringer_setting;
            } break;

            case PASS_IDX_ALERT_STATUS_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, (pasps_env->env[conidx]->ntf_state & PASPS_FLAG_ALERT_STATUS_CFG)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            } break;

            case PASS_IDX_RINGER_SETTING_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, (pasps_env->env[conidx]->ntf_state & PASPS_FLAG_RINGER_SETTING_CFG)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            } break;

            default:
            {
                status = ATT_ERR_REQUEST_NOT_SUPPORTED;
            } break;
        }

        // Send data to peer device
        struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, value_size);
        cfm->length = value_size;
        memcpy(cfm->value, value, value_size);
        cfm->handle = param->handle;
        cfm->status = status;

        // Send value to peer device.
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_REQ_IND message.
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
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Get the conidx
    uint8_t conidx = KE_IDX_GET(src_id);
    // Get the address of the environment
    struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);

    // Check if the connection exists
    if (pasps_env->env[conidx] != NULL)
    {
        /*
         * ---------------------------------------------------------------------------------------------
         * Alert Status Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         * Ringer Setting Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        if ((param->handle == (pasps_env->shdl + PASS_IDX_ALERT_STATUS_CFG)) ||
            (param->handle == (pasps_env->shdl + PASS_IDX_RINGER_SETTING_CFG)))
        {
            // Received configuration value
            uint16_t ntf_cfg = co_read16p(&param->value[0]);

            if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
            {
                struct pasps_written_char_val_ind *ind = KE_MSG_ALLOC(PASPS_WRITTEN_CHAR_VAL_IND,
                        prf_dst_task_get(&pasps_env->prf_env, conidx),
                        prf_src_task_get(&pasps_env->prf_env, conidx),
                        pasps_written_char_val_ind);

                if (param->handle == pasps_env->shdl + PASS_IDX_ALERT_STATUS_CFG)
                {
                    ind->value.alert_status_ntf_cfg = ntf_cfg;
                    ind->att_code = PASPS_ALERT_STATUS_NTF_CFG;

                    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                    {
                        PASPS_DISABLE_NTF(conidx, pasps_env, PASPS_FLAG_ALERT_STATUS_CFG);
                    }
                    else
                    {
                        PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_ALERT_STATUS_CFG);
                    }
                }
                else
                {
                    ind->value.ringer_setting_ntf_cfg = ntf_cfg;
                    ind->att_code = PASPS_RINGER_SETTING_NTF_CFG;

                    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                    {
                        PASPS_DISABLE_NTF(conidx, pasps_env, PASPS_FLAG_RINGER_SETTING_CFG);
                    }
                    else
                    {
                        PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_RINGER_SETTING_CFG);
                    }
                }

                ke_msg_send(ind);

                // Enable Bonded Data
                PASPS_ENABLE_NTF(conidx, pasps_env, PASPS_FLAG_CFG_PERFORMED_OK);
            }
            else
            {
                status = PRF_APP_ERROR;
            }
        }
        /*
         * ---------------------------------------------------------------------------------------------
         * Ringer Control Point Characteristic Value - Write Without Response
         * ---------------------------------------------------------------------------------------------
         */
        else if (param->handle == (pasps_env->shdl + PASS_IDX_RINGER_CTNL_PT_VAL))
        {
            // Inform the HL ?
            bool inform_hl = false;

            // Check the received value
            switch (param->value[0])
            {
                case (PASP_SILENT_MODE):
                {
                    // Ignore if ringer is already silent
                    if (pasps_env->env[conidx]->ringer_state == PASP_RINGER_NORMAL)
                    {
                        inform_hl = true;
                    }
                } break;

                case (PASP_CANCEL_SILENT_MODE):
                {
                    // Ignore if ringer is not silent
                    if (pasps_env->env[conidx]->ringer_state == PASP_RINGER_SILENT)
                    {
                        inform_hl = true;
                    }
                } break;

                case (PASP_MUTE_ONCE):
                {
                    inform_hl = true;
                } break;

                // No need to respond with an error (Write Without Response)
                default: break;
            }

            if (inform_hl)
            {
                struct pasps_written_char_val_ind *ind = KE_MSG_ALLOC(PASPS_WRITTEN_CHAR_VAL_IND,
                        prf_dst_task_get(&pasps_env->prf_env, conidx),
                        prf_src_task_get(&pasps_env->prf_env, conidx),
                        pasps_written_char_val_ind);

                ind->att_code = PASPS_RINGER_CTNL_PT_CHAR_VAL;
                ind->value.ringer_ctnl_pt = param->value[0];

                ke_msg_send(ind);
            }
        }
        // Send the write response to the peer device
        struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
        cfm->handle = param->handle;
        cfm->status = status;
        ke_msg_send(cfm);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT message meaning that a notification or an indication
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
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
    struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);
    uint8_t conidx = KE_IDX_GET(src_id);

    if (pasps_env != NULL)
    {
        // Send a complete event status to the application
        pasps_send_cmp_evt(prf_src_task_get(&pasps_env->prf_env, conidx),
                prf_dst_task_get(&pasps_env->prf_env, conidx),
                pasps_env->operation, param->status);
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(pasps)
{
    {PASPS_ENABLE_REQ,              (ke_msg_func_t) pasps_enable_req_handler},
    {PASPS_UPDATE_CHAR_VAL_CMD,     (ke_msg_func_t) pasps_update_char_val_cmd_handler},
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

void pasps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct pasps_env_tag *pasps_env = PRF_ENV_GET(PASPS, pasps);

    task_desc->msg_handler_tab = pasps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(pasps_msg_handler_tab);
    task_desc->state           = pasps_env->state;
    task_desc->idx_max         = PASPS_IDX_MAX;
}

#endif //(BLE_PASS_SERVER)

/// @} PASPSTASK
