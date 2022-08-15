/**
 ****************************************************************************************
 * @addtogroup ANPC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_AN_CLIENT)

#include "anp_common.h"
#include "anpc.h"
#include "anpc_task.h"

#include "ke_mem.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
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
static uint8_t anpc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct anpc_env_tag* anpc_env =
            (struct anpc_env_tag* ) ke_malloc(sizeof(struct anpc_env_tag), KE_MEM_ATT_DB);

    // allocate ANPC required environment variable
    env->env = (prf_env_t*) anpc_env;

    anpc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    anpc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_ANPC;
    anpc_task_init(&(env->desc));

    for(idx = 0; idx < ANPC_IDX_MAX ; idx++)
    {
        anpc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), ANPC_FREE);
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
static void anpc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct anpc_env_tag* anpc_env = (struct anpc_env_tag*) env->env;

    // clean-up environment variable allocated for task instance
    if(anpc_env->env[conidx] != NULL)
    {
        ke_free(anpc_env->env[conidx]);
        anpc_env->env[conidx] = NULL;
    }

    /* Put ANP Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANPC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the ANPC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void anpc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct anpc_env_tag* anpc_env = (struct anpc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < ANPC_IDX_MAX ; idx++)
    {
        anpc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(anpc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void anpc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put ANP Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANPC_IDLE);
}

/// ANPC Task interface required by profile manager
const struct prf_task_cbs anpc_itf =
{
        anpc_init,
        anpc_destroy,
        anpc_create,
        anpc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* anpc_prf_itf_get(void)
{
   return &anpc_itf;
}

void anpc_enable_rsp_send(struct anpc_env_tag *anpc_env, uint8_t conidx, uint8_t status)
{
    // Send to APP the details of the discovered attributes on ANPS
    struct anpc_enable_rsp * rsp = KE_MSG_ALLOC(
            ANPC_ENABLE_RSP,
            prf_dst_task_get(&(anpc_env->prf_env), conidx),
            prf_src_task_get(&(anpc_env->prf_env), conidx),
            anpc_enable_rsp);
    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->ans = anpc_env->env[conidx]->ans;
        // Register ANPC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(anpc_env->prf_env), conidx, &(anpc_env->env[conidx]->ans.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(anpc_env->prf_env), conidx), ANPC_IDLE);
    }

    ke_msg_send(rsp);
}

bool anpc_found_next_alert_cat(struct anpc_env_tag *idx_env, uint8_t conidx, struct anp_cat_id_bit_mask cat_id)
{
    // Next Category Found ?
    bool found = false;

    if (idx_env->env[conidx]->last_uuid_req < CAT_ID_HIGH_PRTY_ALERT)
    {
        // Look in the first part of the categories
        while ((idx_env->env[conidx]->last_uuid_req < CAT_ID_HIGH_PRTY_ALERT) && (!found))
        {
            if (((cat_id.cat_id_mask_0 >> idx_env->env[conidx]->last_uuid_req) & 1) == 1)
            {
                found = true;
            }

            idx_env->env[conidx]->last_uuid_req++;
        }
    }

    if (idx_env->env[conidx]->last_uuid_req >= CAT_ID_HIGH_PRTY_ALERT)
    {
        // Look in the first part of the categories
        while ((idx_env->env[conidx]->last_uuid_req < CAT_ID_NB) && (!found))
        {
            if (((cat_id.cat_id_mask_1 >> (idx_env->env[conidx]->last_uuid_req - CAT_ID_HIGH_PRTY_ALERT)) & 1) == 1)
            {
                found = true;
            }

            idx_env->env[conidx]->last_uuid_req++;
        }
    }

    return found;
}

void anpc_write_alert_ntf_ctnl_pt(struct anpc_env_tag *idx_env, uint8_t conidx, uint8_t cmd_id, uint8_t cat_id)
{
    struct anp_ctnl_pt ctnl_pt = {cmd_id, cat_id};

    // Send the write request
    prf_gatt_write(&(idx_env->prf_env), conidx,
                   idx_env->env[conidx]->ans.chars[ANPC_CHAR_ALERT_NTF_CTNL_PT].val_hdl,
                   (uint8_t *)&ctnl_pt, sizeof(struct anp_ctnl_pt), GATTC_WRITE);
}

void anpc_send_cmp_evt(struct anpc_env_tag *anpc_env, uint8_t conidx, uint8_t operation, uint8_t status)
{
    if (anpc_env->env[conidx] != NULL)
    {
        // Free the stored operation if needed
        if (anpc_env->env[conidx]->operation != NULL)
        {
            ke_msg_free(ke_param2msg(anpc_env->env[conidx]->operation));
            anpc_env->env[conidx]->operation = NULL;
        }
    }

    // Go back to the CONNECTED state if the state is busy
    if (ke_state_get(prf_src_task_get(&(anpc_env->prf_env), conidx)) == ANPC_BUSY)
    {
        ke_state_set(prf_src_task_get(&anpc_env->prf_env, conidx), ANPC_IDLE);
    }

    // Send the message
    struct anpc_cmp_evt *evt = KE_MSG_ALLOC(ANPC_CMP_EVT,
            prf_dst_task_get(&(anpc_env->prf_env), conidx),
            prf_src_task_get(&(anpc_env->prf_env), conidx),
            anpc_cmp_evt);

    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

#endif //(BLE_AN_CLIENT)

/// @} ANPC
