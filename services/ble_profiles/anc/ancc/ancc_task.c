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
 * @addtogroup ANCCTASK
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

#include "attm.h"
#include "gap.h"
#include "gattc_task.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

#include "compiler.h"
#include "ke_timer.h"

#include "co_utils.h"
#include "ke_mem.h"

#if (ANCS_PROXY_ENABLE)
#include "ancs_gatt_server.h"
#endif
/*
 * STRUCTURES
 ****************************************************************************************
 */

/// ANCS UUID
const struct att_uuid_128 ancc_anc_svc = {{0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B,
                                           0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5,
                                           0x31, 0xF4, 0x05, 0x79}};

/// State machine used to retrieve ANCS characteristics information
const struct prf_char_def_128 ancc_anc_char[ANCC_CHAR_MAX] = {
    /// Notification Source
    [ANCC_CHAR_NTF_SRC] = {{0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C,
                            0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F},
                           ATT_MANDATORY,
                           ATT_CHAR_PROP_NTF},

    /// Control Point
    [ANCC_CHAR_CTRL_PT] = {{0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98,
                            0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69},
                           ATT_OPTIONAL,
                           ATT_CHAR_PROP_WR},

    /// Data Source
    [ANCC_CHAR_DATA_SRC] = {{0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE,
                             0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22},
                            ATT_OPTIONAL,
                            ATT_CHAR_PROP_NTF},

};

/// State machine used to retrieve ANCS characteristic descriptor information
const struct prf_char_desc_def ancc_anc_char_desc[ANCC_DESC_MAX] = {
    /// Notification Source Char. - Client Characteristic Configuration
    [ANCC_DESC_NTF_SRC_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_MANDATORY,
                                  ANCC_CHAR_NTF_SRC},
    /// Data Source Char. - Client Characteristic Configuration
    [ANCC_DESC_DATA_SRC_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG, ATT_OPTIONAL,
                                   ANCC_CHAR_DATA_SRC},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_ENABLE_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_enable_req_handler(ke_msg_id_t const msgid,
                                   struct ancc_enable_req *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
  // Status
  uint8_t status = GAP_ERR_NO_ERROR;
  struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);
  // Get connection index
  uint8_t conidx = param->conidx;
  uint8_t state = ke_state_get(dest_id);
  TRACE(3,"ANCSC %s Entry. state=%d, conidx=%d",
      __func__, state, conidx);

  if ((state == ANCC_IDLE) && (ancc_env->env[conidx] == NULL)) {
    TRACE(1,"ANCSC %s passed state check", __func__);
    // allocate environment variable for task instance
    ancc_env->env[conidx] = (struct ancc_cnx_env *)ke_malloc(
        sizeof(struct ancc_cnx_env), KE_MEM_ATT_DB);
    ASSERT(ancc_env->env[conidx], "%s alloc error", __func__);
    memset(ancc_env->env[conidx], 0, sizeof(struct ancc_cnx_env));

    ancc_env->env[conidx]->last_char_code = ANCC_ENABLE_OP_CODE;

    // Start discovering
    // Discover ANCS service by 128-bit UUID
    prf_disc_svc_send_128(&(ancc_env->prf_env), conidx,
                          (uint8_t *)&ancc_anc_svc.uuid);

    // Go to DISCOVERING state
    ke_state_set(dest_id, ANCC_DISCOVERING);

    // Configure the environment for a discovery procedure
    ancc_env->env[conidx]->last_req = ATT_DECL_PRIMARY_SERVICE;
  }

  else if (state != ANCC_FREE) {
    status = PRF_ERR_REQ_DISALLOWED;
  }

  // send an error if request fails
  if (status != GAP_ERR_NO_ERROR) {
    ancc_enable_rsp_send(ancc_env, conidx, status);
  }

  return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ANCC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_read_cmd_handler(ke_msg_id_t const msgid,
                                 struct gattc_read_cmd *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id) {

  uint8_t conidx = KE_IDX_GET(dest_id);
  TRACE(6,"ANCSC %s Entry. conidex %d hdl=0x%4.4x, op=%d, len=%d, off=%d", __func__,
        conidx, param->req.simple.handle, param->operation, param->req.simple.length,
        param->req.simple.offset);

  // Get the address of the environment
  struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);

  if (ancc_env != NULL) {
    // Send the read request
    prf_read_char_send(
        &(ancc_env->prf_env), conidx, ancc_env->env[conidx]->anc.svc.shdl,
        ancc_env->env[conidx]->anc.svc.ehdl, param->req.simple.handle);
  } else {
    //        amsc_send_no_conn_cmp_evt(dest_id, src_id, param->handle,
    //        ANCC_WRITE_CL_CFG_OP_CODE);
    ASSERT(0, "%s implement me", __func__);
  }

  return KE_MSG_CONSUMED;
}

