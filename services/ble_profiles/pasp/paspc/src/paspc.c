/**
 ****************************************************************************************
 * @addtogroup PASPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_PAS_CLIENT)
#include "pasp_common.h"

#include "paspc.h"
#include "paspc_task.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PASPC module.
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
static uint8_t paspc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct paspc_env_tag* paspc_env =
            (struct paspc_env_tag* ) ke_malloc(sizeof(struct paspc_env_tag), KE_MEM_ATT_DB);

    // allocate PASPC required environment variable
    env->env = (prf_env_t*) paspc_env;

    paspc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    paspc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_PASPC;
    paspc_task_init(&(env->desc));

    for(idx = 0; idx < PASPC_IDX_MAX ; idx++)
    {
        paspc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), PASPC_FREE);
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
static void paspc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct paspc_env_tag* paspc_env = (struct paspc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(paspc_env->env[conidx] != NULL)
    {
        ke_free(paspc_env->env[conidx]);
        paspc_env->env[conidx] = NULL;
    }

    /* Put PASP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), PASPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the PASPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void paspc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct paspc_env_tag* paspc_env = (struct paspc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < PASPC_IDX_MAX ; idx++)
    {
        paspc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(paspc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void paspc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put PASP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), PASPC_IDLE);
}

/// PASPC Task interface required by profile manager
const struct prf_task_cbs paspc_itf =
{
        paspc_init,
        paspc_destroy,
        paspc_create,
        paspc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* paspc_prf_itf_get(void)
{
   return &paspc_itf;
}

void paspc_enable_rsp_send(struct paspc_env_tag *paspc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on PASPS
    struct paspc_enable_rsp * rsp = KE_MSG_ALLOC(
            PASPC_ENABLE_RSP,
            prf_dst_task_get(&(paspc_env->prf_env), conidx),
            prf_src_task_get(&(paspc_env->prf_env), conidx),
            paspc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->pass = paspc_env->env[conidx]->pass;
        // Register PASPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(paspc_env->prf_env), conidx, &(paspc_env->env[conidx]->pass.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(paspc_env->prf_env), conidx), PASPC_IDLE);
    }

    ke_msg_send(rsp);
}

void paspc_send_cmp_evt(struct paspc_env_tag *paspc_env, uint8_t conidx,
                        uint8_t operation, uint8_t status)
{
    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(prf_src_task_get(&(paspc_env->prf_env), conidx)) == PASPC_BUSY)
    {
        ke_state_set(prf_src_task_get(&paspc_env->prf_env, conidx), PASPC_IDLE);
    }

    // Send the message
    struct paspc_cmp_evt *evt = KE_MSG_ALLOC(PASPC_CMP_EVT,
            prf_dst_task_get(&(paspc_env->prf_env), conidx),
            prf_src_task_get(&(paspc_env->prf_env), conidx),
            paspc_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_PAS_CLIENT)

/// @} PASPC
