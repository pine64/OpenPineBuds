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
 * @addtogroup AMSCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_AMS_CLIENT)

#include "ams_common.h"
#include "ams_gatt_server.h"
#include "amsc.h"
#include "amsc_task.h"

#include "attm.h"
#include "gap.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

#include "compiler.h"
#include "ke_timer.h"

#include "co_utils.h"
#include "ke_mem.h"

static uint16_t amsc_last_read_handle = BTIF_INVALID_HCI_HANDLE;

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// AMSC UUID
const struct att_uuid_128 amsc_ams_svc = {{0xdc, 0xf8, 0x55, 0xad, 0x02, 0xc5,
                                           0xf4, 0x8e, 0x3a, 0x43, 0x36, 0x0f,
                                           0x2b, 0x50, 0xd3, 0x89}};

/// State machine used to retrieve AMS characteristics information
const struct prf_char_def_128 amsc_ams_char[AMSC_CHAR_MAX] = {
    /// Remote Command
    [AMSC_REMOTE_COMMAND_CHAR] = {{0xc2, 0x51, 0xca, 0xf7, 0x56, 0x0e, 0xdf,
                                   0xb8, 0x8a, 0x4a, 0xb1, 0x57, 0xd8, 0x81,
                                   0x3c, 0x9b},
                                  ATT_MANDATORY,
                                  ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF},

    /// Entity Update
    [AMSC_ENTITY_UPDATE_CHAR] = {{0x02, 0xc1, 0x96, 0xba, 0x92, 0xbb, 0x0c,
                                  0x9a, 0x1f, 0x41, 0x8d, 0x80, 0xce, 0xab,
                                  0x7c, 0x2f},
                                 ATT_OPTIONAL,
                                 ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF},

    /// Entity Attribute
    [AMSC_ENTITY_ATTRIBUTE_CHAR] = {{0xd7, 0xd5, 0xbb, 0x70, 0xa8, 0xa3, 0xab,
                                     0xa6, 0xd8, 0x46, 0xab, 0x23, 0x8c, 0xf3,
                                     0xb2, 0xc6},
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_RD},
};

/// State machine used to retrieve AMS characteristic descriptor information
const struct prf_char_desc_def amsc_ams_char_desc[AMSC_DESC_MAX] = {
    /// Remote Command Char. - Client Characteristic Configuration
    [AMSC_DESC_REMOTE_CMD_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY,
                                     AMSC_REMOTE_COMMAND_CHAR},
    /// Entity Update Char. - Client Characteristic Configuration
    [AMSC_DESC_ENTITY_UPDATE_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL,
                                        AMSC_ENTITY_UPDATE_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref AMSC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int amsc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct amsc_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
  // Status
  uint8_t status = GAP_ERR_NO_ERROR;
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);
  // Get connection index
  uint8_t conidx = param->conidx;
  uint8_t state = ke_state_get(dest_id);
  TRACE(3,"AMSC %s Entry. state=%d, conidx=%d",
      __func__, state, conidx);

  TRACE(3,"AMSC %s amsc_env->env[%d] = 0x%8.8x", __func__, conidx,
        (uint32_t)amsc_env->env[conidx]);
  if ((state == AMSC_IDLE) && (amsc_env->env[conidx] == NULL)) {
    TRACE(1,"AMSC %s passed state check", __func__);
    // allocate environment variable for task instance
    amsc_env->env[conidx] = (struct amsc_cnx_env *)ke_malloc(
        sizeof(struct amsc_cnx_env), KE_MEM_ATT_DB);
    ASSERT(amsc_env->env[conidx], "%s alloc error", __func__);
    memset(amsc_env->env[conidx], 0, sizeof(struct amsc_cnx_env));

    amsc_env->env[conidx]->last_char_code = AMSC_ENABLE_OP_CODE;

    // Start discovering
    // Discover AMS service by 128-bit UUID
    prf_disc_svc_send_128(&(amsc_env->prf_env), conidx,
                          (uint8_t *)&amsc_ams_svc.uuid);

    // Go to DISCOVERING state
    ke_state_set(dest_id, AMSC_DISCOVERING);

    // Configure the environment for a discovery procedure
    amsc_env->env[conidx]->last_req = ATT_DECL_PRIMARY_SERVICE;
  }

  else if (state != AMSC_FREE) {
    status = PRF_ERR_REQ_DISALLOWED;
  }

  // send an error if request fails
  if (status != GAP_ERR_NO_ERROR) {
    amsc_enable_rsp_send(amsc_env, conidx, status);
  }

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref AMSC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int amsc_read_cmd_handler(ke_msg_id_t const msgid,
                                 struct gattc_read_cmd *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id) {
  TRACE(5,"AMSC %s Entry. hdl=0x%4.4x, op=%d, len=%d, off=%d", __func__,
        param->req.simple.handle, param->operation, param->req.simple.length,
        param->req.simple.offset);

  uint8_t conidx = KE_IDX_GET(dest_id);

  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);

  if (amsc_env != NULL) {
    amsc_last_read_handle = param->req.simple.handle;
    // Send the read request
    prf_read_char_send(
        &(amsc_env->prf_env), conidx, amsc_env->env[conidx]->ams.svc.shdl,
        amsc_env->env[conidx]->ams.svc.ehdl, param->req.simple.handle);
  } else {
    //        amsc_send_no_conn_cmp_evt(dest_id, src_id, param->handle,
    //        AMSC_WRITE_CL_CFG_OP_CODE);
    ASSERT(0, "%s implement me", __func__);
  }

  return KE_MSG_CONSUMED;
}

