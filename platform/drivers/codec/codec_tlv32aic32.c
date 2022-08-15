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
#include "plat_types.h"
#include "codec_tlv32aic32.h"
#include "hal_i2c.h"
#include "hal_i2s.h"
#include "hal_uart.h"
#include "hal_trace.h"
#include "string.h"

#define TLV32AIC32_INST HAL_I2S_ID_0

static struct HAL_I2C_CONFIG_T _codec_i2c_cfg;

static uint8_t sample_rate_div[] = {
    10, /* index is 0 */
    15,
    20,
    25,
    30,
    35,
    40,
    45,
    50,
    55,
    60,
};

/* i2c write */
static int codec_write(uint8_t reg_addr, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = reg_addr;
    buf[1] = value;

    return hal_i2c_task_send(HAL_I2C_ID_0, TLV32AIC32_I2C_ADDRESS, buf, 1 + 1, 0, 0);
}

/* i2c read */
#if 0
static uint8_t codec_read(uint8_t reg_addr)
{
    uint8_t buf[2];

    buf[0] = reg_addr;
    buf[1] = 0;

    hal_i2c_task_recv(HAL_I2C_ID_0, TLV32AIC32_I2C_ADDRESS, &buf[0], 1, &buf[1], 1, 0, 0);

    return buf[1];
}
#endif

uint32_t tlv32aic32_open(void)
{
    _codec_i2c_cfg.mode = HAL_I2C_API_MODE_TASK;
    _codec_i2c_cfg.use_dma  = 0;
    _codec_i2c_cfg.use_sync = 1;
    _codec_i2c_cfg.speed = 20000;
    _codec_i2c_cfg.as_master = 1;
    hal_i2c_open(HAL_I2C_ID_0, &_codec_i2c_cfg);

    return 0;
}
uint32_t tlv32aic32_stream_open(enum AUD_STREAM_T stream)
{
    hal_i2s_open(TLV32AIC32_INST, stream, HAL_I2S_MODE_MASTER);

    return 0;
}

