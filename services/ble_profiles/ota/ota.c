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



/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_OTA)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "ota.h"
#include "ota_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"
#include "ota_bes.h"
#ifdef __GATT_OVER_BR_EDR__
#include "app_btgatt.h"
#endif

/*
 * OTA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */

static const uint8_t OTA_SERVICE_UUID_128[ATT_UUID_128_LEN]    = BES_OTA_UUID_128;  

#define RD_PERM PERM(RD, ENABLE)
#define VAL_PERM PERM(WRITE_REQ, ENABLE) | PERM(NTF, ENABLE)
#define NTF_CFG_PERM PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE)

/// Full OTA SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 ota_att_db[OTA_IDX_NB] =
{
    // OTA Service Declaration
    [OTA_IDX_SVC]     =   {ATT_DECL_PRIMARY_SERVICE_UUID, RD_PERM, 0, 0},
    // OTA Characteristic Declaration
    [OTA_IDX_CHAR]    =  {ATT_DECL_CHARACTERISTIC_UUID, RD_PERM, 0, 0},
    // OTA service
    [OTA_IDX_VAL]     =  {ota_val_char_val_uuid_128_content, VAL_PERM, PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), OTA_MAX_LEN},
    // OTA Characteristic
    [OTA_IDX_NTF_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, NTF_CFG_PERM, 0, 0},

};


/**
 ****************************************************************************************
 * @brief Initialization of the OTA module.
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
static uint8_t ota_init(struct prf_task_env* env, uint16_t* start_hdl, 
    uint16_t app_task, uint8_t sec_lvl, void* params)
{
    uint8_t status;
        
    //Add Service Into Database
    status = attm_svc_create_db_128(start_hdl, OTA_SERVICE_UUID_128, NULL,
            OTA_IDX_NB, NULL, env->task, &ota_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE)
            | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));


    BLE_GATT_DBG("attm_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

#ifdef __GATT_OVER_BR_EDR__
        app_btgatt_addsdp(ATT_SERVICE_UUID, *start_hdl, *start_hdl+OTA_IDX_NB-1);
#endif

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate OTA required environment variable
        struct ota_env_tag* ota_env =
                (struct ota_env_tag* ) ke_malloc(sizeof(struct ota_env_tag), KE_MEM_ATT_DB);

        // Initialize OTA environment
        env->env           = (prf_env_t*) ota_env;
        ota_env->shdl     = *start_hdl;

        ota_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        ota_env->prf_env.prf_task    = env->task | 
            (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));

        // initialize environment variable
        env->id                     = TASK_ID_OTA;
        ota_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, OTA_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the OTA module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void ota_destroy(struct prf_task_env* env)
{
    struct ota_env_tag* ota_env = (struct ota_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(ota_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void ota_create(struct prf_task_env* env, uint8_t conidx)
{
    struct ota_env_tag* ota_env = (struct ota_env_tag*) env->env;
    struct prf_svc ota_ota_svc = {ota_env->shdl, ota_env->shdl + OTA_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &ota_ota_svc);
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
static void ota_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// OTA Task interface required by profile manager
const struct prf_task_cbs ota_itf =
{
    (prf_init_fnct) ota_init,
    ota_destroy,
    ota_create,
    ota_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* ota_prf_itf_get(void)
{
   return &ota_itf;
}


#endif /* BLE_OTA */

/// @} OTA

