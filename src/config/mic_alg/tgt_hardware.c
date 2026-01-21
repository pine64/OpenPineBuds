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
#include "drc.h"
#include "fir_process.h"
#include "iir_process.h"
#include "limiter.h"
#include "spectrum_fix.h"

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM] = {
#if (CFG_HW_PLW_NUM > 0)
    {HAL_IOMUX_PIN_LED2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
};

#ifdef __APP_USE_LED_INDICATE_IBRT_STATUS__
const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_ibrt_indication_pinmux_pwl[3] = {
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VBAT,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_LED2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VBAT,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
};
#endif

#ifdef __KNOWLES
const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_pinmux_uart[2] = {
    {HAL_IOMUX_PIN_P2_2, HAL_IOMUX_FUNC_UART2_RX, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_NOPULL},
    {HAL_IOMUX_PIN_P2_3, HAL_IOMUX_FUNC_UART2_TX, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_NOPULL},
};
#endif

// adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
#if (CFG_HW_ADCKEY_NUMBER > 0)
    HAL_KEY_CODE_FN9, HAL_KEY_CODE_FN8, HAL_KEY_CODE_FN7,
    HAL_KEY_CODE_FN6, HAL_KEY_CODE_FN5, HAL_KEY_CODE_FN4,
    HAL_KEY_CODE_FN3, HAL_KEY_CODE_FN2, HAL_KEY_CODE_FN1,
#endif
};

// gpiokey define
#define CFG_HW_GPIOKEY_DOWN_LEVEL (0)
#define CFG_HW_GPIOKEY_UP_LEVEL (1)
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {
    /*
    #if (CFG_HW_GPIOKEY_NUM > 0)
    #ifdef BES_AUDIO_DEV_Main_Board_9v0
        {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        {HAL_KEY_CODE_FN3,{HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        {HAL_KEY_CODE_FN4,{HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        //{HAL_KEY_CODE_FN5,{HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
       // {HAL_KEY_CODE_FN6,{HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}}, #else #ifndef
    TPORTS_KEY_COEXIST {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_3,
    HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE}}, {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P1_0,
    HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE}},
        // {HAL_KEY_CODE_FN3,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        {HAL_KEY_CODE_FN15,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}}, #else
        {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        {HAL_KEY_CODE_FN15,{HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}}, #endif #endif
    #ifdef IS_MULTI_AI_ENABLED
        //{HAL_KEY_CODE_FN13,{HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
        //{HAL_KEY_CODE_FN14,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO,
    HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}}, #endif #endif
    */
    {HAL_KEY_CODE_FN1,
     {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
};

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
const int8_t cfg_aud_eq_sbc_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {
    0, 0, 0, 0, 0, 0, 0, 0};

#define TX_PA_GAIN CODEC_TX_PA_GAIN_DEFAULT

const struct CODEC_DAC_VOL_T codec_dac_vol[TGT_VOLUME_LEVEL_QTY] = {
    {TX_PA_GAIN, 0x03, -21}, {TX_PA_GAIN, 0x03, -99},
    {TX_PA_GAIN, 0x03, -45}, {TX_PA_GAIN, 0x03, -42},
    {TX_PA_GAIN, 0x03, -39}, {TX_PA_GAIN, 0x03, -36},
    {TX_PA_GAIN, 0x03, -33}, {TX_PA_GAIN, 0x03, -30},
    {TX_PA_GAIN, 0x03, -27}, {TX_PA_GAIN, 0x03, -24},
    {TX_PA_GAIN, 0x03, -21}, {TX_PA_GAIN, 0x03, -18},
    {TX_PA_GAIN, 0x03, -15}, {TX_PA_GAIN, 0x03, -12},
    {TX_PA_GAIN, 0x03, -9},  {TX_PA_GAIN, 0x03, -6},
    {TX_PA_GAIN, 0x03, -3},  {TX_PA_GAIN, 0x03, 0}, // 0dBm
};

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC2 |            \
   AUD_VMIC_MAP_VMIC3)
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 | AUD_CHANNEL_MAP_CH4 |           \
   AUD_VMIC_MAP_VMIC1)
#else
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC3)
#endif

#define CFG_HW_AUD_INPUT_PATH_LINEIN_DEV                                       \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#ifdef VOICE_DETECTOR_EN
#define CFG_HW_AUD_INPUT_PATH_VADMIC_DEV                                       \
  (AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC1)
#else
#define CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV                                       \
  (AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC3)
#endif

