/**
 ****************************************************************************************
 * @addtogroup BLPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BP_COLLECTOR)
#include "blpc.h"
#include "blpc_task.h"
#include "gap.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BLPC module.
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
static uint8_t blpc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct blpc_env_tag* blpc_env =
            (struct blpc_env_tag* ) ke_malloc(sizeof(struct blpc_env_tag), KE_MEM_ATT_DB);

    // allocate BLPC required environment variable
    env->env = (prf_env_t*) blpc_env;

    blpc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    blpc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_BLPC;
    blpc_task_init(&(env->desc));

    for(idx = 0; idx < BLPC_IDX_MAX ; idx++)
    {
        blpc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), BLPC_FREE);
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
static void blpc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct blpc_env_tag* blpc_env = (struct blpc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(blpc_env->env[conidx] != NULL)
    {
        ke_free(blpc_env->env[conidx]);
        blpc_env->env[conidx] = NULL;
    }

    /* Put BLP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), BLPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the BLPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void blpc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct blpc_env_tag* blpc_env = (struct blpc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < BLPC_IDX_MAX ; idx++)
    {
        blpc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(blpc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void blpc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put BLP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), BLPC_IDLE);
}

/// BLPC Task interface required by profile manager
const struct prf_task_cbs blpc_itf =
{
        blpc_init,
        blpc_destroy,
        blpc_create,
        blpc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* blpc_prf_itf_get(void)
{
   return &blpc_itf;
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void blpc_enable_rsp_send(struct blpc_env_tag *blpc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on BLPS
    struct blpc_enable_rsp * rsp = KE_MSG_ALLOC(
            BLPC_ENABLE_RSP,
            prf_dst_task_get(&(blpc_env->prf_env), conidx),
            prf_src_task_get(&(blpc_env->prf_env), conidx),
            blpc_enable_rsp);

    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->bps = blpc_env->env[conidx]->bps;

        prf_register_atthdl2gatt(&(blpc_env->prf_env), conidx, &(blpc_env->env[conidx]->bps.svc));

        // Go to connected state
        ke_state_set(prf_src_task_get(&(blpc_env->prf_env), conidx), BLPC_IDLE);
    }

    ke_msg_send(rsp);
}

void blpc_unpack_meas_value(struct bps_bp_meas* pmeas_val, uint8_t* packed_bp)
{
    uint8_t cursor;

    // blood pressure measurement flags
    pmeas_val->flags = packed_bp[0];

    // Blood Pressure Measurement Compound Value - Systolic
    pmeas_val->systolic = co_read16p(&(packed_bp[1]));

    // Blood Pressure Measurement Compound Value - Diastolic (mmHg)
    pmeas_val->diastolic = co_read16p(&(packed_bp[3]));

    //  Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg)
    pmeas_val->mean_arterial_pressure = co_read16p(&(packed_bp[5]));

    cursor = 7;

    // time flag set
    if ((pmeas_val->flags & BPS_FLAG_TIME_STAMP_PRESENT) == BPS_FLAG_TIME_STAMP_PRESENT)
    {
        cursor += prf_unpack_date_time(packed_bp + cursor, &(pmeas_val->time_stamp));
    }

    // pulse rate flag set
    if ((pmeas_val->flags & BPS_FLAG_PULSE_RATE_PRESENT) == BPS_FLAG_PULSE_RATE_PRESENT)
    {
        pmeas_val->pulse_rate = co_read16p(&(packed_bp[cursor + 0]));
        cursor += 2;
    }

    // User ID flag set
    if ((pmeas_val->flags & BPS_FLAG_USER_ID_PRESENT) == BPS_FLAG_USER_ID_PRESENT)
    {
        pmeas_val->user_id = packed_bp[cursor + 0];
        cursor += 1;
    }

    // measurement status flag set
    if ((pmeas_val->flags & BPS_FLAG_MEAS_STATUS_PRESENT) == BPS_FLAG_MEAS_STATUS_PRESENT)
    {
        pmeas_val->meas_status = co_read16p(&(packed_bp[cursor + 0]));
        cursor += 2;
    }
}

#endif /* (BLE_BP_COLLECTOR) */

/// @} BLPC
