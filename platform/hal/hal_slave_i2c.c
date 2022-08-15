/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "plat_addr_map.h"

#ifdef I2C_SLAVE_BASE

#include "hal_slave_i2c.h"
#include "reg_slave_i2c.h"

static struct SLAVE_I2C_T * const slave_i2c = (struct SLAVE_I2C_T *)I2C_SLAVE_BASE;

void hal_slave_i2c_enable(void)
{
    slave_i2c->EN |= I2C_EN;
}

void hal_slave_i2c_disable(void)
{
    slave_i2c->EN &= ~I2C_EN;
}

uint32_t hal_slave_i2c_get_filter_len(void)
{
    return GET_BITFIELD(slave_i2c->EN, R_FILTERLEN);
}

int hal_slave_i2c_set_filter_len(uint32_t len)
{
    if (len > (W_FILTERLEN_MASK >> W_FILTERLEN_SHIFT)) {
        return 1;
    }
    slave_i2c->EN = SET_BITFIELD(slave_i2c->EN, W_FILTERLEN, len);
    return 0;
}

uint32_t hal_slave_i2c_get_dev_id(void)
{
    return GET_BITFIELD(slave_i2c->ID, DEV_ID);
}

int hal_slave_i2c_set_dev_id(uint32_t dev_id)
{
    if (dev_id > 0x7F) {
        return 1;
    }
    slave_i2c->ID = SET_BITFIELD(slave_i2c->ID, DEV_ID, dev_id);
    return 0;
}

void hal_slave_i2c_bypass_timeout(void)
{
    slave_i2c->TBP |= TIMEOUT_BYPASS;
}

void hal_slave_i2c_restore_timeout(void)
{
    slave_i2c->TBP &= ~TIMEOUT_BYPASS;
}

#endif
