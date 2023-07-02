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
#include "app_factory.h"
#include "app_bt_stream.h"
#include "app_key.h"
#include "app_media_player.h"
#include "bluetooth.h"
#include "bt_drv_interface.h"
#include "bt_drv_reg_op.h"
#include "cmsis_os.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "list.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "pmu.h"
#include "resources.h"

// for init
#include "app_battery.h"
#include "app_key.h"
#include "app_overlay.h"
#include "app_pwl.h"
#include "app_status_ind.h"
#include "app_thread.h"
#include "app_utils.h"
#include "apps.h"

// for bt
#include "app_bt.h"
#include "app_factory_bt.h"
#include "besbt.h"

// for audio
#include "app_audio.h"
#include "app_utils.h"
#include "audioflinger.h"

// for progress
#include "hal_uart.h"
#include "tool_msg.h"

#include "factory_section.h"

#ifdef __FACTORY_MODE_SUPPORT__

#define APP_FACTORYMODE_RETRY_LIMITED (2)

typedef enum APP_FACTORYMODE_STATUS_INDICATION_T {
  APP_FACTORYMODE_STATUS_INDICATION_RUNNING = 0,
  APP_FACTORYMODE_STATUS_INDICATION_PASS,
  APP_FACTORYMODE_STATUS_INDICATION_FAILED,
  APP_FACTORYMODE_STATUS_INDICATION_INVALID,

  APP_FACTORYMODE_STATUS_INDICATION_NUM
} APP_FACTORYMODE_STATUS_INDICATION_T;

static void app_factorymode_timehandler(void const *param);
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
void app_factorymode_result_set(bool result);

static osThreadId app_factorymode_tid = NULL;
static struct message_t send_msg = {
    {
        PREFIX_CHAR,
    },
};
static unsigned char send_seq = 0;

osTimerId app_factory_timer = NULL;
osTimerDef(APP_FACTORY_TIMER, app_factorymode_timehandler);

int app_factorymode_languageswitch_proc(void) {
#ifdef MEDIA_PLAYER_SUPPORT
  int lan;
  int new_lan;
  struct nvrecord_env_t *nvrecord_env;

  APP_FACTORY_TRACE(1, "%s", __func__);
  lan = app_play_audio_get_lang();
  new_lan = lan;
  app_play_audio_set_lang(new_lan);

  nv_record_env_get(&nvrecord_env);
  nvrecord_env->media_language.language = new_lan;
  nv_record_env_set(nvrecord_env);

  APP_FACTORY_TRACE(2, "languages old:%d new:%d", lan, new_lan);
  media_PlayAudio(AUD_ID_LANGUAGE_SWITCH, 0);
#endif
  return 0;
}

void app_factorymode_languageswitch(APP_KEY_STATUS *status, void *param) {
  app_factorymode_languageswitch_proc();
}

void app_factorymode_enter(void) {
  APP_FACTORY_TRACE(1, "%s", __func__);
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_TEST_MODE |
                      HAL_SW_BOOTMODE_TEST_SIGNALINGMODE);
  hal_cmu_sys_reboot();
}

extern "C" {

static bool isInFactoryMode = false;

bool app_factorymode_get(void) { return isInFactoryMode; }

void app_factorymode_set(bool set) { isInFactoryMode = set; }
}
#ifdef POWERKEY_I2C_SWITCH
void app_factorymode_i2c_switch(APP_KEY_STATUS *status, void *param) {
  static int i = 0;

  i++;
  if (i & 1) {
    TRACE(0, "set analog i2c mode !!!");
    osDelay(100);
    hal_iomux_set_analog_i2c();
  } else {
    hal_iomux_set_uart0();
    osDelay(100);
    TRACE(0, "hal_iomux_set_uart0 !!!");
  }
}
#endif

#ifdef __IBRT_IBRT_TESTMODE__
void bt_drv_ibrt_test_key_click(APP_KEY_STATUS *status, void *param);
void bt_drv_ibrt_test_key_click(APP_KEY_STATUS *status, void *param) {
  btdrv_connect_ibrt_device(bt_addr);
}
#endif