static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                  struct gattc_read_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);
  TRACE(3,"AMSC %s param->handle=%x param->length=%d", __func__, param->handle,
        param->length);
  uint8_t conidx = KE_IDX_GET(src_id);

  if (amsc_env != NULL) {
  	amsc_last_read_handle = BTIF_INVALID_HCI_HANDLE;
    struct gattc_read_cfm *cfm =
        KE_MSG_ALLOC_DYN(GATTC_READ_CFM, 
        				 KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSP), conidx),
                         dest_id, gattc_read_cfm, param->length);
    cfm->status = 0;  // read_ind has no status???
    cfm->handle = param->handle;
    cfm->length = param->length;
    memcpy(cfm->value, param->value, param->length);
    ke_msg_send(cfm);
  }
  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref AMSC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int amsc_write_cmd_handler(ke_msg_id_t const msgid,
                                  struct gattc_write_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  TRACE(4,"AMSC %s Entry. hdl=0x%4.4x, op=%d, len=%d", __func__,
        param->handle, param->operation, param->length);

  uint8_t conidx = KE_IDX_GET(dest_id);

  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);

  if (amsc_env != NULL) {
    amsc_env->last_write_handle[conidx] = param->handle;
    // TODO(jkessinger): Use ke_msg_forward.
    struct gattc_write_cmd *wr_char = KE_MSG_ALLOC_DYN(
        GATTC_WRITE_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
        dest_id, gattc_write_cmd, param->length);
    memcpy(wr_char, param, sizeof(struct gattc_write_cmd) + param->length);
    // Send the message
    ke_msg_send(wr_char);
  } else {
    //        amsc_send_no_conn_cmp_evt(dest_id, src_id, param->handle,
    //        AMSC_WRITE_CL_CFG_OP_CODE);
    ASSERT(0, "%s implement me", __func__);
  }

  return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_SDP_SVC_IND_HANDLER message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
