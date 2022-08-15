/**
 ****************************************************************************************
 * @addtogroup GFPSP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_GFPS_PROVIDER)
#include "attm.h"
#include "gfps_provider.h"
#include "gfps_provider_task.h"
#include "gfps_crypto.h"
#include "prf_utils.h"
#include "prf.h"

#include "ke_mem.h"

#include "nvrecord_fp_account_key.h"

/*
 * MACROS
 ****************************************************************************************
 */

/// Maximal length for Characteristic values - 128 bytes
#define GFPSP_VAL_MAX_LEN                         (128)
///System ID string length
#define GFPSP_SYS_ID_LEN                          (0x08)
///IEEE Certif length (min 6 bytes)
#define GFPSP_IEEE_CERTIF_MIN_LEN                 (0x06)
///PnP ID length
#define GFPSP_PNP_ID_LEN                          (0x07)

#define GFPS_USE_128BIT_UUIDx

#ifdef GFPS_USE_128BIT_UUID

static const uint8_t ATT_SVC_GOOGLE_FAST_PAIR_PROVIDER[ATT_UUID_16_LEN] = {0x2C, 0xFE};

/*------------------- UNITS ---------------------*/

/*---------------- DECLARATIONS -----------------*/
#define ATT_DECL_PRIMARY_SERVICE_UUID       { 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID        { 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID       { 0x02, 0x29 }

/*----------------- DESCRIPTORS -----------------*/
/*--------------- CHARACTERISTICS ---------------*/
#define ATT_CHAR_KEY_BASED_PAIRING                  {0xEA, 0x0B, 0x10, 0x32, 0xDE, 0x01, 0xB0, 0x8E, 0x14, 0x48, 0x66, 0x83, 0x34, 0x12, 0x2C, 0xFE}
#define ATT_CHAR_PASSKEY                            {0xEA, 0x0B, 0x10, 0x32, 0xDE, 0x01, 0xB0, 0x8E, 0x14, 0x48, 0x66, 0x83, 0x35, 0x12, 0x2C, 0xFE}
#define ATT_CHAR_ACCOUNTKEY                         {0xEA, 0x0B, 0x10, 0x32, 0xDE, 0x01, 0xB0, 0x8E, 0x14, 0x48, 0x66, 0x83, 0x36, 0x12, 0x2C, 0xFE}
#define ATT_CHAR_NAME                               {0xEA, 0x0B, 0x10, 0x32, 0xDE, 0x01, 0xB0, 0x8E, 0x14, 0x48, 0x66, 0x83, 0x37, 0x12, 0x2C, 0xFE}

/*
 * DIS ATTRIBUTES
 ****************************************************************************************
 */ 
 
const struct attm_desc_128 gfpsp_att_db[GFPSP_IDX_NB] =
{
    // Google fast pair service provider Declaration
    [GFPSP_IDX_SVC]                             =   {ATT_DECL_PRIMARY_SERVICE_UUID, PERM(RD, ENABLE), 0, 0},

    [GFPSP_IDX_KEY_BASED_PAIRING_CHAR]          =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    [GFPSP_IDX_KEY_BASED_PAIRING_VAL]           =   {ATT_CHAR_KEY_BASED_PAIRING, PERM(NTF, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG]       =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},
    
    
    [GFPSP_IDX_PASSKEY_CHAR]                    =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE), 0, 0},
    [GFPSP_IDX_PASSKEY_VAL]                     =   {ATT_CHAR_PASSKEY, PERM(NTF, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_PASSKEY_NTF_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},
    
    [GFPSP_IDX_ACCOUNT_KEY_CHAR]                =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_ACCOUNT_KEY_VAL]                 =   {ATT_CHAR_ACCOUNTKEY,   PERM(NTF, ENABLE) |PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_ACCOUNT_KEY_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},

    [GFPSP_IDX_NAME_CHAR]                       =   {ATT_DECL_CHARACTERISTIC_UUID, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_NAME_VAL]                        =   {ATT_CHAR_NAME,   PERM(NTF, ENABLE) |PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE) | PERM_VAL(UUID_LEN, PERM_UUID_128), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_NAME_CFG]                        =   {ATT_DESC_CLIENT_CHAR_CFG_UUID, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},

};
#else
enum {

/*----------------- SERVICES ---------------------*/
/// 
ATT_SVC_GOOGLE_FAST_PAIR_PROVIDER                      = ATT_UUID_16(0xFE2C),
/*------------------- UNITS ---------------------*/

/*---------------- DECLARATIONS -----------------*/
/*----------------- DESCRIPTORS -----------------*/
/*--------------- CHARACTERISTICS ---------------*/
ATT_CHAR_KEY_BASED_PAIRING    = ATT_UUID_16(0x1234),
ATT_CHAR_PASSKEY              = ATT_UUID_16(0x1235),
ATT_CHAR_ACCOUNTKEY           = ATT_UUID_16(0x1236),
ATT_CHAR_NAME                 = ATT_UUID_16(0x1237),
};
/*
 * DIS ATTRIBUTES
 ****************************************************************************************
 */ 
 
