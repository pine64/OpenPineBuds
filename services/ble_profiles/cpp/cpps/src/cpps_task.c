/**
 ****************************************************************************************
 * @addtogroup CPPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include "rwip_config.h"
#if (BLE_CP_SENSOR)
#include "cpp_common.h"

#include "gapc.h"
#include "gattc.h"
#include "gattc_task.h"
#include "cpps.h"
#include "cpps_task.h"
#include "prf_utils.h"
#include "co_math.h"

#include "ke_mem.h"
#include "co_utils.h"


/*
 *  CYCLING POWER SERVICE ATTRIBUTES
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int cpps_enable_req_handler(ke_msg_id_t const msgid,
                                    struct cpps_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(ke_state_get(dest_id) == CPPS_IDLE)
    {
        // Bonded data was not used before
        if (!CPPS_IS_PRESENT(cpps_env->env[param->conidx].prfl_ntf_ind_cfg, CPP_PRF_CFG_PERFORMED_OK))
        {
            status = GAP_ERR_NO_ERROR;
            // Save configuration and set flag
            cpps_env->env[param->conidx].prfl_ntf_ind_cfg = param->prfl_ntf_ind_cfg;
            // Enable Bonded Data
            CPPS_ENABLE_NTF_IND_BCST(param->conidx, CPP_PRF_CFG_PERFORMED_OK);
        }
    }

    // send completed information to APP task that contains error status
    struct cpps_enable_rsp *cmp_evt = KE_MSG_ALLOC(CPPS_ENABLE_RSP, src_id, dest_id, cpps_enable_rsp);
    cmp_evt->status     = status;
    cmp_evt->conidx     = param->conidx;
    ke_msg_send(cmp_evt);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_GET_ADV_DATA_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_get_adv_data_req_handler(ke_msg_id_t const msgid,
        struct cpps_get_adv_data_req *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // State shall not be Connected or Busy
    if (ke_state_get(dest_id) == CPPS_IDLE)
    {
        // Get the address of the environment
        struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

        // Allocate the message
        struct cpps_get_adv_data_rsp *rsp = KE_MSG_ALLOC_DYN(CPPS_GET_ADV_DATA_RSP,
                src_id, dest_id,
                cpps_get_adv_data_rsp, CPP_CP_MEAS_ADV_MAX_LEN);
        // Set status
        rsp->status = GAP_ERR_COMMAND_DISALLOWED;
        // Set length
        rsp->data_len = 0;

        // Check Broadcast is supported
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK))
        {
            // Check if broadcast has been enabled
            if (cpps_env->broadcast_enabled)
            {
                // Pack Cp Measurement
                rsp->adv_data[0] = cpps_pack_meas_ntf(&param->parameters, &rsp->adv_data[4]) + CPP_CP_ADV_HEADER_LEN;
                // Pack Service Data AD type
                rsp->adv_data[1] = GAP_AD_TYPE_SERVICE_16_BIT_DATA;
                // Pack UUID of CPS
                co_write16p(&rsp->adv_data[2], ATT_SVC_CYCLING_POWER);

                // Set data length
                rsp->data_len = rsp->adv_data[0] + CPP_CP_ADV_LENGTH_LEN;
                // Set status
                rsp->status = GAP_ERR_NO_ERROR;
            }
        }

        // Send message
        ke_msg_send(rsp);
    }
    else
    {
        msg_status = KE_MSG_SAVED;
    }

    return (msg_status);
}
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_NTF_CP_MEAS_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ntf_cp_meas_req_handler(ke_msg_id_t const msgid,
                                          struct cpps_ntf_cp_meas_req *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_NO_FREE;

    // State shall not be Connected or Busy
    if (ke_state_get(dest_id) == CPPS_IDLE)
    {
        // Get the address of the environment
        struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

        // Configure the environment
        cpps_env->operation = CPPS_NTF_MEAS_OP_CODE;
        // allocate operation data
        cpps_env->op_data = (struct cpps_op*) ke_malloc(sizeof(struct cpps_op), KE_MEM_KE_MSG);
        cpps_env->op_data->cmd         = ke_param2msg(param);
        cpps_env->op_data->cursor      = 0;
        cpps_env->op_data->ntf_pending = NULL;

        // Go to busy state
        ke_state_set(dest_id, CPPS_BUSY);

        // Should be updated just once
        if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_WHEEL_REV_DATA_PRESENT))
        {
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->features, CPP_FEAT_WHEEL_REV_DATA_SUPP))
            {
                // Update the cumulative wheel revolutions value stored in the environment
                // The value shall not decrement below zero
                if (param->parameters.cumul_wheel_rev < 0)
                {
                    cpps_env->cumul_wheel_rev =  (co_abs(param->parameters.cumul_wheel_rev) > cpps_env->cumul_wheel_rev) ?
                            0 : cpps_env->cumul_wheel_rev + param->parameters.cumul_wheel_rev;
                }
                else
                {
                    cpps_env->cumul_wheel_rev += param->parameters.cumul_wheel_rev;
                }
            }
        }

        // start operation execution
        cpps_exe_operation();
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
 * @brief Handles reception of the @ref CPPS_NTF_CP_VECTOR_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ntf_cp_vector_req_handler(ke_msg_id_t const msgid,
                                          struct cpps_ntf_cp_vector_req *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // State shall be Connected or Busy
    if (ke_state_get(dest_id) == CPPS_IDLE)
    {
        // Get the address of the environment
        struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
        // Check Vector characteristic is supported
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_VECTOR_MASK))
        {
            // Configure the environment
            cpps_env->operation = CPPS_NTF_VECTOR_OP_CODE;
            // allocate operation data
            cpps_env->op_data = (struct cpps_op*) ke_malloc(sizeof(struct cpps_op), KE_MEM_KE_MSG);
            cpps_env->op_data->cmd         = ke_param2msg(param);
            cpps_env->op_data->cursor      = 0;
            cpps_env->op_data->ntf_pending = NULL;
            // Go to busy state
            ke_state_set(dest_id, CPPS_BUSY);
            // start operation execution
            cpps_exe_operation();

            msg_status = KE_MSG_NO_FREE;
        }
        else
        {
            // send response to requester
            struct cpps_ntf_cp_vector_rsp * rsp = KE_MSG_ALLOC(
                    CPPS_NTF_CP_VECTOR_RSP,
                    src_id,
                    dest_id,
                    cpps_ntf_cp_vector_rsp);

            rsp->status = PRF_ERR_FEATURE_NOT_SUPPORTED;
            ke_msg_send(rsp);
        }
    }
    else
    {
        // Keep the message for later
        msg_status = KE_MSG_SAVED;
    }

    return (int)msg_status;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_REQ_IND message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    uint8_t conidx = KE_IDX_GET(src_id);
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Status
    uint8_t status = GAP_ERR_NO_ERROR;
    // Control Point Status
    uint8_t ctl_pt_status = CPP_CTNL_PT_RESP_SUCCESS;

    if (cpps_env != NULL)
    {
        // CP Measurement Characteristic, Client Characteristic Configuration Descriptor
        if (param->handle == (CPPS_HANDLE(CPS_IDX_CP_MEAS_NTF_CFG)))
        {
            // Update value
            status = cpps_update_characteristic_config(conidx, CPP_PRF_CFG_FLAG_CP_MEAS_NTF, param);
        }
        // CP Measurement Characteristic, Server Characteristic Configuration Descriptor
        else if (param->handle == (CPPS_HANDLE(CPS_IDX_CP_MEAS_BCST_CFG)))
        {
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK))
            {
                // Save value
                status = cpps_update_characteristic_config(conidx, CPP_PRF_CFG_FLAG_SP_MEAS_NTF, param);
            }
            else
            {
                status = ATT_ERR_ATTRIBUTE_NOT_FOUND;
            }
        }
        // CP Vector Characteristic, Client Characteristic Configuration Descriptor
        else if (param->handle == (CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG)))
        {
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_VECTOR_MASK))
            {
                // Update value
                status = cpps_update_characteristic_config(conidx, CPP_PRF_CFG_FLAG_VECTOR_NTF, param);
            }
            else
            {
                status = ATT_ERR_ATTRIBUTE_NOT_FOUND;
            }
        }
        // CP Control Point Characteristic
        else
        {
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_CTNL_PT_MASK))
            {
                // CP Control Point, Client Characteristic Configuration Descriptor
                if (param->handle == (CPPS_HANDLE(CPS_IDX_CTNL_PT_IND_CFG)))
                {
                    // Update value
                    status = cpps_update_characteristic_config(conidx, CPP_PRF_CFG_FLAG_CTNL_PT_IND, param);
                }
                // CP Control Point Characteristic
                else if (param->handle == (CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL)))
                {
                    do
                    {
                        // Check if sending of indications has been enabled
                        if (!CPPS_IS_NTF_IND_BCST_ENABLED(conidx, CPP_PRF_CFG_FLAG_CTNL_PT_IND))
                        {
                            // CPP improperly configured
                            status = PRF_CCCD_IMPR_CONFIGURED;
                            break;
                        }

                        if (cpps_env->operation >= CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE)
                        {
                            // A procedure is already in progress
                            status = CPP_ERROR_PROC_IN_PROGRESS;
                            break;
                        }

                        if (cpps_env->operation == CPPS_NTF_MEAS_OP_CODE)
                        {
                            // Keep the message until the end of the current procedure
                            msg_status = KE_MSG_NO_FREE;
                            break;
                        }
                        // Unpack Control Point parameters
                        ctl_pt_status = cpps_unpack_ctnl_point_ind (conidx, param);

                    } while (0);
                }
                else
                {
                    ASSERT_ERR(0);
                }
            }
            else
            {
                status = ATT_ERR_ATTRIBUTE_NOT_FOUND;
            }
        }

        // Vector confirmation is sent by calling cpps_vector_cfg_cfm_handler
        if (param->handle != (CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG)))
        {
            //Send write response
            struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                    GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
            cfm->handle = param->handle;
            cfm->status = status;
            ke_msg_send(cfm);

            // Check if control point failed
            if ((param->handle == (CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL))) &&
                    (ctl_pt_status != CPP_CTNL_PT_RESP_SUCCESS))
            {
                // Send response to peer
                cpps_send_rsp_ind(conidx, param->value[0], ctl_pt_status);
            }
        }
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
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gattc_att_info_req_ind *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
        if(ke_state_get(dest_id) == CPPS_IDLE)
        {
            // Get the address of the environment
            struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
            uint8_t att_idx = CPPS_IDX(param->handle);
            struct gattc_att_info_cfm * cfm;

            //Send write response
            cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
            cfm->handle = param->handle;

            // check if it's a client configuration char
            if((att_idx == CPS_IDX_CP_MEAS_NTF_CFG)
                    || (att_idx == CPS_IDX_CP_MEAS_BCST_CFG)
                    || (att_idx == CPS_IDX_VECTOR_NTF_CFG)
                    || (att_idx == CPS_IDX_CTNL_PT_IND_CFG))
            {
                // CCC attribute length = 2
                cfm->length = 2;
                cfm->status = GAP_ERR_NO_ERROR;
            }

            else if (att_idx == CPS_IDX_CTNL_PT_VAL)
            {
                // force length to zero to reject any write starting from something != 0
                cfm->length = 0;
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
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    if(ke_state_get(dest_id) == CPPS_IDLE)
    {
        // Get the address of the environment
        struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = CPPS_IDX(param->handle);

        uint8_t value[4];
        uint8_t value_size = 0;
        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case CPS_IDX_CP_MEAS_NTF_CFG:
            {
                value_size = sizeof(uint16_t);
                co_write16p(value, (cpps_env->env[conidx].prfl_ntf_ind_cfg & CPP_PRF_CFG_FLAG_CP_MEAS_NTF)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            } break;

            case CPS_IDX_CP_MEAS_BCST_CFG:
            {
                // Broadcast feature is profile specific
                if (CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_MEAS_BCST_MASK))
                {
                    // Fill data
                    value_size = sizeof(uint16_t);
                    // Broadcast status is also masked per connection (useful for bonding data)
                    co_write16p(value, (cpps_env->env[conidx].prfl_ntf_ind_cfg & CPP_PRF_CFG_FLAG_SP_MEAS_NTF)
                            ? PRF_SRV_START_BCST : PRF_SRV_STOP_BCST);
                }
                else
                {
                    status = ATT_ERR_ATTRIBUTE_NOT_FOUND;
                }
            } break;

            case CPS_IDX_VECTOR_NTF_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, (cpps_env->env[conidx].prfl_ntf_ind_cfg & CPP_PRF_CFG_FLAG_VECTOR_NTF)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            } break;

            case CPS_IDX_CTNL_PT_IND_CFG:
            {
                // Fill data
                value_size = sizeof(uint16_t);
                co_write16p(value, (cpps_env->env[conidx].prfl_ntf_ind_cfg & CPP_PRF_CFG_FLAG_CTNL_PT_IND)
                        ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND);
            } break;

            case CPS_IDX_CP_FEAT_VAL:
            {
                // Fill data
                value_size = sizeof(uint32_t);
                co_write32p(value, cpps_env->features);
            } break;

            case CPS_IDX_SENSOR_LOC_VAL:
            {
                // Fill data
                value_size = sizeof(uint8_t);
                value[0] = cpps_env->sensor_loc;
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
 * @brief Handles reception of the @ref CPPS_VECTOR_CFG_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_vector_cfg_cfm_handler(ke_msg_id_t const msgid,
                                          struct cpps_vector_cfg_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    // Status
    uint8_t status = PRF_APP_ERROR;

    if (ke_state_get(dest_id) == CPPS_IDLE)
    {
        if (param->status == GAP_ERR_NO_ERROR)
        {
            if ((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF))
            {
                // Save the new configuration in the environment
                (param->ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                        CPPS_DISABLE_NTF_IND_BCST(param->conidx, CPP_PRF_CFG_FLAG_VECTOR_NTF) :
                        CPPS_ENABLE_NTF_IND_BCST(param->conidx, CPP_PRF_CFG_FLAG_VECTOR_NTF);

                status = GAP_ERR_NO_ERROR;
            }
        }

        //Send write response
        struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                GATTC_WRITE_CFM, KE_BUILD_ID(TASK_GATTC, param->conidx), dest_id, gattc_write_cfm);
        cfm->handle = CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG);
        cfm->status = status;
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_CTNL_PT_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ctnl_pt_cfm_handler(ke_msg_id_t const msgid,
                                          struct cpps_ctnl_pt_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);
    uint8_t status = GAP_ERR_NO_ERROR;

    if (ke_state_get(dest_id) == CPPS_BUSY)
    {
        do
        {
            // Check the current operation
            if (cpps_env->operation < CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE)
            {
                // The confirmation has been sent without request indication, ignore
                break;
            }

            // The CP Control Point Characteristic must be supported if we are here
            if (!CPPS_IS_FEATURE_SUPPORTED(cpps_env->prfl_cfg, CPPS_CTNL_PT_MASK))
            {
                status = PRF_ERR_REQ_DISALLOWED;
                break;
            }

            // Check if sending of indications has been enabled
            if (!CPPS_IS_NTF_IND_BCST_ENABLED(param->conidx, CPP_PRF_CFG_FLAG_CTNL_PT_IND))
            {
                // CPP improperly configured
                status = PRF_CCCD_IMPR_CONFIGURED;
                break;
            }

            // Allocate the GATT notification message
            struct gattc_send_evt_cmd *ctl_pt_rsp = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, param->conidx), dest_id,
                    gattc_send_evt_cmd, CPP_CP_CNTL_PT_RSP_MAX_LEN);

            // Fill in the parameter structure
            ctl_pt_rsp->operation = GATTC_INDICATE;
            ctl_pt_rsp->handle = CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL);
            // Pack Control Point confirmation
            ctl_pt_rsp->length = cpps_pack_ctnl_point_cfm (param->conidx, param, ctl_pt_rsp->value);

            // Send the event
            ke_msg_send(ctl_pt_rsp);

        } while (0);

        if (status != GAP_ERR_NO_ERROR)
        {
            // Inform the application that a procedure has been completed
            cpps_send_cmp_evt(param->conidx,
                    prf_src_task_get(&cpps_env->prf_env, param->conidx),
                    prf_dst_task_get(&cpps_env->prf_env, param->conidx),
                    cpps_env->operation, status);
        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that a notification or an indication
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    // Check if a connection exists
    if (ke_state_get(dest_id) == CPPS_BUSY)
    {
        switch (param->operation)
        {
            case (GATTC_NOTIFY):
            {
                // On-going operation execution
                cpps_exe_operation();
            } break;

            case (GATTC_INDICATE):
            {
                ASSERT_INFO(cpps_env->operation >= CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE, 0, 0);

                // Inform the application that a procedure has been completed
                cpps_send_cmp_evt(conidx,
                        prf_src_task_get(&cpps_env->prf_env, conidx),
                        prf_dst_task_get(&cpps_env->prf_env, conidx),
                        cpps_env->operation, param->status);
                // else ignore the message
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
KE_MSG_HANDLER_TAB(cpps)
{
    {CPPS_ENABLE_REQ,               (ke_msg_func_t) cpps_enable_req_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler},
    {CPPS_GET_ADV_DATA_REQ,         (ke_msg_func_t) cpps_get_adv_data_req_handler},
    {CPPS_NTF_CP_MEAS_REQ,          (ke_msg_func_t) cpps_ntf_cp_meas_req_handler},
    {CPPS_NTF_CP_VECTOR_REQ,        (ke_msg_func_t) cpps_ntf_cp_vector_req_handler},
    {CPPS_CTNL_PT_CFM,              (ke_msg_func_t) cpps_ctnl_pt_cfm_handler},
    {CPPS_VECTOR_CFG_CFM,           (ke_msg_func_t) cpps_vector_cfg_cfm_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

void cpps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct cpps_env_tag *cpps_env = PRF_ENV_GET(CPPS, cpps);

    task_desc->msg_handler_tab = cpps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(cpps_msg_handler_tab);
    task_desc->state           = cpps_env->state;
    task_desc->idx_max         = CPPS_IDX_MAX;
}

#endif //(BLE_CSC_SENSOR)

/// @} CPPSTASK
