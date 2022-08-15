/**
 ****************************************************************************************
 * @addtogroup ANPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_AN_SERVER)

#include "gapm.h"
#include "gattc_task.h"
#include "anps.h"
#include "anps_task.h"

#include "prf_utils.h"
#include "prf_types.h"

#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int anps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct anps_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Alert Notification Profile Server Role Task Index Environment
    struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);
    uint8_t conidx = KE_IDX_GET(src_id);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(ke_state_get(dest_id) == ANPS_IDLE)
    {
        if (anps_env->env[conidx] != NULL)
        {
            // Bonded data was not used before
            if (!ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANPS_FLAG_CFG_PERFORMED_OK))
            {
                status = GAP_ERR_NO_ERROR;
                // Update the state in the environment
                if (param->new_alert_ntf_cfg != PRF_CLI_STOP_NTFIND)
                {
                    // Force to PRF_CLI_START_NTF
                    param->new_alert_ntf_cfg = PRF_CLI_START_NTF;

                    ANPS_ENABLE_ALERT(conidx, anps_env, ANP_NEW_ALERT);
                }

                if (param->unread_alert_status_ntf_cfg != PRF_CLI_STOP_NTFIND)
                {
                    // Force to PRF_CLI_START_NTF
                    param->unread_alert_status_ntf_cfg = PRF_CLI_START_NTF;

                    ANPS_ENABLE_ALERT(conidx, anps_env, ANP_UNREAD_ALERT);
                }
            }
            // Enable Bonded Data
            ANPS_ENABLE_ALERT(conidx, anps_env, ANPS_FLAG_CFG_PERFORMED_OK);
        }
     }

     // send completed information to APP task that contains error status
     struct anps_enable_rsp *cmp_evt = KE_MSG_ALLOC(ANPS_ENABLE_RSP, src_id, dest_id, anps_enable_rsp);
     cmp_evt->status     = status;
     ke_msg_send(cmp_evt);

     return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANPS_NTF_ALERT_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int anps_ntf_alert_cmd_handler(ke_msg_id_t const msgid,
                                      struct anps_ntf_alert_cmd const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_INVALID_PARAM;
    // Message Status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Task environment
    struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);
    uint8_t conidx = KE_IDX_GET(dest_id);
    uint8_t state = ke_state_get(dest_id);

    if(state == ANPS_IDLE)
    {
        // Category ID
        uint8_t cat_id;
        // Is the category supported ?
        bool cat_supported;
        // Handle
        uint16_t handle = ATT_ERR_INVALID_HANDLE;

        ASSERT_ERR(anps_env != NULL);
        ASSERT_ERR(anps_env->env[conidx] != NULL);

        do
        {
            // Check the operation code, get the category ID.
            if (param->operation == ANPS_UPD_NEW_ALERT_OP_CODE)
            {
                // Check the length of the string info value
                if (param->value.new_alert.info_str_len > ANS_NEW_ALERT_STRING_INFO_MAX_LEN)
                {
                    status = PRF_ERR_INVALID_PARAM;
                    break;
                }

                // Get the category ID
                cat_id = param->value.new_alert.cat_id;
                // Check if the category is supported
                cat_supported = ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(cat_id);
            }
            else if (param->operation == ANPS_UPD_UNREAD_ALERT_STATUS_OP_CODE)
            {
                // Get the category ID
                cat_id = param->value.unread_alert_status.cat_id;
                // Check if the category is supported
                cat_supported = ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(cat_id);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check the category ID
            if ((cat_id >= CAT_ID_NB) || (!cat_supported))
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (param->operation == ANPS_UPD_NEW_ALERT_OP_CODE)
            {
                // Check if sending of notification is enabled for the provided category
                if (ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(conidx, cat_id, anps_env) &&
                    ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_NEW_ALERT))
                {
                    handle = anps_env->shdl + ANS_IDX_NEW_ALERT_VAL;

                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                            gattc_send_evt_cmd, 2 + param->value.new_alert.info_str_len);

                    // Fill in the parameter structure
                    ntf->operation = GATTC_NOTIFY;
                    ntf->handle = handle;
                    // pack measured value in database
                    ntf->length = 2 + param->value.new_alert.info_str_len;
                    // Fill data
                    memcpy(&ntf->value, &param->value.new_alert.cat_id, ntf->length);

                    // The notification can be sent, send the notification
                    status = GAP_ERR_NO_ERROR;
                    ke_msg_send(ntf);
                }
                else
                {
                    status = PRF_ERR_NTF_DISABLED;
                    break;
                }
            }
            else
            {
                if (ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(conidx, cat_id, anps_env) &&
                    ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_UNREAD_ALERT))
                {
                    handle = anps_env->shdl + ANS_IDX_UNREAD_ALERT_STATUS_VAL;

                    // Allocate the GATT notification message
                    struct gattc_send_evt_cmd *ntf = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                            KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                            gattc_send_evt_cmd, sizeof(struct anp_unread_alert));

                    // Fill in the parameter structure
                    ntf->operation = GATTC_NOTIFY;
                    ntf->handle = handle;
                    // pack measured value in database
                    ntf->length = sizeof(struct anp_unread_alert);
                    // Fill data
                    memcpy(&ntf->value, &param->value.unread_alert_status, ntf->length);

                    // The notification can be sent, send the notification
                    status = GAP_ERR_NO_ERROR;
                    ke_msg_send(ntf);
                }
                else
                {
                    status = PRF_ERR_NTF_DISABLED;
                    break;
                }
            }

            // Configure the environment
            anps_env->operation = param->operation;
            // Go to Busy state
            ke_state_set(dest_id, ANPS_BUSY);
        } while(0);
    }
    else if (state == ANPS_BUSY)
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
        anps_send_cmp_evt(dest_id, src_id, param->operation, status);
    }

    return (int)msg_status;
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
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);
    uint8_t conidx = KE_IDX_GET(src_id);

    if (anps_env != NULL)
    {
        // Send a complete event status to the application
        anps_send_cmp_evt(prf_src_task_get(&anps_env->prf_env, conidx),
                prf_dst_task_get(&anps_env->prf_env, conidx),
                anps_env->operation, param->status);
    }
    // else ignore the message

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
    if(ke_state_get(dest_id) == ANPS_IDLE)
    {
        // Get the address of the environment
        struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = param->handle - anps_env->shdl;

        uint8_t value[2];
        uint8_t value_size = 0;
        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case ANS_IDX_SUPP_NEW_ALERT_CAT_VAL:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, anps_env->supp_new_alert_cat);
            } break;

            case ANS_IDX_NEW_ALERT_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_NEW_ALERT));
            } break;

            case ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, anps_env->supp_unread_alert_cat);
            } break;

            case ANS_IDX_UNREAD_ALERT_STATUS_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_UNREAD_ALERT));
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
    struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);

    // Check if the connection exists
    if (anps_env != NULL)
    {
        /*
         * ---------------------------------------------------------------------------------------------
         * New Alert Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         * Unread Status Alert Client Characteristic Configuration Descriptor Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        if ((param->handle == (anps_env->shdl + ANS_IDX_NEW_ALERT_CFG)) ||
            (param->handle == (anps_env->shdl + ANS_IDX_UNREAD_ALERT_STATUS_CFG)))
        {
            // Received configuration value
            uint16_t ntf_cfg = co_read16p(&param->value[0]);

            if (ntf_cfg <= PRF_CLI_START_NTF)
            {
                // Alert type
                uint8_t alert_type = (param->handle == (anps_env->shdl + ANS_IDX_NEW_ALERT_CFG))
                                     ? ANP_NEW_ALERT : ANP_UNREAD_ALERT;

                // Update the status in the environment
                if (ntf_cfg == PRF_CLI_START_NTF)
                {
                    ANPS_ENABLE_ALERT(conidx, anps_env, alert_type);
                }
                else
                {
                    ANPS_DISABLE_ALERT(conidx, anps_env, alert_type);
                }

                // Inform the HL that the notification configuration status has been written
                anps_send_ntf_status_update_ind(conidx, anps_env, alert_type);
                // Enable bond Data
                ANPS_ENABLE_ALERT(conidx, anps_env, ANPS_FLAG_CFG_PERFORMED_OK);
            }
            else
            {
                status = PRF_APP_ERROR;
            }
        }
        /*
         * ---------------------------------------------------------------------------------------------
         * Alert Notification Control Point Characteristic Value - Write
         * ---------------------------------------------------------------------------------------------
         */
        else if (param->handle == (anps_env->shdl + ANS_IDX_ALERT_NTF_CTNL_PT_VAL))
        {
            do
            {
                // Check the command ID value
                if (param->value[0] >= CMD_ID_NB)
                {
                    status = ANP_CMD_NOT_SUPPORTED;
                    break;
                }

                // Check the category ID value
                if ((param->value[1] >= CAT_ID_NB) &&
                    (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT))
                {
                    status = ANP_CAT_NOT_SUPPORTED;
                    break;
                }

                if (param->value[1] < CAT_ID_NB)
                {
                    // New Alert
                    if ((param->value[0] % 2) == 0)
                    {
                        // Check if the category is supported
                        if (!ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(param->value[1]))
                        {
                            status = ANP_CAT_NOT_SUPPORTED;
                            break;
                        }
                    }
                    // Unread Alert Status
                    else
                    {
                        // Check if the category is supported
                        if (!ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(param->value[1]))
                        {
                            status = ANP_CAT_NOT_SUPPORTED;
                            break;
                        }
                    }
                }

                // React according to the received command id value
                switch (param->value[0])
                {
                    // Enable New Alert Notification
                    case (CMD_ID_EN_NEW_IN_ALERT_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Enable sending of new alert notification for the specified category
                            ANPS_ENABLE_NEW_ALERT_CATEGORY(conidx, param->value[1], anps_env);
                        }
                        else
                        {
                            // Enable sending of new alert notification for all supported category
                            anps_env->env[conidx]->ntf_new_alert_cfg |= anps_env->supp_new_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(conidx, anps_env, ANP_NEW_ALERT);
                    } break;

                    // Enable Unread Alert Status Notification
                    case (CMD_ID_EN_UNREAD_CAT_STATUS_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Enable sending of unread alert notification for the specified category
                            ANPS_ENABLE_UNREAD_ALERT_CATEGORY(conidx, param->value[1], anps_env);
                        }
                        else
                        {
                            // Enable sending of unread alert notification for all supported category
                            anps_env->env[conidx]->ntf_unread_alert_cfg |= anps_env->supp_unread_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(conidx, anps_env, ANP_UNREAD_ALERT);
                    } break;

                    // Disable New Alert Notification
                    case (CMD_ID_DIS_NEW_IN_ALERT_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Disable sending of new alert notification for the specified category
                            ANPS_DISABLE_NEW_ALERT_CATEGORY(conidx, param->value[1], anps_env);
                        }
                        else
                        {
                            // Disable sending of new alert notification for all supported category
                            anps_env->env[conidx]->ntf_new_alert_cfg &= ~anps_env->supp_new_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(conidx, anps_env, ANP_NEW_ALERT);
                    } break;

                    // Disable Unread Alert Status Notification
                    case (CMD_ID_DIS_UNREAD_CAT_STATUS_NTF):
                    {
                        if (param->value[1] != CAT_ID_ALL_SUPPORTED_CAT)
                        {
                            // Disable sending of unread alert notification for the specified category
                            ANPS_DISABLE_UNREAD_ALERT_CATEGORY(conidx, param->value[1], anps_env);
                        }
                        else
                        {
                            // Enable sending of unread alert notification for all supported category
                            anps_env->env[conidx]->ntf_unread_alert_cfg &= ~anps_env->supp_unread_alert_cat;
                        }

                        anps_send_ntf_status_update_ind(conidx, anps_env, ANP_UNREAD_ALERT);
                    } break;

                    // Notify New Alert immediately
                    case (CMD_ID_NTF_NEW_IN_ALERT_IMM):
                    {
                        // Check if sending of notification is enabled
                        if (ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_NEW_ALERT))
                        {
                            if (param->value[1] == CAT_ID_ALL_SUPPORTED_CAT)
                            {
                                // Check if at least one category can be notified
                                if (anps_env->env[conidx]->ntf_new_alert_cfg != 0)
                                {
                                    anps_send_ntf_immediate_req_ind(conidx, anps_env, ANP_NEW_ALERT,
                                                                    CAT_ID_ALL_SUPPORTED_CAT);
                                }
                            }
                            else
                            {
                                // Check if sending of notifications has been enabled for the specified category.
                                if (ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(conidx, param->value[1], anps_env))
                                {
                                    anps_send_ntf_immediate_req_ind(conidx, anps_env, ANP_NEW_ALERT,
                                                                    param->value[1]);
                                }
                            }
                        }
                    } break;

                    // Notify Unread Alert Status immediately
                    case (CMD_ID_NTF_UNREAD_CAT_STATUS_IMM):
                    {
                        if (ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_UNREAD_ALERT))
                        {
                            // Check if sending of notification is enabled
                            if (ANPS_IS_ALERT_ENABLED(conidx, anps_env, ANP_UNREAD_ALERT))
                            {
                                if (param->value[1] == CAT_ID_ALL_SUPPORTED_CAT)
                                {
                                    // Check if at least one category can be notified
                                    if (anps_env->env[conidx]->ntf_unread_alert_cfg != 0)
                                    {
                                        anps_send_ntf_immediate_req_ind(conidx, anps_env, ANP_UNREAD_ALERT,
                                                                        CAT_ID_ALL_SUPPORTED_CAT);
                                    }
                                }
                                else
                                {
                                    // Check if sending of notifications has been enabled for the specified category.
                                    if (ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(conidx, param->value[1], anps_env))
                                    {
                                        anps_send_ntf_immediate_req_ind(conidx, anps_env, ANP_UNREAD_ALERT,
                                                                        param->value[1]);
                                    }
                                }
                            }
                        }
                    } break;

                    default:
                    {
                        ASSERT_ERR(0);
                    } break;
                }
            } while (0);
        }
        else
        {
            ASSERT_ERR(0);
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

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(anps)
{
    {ANPS_ENABLE_REQ,               (ke_msg_func_t)anps_enable_req_handler},
    {ANPS_NTF_ALERT_CMD,            (ke_msg_func_t)anps_ntf_alert_cmd_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t)gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t)gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
};

void anps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct anps_env_tag *anps_env = PRF_ENV_GET(ANPS, anps);

    task_desc->msg_handler_tab = anps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(anps_msg_handler_tab);
    task_desc->state           = anps_env->state;
    task_desc->idx_max         = ANPS_IDX_MAX;
}

#endif //(BLE_AN_SERVER)

/// @} ANPSTASK
