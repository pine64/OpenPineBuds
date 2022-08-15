/**
 ****************************************************************************************
 * @addtogroup TIPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TIP_CLIENT)
#include "prf.h"
#include "tipc.h"
#include "tipc_task.h"
#include "gap.h"

#include "ke_mem.h"

/**
 ****************************************************************************************
 * @brief Initialization of the TIPC module.
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
static uint8_t tipc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct tipc_env_tag* tipc_env =
            (struct tipc_env_tag* ) ke_malloc(sizeof(struct tipc_env_tag), KE_MEM_ATT_DB);

    // allocate TIPC required environment variable
    env->env = (prf_env_t*) tipc_env;

    tipc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    tipc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_TIPC;
    tipc_task_init(&(env->desc));

    for(idx = 0; idx < TIPC_IDX_MAX ; idx++)
    {
        tipc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), TIPC_FREE);
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
static void tipc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct tipc_env_tag* tipc_env = (struct tipc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(tipc_env->env[conidx] != NULL)
    {
        ke_free(tipc_env->env[conidx]);
        tipc_env->env[conidx] = NULL;
    }

    /* Put TIP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), TIPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the TIPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void tipc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct tipc_env_tag* tipc_env = (struct tipc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < TIPC_IDX_MAX ; idx++)
    {
        tipc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(tipc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void tipc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put TIP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), TIPC_IDLE);
}

/// TIPC Task interface required by profile manager
const struct prf_task_cbs tipc_itf =
{
        tipc_init,
        tipc_destroy,
        tipc_create,
        tipc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* tipc_prf_itf_get(void)
{
   return &tipc_itf;
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void tipc_enable_rsp_send(struct tipc_env_tag *tipc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on TIPS
    struct tipc_enable_rsp * rsp = KE_MSG_ALLOC(TIPC_ENABLE_RSP,
            prf_dst_task_get(&(tipc_env->prf_env), conidx),
            prf_src_task_get(&(tipc_env->prf_env), conidx),
            tipc_enable_rsp);

    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->cts    = tipc_env->env[conidx]->cts;
        rsp->ndcs   = tipc_env->env[conidx]->ndcs;
        rsp->rtus   = tipc_env->env[conidx]->rtus;

        //register TIPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&tipc_env->prf_env, conidx, &tipc_env->env[conidx]->cts.svc);

        // Go to connected state
        ke_state_set(prf_src_task_get(&tipc_env->prf_env, conidx), TIPC_IDLE);
    }

    ke_msg_send(rsp);
}

void tipc_unpack_curr_time_value(struct tip_curr_time* p_curr_time_val, uint8_t* packed_ct)
{
    //Date-Time
    prf_unpack_date_time(packed_ct, &(p_curr_time_val->date_time));

    //Day of Week
    p_curr_time_val->day_of_week = packed_ct[7];

    //Fraction 256
    p_curr_time_val->fraction_256 = packed_ct[8];

    //Adjust Reason
    p_curr_time_val->adjust_reason = packed_ct[9];
}

void tipc_unpack_time_dst_value(struct tip_time_with_dst* p_time_dst_val, uint8_t* packed_tdst)
{
    //Date-Time
    prf_unpack_date_time(packed_tdst, &(p_time_dst_val->date_time));

    //DST Offset
    p_time_dst_val->dst_offset = packed_tdst[7];
}

#endif /* (BLE_TIP_CLIENT) */

/// @} TIPC
