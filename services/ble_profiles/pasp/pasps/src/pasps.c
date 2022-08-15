/**
 ****************************************************************************************
 * @addtogroup PASPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"
#if (BLE_PAS_SERVER)
#include "pasp_common.h"

#include "pasps_task.h"
#include "pasps.h"
#include "prf_utils.h"

#include "ke_mem.h"

/*
 * PHONE ALERT STATUS SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Full PAS Database Description - Used to add attributes into the database
const struct attm_desc pasps_att_db[PASS_IDX_NB] =
{
    // Phone Alert Status Service Declaration
    [PASS_IDX_SVC]                   =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // Alert Status Characteristic Declaration
    [PASS_IDX_ALERT_STATUS_CHAR]     =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Alert Status Characteristic Value
    [PASS_IDX_ALERT_STATUS_VAL]      =   {ATT_CHAR_ALERT_STATUS, PERM(RD, ENABLE) | PERM(NTF, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
    // Alert Status Characteristic - Client Characteristic Configuration Descriptor
    [PASS_IDX_ALERT_STATUS_CFG]      =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Ringer Setting Characteristic Declaration
    [PASS_IDX_RINGER_SETTING_CHAR]   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Ringer Settings Characteristic Value
    [PASS_IDX_RINGER_SETTING_VAL]    =   {ATT_CHAR_RINGER_SETTING, PERM(RD, ENABLE) | PERM(NTF, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
    // Ringer Settings Characteristic - Client Characteristic Configuration Descriptor
    [PASS_IDX_RINGER_SETTING_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    // Ringer Control Point Characteristic Declaration
    [PASS_IDX_RINGER_CTNL_PT_CHAR]   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Ringer Control Point Characteristic Value
    [PASS_IDX_RINGER_CTNL_PT_VAL]    =   {ATT_CHAR_RINGER_CNTL_POINT, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PASPS module.
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
static uint8_t pasps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct pasps_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    // Service content flag
    uint32_t cfg_flag= PASPS_DB_CFG_FLAG;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_PHONE_ALERT_STATUS, (uint8_t *)&cfg_flag,
            PASS_IDX_NB, NULL, env->task, &pasps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, ENABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate PASPS required environment variable
        struct pasps_env_tag* pasps_env =
                (struct pasps_env_tag* ) ke_malloc(sizeof(struct pasps_env_tag), KE_MEM_ATT_DB);

        // Initialize PASPS environment
        env->env           = (prf_env_t*) pasps_env;
        pasps_env->shdl     = *start_hdl;

        pasps_env->alert_status = params->alert_status;
        pasps_env->ringer_setting = params->ringer_setting;

        pasps_env->operation = PASPS_RESERVED_OP_CODE;

        pasps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Multi Instantiated task
        pasps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, ENABLE);

        // initialize environment variable
        env->id                     = TASK_ID_PASPS;
        pasps_task_init(&(env->desc));

        for(uint8_t idx = 0; idx < BLE_CONNECTION_MAX ; idx++)
        {
            pasps_env->env[idx] = NULL;
            /* Put PASS in disabled state */
            ke_state_set(KE_BUILD_ID(env->task, idx), PASPS_FREE);
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the PASPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void pasps_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct pasps_env_tag* pasps_env = (struct pasps_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < BLE_CONNECTION_MAX ; idx++)
    {
        if(pasps_env->env[idx] != NULL)
        {
            ke_free(pasps_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(pasps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void pasps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct pasps_env_tag* pasps_env = (struct pasps_env_tag*) env->env;
    pasps_env->env[conidx] = (struct pasps_cnx_env*)
            ke_malloc(sizeof(struct pasps_cnx_env), KE_MEM_ATT_DB);

    memset(pasps_env->env[conidx], 0, sizeof(struct pasps_cnx_env));

    pasps_env->env[conidx]->ringer_state = pasps_env->ringer_setting;
    /* Put PASS in idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), PASPS_IDLE);
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
static void pasps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct pasps_env_tag* pasps_env = (struct pasps_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(pasps_env->env[conidx] != NULL)
    {
        ke_free(pasps_env->env[conidx]);
        pasps_env->env[conidx] = NULL;
    }

    /* Put PASS in disabled state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), PASPS_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// PASPS Task interface required by profile manager
const struct prf_task_cbs pasps_itf =
{
        (prf_init_fnct) pasps_init,
        pasps_destroy,
        pasps_create,
        pasps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* pasps_prf_itf_get(void)
{
   return &pasps_itf;
}


///*
// * FUNCTION DEFINITIONS
// ****************************************************************************************
// */

void pasps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id,
                        uint8_t operation, uint8_t status)
{
    // Come back to the Connected state if the state was busy.
    if (ke_state_get(src_id) == PASPS_BUSY)
    {
        ke_state_set(src_id, PASPS_IDLE);
    }

    // Send the message to the application
    struct pasps_cmp_evt *evt = KE_MSG_ALLOC(PASPS_CMP_EVT,
                                             dest_id, src_id,
                                             pasps_cmp_evt);

    evt->operation   = operation;
    evt->status      = status;

    ke_msg_send(evt);
}

#endif //(BLE_PASP_SERVER)

/// @} PASPS
