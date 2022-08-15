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
#ifdef CHIP_HAS_SPDIF

#include "plat_types.h"
#include "plat_addr_map.h"
#include "reg_spdifip.h"
#include "hal_spdifip.h"
#include "hal_spdif.h"
#include "hal_iomux.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "analog.h"
#include "cmsis.h"

//#define SPDIF_CLOCK_SOURCE 240000000
//#define SPDIF_CLOCK_SOURCE 22579200
//#define SPDIF_CLOCK_SOURCE 48000000
#define SPDIF_CLOCK_SOURCE 3072000

//#define SPDIF_CLOCK_SOURCE 76800000
//#define SPDIF_CLOCK_SOURCE 84672000

// Trigger DMA request when TX-FIFO count <= threshold
#define HAL_SPDIF_TX_FIFO_TRIGGER_LEVEL     (SPDIFIP_FIFO_DEPTH/2)
// Trigger DMA request when RX-FIFO count >= threshold
#define HAL_SPDIF_RX_FIFO_TRIGGER_LEVEL     (SPDIFIP_FIFO_DEPTH/2)

#define HAL_SPDIF_YES 1
#define HAL_SPDIF_NO 0

#ifdef CHIP_BEST2000
#define SPDIF_CMU_DIV                       CODEC_CMU_DIV
#else
#define SPDIF_CMU_DIV                       CODEC_PLL_DIV
#endif

enum HAL_SPDIF_STATUS_T {
    HAL_SPDIF_STATUS_NULL,
    HAL_SPDIF_STATUS_OPENED,
    HAL_SPDIF_STATUS_STARTED,
};

struct HAL_SPDIF_MOD_NAME_T {
    enum HAL_CMU_MOD_ID_T mod;
    enum HAL_CMU_MOD_ID_T apb;
};

struct HAL_SPDIF_MOD_NAME_T spdif_mod[HAL_SPDIF_ID_QTY] = {
    {
        .mod = HAL_CMU_MOD_O_SPDIF0,
        .apb = HAL_CMU_MOD_P_SPDIF0,
    },
#if (CHIP_HAS_SPDIF > 1)
    {
        .mod = HAL_CMU_MOD_O_SPDIF1,
        .apb = HAL_CMU_MOD_P_SPDIF1,
    },
#endif
};

static const char * const invalid_id = "Invalid SPDIF ID: %d\n";
//static const char * const invalid_ch = "Invalid SPDIF CH: %d\n";

struct SPDIF_SAMPLE_RATE_T {
    enum AUD_SAMPRATE_T sample_rate;
    uint32_t codec_freq;
    uint8_t codec_div;
    uint8_t pcm_div;
    uint8_t tx_ratio;
};

// SAMPLE_RATE * 128 = PLL_nominal / PCM_DIV / TX_RATIO
static const struct SPDIF_SAMPLE_RATE_T spdif_sample_rate[]={
#if defined(CHIP_BEST1000) && defined(AUD_PLL_DOUBLE)
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES*2, CODEC_PLL_DIV, SPDIF_CMU_DIV,  4},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES*2, CODEC_PLL_DIV, SPDIF_CMU_DIV,  2},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES*2, CODEC_PLL_DIV, SPDIF_CMU_DIV,  1},
#else
    {AUD_SAMPRATE_8000,   CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV, 24},
    {AUD_SAMPRATE_16000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV, 12},
    {AUD_SAMPRATE_22050,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, SPDIF_CMU_DIV,  8},
    {AUD_SAMPRATE_24000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV,  8},
    {AUD_SAMPRATE_44100,  CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, SPDIF_CMU_DIV,  4},
    {AUD_SAMPRATE_48000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV,  4},
    {AUD_SAMPRATE_96000,  CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV,  2},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES,   CODEC_PLL_DIV, SPDIF_CMU_DIV,  1},
#endif
};

