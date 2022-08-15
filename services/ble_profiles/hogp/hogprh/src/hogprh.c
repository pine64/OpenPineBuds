/**
 ****************************************************************************************
 * @addtogroup HOGPRH
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_HID_REPORT_HOST)

#include "hogprh.h"
#include "hogprh_task.h"
#include "co_math.h"
#include "gap.h"

#include "ke_mem.h"

/**
 ****************************************************************************************
 * @brief Initialization of the HOGPRH module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of datahide (if it's a service)
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
static uint8_t hogprh_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct hogprh_env_tag* hogprh_env =
            (struct hogprh_env_tag* ) ke_malloc(sizeof(struct hogprh_env_tag), KE_MEM_ATT_DB);

    // allocate HOGPRH required environment variable
    env->env = (prf_env_t*) hogprh_env;

    hogprh_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    hogprh_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_HOGPRH;
    hogprh_task_init(&(env->desc));

    for(idx = 0; idx < HOGPRH_IDX_MAX ; idx++)
    {
        hogprh_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), HOGPRH_FREE);
    }


    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Destruction of the HOGPRH module - due to a reset for instance.
 * This function clean-up allocated memory (attribute datahide is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void hogprh_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct hogprh_env_tag* hogprh_env = (struct hogprh_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < HOGPRH_IDX_MAX ; idx++)
    {
        if(hogprh_env->env[idx] != NULL)
        {
            if(hogprh_env->env[idx]->operation != NULL)
            {
                ke_free(hogprh_env->env[idx]->operation);
            }
            ke_free(hogprh_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(hogprh_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void hogprh_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put HID Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), HOGPRH_IDLE);
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
static void hogprh_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct hogprh_env_tag* hogprh_env = (struct hogprh_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(hogprh_env->env[conidx] != NULL)
    {
        if(hogprh_env->env[conidx]->operation != NULL)
        {
            ke_free(hogprh_env->env[conidx]->operation);
        }
        ke_free(hogprh_env->env[conidx]);
        hogprh_env->env[conidx] = NULL;
    }

    /* Put HID Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), HOGPRH_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HOGPRH Task interface required by profile manager
const struct prf_task_cbs hogprh_itf =
{
        hogprh_init,
        hogprh_destroy,
        hogprh_create,
        hogprh_cleanup,
};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* hogprh_prf_itf_get(void)
{
   return &hogprh_itf;
}

void hogprh_enable_rsp_send(struct hogprh_env_tag *hogprh_env, uint8_t conidx, uint8_t status)
{
    // Counter
    uint8_t svc_inst;

    // Send APP the details of the discovered attributes on HOGPRH
    struct hogprh_enable_rsp * rsp = KE_MSG_ALLOC(HOGPRH_ENABLE_RSP,
                                                prf_dst_task_get(&(hogprh_env->prf_env) ,conidx),
                                                prf_src_task_get(&(hogprh_env->prf_env) ,conidx),
                                                hogprh_enable_rsp);
    rsp->status = status;
    rsp->hids_nb = 0;
    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->hids_nb = hogprh_env->env[conidx]->hids_nb;

        for (svc_inst = 0; svc_inst < co_min(hogprh_env->env[conidx]->hids_nb, HOGPRH_NB_HIDS_INST_MAX) ; svc_inst++)
        {
            rsp->hids[svc_inst] = hogprh_env->env[conidx]->hids[svc_inst];

            // Register HOGPRH task in gatt for indication/notifications
            prf_register_atthdl2gatt(&(hogprh_env->prf_env), conidx, &hogprh_env->env[conidx]->hids[svc_inst].svc);
        }
    }

    ke_msg_send(rsp);
}

#endif /* (BLE_HID_REPORT_HOST) */

/// @} HOGPRH
