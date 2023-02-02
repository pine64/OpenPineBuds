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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "string.h"

#include "app_ble_mode_switch.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "besbt.h"
#include "bluetooth.h"
#include "hfp_api.h"
#include "os_api.h"

extern "C" void OS_NotifyEvm(void);
static const char *const app_bt_func_table_str[] = {
    "Me_switch_sco",
    "ME_SwitchRole",
    "ME_SetConnectionRole",
    "MeDisconnectLink",
    "ME_StopSniff",
    "ME_SetAccessibleMode",
    "Me_SetLinkPolicy",
    "CMGR_SetSniffTimer",
    "CMGR_SetSniffInofToAllHandlerByRemDev",
    "A2DP_OpenStream",
    "A2DP_CloseStream",
    "A2DP_SetMasterRole",
    "HF_CreateServiceLink",
    "HF_DisconnectServiceLink",
    "HF_CreateAudioLink",
    "HF_DisconnectAudioLink",
    "HF_EnableSniffMode",
    "HF_SetMasterRole",
    "HS_CreateServiceLink",
    "HS_CreateAudioLink",
    "HS_DisconnectAudioLink",
    "HS_EnableSniffMode",
    "HS_DisconnectServiceLink",
    "BT_Control_SleepMode",
    "BT_Custom_Func",
    "ME_StartSniff",
    "DIP_QuryService",
    "A2DP_Force_OpenStream",
    "HF_Force_CreateServiceLink",
    "app_set_access_mode_test",
    "app_set_adv_mode_test",
    "FPGA_write_mem_test",
    "FPGA_read_mem_test",
};

#define APP_BT_MAILBOX_MAX (40)
osMailQDef(app_bt_mailbox, APP_BT_MAILBOX_MAX, APP_BT_MAIL);
static osMailQId app_bt_mailbox = NULL;

static btif_accessible_mode_t gBT_DEFAULT_ACCESS_MODE = BTIF_BAM_NOT_ACCESSIBLE;
static uint8_t bt_access_mode_set_pending = 0;
void app_set_accessmode(btif_accessible_mode_t mode) {
#if !defined(IBRT)
  const btif_access_mode_info_t info = {
      BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL, BTIF_BT_DEFAULT_INQ_SCAN_WINDOW,
      BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL, BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW};
  bt_status_t status;
  osapi_lock_stack();
  gBT_DEFAULT_ACCESS_MODE = mode;

  status = btif_me_set_accessible_mode(mode, &info);
  TRACE(1, "app_set_accessmode status=0x%x", status);

  if (status == BT_STS_IN_PROGRESS)
    bt_access_mode_set_pending = 1;
  else
    bt_access_mode_set_pending = 0;
  osapi_unlock_stack();
#endif
}

bool app_is_access_mode_set_pending(void) { return bt_access_mode_set_pending; }

extern "C" void app_bt_accessmode_set(btif_accessible_mode_t mode);
void app_set_pending_access_mode(void) {
  if (bt_access_mode_set_pending) {
    TRACE(1, "Pending for change access mode to %d", gBT_DEFAULT_ACCESS_MODE);
    bt_access_mode_set_pending = 0;
    app_bt_accessmode_set(gBT_DEFAULT_ACCESS_MODE);
  }
}

void app_retry_setting_access_mode(void) {
  TRACE(0, "Former setting access mode failed, retry it.");
  app_bt_accessmode_set(gBT_DEFAULT_ACCESS_MODE);
}

#define PENDING_SET_LINKPOLICY_REQ_BUF_CNT 5
static BT_SET_LINKPOLICY_REQ_T
    pending_set_linkpolicy_req[PENDING_SET_LINKPOLICY_REQ_BUF_CNT];

static uint8_t pending_set_linkpolicy_in_cursor = 0;
static uint8_t pending_set_linkpolicy_out_cursor = 0;

