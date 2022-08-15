/**
 ****************************************************************************************
 * @addtogroup FINDT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_FINDME_TARGET)
#include "findt.h"
#include "findt_task.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * FINDT PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Full IAS Database Description - Used to add attributes into the database
const struct attm_desc findt_att_db[FINDT_IAS_IDX_NB] =
{
    // Immediate Alert Service Declaration
    [FINDT_IAS_IDX_SVC]             =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},
    // Alert Level Characteristic Declaration
    [FINDT_IAS_IDX_ALERT_LVL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Alert Level Characteristic Value
    [FINDT_IAS_IDX_ALERT_LVL_VAL]   =   {ATT_CHAR_ALERT_LEVEL, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the FINDT module.
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
static uint8_t findt_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task,
                            uint8_t sec_lvl, struct findt_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------

    // Service content flag
    uint32_t cfg_flag = FINDT_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    status = attm_svc_create_db(start_hdl, ATT_SVC_IMMEDIATE_ALERT, (uint8_t *)&cfg_flag,
            FINDT_IAS_IDX_NB, NULL, env->task, &findt_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE) );

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        struct findt_env_tag* findt_env =
                (struct findt_env_tag* ) ke_malloc(sizeof(struct findt_env_tag), KE_MEM_ATT_DB);

        // allocate FINDT required environment variable
        env->env = (prf_env_t*) findt_env;
        findt_env->shdl = *start_hdl;
        findt_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        findt_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);


        // initialize environment variable
        env->id                     = TASK_ID_FINDT;
        findt_task_init(&(env->desc));

        // service is ready, go into an Idle state
        ke_state_set(env->task, FINDT_IDLE);
    }
    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the FINDT module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void findt_destroy(struct prf_task_env* env)
{
    struct findt_env_tag* findt_env = (struct findt_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(findt_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void findt_create(struct prf_task_env* env, uint8_t conidx)
{
    /* nothing to do */
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
static void findt_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
   /* nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// FINDT Task interface required by profile manager
const struct prf_task_cbs findt_itf =
{
        (prf_init_fnct) findt_init,
        findt_destroy,
        findt_create,
        findt_cleanup,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* findt_prf_itf_get(void)
{
   return &findt_itf;
}


#endif //BLE_FINDME_TARGET

/// @} FINDT
