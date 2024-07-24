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
#ifndef __PMU_H__
#define __PMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hal_analogif.h"
#include "hal_cmu.h"
#include "hal_gpio.h"
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(pmu)

#define PMU_DEBUG_TRACE(n, s, ...)          LOG_DUMMY(n, s, ##__VA_ARGS__)
#define PMU_DEBUG_TRACE_IMM(n, s, ...)      LOG_INFO((n) | LOG_ATTR_IMM, s, ##__VA_ARGS__)
#define PMU_INFO_TRACE_IMM(n, s, ...)       LOG_INFO((n) | LOG_ATTR_IMM, s, ##__VA_ARGS__)
#define PMU_INFO_TRACE(n, s, ...)           LOG_INFO(n, s, ##__VA_ARGS__)

#ifndef ISPI_PMU_REG
#define ISPI_PMU_REG(reg)                   (reg)
#endif
#define pmu_read(reg, val)                  hal_analogif_reg_read(ISPI_PMU_REG(reg), val)
#define pmu_write(reg, val)                 hal_analogif_reg_write(ISPI_PMU_REG(reg), val)

#define  PMU_MANUAL_MODE                    1
#define  PMU_AUTO_MODE                      0
#define  PMU_LDO_ON                         1
#define  PMU_LDO_OFF                        0
#define  PMU_LP_MODE_ON                     1
#define  PMU_LP_MODE_OFF                    0
#define  PMU_DSLEEP_MODE_ON                 1
#define  PMU_DSLEEP_MODE_OFF                0

union SECURITY_VALUE_T {
    struct {
        unsigned short security_en      :1;
        unsigned short key_id           :3;
        unsigned short key_chksum       :2;
        unsigned short vendor_id        :6;
        unsigned short vendor_chksum    :3;
        unsigned short chksum           :1;
    };
    unsigned short reg;
};

enum PMU_CHARGER_STATUS_T {
    PMU_CHARGER_PLUGIN,
    PMU_CHARGER_PLUGOUT,
    PMU_CHARGER_UNKNOWN,
};

enum PMU_POWER_ON_CAUSE_T {
    PMU_POWER_ON_CAUSE_NONE,
    PMU_POWER_ON_CAUSE_POWER_KEY,
    PMU_POWER_ON_CAUSE_RTC,
    PMU_POWER_ON_CAUSE_CHARGER_ACON,
    PMU_POWER_ON_CAUSE_CHARGER_ACOFF,
};

enum PMU_USB_PIN_CHK_STATUS_T {
    PMU_USB_PIN_CHK_NONE,
    // Chip acts as host
    PMU_USB_PIN_CHK_DEV_CONN,
    // Chip acts as host
    PMU_USB_PIN_CHK_DEV_DISCONN,
    // Chip acts as device
    PMU_USB_PIN_CHK_HOST_RESUME,

    PMU_USB_PIN_CHK_STATUS_QTY
};

enum PMU_USB_CONFIG_TYPE_T {
    PMU_USB_CONFIG_TYPE_NONE,
    PMU_USB_CONFIG_TYPE_DEVICE,
    PMU_USB_CONFIG_TYPE_HOST,
};

enum PMU_POWER_MODE_T {
    PMU_POWER_MODE_NONE,
    PMU_POWER_MODE_LDO,
    PMU_POWER_MODE_ANA_DCDC,
    PMU_POWER_MODE_DIG_DCDC,
};

enum PMU_VIORISE_REQ_USER_T {
   PMU_VIORISE_REQ_USER_PWL0,
   PMU_VIORISE_REQ_USER_PWL1,
   PMU_VIORISE_REQ_USER_FLASH,
   PMU_VIORISE_REQ_USER_CHARGER,

   PMU_VIORISE_REQ_USER_QTY
};

struct PMU_LED_BR_CFG_T {
    uint16_t off_time_ms;
    uint16_t on_time_ms;
    uint16_t fade_time_ms;
};

typedef void (*PMU_USB_PIN_CHK_CALLBACK)(enum PMU_USB_PIN_CHK_STATUS_T status);

typedef void (*PMU_RTC_IRQ_HANDLER_T)(uint32_t seconds);

typedef void (*PMU_CHARGER_IRQ_HANDLER_T)(enum PMU_CHARGER_STATUS_T status);