static void app_bt_print_pending_set_linkpolicy_req(void) {
  TRACE(0, "Pending set link policy requests:");
  uint8_t index = pending_set_linkpolicy_out_cursor;
  while (index != pending_set_linkpolicy_in_cursor) {
    TRACE(3, "index %d RemDev %p LinkPolicy %d", index,
          pending_set_linkpolicy_req[index].remDev,
          pending_set_linkpolicy_req[index].policy);
    index++;
    if (PENDING_SET_LINKPOLICY_REQ_BUF_CNT == index) {
      index = 0;
    }
  }
}

static void app_bt_push_pending_set_linkpolicy(btif_remote_device_t *remDev,
                                               btif_link_policy_t policy) {
  // go through the existing pending list to see if the remDev is already in
  uint8_t index = pending_set_linkpolicy_out_cursor;
  while (index != pending_set_linkpolicy_in_cursor) {
    if (remDev == pending_set_linkpolicy_req[index].remDev) {
      pending_set_linkpolicy_req[index].policy = policy;
      return;
    }
    index++;
    if (PENDING_SET_LINKPOLICY_REQ_BUF_CNT == index) {
      index = 0;
    }
  }

  pending_set_linkpolicy_req[pending_set_linkpolicy_in_cursor].remDev = remDev;
  pending_set_linkpolicy_req[pending_set_linkpolicy_in_cursor].policy = policy;
  pending_set_linkpolicy_in_cursor++;
  if (PENDING_SET_LINKPOLICY_REQ_BUF_CNT == pending_set_linkpolicy_in_cursor) {
    pending_set_linkpolicy_in_cursor = 0;
  }

  app_bt_print_pending_set_linkpolicy_req();
}

BT_SET_LINKPOLICY_REQ_T *app_bt_pop_pending_set_linkpolicy(void) {
  if (pending_set_linkpolicy_out_cursor == pending_set_linkpolicy_in_cursor) {
    return NULL;
  }

  BT_SET_LINKPOLICY_REQ_T *ptReq =
      &pending_set_linkpolicy_req[pending_set_linkpolicy_out_cursor];
  pending_set_linkpolicy_out_cursor++;
  if (PENDING_SET_LINKPOLICY_REQ_BUF_CNT == pending_set_linkpolicy_out_cursor) {
    pending_set_linkpolicy_out_cursor = 0;
  }

  app_bt_print_pending_set_linkpolicy_req();
  return ptReq;
}

void app_bt_set_linkpolicy(btif_remote_device_t *remDev,
                           btif_link_policy_t policy) {
  if (btif_me_get_remote_device_state(remDev) == BTIF_BDS_CONNECTED) {
    bt_status_t ret = btif_me_set_link_policy(remDev, policy);
    TRACE(3, "%s policy %d returns %d", __FUNCTION__, policy, ret);

    osapi_lock_stack();
    if (BT_STS_IN_PROGRESS == ret) {
      app_bt_push_pending_set_linkpolicy(remDev, policy);
    }
    osapi_unlock_stack();
  }
}

#define COUNT_OF_PENDING_REMOTE_DEV_TO_EXIT_SNIFF_MODE 8
static btif_remote_device_t *pendingRemoteDevToExitSniffMode
    [COUNT_OF_PENDING_REMOTE_DEV_TO_EXIT_SNIFF_MODE];
