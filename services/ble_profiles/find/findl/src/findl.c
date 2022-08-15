/**
 ****************************************************************************************
 * @addtogroup FINDL
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_FINDME_LOCATOR)
#include "findl.h"
#include "findl_task.h"
#include "prf_utils.h"
#include "gap.h"

#include "ke_mem.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the FINDL module.
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
static uint8_t findl_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task,
                            uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct findl_env_tag* findl_env =
            (struct findl_env_tag* ) ke_malloc(sizeof(struct findl_env_tag), KE_MEM_ATT_DB);

    // allocate FINDL required environment variable
    env->env = (prf_env_t*) findl_env;

    findl_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    findl_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_FINDL;
    findl_task_init(&(env->desc));

    for(idx = 0; idx < FINDL_IDX_MAX ; idx++)
    {
        findl_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), FINDL_FREE);
    }

    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Destruction of the FINDL module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void findl_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct findl_env_tag* findl_env = (struct findl_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < FINDL_IDX_MAX ; idx++)
    {
        if(findl_env->env[idx] != NULL)
        {
            ke_free(findl_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(findl_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void findl_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put FINDL in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), FINDL_IDLE);
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
static void findl_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct findl_env_tag* findl_env = (struct findl_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(findl_env->env[conidx] != NULL)
    {
        ke_free(findl_env->env[conidx]);
        findl_env->env[conidx] = NULL;
    }

    /* Put FINDL in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), FINDL_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/// FINDL Task interface required by profile manager
const struct prf_task_cbs findl_itf =
{
        (prf_init_fnct) findl_init,
        findl_destroy,
        findl_create,
        findl_cleanup,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* findl_prf_itf_get(void)
{
   return &findl_itf;
}

void findl_enable_rsp_send(struct findl_env_tag *findl_env, uint8_t conidx, uint8_t status)
{
    //send response to app
    struct findl_enable_rsp * cfm = KE_MSG_ALLOC(FINDL_ENABLE_RSP,
                                                prf_dst_task_get(&(findl_env->prf_env), conidx),
                                                prf_src_task_get(&(findl_env->prf_env), conidx),
                                                 findl_enable_rsp);

    cfm->ias = findl_env->env[conidx]->ias;
    cfm->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        // Go to idle state
        ke_state_set(prf_src_task_get(&(findl_env->prf_env), conidx), FINDL_IDLE);
    }
    else
    {
        // clean-up environment variable allocated for task instance
        ke_free(findl_env->env[conidx]);
        findl_env->env[conidx] = NULL;
    }

    ke_msg_send(cfm);
}

#endif //BLE_FINDME_LOCATOR

/// @} FINDL
