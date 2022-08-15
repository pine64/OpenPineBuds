/**
 ****************************************************************************************
 * @addtogroup RSCPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_RSC_COLLECTOR)
#include "rscp_common.h"


#include "rscpc.h"
#include "rscpc_task.h"
#include "ke_timer.h"
#include "ke_mem.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the RSCPC module.
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
static uint8_t rscpc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct rscpc_env_tag* rscpc_env =
            (struct rscpc_env_tag* ) ke_malloc(sizeof(struct rscpc_env_tag), KE_MEM_ATT_DB);

    // allocate RSCPC required environment variable
    env->env = (prf_env_t*) rscpc_env;

    rscpc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    rscpc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_RSCPC;
    rscpc_task_init(&(env->desc));

    for(idx = 0; idx < RSCPC_IDX_MAX ; idx++)
    {
        rscpc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), RSCPC_FREE);
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
static void rscpc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct rscpc_env_tag* rscpc_env = (struct rscpc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(rscpc_env->env[conidx] != NULL)
    {
        if (rscpc_env->env[conidx]->operation != NULL)
        {
            ke_free(ke_param2msg(rscpc_env->env[conidx]->operation));
        }
        ke_timer_clear(RSCPC_TIMEOUT_TIMER_IND, prf_src_task_get(&rscpc_env->prf_env, conidx));
        ke_free(rscpc_env->env[conidx]);
        rscpc_env->env[conidx] = NULL;
    }

    /* Put RSCP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), RSCPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the RSCPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void rscpc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct rscpc_env_tag* rscpc_env = (struct rscpc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < RSCPC_IDX_MAX ; idx++)
    {
        rscpc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(rscpc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void rscpc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put RSCP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), RSCPC_IDLE);
}

/// RSCPC Task interface required by profile manager
const struct prf_task_cbs rscpc_itf =
{
        rscpc_init,
        rscpc_destroy,
        rscpc_create,
        rscpc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* rscpc_prf_itf_get(void)
{
   return &rscpc_itf;
}

void rscpc_enable_rsp_send(struct rscpc_env_tag *rscpc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on RSCPS
    struct rscpc_enable_rsp * rsp = KE_MSG_ALLOC(
            RSCPC_ENABLE_RSP,
            prf_dst_task_get(&(rscpc_env->prf_env), conidx),
            prf_src_task_get(&(rscpc_env->prf_env), conidx),
            rscpc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->rscs = rscpc_env->env[conidx]->rscs;
        // Register RSCPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(rscpc_env->prf_env), conidx, &(rscpc_env->env[conidx]->rscs.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(rscpc_env->prf_env), conidx), RSCPC_IDLE);
    }

    ke_msg_send(rsp);
}

void rscps_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation)
{
    // Send the message
    struct rscpc_cmp_evt *evt = KE_MSG_ALLOC(RSCPC_CMP_EVT,
                                             dest_id, src_id,
                                             rscpc_cmp_evt);

    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void rscpc_send_cmp_evt(struct rscpc_env_tag *rscpc_env, uint8_t conidx, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (rscpc_env->env[conidx]->operation != NULL)
    {
        ke_msg_free(ke_param2msg(rscpc_env->env[conidx]->operation));
        rscpc_env->env[conidx]->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(prf_src_task_get(&(rscpc_env->prf_env), conidx)) == RSCPC_BUSY)
    {
        ke_state_set(prf_src_task_get(&(rscpc_env->prf_env), conidx), RSCPC_IDLE);
    }

    // Send the message
    struct rscpc_cmp_evt *evt = KE_MSG_ALLOC(RSCPC_CMP_EVT,
            prf_dst_task_get(&(rscpc_env->prf_env), conidx),
            prf_src_task_get(&(rscpc_env->prf_env), conidx),
            rscpc_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_RSC_COLLECTOR)

/// @} RSCP