static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                  struct gattc_read_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  // Get the address of the environment
  struct amsc_env_tag *amsc_env = PRF_ENV_GET(ANCC, amsc);
  TRACE(3,"ANCSC %s param->handle=0x%x param->length=%d", __func__, param->handle,
        param->length);
  
  if (amsc_env != NULL) {
    uint8_t conidx = KE_IDX_GET(src_id);
    struct gattc_read_cfm *cfm =
        KE_MSG_ALLOC_DYN(GATTC_READ_CFM, 
        				 KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCSP), conidx),
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
 * @brief Handles reception of the @ref ANCC_WRITE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int ancc_write_cmd_handler(ke_msg_id_t const msgid,
                                  struct gattc_write_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
  TRACE(4,"ANCSC %s Entry. hdl=0x%4.4x, op=%d, len=%d", __func__,
        param->handle, param->operation, param->length);

  uint8_t conidx = KE_IDX_GET(dest_id);

  // Get the address of the environment
  struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);

  if (ancc_env != NULL) {
    ancc_env->last_write_handle[conidx] = param->handle;
    // TODO(jkessinger): Use ke_msg_forward.
    struct gattc_write_cmd *wr_char = KE_MSG_ALLOC_DYN(
        GATTC_WRITE_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
        dest_id, gattc_write_cmd, param->length);
    memcpy(wr_char, param, sizeof(struct gattc_write_cmd) + param->length);
    // Send the message
    ke_msg_send(wr_char);
  } else {
    //        ancc_send_no_conn_cmp_evt(dest_id, src_id, param->handle,
    //        ANCC_WRITE_CL_CFG_OP_CODE);
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

  TRACE(4,"ANCSC %s Entry. end_hdl=0x%4.4x, start_hdl=0x%4.4x, att.att_type=%d",
      __func__, ind->end_hdl, ind->start_hdl, ind->info[0].att.att_type);
  TRACE(3,"ANCSC att_char.prop=%d, att_char.handle=0x%4.4x, att_char.att_type=%d",
      ind->info[0].att_char.prop, ind->info[0].att_char.handle,
      ind->info[0].att_char.att_type);
  TRACE(4,"ANCSC inc_svc.att_type=%d, inc_svc.end_hdl=0x%4.4x, inc_svc.start_hdl=0x%4.4x, state=%d",
      ind->info[0].att_type, ind->info[0].inc_svc.att_type,
      ind->info[0].inc_svc.start_hdl, state);

  if (state == ANCC_DISCOVERING) {
    uint8_t conidx = KE_IDX_GET(src_id);

    struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);

    ASSERT_INFO(ancc_env != NULL, dest_id, src_id);
    ASSERT_INFO(ancc_env->env[conidx] != NULL, dest_id, src_id);

    if (ancc_env->env[conidx]->nb_svc == 0) {
      TRACE(0,"ANCSC retrieving characteristics and descriptors.");
      // Retrieve ANC characteristics and descriptors
      prf_extract_svc_info_128(ind, ANCC_CHAR_MAX, &ancc_anc_char[0],
                               &ancc_env->env[conidx]->anc.chars[0],
                               ANCC_DESC_MAX, &ancc_anc_char_desc[0],
                               &ancc_env->env[conidx]->anc.descs[0]);

      // Even if we get multiple responses we only store 1 range
      ancc_env->env[conidx]->anc.svc.shdl = ind->start_hdl;
      ancc_env->env[conidx]->anc.svc.ehdl = ind->end_hdl;
    }

    ancc_env->env[conidx]->nb_svc++;
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
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id) {
  // Get the address of the environment
  struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);
  uint8_t conidx = KE_IDX_GET(dest_id);
  TRACE(5,"ANCSC %s entry. op=%d, seq=%d, status=%d, conidx=%d",
      __func__, param->operation, param->seq_num, param->status, conidx);
  // Status
  uint8_t status;

  if (ancc_env->env[conidx] != NULL) {
    uint8_t state = ke_state_get(dest_id);

    TRACE(2,"ANCSC %s state=%d", __func__, state);
    if (state == ANCC_DISCOVERING) {
      status = param->status;

      if ((status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
          (status == ATT_ERR_NO_ERROR)) {
        // Discovery
        // check characteristic validity
        if (ancc_env->env[conidx]->nb_svc == 1) {
          status = prf_check_svc_char_validity_128(
              ANCC_CHAR_MAX, ancc_env->env[conidx]->anc.chars, ancc_anc_char);
        }
        // too much services
        else if (ancc_env->env[conidx]->nb_svc > 1) {
          status = PRF_ERR_MULTIPLE_SVC;
        }
        // no services found
        else {
          status = PRF_ERR_STOP_DISC_CHAR_MISSING;
        }

        // check descriptor validity
        if (status == GAP_ERR_NO_ERROR) {
          status = prf_check_svc_char_desc_validity(
              ANCC_DESC_MAX, ancc_env->env[conidx]->anc.descs,
              ancc_anc_char_desc, ancc_env->env[conidx]->anc.chars);
        }
      }

      ancc_enable_rsp_send(ancc_env, conidx, status);
#if (ANCS_PROXY_ENABLE)
      TRACE(4,"ANCSC %s NSChar=0x%4.4x, NSVal=0x%4.4x, NSCfg=0x%4.4x", __func__,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].char_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl);
      TRACE(4,"ANCSC %s DSChar=0x%4.4x DSVal=0x%4.4x, DSCfg=0x%4.4x", __func__,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].char_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl);
      TRACE(3,"ANCSC %s CPChar=0x%4.4x, CPVal=0x%4.4x", __func__,
            ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].char_hdl,
            ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl);
      ancs_proxy_set_ready_flag(conidx,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].char_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_NTF_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_NTF_SRC_CL_CFG].desc_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].char_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_DATA_SRC].val_hdl,
          ancc_env->env[conidx]->anc.descs[ANCC_DESC_DATA_SRC_CL_CFG].desc_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].char_hdl,
          ancc_env->env[conidx]->anc.chars[ANCC_CHAR_CTRL_PT].val_hdl);
      ke_state_set(dest_id, ANCC_IDLE);