void app_factorymode_key_init(void) {
  const APP_KEY_HANDLE app_factorymode_handle_cfg[] = {
#ifdef POWERKEY_I2C_SWITCH
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_RAMPAGECLICK},
       "bt i2c key",
       app_factorymode_i2c_switch,
       NULL},
#endif
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_UP},
       "bt function key",
       app_bt_key_shutdown,
       NULL},
#else
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGLONGPRESS},
       "bt function key",
       app_bt_key_shutdown,
       NULL},
#endif
#ifdef __IBRT_IBRT_TESTMODE__
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
       "bt function key",
       bt_drv_ibrt_test_key_click,
       NULL},
#else
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
       "bt function key",
       app_factorymode_languageswitch,
       NULL},
#endif
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_DOUBLECLICK},
       "bt function key",
       app_factorymode_bt_xtalcalib,
       NULL},
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_LONGPRESS},
       "bt function key",
       app_factorymode_bt_signalingtest,
       NULL},
  };

  uint8_t i = 0;

  APP_FACTORY_TRACE(1, "%s", __func__);

  app_key_handle_clear();
  for (i = 0; i < (sizeof(app_factorymode_handle_cfg) / sizeof(APP_KEY_HANDLE));
       i++) {
    app_key_handle_registration(&app_factorymode_handle_cfg[i]);
  }
}

static void app_factorymode_audioloopswitch(APP_KEY_STATUS *status,
                                            void *param) {
  static bool onaudioloop = false;

  onaudioloop = onaudioloop ? false : true;

  if (onaudioloop)
    app_audio_sendrequest(APP_FACTORYMODE_AUDIO_LOOP,
                          (uint8_t)APP_BT_SETTING_OPEN, 0);
  else
    app_audio_sendrequest(APP_FACTORYMODE_AUDIO_LOOP,
                          (uint8_t)APP_BT_SETTING_CLOSE, 0);
}

void app_factorymode_test_key_init(void) {
  const APP_KEY_HANDLE app_factorymode_handle_cfg[] = {
      {{APP_KEY_CODE_PWR, APP_KEY_EVENT_CLICK},
       "bt function key",
       app_factorymode_audioloopswitch,
       NULL},
  };

  uint8_t i = 0;
  APP_FACTORY_TRACE(1, "%s", __func__);
  for (i = 0; i < (sizeof(app_factorymode_handle_cfg) / sizeof(APP_KEY_HANDLE));
       i++) {
    app_key_handle_registration(&app_factorymode_handle_cfg[i]);
  }
}

void app_factorymode_result_clean(void) {
  osSignalClear(app_factorymode_tid, 0x01);
  osSignalClear(app_factorymode_tid, 0x02);
}

void app_factorymode_result_set(bool result) {
  if (result)
    osSignalSet(app_factorymode_tid, 0x01);
  else
    osSignalSet(app_factorymode_tid, 0x02);
}

bool app_factorymode_result_wait(void) {
  bool nRet;
  osEvent evt;

  while (1) {
    // wait any signal
    evt = osSignalWait(0x0, osWaitForever);

    // get role from signal value
    if (evt.status == osEventSignal) {
      if (evt.value.signals & 0x01) {
        nRet = true;
        break;
      } else if (evt.value.signals & 0x02) {
        nRet = false;
        break;
      }
    }
  }
  return nRet;
}

static int app_factorymode_send_progress(uint8_t progress) {
  APP_MESSAGE_BLOCK msg;

  msg.mod_id = APP_MODUAL_OHTER;
  msg.msg_body.message_id = 2;
  msg.msg_body.message_Param0 = progress;
  app_mailbox_put(&msg);

  return 0;
}

static int app_factorymode_send_code(uint32_t progress) {
  APP_MESSAGE_BLOCK msg;

  msg.mod_id = APP_MODUAL_OHTER;
  msg.msg_body.message_id = 3;
  msg.msg_body.message_Param0 = progress;
  app_mailbox_put(&msg);

  return 0;
}

