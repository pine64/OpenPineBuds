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
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"              // SW configuration

#if (BLE_APP_PRESENT)

#if (BLE_AMS_CLIENT)

#include "app_amsc.h"
#include "app_amsc_task.h"
#include "app_task.h"                  // Application Task API
#include "app_sec.h"
#include "ke_msg.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles AMS Eable reponse
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int amsc_anc_enable_rsp_handler(ke_msg_id_t const msgid,
                                 struct amsc_enable_rsp const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{    
    
    return (KE_MSG_CONSUMED);
}

/// Default State handlers definition
const struct ke_msg_handler app_amsc_msg_handler_list[] =
{
    {AMSC_ENABLE_RSP,                      (ke_msg_func_t)amsc_anc_enable_rsp_handler},
};

const struct ke_state_handler app_amsc_table_handler =
    {&app_amsc_msg_handler_list[0], (sizeof(app_amsc_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //BLE_AMS_CLIENT

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
