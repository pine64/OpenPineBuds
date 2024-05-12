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
// #include "mbed.h"
#include "analog.h"
#include "app_bt_func.h"
#include "app_bt_stream.h"
#include "app_status_ind.h"
#include "app_utils.h"
#include "apps.h"
#include "audioflinger.h"
#include "besbt_cfg.h"
#include "bt_if.h"
#include "cmsis_os.h"
#include "dip_api.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "hfp_api.h"
#include "lockcqueue.h"
#include "nvrecord_dev.h"
#include "os_api.h"
#include "rwapp_config.h"
#include "string.h"
#include "tgt_hardware.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(ENHANCED_STACK)
#include "sdp_api.h"
#endif
#ifdef BTIF_DIP_DEVICE
#include "app_dip.h"
#endif

#ifdef TEST_OVER_THE_AIR_ENANBLED
#include "app_tota.h"
#endif
extern "C" {
#ifdef __IAG_BLE_INCLUDE__
#include "besble.h"
#endif
#ifdef TX_RX_PCM_MASK
#include "app_audio.h"
#include "app_bt_stream.h"
#include "hal_intersys.h"
#endif
#include "bt_drv_interface.h"

#ifdef __GATT_OVER_BR_EDR__
#include "app_btgatt.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif

#ifdef __AI_VOICE__
#include "app_ai_if.h"
#endif

#ifdef GFPS_ENABLED
#include "app_fp_rfcomm.h"
#endif

void BESHCI_Open(void);
void BESHCI_Poll(void);
void BESHCI_SCO_Data_Start(void);
void BESHCI_SCO_Data_Stop(void);
void BESHCI_LockBuffer(void);
void BESHCI_UNLockBuffer(void);
}
#include "besbt.h"

#include "app_bt.h"
#include "btapp.h"
#include "cqueue.h"
#if defined(__BTMAP_ENABLE__)
#include "app_btmap_sms.h"
#endif

#if defined(IBRT)
#include "app_ibrt_if.h"
#include "app_ibrt_peripheral_manager.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#endif

#ifdef __AI_VOICE__
#include "ai_thread.h"
#endif

#if defined(IBRT)
rssi_t raw_rssi[2];
#endif

struct besbt_cfg_t besbt_cfg = {
#ifdef __BTIF_SNIFF__
    .sniff = true,
#else
    .sniff = false,
#endif

    .force_use_cvsd = false,

#ifdef __BT_ONE_BRING_TWO__
    .one_bring_two = true,
#else
    .one_bring_two = false,
#endif
#ifdef __A2DP_AVDTP_CP__
    .avdtp_cp_enable = true,
#else
    .avdtp_cp_enable = false,
#endif
#if defined(APP_LINEIN_A2DP_SOURCE) || defined(APP_I2S_A2DP_SOURCE)
    .source_enable = true,
#else
    .source_enable = false,
#endif
#ifdef A2DP_LHDC_V3
    .lhdc_v3 = true,
#else
    .lhdc_v3 = false,
#endif
};

osMessageQDef(evm_queue, 128, uint32_t);
osMessageQId evm_queue_id;

/* besbt thread */
#ifndef BESBT_STACK_SIZE
#if defined(ENHANCED_STACK)
#ifdef __IAG_BLE_INCLUDE__
#define BESBT_STACK_SIZE 6144
#else
#define BESBT_STACK_SIZE (3326)
#endif
#else
#ifdef __IAG_BLE_INCLUDE__
#define BESBT_STACK_SIZE 5120
#else
#if defined(IBRT)
#define BESBT_STACK_SIZE (3328)
#else
#define BESBT_STACK_SIZE (2304)
#endif
#endif
#endif
#endif

osThreadDef(BesbtThread, (osPriorityAboveNormal), 1, (BESBT_STACK_SIZE),
            "bes_bt_main");

static BESBT_HOOK_HANDLER bt_hook_handler[BESBT_HOOK_USER_QTY] = {0};

int Besbt_hook_handler_set(enum BESBT_HOOK_USER_T user,
                           BESBT_HOOK_HANDLER handler) {
  bt_hook_handler[user] = handler;
  return 0;
}

static void Besbt_hook_proc(void) {
  uint8_t i;
  for (i = 0; i < BESBT_HOOK_USER_QTY; i++) {
    if (bt_hook_handler[i]) {
      bt_hook_handler[i]();
    }
  }
}

extern struct BT_DEVICE_T app_bt_device;

extern void a2dp_init(void);
extern void app_hfp_init(void);

unsigned char *bt_get_local_address(void) { return bt_addr; }

void bt_set_local_address(unsigned char *btaddr) {
  if (btaddr != NULL) {
    memcpy(bt_addr, btaddr, BTIF_BD_ADDR_SIZE);
  }
}