const struct attm_desc gfpsp_att_db[GFPSP_IDX_NB] =
{
    // Google fast pair service provider Declaration
    [GFPSP_IDX_SVC]                             =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    [GFPSP_IDX_KEY_BASED_PAIRING_CHAR]          =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    [GFPSP_IDX_KEY_BASED_PAIRING_VAL]           =   {ATT_CHAR_KEY_BASED_PAIRING, PERM(NTF, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_KEY_BASED_PAIRING_NTF_CFG]       =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},
    
    
    [GFPSP_IDX_PASSKEY_CHAR]                    =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    [GFPSP_IDX_PASSKEY_VAL]                     =   {ATT_CHAR_PASSKEY, PERM(NTF, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_PASSKEY_NTF_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), sizeof(uint16_t)},
    
    [GFPSP_IDX_ACCOUNT_KEY_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_ACCOUNT_KEY_VAL]                 =   {ATT_CHAR_ACCOUNTKEY,   PERM(NTF, ENABLE) |PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_ACCOUNT_KEY_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},

    [GFPSP_IDX_NAME_CHAR]                       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE)|PERM(WRITE_REQ, ENABLE), 0, GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_NAME_VAL]                        =   {ATT_CHAR_NAME,   PERM(NTF, ENABLE) |PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},
    [GFPSP_IDX_NAME_CFG]                        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), PERM(RI, ENABLE), GFPSP_VAL_MAX_LEN},

};
#endif

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GFPSP module.
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
static uint8_t gfpsp_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct gfpsp_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------

    // Service content flag
    uint32_t cfg_flag;
    // DB Creation Statis
    uint8_t status = ATT_ERR_NO_ERROR;


    // Compute Attribute Table and save it in environment
    cfg_flag = gfpsp_compute_cfg_flag(params->features);

#ifdef GFPS_USE_128BIT_UUID
    status = attm_svc_create_db_128(start_hdl, ATT_SVC_GOOGLE_FAST_PAIR_PROVIDER, (uint8_t *)&cfg_flag,
            GFPSP_IDX_NB, NULL, env->task, &gfpsp_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));
#else
    status = attm_svc_create_db(start_hdl, ATT_SVC_GOOGLE_FAST_PAIR_PROVIDER, (uint8_t *)&cfg_flag,
            GFPSP_IDX_NB, NULL, env->task, &gfpsp_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));