#if (CHIP_HAS_SPDIF > 1)
static uint8_t spdif_map;
STATIC_ASSERT(HAL_SPDIF_ID_QTY <= sizeof(spdif_map) * 8, "Too many SPDIF IDs");
#endif

static enum HAL_SPDIF_STATUS_T spdif_status[HAL_SPDIF_ID_QTY][AUD_STREAM_NUM];
static bool spdif_dma[HAL_SPDIF_ID_QTY][AUD_STREAM_NUM];

static inline uint32_t _spdif_get_reg_base(enum HAL_SPDIF_ID_T id)
{
    ASSERT(id < HAL_SPDIF_ID_QTY, invalid_id, id);

    switch(id) {
        case HAL_SPDIF_ID_0:
        default:
            return SPDIF0_BASE;
            break;
#if (CHIP_HAS_SPDIF > 1)
        case HAL_SPDIF_ID_1:
            return SPDIF1_BASE;
            break;
#endif
    }
	return 0;
}

int hal_spdif_open(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;

    reg_base = _spdif_get_reg_base(id);

    if (spdif_status[id][stream] != HAL_SPDIF_STATUS_NULL) {
        TRACE(2,"Invalid SPDIF opening status for stream %d: %d", stream, spdif_status[id][stream]);
        return 1;
    }

    if (spdif_status[id][AUD_STREAM_PLAYBACK] == HAL_SPDIF_STATUS_NULL &&
            spdif_status[id][AUD_STREAM_CAPTURE] == HAL_SPDIF_STATUS_NULL) {
#ifndef SIMU
        bool cfg_pll = true;
        int i;

        for (i = HAL_SPDIF_ID_0; i < HAL_SPDIF_ID_QTY; i++) {
            if (spdif_status[i][AUD_STREAM_PLAYBACK] != HAL_SPDIF_STATUS_NULL ||
                    spdif_status[i][AUD_STREAM_CAPTURE] != HAL_SPDIF_STATUS_NULL) {
                cfg_pll = false;
                break;
            }
        }
        if (cfg_pll) {
            analog_aud_pll_open(ANA_AUD_PLL_USER_SPDIF);
        }
#endif

#if (CHIP_HAS_SPDIF > 1)
        if (id == HAL_SPDIF_ID_1) {
            hal_iomux_set_spdif1();
        } else
#endif
        {
            hal_iomux_set_spdif0();
        }
        hal_cmu_spdif_clock_enable(id);
        hal_cmu_clock_enable(spdif_mod[id].mod);
        hal_cmu_clock_enable(spdif_mod[id].apb);
        hal_cmu_reset_clear(spdif_mod[id].mod);
        hal_cmu_reset_clear(spdif_mod[id].apb);

        spdifip_w_enable_spdifip(reg_base, HAL_SPDIF_YES);
    }

    spdif_dma[id][stream] = false;
    spdif_status[id][stream] = HAL_SPDIF_STATUS_OPENED;

    return 0;
}

int hal_spdif_close(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;

    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    if (spdif_status[id][stream] != HAL_SPDIF_STATUS_OPENED) {
        TRACE(2,"Invalid SPDIF closing status for stream %d: %d", stream, spdif_status[id][stream]);
        return 1;
    }

    spdif_status[id][stream] = HAL_SPDIF_STATUS_NULL;

    if (spdif_status[id][AUD_STREAM_PLAYBACK] == HAL_SPDIF_STATUS_NULL &&
            spdif_status[id][AUD_STREAM_CAPTURE] == HAL_SPDIF_STATUS_NULL) {
        reg_base = _spdif_get_reg_base(id);

        spdifip_w_enable_spdifip(reg_base, HAL_SPDIF_NO);

        hal_cmu_spdif_set_div(id, 0x1FFF + 2);
        hal_cmu_reset_set(spdif_mod[id].apb);
        hal_cmu_reset_set(spdif_mod[id].mod);
        hal_cmu_clock_disable(spdif_mod[id].apb);
        hal_cmu_clock_disable(spdif_mod[id].mod);
        hal_cmu_spdif_clock_disable(id);

#ifndef SIMU
        bool cfg_pll = true;
        int i;

        for (i = HAL_SPDIF_ID_0; i < HAL_SPDIF_ID_QTY; i++) {
            if (spdif_status[i][AUD_STREAM_PLAYBACK] != HAL_SPDIF_STATUS_NULL ||
                    spdif_status[i][AUD_STREAM_CAPTURE] != HAL_SPDIF_STATUS_NULL) {
                cfg_pll = false;
                break;
            }
        }
        if (cfg_pll) {
            analog_aud_pll_close(ANA_AUD_PLL_USER_SPDIF);
        }
#endif
    }

    return 0;
}

