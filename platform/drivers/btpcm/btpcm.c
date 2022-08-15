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
#include "cmsis_os.h"
#include "hal_trace.h"
//#include "hwtest.h"
//#include "hal_intersys.h"
//#include "hal_spi.h"
//#include "hal_cmu.h"
//#include "hal_sleep.h"
#include "hal_dma.h"
#include "stdbool.h"
#include "string.h"
#include "hal_btpcm.h"

#if 0
#define AUDIO_BUFFER_SIZE 2048

struct HAL_BTPCM_CONFIG_T btpcm_cfg;
struct HAL_DMA_CH_CFG_T dma_cfg;
struct HAL_DMA_DESC_T play_desc;
struct HAL_DMA_DESC_T record_desc;
uint32_t play_buffer[AUDIO_BUFFER_SIZE / 4];
uint32_t record_buffer[AUDIO_BUFFER_SIZE / 4];

//pcm:rx-->codec:spk
static void btpcm2codec(void)
{
    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.ch = hal_audma_get_chan(HAL_DMA_HIGH_PRIO);
    dma_cfg.src_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg.handler = NULL;
    dma_cfg.dst_periph = HAL_AUDMA_BTPCM_TX;
    dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;

    dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.src_tsize = AUDIO_BUFFER_SIZE/2; /* cause of half word width */
    dma_cfg.try_burst = 1;

    dma_cfg.src = (uint32_t)play_buffer;
    hal_audma_init_desc(&play_desc, &dma_cfg, &play_desc, 0);
    hal_audma_sg_start(&play_desc, &dma_cfg);

    dma_cfg.ch = hal_audma_get_chan(HAL_DMA_HIGH_PRIO);
    dma_cfg.dst_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg.handler = NULL;
    dma_cfg.src_periph = HAL_AUDMA_BTPCM_RX;
    dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;

    dma_cfg.dst = (uint32_t)record_buffer;
    hal_audma_init_desc(&record_desc, &dma_cfg, &record_desc, 0);
    hal_audma_sg_start(&record_desc, &dma_cfg);

    hal_btpcm_open(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK);
    hal_btpcm_open(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE);
    memset(&btpcm_cfg, 0, sizeof(btpcm_cfg));
    btpcm_cfg.use_dma = 1;
    hal_btpcm_setup_stream(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK, &btpcm_cfg);
    hal_btpcm_setup_stream(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE, &btpcm_cfg);
    hal_btpcm_start_stream(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK);
    hal_btpcm_start_stream(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE);
}

//codec:mic-->pcm:tx
static void codec2btpcm(void)
{
    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.ch = hal_audma_get_chan(HAL_DMA_HIGH_PRIO);
    dma_cfg.src_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg.handler = NULL;
    dma_cfg.dst_periph = HAL_AUDMA_BTPCM_TX;
    dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;

    dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
    dma_cfg.dst_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.src_width = HAL_DMA_WIDTH_HALFWORD;
    dma_cfg.src_tsize = AUDIO_BUFFER_SIZE/2; /* cause of half word width */
    dma_cfg.try_burst = 1;

    dma_cfg.src = (uint32_t)play_buffer;
    hal_audma_init_desc(&play_desc, &dma_cfg, &play_desc, 0);
    hal_audma_sg_start(&play_desc, &dma_cfg);

    dma_cfg.ch = hal_audma_get_chan(HAL_DMA_HIGH_PRIO);
    dma_cfg.dst_periph = (enum HAL_DMA_PERIPH_T)0;
    dma_cfg.handler = NULL;
    dma_cfg.src_periph = HAL_AUDMA_BTPCM_RX;
    dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;

    dma_cfg.dst = (uint32_t)record_buffer;
    hal_audma_init_desc(&record_desc, &dma_cfg, &record_desc, 0);
    hal_audma_sg_start(&record_desc, &dma_cfg);

    hal_btpcm_open(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK);
    hal_btpcm_open(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE);
    memset(&btpcm_cfg, 0, sizeof(btpcm_cfg));
    btpcm_cfg.use_dma = 1;
    hal_btpcm_setup_stream(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK, &btpcm_cfg);
    hal_btpcm_setup_stream(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE, &btpcm_cfg);
    hal_btpcm_start_stream(HAL_BTPCM_ID_0, AUD_STREAM_PLAYBACK);
    hal_btpcm_start_stream(HAL_BTPCM_ID_0, AUD_STREAM_CAPTURE);
}
#endif