__STATIC int gattc_sdp_svc_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_sdp_svc_ind const *ind,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
  uint8_t state = ke_state_get(dest_id);

  TRACE(4,"AMSC %s Entry. end_hdl=0x%4.4x, start_hdl=0x%4.4x, att.att_type=%d",
      __func__, ind->end_hdl, ind->start_hdl, ind->info[0].att.att_type);
  TRACE(3,"AMSC att_char.prop=%d, att_char.handle=0x%4.4x, att_char.att_type=%d",
      ind->info[0].att_char.prop, ind->info[0].att_char.handle,
      ind->info[0].att_char.att_type);
  TRACE(4,"AMSC inc_svc.att_type=%d, inc_svc.end_hdl=0x%4.4x, inc_svc.start_hdl=0x%4.4x, state=%d",
      ind->info[0].att_type, ind->info[0].inc_svc.att_type,
      ind->info[0].inc_svc.start_hdl, state);

  if (state == AMSC_DISCOVERING) {
    uint8_t conidx = KE_IDX_GET(src_id);

    struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);

    ASSERT_INFO(amsc_env != NULL, dest_id, src_id);
    ASSERT_INFO(amsc_env->env[conidx] != NULL, dest_id, src_id);

    if (amsc_env->env[conidx]->nb_svc == 0) {
      TRACE(0,"AMSC retrieving characteristics and descriptors.");
      // Retrieve AMS characteristics and descriptors
      prf_extract_svc_info_128(ind, AMSC_CHAR_MAX, &amsc_ams_char[0],
                               &amsc_env->env[conidx]->ams.chars[0],
                               AMSC_DESC_MAX, &amsc_ams_char_desc[0],
                               &amsc_env->env[conidx]->ams.descs[0]);

      // Even if we get multiple responses we only store 1 range
      amsc_env->env[conidx]->ams.svc.shdl = ind->start_hdl;
      amsc_env->env[conidx]->ams.svc.ehdl = ind->end_hdl;
    }

    amsc_env->env[conidx]->nb_svc++;
  }

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int amsc_gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                      struct gattc_cmp_evt const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id) {
  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);
  uint8_t conidx = KE_IDX_GET(dest_id);
  TRACE(5,"AMSC %s entry. op=%d, seq=%d, status=%d, conidx=%d",
      __func__, param->operation, param->seq_num, param->status, conidx);
  // Status
  uint8_t status;

  if (amsc_env->env[conidx] != NULL) {
    uint8_t state = ke_state_get(dest_id);

    TRACE(2,"AMSC %s state=%d", __func__, state);
    if (state == AMSC_DISCOVERING) {
      status = param->status;

      if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
          (status == ATT_ERR_NO_ERROR)) {
        // Discovery
        // check characteristic validity
        if (amsc_env->env[conidx]->nb_svc == 1) {
          status = prf_check_svc_char_validity_128(
              AMSC_CHAR_MAX, amsc_env->env[conidx]->ams.chars, amsc_ams_char);
        }
        // too much services
        else if (amsc_env->env[conidx]->nb_svc > 1) {
          status = PRF_ERR_MULTIPLE_SVC;
        }
        // no services found
        else {
          status = PRF_ERR_STOP_DISC_CHAR_MISSING;
        }

        // check descriptor validity
        if (status == GAP_ERR_NO_ERROR) {
          status = prf_check_svc_char_desc_validity(
              AMSC_DESC_MAX, amsc_env->env[conidx]->ams.descs,
              amsc_ams_char_desc, amsc_env->env[conidx]->ams.chars);
        }
      }

      amsc_enable_rsp_send(amsc_env, conidx, status);
#if (ANCS_PROXY_ENABLE)
      TRACE(4,"AMSC %s rmtChar=0x%4.4x, rmtVal=0x%4.4x, rmtCfg=0x%4.4x",
   		    __func__,
            amsc_env->env[conidx]->ams.chars[AMSC_REMOTE_COMMAND_CHAR].char_hdl,
            amsc_env->env[conidx]->ams.chars[AMSC_REMOTE_COMMAND_CHAR].val_hdl,
            amsc_env->env[conidx]
                ->ams.descs[AMSC_DESC_REMOTE_CMD_CL_CFG]
                .desc_hdl);
      TRACE(4,"AMSC %s EnUpChar=0x%4.4x EnUpVal=0x%4.4x, EnUpCfg=0x%4.4x",
            __func__,
            amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_UPDATE_CHAR].char_hdl,
            amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_UPDATE_CHAR].val_hdl,
            amsc_env->env[conidx]
                ->ams.descs[AMSC_DESC_ENTITY_UPDATE_CL_CFG]
                .desc_hdl);
      TRACE(3,"AMSC %s EnAtrChar=0x%4.4x, EnAtrVal=0x%4.4x", __func__,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_ATTRIBUTE_CHAR].char_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_ATTRIBUTE_CHAR].val_hdl);
      ams_proxy_set_ready_flag(conidx,
          amsc_env->env[conidx]->ams.chars[AMSC_REMOTE_COMMAND_CHAR].char_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_REMOTE_COMMAND_CHAR].val_hdl,
          amsc_env->env[conidx]
              ->ams.descs[AMSC_DESC_REMOTE_CMD_CL_CFG]
              .desc_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_UPDATE_CHAR].char_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_UPDATE_CHAR].val_hdl,
          amsc_env->env[conidx]
              ->ams.descs[AMSC_DESC_ENTITY_UPDATE_CL_CFG]
              .desc_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_ATTRIBUTE_CHAR].char_hdl,
          amsc_env->env[conidx]->ams.chars[AMSC_ENTITY_ATTRIBUTE_CHAR].val_hdl);
      ke_state_set(dest_id, AMSC_IDLE);
