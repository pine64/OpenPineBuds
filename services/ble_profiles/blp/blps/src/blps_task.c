/**
 ****************************************************************************************
 * @addtogroup BLPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BP_SENSOR)
#include "co_utils.h"

#include "gap.h"
#include "gattc_task.h"
#include "blps.h"
#include "blps_task.h"
#include "prf_utils.h"



/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BLPS_ENABLE_REQ message.
 * The handler enables the Blood Pressure Sensor Profile and initialize readable values.
 * @param[in] msgid Id of the message received (probably unused).off
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int blps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct blps_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
     // Get the address of the environment
     struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);

     // Status
     uint8_t status = PRF_ERR_REQ_DISALLOWED;

     if(gapc_get_conhdl(param->conidx) == GAP_INVALID_CONHDL)
     {
         status = PRF_ERR_DISCONNECTED;
     }
     else if(ke_state_get(dest_id) == BLPS_IDLE)
     {
         if (param->interm_cp_ntf_en == PRF_CLI_START_NTF)
         {
             // Enable Bonded Data
             blps_env->prfl_ntf_ind_cfg[param->conidx] |= BLPS_INTM_CUFF_PRESS_NTF_CFG;
         }

         if (param->bp_meas_ind_en == PRF_CLI_START_IND)
         {
             // Enable Bonded Data
             blps_env->prfl_ntf_ind_cfg[param->conidx] |= BLPS_BP_MEAS_IND_CFG;
         }

         status = GAP_ERR_NO_ERROR;
     }

     // send completed information to APP task that contains error status
     struct blps_enable_rsp *cmp_evt = KE_MSG_ALLOC(BLPS_ENABLE_RSP, src_id, dest_id, blps_enable_rsp);
     cmp_evt->status     = status;
     cmp_evt->conidx     = param->conidx;
     ke_msg_send(cmp_evt);

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
    if(ke_state_get(dest_id) == BLPS_IDLE)
    {
        // Get the address of the environment
        struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = param->handle - blps_env->shdl;

        uint16_t value = 0;
        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case BPS_IDX_BP_FEATURE_VAL:
            {
                value = blps_env->features;
            } break;

            case BPS_IDX_BP_MEAS_IND_CFG:
            {
                value = (blps_env->prfl_ntf_ind_cfg[conidx] & BLPS_BP_MEAS_IND_CFG)
                        ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND;
            } break;
            case BPS_IDX_INTM_CUFF_PRESS_NTF_CFG:
            {
                // Characteristic is profile specific
                if (BLPS_IS_SUPPORTED(BLPS_INTM_CUFF_PRESS_SUP))
                {
                    // Fill data
                    value = (blps_env->prfl_ntf_ind_cfg[conidx] & BLPS_INTM_CUFF_PRESS_NTF_CFG)
                            ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
                }
                else
                {
                    status = ATT_ERR_ATTRIBUTE_NOT_FOUND;
                }
            } break;

            default:
            {
                status = ATT_ERR_REQUEST_NOT_SUPPORTED;
            } break;
        }

        // Send data to peer device
        struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint16_t));
        cfm->length = sizeof(uint16_t);
        memcpy(cfm->value, &value, sizeof(uint16_t));
        cfm->handle = param->handle;
        cfm->status = status;

        // Send value to peer device.
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BLPS_MEAS_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int blps_meas_send_req_handler(ke_msg_id_t const msgid,
                                      struct blps_meas_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;

    if(gapc_get_conhdl(param->conidx) == GAP_INVALID_CONHDL)
    {
        status = PRF_ERR_DISCONNECTED;
    }
    else if(ke_state_get(dest_id) == BLPS_IDLE)
    {
        // Get the address of the environment
        struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);

        //Intermediary blood pressure, must be notified if enabled
        if (param->flag_interm_cp)
        {
            // Check if supported
            if (BLPS_IS_SUPPORTED(BLPS_INTM_CUFF_PRESS_SUP))
            {
                if (!BLPS_IS_ENABLED(BLPS_INTM_CUFF_PRESS_NTF_CFG, param->conidx))
                {
                    status = PRF_ERR_NTF_DISABLED;
                }
            }
            else
            {
                status = PRF_ERR_FEATURE_NOT_SUPPORTED;
            }
        }

        //Stable Blood Pressure Measurement, must be indicated if enabled
        else if(! BLPS_IS_ENABLED(BLPS_BP_MEAS_IND_CFG, param->conidx))
        {
            status = PRF_ERR_IND_DISABLED;
        }

        // Check if message can be sent
        if (status == GAP_ERR_NO_ERROR)
        {
            // Allocate the GATT notification message
            struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, param->conidx), dest_id,
                    gattc_send_evt_cmd, BLPS_BP_MEAS_MAX_LEN);

            // Fill event type and handle which trigger event
            if (param->flag_interm_cp)
            {
                meas_val->operation = GATTC_NOTIFY;
                meas_val->handle = blps_env->shdl + BPS_IDX_INTM_CUFF_PRESS_VAL;
            }
            else
            {
                // Fill in the parameter structure
                meas_val->operation = GATTC_INDICATE;
                meas_val->handle = blps_env->shdl + BPS_IDX_BP_MEAS_VAL;
            }

            //Pack the BP Measurement value
            meas_val->length = blps_pack_meas_value(meas_val->value, &param->meas_val);

            // Send the event
            ke_msg_send(meas_val);
            // Go to busy state
            ke_state_set(dest_id, BLPS_BUSY);
        }
    }
    else
    {
        // Save it for later
        msg_status = KE_MSG_SAVED;
    }

    if(status != GAP_ERR_NO_ERROR)
    {
        // Send error message to application
        blps_meas_send_rsp_send(param->conidx, status);
    }

    return (msg_status);
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
    struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint16_t value = 0x0000;
    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t char_code = 0;

    //Extract value before check
    memcpy(&value, &(param->value), sizeof(uint16_t));

    //BP Measurement Char. - Client Char. Configuration
    if (param->handle == (blps_env->shdl + BPS_IDX_BP_MEAS_IND_CFG))
    {
        if ((value == PRF_CLI_STOP_NTFIND) || (value == PRF_CLI_START_IND))
        {
            char_code = BPS_BP_MEAS_CHAR;

            if (value == PRF_CLI_STOP_NTFIND)
            {
                blps_env->prfl_ntf_ind_cfg[conidx] &= ~BLPS_BP_MEAS_IND_CFG;
            }
            else //PRF_CLI_START_IND
            {
                blps_env->prfl_ntf_ind_cfg[conidx] |= BLPS_BP_MEAS_IND_CFG;
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }
    else if (param->handle == (blps_env->shdl + BPS_IDX_INTM_CUFF_PRESS_NTF_CFG))
    {
        if ((value == PRF_CLI_STOP_NTFIND) || (value == PRF_CLI_START_NTF))
        {
            char_code = BPS_INTM_CUFF_MEAS_CHAR;

            if (value == PRF_CLI_STOP_NTFIND)
            {
                blps_env->prfl_ntf_ind_cfg[conidx] &= ~BLPS_INTM_CUFF_PRESS_NTF_CFG;
            }
            else //PRF_CLI_START_NTF
            {
                blps_env->prfl_ntf_ind_cfg[conidx] |= BLPS_INTM_CUFF_PRESS_NTF_CFG;
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }
    }

    if (status == GAP_ERR_NO_ERROR)
    {
        //Inform APP of configuration change
        struct blps_cfg_indntf_ind * ind = KE_MSG_ALLOC(
                BLPS_CFG_INDNTF_IND,
                prf_dst_task_get(&blps_env->prf_env, conidx),
                prf_src_task_get(&blps_env->prf_env, conidx),
                blps_cfg_indntf_ind);

        ind->conidx = conidx;
        ind->char_code = char_code;
        memcpy(&ind->cfg_val, &value, sizeof(uint16_t));

        ke_msg_send(ind);
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
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY and GATT_INDICATE message meaning
 * that Measurement notification/indication has been correctly sent to peer device
 *
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
     uint8_t conidx = KE_IDX_GET(src_id);

    switch(param->operation)
    {
        case GATTC_NOTIFY:
        case GATTC_INDICATE:
        {
            // Send confirmation
            blps_meas_send_rsp_send(conidx, param->status);
            // Go to idle state
            ke_state_set(dest_id, BLPS_IDLE);
        }
        break;

        default: break;
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
KE_MSG_HANDLER_TAB(blps)
{
    {BLPS_ENABLE_REQ,       (ke_msg_func_t) blps_enable_req_handler},
    {GATTC_READ_REQ_IND,    (ke_msg_func_t) gattc_read_req_ind_handler},
    {BLPS_MEAS_SEND_REQ,    (ke_msg_func_t) blps_meas_send_req_handler},
    {GATTC_CMP_EVT,         (ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_WRITE_REQ_IND,   (ke_msg_func_t) gattc_write_req_ind_handler},
};

void blps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct blps_env_tag *blps_env = PRF_ENV_GET(BLPS, blps);

    task_desc->msg_handler_tab = blps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(blps_msg_handler_tab);
    task_desc->state           = blps_env->state;
    task_desc->idx_max         = BLPS_IDX_MAX;
}

#endif /* #if (BLE_BP_SENSOR) */

/// @} BLPSTASK