int hal_spdif_start_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;
    uint32_t lock;

    reg_base = _spdif_get_reg_base(id);

    if (spdif_status[id][stream] != HAL_SPDIF_STATUS_OPENED) {
        TRACE(2,"Invalid SPDIF starting status for stream %d: %d", stream, spdif_status[id][stream]);
        return 1;
    }

    if (stream == AUD_STREAM_PLAYBACK) {
        lock = int_lock();
        spdifip_w_enable_tx_channel0(reg_base, HAL_SPDIF_YES);
        spdifip_w_enable_tx(reg_base, HAL_SPDIF_YES);
        spdifip_w_tx_valid(reg_base, HAL_SPDIF_YES);
        if (spdif_dma[id][stream]) {
            spdifip_w_enable_tx_dma(reg_base, HAL_SPDIF_YES);
        }
        int_unlock(lock);
    } else {
        if (spdif_dma[id][stream]) {
            spdifip_w_enable_rx_dma(reg_base, HAL_SPDIF_YES);
        }
        spdifip_w_enable_rx_channel0(reg_base, HAL_SPDIF_YES);
        spdifip_w_enable_rx(reg_base, HAL_SPDIF_YES);
        spdifip_w_sample_en(reg_base, HAL_SPDIF_YES);
    }

    spdif_status[id][stream] = HAL_SPDIF_STATUS_STARTED;

    return 0;
}

