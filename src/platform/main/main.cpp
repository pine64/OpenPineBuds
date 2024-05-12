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
#include "analog.h"
#include "app_bt_stream.h"
#include "app_utils.h"
#include "apps.h"
#include "cmsis.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_gpio.h"
#include "hal_iomux.h"
#include "hal_location.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_wdt.h"
#include "hwtimer_list.h"
#include "mpu.h"
#include "norflash_api.h"
#include "plat_addr_map.h"
#include "pmu.h"
#include "stdlib.h"
#include "tgt_hardware.h"

#ifdef RTOS
#include "app_factory.h"
#include "cmsis_os.h"
#endif
#ifdef CORE_DUMP_TO_FLASH
#include "coredump_section.h"
#endif

extern "C" void log_dump_init(void);
extern "C" void crash_dump_init(void);

#ifdef FIRMWARE_REV
#define SYS_STORE_FW_VER(x)                                                    \
  if (fw_rev_##x) {                                                            \
    *fw_rev_##x = fw.softwareRevByte##x;                                       \
  }

typedef struct {
  uint8_t softwareRevByte0;
  uint8_t softwareRevByte1;
  uint8_t softwareRevByte2;
  uint8_t softwareRevByte3;
} FIRMWARE_REV_INFO_T;

static FIRMWARE_REV_INFO_T fwRevInfoInFlash
    __attribute((section(".fw_rev"))) = {0, 0, 1, 0};
FIRMWARE_REV_INFO_T fwRevInfoInRam;

extern "C" void system_get_info(uint8_t *fw_rev_0, uint8_t *fw_rev_1,
                                uint8_t *fw_rev_2, uint8_t *fw_rev_3) {
  FIRMWARE_REV_INFO_T fw = fwRevInfoInFlash;

  SYS_STORE_FW_VER(0);
  SYS_STORE_FW_VER(1);
  SYS_STORE_FW_VER(2);
  SYS_STORE_FW_VER(3);
}
#endif

#if defined(_AUTO_TEST_)
static uint8_t fwversion[4] = {0, 0, 1, 0};

void system_get_fwversion(uint8_t *fw_rev_0, uint8_t *fw_rev_1,
                          uint8_t *fw_rev_2, uint8_t *fw_rev_3) {
  *fw_rev_0 = fwversion[0];
  *fw_rev_1 = fwversion[1];
  *fw_rev_2 = fwversion[2];
  *fw_rev_3 = fwversion[3];
}
#endif

#define system_shutdown_wdt_config(seconds)                                    \
  do {                                                                         \
    hal_wdt_stop(HAL_WDT_ID_0);                                                \
    hal_wdt_set_irq_callback(HAL_WDT_ID_0, NULL);                              \
    hal_wdt_set_timeout(HAL_WDT_ID_0, seconds);                                \
    hal_wdt_start(HAL_WDT_ID_0);                                               \
    hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_USER_WDT, NULL);         \
  } while (0)

static osThreadId main_thread_tid = NULL;

extern "C" int system_shutdown(void) {
  TRACE(0, "system_shutdown!!");
  osThreadSetPriority(main_thread_tid, osPriorityRealtime);
  osSignalSet(main_thread_tid, 0x4);
  return 0;
}

int system_reset(void) {
  osThreadSetPriority(main_thread_tid, osPriorityRealtime);
  osSignalSet(main_thread_tid, 0x8);
  return 0;
}

int signal_send_to_main_thread(uint32_t signals) {
  osSignalSet(main_thread_tid, signals);
  return 0;
}

int tgt_hardware_setup(void) {
#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
  for (uint8_t i = 0; i < 3; i++) {
    hal_iomux_init(
        (struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_ibrt_indication_pinmux_pwl[i],
        1);
    if (i == 0)
      hal_gpio_pin_set_dir(
          (enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin,
          HAL_GPIO_DIR_OUT, 0);
    else
      hal_gpio_pin_set_dir(
          (enum HAL_GPIO_PIN_T)cfg_ibrt_indication_pinmux_pwl[i].pin,
          HAL_GPIO_DIR_OUT, 1);
  }
#endif

  hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl,
                 sizeof(cfg_hw_pinmux_pwl) /
                     sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
  if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM) {
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP
                        *)&app_battery_ext_charger_indicator_cfg,
                   1);
    hal_gpio_pin_set_dir(
        (enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin,
        HAL_GPIO_DIR_IN, 1);
  }
  return 0;
}

#if defined(ROM_UTILS_ON)
void rom_utils_init(void);
#endif

#if defined(_AUTO_TEST_)
extern int32_t at_Init(void);
#endif

