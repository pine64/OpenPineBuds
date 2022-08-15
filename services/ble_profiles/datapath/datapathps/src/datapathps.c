/**
 ****************************************************************************************
 * @addtogroup DPPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_DATAPATH_SERVER)
#include "gap.h"
#include "gattc_task.h"
#include "attm.h"
#include "gapc_task.h"
#include "datapathps.h"
#include "datapathps_task.h"
#include "prf_utils.h"

#include "ke_mem.h"
#include "co_utils.h"

/*
 * DATAPATH SRVER PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define USE_128BIT_UUID 1

#if USE_128BIT_UUID

#define datapath_service_uuid_128_content       {0x12, 0x34, 0x56, 0x78, 0x90, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01 }
#define datapath_tx_char_val_uuid_128_content   {0x12, 0x34, 0x56, 0x78, 0x91, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02 }
#define datapath_rx_char_val_uuid_128_content   {0x12, 0x34, 0x56, 0x78, 0x92, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03 }

#define ATT_DECL_PRIMARY_SERVICE_UUID       { 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID       { 0x02, 0x29 }
#define ATT_DESC_CHAR_USER_DESCRIPTION_UUID { 0x01, 0x29 }


static const uint8_t DATAPATH_SERVICE_UUID_128[ATT_UUID_128_LEN] = datapath_service_uuid_128_content;

/// Full DATAPATH SERVER Database Description - Used to add attributes into the database
const struct attm_desc_128 datapathps_att_db[DATAPATHPS_IDX_NB] =
{
    // Service Declaration
    [DATAPATHPS_IDX_SVC]        =   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},

    // TX Characteristic Declaration
    [DATAPATHPS_IDX_TX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // TX Characteristic Value
    [DATAPATHPS_IDX_TX_VAL]     =   {datapath_tx_char_val_uuid_128_content, PERM(NTF, ENABLE) | PERM(RD, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), DATAPATHPS_MAX_LEN},
    // TX Characteristic - Client Characteristic Configuration Descriptor
    [DATAPATHPS_IDX_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
    // TX Characteristic - Characteristic User Description Descriptor
    [DATAPATHPS_IDX_TX_DESC]    =   {ATT_DESC_CHAR_USER_DESCRIPTION_UUID, PERM(RD, ENABLE), PERM(RI, ENABLE), 32},

    // RX Characteristic Declaration
    [DATAPATHPS_IDX_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    // RX Characteristic Value
    [DATAPATHPS_IDX_RX_VAL]     =   {datapath_rx_char_val_uuid_128_content, PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), DATAPATHPS_MAX_LEN},
    // RX Characteristic - Characteristic User Description Descriptor
    [DATAPATHPS_IDX_RX_DESC]    =   {ATT_DESC_CHAR_USER_DESCRIPTION_UUID, PERM(RD, ENABLE), PERM(RI, ENABLE), 32},
};
#else

#define DATAPATHPS_SERVICE_UUID_16BIT       0xFEF8
#define DATAPATHPS_TX_CHAR_VAL_UUID_16BIT   0xFEF9
#define DATAPATHPS_RX_CHAR_VAL_UUID_16BIT   0xFEFA

const struct attm_desc datapathps_att_db[DATAPATHPS_IDX_NB] =
{
    // Service Declaration
    [DATAPATHPS_IDX_SVC]        =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // TX Characteristic Declaration
    [DATAPATHPS_IDX_TX_CHAR]    =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // TX Characteristic Value
    [DATAPATHPS_IDX_TX_VAL]     =   {DATAPATHPS_TX_CHAR_VAL_UUID_16BIT, PERM(NTF, ENABLE), PERM(RI, ENABLE), DATAPATHPS_MAX_LEN},
    // TX Characteristic - Client Characteristic Configuration Descriptor
    [DATAPATHPS_IDX_TX_NTF_CFG] =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, 0},
    // TX Characteristic - Characteristic User Description Descriptor
    [DATAPATHPS_IDX_TX_DESC]    =   {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), PERM(RI, ENABLE), 32},

    // RX Characteristic Declaration
    [DATAPATHPS_IDX_RX_CHAR]    =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // RX Characteristic Value
    [DATAPATHPS_IDX_RX_VAL]     =   {DATAPATHPS_RX_CHAR_VAL_UUID_16BIT, PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), PERM(RI, ENABLE), DATAPATHPS_MAX_LEN},
    // RX Characteristic - Characteristic User Description Descriptor
    [DATAPATHPS_IDX_RX_DESC]    =   {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), PERM(RI, ENABLE), 32},
};

#endif
/**
 ****************************************************************************************
 * @brief Initialization of the DATAPATHPS module.
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
static uint8_t datapathps_init(struct prf_task_env* env, uint16_t* start_hdl, 
    uint16_t app_task, uint8_t sec_lvl, void* params)
{
    uint8_t status;

    //Add Service Into Database
#if USE_128BIT_UUID

    status = attm_svc_create_db_128(start_hdl, DATAPATH_SERVICE_UUID_128, NULL,
            DATAPATHPS_IDX_NB, NULL, env->task, &datapathps_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS))| PERM(SVC_MI, DISABLE)
            | PERM_VAL(SVC_UUID_LEN, PERM_UUID_128));
#else
    status = attm_svc_create_db(start_hdl, DATAPATHPS_SERVICE_UUID_16BIT, NULL,
           DATAPATHPS_IDX_NB, NULL, env->task, &datapathps_att_db[0],
          (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)) | PERM(SVC_MI, DISABLE) );
#endif

    BLE_GATT_DBG("attm_svc_create_db_128 returns %d start handle is %d", status, *start_hdl);

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        // Allocate DATAPATHPS required environment variable
        struct datapathps_env_tag* datapathps_env =
                (struct datapathps_env_tag* ) ke_malloc(sizeof(struct datapathps_env_tag), KE_MEM_ATT_DB);

        memset((uint8_t *)datapathps_env, 0, sizeof(struct datapathps_env_tag));
        // Initialize DATAPATHPS environment
        env->env           = (prf_env_t*) datapathps_env;
        datapathps_env->shdl     = *start_hdl;

        datapathps_env->prf_env.app_task    = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        // Mono Instantiated task
        datapathps_env->prf_env.prf_task    = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_DATAPATHPS;
        datapathps_task_init(&(env->desc));

        /* Put HRS in Idle state */
        ke_state_set(env->task, DATAPATHPS_IDLE);
    }

    return (status);
}

/**
 ****************************************************************************************
 * @brief Destruction of the DATAPATHPS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void datapathps_destroy(struct prf_task_env* env)
{
    struct datapathps_env_tag* datapathps_env = (struct datapathps_env_tag*) env->env;

    // free profile environment variables
    env->env = NULL;
    ke_free(datapathps_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void datapathps_create(struct prf_task_env* env, uint8_t conidx)
{
    struct datapathps_env_tag* datapathps_env = (struct datapathps_env_tag*) env->env;
    struct prf_svc datapathps_svc = {datapathps_env->shdl, datapathps_env->shdl + DATAPATHPS_IDX_NB};
    prf_register_atthdl2gatt(env->env, conidx, &datapathps_svc);
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
static void datapathps_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// DATAPATHPS Task interface required by profile manager
const struct prf_task_cbs datapathps_itf =
{
    (prf_init_fnct) datapathps_init,
    datapathps_destroy,
    datapathps_create,
    datapathps_cleanup,
};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* datapathps_prf_itf_get(void)
{
   return &datapathps_itf;
}


#endif /* BLE_DATAPATH_SERVER */

/// @} DATAPATHPS