int app_factorymode_proc(void) {
  uint8_t cnt = 0;
  bool nRet;
  app_factorymode_tid = osThreadGetId();

  app_factorymode_send_progress(60);
  app_factorymode_bt_init_connect();

  do {
    app_factorymode_result_clean();
    app_factorymode_bt_create_connect();
    nRet = app_factorymode_result_wait();
  } while (!nRet && ++cnt < APP_FACTORYMODE_RETRY_LIMITED);

  if (!nRet)
    goto exit;
  app_factorymode_send_progress(90);
  app_factorymode_result_clean();
  if (!nRet)
    goto exit;
  app_factorymode_send_progress(100);
  osDelay(100);
exit:
  app_factorymode_result_clean();
  if (nRet) {
    return 0;
  } else {
    return -1;
  }
}

static unsigned char app_factorymode_msg_check_sum(unsigned char *buf,
                                                   unsigned char len) {
  int i;
  unsigned char sum = 0;

  for (i = 0; i < len; i++) {
    sum += buf[i];
  }

  return sum;
}

static int app_factorymode_msg_uart_send(const unsigned char *buf, size_t len) {
  uint32_t sent = 0;

  while (sent < len) {
    hal_uart_blocked_putc(HAL_UART_ID_0, buf[sent++]);
  }

  if (sent != len) {
    return 1;
  }

  return 0;
}

