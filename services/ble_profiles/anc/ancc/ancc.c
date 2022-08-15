/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/



/**
 ****************************************************************************************
 * @addtogroup ANCC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwip_config.h"

#if (BLE_ANC_CLIENT)
#include "anc_common.h"
#include "ancc.h"
#include "ancc_task.h"
#include "ke_timer.h"
#include "ke_mem.h"

/**
 ****************************************************************************************
 * @brief Initialization of the ANCC module.
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
static uint8_t ancc_init(struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  void* params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    BLE_FUNC_ENTER();

    struct ancc_env_tag* ancc_env =
            (struct ancc_env_tag* ) ke_malloc(sizeof(struct ancc_env_tag), KE_MEM_ATT_DB);

    // allocate ANCC required environment variable
    env->env = (prf_env_t*) ancc_env;

    ancc_env->prf_env.app_task = app_task
            | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    ancc_env->prf_env.prf_task = env->task | 
		(PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));

    // initialize environment variable
    env->id                     = TASK_ID_ANCC;
    ancc_task_init(&(env->desc));

    for(idx = 0; idx < ANCC_IDX_MAX ; idx++)
    {
        ancc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), ANCC_FREE);
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
static void ancc_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    struct ancc_env_tag* ancc_env = (struct ancc_env_tag*) env->env;

    BLE_FUNC_ENTER();

    // clean-up environment variable allocated for task instance
    if(ancc_env->env[conidx] != NULL)
    {
        ke_timer_clear(ANCC_TIMEOUT_TIMER_IND, prf_src_task_get(&ancc_env->prf_env, conidx));
        ke_free(ancc_env->env[conidx]);
        ancc_env->env[conidx] = NULL;
    }

    /* Put ANC Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANCC_FREE);
}

/**
 ****************************************************************************************
 * @brief Destruction of the ANCC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void ancc_destroy(struct prf_task_env* env)
{
    uint8_t idx;
    struct ancc_env_tag* ancc_env = (struct ancc_env_tag*) env->env;

    // cleanup environment variable for each task instances
    for(idx = 0; idx < ANCC_IDX_MAX ; idx++)
    {
        ancc_cleanup(env, idx, 0);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(ancc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void ancc_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Put ANC Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), ANCC_IDLE);
}

/// ANCC Task interface required by profile manager
const struct prf_task_cbs ancc_itf =
{
        ancc_init,
        ancc_destroy,
        ancc_create,
        ancc_cleanup,
};

const struct prf_task_cbs* ancc_prf_itf_get(void)
{
   return &ancc_itf;
}

void ancc_enable_rsp_send(struct ancc_env_tag *ancc_env, uint8_t conidx, uint8_t status)
{
    BLE_FUNC_ENTER();
    
    //ASSERT(status == GAP_ERR_NO_ERROR, "%s error %d", __func__, status);
    if (status == GAP_ERR_NO_ERROR)
    {
        // Register ANCC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&(ancc_env->prf_env), conidx, &(ancc_env->env[conidx]->anc.svc));
        // Go to connected state
        ke_state_set(prf_src_task_get(&(ancc_env->prf_env), conidx), ANCC_IDLE);
    }
}

#endif //(BLE_ANC_CLIENT)

/// @} ANCC