int hal_spdif_stop_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream)
{
    uint32_t reg_base;

    reg_base = _spdif_get_reg_base(id);

    if (spdif_status[id][stream] != HAL_SPDIF_STATUS_STARTED) {
        TRACE(2,"Invalid SPDIF stopping status for stream %d: %d", stream, spdif_status[id][stream]);
        return 1;
    }

    spdif_status[id][stream] = HAL_SPDIF_STATUS_OPENED;

    if (stream == AUD_STREAM_PLAYBACK) {
        spdifip_w_enable_tx(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_tx_channel0(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_tx_dma(reg_base, HAL_SPDIF_NO);
        spdifip_w_tx_fifo_reset(reg_base);
    } else {
        spdifip_w_enable_rx(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_rx_channel0(reg_base, HAL_SPDIF_NO);
        spdifip_w_enable_rx_dma(reg_base, HAL_SPDIF_NO);
        spdifip_w_rx_fifo_reset(reg_base);
    }

    return 0;
}

int hal_spdif_setup_stream(enum HAL_SPDIF_ID_T id, enum AUD_STREAM_T stream, struct HAL_SPDIF_CONFIG_T *cfg)
{
    uint8_t i;
    uint32_t reg_base;
    uint8_t fmt;

    reg_base = _spdif_get_reg_base(id);

    if (spdif_status[id][stream] != HAL_SPDIF_STATUS_OPENED) {
        TRACE(2,"Invalid SPDIF setup status for stream %d: %d", stream, spdif_status[id][stream]);
        return 1;
    }

    for (i = 0; i < ARRAY_SIZE(spdif_sample_rate); i++)
    {
        if (spdif_sample_rate[i].sample_rate == cfg->sample_rate)
        {
            break;
        }
    }
    ASSERT(i < ARRAY_SIZE(spdif_sample_rate), "%s: Invalid spdif sample rate: %d", __func__, cfg->sample_rate);

    TRACE(3,"[%s] stream=%d sample_rate=%d", __func__, stream, cfg->sample_rate);

#ifdef FPGA
    hal_cmu_spdif_set_div(id, 2);
#else
#ifndef SIMU
    analog_aud_freq_pll_config(spdif_sample_rate[i].codec_freq, spdif_sample_rate[i].codec_div);
#ifdef CHIP_BEST2000
    analog_aud_pll_set_dig_div(spdif_sample_rate[i].codec_div / spdif_sample_rate[i].pcm_div);
#endif
#endif
    // SPDIF module is working on 24.576M or 22.5792M
    hal_cmu_spdif_set_div(id, spdif_sample_rate[i].pcm_div);
#endif

    if ((stream == AUD_STREAM_PLAYBACK && spdif_status[id][AUD_STREAM_CAPTURE] == HAL_SPDIF_STATUS_NULL) ||
            (stream == AUD_STREAM_CAPTURE && spdif_status[id][AUD_STREAM_PLAYBACK] == HAL_SPDIF_STATUS_NULL)) {
        hal_cmu_reset_pulse(spdif_mod[id].mod);
    }

    spdif_dma[id][stream] = cfg->use_dma;

    fmt = 0;
    switch (cfg->bits) {
        case AUD_BITS_16:
            fmt = 0;
            break;
        // here, 32-bit is treated as 24-bit
        case AUD_BITS_32:
        case AUD_BITS_24:
            fmt = 8;
            break;
        default:
            ASSERT(0, "%s: invalid bits[%d]", __func__, cfg->bits);
    }

    if (stream == AUD_STREAM_PLAYBACK) {
        spdifip_w_tx_ratio(reg_base, spdif_sample_rate[i].tx_ratio - 1);
        spdifip_w_tx_format_cfg_reg(reg_base, fmt);
        spdifip_w_tx_fifo_threshold(reg_base, HAL_SPDIF_TX_FIFO_TRIGGER_LEVEL);
    } else {
        spdifip_w_rx_format_cfg_reg(reg_base, fmt);
        spdifip_w_rx_fifo_threshold(reg_base, HAL_SPDIF_RX_FIFO_TRIGGER_LEVEL);
    }

    return 0;
}

int hal_spdif_send(enum HAL_SPDIF_ID_T id, uint8_t *value, uint32_t value_len)
{
    uint32_t i = 0;
    uint32_t reg_base;

    reg_base = _spdif_get_reg_base(id);

    for (i = 0; i < value_len; i += 4) {
        while (!(spdifip_r_int_status(reg_base) & SPDIFIP_INT_STATUS_TX_FIFO_EMPTY_MASK));

        spdifip_w_tx_left_fifo(reg_base, value[i+1]<<8 | value[i]);
        spdifip_w_tx_right_fifo(reg_base, value[i+3]<<8 | value[i+2]);
    }

    return 0;
}

uint8_t hal_spdif_recv(enum HAL_SPDIF_ID_T id, uint8_t *value, uint32_t value_len)
{
    //uint32_t reg_base;

    //reg_base = _spdif_get_reg_base(id);
    return 0;
}

int hal_spdif_clock_out_enable(enum HAL_SPDIF_ID_T id, uint32_t div)
{
    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    hal_cmu_spdif_clock_enable(id);
    hal_cmu_spdif_set_div(id, div);

    return 0;
}

int hal_spdif_clock_out_disable(enum HAL_SPDIF_ID_T id)
{
    if (id >= HAL_SPDIF_ID_QTY) {
        return 1;
    }

    hal_cmu_spdif_set_div(id, 0x1FFF + 2);
    hal_cmu_spdif_clock_disable(id);

    return 0;
}

#endif
