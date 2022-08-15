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
#ifndef __APP_CMD_H__
#define __APP_CMD_H__
#ifdef __PC_CMD_UART__

#include "hal_cmd.h"
typedef struct {
    void *param;
} APP_CMD_HANDLE;

void app_cmd_open(void);

void app_cmd_close(void);
#endif
#endif//__FMDEC_H__
