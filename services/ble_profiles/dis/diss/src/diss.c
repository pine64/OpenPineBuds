/**
 ****************************************************************************************
 * @addtogroup DISS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_DIS_SERVER)
#include "attm.h"
#include "diss.h"
#include "diss_task.h"
#include "prf_utils.h"
#include "prf.h"

#include "ke_mem.h"

/*
 * MACROS
 ****************************************************************************************
 */

/// Maximal length for Characteristic values - 128 bytes
#define DIS_VAL_MAX_LEN                         (128)
///System ID string length
#define DIS_SYS_ID_LEN                          (0x08)
///IEEE Certif length (min 6 bytes)
#define DIS_IEEE_CERTIF_MIN_LEN                 (0x06)
///PnP ID length
#define DIS_PNP_ID_LEN                          (0x07)


/*
 * DIS ATTRIBUTES
 ****************************************************************************************
 */

/// Full DIS Database Description - Used to add attributes into the database
const struct attm_desc diss_att_db[DIS_IDX_NB] =
{
    // Device Information Service Declaration
    [DIS_IDX_SVC]                           =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), 0, 0},

    // Manufacturer Name Characteristic Declaration
    [DIS_IDX_MANUFACTURER_NAME_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Manufacturer Name Characteristic Value
    [DIS_IDX_MANUFACTURER_NAME_VAL]         =   {ATT_CHAR_MANUF_NAME, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // Model Number String Characteristic Declaration
    [DIS_IDX_MODEL_NB_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Model Number String Characteristic Value
    [DIS_IDX_MODEL_NB_STR_VAL]              =   {ATT_CHAR_MODEL_NB, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // Serial Number String Characteristic Declaration
    [DIS_IDX_SERIAL_NB_STR_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Serial Number String Characteristic Value
    [DIS_IDX_SERIAL_NB_STR_VAL]             =   {ATT_CHAR_SERIAL_NB, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // Hardware Revision String Characteristic Declaration
    [DIS_IDX_HARD_REV_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Hardware Revision String Characteristic Value
    [DIS_IDX_HARD_REV_STR_VAL]              =   {ATT_CHAR_HW_REV, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // Firmware Revision String Characteristic Declaration
    [DIS_IDX_FIRM_REV_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Firmware Revision String Characteristic Value
    [DIS_IDX_FIRM_REV_STR_VAL]              =   {ATT_CHAR_FW_REV, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // Software Revision String Characteristic Declaration
    [DIS_IDX_SW_REV_STR_CHAR]               =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // Software Revision String Characteristic Value
    [DIS_IDX_SW_REV_STR_VAL]                =   {ATT_CHAR_SW_REV, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_VAL_MAX_LEN},

    // System ID Characteristic Declaration
    [DIS_IDX_SYSTEM_ID_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // System ID Characteristic Value
    [DIS_IDX_SYSTEM_ID_VAL]                 =   {ATT_CHAR_SYS_ID, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_SYS_ID_LEN},

    // IEEE 11073-20601 Regulatory Certification Data List Characteristic Declaration
    [DIS_IDX_IEEE_CHAR]                     =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // IEEE 11073-20601 Regulatory Certification Data List Characteristic Value
    [DIS_IDX_IEEE_VAL]                      =   {ATT_CHAR_IEEE_CERTIF, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_SYS_ID_LEN},

    // PnP ID Characteristic Declaration
    [DIS_IDX_PNP_ID_CHAR]                   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), 0, 0},
    // PnP ID Characteristic Value
    [DIS_IDX_PNP_ID_VAL]                    =   {ATT_CHAR_PNP_ID, PERM(RD, ENABLE), PERM(RI, ENABLE), DIS_PNP_ID_LEN},
};



/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the DISS module.
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
static uint8_t diss_init (struct prf_task_env* env, uint16_t* start_hdl, uint16_t app_task, uint8_t sec_lvl,  struct diss_db_cfg* params)
{
    //------------------ create the attribute database for the profile -------------------

    // Service content flag
    uint32_t cfg_flag;
    // DB Creation Statis
    uint8_t status = ATT_ERR_NO_ERROR;

    // Compute Attribute Table and save it in environment
    cfg_flag = diss_compute_cfg_flag(params->features);

    status = attm_svc_create_db(start_hdl, ATT_SVC_DEVICE_INFO, (uint8_t *)&cfg_flag,
            DIS_IDX_NB, NULL, env->task, &diss_att_db[0],
            (sec_lvl & (PERM_MASK_SVC_DIS | PERM_MASK_SVC_AUTH | PERM_MASK_SVC_EKS)));

    //-------------------- allocate memory required for the profile  ---------------------
    if (status == ATT_ERR_NO_ERROR)
    {
        struct diss_env_tag* diss_env =
                (struct diss_env_tag* ) ke_malloc(sizeof(struct diss_env_tag), KE_MEM_ATT_DB);

        // allocate DISS required environment variable
        env->env = (prf_env_t*) diss_env;
        diss_env->start_hdl = *start_hdl;
        diss_env->features  = params->features;
        diss_env->prf_env.app_task = app_task
                | (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
        diss_env->prf_env.prf_task = env->task | PERM(PRF_MI, DISABLE);

        // initialize environment variable
        env->id                     = TASK_ID_DISS;
        diss_task_init(&(env->desc));
        co_list_init(&(diss_env->values));

        // service is ready, go into an Idle state
        ke_state_set(env->task, DISS_IDLE);
    }

    return (status);
}
/**
 ****************************************************************************************
 * @brief Destruction of the DISS module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void diss_destroy(struct prf_task_env* env)
{
    struct diss_env_tag* diss_env = (struct diss_env_tag*) env->env;

    // remove all values present in list
    while(!co_list_is_empty(&(diss_env->values)))
    {
        struct co_list_hdr* hdr = co_list_pop_front(&(diss_env->values));
        ke_free(hdr);
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(diss_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void diss_create(struct prf_task_env* env, uint8_t conidx)
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
static void diss_cleanup(struct prf_task_env* env, uint8_t conidx, uint8_t reason)
{
    /* Nothing to do */
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// DISS Task interface required by profile manager
const struct prf_task_cbs diss_itf =
{
        (prf_init_fnct) diss_init,
        diss_destroy,
        diss_create,
        diss_cleanup,
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs* diss_prf_itf_get(void)
{
   return &diss_itf;
}

uint32_t diss_compute_cfg_flag(uint16_t features)
{
    //Service Declaration
    uint32_t cfg_flag = 1;

    for (uint8_t i = 0; i<DIS_CHAR_MAX; i++)
    {
        if (((features >> i) & 1) == 1)
        {
            cfg_flag |= (3 << (i*2 + 1));
        }
    }

    return cfg_flag;
}


uint8_t diss_handle_to_value(struct diss_env_tag* env, uint16_t handle)
{
    uint8_t value = DIS_CHAR_MAX;

    // handle cursor, start from first characteristic of service handle
    uint16_t cur_hdl = env->start_hdl + 1;

    for (uint8_t i = 0; i<DIS_CHAR_MAX; i++)
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

uint16_t diss_value_to_handle(struct diss_env_tag* env, uint8_t value)
{
    uint16_t handle = env->start_hdl + 1;
    int8_t i;

    for (i = 0; i<DIS_CHAR_MAX; i++)
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
    return ((i == DIS_CHAR_MAX) ? ATT_INVALID_HDL : handle);
}

uint8_t diss_check_val_len(uint8_t char_code, uint8_t val_len)
{
    uint8_t status = GAP_ERR_NO_ERROR;

    // Check if length is upper than the general maximal length
    if (val_len > DIS_VAL_MAX_LEN)
    {
        status = PRF_ERR_UNEXPECTED_LEN;
    }
    else
    {
        // Check if length matches particular requirements
        switch (char_code)
        {
            case DIS_SYSTEM_ID_CHAR:
                if (val_len != DIS_SYS_ID_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case DIS_IEEE_CHAR:
                if (val_len < DIS_IEEE_CERTIF_MIN_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case DIS_PNP_ID_CHAR:
                if (val_len != DIS_PNP_ID_LEN)
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



#endif //BLE_DIS_SERVER

/// @} DISS