typedef void (*PMU_WDT_IRQ_HANDLER_T)(void);

typedef void (*PMU_IRQ_UNIFIED_HANDLER_T)(uint16_t irq_status);

int pmu_open(void);

void pmu_sleep(void);

void pmu_wakeup(void);

void pmu_mode_change(enum PMU_POWER_MODE_T mode);

int pmu_get_security_value(union SECURITY_VALUE_T *val);

void pmu_shutdown(void);

void pmu_reset(void);

int pmu_get_efuse(enum PMU_EFUSE_PAGE_T page, unsigned short *efuse);

void pmu_codec_config(int enable);

void pmu_anc_config(int enable);

void pmu_usb_config(enum PMU_USB_CONFIG_TYPE_T type);

void pmu_sleep_en(unsigned char sleep_en);

void pmu_flash_write_config(void);

void pmu_flash_read_config(void);

void pmu_flash_freq_config(uint32_t freq);

void pmu_psram_freq_config(uint32_t freq);

void pmu_fir_high_speed_config(int enable);

void pmu_iir_freq_config(uint32_t freq);

void pmu_rs_freq_config(uint32_t freq);

void pmu_sys_freq_config(enum HAL_CMU_FREQ_T freq);

void pmu_high_performance_mode_enable(bool enable);

void pmu_charger_init(void);

void pmu_charger_set_irq_handler(PMU_CHARGER_IRQ_HANDLER_T handler);

void pmu_charger_plugin_config(void);

void pmu_charger_plugout_config(void);

enum PMU_CHARGER_STATUS_T pmu_charger_get_status(void);

int pmu_usb_config_pin_status_check(enum PMU_USB_PIN_CHK_STATUS_T status, PMU_USB_PIN_CHK_CALLBACK callback, int enable);

void pmu_usb_enable_pin_status_check(void);

void pmu_usb_disable_pin_status_check(void);

void pmu_usb_get_pin_status(int *dp, int *dm);

void pmu_rtc_enable(void);

void pmu_rtc_disable(void);

int pmu_rtc_enabled(void);

void pmu_rtc_set(uint32_t seconds);

uint32_t pmu_rtc_get(void);

void pmu_rtc_set_alarm(uint32_t seconds);

uint32_t pmu_rtc_get_alarm(void);

void pmu_rtc_clear_alarm(void);

int pmu_rtc_alarm_status_set(void);

int pmu_rtc_alarm_alerted();

void pmu_rtc_set_irq_handler(PMU_RTC_IRQ_HANDLER_T handler);

void pmu_viorise_req(enum PMU_VIORISE_REQ_USER_T user, bool rise);

enum PMU_POWER_ON_CAUSE_T pmu_get_power_on_cause(void);

int pmu_debug_config_ana(uint16_t volt);

int pmu_debug_config_codec(uint16_t volt);

int pmu_debug_config_audio_output(bool diff);

int pmu_debug_config_vcrystal(bool on);

void pmu_debug_reliability_test(int stage);

void pmu_at_skip_shutdown(bool enable);

void pmu_led_set_direction(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_DIR_T dir);

enum HAL_GPIO_DIR_T pmu_led_get_direction(enum HAL_GPIO_PIN_T pin);

void pmu_led_set_voltage_domains(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt);

void pmu_led_set_pull_select(enum HAL_IOMUX_PIN_T pin, enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel);

void pmu_led_set_value(enum HAL_GPIO_PIN_T pin, int data);

int pmu_led_get_value(enum HAL_GPIO_PIN_T pin);

void pmu_wdt_set_irq_handler(PMU_WDT_IRQ_HANDLER_T handler);

int pmu_wdt_config(uint32_t irq_ms, uint32_t reset_ms);

void pmu_wdt_start(void);

void pmu_wdt_stop(void);

void pmu_wdt_feed(void);

void pmu_ntc_capture_enable(void);

void pmu_ntc_capture_disable(void);

int pmu_set_irq_unified_handler(enum PMU_IRQ_TYPE_T type, PMU_IRQ_UNIFIED_HANDLER_T hdlr);

enum HAL_PWRKEY_IRQ_T pmu_pwrkey_irq_value_to_state(uint16_t irq_status);

#ifdef __cplusplus
}
#endif

#endif

