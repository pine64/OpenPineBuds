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
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_DIS)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app.h"                     // Application Manager Definitions
#include "app_dis.h"                 // Device Information Service Application Definitions
#include "diss_task.h"               // Device Information Profile Functions
#include "prf_types.h"               // Profile Common Types Definitions
#include "gapm_task.h"               // GAP Manager Task API
#include <string.h>

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static int diss_value_req_ind_handler(ke_msg_id_t const msgid,
                                          struct diss_value_req_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Initialize length
    uint8_t len = 0;
    // Pointer to the data
    uint8_t *data = NULL;

    // Check requested value
    switch (param->value)
    {
        case DIS_MANUFACTURER_NAME_CHAR:
        {
            // Set information
            len = APP_DIS_MANUFACTURER_NAME_LEN;
            data = (uint8_t *)APP_DIS_MANUFACTURER_NAME;
        } break;

        case DIS_MODEL_NB_STR_CHAR:
        {
            // Set information
            len = APP_DIS_MODEL_NB_STR_LEN;
            data = (uint8_t *)APP_DIS_MODEL_NB_STR;
        } break;

        case DIS_SYSTEM_ID_CHAR:
        {
            // Set information
            len = APP_DIS_SYSTEM_ID_LEN;
            data = (uint8_t *)APP_DIS_SYSTEM_ID;
        } break;

        case DIS_PNP_ID_CHAR:
        {
            // Set information
            len = APP_DIS_PNP_ID_LEN;
            data = (uint8_t *)APP_DIS_PNP_ID;
        } break;

        case DIS_SERIAL_NB_STR_CHAR:
        {
            // Set information
            len = APP_DIS_SERIAL_NB_STR_LEN;
            data = (uint8_t *)APP_DIS_SERIAL_NB_STR;
        } break;

        case DIS_HARD_REV_STR_CHAR:
        {
            // Set information
            len = APP_DIS_HARD_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_HARD_REV_STR;
        } break;

        case DIS_FIRM_REV_STR_CHAR:
        {
            // Set information
            len = APP_DIS_FIRM_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_FIRM_REV_STR;
        } break;

        case DIS_SW_REV_STR_CHAR:
        {
            // Set information
            len = APP_DIS_SW_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_SW_REV_STR;
        } break;

        case DIS_IEEE_CHAR:
        {
            // Set information
            len = APP_DIS_IEEE_LEN;
            data = (uint8_t *)APP_DIS_IEEE;
        } break;

        default:
            ASSERT_ERR(0);
            break;
    }

    // Allocate confirmation to send the value
    struct diss_value_cfm *cfm_value = KE_MSG_ALLOC_DYN(DISS_VALUE_CFM,
            src_id, dest_id,
            diss_value_cfm,
            len);

    // Set parameters
    cfm_value->value = param->value;
    cfm_value->length = len;
    if (len)
    {
        // Copy data
        memcpy(&cfm_value->data[0], data, len);
    }
    // Send message
    ke_msg_send(cfm_value);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_dis_init(void)
{
    // Nothing to do
}

void app_dis_add_dis(void)
{
    struct diss_db_cfg* db_cfg;
    // Allocate the DISS_CREATE_DB_REQ
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct diss_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX>1
	req->sec_lvl = PERM(SVC_AUTH, ENABLE)|PERM(SVC_MI, ENABLE);
#else
	req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif

    req->prf_task_id = TASK_ID_DISS;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    // Set parameters
    db_cfg = (struct diss_db_cfg* ) req->param;
    db_cfg->features = APP_DIS_FEATURES;

    // Send the message
    ke_msg_send(req);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_dis_msg_handler_list[] =
{
    {DISS_VALUE_REQ_IND,     (ke_msg_func_t)diss_value_req_ind_handler},
};

const struct ke_state_handler app_dis_table_handler =
    {&app_dis_msg_handler_list[0], (sizeof(app_dis_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_APP_DIS

/// @} APP
