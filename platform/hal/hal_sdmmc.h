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
#ifndef _HAL_SDMMC_H_
#define _HAL_SDMMC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

enum HAL_SDMMC_ID_T {
    HAL_SDMMC_ID_0 = 0,
    HAL_SDMMC_ID_NUM,
};

typedef void (*HAL_SDMMC_DELAY_FUNC)(uint32_t ms);

/* hal api */
HAL_SDMMC_DELAY_FUNC hal_sdmmc_set_delay_func(HAL_SDMMC_DELAY_FUNC new_func);
int32_t hal_sdmmc_open(enum HAL_SDMMC_ID_T id);
void hal_sdmmc_info(enum HAL_SDMMC_ID_T id, uint32_t *sector_count, uint32_t *sector_size);
uint32_t hal_sdmmc_read_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t* dest);
uint32_t hal_sdmmc_write_blocks(enum HAL_SDMMC_ID_T id, uint32_t start_block, uint32_t block_count, uint8_t* src);
void hal_sdmmc_close(enum HAL_SDMMC_ID_T id);
void hal_sdmmc_dump(enum HAL_SDMMC_ID_T id);
int hal_sdmmc_enable_detecter(enum HAL_SDMMC_ID_T id,void (* cb)(uint8_t));

#ifdef __cplusplus
}
#endif

#endif /* _HAL_SDMMC_H_ */