static uint8_t maskOfRemoteDevPendingForExitingSniffMode = 0;
void app_check_pending_stop_sniff_op(void) {
  if (maskOfRemoteDevPendingForExitingSniffMode > 0) {
    for (uint8_t index = 0;
         index < COUNT_OF_PENDING_REMOTE_DEV_TO_EXIT_SNIFF_MODE; index++) {
      if (maskOfRemoteDevPendingForExitingSniffMode & (1 << index)) {
        btif_remote_device_t *remDev = pendingRemoteDevToExitSniffMode[index];
        if (!btif_me_is_op_in_progress(remDev)) {
          if (btif_me_get_remote_device_state(remDev) == BTIF_BDS_CONNECTED) {
            if (btif_me_get_current_mode(remDev) == BTIF_BLM_SNIFF_MODE) {
              TRACE(1, "!!! stop sniff currmode:%d\n",
                    btif_me_get_current_mode(remDev));
              bt_status_t ret = btif_me_stop_sniff(remDev);
              TRACE(1, "Return status %d", ret);
              if (BT_STS_IN_PROGRESS != ret) {
                maskOfRemoteDevPendingForExitingSniffMode &= (~(1 << index));
                break;
              }
            }
          }
        }
      }
    }

    if (maskOfRemoteDevPendingForExitingSniffMode > 0) {
      osapi_notify_evm();
    }
  }
}

static void app_add_pending_stop_sniff_op(btif_remote_device_t *remDev) {
  for (uint8_t index = 0;
       index < COUNT_OF_PENDING_REMOTE_DEV_TO_EXIT_SNIFF_MODE; index++) {
    if (maskOfRemoteDevPendingForExitingSniffMode & (1 << index)) {
      if (pendingRemoteDevToExitSniffMode[index] == remDev) {
        return;
      }
    }
  }

  for (uint8_t index = 0;
       index < COUNT_OF_PENDING_REMOTE_DEV_TO_EXIT_SNIFF_MODE; index++) {
    if (0 == (maskOfRemoteDevPendingForExitingSniffMode & (1 << index))) {
      pendingRemoteDevToExitSniffMode[index] = remDev;
      maskOfRemoteDevPendingForExitingSniffMode |= (1 << index);
    }
  }
}

extern "C" bt_status_t
app_tws_ibrt_set_access_mode(btif_accessible_mode_t mode);
void app_start_ble_adv_for_test(void);