static int app_factorymode_msg_send_ping(void) {
  int ret;

  send_msg.hdr.type = 0x88;
  send_msg.hdr.seq = send_seq++;
  send_msg.hdr.len = 2;
  send_msg.data[0] = 0xaa;
  send_msg.data[1] = 0x55;
  send_msg.data[2] = ~app_factorymode_msg_check_sum(
      (unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

  ret = app_factorymode_msg_uart_send((unsigned char *)&send_msg,
                                      MSG_TOTAL_LEN(&send_msg));

  return ret;
}

static int app_factorymode_msg_send_progress(uint8_t progress) {
  int ret;

  send_msg.hdr.type = 0x88;
  send_msg.hdr.seq = send_seq++;
  send_msg.hdr.len = 2;
  send_msg.data[0] = progress;
  send_msg.data[1] = 100;
  send_msg.data[2] = ~app_factorymode_msg_check_sum(
      (unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

  ret = app_factorymode_msg_uart_send((unsigned char *)&send_msg,
                                      MSG_TOTAL_LEN(&send_msg));

  return ret;
}

static int app_factorymode_msg_send_32bitcode(uint32_t code) {
  int ret;

  send_msg.hdr.type = 0x88;
  send_msg.hdr.seq = send_seq++;
  send_msg.hdr.len = 4;
  send_msg.data[0] = 0xf2;
  *(uint32_t *)&(send_msg.data[1]) = code;
  send_msg.data[4] = ~app_factorymode_msg_check_sum(
      (unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

  ret = app_factorymode_msg_uart_send((unsigned char *)&send_msg,
                                      MSG_TOTAL_LEN(&send_msg));

  return ret;
}

static int app_factorymode_process(APP_MESSAGE_BODY *msg_body) {
  if (msg_body->message_id == 1) {
    app_factorymode_msg_send_ping();
  }
  if (msg_body->message_id == 2) {
    app_factorymode_msg_send_progress(msg_body->message_Param0);
  }
  if (msg_body->message_id == 3) {
    app_factorymode_msg_send_32bitcode(msg_body->message_Param0);
  }
  return 0;
}

static int app_factorymode_uart_init(void) {
  struct HAL_UART_CFG_T uart_cfg;

  memset(&uart_cfg, 0, sizeof(struct HAL_UART_CFG_T));
  uart_cfg.parity = HAL_UART_PARITY_NONE, uart_cfg.stop = HAL_UART_STOP_BITS_1,
  uart_cfg.data = HAL_UART_DATA_BITS_8,
  uart_cfg.flow = HAL_UART_FLOW_CONTROL_NONE, // HAL_UART_FLOW_CONTROL_RTSCTS,
      uart_cfg.tx_level = HAL_UART_FIFO_LEVEL_1_2,
  uart_cfg.rx_level = HAL_UART_FIFO_LEVEL_1_4, uart_cfg.baud = 921600,
  uart_cfg.dma_rx = false, uart_cfg.dma_tx = false,
  uart_cfg.dma_rx_stop_on_err = false;
  hal_uart_close(HAL_UART_ID_0);
  hal_uart_open(HAL_UART_ID_0, &uart_cfg);

  return 0;
}

static void app_factorymode_timehandler(void const *param) {
  APP_MESSAGE_BLOCK msg;

  msg.mod_id = APP_MODUAL_OHTER;
  msg.msg_body.message_id = 1;
  app_mailbox_put(&msg);
}

static uint8_t app_factorymode_indication_init(void) {
  struct APP_PWL_CFG_T cfg;

  memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
  app_pwl_open();
  app_pwl_setup(APP_PWL_ID_0, &cfg);
  app_pwl_setup(APP_PWL_ID_1, &cfg);
  return 0;
}

static uint8_t
app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_T status) {
  struct APP_PWL_CFG_T cfg0;
  struct APP_PWL_CFG_T cfg1;
  APP_FACTORY_TRACE(2, "%s %d", __func__, status);
  memset(&cfg0, 0, sizeof(struct APP_PWL_CFG_T));
  memset(&cfg1, 0, sizeof(struct APP_PWL_CFG_T));
  app_pwl_stop(APP_PWL_ID_0);
  app_pwl_stop(APP_PWL_ID_1);
  switch (status) {
  case APP_FACTORYMODE_STATUS_INDICATION_RUNNING:
    cfg0.part[0].level = 0;
    cfg0.part[0].time = (300);
    cfg0.part[1].level = 1;
    cfg0.part[1].time = (300);
    cfg0.parttotal = 2;
    cfg0.startlevel = 0;
    cfg0.periodic = true;

    cfg1.part[0].level = 1;
    cfg1.part[0].time = (300);
    cfg1.part[1].level = 0;
    cfg1.part[1].time = (300);
    cfg1.parttotal = 2;
    cfg1.startlevel = 1;
    cfg1.periodic = true;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);
    break;
  case APP_FACTORYMODE_STATUS_INDICATION_PASS:
    cfg0.part[0].level = 1;
    cfg0.part[0].time = (5000);
    cfg0.parttotal = 1;
    cfg0.startlevel = 1;
    cfg0.periodic = false;

    app_pwl_setup(APP_PWL_ID_0, &cfg0);
    app_pwl_start(APP_PWL_ID_0);
    break;
  case APP_FACTORYMODE_STATUS_INDICATION_FAILED:
    cfg1.part[0].level = 1;
    cfg1.part[0].time = (5000);
    cfg1.parttotal = 1;
    cfg1.startlevel = 1;
    cfg1.periodic = false;

    app_pwl_setup(APP_PWL_ID_1, &cfg1);
    app_pwl_start(APP_PWL_ID_1);

    break;

  default:
    break;
  }
  return 0;
}

int app_factorymode_init(uint32_t factorymode) {
  uint8_t cnt = 0;
  int nRet = 0;
  uint32_t capval = 0x00;
  struct nvrecord_env_t *nvrecord_env;
  APP_FACTORY_TRACE(1, "app_factorymode_init mode:%x\n", factorymode);

  osThreadSetPriority(osThreadGetId(), osPriorityRealtime);
  app_factorymode_uart_init();
#ifdef __WATCHER_DOG_RESET__
  app_wdt_open(60);
#endif
  app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
  list_init();
  app_os_init();
  app_factorymode_indication_init();
  app_battery_open();
  if (app_key_open(false)) {
    nRet = -1;
    goto exit;
  }
  app_set_threadhandle(APP_MODUAL_OHTER, app_factorymode_process);
  app_factory_timer =
      osTimerCreate(osTimer(APP_FACTORY_TIMER), osTimerPeriodic, NULL);
  osTimerStart(app_factory_timer, 300);
  app_factorymode_send_progress(10);

  app_bt_init();
  af_open();
  app_audio_open();
  app_overlay_open();

  nv_record_env_init();
  nvrec_dev_data_open();
  nv_record_env_get(&nvrecord_env);
#ifdef MEDIA_PLAYER_SUPPORT
  app_play_audio_set_lang(nvrecord_env->media_language.language);
  app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
  app_status_indication_set(APP_STATUS_INDICATION_POWERON);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_RUNNING);

  if (factorymode & HAL_SW_BOOTMODE_CALIB) {
    btdrv_start_bt();
    app_factorymode_send_progress(20);

    do {
      nRet = app_factorymode_bt_xtalcalib_proc();
    } while (nRet && cnt++ < APP_FACTORYMODE_RETRY_LIMITED);

    if (nRet)
      goto err;
    osDelay(200);
    app_factorymode_send_progress(30);
  }

  nvrec_dev_get_xtal_fcap((unsigned int *)&capval);
  app_factorymode_send_code(capval);

  btdrv_start_bt();
  bt_drv_reg_op_key_gen_after_reset(false);
  app_factorymode_send_progress(40);
  BesbtInit();
  osDelay(600);

  nRet = app_factorymode_proc();
  if (nRet)
    goto err;

  app_factorymode_test_key_init();
  // osTimerStop(app_factory_timer);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_PASS);

  // wait forever
  osSignalWait(0x01, osWaitForever);
  goto exit;

err:
  osTimerStop(app_factory_timer);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_FAILED);
  app_factorymode_send_code(0xff);
  osSignalWait(0x01, osWaitForever);
exit:
  app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  pmu_shutdown();
  return nRet;
}

int app_factorymode_calib_only(void) {
  uint8_t cnt = 0;
  int nRet = 0;
  uint32_t capval = 0x00;

  app_factorymode_uart_init();
#ifdef __WATCHER_DOG_RESET__
  app_wdt_reopen(60);
#endif
  app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
  list_init();
  app_os_init();
  nv_record_env_init();
  nvrec_dev_data_open();
  factory_section_open();
  app_factorymode_indication_init();

  app_set_threadhandle(APP_MODUAL_OHTER, app_factorymode_process);
  app_factory_timer =
      osTimerCreate(osTimer(APP_FACTORY_TIMER), osTimerPeriodic, NULL);
  osTimerStart(app_factory_timer, 300);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_RUNNING);
  app_factorymode_send_progress(10);
  btdrv_start_bt();
  osDelay(20);
  app_factorymode_send_progress(20);
  do {
    nRet = app_factorymode_bt_xtalcalib_proc();
  } while (nRet && ++cnt < APP_FACTORYMODE_RETRY_LIMITED);
  if (nRet)
    goto err;

  nvrec_dev_get_xtal_fcap((unsigned int *)&capval);
  app_factorymode_send_code(capval);

  app_factorymode_send_progress(50);
  osDelay(200);
  app_factorymode_send_progress(80);
  osDelay(100);
  app_factorymode_send_progress(100);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_PASS);
  osSignalWait(0x01, osWaitForever);
  goto exit;

err:
  osTimerStop(app_factory_timer);
  app_factorymode_status_indication(APP_FACTORYMODE_STATUS_INDICATION_FAILED);
  app_factorymode_send_code(0xff);
  osSignalWait(0x01, osWaitForever);
exit:

  app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
  pmu_shutdown();
  return nRet;
}

#ifdef __USB_COMM__
// for usb
#include "hal_timer.h"
#include "hwtimer_list.h"
#include "usb_cdc.h"

#include "app_factory_cdc_comm.h"
#include "hal_usb.h"
#include "sys_api_cdc_comm.h"

static const struct USB_SERIAL_CFG_T cdc_cfg = {
    .mode = USB_SERIAL_API_NONBLOCKING,
};

static void usb_serial_recv_timeout(void *param) { usb_serial_cancel_recv(); }

int app_factorymode_cdc_comm(void) {
  HWTIMER_ID timer;
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  usb_serial_open(&cdc_cfg);
  osDelay(500);

  hwtimer_init();
  timer = hwtimer_alloc(usb_serial_recv_timeout, NULL);
  ASSERT(timer, "Failed to alloc usb serial recv timer");

  usb_serial_flush_recv_buffer();
  usb_serial_init_xfer();
  af_open();

  comm_loop();
  return 1;
}
#endif
#endif