void bt_set_ble_local_address(uint8_t *bleAddr) {
  if (bleAddr) {
    memcpy(ble_addr, bleAddr, BTIF_BD_ADDR_SIZE);
  }
}

unsigned char *bt_get_ble_local_address(void) { return ble_addr; }

const char *bt_get_local_name(void) { return BT_LOCAL_NAME; }

void bt_set_local_name(const char *name) {
  if (name != NULL) {
    BT_LOCAL_NAME = name;
  }
}

const char *bt_get_ble_local_name(void) { return BLE_DEFAULT_NAME; }

void bt_key_init(void);
void pair_handler_func(enum pair_event evt, const btif_event_t *event);
#ifdef BTIF_SECURITY
void auth_handler_func();
#endif

typedef void (*bt_hci_delete_con_send_complete_cmd_func)(uint16_t handle,
                                                         uint8_t num);
extern "C" void register_hci_delete_con_send_complete_cmd_callback(
    bt_hci_delete_con_send_complete_cmd_func func);
extern "C" void HciSendCompletePacketCommandRightNow(uint16_t handle,
                                                     uint8_t num);

void gen_bt_addr_for_debug(void) {
  static const char host[] = TO_STRING(BUILD_HOSTNAME);
  static const char user[] = TO_STRING(BUILD_USERNAME);
  uint32_t hlen, ulen;
  uint32_t i, j;
  uint32_t addr_size = BTIF_BD_ADDR_SIZE;

  hlen = strlen(host);
  ulen = strlen(user);

  TRACE(0, "Configured BT addr is:");
  DUMP8("%02x ", bt_addr, BTIF_BD_ADDR_SIZE);

  j = 0;
  for (i = 0; i < hlen; i++) {
    bt_addr[j++] ^= host[i];
    if (j >= addr_size / 2) {
      j = 0;
    }
  }

  j = addr_size / 2;
  for (i = 0; i < ulen; i++) {
    bt_addr[j++] ^= user[i];
    if (j >= addr_size) {
      j = addr_size / 2;
    }
  }

  TRACE(0, "Modified debug BT addr is:");
  DUMP8("%02x ", bt_addr, BTIF_BD_ADDR_SIZE);
}

#if !defined(ENHANCED_STACK)
static void __set_local_dev_name(void) {
  dev_addr_name devinfo;

  devinfo.btd_addr = bt_get_local_address();
  devinfo.ble_addr = bt_get_ble_local_address();
  devinfo.localname = bt_get_local_name();
  devinfo.ble_name = bt_get_ble_local_name();

  nvrec_dev_localname_addr_init(&devinfo);
  bt_set_local_dev_name((const unsigned char *)devinfo.localname,
                        strlen(devinfo.localname) + 1);
}
#endif

static void add_randomness(void) {
  uint32_t generatedSeed = hal_sys_timer_get();

  // avoid bt address collision low probability
  for (uint8_t index = 0; index < sizeof(bt_addr); index++) {
    generatedSeed ^=
        (((uint32_t)(bt_addr[index])) << (hal_sys_timer_get() & 0xF));
  }
  srand(generatedSeed);
}

static void __set_bt_sco_num(void) {
  uint8_t sco_num;

#ifdef CHIP_BEST1000
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
#endif
  {
    sco_num = 2;
  }
#ifdef CHIP_BEST1000
  else {
    sco_num = 1;
  }
#endif
#if defined(__BT_ONE_BRING_TWO__) //&&defined(HFP_NO_PRERMPT)
  sco_num = 1;
#endif
  bt_set_sco_number(sco_num);
}

#if defined(ENHANCED_STACK)
void app_notify_stack_ready(uint8_t ready_flag);
static void stack_ready_callback(int status) {
  dev_addr_name devinfo;

  devinfo.btd_addr = bt_get_local_address();
  devinfo.ble_addr = bt_get_ble_local_address();
  devinfo.localname = bt_get_local_name();
  devinfo.ble_name = bt_get_ble_local_name();

  nvrec_dev_localname_addr_init(&devinfo);
  bt_set_local_dev_name((const unsigned char *)devinfo.localname,
                        strlen(devinfo.localname) + 1);

  bt_stack_config((const unsigned char *)devinfo.localname,
                  strlen(devinfo.localname) + 1);

  app_notify_stack_ready(STACK_READY_BT);
}
#endif /* ENHANCED_STACK */

