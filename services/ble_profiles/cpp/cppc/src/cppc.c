/**
 ****************************************************************************************
 * @addtogroup CPPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#if (BLE_CP_COLLECTOR)

#include "cpp_common.h"
#include "cppc.h"
#include "cppc_task.h"
#include "ke_timer.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CPPC module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @see enum attm_value_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t cppc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct cppc_env_tag* cppc_env =
            (struct cppc_env_tag* ) ke_malloc(sizeof(struct cppc_env_tag), KE_MEM_ATT_DB);

    // allocate CPPC required environment variable
    env->env = (prf_env_t*) cppc_env;

    cppc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    cppc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_CPPC;
    cppc_task_init(&(env->desc));

    for(idx = 0; idx < CPPC_IDX_MAX ; idx++)
    {
        cppc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), CPPC_FREE);
    }

    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void cppc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct cppc_env_tag* cppc_env = (struct cppc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(cppc_env->env[conidx] != NULL)
    {
        if (cppc_env->env[conidx]->operation != NULL)
        {
            ke_free(ke_param2msg(cppc_env->env[conidx]->operation));
        }
        ke_timer_clear(CPPC_TIMEOUT_TIMER_IND, prf_src_task_get(&cppc_env->prf_env, conidx));
        ke_free(cppc_env->env[conidx]);
        cppc_env->env[conidx] = NULL;
    }

    /* Put CPP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CPPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the CPPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void cppc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct cppc_env_tag* cppc_env = (struct cppc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < CPPC_IDX_MAX ; idx++)
    {
        cppc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(cppc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void cppc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put CPP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CPPC_IDLE);
}

/// CPPC Task interface required by profile manager
const struct prf_task_cbs cppc_itf =
{
        cppc_init,
        cppc_destroy,
        cppc_create,
        cppc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* cppc_prf_itf_get(void)
{
   return &cppc_itf;
}


void cppc_enable_rsp_send(struct cppc_env_tag *cppc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on CPPS
    struct cppc_enable_rsp * rsp = KE_MSG_ALLOC(
            CPPC_ENABLE_RSP,
            prf_dst_task_get(&(cppc_env->prf_env), conidx),
            prf_src_task_get(&(cppc_env->prf_env), conidx),
            cppc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->cps = cppc_env->env[conidx]->cps;
        // Register CPPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(cppc_env->prf_env), conidx, &(cppc_env->env[conidx]->cps.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(cppc_env->prf_env), conidx), CPPC_IDLE);
    }

    ke_msg_send(rsp);
}


void cppc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation)
{
    // Send the message
    struct cppc_cmp_evt *evt = KE_MSG_ALLOC(CPPC_CMP_EVT,
                                             dest_id, src_id,
                                             cppc_cmp_evt);

    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void cppc_send_cmp_evt(struct cppc_env_tag *cppc_env, uint8_t conidx, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (cppc_env->env[conidx]->operation != NULL)
    {
        ke_msg_free(ke_param2msg(cppc_env->env[conidx]->operation));
        cppc_env->env[conidx]->operation = NULL;
    }

    // Go back to the IDLE state if the state is busy
    if (ke_state_get(prf_src_task_get(&(cppc_env->prf_env), conidx)) == CPPC_BUSY)
    {
        ke_state_set(prf_src_task_get(&cppc_env->prf_env, conidx), CPPC_IDLE);
    }

    // Send the message
    struct cppc_cmp_evt *evt = KE_MSG_ALLOC(CPPC_CMP_EVT,
                                             prf_dst_task_get(&(cppc_env->prf_env), conidx),
                                             prf_src_task_get(&(cppc_env->prf_env), conidx),
                                             cppc_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

uint16_t cppc_get_read_handle_req (struct cppc_env_tag *cppc_env, uint8_t conidx, struct cpps_read_cmd *param)
{
    // Attribute Handle
    uint16_t handle = ATT_INVALID_SEARCH_HANDLE;

    switch (param->read_code)
    {
        // Read CP Feature
        case (CPPC_RD_CP_FEAT):
        {
            handle = cppc_env->env[conidx]->cps.chars[CPP_CPS_FEAT_CHAR].val_hdl;
        } break;

        // Read Sensor Location
        case (CPPC_RD_SENSOR_LOC):
        {
            handle = cppc_env->env[conidx]->cps.chars[CPP_CPS_SENSOR_LOC_CHAR].val_hdl;
        } break;

        // Read CP Measurement Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_CL_CFG):
        {
            handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CP_MEAS_CL_CFG].desc_hdl;
        } break;

        // Read CP Measurement Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_SV_CFG):
        {
            handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CP_MEAS_SV_CFG].desc_hdl;
        } break;

        // Read CP Vector Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_VECTOR_CFG):
        {
            handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_VECTOR_CL_CFG].desc_hdl;
        } break;

        // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CTNL_PT_CFG):
        {
            handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CTNL_PT_CL_CFG].desc_hdl;
        } break;

        default:
        {
            handle = ATT_INVALID_SEARCH_HANDLE;
        } break;
    }

    return handle;
}

uint8_t cppc_get_write_desc_handle_req (uint8_t conidx, struct cppc_cfg_ntfind_cmd *param, struct cppc_env_tag *cppc_env, uint16_t *handle)
{
    // Status
    uint8_t status    = GAP_ERR_NO_ERROR;

    switch(param->desc_code)
    {
        // Write CP Measurement Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_CL_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CP_MEAS_CL_CFG].desc_hdl;

                // The descriptor is mandatory
                ASSERT_ERR(*handle != ATT_INVALID_SEARCH_HANDLE);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        } break;

        // Write CP Measurement Characteristic Server Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CP_MEAS_SV_CFG):
        {
            if (param->ntfind_cfg <= PRF_SRV_START_BCST)
            {
                *handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CP_MEAS_SV_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        break;

        // Write CP Vector Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_VECTOR_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_VECTOR_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        break;

        // Write SC Control Point Characteristic Client Char. Cfg. Descriptor Value
        case (CPPC_RD_WR_CTNL_PT_CFG):
        {
            if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                (param->ntfind_cfg == PRF_CLI_START_IND))
            {
                *handle = cppc_env->env[conidx]->cps.descs[CPPC_DESC_CTNL_PT_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
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

    return (status);
}

uint8_t cppc_unpack_meas_ind (uint8_t conidx, struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // CP Measurement value has been received
    struct cppc_value_meas_ind *ind = KE_MSG_ALLOC(
            CPPC_VALUE_IND,
            prf_dst_task_get(&(cppc_env->prf_env), conidx),
            prf_src_task_get(&(cppc_env->prf_env), conidx),
            cppc_value_meas_ind);

    // Offset
    uint8_t offset = CPP_CP_MEAS_NTF_MIN_LEN;

    // Attribute code
    ind->att_code = CPPC_NTF_CP_MEAS;

    // Flags
    ind->value.cp_meas.flags = co_read16p(&param->value[0]);
    // Instant power
    ind->value.cp_meas.inst_power = co_read16p(&param->value[2]);

    if (ind->value.cp_meas.flags & CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT)
    {
        //Unpack Pedal Power Balance info
        ind->value.cp_meas.pedal_power_balance = param->value[offset];
        offset++;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_ACCUM_TORQUE_PRESENT)
    {
        //Unpack Accumulated Torque info
        ind->value.cp_meas.accum_torque = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_WHEEL_REV_DATA_PRESENT)
    {
        //Unpack Wheel Revolution Data (Cumulative Wheel & Last Wheel Event Time)
        ind->value.cp_meas.cumul_wheel_rev = co_read32p(&param->value[offset]);
        offset += 4;
        ind->value.cp_meas.last_wheel_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_CRANK_REV_DATA_PRESENT)
    {
        //Unpack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
        ind->value.cp_meas.cumul_crank_rev = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.last_crank_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT)
    {
        //Unpack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
        ind->value.cp_meas.max_force_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.min_force_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
    }

    else if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT)
    {
        //Unpack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
        ind->value.cp_meas.max_torque_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
        ind->value.cp_meas.min_torque_magnitude = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_EXTREME_ANGLES_PRESENT)
    {
        //Unpack Extreme Angles (Maximum Angle & Minimum Angle)
        uint32_t angle = co_read24p(&param->value[offset]);
        offset += 3;

        //Force to 12 bits
        ind->value.cp_meas.max_angle = (angle & (0x0FFF));
        ind->value.cp_meas.min_angle = ((angle>>12) & 0x0FFF);
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT)
    {
        //Unpack Top Dead Spot Angle
        ind->value.cp_meas.top_dead_spot_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT)
    {
        //Unpack Bottom Dead Spot Angle
        ind->value.cp_meas.bot_dead_spot_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_meas.flags & CPP_MEAS_ACCUM_ENERGY_PRESENT)
    {
        //Unpack Accumulated Energy
        ind->value.cp_meas.accum_energy = co_read16p(&param->value[offset]);
        offset += 2;
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}

uint8_t cppc_unpack_vector_ind (uint8_t conidx, struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // CP Measurement value has been received
    struct cppc_value_ind *ind = KE_MSG_ALLOC_DYN(
            CPPC_VALUE_IND,
            prf_dst_task_get(&cppc_env->prf_env, conidx),
            prf_src_task_get(&cppc_env->prf_env, conidx),
            cppc_value_ind,
            param->length);

    // Offset
    uint8_t offset = CPP_CP_VECTOR_MIN_LEN;

    // Attribute code
    ind->att_code = CPPC_NTF_CP_VECTOR;

    // Flags
    ind->value.cp_vector.flags = param->value[0];

    if (ind->value.cp_vector.flags & CPP_VECTOR_CRANK_REV_DATA_PRESENT)
    {
        // Unpack Crank Revolution Data
        ind->value.cp_vector.cumul_crank_rev = co_read16p(&param->value[offset]);
        offset += 2;
        // Unpack Last Crank Evt time
        ind->value.cp_vector.last_crank_evt_time = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.cp_vector.flags & CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT)
    {
        // Unpack First Crank Measurement Angle
        ind->value.cp_vector.first_crank_meas_angle = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (!(ind->value.cp_vector.flags & CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT) !=
            !(ind->value.cp_vector.flags & CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT))
    {
        // Unpack Force or Torque magnitude (mutually excluded)
        ind->value.cp_vector.nb = (param->length - offset)/2;
        if (ind->value.cp_vector.nb)
        {
            for (int i = 0; i < ind->value.cp_vector.nb; i++)
            {
                // Handle the array buffer to extract parameters
                ind->value.cp_vector.force_torque_magnitude[i] = co_read16p(&param->value[offset]);
                offset += 2;
            }
        }
    }

//    if ((ind->value.cp_vector.flags & CPP_VECTOR_INST_MEAS_DIRECTION_LSB) ||
//            (ind->value.cp_vector.flags & CPP_VECTOR_INST_MEAS_DIRECTION_MSB))
//    {
//
//    }

    // Send the message
    ke_msg_send(ind);

    return offset;

}

uint8_t cppc_pack_ctnl_pt_req (struct cppc_ctnl_pt_cfg_req *param, uint8_t *req, uint8_t *status)
{
    // Request Length
    uint8_t req_len = CPP_CP_CNTL_PT_REQ_MIN_LEN;

    // Set the operation code
    req[0] = param->ctnl_pt.op_code;

    // Fulfill the message according to the operation code
    switch (param->ctnl_pt.op_code)
    {
        case (CPP_CTNL_PT_SET_CUMUL_VAL):
        {
            // Set the cumulative value
            co_write32p(&req[req_len], param->ctnl_pt.value.cumul_val);
            // Update length
            req_len += 4;
        } break;

        case (CPP_CTNL_PT_UPD_SENSOR_LOC):
        {
            // Set the sensor location
            req[req_len] = param->ctnl_pt.value.sensor_loc;
            // Update length
            req_len++;
        } break;

        case (CPP_CTNL_PT_SET_CRANK_LENGTH):
        {
            // Set the crank length
            co_write16p(&req[req_len], param->ctnl_pt.value.crank_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
        {
            // Set the chain length
            co_write16p(&req[req_len], param->ctnl_pt.value.chain_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
        {
            // Set the chain weight
            co_write16p(&req[req_len], param->ctnl_pt.value.chain_weight);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_SET_SPAN_LENGTH):
        {
            // Set the span length
            co_write16p(&req[req_len], param->ctnl_pt.value.span_length);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
        {
            // Set the Content Mask
            co_write16p(&req[req_len], param->ctnl_pt.value.mask_content);
            // Update length
            req_len += 2;
        } break;

        case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
        case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
        case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
        case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
        case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
        case (CPP_CTNL_PT_START_OFFSET_COMP):
        case (CPP_CTNL_REQ_SAMPLING_RATE):
        case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
        {
            // Nothing more to do
        } break;

        default:
        {
            *status = PRF_ERR_INVALID_PARAM;
        } break;
    }

    return req_len;
}

uint8_t cppc_unpack_ctln_pt_ind (uint8_t conidx, struct gattc_event_ind const *param, struct cppc_env_tag *cppc_env)
{
    // Offset
    uint8_t offset = CPP_CP_CNTL_PT_RSP_MIN_LEN;

    // Control Point value has been received
    struct cppc_ctnl_pt_rsp *ind = KE_MSG_ALLOC(
            CPPC_CTNL_PT_RSP,
            prf_dst_task_get(&cppc_env->prf_env, conidx),
            prf_src_task_get(&cppc_env->prf_env, conidx),
            cppc_ctnl_pt_rsp);

    // Requested operation code
    ind->rsp.req_op_code = param->value[1];
    // Response value
    ind->rsp.resp_value  = param->value[2];

    if ((ind->rsp.resp_value == CPP_CTNL_PT_RESP_SUCCESS) && (param->length >= 3))
    {
        switch (ind->rsp.req_op_code)
        {
            case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
            {
                // Get the number of supported locations that have been received
               uint8_t nb_supp_loc = (param->length - 3);
               // Location
               uint8_t loc;

               for (uint8_t counter = 0; counter < nb_supp_loc; counter++)
               {
                   loc = param->value[counter + CPP_CP_CNTL_PT_RSP_MIN_LEN];
                   // Check if valid
                   if (loc < CPP_LOC_MAX)
                   {
                       ind->rsp.value.supp_loc |= (1 << loc);
                   }
                   offset++;
               }
            } break;

            case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
            {
                ind->rsp.value.crank_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
            {
                ind->rsp.value.chain_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
            {
                ind->rsp.value.chain_weight = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
            {
                ind->rsp.value.span_length = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_PT_START_OFFSET_COMP):
            {
                ind->rsp.value.offset_comp = co_read16p(&param->value[offset]);
                offset += 2;
            } break;

            case (CPP_CTNL_REQ_SAMPLING_RATE):
            {
                ind->rsp.value.sampling_rate = param->value[offset];
                offset++;
            } break;

            case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
            {
                offset += prf_unpack_date_time(
                        (uint8_t *) &(param->value[offset]), &(ind->rsp.value.factory_calibration));
            } break;

            case (CPP_CTNL_PT_SET_CUMUL_VAL):
            case (CPP_CTNL_PT_UPD_SENSOR_LOC):
            case (CPP_CTNL_PT_SET_CRANK_LENGTH):
            case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
            case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
            case (CPP_CTNL_PT_SET_SPAN_LENGTH):
            case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
            {
                // No parameters
            } break;

            default:
            {

            } break;
        }
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}
#endif //(BLE_CP_COLLECTOR)

/// @} CPP
