/**
 ****************************************************************************************
 * @addtogroup PROXR
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROX_REPORTER)

#include "proxr.h"
#include "proxr_task.h"
#include "attm.h"
#include "gap.h"

#include "ke_mem.h"

/*
 * PROXIMITY PROFILE ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Full LLS Database Description - Used to add attributes into the database
const struct attm_desc proxr_lls_att_db[LLS_IDX_NB] =
{
    // Link Loss Service Declaration
    [LLS_IDX_SVC]                   =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},
    // Alert Level Characteristic Declaration
    [LLS_IDX_ALERT_LVL_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Alert Level Characteristic Value
    [LLS_IDX_ALERT_LVL_VAL]         =   {ATT_CHAR_ALERT_LEVEL, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};

/// Full IAS Database Description - Used to add attributes into the database
const struct attm_desc proxr_ias_att_db[IAS_IDX_NB] =
{
    // Immediate Alert Service Declaration
    [IAS_IDX_SVC]                   =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE),0, 0},
    // Alert Level Characteristic Declaration
    [IAS_IDX_ALERT_LVL_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Alert Level Characteristic Value
    [IAS_IDX_ALERT_LVL_VAL]         =   {ATT_CHAR_ALERT_LEVEL, PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};

/// Full TXPS Database Description - Used to add attributes into the database
const struct attm_desc proxr_txps_att_db[TXPS_IDX_NB] =
{
    // TX Power Service Declaration
    [TXPS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},
    // TX Power Level Characteristic Declaration
    [TXPS_IDX_TX_POWER_LVL_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // TX Power Level Characteristic Value
    [TXPS_IDX_TX_POWER_LVL_VAL]     =   {ATT_CHAR_TX_POWER_LEVEL, PERM(RD, ENABLE), PERM(RI, ENABLE), sizeof(uint8_t)},
};
/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static uint8_t proxr_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task,
                            uint8_t sec_lvl, struct proxr_db_cfg* params)
{
    // Service content flag
    uint32_t cfg_flag = PROXR_LLS_MANDATORY_MASK;
    // DB Creation Status
    uint8_t status = ATT_ERR_NO_ERROR;

    //-------------------- allocate memory required for the profile  ---------------------
    struct proxr_env_tag* proxr_env =
            (struct proxr_env_tag* ) ke_malloc(sizeof(struct proxr_env_tag), KE_MEM_ATT_DB);

    // allocate PROXR required environment variable
    env->env = (prf_env_t*) proxr_env;

    status = attm_svc_create_db(start_hdl, ATT_SVC_LINK_LOSS, (uint8_t *)&cfg_flag,
               LLS_IDX_NB, NULL, env->task, &proxr_lls_att_db[0],
              (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
              | PERM(SVC_MI, DISABLE) );

    proxr_env->lls_start_hdl     = *start_hdl;
    *start_hdl += LLS_IDX_NB;

    if( (status == ATT_ERR_NO_ERROR) && (params->features == PROXR_IAS_TXPS_SUP) )
    {
        cfg_flag = PROXR_IAS_MANDATORY_MASK;
        status = attm_svc_create_db(start_hdl, ATT_SVC_IMMEDIATE_ALERT, (uint8_t *)&cfg_flag,
                    IAS_IDX_NB, NULL, env->task, &proxr_ias_att_db[0],
                   (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
                   | PERM(SVC_MI, DISABLE) );

       proxr_env->ias_start_hdl     = *start_hdl;
       *start_hdl += IAS_IDX_NB;

       if( status == ATT_ERR_NO_ERROR )
       {
           cfg_flag = PROXR_TXP_MANDATORY_MASK;
           status = attm_svc_create_db(start_hdl, ATT_SVC_TX_POWER, (uint8_t *)&cfg_flag,
                      TXPS_IDX_NB, NULL, env->task, &proxr_txps_att_db[0],
                     (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))
                     | PERM(SVC_MI, DISABLE) );

           proxr_env->txp_start_hdl     = *start_hdl;

           if (status == ATT_ERR_NO_ERROR)
           {
               // set start handle to first allocated service value
               *start_hdl = proxr_env->ias_start_hdl;

               proxr_env->features          = params->features;
               proxr_env->prf_env.app_task  = app_task
                       | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
               proxr_env->prf_env.prf_task  = env->task | PERM(PRF_MI, DISABLE);

               // initialize environment variable
               env->id                     = TASK_ID_PROXR;
               proxr_task_init(&(env->desc));

               // service is ready, go into an Idle state
               ke_state_set(env->task, PROXR_IDLE);
           }
       }
    }
    return (status);
}

static void proxr_destroy(struct prf_task_env* env)
{
    struct proxr_env_tag* proxr_env = (struct proxr_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(proxr_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void proxr_create(struct prf_task_env* env, uint8_t conidx)
{
    struct proxr_env_tag* proxr_env = (struct proxr_env_tag*) env->env;

    proxr_env->lls_alert_lvl[conidx] = PROXR_ALERT_NONE;
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
static void proxr_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct proxr_env_tag* proxr_env = (struct proxr_env_tag*) env->env;

    if((proxr_env->features == PROXR_IAS_TXPS_SUP) && (reason != LL_ERR_REMOTE_USER_TERM_CON))
    {
        if(proxr_env->lls_alert_lvl[conidx] > PROXR_ALERT_NONE)
        {

            // Allocate the alert value change indication
            struct proxr_alert_ind *ind = KE_MSG_ALLOC(PROXR_ALERT_IND,
                    prf_dst_task_get(&(proxr_env->prf_env), conidx),
                    prf_src_task_get(&(proxr_env->prf_env), conidx), proxr_alert_ind);

            // Fill in the parameter structure
            ind->alert_lvl = proxr_env->lls_alert_lvl[conidx];
            ind->char_code = PROXR_LLS_CHAR;
            ind->conidx = conidx;

            // Send the message
            ke_msg_send(ind);
        }
    }

}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */


/// PROXR Task interface required by profile manager
const struct prf_task_cbs proxr_itf =
{
    (prf_init_fnct) proxr_init,
    proxr_destroy,
    proxr_create,
    proxr_cleanup,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* proxr_prf_itf_get(void)
{
   return &proxr_itf;
}


#endif //BLE_PROX_REPORTER

/// @} PROXR