#endif
    } else {
      switch (param->operation) {
        case GATTC_READ: {
          TRACE(2,"ANCSC %s read complete status=%d", __func__,
                param->status);
          break;
        }
        case GATTC_WRITE: {
          struct gattc_write_cfm *cfm =
              KE_MSG_ALLOC(GATTC_WRITE_CFM, 
              			   KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCSP), conidx),
                           dest_id, gattc_write_cfm);
          cfm->handle = ancc_env->last_write_handle[conidx];
          ancc_env->last_write_handle[conidx] = ATT_INVALID_HANDLE;
          cfm->status = param->status;
          ke_msg_send(cfm);
          break;
        }
        case GATTC_WRITE_NO_RESPONSE:
          // There's currently no need to notify the proxy task that this completed.
          break;
        case GATTC_NOTIFY:
        case GATTC_INDICATE:
          // Nothing to do. Notification sent.
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
  TRACE(5,"ANCSC %s Entry. handle=0x%x, len=%d, type=%d, val[0]=0x%x",
        __func__, param->handle, param->length, param->type, param->value[0]);
  uint8_t conidx = KE_IDX_GET(src_id);
  struct gattc_send_evt_cmd *cmd;
  cmd =
      KE_MSG_ALLOC_DYN(ANCS_PROXY_IND_EVT, 
                       KE_BUILD_ID(prf_get_task_from_id(TASK_ID_ANCSP), conidx),
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
KE_MSG_HANDLER_TAB(ancc){
    {ANCC_ENABLE_REQ, (ke_msg_func_t)ancc_enable_req_handler},
    {ANCC_WRITE_CMD, (ke_msg_func_t)ancc_write_cmd_handler},
    {ANCC_READ_CMD, (ke_msg_func_t)ancc_read_cmd_handler},
    {GATTC_READ_IND, (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_SDP_SVC_IND, (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GATTC_EVENT_IND, (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT, (ke_msg_func_t)gattc_cmp_evt_handler},
};

void ancc_task_init(struct ke_task_desc *task_desc) {
  TRACE(1,"ANCSC %s Entry.", __func__);
  // Get the address of the environment
  struct ancc_env_tag *ancc_env = PRF_ENV_GET(ANCC, ancc);

  task_desc->msg_handler_tab = ancc_msg_handler_tab;
  task_desc->msg_cnt = ARRAY_LEN(ancc_msg_handler_tab);
  task_desc->state = ancc_env->state;
  task_desc->idx_max = ANCC_IDX_MAX;
}

#endif  //(BLE_ANC_CLIENT)

/// @} ANCCTASK