uint32_t tlv32aic32_stream_setup(enum AUD_STREAM_T stream, struct tlv32aic32_config_t *cfg)
{
    uint32_t param = 0, i = 0;

    /* select page : 0*/
    codec_write(AIC3X_PAGE_SELECT, PAGE0_SELECT);

    /* soft reset : 1 */
    codec_write(AIC3X_RESET, SOFT_RESET);

    /* CLKDIV_IN uses mCLK : 102 , PLLDIV_IN use mCLK */
    codec_write(AIC3X_CLKGEN_CTRL_REG, 0x02);

    /* CODEC_CLK_IN uses CLKDIV : 101, CODEC_CLK_IN use CLKDIV_IN */
    codec_write(101, 0x00);

    /* PLL R value : 11, R is 1 */
    codec_write(AIC3X_OVRF_STATUS_AND_PLLR_REG, 0x1);

    /* PLL J value : 4, J is 8 */
    codec_write(AIC3X_PLL_PROGB_REG, 0x8);

    /* PLL D value : 5,6, D is 1920 */
    codec_write(AIC3X_PLL_PROGC_REG, 0x780>>6);

    codec_write(AIC3X_PLL_PROGD_REG, (0x780&0x3f)<<2);

    /* PLL disable and select Q value : 3, enable pll, Q is 2 */
    codec_write(AIC3X_PLL_PROGA_REG, 0x81);

    /* left and right DAC open : 7, open codec dac path and fref */
    if ((cfg->sample_rate%8000) == 0) {
        /* 48000 */
        codec_write(AIC3X_CODEC_DATAPATH_REG,  0x0a);
        param = 480000/cfg->sample_rate;
    }
    else {
        /* 44100 */
        codec_write(AIC3X_CODEC_DATAPATH_REG,  0x8a);
        param = 441000/cfg->sample_rate;
    }

    for (i = 0; i < sizeof(sample_rate_div); ++i) {
        if (param == sample_rate_div[i])
            break;
    }

    if (i == sizeof(sample_rate_div)) {
        i = 0;
    }

    /* sample_rate divider : 2, default is /1 */
    codec_write(AIC3X_SAMPLE_RATE_SEL_REG,  i<<4|i<<4);

    /* ctrl mode : 8, bitclock input, word clock input */
    codec_write(AIC3X_ASD_INTF_CTRLA,  0x20);

    /* Audio Serial Data Interface Control : 9, 16bit i2s mode */
    if (cfg->bits == AUD_BITS_16)
        param = 0;
    else if (cfg->bits == AUD_BITS_20)
        param = 1;
    else if (cfg->bits == AUD_BITS_24)
        param = 2;
    else if (cfg->bits == AUD_BITS_32)
        param = 3;
    else
        param = 0;


    codec_write(AIC3X_ASD_INTF_CTRLB,  0x00 | param<<4);

    /* out ac-coupled : 14, headset control */
    codec_write(AIC3X_HEADSET_DETECT_CTRL_B, 0x08);

    /* left and right DAC power on : 37 */
    //codec_write(DAC_PWR, 0xC0);
    codec_write(DAC_PWR, 0xD0);

    /* out common-mode voltage : 40 */
    codec_write(HPOUT_SC, 0x00);

    /* out path select : 41 */
    codec_write(DAC_LINE_MUX, 0xA1);

    /* left DAC not muted : 43 */
    codec_write(LDAC_VOL, 0x0f);

    /* right DAC not muted : 44 */
    codec_write(RDAC_VOL, 0x0f);

    /* HPLOUT is not muted : 51 */
    codec_write(HPLOUT_CTRL, 0x9F);

    /* HPROUT is not muted : 65 */
    codec_write(HPROUT_CTRL, 0x9F);

    /* out short circuit protection : 38 */
    //codec_write(HPRCOM_CFG, 0x3A);
    codec_write(HPRCOM_CFG, 0x22);

    /* input */
    /* mic bias : 25 */
    codec_write(MICBIAS_CTRL, 0x40);
    /* left adc power : 19 */
    codec_write(LINE1L_2_LADC_CTRL, 0x04);
    /* right adc power : 22 */
    codec_write(LINE1R_2_RADC_CTRL, 0x04);
    /* left adc pga mute : 15 */
    codec_write(LADC_VOL, 0x0);
    /* right adc pga mute : 16 */
    codec_write(RADC_VOL, 0x0);
    /* adc flag : 36 */
    //TRACE(1,"tlv adc flag 0x%x", codec_read(ADC_FLAG));
    /* left adc pga mute : 15 */
    //TRACE(1,"tlv left adc pga 0x%x", codec_read(LADC_VOL));
    /* right adc pga mute : 16 */
    //TRACE(1,"tlv right adc pga 0x%x", codec_read(RADC_VOL));
    /* input end */

    struct HAL_I2S_CONFIG_T i2s_cfg;

    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.use_dma = cfg->use_dma;
    i2s_cfg.chan_sep_buf = cfg->chan_sep_buf;
    i2s_cfg.sync_start = cfg->sync_start;
    i2s_cfg.cycles = cfg->slot_cycles;
    i2s_cfg.bits = cfg->bits;
    i2s_cfg.channel_num = cfg->channel_num;
    i2s_cfg.channel_map = cfg->channel_map;
    i2s_cfg.sample_rate = cfg->sample_rate;
    hal_i2s_setup_stream(TLV32AIC32_INST, stream, &i2s_cfg);

    return 0;
}

uint32_t tlv32aic32_stream_start(enum AUD_STREAM_T stream)
{
    hal_i2s_start_stream(TLV32AIC32_INST, stream);
    return 0;
}
uint32_t tlv32aic32_stream_stop(enum AUD_STREAM_T stream)
{
    hal_i2s_stop_stream(TLV32AIC32_INST, stream);
    return 0;
}
uint32_t tlv32aic32_stream_close(enum AUD_STREAM_T stream)
{
    hal_i2s_close(TLV32AIC32_INST, stream);
    return 0;
}
uint32_t tlv32aic32_close(void)
{
    return 0;
}
