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
#include "cmsis.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hwtimer_list.h"
#include "main_entry.h"
#include "pmu.h"

#ifdef RTOS
#include "cmsis_os.h"
#ifdef KERNEL_RTX
#include "rt_Time.h"
#endif
#endif

#ifdef HWTEST
#include "hwtest.h"
#ifdef VD_TEST
#include "voice_detector.h"
#endif
#endif

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

#define TIMER_IRQ_PERIOD_MS 2500
#define DELAY_PERIOD_MS 4000

#ifndef FLASH_FILL
#define FLASH_FILL 1
#endif

#ifdef KERNEL_RTX
#define OS_TIME_STR "[%2u/%u]"
#define OS_CUR_TIME , SysTick->VAL, os_time
#else
#define OS_TIME_STR
#define OS_CUR_TIME
#endif

#if defined(MS_TIME)
#define TIME_STR "[%u]" OS_TIME_STR
#define CUR_TIME TICKS_TO_MS(hal_sys_timer_get()) OS_CUR_TIME
#elif defined(RAW_TIME)
#define TIME_STR "[0x%X]" OS_TIME_STR
#define CUR_TIME hal_sys_timer_get() OS_CUR_TIME
#else
#define TIME_STR "[%u/0x%X]" OS_TIME_STR
#define CUR_TIME                                                               \
  TICKS_TO_MS(hal_sys_timer_get()), hal_sys_timer_get() OS_CUR_TIME
#endif

#ifndef NO_TIMER
static HWTIMER_ID hw_timer = NULL;

static void timer_handler(void *param) {
  TRACE(1, TIME_STR " Timer handler: %u", CUR_TIME, (uint32_t)param);
  hwtimer_start(hw_timer, MS_TO_TICKS(TIMER_IRQ_PERIOD_MS));
  TRACE(1, TIME_STR " Start timer %u ms", CUR_TIME, TIMER_IRQ_PERIOD_MS);
}
#endif

const static unsigned char bytes[FLASH_FILL] = {
    0x1,
};

// GDB can set a breakpoint on the main function only if it is
// declared as below, when linking with STD libraries.

int MAIN_ENTRY(void) {
  int POSSIBLY_UNUSED ret;

  hwtimer_init();
  hal_audma_open();
  hal_gpdma_open();
#ifdef DEBUG
#if (DEBUG_PORT == 3)
  hal_iomux_set_analog_i2c();
  hal_iomux_set_uart2();
  hal_trace_open(HAL_TRACE_TRANSPORT_UART2);
#elif (DEBUG_PORT == 2)
  hal_iomux_set_analog_i2c();
  hal_iomux_set_uart1();
  hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#else
  hal_iomux_set_uart0();
  hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
#endif
#endif

#if !defined(SIMU)
  uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
  hal_norflash_get_id(HAL_NORFLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
  TRACE(3, "FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
#endif

  TRACE(1, TIME_STR " main started: filled@0x%08x", CUR_TIME, (uint32_t)bytes);

#ifndef NO_PMU
  ret = pmu_open();
  ASSERT(ret == 0, "Failed to open pmu");
#endif
  analog_open();

  hal_cmu_simu_pass();

#ifdef SIMU
  hal_sw_bootmode_set(HAL_SW_BOOTMODE_FLASH_BOOT);
  hal_cmu_sys_reboot();
#else
#ifdef SLEEP_TEST
  hal_sleep_start_stats(10000, 10000);
  hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);
#else
  hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_104M);
#endif
  TRACE(1, "CPU freq: %u", hal_sys_timer_calc_cpu_freq(5, 0));
#endif

#ifdef HWTEST

#ifdef USB_SERIAL_TEST
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  usb_serial_test();
#endif
#ifdef USB_SERIAL_DIRECT_XFER_TEST
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  usb_serial_direct_xfer_test();
#endif
#ifdef USB_AUDIO_TEST
  pmu_usb_config(PMU_USB_CONFIG_TYPE_DEVICE);
  usb_audio_test();
#endif
#ifdef I2C_TEST
  i2c_test();
#endif
#ifdef AF_TEST
  af_test();
#endif
#ifdef VD_TEST
  voice_detector_test();
#endif
#ifdef CP_TEST
  cp_test();
#endif
#ifdef SEC_ENG_TEST
  sec_eng_test();
#endif
#ifdef TDM_TEST
  tdm_test();
#endif
#ifdef A7_DSP_TEST
  a7_dsp_test();
#endif
#ifdef TRANSQ_TEST
  transq_test();
#endif
#ifdef PSRAM_TEST
  psram_test();
#endif
#ifdef PSRAMUHS_TEST
  psramuhs_test();
#endif

  SAFE_PROGRAM_STOP();

#endif // HWTEST

#ifdef NO_TIMER
  TRACE(0, TIME_STR " Enter sleep ...", CUR_TIME);
#else
  hw_timer = hwtimer_alloc(timer_handler, 0);
  hwtimer_start(hw_timer, MS_TO_TICKS(TIMER_IRQ_PERIOD_MS));
  TRACE(1, TIME_STR " Start timer %u ms", CUR_TIME, TIMER_IRQ_PERIOD_MS);
#endif

  while (1) {
#if defined(SLEEP_TEST) && !defined(RTOS)
    hal_sleep_enter_sleep();
#else
    osDelay(DELAY_PERIOD_MS);
    TRACE(1, TIME_STR " Delay %u ms done", CUR_TIME, DELAY_PERIOD_MS);
#endif
  }

  SAFE_PROGRAM_STOP();
  return 0;
}
