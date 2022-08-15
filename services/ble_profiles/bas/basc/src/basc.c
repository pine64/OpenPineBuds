/**
 ****************************************************************************************
 * @addtogroup BASC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_BATT_CLIENT)

#include "gap.h"
#include "basc.h"
#include "basc_task.h"
#include "co_math.h"

#include "ke_mem.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the BASC module.
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
static uint8_t basc_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct basc_env_tag* basc_env =
            (struct basc_env_tag* ) ke_malloc(sizeof(struct basc_env_tag), KE_MEM_ATT_DB);

    // allocate BASC required environment variable
    env->env = (prf_env_t*) basc_env;

    basc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    basc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_BASC;
    basc_task_init(&(env->desc));

    for(idx = 0; idx < BASC_IDX_MAX ; idx++)
    {
        basc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), BASC_FREE);
    }


    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Destruction of the BASC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void basc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct basc_env_tag* basc_env = (struct basc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < BASC_IDX_MAX ; idx++)
    {
        if(basc_env->env[idx] != NULL)
        {
            if(basc_env->env[idx]->operation != NULL)
            {
                ke_free(basc_env->env[idx]->operation);
            }
            ke_free(basc_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(basc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void basc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put BAS Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), BASC_IDLE);
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
static void basc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct basc_env_tag* basc_env = (struct basc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(basc_env->env[conidx] != NULL)
    {
        if(basc_env->env[conidx]->operation != NULL)
        {
            ke_free(basc_env->env[conidx]->operation);
        }
        ke_free(basc_env->env[conidx]);
        basc_env->env[conidx] = NULL;
    }

    /* Put BAS Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), BASC_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// BASC Task interface required by profile manager
const struct prf_task_cbs basc_itf =
{
        basc_init,
        basc_destroy,
        basc_create,
        basc_cleanup,
};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* basc_prf_itf_get(void)
{
   return &basc_itf;
}


void basc_enable_rsp_send(struct basc_env_tag *basc_env, uint8_t conidx, uint8_t status)
{
    // Counter
    uint8_t svc_inst;

    // Send APP the details of the discovered attributes on BASC
    struct basc_enable_rsp * rsp = KE_MSG_ALLOC(BASC_ENABLE_RSP,
                                                prf_dst_task_get(&(basc_env->prf_env) ,conidx),
                                                prf_src_task_get(&(basc_env->prf_env) ,conidx),
                                                basc_enable_rsp);
    rsp->status = status;
    rsp->bas_nb = 0;
    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->bas_nb = basc_env->env[conidx]->bas_nb;

        for (svc_inst = 0; svc_inst < co_min(basc_env->env[conidx]->bas_nb, BASC_NB_BAS_INSTANCES_MAX) ; svc_inst++)
        {
            rsp->bas[svc_inst] = basc_env->env[conidx]->bas[svc_inst];

            // Register BASC task in gatt for indication/notifications
            prf_register_atthdl2gatt(&(basc_env->prf_env), conidx, &basc_env->env[conidx]->bas[svc_inst].svc);
        }
    }

    ke_msg_send(rsp);
}

#endif /* (BLE_BATT_CLIENT) */

/// @} BASC
