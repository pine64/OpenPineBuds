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
#include "app_factory_bt.h"
#include "app_battery.h"
#include "app_factory.h"
#include "app_utils.h"
#include "apps.h"
#include "bluetooth.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "cmsis_os.h"
#include "conmgr_api.h"
#include "hal_bootmode.h"
#include "hal_chipid.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "intersyshci.h"
#include "me_api.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "pmu.h"
#include "tgt_hardware.h"

#define APP_FACT_CPU_WAKE_LOCK HAL_CPU_WAKE_LOCK_USER_3

#ifdef __FACTORY_MODE_SUPPORT__
static uint8_t inquiry_buff[] = {0x01, 0x72, 0x77, 0xb0, 0x18, 0x57, 0x60, 0x01,
                                 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00};

static btif_cmgr_handler_t *app_factorymode_cmgrHandler;

static void bt_error_check_timer_handler(void const *param);
osTimerDef(bt_error_check_timer, bt_error_check_timer_handler);
static osTimerId bt_error_check_timer_id = NULL;
uint8_t test_mode_type = 0;
static void bt_error_check_timer_handler(void const *param) {
  // dump rssi
  bt_drv_rssi_dump_handler();
  // check BT core status
  if (bt_drv_error_check_handler()) {
    if (test_mode_type == 1) {
      hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE |
                          HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
    } else if (test_mode_type == 2) {
      hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE |
                          HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE);
    }
    hal_cmu_sys_reboot();
  }
}

static void app_factorymode_bt_inquiry_buff_update(void) {
  bt_bdaddr_t flsh_dongle_addr;
  int ret = -1;

  ret = nvrec_dev_get_dongleaddr(&flsh_dongle_addr);
  if (0 == ret) {
    memcpy((void *)&inquiry_buff[1], (void *)flsh_dongle_addr.address,
           BTIF_BD_ADDR_SIZE);
    DUMP8("0x%02x ", &inquiry_buff[2], BTIF_BD_ADDR_SIZE);
  }
}

static void app_factorymode_CmgrCallback(btif_cmgr_handler_t *cHandler,
                                         cmgr_event_t Event,
                                         bt_status_t Status) {
  APP_FACTORY_TRACE(4, "%s cHandler:%p Event:%d status:%d", __func__, cHandler,
                    Event, Status);
  if (Event == BTIF_CMEVENT_DATA_LINK_CON_CNF) {
    if (Status == BT_STS_SUCCESS) {
      APP_FACTORY_TRACE(0, "connect ok");
      app_factorymode_result_set(true);
      btif_cmgr_remove_data_link(cHandler);

    } else {
      APP_FACTORY_TRACE(0, "connect failed");
      app_factorymode_result_set(false);
    }
  }

  if (Event == BTIF_CMEVENT_DATA_LINK_DIS) {
    if (Status == BT_STS_SUCCESS) {
      APP_FACTORY_TRACE(0, "disconnect ok");
    } else {
      APP_FACTORY_TRACE(0, "disconnect failed");
    }
  }
}

static void app_factorymode_bt_InquiryResult_add(void) {
  U8 len = 15;
  bool rssi = false, extended = false;
  U8 *parm = (U8 *)inquiry_buff;

  /* Found one or more devices. Report to clients */
  APP_FACTORY_TRACE(4, "%s len:%d rssi:%d extended:%d", __func__, len, rssi,
                    extended);
  DUMP8("0x%02x ", parm, len);
  btif_me_inquiry_result_setup(parm, rssi, extended);
}

void app_factorymode_bt_create_connect(void) {
  bt_status_t status;
  bt_bdaddr_t *bdAddr = (bt_bdaddr_t *)(inquiry_buff + 1);

  status = btif_cmgr_create_data_link(app_factorymode_cmgrHandler, bdAddr);
  APP_FACTORY_TRACE(2, "%s:%d", __func__, status);
}

