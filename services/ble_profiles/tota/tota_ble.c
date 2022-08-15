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

#if (BLE_TOTA)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "tota_ble.h"
#include "tota_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include "co_utils.h"


/*
 * TOTA CMD PROFILE ATTRIBUTES
 ****************************************************************************************
 */
#define tota_service_uuid_128_content  			{0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86, 0x86 }
#define tota_val_char_val_uuid_128_content  		{0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97 }	

#define ATT_DECL_PRIMARY_SERVICE_UUID		{ 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID		{ 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID		{ 0x02, 0x29 }

static const uint8_t TOTA_SERVICE_UUID_128[ATT_UUID_128_LEN]	= tota_service_uuid_128_content;  

/// Full OTA SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 tota_att_db[TOTA_IDX_NB] =
{
    // TOTA Service Declaration
    [TOTA_IDX_SVC] 	=   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},
	// TOTA Characteristic Declaration
	[TOTA_IDX_CHAR]    =  {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
	// TOTA service
    [TOTA_IDX_VAL]     =  {tota_val_char_val_uuid_128_content, PERM(WRITE_REQ, ENABLE) | PERM(NTF, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), TOTA_MAX_LEN},
	// TOTA Characteristic
	[TOTA_IDX_NTF_CFG]	=   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},

};


/**
 ****************************************************************************************
 * @brief Initialization of the TOTA module.
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
static uint8_t tota_init(struct prf_task_env* env, uint16_t* start_hdl, 
	uint16_t app_task, uint8_t sec_lvl, void* params)
{
	uint8_t status;
    //Add Service Into Database
    status = attm_svc_create_db_128(start_hdl, TOTA_SERVICE_UUID_128, NULL,
            TOTA_IDX_NB, NULL, env->task, &tota_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE)
            | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));


	BLE_GATT_DBG("attm_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate TOTA required environment variable
        struct tota_env_tag* tota_env =
                (struct tota_env_tag* ) ke_malloc(sizeof(struct tota_env_tag), KE_MEM_ATT_DB);

        // Initialize TOTA environment
        env->env           = (prf_env_t*) tota_env;
        tota_env->shdl     = *start_hdl;

        tota_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        tota_env->prf_env.prf_task    = env->task | 
        	(PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));

        // initialize environment variable
        env->id                     = TASK_ID_TOTA;
        tota_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, TOTA_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the TOTA module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void tota_destroy(struct prf_task_env* env)
{
    struct tota_env_tag* tota_env = (struct tota_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(tota_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void tota_create(struct prf_task_env* env, uint8_t conidx)
{
	struct tota_env_tag* tota_env = (struct tota_env_tag*) env->env;
	struct prf_svc tota_tota_svc = {tota_env->shdl, tota_env->shdl + TOTA_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &tota_tota_svc);

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
static void tota_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// TOTA Task interface required by profile manager
const struct prf_task_cbs tota_itf =
{
    (prf_init_fnct) tota_init,
    tota_destroy,
    tota_create,
    tota_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* tota_prf_itf_get(void)
{
   return &tota_itf;
}


#endif /* BLE_TOTA */