static inline int app_bt_mail_process(APP_BT_MAIL *mail_p) {
  bt_status_t status = BT_STS_LAST_CODE;
  if (mail_p->request_id != CMGR_SetSniffTimer_req) {
    TRACE(3, "[BT_FUNC] src_thread:0x%08x call request_id=%x->:%s",
          mail_p->src_thread, mail_p->request_id,
          app_bt_func_table_str[mail_p->request_id]);
  }
  switch (mail_p->request_id) {
  case Me_switch_sco_req:
    status = btif_me_switch_sco(mail_p->param.Me_switch_sco_param.scohandle);
    break;
  case ME_SwitchRole_req:
    status = btif_me_switch_role(mail_p->param.ME_SwitchRole_param.remDev);
    break;
  case ME_SetConnectionRole_req:
    status =
        btif_me_set_connection_role(mail_p->param.BtConnectionRole_param.role);
    break;
  case MeDisconnectLink_req:
    status = btif_me_force_disconnect_link_with_reason(
        NULL, mail_p->param.MeDisconnectLink_param.remDev,
        BTIF_BEC_USER_TERMINATED, TRUE);
    break;
  case ME_StopSniff_req: {
    if (btif_me_get_remote_device_state(
            mail_p->param.ME_StopSniff_param.remDev) == BTIF_BDS_CONNECTED) {
      status = btif_me_stop_sniff(mail_p->param.ME_StopSniff_param.remDev);
      if (BT_STS_IN_PROGRESS == status) {
        app_add_pending_stop_sniff_op(mail_p->param.ME_StopSniff_param.remDev);
      }
    }
    break;
  }
  case ME_StartSniff_req: {
    if (btif_me_get_remote_device_state(
            mail_p->param.ME_StartSniff_param.remDev) == BTIF_BDS_CONNECTED) {
      status =
          btif_me_start_sniff(mail_p->param.ME_StartSniff_param.remDev,
                              &(mail_p->param.ME_StartSniff_param.sniffInfo));
    }
    break;
  }
  case BT_Control_SleepMode_req: {
    btif_me_write_bt_sleep_enable(
        mail_p->param.ME_BtControlSleepMode_param.isEnable);
    break;
  }
  case ME_SetAccessibleMode_req:
    app_set_accessmode(mail_p->param.ME_SetAccessibleMode_param.mode);
    break;
  case Me_SetLinkPolicy_req:
    app_bt_set_linkpolicy(mail_p->param.Me_SetLinkPolicy_param.remDev,
                          mail_p->param.Me_SetLinkPolicy_param.policy);
    break;
  case CMGR_SetSniffTimer_req:
    if (mail_p->param.CMGR_SetSniffTimer_param.SniffInfo.maxInterval == 0) {
      status = btif_cmgr_set_sniff_timer(
          mail_p->param.CMGR_SetSniffTimer_param.Handler, NULL,
          mail_p->param.CMGR_SetSniffTimer_param.Time);
    } else {
      status = btif_cmgr_set_sniff_timer(
          mail_p->param.CMGR_SetSniffTimer_param.Handler,
          &mail_p->param.CMGR_SetSniffTimer_param.SniffInfo,
          mail_p->param.CMGR_SetSniffTimer_param.Time);
    }
    break;
  case CMGR_SetSniffInofToAllHandlerByRemDev_req:
    status = btif_cmgr_set_sniff_info_to_all_handler_by_remdev(
        &mail_p->param.CMGR_SetSniffInofToAllHandlerByRemDev_param.SniffInfo,
        mail_p->param.CMGR_SetSniffInofToAllHandlerByRemDev_param.RemDev);
    break;
  case A2DP_OpenStream_req:
    status = btif_a2dp_open_stream(mail_p->param.A2DP_OpenStream_param.Stream,
                                   mail_p->param.A2DP_OpenStream_param.Addr);
    if ((BT_STS_NO_RESOURCES == status) || (BT_STS_IN_PROGRESS == status)) {
      app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
      app_bt_reset_reconnect_timer(mail_p->param.A2DP_OpenStream_param.Addr);
    } else {
      app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
    }
    break;
  case A2DP_CloseStream_req:
    status =
        btif_a2dp_close_stream(mail_p->param.A2DP_CloseStream_param.Stream);
    break;
  case A2DP_SetMasterRole_req:
    status =
        btif_a2dp_set_master_role(mail_p->param.A2DP_SetMasterRole_param.Stream,
                                  mail_p->param.A2DP_SetMasterRole_param.Flag);
    break;
  case HF_CreateServiceLink_req:
    status = btif_hf_create_service_link(
        mail_p->param.HF_CreateServiceLink_param.Chan,
        mail_p->param.HF_CreateServiceLink_param.Addr);
    if ((BT_STS_NO_RESOURCES == status) || (BT_STS_IN_PROGRESS == status)) {
      app_bt_accessmode_set(BTIF_BAM_CONNECTABLE_ONLY);
      app_bt_reset_reconnect_timer(
          mail_p->param.HF_CreateServiceLink_param.Addr);
    } else {
      app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
    }
    break;
  case HF_DisconnectServiceLink_req:
    status = btif_hf_disconnect_service_link(
        mail_p->param.HF_DisconnectServiceLink_param.Chan);
    break;
  case HF_CreateAudioLink_req:
    status =
        btif_hf_create_audio_link(mail_p->param.HF_CreateAudioLink_param.Chan);
    break;
  case HF_DisconnectAudioLink_req:
    status = btif_hf_disc_audio_link(
        mail_p->param.HF_DisconnectAudioLink_param.Chan);
    break;
  case HF_EnableSniffMode_req:
    status = btif_hf_enable_sniff_mode(
        mail_p->param.HF_EnableSniffMode_param.Chan,
        mail_p->param.HF_EnableSniffMode_param.Enable);
    break;
  case HF_SetMasterRole_req:
    status = btif_hf_set_master_role(mail_p->param.HF_SetMasterRole_param.Chan,
                                     mail_p->param.HF_SetMasterRole_param.Flag);
    break;
#ifdef BTIF_DIP_DEVICE
  case DIP_QuryService_req:
    status = btif_dip_query_for_service(
        mail_p->param.DIP_QuryService_param.dip_client,
        mail_p->param.DIP_QuryService_param.remDev);
    break;
#endif
#if defined(__HSP_ENABLE__)
  case HS_CreateServiceLink_req:
    app_bt_accessmode_set(BTIF_BAM_NOT_ACCESSIBLE);
    status =
        HS_CreateServiceLink(mail_p->param.HS_CreateServiceLink_param.Chan,
                             mail_p->param.HS_CreateServiceLink_param.Addr);
    break;
  case HS_CreateAudioLink_req:
    status = HS_CreateAudioLink(mail_p->param.HS_CreateAudioLink_param.Chan);
    break;
  case HS_DisconnectAudioLink_req:
    status =
        HS_DisconnectAudioLink(mail_p->param.HS_DisconnectAudioLink_param.Chan);
    break;
  case HS_DisconnectServiceLink_req:
    status = HS_DisconnectServiceLink(
        mail_p->param.HS_DisconnectServiceLink_param.Chan);
    break;
  case HS_EnableSniffMode_req:
    status = HS_EnableSniffMode(mail_p->param.HS_EnableSniffMode_param.Chan,
                                mail_p->param.HS_EnableSniffMode_param.Enable);
    break;
#endif
  case BT_Custom_Func_req:
    if (mail_p->param.CustomFunc_param.func_ptr) {
      TRACE(3, "func:0x%08x,param0:0x%08x, param1:0x%08x",
            mail_p->param.CustomFunc_param.func_ptr,
            mail_p->param.CustomFunc_param.param0,
            mail_p->param.CustomFunc_param.param1);
      ((APP_BTTHREAD_REQ_CUSTOMER_CALL_CB_T)(mail_p->param.CustomFunc_param
                                                 .func_ptr))(
          (void *)mail_p->param.CustomFunc_param.param0,
          (void *)mail_p->param.CustomFunc_param.param1);
    }
    break;
  }

  if (mail_p->request_id != CMGR_SetSniffTimer_req) {
    TRACE(2, "[BT_FUNC] exit request_id:%d :status:%d", mail_p->request_id,
          status);
  }
  return 0;
}

static inline int app_bt_mail_alloc(APP_BT_MAIL **mail) {
  *mail = (APP_BT_MAIL *)osMailAlloc(app_bt_mailbox, 0);
  ASSERT(*mail, "app_bt_mail_alloc error");
  return 0;
}

static inline int app_bt_mail_send(APP_BT_MAIL *mail) {
  osStatus status;

  ASSERT(mail, "osMailAlloc NULL");
  status = osMailPut(app_bt_mailbox, mail);
  ASSERT(osOK == status, "osMailAlloc Put failed");

  OS_NotifyEvm();

  return (int)status;
}

static inline int app_bt_mail_free(APP_BT_MAIL *mail_p) {
  osStatus status;

  status = osMailFree(app_bt_mailbox, mail_p);
  ASSERT(osOK == status, "osMailAlloc Put failed");

  return (int)status;
}

static inline int app_bt_mail_get(APP_BT_MAIL **mail_p) {
  osEvent evt;
  evt = osMailGet(app_bt_mailbox, 0);
  if (evt.status == osEventMail) {
    *mail_p = (APP_BT_MAIL *)evt.value.p;
    return 0;
  }
  return -1;
}

static void app_bt_mail_poll(void) {
  APP_BT_MAIL *mail_p = NULL;
  if (!app_bt_mail_get(&mail_p)) {
    app_bt_mail_process(mail_p);
    app_bt_mail_free(mail_p);
    osapi_notify_evm();
  }
}

int app_bt_mail_init(void) {
  app_bt_mailbox = osMailCreate(osMailQ(app_bt_mailbox), NULL);
  if (app_bt_mailbox == NULL) {
    TRACE(0, "Failed to Create app_mailbox\n");
    return -1;
  }
  Besbt_hook_handler_set(BESBT_HOOK_USER_1, app_bt_mail_poll);

  return 0;
}