void app_factorymode_bt_init_connect(void) {
  app_factorymode_cmgrHandler = btif_cmgr_handler_create();

  btif_cmgr_register_handler(app_factorymode_cmgrHandler,
                             app_factorymode_CmgrCallback);
  app_factorymode_bt_inquiry_buff_update();
  app_factorymode_bt_InquiryResult_add();
}

extern osTimerId app_bt_accessmode_timer;

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
#define XTAL_FCAP_RANGE (0x1FF)
#else
#define XTAL_FCAP_RANGE (0xFF)
#endif

void app_factorymode_bt_xtalrangetest(APP_KEY_STATUS *status, void *param) {
  dev_addr_name devinfo;
  uint32_t fcap = 0;
  APP_FACTORY_TRACE(1, "%s", __func__);
#ifdef __WATCHER_DOG_RESET__
  app_wdt_close();
#endif
  hal_cpu_wake_lock(APP_FACT_CPU_WAKE_LOCK);
  app_stop_10_second_timer(APP_PAIR_TIMER_ID);
  app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
  if (app_bt_accessmode_timer) {
    osTimerStop(app_bt_accessmode_timer);
  }
  if (!bt_error_check_timer_id) {
    bt_error_check_timer_id =
        osTimerCreate(osTimer(bt_error_check_timer), osTimerPeriodic, NULL);
  }
  if (bt_error_check_timer_id != NULL) {
    osTimerStart(bt_error_check_timer_id, 1000);
  }
  test_mode_type = 1;
  app_status_indication_set(APP_STATUS_INDICATION_TESTMODE);
  pmu_sleep_en(0);
  BESHCI_Close();
  btdrv_hciopen();
  btdrv_hci_reset();

#ifndef BT_50_FUNCTION
  btdrv_sleep_config(0);
  osDelay(2000);
  btdrv_ins_patch_test_init();
  btdrv_feature_default();
#endif
  devinfo.btd_addr = bt_addr;
  devinfo.ble_addr = ble_addr;
  devinfo.localname = BT_LOCAL_NAME;
  nvrec_dev_localname_addr_init(&devinfo);
  btdrv_write_localinfo((char *)devinfo.localname,
                        strlen(devinfo.localname) + 1, devinfo.btd_addr);

  btdrv_vco_test_start(78);
  while (1) {
    btdrv_rf_set_xtal_fcap(fcap % XTAL_FCAP_RANGE, 1);
    osDelay(300);
    TRACE(2, "xtal tune:%d", fcap % XTAL_FCAP_RANGE);
    fcap++;
  }
}

