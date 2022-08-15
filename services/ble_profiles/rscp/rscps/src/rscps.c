/**
 ****************************************************************************************
 * @addtogroup RSCPS
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
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Full RSCPS Database Description - Used to add attributes into the database
static const struct attm_desc rscps_att_db[RSCS_IDX_NB] =
{
    // Running Speed and Cadence Service Declaration
    [RSCS_IDX_SVC]                     =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // RSC Measurement Characteristic Declaration
    [RSCS_IDX_RSC_MEAS_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // RSC Measurement Characteristic Value
    [RSCS_IDX_RSC_MEAS_VAL]            =   {ATT_CHAR_RSC_MEAS, PERM(NTF, ENABLE), PERM(RI, ENABLE), RSCP_RSC_MEAS_MAX_LEN},
    // RSC Measurement Characteristic - Client Characteristic Configuration Descriptor
    [RSCS_IDX_RSC_MEAS_NTF_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // RSC Feature Characteristic Declaration
    [RSCS_IDX_RSC_FEAT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // RSC Feature Characteristic Value
    [RSCS_IDX_RSC_FEAT_VAL]            =   {ATT_CHAR_RSC_FEAT, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},

    // Sensor Location Characteristic Declaration
    [RSCS_IDX_SENSOR_LOC_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Sensor Location Characteristic Value
    [RSCS_IDX_SENSOR_LOC_VAL]          =   {ATT_CHAR_SENSOR_LOC, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},

    // SC Control Point Characteristic Declaration
    [RSCS_IDX_SC_CTNL_PT_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // SC Control Point Characteristic Value - The response has the maximal length
    [RSCS_IDX_SC_CTNL_PT_VAL]          =   {ATT_CHAR_SC_CNTL_PT, PERM(IND, ENABLE) | PERM(WRITE_REQ, ENABLE),
                                                                 PERM(RI, ENABLE), RSCP_SC_CNTL_PT_RSP_MAX_LEN},
    // SC Control Point Characteristic - Client Characteristic Configuration Descriptor
    [RSCS_IDX_SC_CTNL_PT_NTF_CFG]      =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the RSCPS module.
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
static uint8_t rscps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct rscps_db_cfg* param)
{
    //------------------ create the attribute database for the profile -------------------
    // Service content flag
    uint32_t cfg_flag= RSCPS_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    /*
     * Check if the Sensor Location characteristic shall be added.
     * Mandatory if the Multiple Sensor Location feature is supported, otherwise optional.
     */
    if ((param->sensor_loc_supp == RSCPS_SENSOR_LOC_SUPP) ||
        (RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_MULT_SENSOR_LOC_SUPP)))
    {
        cfg_flag |= RSCPS_SENSOR_LOC_MASK;
    }

    /*
     * Check if the SC Control Point characteristic shall be added
     * Mandatory if at least one SC Control Point procedure is supported, otherwise excluded.
     */
    if (RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_CALIB_PROC_SUPP) ||
        RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_MULT_SENSOR_LOC_SUPP) ||
        RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_TOTAL_DST_MEAS_SUPP))
    {
        cfg_flag |= RSCPS_SC_CTNL_PT_MASK;
    }

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_RUNNING_SPEED_CADENCE, (uint8_t *)&cfg_flag,
            RSCS_IDX_NB, NULL, env->task, &rscps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE));

    // Check if an error has occured
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate RSCPS required environment variable
        struct rscps_env_tag* rscps_env =
                (struct rscps_env_tag* ) ke_malloc(sizeof(struct rscps_env_tag), KE_MEM_ATT_DB);

        // Initialize RSCPS environment

        env->env           = (prf_env_t*) rscps_env;
        rscps_env->shdl     = *start_hdl;
        rscps_env->prf_cfg = cfg_flag;
        rscps_env->features = param->rsc_feature;
        rscps_env->operation = RSCPS_RESERVED_OP_CODE;

        if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env->prf_cfg, RSCPS_SENSOR_LOC_MASK))
        {
            rscps_env->sensor_loc = (param->sensor_loc >= RSCP_LOC_MAX) ?
                    RSCP_LOC_OTHER : param->sensor_loc;
        }

        rscps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        rscps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_RSCPS;
        rscps_task_init(&(env->desc));

        rscps_env->ntf = NULL;
        memset(rscps_env->prfl_ntf_ind_cfg, 0, BLE_CONNECTION_MAX);

        // If we are here, database has been fulfilled with success, go to idle state
        ke_state_set(env->task, RSCPS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the RSCPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void rscps_destroy(struct prf_task_env* env)
{
    struct rscps_env_tag* rscps_env = (struct rscps_env_tag*) env->env;

    // cleanup environment variable for each task instances
    if(rscps_env->ntf != NULL)
    {
        ke_free(rscps_env->ntf);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(rscps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void rscps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct rscps_env_tag* rscps_env = (struct rscps_env_tag*) env->env;
    rscps_env->prfl_ntf_ind_cfg[conidx] = 0;

}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void rscps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct rscps_env_tag* rscps_env = (struct rscps_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    rscps_env->prfl_ntf_ind_cfg[conidx] = 0;
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// RSCPS Task interface required by profile manager
const struct prf_task_cbs rscps_itf =
{
        (prf_init_fnct) rscps_init,
        rscps_destroy,
        rscps_create,
        rscps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* rscps_prf_itf_get(void)
{
   return &rscps_itf;
}

void rscps_send_cmp_evt(uint8_t conidx, uint8_t src_id, uint8_t dest_id, uint8_t operation, uint8_t status)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);

    // Go back to the Connected state if the state is busy
    if (ke_state_get(src_id) == RSCPS_BUSY)
    {
        ke_state_set(src_id, RSCPS_IDLE);
    }

    // Set the operation code
    rscps_env->operation = RSCPS_RESERVED_OP_CODE;

    // Send the message
    struct rscps_cmp_evt *evt = KE_MSG_ALLOC(RSCPS_CMP_EVT,
                                             dest_id, src_id,
                                             rscps_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}


void rscps_exe_operation(void)
{
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);

    ASSERT_ERR(rscps_env->ntf != NULL);

    bool finished = true;

    while(rscps_env->ntf->cursor < BLE_CONNECTION_MAX)
    {
        // Check if notifications are enabled
        if(RSCPS_IS_NTFIND_ENABLED(rscps_env->ntf->cursor, RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF))
        {
            // Allocate the GATT notification message
            struct gattc_send_evt_cmd *meas_val = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
                    KE_BUILD_ID(TASK_GATTC, rscps_env->ntf->cursor),
                    prf_src_task_get(&(rscps_env->prf_env), 0),
                    gattc_send_evt_cmd, rscps_env->ntf->length);

            // Fill in the parameter structure
            meas_val->operation = GATTC_NOTIFY;
            meas_val->handle = RSCPS_HANDLE(RSCS_IDX_RSC_MEAS_VAL);
            meas_val->length = rscps_env->ntf->length;
            memcpy(meas_val->value, rscps_env->ntf->value, rscps_env->ntf->length);

            // Send the event
            ke_msg_send(meas_val);

            finished = false;
            rscps_env->ntf->cursor++;
            break;
        }

        rscps_env->ntf->cursor++;
    }

    // check if operation is finished
    if(finished)
    {
        // Inform the application that a procedure has been completed
        struct rscps_ntf_rsc_meas_rsp *rsp = KE_MSG_ALLOC(RSCPS_NTF_RSC_MEAS_RSP,
                prf_dst_task_get(&(rscps_env->prf_env), 0),
                prf_src_task_get(&(rscps_env->prf_env), 0),
                rscps_ntf_rsc_meas_rsp);

        rsp->status     = GAP_ERR_NO_ERROR;
        ke_msg_send(rsp);

        // free operation
        ke_free(rscps_env->ntf);
        rscps_env->ntf = NULL;
        rscps_env->operation = RSCPS_RESERVED_OP_CODE;

        ke_state_set(prf_src_task_get(&(rscps_env->prf_env), 0), RSCPS_IDLE);
    }
}

void rscps_send_rsp_ind(uint8_t conidx, uint8_t req_op_code, uint8_t status)
{
    // Get the address of the environment
    struct rscps_env_tag *rscps_env = PRF_ENV_GET(RSCPS, rscps);

    // Allocate the GATT notification message
    struct gattc_send_evt_cmd *ctl_pt_rsp = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, conidx),
            prf_src_task_get(&rscps_env->prf_env, conidx),
            gattc_send_evt_cmd, RSCP_SC_CNTL_PT_RSP_MIN_LEN);

    // Fill in the parameter structure
    ctl_pt_rsp->operation = GATTC_INDICATE;
    ctl_pt_rsp->handle = RSCPS_HANDLE(RSCS_IDX_SC_CTNL_PT_VAL);
    // Pack Control Point confirmation
    ctl_pt_rsp->length = RSCP_SC_CNTL_PT_RSP_MIN_LEN;
    // Response Code
    ctl_pt_rsp->value[0] = RSCP_CTNL_PT_RSP_CODE;
    // Request Operation Code
    ctl_pt_rsp->value[1] = req_op_code;
    // Response value
    ctl_pt_rsp->value[2] = status;

    // Send the event
    ke_msg_send(ctl_pt_rsp);
}

#endif //(BLE_RSC_SENSOR)

/// @} RSCPS
