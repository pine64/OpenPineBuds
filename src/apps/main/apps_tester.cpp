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
#include "stdio.h"
#include "string.h"

#include "app_key.h"
#include "hal_iomux.h"
#include "hal_sleep.h"
#include "hal_trace.h"

#include "app_thread.h"
#include "app_utils.h"
#include "audiobuffer.h"
#include "audioflinger.h"
#include "besbt.h"
#include "bt_drv_interface.h"

#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
#include "anc_usb_app.h"
#include "usb_audio_app.h"
//#include "dualadc_audio_app.h"
#endif

#define APP_TESTER_CPU_WAKE_LOCK HAL_CPU_WAKE_LOCK_USER_3

extern "C" int hal_analogif_reg_write(unsigned short reg, unsigned short val);
extern "C" void OS_NotifyEvm(void);
extern void app_anc_usb_init(void);

#define REG(a) *(volatile uint32_t *)(a)

void bt_signaling_test(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  hal_cpu_wake_lock(APP_TESTER_CPU_WAKE_LOCK);
  btdrv_testmode_start();
  btdrv_enable_dut();
}

void bt_stack_test(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  btdrv_start_bt();
  BesbtInit();
}

void bt_ble_test(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  hal_cpu_wake_lock(APP_TESTER_CPU_WAKE_LOCK);
  btdrv_testmode_start();

  btdrv_hcioff();
  hal_iomux_set_uart1();
  btdrv_uart_bridge_loop();
}

void bt_test_104m(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  hal_analogif_reg_write(0x35, 0x0);
  hal_analogif_reg_write(0x36, 0x8000);
  hal_analogif_reg_write(0x37, 0x1000);
  hal_analogif_reg_write(0x31, 0xfd31);
  REG(0xd0350248) = 0X80C00000;
  hal_analogif_reg_write(0xC, 0x3790);
}

void bt_change_to_iic(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  hal_iomux_set_analog_i2c();
}

void bt_change_to_uart0(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  hal_iomux_set_uart0();
}

void app_switch_i2c_uart(APP_KEY_STATUS *status, void *param) {
  static uint32_t flag = 1;

  TRACE(2, "[%s] flag = %d", __func__, flag);
  if (flag) {
    bt_change_to_iic(NULL, NULL);
  } else {
    bt_change_to_uart0(NULL, NULL);
  }
  flag = !flag;
}

void test_power_off(APP_KEY_STATUS *status, void *param) {
  TRACE(0, "app_power_off\n");
}

extern APP_KEY_STATUS bt_key;

void test_bt_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);
  if (bt_key.code == 0xff) {
    bt_key.code = status->code;
    bt_key.event = status->event;
    OS_NotifyEvm();
  }
}

#ifdef __APP_TEST_SDMMC__
#include "app_sdmmc.h"

#define SD_BUF_SIZE (10)
uint8_t sd_buf[SD_BUF_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void test_sd_card() {
  sd_open();

  dump_data2sd(APP_SDMMC_DUMP_OPEN, NULL, 0);
  dump_data2sd(APP_SDMMC_DUMP_WRITE, sd_buf, SD_BUF_SIZE);
  dump_data2sd(APP_SDMMC_DUMP_CLOSE, NULL, 0);
}
#endif

#ifdef APP_TEST_AUDIO
extern void adc_looptester(bool on, enum AUD_IO_PATH_T input_path,
                           enum AUD_SAMPRATE_T sample_rate);
void test_codec_loop(APP_KEY_STATUS *status, void *param) {
  audio_buffer_init();
  adc_looptester(true, AUD_INPUT_PATH_MAINMIC, AUD_SAMPRATE_8000);
}

#ifdef ANC_APP
void test_anc(APP_KEY_STATUS *status, void *param) { anc_usb_app((bool)param); }

void test_usb_audio(APP_KEY_STATUS *status, void *param) {
  usb_audio_app((bool)param);
  // dualadc_audio_app((bool)param);
}
#endif
#endif

void bt_change_to_jlink(APP_KEY_STATUS *status, void *param) {
  hal_iomux_set_jtag();

  hal_cmu_jtag_clock_enable();
}

void bt_enable_tports(void) {
  hal_iomux_set_bt_tport();
  bt_drv_bt_tport_type_config();
}

#ifdef APP_TEST_AUDIO
extern void da_tester(uint8_t on);
void bt_test_dsp_process(APP_KEY_STATUS *status, void *param) { da_tester(1); }
#endif
#define MENU_TITLE_MAX_SIZE (50)
APP_KEY_HANDLE app_testcase[] = {
#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP},
     "USB AUDIO TEST OFF",
     test_usb_audio,
     (void *)0},
    {{APP_KEY_CODE_FN5, APP_KEY_EVENT_UP},
     "USB AUDIO TEST ON",
     test_usb_audio,
     (void *)1},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_UP}, "ANC TEST OFF", test_anc, (void *)0},
    {{APP_KEY_CODE_FN6, APP_KEY_EVENT_UP}, "ANC TEST ON", test_anc, (void *)1},
#else
#ifdef APP_TEST_AUDIO
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_LONGPRESS},
     "LONGPRESS: test_codec_loop",
     test_codec_loop,
     NULL},
#endif
    {{APP_KEY_CODE_FN1, APP_KEY_EVENT_UP},
     "bt_signaling_test",
     bt_signaling_test,
     NULL},
    {{APP_KEY_CODE_FN2, APP_KEY_EVENT_UP}, "bt gogogogo", bt_stack_test, NULL},
    {{APP_KEY_CODE_FN3, APP_KEY_EVENT_UP},
     "bt change to 104m",
     bt_test_104m,
     NULL},
    {{APP_KEY_CODE_FN4, APP_KEY_EVENT_UP}, "ble test mode", bt_ble_test, NULL},
