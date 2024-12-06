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
#ifdef __ARMCC_VERSION

extern "C" {

#include "analog.h"
#include "codec_int.h"
#include "hal_codec.h"
#include "hal_trace.h"
#include "norflash_drv.h"
#include "pmu.h"

#ifdef NOSTD

void __rt_raise(int sig, intptr_t type) {
  ASSERT(false, "__rt_raise: sig=%d type=%p", sig, type);
}

#endif

// Stupid armlink
WEAK void analog_aud_xtal_tune(float ratio) {}
WEAK void
hal_codec_dac_gain_m60db_check(enum HAL_CODEC_PERF_TEST_POWER_T type) {}
WEAK void hal_codec_set_noise_reduction(bool enable) {}
WEAK void hal_codec_tune_both_resample_rate(float ratio) {}
WEAK void
hal_codec_set_bt_trigger_callback(HAL_CODEC_BT_TRIGGER_CALLBACK callback) {}
WEAK int hal_codec_bt_trigger_start(void) { return 0; }
WEAK int hal_codec_bt_trigger_stop(void) { return 0; }
WEAK void hal_codec_sync_dac_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_dac_disable(void) {}
WEAK void hal_codec_sync_adc_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_adc_disable(void) {}
WEAK void
hal_codec_sync_dac_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_dac_resample_rate_disable(void) {}
WEAK void
hal_codec_sync_adc_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_adc_resample_rate_disable(void) {}
WEAK void hal_codec_sync_dac_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_dac_gain_disable(void) {}
WEAK void hal_codec_sync_adc_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}
WEAK void hal_codec_sync_adc_gain_disable(void) {}
WEAK int codec_anc_open(enum ANC_TYPE_T type, enum AUD_SAMPRATE_T dac_rate,
                        enum AUD_SAMPRATE_T adc_rate, CODEC_ANC_HANDLER hdlr) {
  return 0;
}
WEAK int codec_anc_close(enum ANC_TYPE_T type) { return 0; }

#ifdef ROM_BUILD
WEAK void norflash_reset(void) {}
WEAK int norflash_get_size(uint32_t *total_size, uint32_t *block_size,
                           uint32_t *sector_size, uint32_t *page_size) {
  return HAL_NORFLASH_OK;
}
WEAK int norflash_set_mode(uint32_t op) { return HAL_NORFLASH_OK; }
WEAK int norflash_pre_operation(void) { return HAL_NORFLASH_OK; }
WEAK int norflash_post_operation(void) { return HAL_NORFLASH_OK; }
WEAK int norflash_init_sample_delay_by_div(uint32_t div) {
  return HAL_NORFLASH_OK;
}
WEAK void norflash_set_sample_delay(uint32_t index) {}
WEAK int norflash_sample_delay_calib(void) { return HAL_NORFLASH_OK; }
WEAK int norflash_init_div(const struct HAL_NORFLASH_CONFIG_T *cfg) {
  return HAL_NORFLASH_OK;
}
WEAK int norflash_match_chip(const uint8_t *id, uint32_t len) {
  return HAL_NORFLASH_OK;
}
WEAK int norflash_get_id(uint8_t *value, uint32_t len) {
  return HAL_NORFLASH_OK;
}
WEAK int norflash_get_unique_id(uint8_t *value, uint32_t len) {
  return HAL_NORFLASH_OK;
}
WEAK enum HAL_NORFLASH_RET_T norflash_erase(uint32_t start_address,
                                            enum DRV_NORFLASH_ERASE_T type,
                                            int suspend) {
  return HAL_NORFLASH_OK;
}
WEAK enum HAL_NORFLASH_RET_T norflash_write(uint32_t start_address,
                                            const uint8_t *buffer, uint32_t len,
                                            int suspend) {
  return HAL_NORFLASH_OK;
}
WEAK int norflash_read(uint32_t start_address, uint8_t *buffer, uint32_t len) {
  return HAL_NORFLASH_OK;
}

WEAK void pmu_flash_freq_config(uint32_t freq) {}
WEAK void pmu_rs_freq_config(uint32_t freq) {}
WEAK void
pmu_led_set_voltage_domains(enum HAL_IOMUX_PIN_T pin,
                            enum HAL_IOMUX_PIN_VOLTAGE_DOMAINS_T volt) {}
WEAK void pmu_led_set_pull_select(enum HAL_IOMUX_PIN_T pin,
                                  enum HAL_IOMUX_PIN_PULL_SELECT_T pull_sel) {}

#ifdef SPI_ROM_ONLY
WEAK int hal_ispi_open(const struct HAL_SPI_CFG_T *cfg) { return 0; }
WEAK int hal_ispi_send(const void *data, uint32_t len) { return 0; }
WEAK int hal_ispi_recv(const void *cmd, void *data, uint32_t len) { return 0; }
#endif

WEAK void analog_aud_freq_pll_config(uint32_t freq, uint32_t div) {}
WEAK void analog_aud_pll_open(enum ANA_AUD_PLL_USER_T user) {}
WEAK void analog_aud_pll_close(enum ANA_AUD_PLL_USER_T user) {}
WEAK void analog_aud_codec_speaker_enable(bool en) {}
WEAK void analog_aud_set_dac_gain(int32_t v) {}
WEAK uint32_t analog_aud_get_max_dre_gain(void) { return 0; }
WEAK void analog_aud_vad_adc_enable(bool en) {}
WEAK void analog_aud_vad_enable(enum AUD_VAD_TYPE_T type, bool en) {}
#endif
}

#endif // __ARMCC_VERSION