int besmain(void) {
  enum APP_SYSFREQ_FREQ_T sysfreq;

#if !defined(BLE_ONLY_ENABLED)
#ifdef A2DP_CP_ACCEL
  sysfreq = APP_SYSFREQ_26M;
#else
  sysfreq = APP_SYSFREQ_52M;
#endif
#else
  sysfreq = APP_SYSFREQ_26M;
#endif

  BESHCI_Open();
#if defined(TX_RX_PCM_MASK)
  if (btdrv_is_pcm_mask_enable() == 1)
    hal_intersys_mic_open(HAL_INTERSYS_ID_1, store_encode_frame2buff);
#endif
  __set_bt_sco_num();
  add_randomness();

#ifdef __IAG_BLE_INCLUDE__
  bes_ble_init();
#endif

  btif_set_btstack_chip_config(bt_drv_get_btstack_chip_config());

  /* bes stack init */
  bt_stack_initilize();

#if defined(ENHANCED_STACK)
  bt_stack_register_ready_callback(stack_ready_callback);
  btif_sdp_init();
#endif

#if defined(ENHANCED_STACK)
  btif_cmgr_handler_init();
#endif

  a2dp_init();
  btif_avrcp_init(&app_bt_device);

#ifdef __AI_VOICE__
  app_ai_voice_init();
#endif
#if defined(VOICE_DATAPATH)
  app_voicepath_init();
#endif
#ifdef GFPS_ENABLED
  app_fp_rfcomm_init();
#endif

  app_hfp_init();
#if defined(__HSP_ENABLE_)
  app_hsp_init();
#endif
#if defined(__BTMAP_ENABLE__)
  app_btmap_sms_init();
#endif
#if defined(__GATT_OVER_BR_EDR__)
  app_btgatt_init();
#endif

  /* pair callback init */
  bt_pairing_init(pair_handler_func);
  bt_authing_init(auth_handler_func);

  a2dp_hid_init();
  a2dp_codec_init();

#ifdef BTIF_HID_DEVICE
  app_bt_hid_init();
#endif

#ifdef BTIF_DIP_DEVICE
  app_dip_init();
#endif

#if defined(ENHANCED_STACK)
  // register_hci_delete_con_send_complete_cmd_callback(HciSendCompletePacketCommandRightNow);

  /* bt local name */
  /*
  nvrec_dev_localname_addr_init(&devinfo);
  */
#else

  register_hci_delete_con_send_complete_cmd_callback(
      HciSendCompletePacketCommandRightNow);

  __set_local_dev_name();
#endif
#if defined(IBRT)
  app_ibrt_set_cmdhandle(TWS_CMD_IBRT, app_ibrt_cmd_table_get);
  app_ibrt_set_cmdhandle(TWS_CMD_CUSTOMER, app_ibrt_customif_cmd_table_get);
#if defined(IBRT_OTA) || defined(__GMA_OTA_TWS__) || defined(BISTO_ENABLED)
  app_ibrt_set_cmdhandle(TWS_CMD_IBRT_OTA, app_ibrt_ota_tws_cmd_table_get);
#endif
#ifdef __INTERACTION__
  app_ibrt_set_cmdhandle(TWS_CMD_OTA, app_ibrt_ota_cmd_table_get);
#endif
  tws_ctrl_thread_init();
  app_ibrt_peripheral_thread_init();
#endif

#if defined(APP_LINEIN_A2DP_SOURCE)
  app_source_init();
#endif

#if defined(ENHANCED_STACK)
  /*
  __set_local_dev_name();
  bt_stack_config();
  */
#else
  bt_stack_config();
#endif
  // init bt key
  bt_key_init();
#ifdef TEST_OVER_THE_AIR_ENANBLED
  app_tota_init();
#endif
  osapi_notify_evm();
  while (1) {
    app_sysfreq_req(APP_SYSFREQ_USER_BT_MAIN, APP_SYSFREQ_32K);
    osMessageGet(evm_queue_id, osWaitForever);
    app_sysfreq_req(APP_SYSFREQ_USER_BT_MAIN, sysfreq);
    //    BESHCI_LockBuffer();
#ifdef __LOCK_AUDIO_THREAD__
    bool stream_a2dp_sbc_isrun = app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC);
    if (stream_a2dp_sbc_isrun) {
      af_lock_thread();
    }
#endif
    bt_process_stack_events();

#ifdef __IAG_BLE_INCLUDE__
    bes_ble_schedule();
#endif

    Besbt_hook_proc();

#ifdef __LOCK_AUDIO_THREAD__
    if (stream_a2dp_sbc_isrun) {
      af_unlock_thread();
    }
#endif
    // BESHCI_UNLockBuffer();
    BESHCI_Poll();

#if defined(IBRT)
    app_ibrt_data_send_handler();
    app_ibrt_data_receive_handler();
    app_ibrt_ui_controller_dbg_state_checker();
    app_ibrt_ui_stop_ibrt_condition_checker();
#endif
    app_check_pending_stop_sniff_op();
  }

  return 0;
}

void BesbtThread(void const *argument) { besmain(); }
osThreadId besbt_tid;
void BesbtInit(void) {

  evm_queue_id = osMessageCreate(osMessageQ(evm_queue), NULL);
  /* bt */
  besbt_tid = osThreadCreate(osThread(BesbtThread), NULL);
  TRACE(1, "BesbtThread: %p\n", besbt_tid);
}