const struct AUD_IO_PATH_CFG_T
    cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM] = {
#if defined(SPEECH_TX_AEC_CODEC_REF)
        // NOTE: If enable Ch5 and CH6, need to add channel_num when setup
        // audioflinger stream
        {
            AUD_INPUT_PATH_MAINMIC,
            CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV | AUD_CHANNEL_MAP_CH4,
        },
#else
        {
            AUD_INPUT_PATH_MAINMIC,
            CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV,
        },
#endif
        {
            AUD_INPUT_PATH_LINEIN,
            CFG_HW_AUD_INPUT_PATH_LINEIN_DEV,
        },
#ifdef VOICE_DETECTOR_EN
        {
            AUD_INPUT_PATH_VADMIC,
            CFG_HW_AUD_INPUT_PATH_VADMIC_DEV,
        },
#else
        {
            AUD_INPUT_PATH_ASRMIC,
            CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV,
        },
#endif
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP MuteOutPwl = {
    HAL_IOMUX_PIN_P1_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_NOPULL};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_enable_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {
    HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg =
    {HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_tws_channel_cfg = {
    HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE // HAL_IOMUX_PIN_P1_5
                                // 500:HAL_IOMUX_PIN_P2_5
};
/*
const struct HAL_IOMUX_PIN_FUNCTION_MAP TOUCH_INT ={
        HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
HAL_IOMUX_PIN_PULLUP_ENABLE
};
*/

const struct HAL_IOMUX_PIN_FUNCTION_MAP TOUCH_I2C_SDA = {
    HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};

const struct HAL_IOMUX_PIN_FUNCTION_MAP TOUCH_I2C_SCL = {
    HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
    HAL_IOMUX_PIN_PULLUP_ENABLE};
bool tgt_tws_get_channel_is_right(void) {
#ifdef __FIXED_TWS_EAR_SIDE__
  return TWS_EAR_SIDE_ROLE;
#else
  return hal_gpio_pin_get_val((enum HAL_GPIO_PIN_T)cfg_hw_tws_channel_cfg.pin);
#endif
}

const IIR_CFG_T audio_eq_sw_iir_cfg = {
    .gain0 = 0,
    .gain1 = 0,
    .num = 5,
    .param = {{IIR_TYPE_PEAK, .0, 200, 2},
              {IIR_TYPE_PEAK, .0, 600, 2},
              {IIR_TYPE_PEAK, .0, 2000.0, 2},
              {IIR_TYPE_PEAK, .0, 6000.0, 2},
              {IIR_TYPE_PEAK, .0, 12000.0, 2}}};

const IIR_CFG_T *const audio_eq_sw_iir_cfg_list[EQ_SW_IIR_LIST_NUM] = {
    &audio_eq_sw_iir_cfg,
};

const FIR_CFG_T audio_eq_hw_fir_cfg_44p1k = {.gain = 0.0f,
                                             .len = 384,
                                             .coef = {
                                                 (1 << 23) - 1,
                                             }};

const FIR_CFG_T audio_eq_hw_fir_cfg_48k = {.gain = 0.0f,
                                           .len = 384,
                                           .coef = {
                                               (1 << 23) - 1,
                                           }};

const FIR_CFG_T audio_eq_hw_fir_cfg_96k = {.gain = 0.0f,
                                           .len = 384,
                                           .coef = {
                                               (1 << 23) - 1,
                                           }};

const FIR_CFG_T *const audio_eq_hw_fir_cfg_list[EQ_HW_FIR_LIST_NUM] = {
    &audio_eq_hw_fir_cfg_44p1k,
    &audio_eq_hw_fir_cfg_48k,
    &audio_eq_hw_fir_cfg_96k,
};

// hardware dac iir eq
const IIR_CFG_T audio_eq_hw_dac_iir_cfg = {.gain0 = 0,
                                           .gain1 = 0,
                                           .num = 8,
                                           .param = {
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                               {IIR_TYPE_PEAK, 0, 1000.0, 0.7},
                                           }};

const IIR_CFG_T *const POSSIBLY_UNUSED
    audio_eq_hw_dac_iir_cfg_list[EQ_HW_DAC_IIR_LIST_NUM] = {
        &audio_eq_hw_dac_iir_cfg,
};

// hardware dac iir eq
const IIR_CFG_T audio_eq_hw_adc_iir_adc_cfg = {
    .gain0 = 0,
    .gain1 = 0,
    .num = 1,
    .param = {
        {IIR_TYPE_PEAK, 0.0, 1000.0, 0.7},
    }};

const IIR_CFG_T *const POSSIBLY_UNUSED
    audio_eq_hw_adc_iir_cfg_list[EQ_HW_ADC_IIR_LIST_NUM] = {
        &audio_eq_hw_adc_iir_adc_cfg,
};

// hardware iir eq
const IIR_CFG_T audio_eq_hw_iir_cfg = {.gain0 = 0,
                                       .gain1 = 0,
                                       .num = 8,
                                       .param = {
                                           {IIR_TYPE_PEAK, -10.1, 100.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 400.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 700.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 1000.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 3000.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 5000.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 7000.0, 7},
                                           {IIR_TYPE_PEAK, -10.1, 9000.0, 7},

                                       }};

const IIR_CFG_T *const POSSIBLY_UNUSED
    audio_eq_hw_iir_cfg_list[EQ_HW_IIR_LIST_NUM] = {
        &audio_eq_hw_iir_cfg,
};

const DrcConfig audio_drc_cfg = {.knee = 3,
                                 .filter_type = {14, -1},
                                 .band_num = 2,
                                 .look_ahead_time = 10,
                                 .band_settings = {
                                     {-20, 0, 2, 3, 3000, 1},
                                     {-20, 0, 2, 3, 3000, 1},
                                 }};

const LimiterConfig audio_drc2_cfg = {
    .knee = 2,
    .look_ahead_time = 10,
    .threshold = -20,
    .makeup_gain = 19,
    .ratio = 1000,
    .attack_time = 3,
    .release_time = 3000,
};

const SpectrumFixConfig audio_spectrum_cfg = {
    .freq_num = 9,
    .freq_list = {200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800},
};
