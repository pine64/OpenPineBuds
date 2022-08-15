/**
 ****************************************************************************************
 * @addtogroup TIPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TIP_SERVER)
#include "co_utils.h"
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "tips.h"
#include "tips_task.h"

#include "prf_utils.h"


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_ENABLE_REQ message.
 * The handler enables the Time Server Profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tips_enable_req_handler(ke_msg_id_t const msgid,
                                   struct tips_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
     // Get the address of the environment
     struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
     uint8_t conidx = KE_IDX_GET(src_id);

     // Status
     uint8_t status = GAP_ERR_NO_ERROR;

     if(gapc_get_conhdl(conidx) == GAP_INVALID_CONHDL)
     {
         status = PRF_ERR_DISCONNECTED;
     }
     else if(ke_state_get(dest_id) == TIPS_IDLE)
     {
         if (param->current_time_ntf_en == PRF_CLI_START_NTF)
         {
             // Enable Bonded Data
             tips_env->env[conidx]->ntf_state |= TIPS_CTS_CURRENT_TIME_CFG;
         }
     }
     else
     {
         status = PRF_ERR_REQ_DISALLOWED;
     }

     // Send to APP the details of the discovered attributes on TIPS
     struct tips_enable_rsp * rsp = KE_MSG_ALLOC(
             TIPS_ENABLE_RSP,
             src_id,
             dest_id,
             tips_enable_rsp);

     rsp->status = status;
     ke_msg_send(rsp);

     return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_UPD_CURR_TIME_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tips_upd_curr_time_req_handler(ke_msg_id_t const msgid,
                                          struct tips_upd_curr_time_req const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
    uint8_t conidx = KE_IDX_GET(src_id);

    if ((tips_env->env[conidx] != NULL) && (ke_state_get(dest_id) == TIPS_IDLE))
    {

        //Check if Notifications are enabled
        if ((tips_env->env[conidx]->ntf_state & TIPS_CTS_CURRENT_TIME_CFG) == TIPS_CTS_CURRENT_TIME_CFG)
        {
            //Check if notification can be sent
            if ((param->current_time.adjust_reason & TIPS_FLAG_EXT_TIME_UPDATE) == TIPS_FLAG_EXT_TIME_UPDATE)
            {
                if(param->enable_ntf_send == 0)
                {
                    status = PRF_ERR_REQ_DISALLOWED;
                }
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                // Allocate the GATT notification message
                struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                        KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                        gattc_send_evt_cmd, CTS_CURRENT_TIME_VAL_LEN);

                // Fill in the parameter structure
                meas_val->operation = GATTC_NOTIFY;
                meas_val->handle = tips_env->cts_shdl + CTS_IDX_CURRENT_TIME_VAL;
                // Pack the Current Time value
                meas_val->length =  tips_pack_curr_time_value(meas_val->value, &param->current_time);

                // Go to idle state
                ke_state_set(dest_id, TIPS_BUSY);
                // Send the event
                ke_msg_send(meas_val);
            }
        }
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    else
    {
        status = GAP_ERR_INVALID_PARAM;
    }

    if (status != GAP_ERR_NO_ERROR)
    {
        // Inform APP
        struct tips_upd_curr_time_rsp *ind = KE_MSG_ALLOC(
                TIPS_UPD_CURR_TIME_RSP,
                prf_dst_task_get(&(tips_env->prf_env), conidx),
                prf_src_task_get(&(tips_env->prf_env), conidx),
                tips_upd_curr_time_rsp);

        ind->status    = status;

        // send the message
        ke_msg_send(ind);
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
    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if((ke_state_get(dest_id) == TIPS_IDLE) && (tips_env->env[conidx] != NULL))
    {
        //CTS : Current Time client configuration
        if (param->handle == tips_env->cts_shdl + CTS_IDX_CURRENT_TIME_CFG)
        {
            uint16_t value = (tips_env->env[conidx]->ntf_state & TIPS_CTS_CURRENT_TIME_CFG)
                                    ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
            status = ATT_ERR_NO_ERROR;

            // Send data to peer device
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(
                    GATTC_READ_CFM,
                    src_id, dest_id,
                    gattc_read_cfm,
                    sizeof(uint16_t));

            cfm->length = sizeof(uint16_t);
            memcpy(cfm->value, &value, sizeof(uint16_t));
            cfm->handle = param->handle;
            cfm->status = status;

            // Send value to peer device.
            ke_msg_send(cfm);
        }
        else
        {
            // Inform APP about read request
            struct tips_rd_req_ind* ind = KE_MSG_ALLOC(
                    TIPS_RD_REQ_IND,
                    prf_dst_task_get(&(tips_env->prf_env), conidx),
                    prf_src_task_get(&(tips_env->prf_env), conidx),
                    tips_rd_req_ind);

            // Current Time Service
            if (param->handle < tips_env->cts_shdl + CTS_IDX_NB)
            {
                // Current Time Char value
                if(param->handle == tips_env->cts_shdl + CTS_IDX_CURRENT_TIME_VAL)
                {
                    ind->char_code = TIPS_CTS_CURRENT_TIME;
                    status = ATT_ERR_NO_ERROR;
                }
                // Local Time Info Char value
                else if ((param->handle == tips_env->cts_shdl + CTS_IDX_LOCAL_TIME_INFO_VAL)
                        && (TIPS_IS_SUPPORTED(TIPS_CTS_LOC_TIME_INFO_SUP)))
                {
                    ind->char_code = TIPS_CTS_LOCAL_TIME_INFO;
                    status = ATT_ERR_NO_ERROR;
                }
                // Reference Time Info Char value
                else if ((param->handle == tips_env->cts_shdl + CTS_IDX_REF_TIME_INFO_VAL)
                        && (TIPS_IS_SUPPORTED(TIPS_CTS_REF_TIME_INFO_SUP)))
                {
                    ind->char_code = TIPS_CTS_REF_TIME_INFO;
                    status = ATT_ERR_NO_ERROR;
                }
            }
            // Time with DST Char value
            else if ((param->handle == tips_env->ndcs_shdl + NDCS_IDX_TIME_DST_VAL)
                    && (TIPS_IS_SUPPORTED(TIPS_NDCS_SUP)))
            {
                ind->char_code = TIPS_NDCS_TIME_DST;
                status = ATT_ERR_NO_ERROR;
            }
            // Time Update State
            else if ((param->handle == tips_env->rtus_shdl + RTUS_IDX_TIME_UPD_STATE_VAL)
                    && (TIPS_IS_SUPPORTED(TIPS_RTUS_SUP)))
            {
                ind->char_code = TIPS_RTUS_TIME_UPD_STATE_VAL;
                status = ATT_ERR_NO_ERROR;
            }
            else
            {
                // Set error code
                status = PRF_ERR_INEXISTENT_HDL;
            }

            if (status == ATT_ERR_NO_ERROR)
            {
                // Save handle
                tips_env->env[conidx]->handle = param->handle;
                // Send message to the APP
                ke_msg_send(ind);
                // Go to busy state
                ke_state_set(dest_id, TIPS_BUSY);
            }
            else
            {
                // Free message
                ke_msg_free(ke_param2msg(ind));
                // Send data to peer device
                struct gattc_read_cfm* cfm = KE_MSG_ALLOC(
                        GATTC_READ_CFM,
                        src_id, dest_id,
                        gattc_read_cfm);

                cfm->handle = param->handle;
                cfm->status = status;

                // Send to peer device.
                ke_msg_send(cfm);
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref TIPS_RD_CFM message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int tips_rd_cfm_handler(ke_msg_id_t const msgid,
                                struct tips_rd_cfm const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
    uint8_t conidx = KE_IDX_GET(src_id);
    uint8_t status = GAP_ERR_NO_ERROR;

    if ((ke_state_get(dest_id) == TIPS_BUSY) && (tips_env->env[conidx] != NULL))
    {
        if (tips_env->env[conidx]->handle != ATT_INVALID_HANDLE)
        {
            // Send data to peer device
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(
                    GATTC_READ_CFM,
                    KE_BUILD_ID(TASK_GATTC, conidx), dest_id,
                    gattc_read_cfm,
                    sizeof(struct tips_rd_cfm));

            switch (param->op_code)
            {
                case TIPS_CTS_CURRENT_TIME:
                {
                    // Pack Current Time
                    cfm->length = tips_pack_curr_time_value(cfm->value, &param->value.curr_time);
                } break;

                case TIPS_CTS_LOCAL_TIME_INFO:
                {
                    // Pack Local Time
                    cfm->length = sizeof(struct tip_loc_time_info);
                    memcpy(cfm->value, &param->value, cfm->length);
                } break;

                case TIPS_CTS_REF_TIME_INFO:
                {
                    // Pack Reference Time
                    cfm->length = sizeof(struct tip_ref_time_info);
                    memcpy(cfm->value, &param->value, cfm->length);
                } break;

                case TIPS_NDCS_TIME_DST:
                {
                    // Pack Time with DST
                    cfm->length = tips_pack_time_dst_value(cfm->value, &param->value.time_with_dst);
                } break;

                case TIPS_RTUS_TIME_UPD_STATE_VAL:
                {
                    // Pack Time Update State
                    cfm->length = sizeof(struct tip_time_upd_state);
                    memcpy(cfm->value, &param->value, cfm->length);
                } break;
                default:
                {
                    // Return Application error to peer
                    cfm->length = 0;
                    status = PRF_APP_ERROR;
                } break;
            }

            // Set parameters
            cfm->handle = tips_env->env[conidx]->handle;
            cfm->status = status;
            // Send value to peer device.
            ke_msg_send(cfm);
            // Go to idle state
            ke_state_set(dest_id, TIPS_IDLE);
            // Reset environment variable
            tips_env->env[conidx]->handle = ATT_INVALID_HANDLE;
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
    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
    uint8_t conidx = KE_IDX_GET(src_id);

    if (tips_env->env[conidx] != NULL)
    {
        if (param->operation == GATTC_NOTIFY)
        {
            // Inform APP
            struct tips_upd_curr_time_rsp *ind = KE_MSG_ALLOC(
                    TIPS_UPD_CURR_TIME_RSP,
                    prf_dst_task_get(&(tips_env->prf_env), conidx),
                    prf_src_task_get(&(tips_env->prf_env), conidx),
                    tips_upd_curr_time_rsp);

            ind->status    = param->status;

            // send the message
            ke_msg_send(ind);
            // Go to idle state
            ke_state_set(dest_id, TIPS_IDLE);
        }
    }

    return (KE_MSG_CONSUMED);
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
    uint16_t cfg_value           = 0x0000;
//    uint8_t time_upd_ctnl_pt     = 0x00;
    uint8_t status               = GAP_ERR_NO_ERROR;

    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);
    uint8_t conidx = KE_IDX_GET(src_id);

    //CTS : Current Time Char. - Notification Configuration
    if (param->handle == tips_env->cts_shdl + CTS_IDX_CURRENT_TIME_CFG)
    {
        //Extract value before check
        memcpy(&cfg_value, &param->value[0], sizeof(uint16_t));

        //only update configuration if value for stop or notification enable
        if ((cfg_value == PRF_CLI_STOP_NTFIND) || (cfg_value == PRF_CLI_START_NTF))
        {
            if (cfg_value == PRF_CLI_STOP_NTFIND)
            {
                tips_env->env[conidx]->ntf_state &= ~TIPS_CTS_CURRENT_TIME_CFG;
            }
            else //PRF_CLI_START_NTF
            {
                tips_env->env[conidx]->ntf_state |= TIPS_CTS_CURRENT_TIME_CFG;
            }

            //Inform APP of configuration change
            struct tips_current_time_ccc_ind * msg = KE_MSG_ALLOC(
                    TIPS_CURRENT_TIME_CCC_IND,
                    prf_dst_task_get(&tips_env->prf_env, conidx),
                    prf_src_task_get(&tips_env->prf_env, conidx),
                    tips_current_time_ccc_ind);

            msg->cfg_val = cfg_value;

            ke_msg_send(msg);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    //RTUS : Time Update Control Point Char. - Val
    else if (param->handle == tips_env->rtus_shdl + RTUS_IDX_TIME_UPD_CTNL_PT_VAL)
    {
        //Check if value to write is in allowed range
        if ((param->value[0] == TIPS_TIME_UPD_CTNL_PT_GET) ||
            (param->value[0] == TIPS_TIME_UPD_CTNL_PT_CANCEL))
        {
            //Update value
            tips_env->time_upd_state = param->value[0];

            //Send APP the indication with the new value
            struct tips_time_upd_ctnl_pt_ind * msg = KE_MSG_ALLOC(
                    TIPS_TIME_UPD_CTNL_PT_IND,
                    prf_dst_task_get(&tips_env->prf_env, conidx),
                    prf_src_task_get(&tips_env->prf_env, conidx),
                    tips_time_upd_ctnl_pt_ind);

            // Time Update Control Point Characteristic Value
            msg->value = tips_env->time_upd_state;
            // Send message
            ke_msg_send(msg);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        status = PRF_ERR_NOT_WRITABLE;
    }

    //Send write response
    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(tips)
{
    {TIPS_ENABLE_REQ,                   (ke_msg_func_t)tips_enable_req_handler},
    {GATTC_WRITE_REQ_IND,               (ke_msg_func_t)gattc_write_req_ind_handler},
    {TIPS_UPD_CURR_TIME_REQ,            (ke_msg_func_t)tips_upd_curr_time_req_handler},
    {GATTC_READ_REQ_IND,                (ke_msg_func_t)gattc_read_req_ind_handler},
    {TIPS_RD_CFM,                       (ke_msg_func_t)tips_rd_cfm_handler},
    {GATTC_CMP_EVT,                     (ke_msg_func_t)gattc_cmp_evt_handler},
};

void tips_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct tips_env_tag *tips_env = PRF_ENV_GET(TIPS, tips);

    task_desc->msg_handler_tab = tips_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(tips_msg_handler_tab);
    task_desc->state           = tips_env->state;
    task_desc->idx_max         = TIPS_IDX_MAX;
}

#endif //BLE_TIP_SERVER

/// @} TIPSTASK
