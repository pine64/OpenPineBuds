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
#include "rwip_config.h"     // SW configuration

#if (BLE_APP_HR)
#include "app_hrps.h"
#include "hrps.h"
#include "hrps_task.h"

#include "app.h"                     // Application Definitions
#include "app_task.h"                // application task definitions
#include "co_bt.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "string.h"

struct app_hrps_env_tag app_hrps_env;

void app_hrps_init(void)
{
    // Reset the environment
    memset(&app_hrps_env, 0x00, sizeof(app_hrps_env));

	app_hrps_env.connectionIndex =  INVALID_CONNECTION_INDEX;
	app_hrps_env.isNotificationEnabled = false;
}

void app_hrps_connected_evt_handler(uint8_t conidx)
{
	app_hrps_env.connectionIndex = conidx;
}

void app_hrps_disconnected_evt_handler(void)
{
	app_hrps_env.connectionIndex =  INVALID_CONNECTION_INDEX;
	app_hrps_env.isNotificationEnabled = false;
}


void app_hrps_send_measument_via_notification(void)
{
    if (app_hrps_env.isNotificationEnabled) {
        struct hrps_meas_send_req *req = KE_MSG_ALLOC(HRPS_MEAS_SEND_REQ,
                                         prf_get_task_from_id(TASK_ID_HRPS),
                                         TASK_APP,
                                         hrps_meas_send_req);

        memcpy(req, &app_hrps_env.meas, sizeof(struct hrps_meas_send_req));

        ke_msg_send(req);
    }
}

void app_hrps_add_profile(void)
{
	TRACE(0,"app_hrps_add_profile");
    struct hrps_db_cfg* db_cfg;

    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct hrps_db_cfg));

    
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
#if BLE_CONNECTION_MAX>1
	req->sec_lvl = PERM(SVC_AUTH, ENABLE)|PERM(SVC_MI, ENABLE);
#else
	req->sec_lvl = PERM(SVC_AUTH, ENABLE);
#endif  

    req->prf_task_id = TASK_ID_HRPS;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    db_cfg = (struct hrps_db_cfg*) req->param;

    db_cfg->features = HRPS_BODY_SENSOR_LOC_CHAR_SUP | HRPS_ENGY_EXP_FEAT_SUP | HRPS_HR_MEAS_NTF_CFG;

    db_cfg->body_sensor_loc = 0;

    // Send the message
    ke_msg_send(req);
}

static int app_hrps_msg_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
    // Do nothing

    return (KE_MSG_CONSUMED);
}

static int app_hrps_measurement_ccc_changed_handler(ke_msg_id_t const msgid,
                              struct hrps_cfg_indntf_ind *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
   TRACE(2,"%s cfg: %d", __func__, param->cfg_val);

   app_hrps_env.isNotificationEnabled = param->cfg_val;
   return (KE_MSG_CONSUMED);
}

static int app_hrps_measurement_sent_handler(ke_msg_id_t const msgid,
                              void const *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{

   return (KE_MSG_CONSUMED);
}

static int app_hrps_ctrl_point_received_handler(ke_msg_id_t const msgid,
                              struct hrps_energy_exp_reset_ind *param,
                              ke_task_id_t const dest_id,
                              ke_task_id_t const src_id)
{
   TRACE(1,"%s", __func__); 

   app_hrps_send_measument_via_notification();
   return (KE_MSG_CONSUMED);
}
/// Default State handlers definition
const struct ke_msg_handler hrps_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_hrps_msg_handler},

    {HRPS_CFG_INDNTF_IND,     (ke_msg_func_t)app_hrps_measurement_ccc_changed_handler},
    {HRP_MEASUREMENT_DATA_SENT,    	(ke_msg_func_t)app_hrps_measurement_sent_handler},
    {HRPS_ENERGY_EXP_RESET_IND,   (ke_msg_func_t)app_hrps_ctrl_point_received_handler},

};

const struct ke_state_handler app_hrps_table_handler =
    {&hrps_msg_handler_list[0], (sizeof(hrps_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif
