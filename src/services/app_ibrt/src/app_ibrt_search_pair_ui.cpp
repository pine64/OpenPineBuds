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
#include "a2dp_decoder.h"
#include "app_battery.h"
#include "app_bt.h"
#include "app_ibrt_if.h"
#include "app_ibrt_peripheral_manager.h"
#include "app_ibrt_ui.h"
#include "app_status_ind.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt_trace.h"
#include "app_tws_if.h"
#include "app_vendor_cmd_evt.h"
#include "besbt.h"
#include "bluetooth.h"
#include "btapp.h"
#include "cmsis_os.h"
#include "factory_section.h"
#include "nvrecord.h"
#include "nvrecord_ble.h"
#include "nvrecord_env.h"
#include "stdlib.h"
#include <string.h>

extern "C" {}
#ifdef IBRT_SEARCH_UI
static void app_tws_inquiry_timeout_handler(void const *param);
osTimerDef(APP_TWS_INQ, app_tws_inquiry_timeout_handler);
static osTimerId app_tws_timer = NULL;

static void app_tws_delay_connect_handler(void const *param);
osTimerDef(APP_TWS_DELAY_CONNECT, app_tws_delay_connect_handler);
static osTimerId app_tws_delay_connect_timer = NULL;

static uint8_t tws_find_process = 0;
static uint8_t tws_inquiry_count = 0;
#define MAX_TWS_INQUIRY_TIMES 3
#define IBRT_MAX_SEARCH_TIME 10 /* 12.8s */

uint8_t tws_inq_addr_used;
typedef struct {
  uint8_t used;
  bt_bdaddr_t bdaddr;
} TWS_INQ_ADDR_STRUCT;
TWS_INQ_ADDR_STRUCT tws_inq_addr[5];

#define IBRT_SEARCH_DEBUG
#ifdef IBRT_SEARCH_DEBUG
#define TWSCON_DBLOG TRACE
#else
#define TWSCON_DBLOG(...)
#endif

uint8_t box_event = IBRT_NONE_EVENT;
static void app_box_handle_timehandler(void const *param);
osTimerDef(APP_BOX_HANDLE, app_box_handle_timehandler);
static osTimerId app_box_handle_timer = NULL;

static void app_box_handle_timehandler(void const *param) {
  uint8_t *box_event_ptr = (uint8_t *)param;
  uint8_t boxStatus = *box_event_ptr;
  TRACE(1, "box event:%d", boxStatus);
  app_ibrt_search_ui_init(true, boxStatus);
  app_ibrt_if_event_entry(boxStatus);
  if (IBRT_IN_BOX_CLOSED == boxStatus)
    app_ibrt_search_ui_init(true, boxStatus);
}

static void app_tws_inquiry_timeout_handler(void const *param) {

  TWSCON_DBLOG(0, "app_tws_inquiry_timeout_handler\n");
  btif_me_inquiry(BTIF_BT_IAC_LIAC, IBRT_MAX_SEARCH_TIME, 0);
}

static void app_tws_delay_connect_handler(void const *parma) {
  TWSCON_DBLOG(1, "%s", __func__);
  app_ibrt_if_enter_pairing_after_tws_connected();
}

bool app_tws_is_addr_in_tws_inq_array(const bt_bdaddr_t *addr) {
  uint8_t i;
  for (i = 0; i < sizeof(tws_inq_addr) / sizeof(tws_inq_addr[0]); i++) {
    if (tws_inq_addr[i].used == 1) {
      if (!memcmp(tws_inq_addr[i].bdaddr.address, addr->address,
                  BTIF_BD_ADDR_SIZE))
        return true;
    }
  }
  return false;
}
void app_tws_clear_tws_inq_array(void) {
  memset(&tws_inq_addr, 0, sizeof(tws_inq_addr));
}

int app_tws_fill_addr_to_array(const bt_bdaddr_t *addr) {
  uint8_t i;
  for (i = 0; i < sizeof(tws_inq_addr) / sizeof(tws_inq_addr[0]); i++) {
    if (tws_inq_addr[i].used == 0) {
      tws_inq_addr[i].used = 1;
      memcpy(tws_inq_addr[i].bdaddr.address, addr->address, BTIF_BD_ADDR_SIZE);
      return 0;
    }
  }

  return -1;
}

uint8_t app_tws_get_tws_addr_inq_num(void) {
  uint8_t i, count = 0;
  for (i = 0; i < sizeof(tws_inq_addr) / sizeof(tws_inq_addr[0]); i++) {
    if (tws_inq_addr[i].used == 1) {
      count++;
    }
  }
  return count;
}
/*****************************************************************************
 Prototype    : app_tws_ibrt_update_info
 Description  : config tws info
 Input        : ibrt_role_e twsRole
                bt_bdaddr_t *twsAddr

 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/3/26
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_tws_ibrt_update_info(ibrt_role_e ibrtRole, bt_bdaddr_t *ibrtPeerAddr) {
  TRACE(1, "%s", __func__);
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW)
    p_ibrt_ctrl->nv_role = ibrtRole;
  if (NULL != ibrtPeerAddr) {
    memcpy(p_ibrt_ctrl->peer_addr.address, ibrtPeerAddr->address, BD_ADDR_LEN);
    nv_record_update_ibrt_info(p_ibrt_ctrl->nv_role, ibrtPeerAddr);
  }
}

void tws_app_stop_find(void) {
  tws_find_process = 0;
  btif_me_unregister_globa_handler((btif_handler *)btif_me_get_bt_handler());
}

void app_bt_manager_ibrt_role_process(const btif_event_t *Event) {
  switch (btif_me_get_callback_event_type(Event)) {
  case BTIF_BTEVENT_LINK_CONNECT_IND:
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
    if (BTIF_BEC_NO_ERROR == btif_me_get_callback_event_err_code(Event)) {
      bt_bdaddr_t *p_remote_dev_addr = NULL;
      ibrt_ctrl_t *p_ibrt_ctrl = NULL;
      p_remote_dev_addr = btif_me_get_callback_event_rem_dev_bd_addr(Event);
      p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
      if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW)
        factory_section_original_btaddr_get(p_ibrt_ctrl->local_addr.address);

      TRACE(2, "local:%x remd:%x", p_ibrt_ctrl->local_addr.address[5],
            p_remote_dev_addr->address[5]);
      if ((p_ibrt_ctrl->local_addr.address[3] ==
           p_remote_dev_addr->address[3]) &&
          (p_ibrt_ctrl->local_addr.address[4] ==
           p_remote_dev_addr->address[4]) &&
          (p_ibrt_ctrl->local_addr.address[5] ==
           p_remote_dev_addr->address[5])) {
        app_tws_ibrt_update_info(IBRT_SLAVE, p_remote_dev_addr);
        if (app_ibrt_ui_get_tws_use_same_addr_enable()) {
          memcpy(p_ibrt_ctrl->local_addr.address, p_remote_dev_addr->address,
                 6);
        }
        if (app_tws_is_left_side()) {
          p_ibrt_ctrl->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LCHNL;
        } else {
          p_ibrt_ctrl->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        }

        // #if 0
        //                     p_ibrt_ctrl->audio_chnl_sel =
        //                     A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
        // #else
        //                     if(IBRT_MASTER == p_ibrt_ctrl->nv_role)
        //                     {
        //                         TRACE(0,"#right");
        //                         p_ibrt_ctrl->audio_chnl_sel =
        //                         A2DP_AUDIO_CHANNEL_SELECT_RCHNL;
        //                     }
        //                     else if(IBRT_SLAVE == p_ibrt_ctrl->nv_role)
        //                     {
        //                         TRACE(0,"#left");
        //                         p_ibrt_ctrl->audio_chnl_sel =
        //                         A2DP_AUDIO_CHANNEL_SELECT_LCHNL;

        //                     }
        // #endif
      }
    }
    break;
  default:
    break;
  }
}

void app_ibrt_config_the_same_bd_addr(bt_bdaddr_t *ibrtSearchedAddr) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  app_tws_ibrt_set_access_mode(BTIF_BAM_NOT_ACCESSIBLE);

  memcpy(p_ibrt_ctrl->local_addr.address, ibrtSearchedAddr->address,
         BD_ADDR_LEN);
  memcpy(p_ibrt_ctrl->peer_addr.address, ibrtSearchedAddr->address,
         BD_ADDR_LEN);
  btif_me_set_bt_address(ibrtSearchedAddr->address);

#ifdef __GMA_VOICE__
  btif_me_set_ble_bd_address(ibrtSearchedAddr->address);
  bt_set_ble_local_address(ibrtSearchedAddr->address);
  NV_EXTENSION_RECORD_T *pNvExtRec = nv_record_get_extension_entry_ptr();
  memcpy(pNvExtRec->tws_info.ble_info.ble_addr, ibrtSearchedAddr->address,
         BLE_IRK_SIZE);
  nv_record_tws_exchange_ble_info();
#endif
  TRACE(1, "%s", __func__);
  DUMP8("%02x ", p_ibrt_ctrl->local_addr.address, BD_ADDR_LEN);

  // after change the bd_addr, we should reset access mode again
  app_tws_ibrt_set_access_mode(BTIF_BAM_CONNECTABLE_ONLY);
}

void app_ibrt_reconfig_btAddr_from_nv() {
  struct nvrecord_env_t *nvrecord_env;
  if (nv_record_env_get(&nvrecord_env) != -1) {
    if (nvrecord_env->ibrt_mode.mode != IBRT_UNKNOW) {
      TRACE(0, "reconfig addr from nv");
      DUMP8("%02x ", nvrecord_env->ibrt_mode.record.bdAddr.address, 6);
      bt_set_local_address(nvrecord_env->ibrt_mode.record.bdAddr.address);

      bt_set_ble_local_address(nvrecord_env->ibrt_mode.record.bdAddr.address);
    }
  }
}
void app_bt_inquiry_call_back(const btif_event_t *event) {
  TWSCON_DBLOG(2, "\nenter: %s %d\n", __func__, __LINE__);
  uint8_t device_name[64];
  uint8_t device_name_len;
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  factory_section_original_btaddr_get(p_ibrt_ctrl->local_addr.address);

  switch (btif_me_get_callback_event_type(event)) {
  case BTIF_BTEVENT_NAME_RESULT:
    TWSCON_DBLOG(2, "\n%s %d BTEVENT_NAME_RESULT\n", __func__, __LINE__);
    break;
  case BTIF_BTEVENT_INQUIRY_RESULT:
    TWSCON_DBLOG(2, "\n%s %d BTEVENT_INQUIRY_RESULT\n", __func__, __LINE__);
    DUMP8("%02x ", btif_me_get_callback_event_inq_result_bd_addr_addr(event),
          6);
    TWSCON_DBLOG(1, "inqmode = %x",
                 btif_me_get_callback_event_inq_result_inq_mode(event));
    DUMP8("%02x ", btif_me_get_callback_event_inq_result_ext_inq_resp(event),
          20);
    /// check the uap and nap if equal ,get the name for tws slave
    TRACE(1, "##RSSI:%d", (int8_t)btif_me_get_callback_event_rssi(event));
    TRACE(
        6, "local %02x %02x %02x %02x %02x %02x\n",
        p_ibrt_ctrl->local_addr.address[0], p_ibrt_ctrl->local_addr.address[1],
        p_ibrt_ctrl->local_addr.address[2], p_ibrt_ctrl->local_addr.address[3],
        p_ibrt_ctrl->local_addr.address[4], p_ibrt_ctrl->local_addr.address[5]);
    if ((btif_me_get_callback_event_inq_result_bd_addr(event)->address[5] ==
         p_ibrt_ctrl->local_addr.address[5]) &&
        (btif_me_get_callback_event_inq_result_bd_addr(event)->address[4] ==
         p_ibrt_ctrl->local_addr.address[4]) &&
        (btif_me_get_callback_event_inq_result_bd_addr(event)->address[3] ==
         p_ibrt_ctrl->local_addr.address[3])) {
      /// check the device is already checked
      TWSCON_DBLOG(0, "<1>");
      if (app_tws_is_addr_in_tws_inq_array(
              btif_me_get_callback_event_inq_result_bd_addr(event))) {
        break;
      }
      ////if rssi event is eir,so find name derictly
      if (btif_me_get_callback_event_inq_result_inq_mode(event) ==
          BTIF_INQ_MODE_EXTENDED) {

        TWSCON_DBLOG(0, "<2>");
        uint8_t *eir =
            (uint8_t *)btif_me_get_callback_event_inq_result_ext_inq_resp(
                event);
        // device_name_len =
        // ME_GetExtInqData(eir,0x09,device_name,sizeof(device_name));
        device_name_len = btif_me_get_ext_inq_data(eir, 0x09, device_name,
                                                   sizeof(device_name));
        if (device_name_len > 0) {
          TWSCON_DBLOG(3, "<3> search name len %d %s local name %s\n",
                       device_name_len, device_name, bt_get_local_name());
          ////if name is the same as the local name so we think the device is
          /// the tws slave
          if (!memcmp(device_name, bt_get_local_name(), device_name_len)) {
            TWSCON_DBLOG(0, "<4>");

            // modify local addr
            app_ibrt_config_the_same_bd_addr(
                btif_me_get_callback_event_inq_result_bd_addr(event));
            btif_me_cancel_inquiry();
            osTimerStop(app_tws_timer);
            tws_app_stop_find();
            app_tws_ibrt_update_info(
                IBRT_MASTER,
                btif_me_get_callback_event_inq_result_bd_addr(event));
            p_ibrt_ctrl->audio_chnl_sel = A2DP_AUDIO_CHANNEL_SELECT_LRMERGE;
            if (NULL != app_tws_delay_connect_timer) {
              osTimerStop(app_tws_delay_connect_timer);
              osTimerStart(app_tws_delay_connect_timer, 500);
            }

          } else {
            if (app_tws_get_tws_addr_inq_num() <
                sizeof(tws_inq_addr) / sizeof(tws_inq_addr[0])) {
              app_tws_fill_addr_to_array(
                  btif_me_get_callback_event_inq_result_bd_addr(event));
              if (app_tws_get_tws_addr_inq_num() ==
                  sizeof(tws_inq_addr) / sizeof(tws_inq_addr[0])) {
                /// fail to find a tws slave
                btif_me_cancel_inquiry();
                tws_app_stop_find();
              }
            } else {
              /// fail to find a tws slave
              btif_me_cancel_inquiry();
              tws_app_stop_find();
            }
          }
          break;
        }
        /////have no name so just wait for next device
        //////we can do remote name req for tws slave if eir can't received
        /// correctly
      }
    }
    break;
  case BTIF_BTEVENT_INQUIRY_COMPLETE:
    TWSCON_DBLOG(2, "\n%s %d BTEVENT_INQUIRY_COMPLETE\n", __FUNCTION__,
                 __LINE__);
    if (tws_inquiry_count >= MAX_TWS_INQUIRY_TIMES) {
      tws_app_stop_find();
      return;
    }
    if (p_ibrt_ui->super_state == IBRT_UI_IDLE) {
      ////inquiry complete if bt don't find any slave ,so do inquiry again

      uint8_t rand_delay = rand() % 5;
      tws_inquiry_count++;

      if (rand_delay == 0) {
        // btif_me_inquiry(BTIF_BT_IAC_GIAC, IBRT_MAX_SEARCH_TIME, 0);
        btif_me_inquiry(BTIF_BT_IAC_LIAC, IBRT_MAX_SEARCH_TIME, 0);
      } else {
        osTimerStart(app_tws_timer, rand_delay * 1000);
      }
    }
    break;
  /** The Inquiry process is canceled. */
  case BTIF_BTEVENT_INQUIRY_CANCELED:
    TWSCON_DBLOG(2, "\n%s %d BTEVENT_INQUIRY_CANCELED\n", __FUNCTION__,
                 __LINE__);
    // tws.notify(&tws);
    break;
  case BTIF_BTEVENT_LINK_CONNECT_CNF:
    TWSCON_DBLOG(3, "\n%s %d BTEVENT_LINK_CONNECT_CNF stats=%x\n", __FUNCTION__,
                 __LINE__, btif_me_get_callback_event_err_code(event));

    // connect fail start inquiry again
    if (btif_me_get_callback_event_err_code(event) == 4 &&
        tws_find_process == 1) {
      if (tws_inquiry_count >= MAX_TWS_INQUIRY_TIMES) {
        tws_app_stop_find();
        return;
      }
      uint8_t rand_delay = rand() % 5;
      tws_inquiry_count++;
      if (rand_delay == 0) {
        // btif_me_inquiry(BTIF_BT_IAC_GIAC, IBRT_MAX_SEARCH_TIME, 0);
        btif_me_inquiry(BTIF_BT_IAC_LIAC, IBRT_MAX_SEARCH_TIME, 0);
      } else {
        osTimerStart(app_tws_timer, rand_delay * 1000);
      }
    }
    /// connect succ,so stop the finding tws procedure
    else if (btif_me_get_callback_event_err_code(event) == 0) {
      tws_app_stop_find();
    }
    break;
  case BTIF_BTEVENT_LINK_CONNECT_IND:
    TWSCON_DBLOG(3, "\n%s %d BTEVENT_LINK_CONNECT_IND stats=%x\n", __FUNCTION__,
                 __LINE__, btif_me_get_callback_event_err_code(event));
    ////there is a incoming connect so cancel the inquiry and the timer and the
    /// connect creating
    btif_me_cancel_inquiry();
    osTimerStop(app_tws_timer);
    break;
  default:
    // TWS_DBLOG("\n%s %d etype:%d\n",__FUNCTION__,__LINE__,event->eType);
    break;
  }

  // TWS_DBLOG("\nexit: %s %d\n",__FUNCTION__,__LINE__);
}

uint8_t is_find_tws_peer_device_onprocess(void) { return tws_find_process; }

void find_tws_peer_device_start(void) {
  TWSCON_DBLOG(2, "\nibrt_ui_log: %s %d\n", __func__, __LINE__);
  bt_status_t status;
  app_tws_clear_tws_inq_array();
  if (tws_find_process == 0) {
    tws_find_process = 1;
    tws_inquiry_count = 0;
    if (app_tws_timer == NULL)
      app_tws_timer = osTimerCreate(osTimer(APP_TWS_INQ), osTimerOnce, NULL);
    btif_me_set_handler(btif_me_get_bt_handler(), app_bt_inquiry_call_back);
    btif_me_register_global_handler(btif_me_get_bt_handler());

    btif_me_set_event_mask(
        btif_me_get_bt_handler(),
        BTIF_BEM_LINK_DISCONNECT | BTIF_BEM_ROLE_CHANGE |
            BTIF_BEM_INQUIRY_RESULT | BTIF_BEM_INQUIRY_COMPLETE |
            BTIF_BEM_INQUIRY_CANCELED | BTIF_BEM_LINK_CONNECT_CNF |
            BTIF_BEM_LINK_CONNECT_IND);

  again:
    TWSCON_DBLOG(2, "\n%s %d\n", __func__, __LINE__);

    status = btif_me_inquiry(BTIF_BT_IAC_LIAC, IBRT_MAX_SEARCH_TIME, 0);
    TWSCON_DBLOG(2, "\n%s %d\n", __func__, __LINE__);
    if (status != BT_STS_PENDING) {
      osDelay(500);
      goto again;
    }
    TWSCON_DBLOG(2, "\n%s %d\n", __func__, __LINE__);
  }
}

void find_tws_peer_device_stop(void) {
  btif_me_cancel_inquiry();
  tws_app_stop_find();
}

void app_start_tws_serching_direactly() {
  btif_accessible_mode_t mode;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();

  mode = p_ibrt_ctrl->access_mode;
  TWSCON_DBLOG(1, "ibrt_ui_log:search tws direactly access_mode:%d", mode);
  // if
  // ((BTIF_DEFAULT_ACCESS_MODE_PAIR==mode)||(BTIF_BAM_LIMITED_ACCESSIBLE==mode))
  if (BTIF_BAM_LIMITED_ACCESSIBLE == mode) {
    if (NULL == app_tws_delay_connect_timer)
      app_tws_delay_connect_timer =
          osTimerCreate(osTimer(APP_TWS_DELAY_CONNECT), osTimerOnce, NULL);
    if (is_find_tws_peer_device_onprocess()) {
      find_tws_peer_device_stop();
    } else {
      p_ibrt_ctrl->nv_role = IBRT_UNKNOW;
      find_tws_peer_device_start();

      // app_status_indication_set(APP_STATUS_INDICATION_CONNECTING);
    }
  }
}

static void
app_ibrt_battery_handle_process_normal(uint32_t status,
                                       union APP_BATTERY_MSG_PRAMS prams) {
  app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();

  switch (status) {
  case APP_BATTERY_STATUS_CHARGING:

    TWSCON_DBLOG(1, "charger:%d", prams.charger);
    if (prams.charger == APP_BATTERY_CHARGER_PLUGIN) {
      ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
      TWSCON_DBLOG(1, "APP_BATTERY_CHARGER_PLUGIN nv_role %02x",
                   p_ibrt_ctrl->nv_role);
      /*if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW)
      {
          return;
      }*/
      if (p_ui_ctrl->config.check_plugin_excute_closedbox_event == true)
        box_event = IBRT_CLOSE_BOX_EVENT;
      else
        box_event = IBRT_PUT_IN_EVENT;

      if (app_box_handle_timer != NULL) {
        osTimerStop(app_box_handle_timer);
        osTimerStart(app_box_handle_timer, 500);
      }

    } else if (prams.charger == APP_BATTERY_CHARGER_PLUGOUT) {
      ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
      TWSCON_DBLOG(1, "APP_BATTERY_CHARGER_PLUGOUT nv_role %02x",
                   p_ibrt_ctrl->nv_role);
      if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW) {
        return;
      }
      box_event = IBRT_FETCH_OUT_EVENT;

      if (app_box_handle_timer != NULL) {
        osTimerStop(app_box_handle_timer);
        osTimerStart(app_box_handle_timer, 500);
      }
    }
    break;
  case APP_BATTERY_STATUS_INVALID:
  default:
    break;
  }
}

void app_ibrt_battery_callback(APP_BATTERY_MV_T currvolt, uint8_t currlevel,
                               enum APP_BATTERY_STATUS_T curstatus,
                               uint32_t status,
                               union APP_BATTERY_MSG_PRAMS prams) {
  switch (curstatus) {
  case APP_BATTERY_STATUS_NORMAL:
  case APP_BATTERY_STATUS_CHARGING:
    app_ibrt_battery_handle_process_normal(status, prams);
    break;

  default:
    break;
  }
}

#ifdef BOX_DET_USE_GPIO
#define BOX_DET_PIN HAL_IOMUX_PIN_P1_0

static void box_det_pin_irq_set(enum HAL_GPIO_IRQ_POLARITY_T polarity);

static void box_det_pin_irq_update(void) {
  if (hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)BOX_DET_PIN))
    box_det_pin_irq_set(HAL_GPIO_IRQ_POLARITY_LOW_FALLING);
  else
    box_det_pin_irq_set(HAL_GPIO_IRQ_POLARITY_HIGH_RISING);
}

static void box_det_handler(uint32_t val) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  TRACE(2, "%s: %d", __func__, p_ibrt_ctrl->nv_role);
  box_det_pin_irq_update();
  if (val) {
    box_event = IBRT_FETCH_OUT_EVENT;
  } else {
    box_event = IBRT_CLOSE_BOX_EVENT;
  }

  if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW)
    return;

  if (app_box_handle_timer) {
    osTimerStop(app_box_handle_timer);
    osTimerStart(app_box_handle_timer, 500);
  }
}

static void box_det_irq_handler(enum HAL_GPIO_PIN_T pin) {
  uint8_t val = hal_gpio_pin_get_val(pin);
  TRACE(3, "%s: %d, %d", __func__, pin, val);
  app_ibrt_peripheral_run1((uint32_t)box_det_handler, (uint32_t)val);
}

static void box_det_pin_irq_set(enum HAL_GPIO_IRQ_POLARITY_T polarity) {
  struct HAL_GPIO_IRQ_CFG_T box_det_pin_cfg;
  box_det_pin_cfg.irq_debounce = true;
  box_det_pin_cfg.irq_handler = box_det_irq_handler;
  box_det_pin_cfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;

  box_det_pin_cfg.irq_enable = true;
  box_det_pin_cfg.irq_polarity = polarity;
  hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)BOX_DET_PIN, &box_det_pin_cfg);
}

static void box_det_pin_init(void) {
  const struct HAL_IOMUX_PIN_FUNCTION_MAP box_det_gpio_cfg = {
      BOX_DET_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE};

  hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&box_det_gpio_cfg, 1);
  hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)box_det_gpio_cfg.pin,
                       HAL_GPIO_DIR_IN, 1);

  box_det_pin_irq_update();
}
#endif

void app_ibrt_search_ui_init(bool boxOperation, ibrt_event_type evt_type) {
  app_ibrt_ui_t *p_ui_ctrl = app_ibrt_ui_get_ctx();
  if ((true == p_ui_ctrl->config.check_plugin_excute_closedbox_event) ||
      (false == boxOperation))
    p_ui_ctrl->box_state = IBRT_IN_BOX_OPEN;

  if (false == boxOperation) {
#ifdef BOX_DET_USE_GPIO
    box_det_pin_init();
#else
    app_battery_register(app_ibrt_battery_callback);
#endif

    if (app_box_handle_timer == NULL)
      app_box_handle_timer =
          osTimerCreate(osTimer(APP_BOX_HANDLE), osTimerOnce, &box_event);
  } else if (evt_type != IBRT_IN_BOX_CLOSED) {
    app_ibrt_ui_judge_scan_type(IBRT_OPEN_BOX_TRIGGER, NO_LINK_TYPE,
                                IBRT_UI_NO_ERROR);
  }
}

void app_ibrt_remove_history_paired_device(void) {
  bt_status_t retStatus;
  btif_device_record_t record;
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  int paired_dev_count = nv_record_get_paired_dev_count();

  TWSCON_DBLOG(0, "Remove all history tws nv records.");
  TWSCON_DBLOG(0, "Master addr:");
  DUMP8("%02x ", p_ibrt_ctrl->local_addr.address, BTIF_BD_ADDR_SIZE);
  TWSCON_DBLOG(0, "Slave addr:");
  DUMP8("%02x ", p_ibrt_ctrl->peer_addr.address, BTIF_BD_ADDR_SIZE);

  for (int32_t index = paired_dev_count - 1; index >= 0; index--) {
    retStatus = nv_record_enum_dev_records(index, &record);
    if (BT_STS_SUCCESS == retStatus) {
      TWSCON_DBLOG(1, "The index %d of nv records:", index);
      DUMP8("%02x ", record.bdAddr.address, BTIF_BD_ADDR_SIZE);
      if (!memcmp(record.bdAddr.address, p_ibrt_ctrl->local_addr.address,
                  BTIF_BD_ADDR_SIZE) ||
          !memcmp(record.bdAddr.address, p_ibrt_ctrl->peer_addr.address,
                  BTIF_BD_ADDR_SIZE)) {
        nv_record_ddbrec_delete(&record.bdAddr);
        TWSCON_DBLOG(1, "Delete the nv record entry %d", index);
      }
    }
  }

  memset(p_ibrt_ctrl->local_addr.address, 0, BTIF_BD_ADDR_SIZE);
  memset(p_ibrt_ctrl->peer_addr.address, 0, BTIF_BD_ADDR_SIZE);
}

void app_bt_enter_mono_pairing_mode(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  p_ibrt_ui->box_state = IBRT_OUT_BOX;

  TRACE(0, "ibrt_ui_log:app_bt_enter_mono_pairing_mode");

  if (!app_device_bt_is_connected()) {
    if (p_ibrt_ctrl->nv_role == IBRT_UNKNOW) {
      app_ibrt_ui_judge_scan_type(IBRT_FREEMAN_PAIR_TRIGGER, NO_LINK_TYPE,
                                  IBRT_UI_NO_ERROR);
      app_ibrt_ui_set_freeman_enable();
    } else {
      // app_tws_ibrt_set_access_mode(BTIF_BAM_GENERAL_ACCESSIBLE);
      app_ibrt_ui_set_enter_pairing_mode(IBRT_CONNECT_MOBILE_FAILED);
      app_ibrt_ui_judge_scan_type(IBRT_CONNECTE_TRIGGER, MOBILE_LINK, 0);
    }
  }
}
void app_ibrt_enter_limited_mode(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  app_ibrt_ui_t *p_ibrt_ui = app_ibrt_ui_get_ctx();

  p_ibrt_ctrl->nv_role = IBRT_UNKNOW;
  p_ibrt_ui->box_state = IBRT_OUT_BOX;

  app_ibrt_remove_history_paired_device();
  TRACE(0, "ibrt_ui_log:power on enter pairing");
  app_ibrt_ui_judge_scan_type(IBRT_SEARCH_SLAVE_TRIGGER, NO_LINK_TYPE,
                              IBRT_UI_NO_ERROR);
}

#endif
