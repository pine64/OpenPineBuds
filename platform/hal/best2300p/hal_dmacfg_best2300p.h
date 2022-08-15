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
#ifndef __HAL_DMACFG_BEST2300P_H__
#define __HAL_DMACFG_BEST2300P_H__

#define AUDMA_PERIPH_NUM                        16
#define GPDMA_PERIPH_NUM                        16

#define AUDMA_CHAN_NUM                          8
#define GPDMA_CHAN_NUM                          8

#define AUDMA_CHAN_START                        (0)
#define GPDMA_CHAN_START                        (0)

static const uint32_t audma_fifo_addr[AUDMA_PERIPH_NUM] = {
    CODEC_BASE + 0x01C,     // CODEC RX
    CODEC_BASE + 0x01C,     // CODEC TX
#ifdef CODEC_DSD
    CODEC_BASE + 0x034,     // DSD RX
    CODEC_BASE + 0x034,     // DSD TX
#else
    BTPCM_BASE + 0x1C0,     // BTPCM RX
    BTPCM_BASE + 0x1C8,     // BTPCM TX
#endif
    I2S0_BASE + 0x200,      // I2S0 RX
    I2S0_BASE + 0x240,      // I2S0 TX
    0,                      // FIR RX
    0,                      // FIR TX
    SPDIF0_BASE + 0x1C0,    // SPDIF0 RX
    SPDIF0_BASE + 0x1C8,    // SPDIF0 TX
    CODEC_BASE + 0x03C,     // IIR RX
    CODEC_BASE + 0x03C,     // IIR TX
    BTDUMP_BASE + 0x34,     // BTDUMP
    CODEC_BASE + 0x038,     // MC RX
    I2S1_BASE + 0x200,      // I2S1 RX
    I2S1_BASE + 0x240,      // I2S1 TX
};

static const enum HAL_DMA_PERIPH_T audma_fifo_periph[AUDMA_PERIPH_NUM] = {
    HAL_AUDMA_CODEC_RX,
    HAL_AUDMA_CODEC_TX,
#ifdef CODEC_DSD
    HAL_AUDMA_DSD_RX,
    HAL_AUDMA_DSD_TX,
#else
    HAL_AUDMA_BTPCM_RX,
    HAL_AUDMA_BTPCM_TX,
#endif
    HAL_AUDMA_I2S0_RX,
    HAL_AUDMA_I2S0_TX,
    HAL_AUDMA_FIR_RX,
    HAL_AUDMA_FIR_TX,
    HAL_AUDMA_SPDIF0_RX,
    HAL_AUDMA_SPDIF0_TX,
    HAL_AUDMA_IIR_RX,
    HAL_AUDMA_IIR_TX,
    HAL_AUDMA_BTDUMP,
    HAL_AUDMA_MC_RX,
    HAL_AUDMA_I2S1_RX,
    HAL_AUDMA_I2S1_TX,
};

static const uint32_t gpdma_fifo_addr[GPDMA_PERIPH_NUM] = {
    FLASH_CTRL_BASE + 0x008, // FLASH CTRL
    SDMMC_BASE + 0x200,     // SDMMC
    I2C0_BASE + 0x010,      // I2C0 RX
    I2C0_BASE + 0x010,      // I2C0 TX
    SPI_BASE + 0x008,       // SPI RX
    SPI_BASE + 0x008,       // SPI TX
    SPILCD_BASE + 0x008,    // SPILCD RX
    SPILCD_BASE + 0x008,    // SPILCD TX
    UART0_BASE + 0x000,     // UART0 RX
    UART0_BASE + 0x000,     // UART0 TX
    UART1_BASE + 0x000,     // UART1 RX
    UART1_BASE + 0x000,     // UART1 TX
    I2C1_BASE + 0x010,      // I2C1 RX
    I2C1_BASE + 0x010,      // I2C1 TX
    UART2_BASE + 0x000,     // UART2 RX
    UART2_BASE + 0x000,     // UART2 TX
};

static const enum HAL_DMA_PERIPH_T gpdma_fifo_periph[GPDMA_PERIPH_NUM] = {
    HAL_GPDMA_FLASH_TX,
    HAL_GPDMA_SDMMC,
    HAL_GPDMA_I2C0_RX,
    HAL_GPDMA_I2C0_TX,
    HAL_GPDMA_SPI_RX,
    HAL_GPDMA_SPI_TX,
    HAL_GPDMA_SPILCD_RX,
    HAL_GPDMA_SPILCD_TX,
    HAL_GPDMA_UART0_RX,
    HAL_GPDMA_UART0_TX,
    HAL_GPDMA_UART1_RX,
    HAL_GPDMA_UART1_TX,
    HAL_GPDMA_I2C1_RX,
    HAL_GPDMA_I2C1_TX,
    HAL_GPDMA_UART2_RX,
    HAL_GPDMA_UART2_TX,
};

#endif

