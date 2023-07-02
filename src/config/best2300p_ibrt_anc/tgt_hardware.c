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
#include "aud_section.h"
#include "drc.h"
#include "fir_process.h"
#include "iir_process.h"
#include "limiter.h"
#include "spectrum_fix.h"

const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM] = {
#if (CFG_HW_PLW_NUM > 0)
    {HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
    {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
     HAL_IOMUX_PIN_PULLUP_ENABLE},
#endif
};

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
#if (CFG_HW_GPIOKEY_NUM > 0)
#ifdef BES_AUDIO_DEV_Main_Board_9v0
    {HAL_KEY_CODE_FN1,
     {HAL_IOMUX_PIN_P0_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN2,
     {HAL_IOMUX_PIN_P0_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN3,
     {HAL_IOMUX_PIN_P0_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN4,
     {HAL_IOMUX_PIN_P0_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN5,
     {HAL_IOMUX_PIN_P2_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN6,
     {HAL_IOMUX_PIN_P2_1, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
#else
#ifndef TPORTS_KEY_COEXIST
    {HAL_KEY_CODE_FN1,
     {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN2,
     {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    // {HAL_KEY_CODE_FN3,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO,
    // HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN15,
     {HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
#else
    {HAL_KEY_CODE_FN1,
     {HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
    {HAL_KEY_CODE_FN15,
     {HAL_IOMUX_PIN_P1_0, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO,
      HAL_IOMUX_PIN_PULLUP_ENABLE}},
#endif
#endif
#ifdef IS_MULTI_AI_ENABLED
//{HAL_KEY_CODE_FN13,{HAL_IOMUX_PIN_P1_3, HAL_IOMUX_FUNC_AS_GPIO,
// HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
//{HAL_KEY_CODE_FN14,{HAL_IOMUX_PIN_P1_2, HAL_IOMUX_FUNC_AS_GPIO,
// HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE}},
#endif
#endif
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

#if SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 2
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 | AUD_VMIC_MAP_VMIC1)
#elif SPEECH_CODEC_CAPTURE_CHANNEL_NUM == 3
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 | AUD_CHANNEL_MAP_CH4 |           \
   AUD_VMIC_MAP_VMIC1)
#else
#define CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV                                      \
  (AUD_CHANNEL_MAP_CH0 | AUD_VMIC_MAP_VMIC1)
#endif
#define CFG_HW_AUD_INPUT_PATH_LINEIN_DEV                                       \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define CFG_HW_AUD_INPUT_PATH_VADMIC_DEV                                       \
  (AUD_CHANNEL_MAP_CH4 | AUD_VMIC_MAP_VMIC1)
#define CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV                                       \
  (AUD_CHANNEL_MAP_CH0 | AUD_VMIC_MAP_VMIC1)

const struct AUD_IO_PATH_CFG_T
    cfg_audio_input_path_cfg[CFG_HW_AUD_INPUT_PATH_NUM] = {
#if defined(SPEECH_TX_AEC_CODEC_REF)
        // NOTE: If enable Ch5 and CH6, need to add channel_num when setup
        // audioflinger stream
        {
            AUD_INPUT_PATH_MAINMIC,
            CFG_HW_AUD_INPUT_PATH_MAINMIC_DEV | AUD_CHANNEL_MAP_ECMIC_CH0,
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
        {
            AUD_INPUT_PATH_VADMIC,
            CFG_HW_AUD_INPUT_PATH_VADMIC_DEV,
        },
        {
            AUD_INPUT_PATH_ASRMIC,
            CFG_HW_AUD_INPUT_PATH_ASRMIC_DEV,
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

/*

//ff


Filter1_B=[     42462788,    -84862242,     42399478];
Filter1_A=[    134217728,   -268358003,    134140286];

Filter2_B=[    135905569,   -267224817,    131334465];
Filter2_A=[    134217728,   -267224817,    133022306];

Filter3_B=[    132936489,   -263935268,    131067941];
Filter3_A=[    134217728,   -263935268,    129786702];

Filter4_B=[    131758190,   -257297054,    126191415];
Filter4_A=[    134217728,   -257297054,    123731878];



*/

#define IIR_COUNTER_FF_L (6)
#define IIR_COUNTER_FF_R (6)
#define IIR_COUNTER_FB_L (5)
#define IIR_COUNTER_FB_R (5)

static const struct_anc_cfg POSSIBLY_UNUSED
    AncFirCoef_50p7k_mode0 =
        {
            .anc_cfg_ff_l =
                {
                    // .total_gain = 440,
                    .total_gain = 350,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_L,

                    .iir_coef[0].coef_b = {42462788, -84862242, 42399478},
                    .iir_coef[0].coef_a = {134217728, -268358003, 134140286},

                    .iir_coef[1].coef_b = {135905569, -267224817, 131334465},
                    .iir_coef[1].coef_a = {134217728, -267224817, 133022306},

                    .iir_coef[2].coef_b = {132936489, -263935268, 131067941},
                    .iir_coef[2].coef_a = {134217728, -263935268, 129786702},

                    .iir_coef[3].coef_b = {131758190, -257297054, 126191415},
                    .iir_coef[3].coef_a = {134217728, -257297054, 123731878},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_ff_r =
                {
                    //  .total_gain = 382,
                    .total_gain = 350,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_R,

                    .iir_coef[0].coef_b = {42462788, -84862242, 42399478},
                    .iir_coef[0].coef_a = {134217728, -268358003, 134140286},

                    .iir_coef[1].coef_b = {135905569, -267224817, 131334465},
                    .iir_coef[1].coef_a = {134217728, -267224817, 133022306},

                    .iir_coef[2].coef_b = {132936489, -263935268, 131067941},
                    .iir_coef[2].coef_a = {134217728, -263935268, 129786702},

                    .iir_coef[3].coef_b = {131758190, -257297054, 126191415},
                    .iir_coef[3].coef_a = {134217728, -257297054, 123731878},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },

            /*

            Filter1_B=[     27461831,    -54408898,     27001841];
            Filter1_A=[    134217728,   -216605724,     82606056];

            Filter2_B=[    138294078,   -267600712,    129323227];
            Filter2_A=[    134217728,   -267600712,    133399577];

            Filter3_B=[    134500015,   -268177932,    133678688];
            Filter3_A=[    134217728,   -268177932,    133960975];

            Filter4_B=[    133629164,   -264794659,    131257050];
            Filter4_A=[    134217728,   -264794659,    130668486];


            */

            .anc_cfg_fb_l =
                {
                    .total_gain = 350,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_L,

                    .iir_coef[0].coef_b = {27461831, -54408898, 27001841},
                    .iir_coef[0].coef_a = {134217728, -216605724, 82606056},

                    .iir_coef[1].coef_b = {138294078, -267600712, 129323227},
                    .iir_coef[1].coef_a = {134217728, -267600712, 133399577},

                    .iir_coef[2].coef_b = {134500015, -268177932, 133678688},
                    .iir_coef[2].coef_a = {134217728, -268177932, 133960975},

                    .iir_coef[3].coef_b = {133629164, -264794659, 131257050},
                    .iir_coef[3].coef_a = {134217728, -264794659, 130668486},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */

                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_fb_r =
                {
                    .total_gain = 350,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_R,

                    .iir_coef[0].coef_b = {27461831, -54408898, 27001841},
                    .iir_coef[0].coef_a = {134217728, -216605724, 82606056},

                    .iir_coef[1].coef_b = {138294078, -267600712, 129323227},
                    .iir_coef[1].coef_a = {134217728, -267600712, 133399577},

                    .iir_coef[2].coef_b = {134500015, -268177932, 133678688},
                    .iir_coef[2].coef_a = {134217728, -268177932, 133960975},

                    .iir_coef[3].coef_b = {133629164, -264794659, 131257050},
                    .iir_coef[3].coef_a = {134217728, -264794659, 130668486},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },

};

/*

//ff


Filter1_B=[     42463913,    -84860822,     42396935];
Filter1_A=[    134217728,   -268353516,    134135801];

Filter2_B=[    136002894,   -267154076,    131168209];
Filter2_A=[    134217728,   -267154076,    132953376];

Filter3_B=[    132863566,   -263674901,    130888668];
Filter3_A=[    134217728,   -263674901,    129534506];

Filter4_B=[    131621817,   -256639526,    125746382];
Filter4_A=[    134217728,   -256639526,    123150471];



*/

static const struct_anc_cfg POSSIBLY_UNUSED
    AncFirCoef_48k_mode0 =
        {
            .anc_cfg_ff_l =
                {
                    // .total_gain = 440,
                    .total_gain = 312,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_L,

                    .iir_coef[0].coef_b = {42463913, -84860822, 42396935},
                    .iir_coef[0].coef_a = {134217728, -268353516, 134135801},

                    .iir_coef[1].coef_b = {136002894, -267154076, 131168209},
                    .iir_coef[1].coef_a = {134217728, -267154076, 132953376},

                    .iir_coef[2].coef_b = {132863566, -263674901, 130888668},
                    .iir_coef[2].coef_a = {134217728, -263674901, 129534506},

                    .iir_coef[3].coef_b = {131621817, -256639526, 125746382},
                    .iir_coef[3].coef_a = {134217728, -256639526, 123150471},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_ff_r =
                {
                    //  .total_gain = 382,
                    .total_gain = 288,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_R,

                    .iir_coef[0].coef_b = {42463913, -84860822, 42396935},
                    .iir_coef[0].coef_a = {134217728, -268353516, 134135801},

                    .iir_coef[1].coef_b = {136002894, -267154076, 131168209},
                    .iir_coef[1].coef_a = {134217728, -267154076, 132953376},

                    .iir_coef[2].coef_b = {132863566, -263674901, 130888668},
                    .iir_coef[2].coef_a = {134217728, -263674901, 129534506},

                    .iir_coef[3].coef_b = {131621817, -256639526, 125746382},
                    .iir_coef[3].coef_a = {134217728, -256639526, 123150471},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},
                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },

            /*

            Filter1_B=[     27172676,    -53803459,     26691412];
            Filter1_A=[    134217728,   -214195429,     80219070];

            Filter2_B=[    138529480,   -267551490,    129040578];
            Filter2_A=[    134217728,   -267551490,    133352330];

            Filter3_B=[    134516353,   -268162980,    133647489];
            Filter3_A=[    134217728,   -268162980,    133946114];

            Filter4_B=[    133595549,   -264581113,    131087955];
            Filter4_A=[    134217728,   -264581113,    130465777];


            */

            .anc_cfg_fb_l =
                {
                    .total_gain = 511,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_L,

                    .iir_coef[0].coef_b = {27172676, -53803459, 26691412},
                    .iir_coef[0].coef_a = {134217728, -214195429, 80219070},

                    .iir_coef[1].coef_b = {138529480, -267551490, 129040578},
                    .iir_coef[1].coef_a = {134217728, -267551490, 133352330},

                    .iir_coef[2].coef_b = {134516353, -268162980, 133647489},
                    .iir_coef[2].coef_a = {134217728, -268162980, 133946114},

                    .iir_coef[3].coef_b = {133595549, -264581113, 131087955},
                    .iir_coef[3].coef_a = {134217728, -264581113, 130465777},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_fb_r =
                {
                    .total_gain = 511,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_R,

                    .iir_coef[0].coef_b = {27172676, -53803459, 26691412},
                    .iir_coef[0].coef_a = {134217728, -214195429, 80219070},

                    .iir_coef[1].coef_b = {138529480, -267551490, 129040578},
                    .iir_coef[1].coef_a = {134217728, -267551490, 133352330},

                    .iir_coef[2].coef_b = {134516353, -268162980, 133647489},
                    .iir_coef[2].coef_a = {134217728, -268162980, 133946114},

                    .iir_coef[3].coef_b = {133595549, -264581113, 131087955},
                    .iir_coef[3].coef_a = {134217728, -264581113, 130465777},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },

#if (AUD_SECTION_STRUCT_VERSION == 2)

            /*
            1.0000000000000000,-1.5858874672928407,0.6974239598044429,0.2832267077115959,-0.3117526885614825,0.1400624733614886,
            Filter1_B=[      4751756,     -5230342,      2349858];
            Filter1_A=[     16777216,    -26606777,     11700832];


            1.0000000000000000,-1.7971697583202608,0.8159624512785459,0.9540998606028980,-1.7971697583202608,0.8618625906756480,
            Filter2_B=[     16007139,    -30151505,     14459655];
            Filter2_A=[     16777216,    -30151505,     13689578];


            1.0000000000000000,-1.9694050640918992,0.9705681145972464,0.3200483744622364,-0.6223829329788905,0.3034976090220014,
            Filter3_B=[      5369521,    -10441853,      5091845];
            Filter3_A=[     16777216,    -33041134,     16283431];


            1.0000000000000000,-1.9921619776276678,0.9921812243512138,0.9968660174712476,-1.9921712178765081,0.9953059666311256,
            Filter4_B=[     16724636,    -33423087,     16698463];
            Filter4_A=[     16777216,    -33422932,     16646039];
            */

            /*

            1.0000000000000000,-1.9868580074509832,0.9869011854430232,1.1834688902733632,-2.3614075958038656,1.1779451659756268,
            Filter1_B=[     19855313,    -39617845,     19762640];
            Filter1_A=[     16777216,    -33333946,     16557454];


            1.0000000000000000,-1.0329261527674278,0.0418392318218667,0.5812322628931170,-1.0329261527674278,0.4606069689287498,
            Filter2_B=[      9751459,    -17329625,      7727703];
            Filter2_A=[     16777216,    -17329625,       701946];


            1.0000000000000000,-1.9576081396140492,0.9591185490154677,1.0729914166044796,-1.9576081396140492,0.8861271324109881,
            Filter3_B=[     18001809,    -32843215,     14866746];
            Filter3_A=[     16777216,    -32843215,     16091339];


            1.0000000000000000,-1.9197071583239940,0.9219883336398085,0.7545642546264146,-1.4392920140632206,0.6870089347526202,
            Filter4_B=[     12659487,    -24147313,     11526097];
            Filter4_A=[     16777216,    -32207342,     15468397];


            1.0000000000000000,-1.9102108535747602,0.9139876710346515,0.9829076121866517,-1.9102108535747602,0.9310800588479999,
            Filter5_B=[     16490453,    -32048020,     15620931];
            Filter5_A=[     16777216,    -32048020,     15334169];
            */

            .anc_cfg_mc_l =
                {
                    .total_gain = 1228,

                    .iir_bypass_flag = 0,
                    .iir_counter = 5,

                    .iir_coef[0].coef_b = {19855313, -39617845, 19762640},
                    .iir_coef[0].coef_a = {16777216, -33333946, 16557454},

                    .iir_coef[1].coef_b = {9751459, -17329625, 7727703},
                    .iir_coef[1].coef_a = {16777216, -17329625, 701946},

                    .iir_coef[2].coef_b = {18001809, -32843215, 14866746},
                    .iir_coef[2].coef_a = {16777216, -32843215, 16091339},

                    .iir_coef[3].coef_b = {12659487, -24147313, 11526097},
                    .iir_coef[3].coef_a = {16777216, -32207342, 15468397},

                    .iir_coef[4].coef_b = {16490453, -32048020, 15620931},
                    .iir_coef[4].coef_a = {16777216, -32048020, 15334169},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_mc_r =
                {
                    .total_gain = 1331,

                    .iir_bypass_flag = 0,
                    .iir_counter = 5,

                    .iir_coef[0].coef_b = {19855313, -39617845, 19762640},
                    .iir_coef[0].coef_a = {16777216, -33333946, 16557454},

                    .iir_coef[1].coef_b = {9751459, -17329625, 7727703},
                    .iir_coef[1].coef_a = {16777216, -17329625, 701946},

                    .iir_coef[2].coef_b = {18001809, -32843215, 14866746},
                    .iir_coef[2].coef_a = {16777216, -32843215, 16091339},

                    .iir_coef[3].coef_b = {12659487, -24147313, 11526097},
                    .iir_coef[3].coef_a = {16777216, -32207342, 15468397},

                    .iir_coef[4].coef_b = {16490453, -32048020, 15620931},
                    .iir_coef[4].coef_a = {16777216, -32048020, 15334169},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
#endif
};

/*
//ff


Filter1_B=[     42465729,    -84858529,     42392831];
Filter1_A=[    134217728,   -268346271,    134128558];

Filter2_B=[    136159949,   -267039705,    130899919];
Filter2_A=[    134217728,   -267039705,    132842140];

Filter3_B=[    132746107,   -263254540,    130599907];
Filter3_A=[    134217728,   -263254540,    129128286];

Filter4_B=[    131402980,   -255575175,    125032243];
Filter4_A=[    134217728,   -255575175,    122217496];



*/

static const struct_anc_cfg POSSIBLY_UNUSED
    AncFirCoef_44p1k_mode0 =
        {
            .anc_cfg_ff_l =
                {
                    // .total_gain = 440,
                    .total_gain = 312,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_L,

                    .iir_coef[0].coef_b = {42465729, -84858529, 42392831},
                    .iir_coef[0].coef_a = {134217728, -268346271, 134128558},

                    .iir_coef[1].coef_b = {136159949, -267039705, 130899919},
                    .iir_coef[1].coef_a = {134217728, -267039705, 132842140},

                    .iir_coef[2].coef_b = {132746107, -263254540, 130599907},
                    .iir_coef[2].coef_a = {134217728, -263254540, 129128286},

                    .iir_coef[3].coef_b = {131402980, -255575175, 125032243},
                    .iir_coef[3].coef_a = {134217728, -255575175, 122217496},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_ff_r =
                {
                    //  .total_gain = 382,
                    .total_gain = 288,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FF_R,

                    .iir_coef[0].coef_b = {42465729, -84858529, 42392831},
                    .iir_coef[0].coef_a = {134217728, -268346271, 134128558},

                    .iir_coef[1].coef_b = {136159949, -267039705, 130899919},
                    .iir_coef[1].coef_a = {134217728, -267039705, 132842140},

                    .iir_coef[2].coef_b = {132746107, -263254540, 130599907},
                    .iir_coef[2].coef_a = {134217728, -263254540, 129128286},

                    .iir_coef[3].coef_b = {131402980, -255575175, 125032243},
                    .iir_coef[3].coef_a = {134217728, -255575175, 122217496},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },

            /*

            Filter1_B=[     26719020,    -52852829,     26204379];
            Filter1_A=[    134217728,   -210410903,     76474119];

            Filter2_B=[    138909433,   -267471808,    128584365];
            Filter2_A=[    134217728,   -267471808,    133276071];

            Filter3_B=[    134542733,   -268138827,    133597115];
            Filter3_A=[    134217728,   -268138827,    133922120];

            Filter4_B=[    133541379,   -264235686,    130815458];
            Filter4_A=[    134217728,   -264235686,    130139109];

            */

            .anc_cfg_fb_l =
                {
                    .total_gain = 511,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_L,

                    .iir_coef[0].coef_b = {26719020, -52852829, 26204379},
                    .iir_coef[0].coef_a = {134217728, -210410903, 76474119},

                    .iir_coef[1].coef_b = {138909433, -267471808, 128584365},
                    .iir_coef[1].coef_a = {134217728, -267471808, 133276071},

                    .iir_coef[2].coef_b = {134542733, -268138827, 133597115},
                    .iir_coef[2].coef_a = {134217728, -268138827, 133922120},

                    .iir_coef[3].coef_b = {133541379, -264235686, 130815458},
                    .iir_coef[3].coef_a = {134217728, -264235686, 130139109},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_fb_r =
                {
                    .total_gain = 511,

                    .iir_bypass_flag = 0,
                    .iir_counter = IIR_COUNTER_FB_R,

                    .iir_coef[0].coef_b = {26719020, -52852829, 26204379},
                    .iir_coef[0].coef_a = {134217728, -210410903, 76474119},

                    .iir_coef[1].coef_b = {138909433, -267471808, 128584365},
                    .iir_coef[1].coef_a = {134217728, -267471808, 133276071},

                    .iir_coef[2].coef_b = {134542733, -268138827, 133597115},
                    .iir_coef[2].coef_a = {134217728, -268138827, 133922120},

                    .iir_coef[3].coef_b = {133541379, -264235686, 130815458},
                    .iir_coef[3].coef_a = {134217728, -264235686, 130139109},

                    .iir_coef[4].coef_b = {0x8000000, 0, 0},
                    .iir_coef[4].coef_a = {0x8000000, 0, 0},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    /*		.fir_bypass_flag=1,
                            .fir_len = AUD_COEF_LEN,
                            .fir_coef =
                            {
                                32767,
                            },
                    */
                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
#if (AUD_SECTION_STRUCT_VERSION == 2)

            /*

            Filter1_B=[     19847881,    -39594823,     19747071];
            Filter1_A=[     16777216,    -33314517,     16538159];

            Filter2_B=[      9442890,    -16603187,      7330251];
            Filter2_A=[     16777216,    -16603187,        -4075];

            Filter3_B=[     18107639,    -32779315,     14701642];
            Filter3_A=[     16777216,    -32779315,     16032065];

            Filter4_B=[     12666347,    -24058210,     11437046];
            Filter4_A=[     16777216,    -32089673,     15357640];

            Filter5_B=[     16466312,    -31915122,     15523589];
            Filter5_A=[     16777216,    -31915122,     15212684];
            */

            .anc_cfg_mc_l =
                {
                    .total_gain = 1228,

                    .iir_bypass_flag = 0,
                    .iir_counter = 5,

                    .iir_coef[0].coef_b = {19847881, -39594823, 19747071},
                    .iir_coef[0].coef_a = {16777216, -33314517, 16538159},

                    .iir_coef[1].coef_b = {9442890, -16603187, 7330251},
                    .iir_coef[1].coef_a = {16777216, -16603187, -4075},

                    .iir_coef[2].coef_b = {18107639, -32779315, 14701642},
                    .iir_coef[2].coef_a = {16777216, -32779315, 16032065},

                    .iir_coef[3].coef_b = {12666347, -24058210, 11437046},
                    .iir_coef[3].coef_a = {16777216, -32089673, 15357640},

                    .iir_coef[4].coef_b = {16466312, -31915122, 15523589},
                    .iir_coef[4].coef_a = {16777216, -31915122, 15212684},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
            .anc_cfg_mc_r =
                {
                    .total_gain = 1331,

                    .iir_bypass_flag = 0,
                    .iir_counter = 5,

                    .iir_coef[0].coef_b = {19847881, -39594823, 19747071},
                    .iir_coef[0].coef_a = {16777216, -33314517, 16538159},

                    .iir_coef[1].coef_b = {9442890, -16603187, 7330251},
                    .iir_coef[1].coef_a = {16777216, -16603187, -4075},

                    .iir_coef[2].coef_b = {18107639, -32779315, 14701642},
                    .iir_coef[2].coef_a = {16777216, -32779315, 16032065},

                    .iir_coef[3].coef_b = {12666347, -24058210, 11437046},
                    .iir_coef[3].coef_a = {16777216, -32089673, 15357640},

                    .iir_coef[4].coef_b = {16466312, -31915122, 15523589},
                    .iir_coef[4].coef_a = {16777216, -31915122, 15212684},

                    .iir_coef[5].coef_b = {0x8000000, 0, 0},
                    .iir_coef[5].coef_a = {0x8000000, 0, 0},

                    .dac_gain_offset = 0,
                    .adc_gain_offset = (0) * 4,
                },
#endif

};

const struct_anc_cfg *anc_coef_list_50p7k[ANC_COEF_LIST_NUM] = {
    &AncFirCoef_50p7k_mode0,
#if (ANC_COEF_LIST_NUM == 2)
    &AncFirCoef_50p7k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 3)
    &AncFirCoef_50p7k_mode0, &AncFirCoef_50p7k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 4)
    &AncFirCoef_50p7k_mode0, &AncFirCoef_50p7k_mode0, &AncFirCoef_50p7k_mode0,
#endif
};

const struct_anc_cfg *anc_coef_list_48k[ANC_COEF_LIST_NUM] = {
    &AncFirCoef_48k_mode0,
#if (ANC_COEF_LIST_NUM == 2)
    &AncFirCoef_48k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 3)
    &AncFirCoef_48k_mode0, &AncFirCoef_48k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 4)
    &AncFirCoef_48k_mode0, &AncFirCoef_48k_mode0, &AncFirCoef_48k_mode0,
#endif
};

const struct_anc_cfg *anc_coef_list_44p1k[ANC_COEF_LIST_NUM] = {
    &AncFirCoef_44p1k_mode0,
#if (ANC_COEF_LIST_NUM == 2)
    &AncFirCoef_44p1k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 3)
    &AncFirCoef_44p1k_mode0, &AncFirCoef_44p1k_mode0,
#endif
#if (ANC_COEF_LIST_NUM == 4)
    &AncFirCoef_44p1k_mode0, &AncFirCoef_44p1k_mode0, &AncFirCoef_44p1k_mode0,
#endif
};

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