#endif

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        struct gfpsp_env_tag* gfpsp_env =
                (struct gfpsp_env_tag* ) ke_malloc(sizeof(struct gfpsp_env_tag), KE_MEM_ATT_DB);

        // allocate GFPSP required environment variable
        env->env = (prf_env_t*) gfpsp_env;
        gfpsp_env->start_hdl = *start_hdl;
        gfpsp_env->features  = params->features;
        gfpsp_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        gfpsp_env->prf_env.prf_task = env->task | 
            (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));

        // initialize environment variable
        env->id                     = TASK_ID_GFPSP;
        gfpsp_task_init(&(env->desc));
        co_list_init(&(gfpsp_env->values));

        // service is ready, go into an Idle state
        ke_state_set(env->task, GFPSP_IDLE);
    }

    return (status);
}
/**
 ****************************************************************************************
 * @brief Destruction of the GFPSP module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void gfpsp_destroy(struct prf_task_env* env)
{
    struct gfpsp_env_tag* gfpsp_env = (struct gfpsp_env_tag*) env->env;

    // remove all values present in list
    while(!co_list_is_empty(&(gfpsp_env->values)))
    {
        struct co_list_hdr* hdr = co_list_pop_front(&(gfpsp_env->values));
        ke_free(hdr);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(gfpsp_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void gfpsp_create(struct prf_task_env* env, uint8_t conidx)
{
    /* Nothing to do */
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
static void gfpsp_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// GFPSP Task interface required by profile manager
const struct prf_task_cbs gfpsp_itf =
{
        (prf_init_fnct) gfpsp_init,
        gfpsp_destroy,
        gfpsp_create,
        gfpsp_cleanup,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* gfpsp_prf_itf_get(void)
{
   return &gfpsp_itf;
}

uint32_t gfpsp_compute_cfg_flag(uint16_t features)
{
    //Service Declaration
    uint32_t cfg_flag = 1;

    for (uint8_t i = 0; i<GFPSP_CHAR_MAX; i++)
    {
        if (((features >> i) & 1) == 1)
        {
            cfg_flag |= (3 << (i*2 + 1));
        }
    }

    return cfg_flag;
}


uint8_t gfpsp_handle_to_value(struct gfpsp_env_tag* env, uint16_t handle)
{
    uint8_t value = GFPSP_CHAR_MAX;

    // handle cursor, start from first characteristic of service handle
    uint16_t cur_hdl = env->start_hdl + 1;

    for (uint8_t i = 0; i<GFPSP_CHAR_MAX; i++)
    {
        if (((env->features >> i) & 1) == 1)
        {
            // check if value handle correspond to requested handle
            if((cur_hdl +1) == handle)
            {
                value = i;
                break;
            }
            cur_hdl += 2;
        }
    }

    return value;
}

uint16_t gfpsp_value_to_handle(struct gfpsp_env_tag* env, uint8_t value)
{
    uint16_t handle = env->start_hdl + 1;
    int8_t i;

    for (i = 0; i<GFPSP_CHAR_MAX; i++)
    {
        if (((env->features >> i) & 1) == 1)
        {
            // requested value
            if(value == i)
            {
                handle += 1;
                break;
            }
            handle += 2;
        }
    }

    // check if handle found
    return ((i == GFPSP_CHAR_MAX) ? ATT_INVALID_HDL : handle);
}

uint8_t gfpsp_check_val_len(uint8_t char_code, uint8_t val_len)
{
    uint8_t status = GAP_ERR_NO_ERROR;

    // Check if length is upper than the general maximal length
    if (val_len > GFPSP_VAL_MAX_LEN)
    {
        status = PRF_ERR_UNEXPECTED_LEN;
    }
    else
    {
        // Check if length matches particular requirements
        switch (char_code)
        {
            case GFPSP_SYSTEM_ID_CHAR:
                if (val_len != GFPSP_SYS_ID_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case GFPSP_IEEE_CHAR:
                if (val_len < GFPSP_IEEE_CERTIF_MIN_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case GFPSP_PNP_ID_CHAR:
                if (val_len != GFPSP_PNP_ID_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            default:
                break;
        }
    }

    return (status);
}


uint32_t gfpsp_crypto_gen_DHKey(const uint8_t *in_PubKey,const uint8_t *in_PrivateKey,uint8_t *out_DHKey)
{
    return gfps_crypto_gen_DHKey(in_PubKey,in_PrivateKey,out_DHKey);
}

uint32_t gfpsp_crypto_make_P256_key(uint8_t * out_public_key,uint8_t * out_private_key)
{
    return gfps_crypto_make_P256_key(out_public_key,out_private_key);
}

#else

uint32_t gfpsp_crypto_gen_DHKey(const uint8_t *in_PubKey,const uint8_t *in_PrivateKey,uint8_t *out_DHKey)
{
    return 1;
}

uint32_t gfpsp_crypto_make_P256_key(uint8_t * out_public_key,uint8_t * out_private_key)
{
    return 1;
}

#endif //BLE_GFPSP_SERVER

/// @} GFPSP