int app_bt_Me_switch_sco(uint16_t scohandle) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = Me_switch_sco_req;
  mail->param.Me_switch_sco_param.scohandle = scohandle;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_ME_SwitchRole(btif_remote_device_t *remDev) {
#if !defined(IBRT)
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = ME_SwitchRole_req;
  mail->param.ME_SwitchRole_param.remDev = remDev;
  app_bt_mail_send(mail);
#endif
  return 0;
}

int app_bt_ME_SetConnectionRole(btif_connection_role_t role) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = ME_SetConnectionRole_req;
  mail->param.BtConnectionRole_param.role = role;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_MeDisconnectLink(btif_remote_device_t *remDev) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = MeDisconnectLink_req;
  mail->param.MeDisconnectLink_param.remDev = remDev;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_ME_StopSniff(btif_remote_device_t *remDev) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = ME_StopSniff_req;
  mail->param.ME_StopSniff_param.remDev = remDev;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_ME_StartSniff(btif_remote_device_t *remDev,
                         btif_sniff_info_t *sniffInfo) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = ME_StartSniff_req;
  mail->param.ME_StartSniff_param.remDev = remDev;
  mail->param.ME_StartSniff_param.sniffInfo = *sniffInfo;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_ME_ControlSleepMode(bool isEnabled) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = BT_Control_SleepMode_req;
  mail->param.ME_BtControlSleepMode_param.isEnable = isEnabled;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_ME_SetAccessibleMode(btif_accessible_mode_t mode,
                                const btif_access_mode_info_t *info) {
#if defined(BLE_ONLY_ENABLED)
  return 0;
#endif

  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = ME_SetAccessibleMode_req;
  mail->param.ME_SetAccessibleMode_param.mode = mode;
  memcpy(&mail->param.ME_SetAccessibleMode_param.info, info,
         sizeof(btif_access_mode_info_t));
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_Me_SetLinkPolicy(btif_remote_device_t *remDev,
                            btif_link_policy_t policy) {
#if !defined(IBRT)
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = Me_SetLinkPolicy_req;
  mail->param.Me_SetLinkPolicy_param.remDev = remDev;
  mail->param.Me_SetLinkPolicy_param.policy = policy;
  app_bt_mail_send(mail);
#endif
  return 0;
}

int app_bt_CMGR_SetSniffTimer(btif_cmgr_handler_t *Handler,
                              btif_sniff_info_t *SniffInfo, TimeT Time) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = CMGR_SetSniffTimer_req;
  mail->param.CMGR_SetSniffTimer_param.Handler = Handler;
  if (SniffInfo) {
    memcpy(&mail->param.CMGR_SetSniffTimer_param.SniffInfo, SniffInfo,
           sizeof(btif_sniff_info_t));
  } else {
    memset(&mail->param.CMGR_SetSniffTimer_param.SniffInfo, 0,
           sizeof(btif_sniff_info_t));
  }
  mail->param.CMGR_SetSniffTimer_param.Time = Time;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_CMGR_SetSniffInfoToAllHandlerByRemDev(btif_sniff_info_t *SniffInfo,
                                                 btif_remote_device_t *RemDev) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = CMGR_SetSniffInofToAllHandlerByRemDev_req;
  memcpy(&mail->param.CMGR_SetSniffInofToAllHandlerByRemDev_param.SniffInfo,
         SniffInfo, sizeof(btif_sniff_info_t));
  mail->param.CMGR_SetSniffInofToAllHandlerByRemDev_param.RemDev = RemDev;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_A2DP_OpenStream(a2dp_stream_t *Stream, bt_bdaddr_t *Addr) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = A2DP_OpenStream_req;
  mail->param.A2DP_OpenStream_param.Stream = Stream;
  mail->param.A2DP_OpenStream_param.Addr = Addr;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_A2DP_CloseStream(a2dp_stream_t *Stream) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = A2DP_CloseStream_req;
  mail->param.A2DP_CloseStream_param.Stream = Stream;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_A2DP_SetMasterRole(a2dp_stream_t *Stream, BOOL Flag) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = A2DP_SetMasterRole_req;
  mail->param.A2DP_SetMasterRole_param.Stream = Stream;
  mail->param.A2DP_SetMasterRole_param.Flag = Flag;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_CreateServiceLink(hf_chan_handle_t Chan, bt_bdaddr_t *Addr) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_CreateServiceLink_req;
  mail->param.HF_CreateServiceLink_param.Chan = Chan;
  mail->param.HF_CreateServiceLink_param.Addr = Addr;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_DisconnectServiceLink(hf_chan_handle_t Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_DisconnectServiceLink_req;
  mail->param.HF_DisconnectServiceLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_CreateAudioLink(hf_chan_handle_t Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_CreateAudioLink_req;
  mail->param.HF_CreateAudioLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_DisconnectAudioLink(hf_chan_handle_t Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_DisconnectAudioLink_req;
  mail->param.HF_DisconnectAudioLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_EnableSniffMode(hf_chan_handle_t Chan, BOOL Enable) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_EnableSniffMode_req;
  mail->param.HF_EnableSniffMode_param.Chan = Chan;
  mail->param.HF_EnableSniffMode_param.Enable = Enable;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HF_SetMasterRole(hf_chan_handle_t Chan, BOOL Flag) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HF_SetMasterRole_req;
  mail->param.HF_SetMasterRole_param.Chan = Chan;
  mail->param.HF_SetMasterRole_param.Flag = Flag;
  app_bt_mail_send(mail);
  return 0;
}

#ifdef BTIF_DIP_DEVICE
int app_bt_dip_QuryService(btif_dip_client_t *client,
                           btif_remote_device_t *rem) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = DIP_QuryService_req;
  mail->param.DIP_QuryService_param.remDev = rem;
  mail->param.DIP_QuryService_param.dip_client = client;
  app_bt_mail_send(mail);
  return 0;
}
#endif

#if defined(__HSP_ENABLE__)
int app_bt_HS_CreateServiceLink(HsChannel *Chan, bt_bdaddr_t *Addr) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HS_CreateServiceLink_req;
  mail->param.HS_CreateServiceLink_param.Chan = Chan;
  mail->param.HS_CreateServiceLink_param.Addr = Addr;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HS_CreateAudioLink(HsChannel *Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HS_CreateAudioLink_req;
  mail->param.HS_CreateAudioLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HS_DisconnectAudioLink(HsChannel *Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HS_DisconnectAudioLink_req;
  mail->param.HS_DisconnectAudioLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HS_DisconnectServiceLink(HsChannel *Chan) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HS_DisconnectServiceLink_req;
  mail->param.HS_DisconnectServiceLink_param.Chan = Chan;
  app_bt_mail_send(mail);
  return 0;
}

int app_bt_HS_EnableSniffMode(HsChannel *Chan, BOOL Enable) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = HS_EnableSniffMode_req;
  mail->param.HS_EnableSniffMode_param.Chan = Chan;
  mail->param.HS_EnableSniffMode_param.Enable = Enable;
  app_bt_mail_send(mail);
  return 0;
}

#endif
int app_bt_start_custom_function_in_bt_thread(uint32_t param0, uint32_t param1,
                                              uint32_t funcPtr) {
  APP_BT_MAIL *mail;
  app_bt_mail_alloc(&mail);
  mail->src_thread = (uint32_t)osThreadGetId();
  mail->request_id = BT_Custom_Func_req;
  mail->param.CustomFunc_param.func_ptr = funcPtr;
  mail->param.CustomFunc_param.param0 = param0;
  mail->param.CustomFunc_param.param1 = param1;
  app_bt_mail_send(mail);
  return 0;
}
