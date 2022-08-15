/**
 ****************************************************************************************
 * @addtogroup CSCPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_CSC_COLLECTOR)
#include "cscp_common.h"

#include "cscpc.h"
#include "cscpc_task.h"
#include "ke_timer.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CSCPC module.
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
static uint8_t cscpc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct cscpc_env_tag* cscpc_env =
            (struct cscpc_env_tag* ) ke_malloc(sizeof(struct cscpc_env_tag), KE_MEM_ATT_DB);

    // allocate CSCPC required environment variable
    env->env = (prf_env_t*) cscpc_env;

    cscpc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    cscpc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_CSCPC;
    cscpc_task_init(&(env->desc));

    for(idx = 0; idx < CSCPC_IDX_MAX ; idx++)
    {
        cscpc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), CSCPC_FREE);
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
static void cscpc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct cscpc_env_tag* cscpc_env = (struct cscpc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(cscpc_env->env[conidx] != NULL)
    {
        if (cscpc_env->env[conidx]->operation != NULL)
        {
            ke_free(ke_param2msg(cscpc_env->env[conidx]->operation));
        }
        ke_timer_clear(CSCPC_TIMEOUT_TIMER_IND, prf_src_task_get(&cscpc_env->prf_env, conidx));

        ke_free(cscpc_env->env[conidx]);
        cscpc_env->env[conidx] = NULL;
    }

    /* Put CSCP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CSCPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the CSCPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void cscpc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct cscpc_env_tag* cscpc_env = (struct cscpc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < CSCPC_IDX_MAX ; idx++)
    {
        cscpc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(cscpc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void cscpc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put CSCP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CSCPC_IDLE);
}

/// CSCPC Task interface required by profile manager
const struct prf_task_cbs cscpc_itf =
{
        cscpc_init,
        cscpc_destroy,
        cscpc_create,
        cscpc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* cscpc_prf_itf_get(void)
{
   return &cscpc_itf;
}


void cscpc_enable_rsp_send(struct cscpc_env_tag *cscpc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on CSCPS
    struct cscpc_enable_rsp * rsp = KE_MSG_ALLOC(
            CSCPC_ENABLE_RSP,
            prf_dst_task_get(&(cscpc_env->prf_env), conidx),
            prf_src_task_get(&(cscpc_env->prf_env), conidx),
            cscpc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->cscs = cscpc_env->env[conidx]->cscs;
        // Register CSCPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(cscpc_env->prf_env), conidx, &(cscpc_env->env[conidx]->cscs.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(cscpc_env->prf_env), conidx), CSCPC_IDLE);
    }

    ke_msg_send(rsp);
}

void cscpc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint8_t operation)
{
    // Send the message
    struct cscpc_cmp_evt *evt = KE_MSG_ALLOC(CSCPC_CMP_EVT,
                                             dest_id, src_id,
                                             cscpc_cmp_evt);

    evt->operation  = operation;
    evt->status     = PRF_ERR_REQ_DISALLOWED;

    ke_msg_send(evt);
}

void cscpc_send_cmp_evt(struct cscpc_env_tag *cscpc_env, uint8_t conidx, uint8_t operation, uint8_t status)
{
    // Free the stored operation if needed
    if (cscpc_env->env[conidx]->operation != NULL)
    {
        ke_msg_free(ke_param2msg(cscpc_env->env[conidx]->operation));
        cscpc_env->env[conidx]->operation = NULL;
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(prf_src_task_get(&(cscpc_env->prf_env), conidx)) == CSCPC_BUSY)
    {
        ke_state_set(prf_src_task_get(&(cscpc_env->prf_env), conidx), CSCPC_IDLE);
    }

    // Send the message
    struct cscpc_cmp_evt *evt = KE_MSG_ALLOC(CSCPC_CMP_EVT,
                                             prf_dst_task_get(&(cscpc_env->prf_env), conidx),
                                             prf_src_task_get(&(cscpc_env->prf_env), conidx),
                                             cscpc_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCP
