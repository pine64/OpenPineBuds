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
#ifndef __HAL_SLAVE_I2C_H__
#define __HAL_SLAVE_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"

void hal_slave_i2c_enable(void);

void hal_slave_i2c_disable(void);

uint32_t hal_slave_i2c_get_filter_len(void);

int hal_slave_i2c_set_filter_len(uint32_t len);

uint32_t hal_slave_i2c_get_dev_id(void);

int hal_slave_i2c_set_dev_id(uint32_t dev_id);

void hal_slave_i2c_bypass_timeout(void);

void hal_slave_i2c_restore_timeout(void);

#ifdef __cplusplus
}
#endif

#endif