#endif
    } else {
      switch (param->operation) {
        case GATTC_READ: {
          TRACE(3,"AMSC %s read complete status=%d amsc_last_read_handle %d",
                __func__, param->status, amsc_last_read_handle);
          if ((0 != param->status) &&
              (BTIF_INVALID_HCI_HANDLE != amsc_last_read_handle)) {
            struct gattc_read_cfm *cfm = KE_MSG_ALLOC_DYN(
                GATTC_READ_CFM, KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSP), conidx),
                dest_id,
                gattc_read_cfm, 0);
            cfm->status = 0;
            cfm->handle = amsc_last_read_handle;
            cfm->length = 0;
            ke_msg_send(cfm);
          }
          amsc_last_read_handle = BTIF_INVALID_HCI_HANDLE;
          break;
        }
        case GATTC_WRITE: {
          struct gattc_write_cfm *cfm =
              KE_MSG_ALLOC(GATTC_WRITE_CFM, 
              			   KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSP), conidx),
                           dest_id, gattc_write_cfm);
          cfm->handle = amsc_env->last_write_handle[conidx];
          amsc_env->last_write_handle[conidx] = ATT_INVALID_HANDLE;
          cfm->status = param->status;
          ke_msg_send(cfm);
          break;
        }
        case GATTC_WRITE_NO_RESPONSE:
          // There's currently no need to notify the proxy task that this completed.
          break;
        case GATTC_NOTIFY:
        case GATTC_INDICATE:
          // Nothing to do. Notify sent.
        case GATTC_REGISTER:
        case GATTC_UNREGISTER:
        case GATTC_SDP_DISC_SVC:
          // Do nothing
          break;

        default:
          ASSERT_ERR(0);
          break;
      }
    }
  }
  // else ignore the message
  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                   struct gattc_event_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
  BLE_FUNC_ENTER();
  TRACE(5,"AMSC %s Entry. handle=0x%x, len=%d, type=%d, val[0]=0x%x",
        __func__, param->handle, param->length, param->type, param->value[0]);
  uint8_t conidx = KE_IDX_GET(src_id);
  
  struct gattc_send_evt_cmd *cmd;
  cmd = KE_MSG_ALLOC_DYN(AMS_PROXY_IND_EVT, 
  	    				 KE_BUILD_ID(prf_get_task_from_id(TASK_ID_AMSP), conidx),
                         dest_id, gattc_send_evt_cmd, param->length);
  cmd->handle = param->handle;
  cmd->operation = GATTC_NOTIFY;
  cmd->seq_num = 0;
  cmd->length = param->length;
  memcpy(cmd->value, param->value, param->length);
  ke_msg_send(cmd);

  return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
KE_MSG_HANDLER_TAB(amsc){
    {AMSC_ENABLE_REQ, (ke_msg_func_t)amsc_enable_req_handler},
    {AMSC_READ_CMD, (ke_msg_func_t)amsc_read_cmd_handler},
    {GATTC_READ_IND, (ke_msg_func_t)gattc_read_ind_handler},
    {AMSC_WRITE_CMD, (ke_msg_func_t)amsc_write_cmd_handler},

    {GATTC_SDP_SVC_IND, (ke_msg_func_t)gattc_sdp_svc_ind_handler},

    {GATTC_EVENT_IND, (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT, (ke_msg_func_t)amsc_gattc_cmp_evt_handler},
};

void amsc_task_init(struct ke_task_desc *task_desc) {
  TRACE(1,"AMSC %s Entry.", __func__);
  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(AMSC, amsc);

  task_desc->msg_handler_tab = amsc_msg_handler_tab;
  task_desc->msg_cnt = ARRAY_LEN(amsc_msg_handler_tab);
  task_desc->state = amsc_env->state;
  task_desc->idx_max = AMSC_IDX_MAX;
}

#endif  //(BLE_AMS_CLIENT)

/// @} AMSCTASK
