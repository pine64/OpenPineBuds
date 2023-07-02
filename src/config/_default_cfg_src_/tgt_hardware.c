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
#include "tgt_hardware.h"

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM] = {};

// adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
    HAL_KEY_CODE_FN9, HAL_KEY_CODE_FN8, HAL_KEY_CODE_FN7,
    HAL_KEY_CODE_FN6, HAL_KEY_CODE_FN5, HAL_KEY_CODE_FN4,
    HAL_KEY_CODE_FN3, HAL_KEY_CODE_FN2, HAL_KEY_CODE_FN1};

// gpiokey define
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {};

// bt config
const char *BT_LOCAL_NAME = TO_STRING(BT_DEV_NAME) "\0";
const char *BLE_DEFAULT_NAME = "BES_BLE";

uint8_t ble_addr[6] = {
#ifdef BLE_DEV_ADDR
    BLE_DEV_ADDR
#else
    0xBE, 0x99, 0x34, 0x45,
    0x56, 0x67
#endif
};
uint8_t bt_addr[6] = {
#ifdef BT_DEV_ADDR
    BT_DEV_ADDR
#else
    0x1e, 0x57, 0x34, 0x45,
    0x56, 0x67
#endif
};

// audio config
// freq bands range {[0k:2.5K], [2.5k:5K], [5k:7.5K], [7.5K:10K], [10K:12.5K],
// [12.5K:15K], [15K:17.5K], [17.5K:20K]} gain range -12~+12
const int8_t cfg_hw_aud_eq_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {
    0, 0, 0, 0, 0, 0, 0, 0};

#define TX_PA_GAIN CODEC_TX_PA_GAIN_DEFAULT

const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY] = {
    {TX_PA_GAIN, 0x03, -11}, {TX_PA_GAIN, 0x03, -99},
    {TX_PA_GAIN, 0x03, -45}, {TX_PA_GAIN, 0x03, -42},
    {TX_PA_GAIN, 0x03, -39}, {TX_PA_GAIN, 0x03, -36},
    {TX_PA_GAIN, 0x03, -33}, {TX_PA_GAIN, 0x03, -30},
    {TX_PA_GAIN, 0x03, -27}, {TX_PA_GAIN, 0x03, -24},
    {TX_PA_GAIN, 0x03, -21}, {TX_PA_GAIN, 0x03, -18},
    {TX_PA_GAIN, 0x03, -15}, {TX_PA_GAIN, 0x03, -12},
    {TX_PA_GAIN, 0x03, -9},  {TX_PA_GAIN, 0x03, -6},
    {TX_PA_GAIN, 0x03, -3},  {TX_PA_GAIN, 0x03, 0}, // 0dBm
};

#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_VMIC_MAP_VMIC1)
#define CFG_HW_AUD_INPUT_PATH_LINEIN_DEV                                       \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define CFG_HW_AUD_INPUT_PATH_VADMIC_DEV                                       \
  (AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC1)
const struct AUD_IO_PATH_CFG_T
    cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM] = {
        {
            AUD_INPUT_PATH_MAINMIC,
            CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV,
        },
        {
            AUD_INPUT_PATH_LINEIN,
            CFG_HW_AUD_INPUT_PATH_LINEIN_DEV,
        },
        {
            AUD_INPUT_PATH_VADMIC,
            CFG_HW_AUD_INPUT_PATH_VADMIC_DEV,
        },
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_enable_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg =
    {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE};
