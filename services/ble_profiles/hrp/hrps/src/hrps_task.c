/**
 ****************************************************************************************
 * @addtogroup HRPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "hrps.h"
#include "hrps_task.h"

#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPS_ENABLE_REQ message.
 * The handler enables the Heart Rate Sensor Profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hrps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hrps_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
     // Get the address of the environment
     struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);

     // Status
     uint8_t status = PRF_ERR_REQ_DISALLOWED;

     if(ke_state_get(dest_id) == HRPS_IDLE)
     {
         if ((param->conidx < BLE_CONNECTION_MAX) &&
                 (param->hr_meas_ntf <= PRF_CLI_START_NTF))
         {
             // Bonded data was not used before
             if (!HRPS_IS_SUPPORTED(hrps_env->hr_meas_ntf[param->conidx], HRP_PRF_CFG_PERFORMED_OK))
             {
                 // Save notification config
                 hrps_env->hr_meas_ntf[param->conidx] = param->hr_meas_ntf;
                 // Enable Bonded Data
                 hrps_env->hr_meas_ntf[param->conidx] |= HRP_PRF_CFG_PERFORMED_OK;
                 status = GAP_ERR_NO_ERROR;
             }
         }
         else
         {
             status = PRF_ERR_INVALID_PARAM;
         }
     }

     // send completed information to APP task that contains error status
     struct hrps_enable_rsp *cmp_evt = KE_MSG_ALLOC(HRPS_ENABLE_RSP, src_id, dest_id, hrps_enable_rsp);
     cmp_evt->status     = status;
     cmp_evt->conidx     = param->conidx;
     ke_msg_send(cmp_evt);

     return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPS_MEAS_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int hrps_meas_send_req_handler(ke_msg_id_t const msgid,
                                      struct hrps_meas_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // State shall not be Connected or Busy
    if (ke_state_get(dest_id) == HRPS_IDLE)
    {
        if(param->meas_val.nb_rr_interval <= HRS_MAX_RR_INTERVAL)
        {

            // Get the address of the environment
            struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);

            // allocate and prepare data to notify
            hrps_env->operation = (struct hrps_op*) ke_malloc(sizeof(struct hrps_op), KE_MEM_KE_MSG);
            hrps_env->operation->length = hrps_pack_meas_value(hrps_env->operation->data, &param->meas_val);
            hrps_env->operation->cursor = 0;

            // start operation execution
            hrps_exe_operation();

            // Go to busy state
            ke_state_set(dest_id, HRPS_BUSY);
        }
        else
        {
            // Send confirmation
            hrps_meas_send_rsp_send(PRF_ERR_INVALID_PARAM);
        }
    }
    else
    {
        // Save it for later
        msg_status = KE_MSG_SAVED;
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
    struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);
    uint8_t conidx = KE_IDX_GET(src_id);

    uint16_t value = 0x0000;
    uint8_t status = GAP_ERR_NO_ERROR;

    if (hrps_env != NULL)
    {
        //BP Measurement Char. - Client Char. Configuration
        if (param->handle == (hrps_env->shdl + HRS_IDX_HR_MEAS_NTF_CFG))
        {
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if ((value == PRF_CLI_STOP_NTFIND) || (value == PRF_CLI_START_NTF))
            {
                hrps_env->hr_meas_ntf[conidx] = value | HRP_PRF_CFG_PERFORMED_OK;
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == GAP_ERR_NO_ERROR)
            {
                //Inform APP of configuration change
                struct hrps_cfg_indntf_ind * ind = KE_MSG_ALLOC(HRPS_CFG_INDNTF_IND,
                        prf_dst_task_get(&hrps_env->prf_env, conidx),
                        prf_src_task_get(&hrps_env->prf_env, conidx),
                        hrps_cfg_indntf_ind);

                ind->conidx = conidx;
                ind->cfg_val = value;

                ke_msg_send(ind);
            }
        }
        //HR Control Point Char. Value
        else
        {
            if (HRPS_IS_SUPPORTED(hrps_env->features, HRPS_ENGY_EXP_FEAT_SUP))
            {
                //Extract value
                memcpy(&value, &(param->value), sizeof(uint8_t));

                if (value == 0x1)
                {
                    //inform APP of configuration change
                    struct hrps_energy_exp_reset_ind * ind = KE_MSG_ALLOC(HRPS_ENERGY_EXP_RESET_IND,
                            prf_dst_task_get(&hrps_env->prf_env, conidx),
                            prf_src_task_get(&hrps_env->prf_env, conidx),
                            hrps_energy_exp_reset_ind);

                    ind->conidx = conidx;
                    ke_msg_send(ind);
                }
                else
                {
                    status = HRS_ERR_HR_CNTL_POINT_NOT_SUPPORTED;
                }
            }
            else
            {
                //Allowed to send Application Error if value inconvenient
                status = HRS_ERR_HR_CNTL_POINT_NOT_SUPPORTED;
            }
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
    if((ke_state_get(dest_id) == HRPS_BUSY) && (param->operation == GATTC_NOTIFY))
    {
        // continue operation execution
        hrps_exe_operation();
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
    if(ke_state_get(dest_id) == HRPS_IDLE)
    {
        // Get the address of the environment
        struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = param->handle - hrps_env->shdl;

        uint8_t value[4];
        uint8_t value_size = 0;
        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case HRS_IDX_HR_MEAS_NTF_CFG:
            {
                value_size = sizeof(uint16_t);
                co_write16p(value, hrps_env->hr_meas_ntf[conidx] & ~HRP_PRF_CFG_PERFORMED_OK);
            } break;

            case HRS_IDX_BODY_SENSOR_LOC_VAL:
            {
                // Broadcast feature is profile specific
                if (HRPS_IS_SUPPORTED(hrps_env->features, HRPS_BODY_SENSOR_LOC_CHAR_SUP))
                {
                    // Fill data
                    value_size = sizeof(uint8_t);
                    value[0] = hrps_env->sensor_location;
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

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(hrps)
{
    {HRPS_ENABLE_REQ,        (ke_msg_func_t) hrps_enable_req_handler},
    {GATTC_WRITE_REQ_IND,    (ke_msg_func_t) gattc_write_req_ind_handler},
    {HRPS_MEAS_SEND_REQ,     (ke_msg_func_t) hrps_meas_send_req_handler},
    {GATTC_CMP_EVT,          (ke_msg_func_t) gattc_cmp_evt_handler},
    {GATTC_READ_REQ_IND,     (ke_msg_func_t) gattc_read_req_ind_handler},
};

void hrps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct hrps_env_tag *hrps_env = PRF_ENV_GET(HRPS, hrps);

    task_desc->msg_handler_tab = hrps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(hrps_msg_handler_tab);
    task_desc->state           = hrps_env->state;
    task_desc->idx_max         = HRPS_IDX_MAX;
}

#endif /* #if (BLE_HR_SENSOR) */

/// @} HRPSTASK