#ifdef DEBUG_MODE_USB_DOWNLOAD
static void process_usb_download_mode(void) {
  if (pmu_charger_get_status() == PMU_CHARGER_PLUGIN && hal_pwrkey_pressed()) {
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
    hal_cmu_sys_reboot();
  }
}
#endif
int main(void) {
  uint8_t sys_case = 0;
  int ret = 0;
#ifdef __WATCHER_DOG_RESET__
#if !defined(BLE_ONLY_ENABLED)
  app_wdt_open(15);
#else
  app_wdt_open(30);
#endif
#endif

#ifdef __FACTORY_MODE_SUPPORT__
  uint32_t bootmode = hal_sw_bootmode_get();
#endif

#ifdef DEBUG_MODE_USB_DOWNLOAD
  process_usb_download_mode();
#endif

  tgt_hardware_setup();

#if defined(ROM_UTILS_ON)
  rom_utils_init();
#endif

  main_thread_tid = osThreadGetId();

  hwtimer_init();

  hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
  hal_audma_open();
  hal_gpdma_open();
  norflash_api_init();
#if defined(DUMP_LOG_ENABLE)
  log_dump_init();
#endif
#if defined(DUMP_CRASH_ENABLE)
  crash_dump_init();
#endif
#ifdef CORE_DUMP_TO_FLASH
  coredump_to_flash_init();
#endif

#ifdef DEBUG
#if (DEBUG_PORT == 1)
  hal_iomux_set_uart0();
#ifdef __FACTORY_MODE_SUPPORT__
  if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
  {
    hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
  }
#endif

#if (DEBUG_PORT == 2)
#ifdef __FACTORY_MODE_SUPPORT__
  if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
  {
    hal_iomux_set_analog_i2c();
  }
  hal_iomux_set_uart1();
  hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#endif
  hal_sleep_start_stats(10000, 10000);
  hal_trace_set_log_level(LOG_LEVEL_DEBUG);
#endif

  hal_iomux_ispi_access_init();

  uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
  hal_norflash_get_id(HAL_NORFLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
  TRACE(3, "FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
  ASSERT(hal_norflash_opened(HAL_NORFLASH_ID_0), "Failed to init flash: %d",
         hal_norflash_get_open_state(HAL_NORFLASH_ID_0));

  // Software will load the factory data and user data from the bottom TWO
  // sectors from the flash, the FLASH_SIZE defined is the common.mk must be
  // equal or greater than the actual chip flash size, otherwise the ota will
  // load the wrong information
  uint32_t actualFlashSize =
      hal_norflash_get_flash_total_size(HAL_NORFLASH_ID_0);
  if (FLASH_SIZE > actualFlashSize) {
    TRACE_IMM(0, "Wrong FLASH_SIZE defined in target.mk!");
    TRACE_IMM(2,
              "FLASH_SIZE is defined as 0x%x while the actual chip flash size "
              "is 0x%x!",
              FLASH_SIZE, actualFlashSize);
    TRACE_IMM(1,
              "Please change the FLASH_SIZE in common.mk to 0x%x to enable the "
              "OTA feature.",
              actualFlashSize);
    ASSERT(false, " ");
  }

  pmu_open();

  analog_open();

  ret = mpu_open();
  if (ret == 0) {
    mpu_setup();
  }

  srand(hal_sys_timer_get());

#if defined(_AUTO_TEST_)
  at_Init();
#endif

#ifdef VOICE_DATAPATH
  app_audio_buffer_check();
#endif

#ifdef __FACTORY_MODE_SUPPORT__
  if (bootmode & HAL_SW_BOOTMODE_FACTORY) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_FACTORY);
    ret = app_factorymode_init(bootmode);

  } else if (bootmode & HAL_SW_BOOTMODE_CALIB) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CALIB);
    ret = app_factorymode_calib_only();
  }
#ifdef __USB_COMM__
  else if (bootmode & HAL_SW_BOOTMODE_CDC_COMM) {
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CDC_COMM);
    ret = app_factorymode_cdc_comm();
  }
#endif
  else
#endif
  {
#ifdef FIRMWARE_REV
    fwRevInfoInRam = fwRevInfoInFlash;
    TRACE(4, "The Firmware rev is %d.%d.%d.%d", fwRevInfoInRam.softwareRevByte0,
          fwRevInfoInRam.softwareRevByte1, fwRevInfoInRam.softwareRevByte2,
          fwRevInfoInRam.softwareRevByte3);
#endif
    ret = app_init();
  }
  if (!ret) {
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("BT Init ok.");
#endif
    while (1) {
      osEvent evt;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
      osSignalClear(main_thread_tid, 0x0f);
#endif
      // wait any signal
      evt = osSignalWait(0x0, osWaitForever);

      // get role from signal value
      if (evt.status == osEventSignal) {
        if (evt.value.signals & 0x04) {
          sys_case = 1;
          break;
        } else if (evt.value.signals & 0x08) {
          sys_case = 2;
          break;
        }
      } else {
        sys_case = 1;
        break;
      }
    }
  }
#ifdef __WATCHER_DOG_RESET__
  system_shutdown_wdt_config(10);
#endif
  app_deinit(ret);
  TRACE(1, "byebye~~~ %d\n", sys_case);
  if ((sys_case == 1) || (sys_case == 0)) {
    TRACE(0, "shutdown\n");
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("System shutdown.");
    osDelay(50);
#endif
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    pmu_shutdown();
  } else if (sys_case == 2) {
    TRACE(0, "reset\n");
#if defined(_AUTO_TEST_)
    AUTO_TEST_SEND("System reset.");
    osDelay(50);
#endif
    hal_cmu_sys_reboot();
  }

  return 0;
}