#ifdef APP_TEST_AUDIO
    {{APP_KEY_CODE_FN5, APP_KEY_EVENT_UP},
     "dsp eq test",
     bt_test_dsp_process,
     NULL},
#endif
    {{APP_KEY_CODE_FN5, APP_KEY_EVENT_LONGPRESS},
     "LONGPRESS: bt volume up key",
     test_bt_key,
     NULL},

    {{APP_KEY_CODE_FN6, APP_KEY_EVENT_UP},
     "bt volume down key",
     test_bt_key,
     NULL},
    {{APP_KEY_CODE_FN6, APP_KEY_EVENT_LONGPRESS},
     "LONGPRESS: bt volume down key",
     test_bt_key,
     NULL},

    {{APP_KEY_CODE_FN7, APP_KEY_EVENT_CLICK},
     "bt function key",
     test_bt_key,
     NULL},
    {{APP_KEY_CODE_FN7, APP_KEY_EVENT_DOUBLECLICK},
     "DOUBLECLICK: bt function key",
     test_bt_key,
     NULL},
    {{APP_KEY_CODE_FN7, APP_KEY_EVENT_LONGPRESS},
     "LONGPRESS: bt function key",
     test_bt_key,
     NULL},

    {{APP_KEY_CODE_FN8, APP_KEY_EVENT_UP},
     "open jlink",
     bt_change_to_jlink,
     NULL},
    {{APP_KEY_CODE_FN9, APP_KEY_EVENT_UP},
     "iic_map2_P3_0",
     bt_change_to_iic,
     NULL},
    {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGPRESS},
     "LONGPRESS: power off",
     test_power_off,
     NULL},
#endif // !(APP_TEST_AUDIO && ANC_APP)
    {{0xff, APP_KEY_EVENT_NONE}, NULL, (uint32_t)NULL, 0},
};

int app_testcase_disp_menu(APP_KEY_HANDLE *testcase, bool printall) {
  char buf[MENU_TITLE_MAX_SIZE + 1];
  if (strlen(testcase->string) > (MENU_TITLE_MAX_SIZE - 15)) {
    TRACE(0, "string too long, please check again\n");
    return -1;
  }

  if (printall) {
    memset(buf, '-', sizeof(buf) - 3);
    buf[0] = '|';
    buf[MENU_TITLE_MAX_SIZE - 3] = '|';
    buf[MENU_TITLE_MAX_SIZE - 2] = '\r';
    buf[MENU_TITLE_MAX_SIZE - 1] = '\n';
    buf[MENU_TITLE_MAX_SIZE] = '\0';
    TRACE(1, "%s", buf);
    osDelay(1);
  }

  do {
    snprintf(buf, sizeof(buf), "|      (0x%X)%s", testcase->key_status.code,
             testcase->string);
    memset(buf + strlen(buf), ' ', sizeof(buf) - strlen(buf) - 3);
    buf[MENU_TITLE_MAX_SIZE - 3] = '|';
    buf[MENU_TITLE_MAX_SIZE - 2] = '\r';
    buf[MENU_TITLE_MAX_SIZE - 1] = '\n';
    buf[MENU_TITLE_MAX_SIZE] = '\0';
    TRACE(1, "%s", buf);
    testcase++;
  } while (testcase->key_status.code != 0xff && printall);

  if (printall) {
    memset(buf, '-', sizeof(buf) - 3);
    buf[0] = '|';
    buf[MENU_TITLE_MAX_SIZE - 3] = '|';
    buf[MENU_TITLE_MAX_SIZE - 2] = '\r';
    buf[MENU_TITLE_MAX_SIZE - 1] = '\n';
    buf[MENU_TITLE_MAX_SIZE] = '\0';
    TRACE(1, "%s", buf);
    osDelay(1);
  }

  return 0;
}

int app_testcase_key_response(APP_MESSAGE_BODY *msg_body) {
  uint8_t i = 0;
  APP_KEY_STATUS key_status;

  APP_KEY_GET_CODE(msg_body->message_id, key_status.code);
  APP_KEY_GET_EVENT(msg_body->message_id, key_status.event);

  if ((key_status.code) > (sizeof(app_testcase) / sizeof(APP_KEY_HANDLE)))
    return -1;

  for (i = 0; i < (sizeof(app_testcase) / sizeof(APP_KEY_HANDLE)); i++) {
    if (app_testcase[i].key_status.code == key_status.code &&
        (app_testcase[i].key_status.event == key_status.event))
      break;
  }

  if (i >= (sizeof(app_testcase) / sizeof(APP_KEY_HANDLE)))
    return -1;

  if (app_testcase[i].function != (uint32_t)NULL) {
    if (app_testcase[i].string != (uint32_t)NULL)
      app_testcase_disp_menu(&app_testcase[i], 0);
    ((APP_KEY_HANDLE_CB_T)app_testcase[i].function)(&key_status,
                                                    app_testcase[i].param);
  }

  return 0;
}

void app_test_init(void) {
  uint8_t i = 0;
  TRACE(1, "%s", __func__);
  for (i = 0; i < (sizeof(app_testcase) / sizeof(APP_KEY_HANDLE)); i++) {
    app_key_handle_registration(&app_testcase[i]);
  }
  app_testcase_disp_menu(app_testcase, 1);

#if defined(APP_TEST_AUDIO) && defined(ANC_APP)
  app_anc_usb_init();
#endif
}
