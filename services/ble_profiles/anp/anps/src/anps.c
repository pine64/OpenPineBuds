/**
 ****************************************************************************************
 * @addtogroup ANPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_AN_SERVER)

#include "anps.h"
#include "anps_task.h"
#include "anp_common.h"
#include "prf_utils.h"
#include "prf_types.h"

#include "ke_mem.h"

/*
 * ANS DATABASE
 ****************************************************************************************
 */

/// Full ANS Database Description - Used to add attributes into the database
const struct attm_desc anps_att_db[ANS_IDX_NB] =
{
    /// Alert Notification Service Declaration
    [ANS_IDX_SVC]                           =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    /// Supported New Alert Category Characteristic Declaration
    [ANS_IDX_SUPP_NEW_ALERT_CAT_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    /// Supported New Alert Category Characteristic Value
    [ANS_IDX_SUPP_NEW_ALERT_CAT_VAL]        =   {ATT_CHAR_SUP_NEW_ALERT_CAT, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(struct anp_cat_id_bit_mask)},

    /// New Alert Characteristic Declaration
    [ANS_IDX_NEW_ALERT_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    /// New Alert Characteristic Value
    [ANS_IDX_NEW_ALERT_VAL]                 =   {ATT_CHAR_NEW_ALERT, PERM(NTF, ENABLE), PERM(RI, ENABLE), ANS_NEW_ALERT_MAX_LEN},
    /// New Alert Characteristic - Client Char. Configuration Descriptor
    [ANS_IDX_NEW_ALERT_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    /// Supported Unread Alert Category Characteristic Declaration
    [ANS_IDX_SUPP_UNREAD_ALERT_CAT_CHAR]    =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    /// Supported New Alert Category Characteristic Value
    [ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL]     =   {ATT_CHAR_SUP_UNREAD_ALERT_CAT, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(struct anp_cat_id_bit_mask)},

    /// Unread Alert Status Characteristic Declaration
    [ANS_IDX_UNREAD_ALERT_STATUS_CHAR]      =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    /// Unread Alert Status Characteristic Value
    [ANS_IDX_UNREAD_ALERT_STATUS_VAL]       =   {ATT_CHAR_UNREAD_ALERT_STATUS, PERM(NTF, ENABLE), PERM(RI, ENABLE), sizeof(struct anp_unread_alert)},
    /// Unread Alert Status Characteristic - Client Char. Configuration Descriptor
    [ANS_IDX_UNREAD_ALERT_STATUS_CFG]       =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

    /// Alert Notification Control Point Characteristic Declaration
    [ANS_IDX_ALERT_NTF_CTNL_PT_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    /// Alert Notification Control Point Characteristic Value
    [ANS_IDX_ALERT_NTF_CTNL_PT_VAL]         =   {ATT_CHAR_ALERT_NTF_CTNL_PT, PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), 2*sizeof(uint8_t)},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ANPS module.
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
static uint8_t anps_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl, struct anps_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------
    // Service content flag
    uint32_t cfg_flag = ANPS_DB_CONFIG_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    // Add service in the database
    status = attm_svc_create_db(start_hdl, ATT_SVC_ALERT_NTF, (uint8_t *)&cfg_flag,
            ANS_IDX_NB, NULL, env->task, &anps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, ENABLE));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate ANPS required environment variable
        struct anps_env_tag* anps_env =
                (struct anps_env_tag* ) ke_malloc(sizeof(struct anps_env_tag), KE_MEM_ATT_DB);

        // Initialize ANPS environment
        env->env           = (prf_env_t*) anps_env;
        anps_env->shdl     = *start_hdl;

        memcpy(&anps_env->supp_new_alert_cat, &params->supp_new_alert_cat, sizeof(struct anp_cat_id_bit_mask));
        memcpy(&anps_env->supp_unread_alert_cat, &params->supp_unread_alert_cat, sizeof(struct anp_cat_id_bit_mask));

        anps_env->operation = ANPS_RESERVED_OP_CODE;

        anps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Multi Instantiated task
        anps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, ENABLE);

        // initialize environment variable
        env->id                     = TASK_ID_ANPS;
        anps_task_init(&(env->desc));

        for(uint8_t idx = 0; idx < BLE_CONNECTION_MAX ; idx++)
        {
            anps_env->env[idx] = NULL;
            /* Put ANS in disabled state */
            ke_state_set(KE_BUILD_ID(env->task, idx), ANPS_FREE);
        }
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the ANPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void anps_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct anps_env_tag* anps_env = (struct anps_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < BLE_CONNECTION_MAX ; idx++)
    {
        if(anps_env->env[idx] != NULL)
        {
            ke_free(anps_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(anps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void anps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct anps_env_tag* anps_env = (struct anps_env_tag*) env->env;
    anps_env->env[conidx] = (struct anps_cnx_env*)
            ke_malloc(sizeof(struct anps_cnx_env), KE_MEM_ATT_DB);

    memset(anps_env->env[conidx], 0, sizeof(struct anps_cnx_env));
    /* Put ANS in idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANPS_IDLE);

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
static void anps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct anps_env_tag* anps_env = (struct anps_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(anps_env->env[conidx] != NULL)
    {
        ke_free(anps_env->env[conidx]);
        anps_env->env[conidx] = NULL;
    }

    /* Put ANS in disabled state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANPS_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// ANPS Task interface required by profile manager
const struct prf_task_cbs anps_itf =
{
        (prf_init_fnct) anps_init,
        anps_destroy,
        anps_create,
        anps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* anps_prf_itf_get(void)
{
   return &anps_itf;
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void anps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id,
                       uint8_t operation, uint8_t status)
{
    // Come back to the Connected state if the state was busy.
    if (ke_state_get(src_id) == ANPS_BUSY)
    {
        ke_state_set(src_id, ANPS_IDLE);
    }

    // Send the message to the application
    struct anps_cmp_evt *evt = KE_MSG_ALLOC(ANPS_CMP_EVT,
                                            dest_id, src_id,
                                            anps_cmp_evt);

    evt->operation   = operation;
    evt->status      = status;

    ke_msg_send(evt);
}

void anps_send_ntf_status_update_ind(uint8_t conidx, struct anps_env_tag *idx_env, uint8_t alert_type)
{
    // Send the message to the application
    struct anps_ntf_status_update_ind *ind = KE_MSG_ALLOC(ANPS_NTF_STATUS_UPDATE_IND,
            prf_dst_task_get(&idx_env->prf_env, conidx),
            prf_src_task_get(&idx_env->prf_env, conidx),
            anps_ntf_status_update_ind);

    ind->alert_type  = alert_type;
    ind->ntf_ccc_cfg = ANPS_IS_ALERT_ENABLED(conidx, idx_env, alert_type) ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    if (alert_type == ANP_NEW_ALERT)
    {
        ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(idx_env->env[conidx]->ntf_new_alert_cfg & 0x00FF);
        ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)((idx_env->env[conidx]->ntf_new_alert_cfg & 0xFF00) >> 8);
    }
    else
    {
        ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(idx_env->env[conidx]->ntf_unread_alert_cfg & 0x00FF);
        ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)(idx_env->env[conidx]->ntf_unread_alert_cfg >> 8);
    }

    ke_msg_send(ind);
}

void anps_send_ntf_immediate_req_ind(uint8_t conidx, struct anps_env_tag *idx_env, uint8_t alert_type,
                                     uint8_t category_id)
{
    uint16_t req_cat;

    // Send the message to the application
    struct anps_ntf_immediate_req_ind *ind = KE_MSG_ALLOC(ANPS_NTF_IMMEDIATE_REQ_IND,
            prf_dst_task_get(&idx_env->prf_env, conidx),
            prf_src_task_get(&idx_env->prf_env, conidx),
            anps_ntf_immediate_req_ind);

    ind->alert_type  = alert_type;

    if (alert_type == ANP_NEW_ALERT)
    {
        if (category_id == CAT_ID_ALL_SUPPORTED_CAT)
        {
            // All category that are supported and enabled shall be notified
            req_cat = idx_env->env[conidx]->ntf_new_alert_cfg;
        }
        else
        {
            req_cat = (1 << category_id);
        }
    }
    else        // Unread alert
    {
        if (category_id == CAT_ID_ALL_SUPPORTED_CAT)
        {
            // All category that are supported and enabled shall be notified
            req_cat = idx_env->env[conidx]->ntf_unread_alert_cfg;
        }
        else
        {
            req_cat = (1 << category_id);
        }
    }

    ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(req_cat & 0x00FF);
    ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)((req_cat & 0xFF00) >> 8);

    ke_msg_send(ind);
}

#endif //(BLE_AN_SERVER)

/// @} ANPS
