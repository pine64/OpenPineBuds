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
#ifdef NEW_NV_RECORD_ENABLED
#ifdef NVREC_BAIDU_DATA_SECTION

#ifndef __NVRECORD_DMA_CONFIG_H__
#define __NVRECORD_DMA_CONFIG_H__

#include "nvrecord_extension.h"

#define BAIDU_DATA_DEF_RAND "abcdefghi"

#ifdef __cplusplus
extern "C" {
#endif

int nvrec_baidu_data_init(void);
int nvrec_get_fm_freq(void);
int nvrec_set_fm_freq(int fmfreq);
int nvrec_get_rand(char *rand);
int nvrec_set_rand(char *rand);
void nvrecord_rebuild_dma_configuration(NV_DMA_CONFIGURATION_T* pDmaConfig);
int nvrec_dev_get_sn(char *sn);

#ifdef __cplusplus
}
#endif
#endif
#endif  // #ifdef NVREC_BAIDU_DATA_SECTION
#endif  // #ifdef NEW_NV_RECORD_ENABLED