void app_factorymode_bt_signalingtest(APP_KEY_STATUS *status, void *param) {
  dev_addr_name devinfo;
  APP_FACTORY_TRACE(1, "%s", __func__);
#ifdef __WATCHER_DOG_RESET__
  app_wdt_close();
#endif
  hal_cpu_wake_lock(APP_FACT_CPU_WAKE_LOCK);
  app_stop_10_second_timer(APP_PAIR_TIMER_ID);
  app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
  if (app_bt_accessmode_timer) {
    osTimerStop(app_bt_accessmode_timer);
  }
  if (!bt_error_check_timer_id) {
    bt_error_check_timer_id =
        osTimerCreate(osTimer(bt_error_check_timer), osTimerPeriodic, NULL);
  }
  if (bt_error_check_timer_id != NULL) {
    osTimerStart(bt_error_check_timer_id, 1000);
  }
  test_mode_type = 1;
  app_status_indication_set(APP_STATUS_INDICATION_TESTMODE);
  pmu_sleep_en(0);
  BESHCI_Close();
  btdrv_hciopen();
  btdrv_ins_patch_test_init();
  btdrv_hci_reset();

#ifndef BT_50_FUNCTION
  btdrv_sleep_config(0);
  osDelay(2000);
  btdrv_testmode_start();
  btdrv_feature_default();
#endif
  devinfo.btd_addr = bt_addr;
  devinfo.ble_addr = ble_addr;
  devinfo.localname = BT_LOCAL_NAME;
  devinfo.ble_name = BT_LOCAL_NAME;
  nvrec_dev_localname_addr_init(&devinfo);
#ifdef __IBRT_IBRT_TESTMODE__
  uint8_t ibrt_address[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  memcpy(bt_addr, ibrt_address, 6);
  memcpy(devinfo.btd_addr, ibrt_address, 6);
#endif
  btdrv_write_localinfo((char *)devinfo.localname,
                        strlen(devinfo.localname) + 1, devinfo.btd_addr);
  bt_drv_extra_config_after_init();
  btdrv_enable_dut();
#ifdef __IBRT_IBRT_TESTMODE__
  btdrv_enable_ibrt_test();
#endif
}

int app_battery_stop(void);
void app_factorymode_bt_nosignalingtest(APP_KEY_STATUS *status, void *param) {
  dev_addr_name devinfo;
  APP_FACTORY_TRACE(1, "%s", __func__);
#ifdef __WATCHER_DOG_RESET__
  app_wdt_close();
#endif
  hal_cpu_wake_lock(APP_FACT_CPU_WAKE_LOCK);
  app_stop_10_second_timer(APP_PAIR_TIMER_ID);
  app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
  app_status_indication_set(APP_STATUS_INDICATION_TESTMODE1);
  osTimerStop(app_bt_accessmode_timer);
  if (!bt_error_check_timer_id) {
    bt_error_check_timer_id =
        osTimerCreate(osTimer(bt_error_check_timer), osTimerPeriodic, NULL);
  }
  if (bt_error_check_timer_id != NULL) {
    osTimerStart(bt_error_check_timer_id, 1000);
  }
  test_mode_type = 2;
  app_battery_stop();
  pmu_sleep_en(0);
  BESHCI_Close();
  btdrv_hciopen();
  btdrv_ins_patch_test_init();
  bt_drv_reg_op_key_gen_after_reset(false);
  btdrv_hci_reset();
#ifndef BT_50_FUNCTION
  btdrv_sleep_config(0);
#endif
  osDelay(2000);
  btdrv_testmode_start();
#ifndef BT_50_FUNCTION
  btdrv_feature_default();
  devinfo.btd_addr = bt_addr;
  devinfo.ble_addr = ble_addr;
  devinfo.localname = BT_LOCAL_NAME;
  devinfo.ble_name = BT_LOCAL_NAME;
  nvrec_dev_localname_addr_init(&devinfo);
  btdrv_write_localinfo((char *)devinfo.localname,
                        strlen(devinfo.localname) + 1, devinfo.btd_addr);
#endif
  bt_drv_extra_config_after_init();
  btdrv_hcioff();
#ifdef __BT_DEBUG_TPORTS__
  {
    extern void bt_enable_tports(void);
    bt_enable_tports();
    // hal_iomux_tportopen();
  }
#endif
  btdrv_uart_bridge_loop();
}
int app_factorymode_bt_xtalcalib_proc(void) {
  uint32_t capval = 0x80;
  int nRet;

  APP_FACTORY_TRACE(1, "%s", __func__);
  hal_cpu_wake_lock(APP_FACT_CPU_WAKE_LOCK);
  APP_FACTORY_TRACE(1, "calib default, capval:%d", capval);
  btdrv_hciopen();
  btdrv_hci_reset();
#ifndef BT_50_FUNCTION
  btdrv_ins_patch_test_init();
#endif
  btdrv_hcioff();
  capval = 0x80;
  bt_drv_calib_open();
  nRet = bt_drv_calib_result_porc(&capval);
  bt_drv_calib_close();
  TRACE(2, "!!!!!!!!!!!!!!!!!!!!!!!!!!!calib ret:%d, capval:%d", nRet, capval);
  if (!nRet)
    nvrec_dev_set_xtal_fcap((unsigned int)capval);

  return nRet;
}

void app_factorymode_bt_xtalcalib(APP_KEY_STATUS *status, void *param) {
  APP_FACTORY_TRACE(1, "%s", __func__);
  app_factorymode_bt_xtalcalib_proc();
}

#endif
