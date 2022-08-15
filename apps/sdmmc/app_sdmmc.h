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
#ifndef __APP_SDMMC_H__
#define __APP_SDMMC_H__

#ifdef __cplusplus
extern "C" {
#endif

enum APP_SDMMC_DUMP_T{
    APP_SDMMC_DUMP_OPEN = 0,
    APP_SDMMC_DUMP_READ,
    APP_SDMMC_DUMP_WRITE,
    APP_SDMMC_DUMP_CLOSE,
    APP_SDMMC_DUMP_NUM
};

int sd_open();

void  dump_data2sd(enum APP_SDMMC_DUMP_T opt, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
	}
#endif

#endif//__FMDEC_H__
