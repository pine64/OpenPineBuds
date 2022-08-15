/**
 ****************************************************************************************
 * @addtogroup RSCPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_RSC_SENSOR)
#include "rscp_common.h"

#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "rscps.h"
#include "rscps_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscps_enable_req_handler(ke_msg_id_t const msgid,
                                    struct rscps_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(ke_state_get(dest_id) == RSCPS_IDLE)
    {
        status = GAP_ERR_NO_ERROR;

        if (!RSCPS_IS_PRESENT(rscps_env->prfl_ntf_ind_cfg[param->conidx], RSCP_PRF_CFG_PERFORMED_OK))
        {
            // Check the provided value
            if (param->rsc_meas_ntf_cfg == PRF_CLI_START_NTF)
            {
                // Store the status
                RSCPS_ENABLE_NTFIND(param->conidx, RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
            }

            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCPS_SC_CTNL_PT_MASK))
            {
                // Check the provided value
                if (param->sc_ctnl_pt_ntf_cfg == PRF_CLI_START_IND)
                {
                    // Store the status
                    RSCPS_ENABLE_NTFIND(param->conidx, RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                }
            }
            // Enable Bonded Data
            RSCPS_ENABLE_NTFIND(param->conidx, RSCP_PRF_CFG_PERFORMED_OK);
        }
    }

    // send completed information to APP task that contains error status
    struct rscps_enable_rsp *cmp_evt = KE_MSG_ALLOC(RSCPS_ENABLE_RSP, src_id, dest_id, rscps_enable_rsp);
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
    if(ke_state_get(dest_id) == RSCPS_IDLE)
    {
        // Get the address of the environment
        struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
        uint8_t conidx = KE_IDX_GET(src_id);
        uint8_t att_idx = RSCPS_IDX(param->handle);

        // Send data to peer device
        struct gattc_read_cfm* cfm = NULL;

        uint8_t status = ATT_ERR_NO_ERROR;

        switch(att_idx)
        {
            case RSCS_IDX_RSC_MEAS_NTF_CFG:
            {
                // Fill data
                cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint16_t));
                cfm->length = sizeof(uint16_t);
                co_write16p(cfm->value, (rscps_env->prfl_ntf_ind_cfg[conidx] & RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF)
                        ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND);
            } break;

            case RSCS_IDX_RSC_FEAT_VAL:
            {
                // Fill data
                cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint16_t));
                cfm->length = sizeof(uint16_t);
                co_write16p(cfm->value, rscps_env->features);
            } break;

            case RSCS_IDX_SENSOR_LOC_VAL:
            {
                // Fill data
                cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint8_t));
                cfm->length = sizeof(uint8_t);
                cfm->value[0] = rscps_env->sensor_loc;
            } break;

            case RSCS_IDX_SC_CTNL_PT_NTF_CFG:
            {
                // Fill data
                cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm, sizeof(uint16_t));
                cfm->length = sizeof(uint16_t);
                co_write16p(cfm->value, (rscps_env->prfl_ntf_ind_cfg[conidx] & RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND)
                        ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND);
            } break;

            default:
            {
                cfm = KE_MSG_ALLOC(GATTC_READ_CFM, src_id, dest_id, gattc_read_cfm);
                cfm->length = 0;
                status = ATT_ERR_REQUEST_NOT_SUPPORTED;
            } break;
        }

        cfm->handle = param->handle;
        cfm->status = status;

        // Send value to peer device.
        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
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
        if(ke_state_get(dest_id) == RSCPS_IDLE)
        {
            // Get the address of the environment
            struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
            uint8_t att_idx = RSCPS_IDX(param->handle);
            struct gattc_att_info_cfm * cfm;

            //Send write response
            cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, src_id, dest_id, gattc_att_info_cfm);
            cfm->handle = param->handle;

            // check if it's a client configuration char
            if((att_idx == RSCS_IDX_RSC_MEAS_NTF_CFG)
                    || (att_idx == RSCS_IDX_SC_CTNL_PT_NTF_CFG))
            {
                // CCC attribute length = 2
                cfm->length = 2;
                cfm->status = GAP_ERR_NO_ERROR;
            }
            else if (att_idx == RSCS_IDX_SC_CTNL_PT_VAL)
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
 * @brief Handles reception of the @ref RSCPS_NTF_RSC_MEAS_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscps_ntf_rsc_meas_req_handler(ke_msg_id_t const msgid,
                                          struct rscps_ntf_rsc_meas_req *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // State shall be Connected or Busy
    if (ke_state_get(dest_id) == RSCPS_IDLE)
    {
        // Get the address of the environment
        struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
        // allocate and prepare data to notify
        rscps_env->ntf = (struct rscps_ntf*) ke_malloc(sizeof(struct rscps_ntf), KE_MEM_KE_MSG);

        // Fill in the parameter structure
        rscps_env->ntf->length = RSCP_RSC_MEAS_MIN_LEN;

        // Check the provided flags value
        if (!RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCP_FEAT_INST_STRIDE_LEN_SUPP) &&
            RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_INST_STRIDE_LEN_PRESENT))
        {
            // Force Measurement Instantaneous Stride Length Present to No (Not supported)
            param->flags &= ~RSCP_MEAS_INST_STRIDE_LEN_PRESENT;
        }

        if (!RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCP_FEAT_TOTAL_DST_MEAS_SUPP) &&
            RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_TOTAL_DST_MEAS_PRESENT))
        {
            // Force Total Distance Measurement Present to No (Not supported)
            param->flags &= ~RSCP_MEAS_TOTAL_DST_MEAS_PRESENT;
        }

        // Force the unused bits of the flag value to 0
        rscps_env->ntf->value[0] = param->flags & RSCP_MEAS_ALL_PRESENT;

        // Instantaneous Speed
        co_write16p(&rscps_env->ntf->value[1], param->inst_speed);
        // Instantaneous Cadence
        rscps_env->ntf->value[3] = param->inst_cad;

        // Instantaneous Stride Length
        if (RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_INST_STRIDE_LEN_PRESENT))
        {
            co_write16p(&rscps_env->ntf->value[rscps_env->ntf->length], param->inst_stride_len);
            rscps_env->ntf->length += 2;
        }

        // Total Distance
        if (RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_TOTAL_DST_MEAS_PRESENT))
        {
            co_write32p(&rscps_env->ntf->value[rscps_env->ntf->length], param->total_dist);
            rscps_env->ntf->length += 4;
        }

        // Configure the environment
        rscps_env->operation = RSCPS_SEND_RSC_MEAS_OP_CODE;
        rscps_env->ntf->cursor = 0;

        // Go to busy state
        ke_state_set(dest_id, RSCPS_BUSY);

        // start operation execution
        rscps_exe_operation();
    }
    else
    {
        // Save it for later
        msg_status = KE_MSG_SAVED;
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_SC_CTNL_PT_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int rscps_sc_ctnl_pt_cfm_handler(ke_msg_id_t const msgid,
                                          struct rscps_sc_ctnl_pt_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
    uint8_t conidx = KE_IDX_GET(src_id);
    // Status
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if (ke_state_get(dest_id) == RSCPS_BUSY)
    {
        do
        {
            // check if op code valid
            if((param->op_code < RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE)
                    || (param->op_code > RSCPS_CTNL_ERR_IND_OP_CODE))
            {
                //Wrong op code
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check the current operation
            if ((rscps_env->operation < RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE) ||
                    (param->op_code != rscps_env->operation))
            {
                // The confirmation has been sent without request indication, ignore
                status = PRF_ERR_REQ_DISALLOWED;
                break;
            }

            // The SC Control Point Characteristic must be supported if we are here
            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCPS_SC_CTNL_PT_MASK))
            {
                // Allocate the GATT notification message
                struct gattc_send_evt_cmd *ctl_pt_rsp = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                        KE_BUILD_ID(TASK_GATTC, param->conidx), dest_id,
                        gattc_send_evt_cmd, RSCP_SC_CNTL_PT_RSP_MAX_LEN);

                // Fill in the parameter structure
                ctl_pt_rsp->operation = GATTC_INDICATE;
                ctl_pt_rsp->handle = RSCPS_HANDLE(RSCS_IDX_SC_CTNL_PT_VAL);
                // Pack Control Point confirmation
                ctl_pt_rsp->length = RSCP_SC_CNTL_PT_RSP_MIN_LEN;

                // Set the operation code (Response Code)
                ctl_pt_rsp->value[0] = RSCP_CTNL_PT_RSP_CODE;
                // Set the response value
                ctl_pt_rsp->value[2] = (param->status > RSCP_CTNL_PT_RESP_FAILED) ? RSCP_CTNL_PT_RESP_FAILED : param->status;

                switch (rscps_env->operation)
                {
                    // Set cumulative value
                    case (RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE):
                    {
                        // Set the request operation code
                        ctl_pt_rsp->value[1] = RSCP_CTNL_PT_OP_SET_CUMUL_VAL;
                        status = GAP_ERR_NO_ERROR;
                    } break;

                    // Start Sensor Calibration
                    case (RSCPS_CTNL_PT_START_CAL_OP_CODE):
                    {
                        // Set the request operation code
                        ctl_pt_rsp->value[1] = RSCP_CTNL_PT_OP_START_CALIB;
                        status = GAP_ERR_NO_ERROR;
                    } break;

                    // Update Sensor Location
                    case (RSCPS_CTNL_PT_UPD_LOC_OP_CODE):
                    {
                        // Set the request operation code
                        ctl_pt_rsp->value[1] = RSCP_CTNL_PT_OP_UPD_LOC;

                        if (param->status == RSCP_CTNL_PT_RESP_SUCCESS)
                        {
                            // The Sensor Location Characteristic is supported if we are here
                            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCPS_SENSOR_LOC_MASK))
                            {
                                // If operation is successful, update the value in the database
                                rscps_env->sensor_loc = param->value.sensor_loc;
                                status = GAP_ERR_NO_ERROR;
                            }
                        }
                    } break;

                    case (RSCPS_CTNL_PT_SUPP_LOC_OP_CODE):
                    {
                        // Set the request operation code
                        ctl_pt_rsp->value[1] = RSCP_CTNL_PT_OP_REQ_SUPP_LOC;

                        if (param->status == RSCP_CTNL_PT_RESP_SUCCESS)
                        {
                            // Counter
                            uint8_t counter;

                            // Set the list of supported location
                            for (counter = 0; counter < RSCP_LOC_MAX; counter++)
                            {
                                if (((param->value.supp_sensor_loc >> counter) & 0x0001) == 0x0001)
                                {
                                    ctl_pt_rsp->value[ctl_pt_rsp->length] = counter;
                                    ctl_pt_rsp->length++;
                                }
                            }
                            status = GAP_ERR_NO_ERROR;
                        }
                    } break;

                    default:
                    {
                        ASSERT_ERR(0);
                    } break;
                }

                // Send the event
                ke_msg_send(ctl_pt_rsp);
            }
        } while (0);

        if (status != GAP_ERR_NO_ERROR)
        {
            // Inform the application that a procedure has been completed
            rscps_send_cmp_evt(conidx,
                    prf_src_task_get(&rscps_env->prf_env, conidx),
                    prf_dst_task_get(&rscps_env->prf_env, conidx),
                    rscps_env->operation, param->status);
        }
    }

    return (KE_MSG_CONSUMED);
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
__STATIC int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);
    uint8_t conidx = KE_IDX_GET(src_id);
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Check if a connection exists
    if (rscps_env != NULL)
    {
        // RSC Measurement Characteristic, Client Characteristic Configuration Descriptor
        if (param->handle == (RSCPS_HANDLE(RSCS_IDX_RSC_MEAS_NTF_CFG)))
        {
            uint16_t ntf_cfg;
            // Status
            uint8_t status = PRF_ERR_INVALID_PARAM;

            // Get the value
            co_write16p(&ntf_cfg, param->value[0]);

            // Check if the value is correct
            if (ntf_cfg <= PRF_CLI_START_NTF)
            {
                status = GAP_ERR_NO_ERROR;

                // Save the new configuration in the environment
                if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                {
                    RSCPS_DISABLE_NTFIND(conidx, RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
                }
                else    // ntf_cfg == PRF_CLI_START_NTF
                {
                    RSCPS_ENABLE_NTFIND(conidx, RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
                }

                // Inform the HL about the new configuration
                struct rscps_cfg_ntfind_ind *ind = KE_MSG_ALLOC(RSCPS_CFG_NTFIND_IND,
                        prf_dst_task_get(&rscps_env->prf_env, conidx),
                        prf_src_task_get(&rscps_env->prf_env, conidx),
                        rscps_cfg_ntfind_ind);

                ind->char_code = RSCP_RSCS_RSC_MEAS_CHAR;
                ind->ntf_cfg   = ntf_cfg;

                ke_msg_send(ind);

                // Enable Bonded Data
                RSCPS_ENABLE_NTFIND(conidx, RSCP_PRF_CFG_PERFORMED_OK);
            }
            // else status is RSCP_ERROR_CCC_INVALID_PARAM

            // Send the write response to the peer device
            struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                    GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
            cfm->handle = param->handle;
            cfm->status = status;
            ke_msg_send(cfm);
        }
        else        // Should be the SC Control Point Characteristic
        {
            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCPS_SC_CTNL_PT_MASK))
            {
                // SC Control Point, Client Characteristic Configuration Descriptor
                if (param->handle == (RSCPS_HANDLE(RSCS_IDX_SC_CTNL_PT_NTF_CFG)))
                {
                    uint16_t ntf_cfg;
                    // Status
                    uint8_t status = PRF_ERR_INVALID_PARAM;

                    // Get the value
                    co_write16p(&ntf_cfg, param->value[0]);

                    // Check if the value is correct
                    if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
                    {
                        status = GAP_ERR_NO_ERROR;

                        // Save the new configuration in the environment
                        if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                        {
                            RSCPS_DISABLE_NTFIND(conidx, RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                        }
                        else    // ntf_cfg == PRF_CLI_START_IND
                        {
                            RSCPS_ENABLE_NTFIND(conidx, RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                        }

                        // Inform the HL about the new configuration
                        struct rscps_cfg_ntfind_ind *ind = KE_MSG_ALLOC(RSCPS_CFG_NTFIND_IND,
                                prf_dst_task_get(&rscps_env->prf_env, conidx),
                                prf_src_task_get(&rscps_env->prf_env, conidx),
                                rscps_cfg_ntfind_ind);

                        ind->char_code = RSCP_RSCS_SC_CTNL_PT_CHAR;
                        ind->ntf_cfg   = ntf_cfg;

                        ke_msg_send(ind);

                        // Enable Bonded Data
                        RSCPS_ENABLE_NTFIND(conidx, RSCP_PRF_CFG_PERFORMED_OK);
                    }
                    // else status is RSCP_ERROR_CCC_INVALID_PARAM

                    // Send the write response to the peer device
                    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
                    cfm->handle = param->handle;
                    cfm->status = status;
                    ke_msg_send(cfm);
                }
                // SC Control Point Characteristic
                else if (param->handle == (RSCPS_HANDLE(RSCS_IDX_SC_CTNL_PT_VAL)))
                {
                    // Write Response Status
                    uint8_t wr_status  = ATT_ERR_NO_ERROR;
                    // Indication Status
                    uint8_t ind_status = RSCP_CTNL_PT_RESP_NOT_SUPP;

                    do
                    {
                        // Check if sending of indications has been enabled
                        if (!RSCPS_IS_NTFIND_ENABLED(conidx, RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND))
                        {
                            // CCC improperly configured
                            wr_status = RSCP_ERROR_CCC_INVALID_PARAM;
                            ind_status = RSCP_CTNL_PT_RESP_FAILED;
                            break;
                        }

                        if ((rscps_env->operation >= RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE) &&
                                (rscps_env->operation <= RSCPS_CTNL_ERR_IND_OP_CODE))
                        {
                            // A procedure is already in progress
                            wr_status = RSCP_ERROR_PROC_IN_PROGRESS;
                            ind_status = RSCP_CTNL_PT_RESP_FAILED;
                            break;
                        }

                        if (rscps_env->operation == RSCPS_SEND_RSC_MEAS_OP_CODE)
                        {
                            // Keep the message until the end of the current procedure
                            msg_status = KE_MSG_SAVED;
                            break;
                        }

                        // Allocate a request indication message for the application
                        struct rscps_sc_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(RSCPS_SC_CTNL_PT_REQ_IND,
                                prf_dst_task_get(&rscps_env->prf_env, conidx),
                                prf_src_task_get(&rscps_env->prf_env, conidx),
                                rscps_sc_ctnl_pt_req_ind);

                        // Operation Code
                        req_ind->op_code   = param->value[0];
                        // Connection index
                        req_ind->conidx = conidx;

                        // Operation Code
                        switch(param->value[0])
                        {
                            // Set Cumulative value
                            case (RSCP_CTNL_PT_OP_SET_CUMUL_VAL):
                            {
                                // Check if the Total Distance Measurement feature is supported
                                if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->features, RSCP_FEAT_TOTAL_DST_MEAS_SUPP))
                                {
                                    // The request can be handled
                                    ind_status = RSCP_CTNL_PT_RESP_SUCCESS;
                                    rscps_env->operation = RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE;

                                    // Cumulative value
                                    req_ind->value.cumul_value = co_read32p(&param->value[1]);
                                }
                            } break;

                            // Start Sensor calibration
                            case (RSCP_CTNL_PT_OP_START_CALIB):
                            {
                                // Check if the Calibration Procedure feature is supported
                                if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->features, RSCP_FEAT_CALIB_PROC_SUPP))
                                {
                                    // The request can be handled
                                    ind_status = RSCP_CTNL_PT_RESP_SUCCESS;

                                    rscps_env->operation = RSCPS_CTNL_PT_START_CAL_OP_CODE;
                                }
                            } break;

                            // Update sensor location
                            case (RSCP_CTNL_PT_OP_UPD_LOC):
                            {
                                // Check if the Multiple Sensor Location feature is supported
                                if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->features, RSCP_FEAT_MULT_SENSOR_LOC_SUPP))
                                {
                                    // Check the sensor location value
                                    if (param->value[1] < RSCP_LOC_MAX)
                                    {
                                        // The request can be handled
                                        ind_status = RSCP_CTNL_PT_RESP_SUCCESS;

                                        rscps_env->operation = RSCPS_CTNL_PT_UPD_LOC_OP_CODE;

                                        // Sensor Location
                                        req_ind->value.sensor_loc = param->value[1];
                                    }
                                    else
                                    {
                                        // Provided parameter in not within the defined range
                                        ind_status = RSCP_CTNL_PT_RESP_INV_PARAM;
                                    }
                                }
                            } break;

                            // Request supported sensor locations
                            case (RSCP_CTNL_PT_OP_REQ_SUPP_LOC):
                            {
                                // Check if the Multiple Sensor Location feature is supported
                                if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->features, RSCP_FEAT_MULT_SENSOR_LOC_SUPP))
                                {
                                    // The request can be handled
                                    ind_status = RSCP_CTNL_PT_RESP_SUCCESS;

                                    rscps_env->operation = RSCPS_CTNL_PT_SUPP_LOC_OP_CODE;
                                }
                            } break;

                            default:
                            {
                                // Operation Code is invalid, status is already RSCP_CTNL_PT_RESP_NOT_SUPP
                            } break;
                        }

                        // If no error raised, inform the application about the request
                        if (ind_status == RSCP_CTNL_PT_RESP_SUCCESS)
                        {
                            // Send the request indication to the application
                            ke_msg_send(req_ind);
                            // Go to the Busy status
                            ke_state_set(dest_id, RSCPS_BUSY);
                            // Align error code
                            wr_status  = GAP_ERR_NO_ERROR;
                        }
                        else
                        {
                            // Free the allocated message
                            ke_msg_free(ke_param2msg(req_ind));
                        }
                    } while (0);

                    // Send the write response to the peer device
                    struct gattc_write_cfm *cfm = KE_MSG_ALLOC(
                            GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
                    cfm->handle = param->handle;
                    cfm->status = wr_status;
                    ke_msg_send(cfm);

                    // If error raised in control point, inform the peer
                    if ((ind_status != RSCP_CTNL_PT_RESP_SUCCESS) &&
                            (param->handle == (RSCPS_HANDLE(RSCS_IDX_SC_CTNL_PT_VAL))))
                    {
                        rscps_send_rsp_ind(conidx, param->value[0], ind_status);
                    }
                }
                else
                {
                    ASSERT_ERR(0);
                }
            }
        }
    }
    // else drop the message

    return (int)msg_status;
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
__STATIC int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);

    // Check if a connection exists
    if (ke_state_get(dest_id) == RSCPS_BUSY)
    {
        switch (param->operation)
        {
            case (GATTC_NOTIFY):
            {
                ASSERT_ERR(rscps_env->operation == RSCPS_SEND_RSC_MEAS_OP_CODE);
                // continuer operation execution
                rscps_exe_operation();
            } break;

            case (GATTC_INDICATE):
            {
                ASSERT_ERR((rscps_env->operation >= RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE) ||
                        (rscps_env->operation != RSCPS_CTNL_ERR_IND_OP_CODE));

                // Inform the application that a procedure has been completed
                rscps_send_cmp_evt(conidx,
                        prf_src_task_get(&rscps_env->prf_env, conidx),
                        prf_dst_task_get(&rscps_env->prf_env, conidx),
                        rscps_env->operation, param->status);
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
KE_MSG_HANDLER_TAB(rscps)
{
    {RSCPS_ENABLE_REQ,              (ke_msg_func_t) rscps_enable_req_handler},
    {GATTC_READ_REQ_IND,            (ke_msg_func_t) gattc_read_req_ind_handler},
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t) gattc_att_info_req_ind_handler},
    {RSCPS_NTF_RSC_MEAS_REQ,        (ke_msg_func_t) rscps_ntf_rsc_meas_req_handler},
    {RSCPS_SC_CTNL_PT_CFM,          (ke_msg_func_t) rscps_sc_ctnl_pt_cfm_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t) gattc_write_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

void rscps_task_init(struct ke_task_desc *task_desc)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);

    task_desc->msg_handler_tab = rscps_msg_handler_tab;
    task_desc->msg_cnt         = ARRAY_LEN(rscps_msg_handler_tab);
    task_desc->state           = rscps_env->state;
    task_desc->idx_max         = RSCPS_IDX_MAX;
}

#endif //(BLE_RSC_SENSOR)

/// @} RSCPSTASK
