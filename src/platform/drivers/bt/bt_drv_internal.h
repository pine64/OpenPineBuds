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
#ifndef __BT_DRV_INTERNAL_H__
#define __BT_DRV_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

#define ASSERT_ERR(cond)                             { if (!(cond)) { BT_DRV_TRACE(2,"line is %d file is %s", __LINE__, __FILE__); } }

#define BT_DRIVER_GET_U8_REG_VAL(regAddr)       (*(uint8_t *)(uintptr_t)(regAddr))
#define BT_DRIVER_GET_U16_REG_VAL(regAddr)      (*(uint16_t *)(uintptr_t)(regAddr))
#define BT_DRIVER_GET_U32_REG_VAL(regAddr)      (*(uint32_t *)(uintptr_t)(regAddr))

#define BT_DRIVER_PUT_U8_REG_VAL(regAddr, val)      *(uint8_t *)(uintptr_t)(regAddr) = (val)
#define BT_DRIVER_PUT_U16_REG_VAL(regAddr, val)     *(uint16_t *)(uintptr_t)(regAddr) = (val)
#define BT_DRIVER_PUT_U32_REG_VAL(regAddr, val)     *(uint32_t *)(uintptr_t)(regAddr) = (val)

typedef uint32_t BT_CONTROLER_TRACE_TYPE;
#define BT_CONTROLER_TRACE_TYPE_INTERSYS                0x01
#define BT_CONTROLER_TRACE_TYPE_CONTROLLER              0x02
#define BT_CONTROLER_TRACE_TYPE_LMP_TRACE               0x04
#define BT_CONTROLER_TRACE_TYPE_SPUV_HCI_BUFF           0x08
#define BT_CONTROLER_FILTER_TRACE_TYPE_A2DP_STREAM      0x10
#define BT_CONTROLER_TRACE_TYPE_DUMP_BUFF               0x20

#define BT_SUB_SYS_TYPE     0
#define MCU_SYS_TYPE        1
#define BT_EM_AREA_1_TYPE   2
#define BT_EM_AREA_2_TYPE   3

#define BT_FA_INVERT_EN   1
#define BT_FA_INVERT_DISABLE   0

void bt_drv_set_fa_invert_enable(uint8_t en);
uint8_t btdrv_rf_init(void);
void btdrv_test_mode_rf_txpwr_init(void);
void btdrv_ins_patch_init(void);
void btdrv_data_patch_init(void);
void btdrv_patch_en(uint8_t en);
void btdrv_config_init(void);
void btdrv_config_end(void);
void btdrv_testmode_config_init(void);
void btdrv_bt_spi_rawbuf_init(void);
void btdrv_bt_spi_xtal_init(void);
void btdrv_sync_config(void);
void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain);
#ifdef BT_50_FUNCTION
void btdrv_config_init_ble5(void);
void btdrv_ins_patch_init_50(void);
void btdrv_data_patch_init_50(void);
#endif
void btdrv_trace_config(BT_CONTROLER_TRACE_TYPE trace_config);
void btdrv_btc_fault_dump(void);
void btdrv_dump_mem(uint8_t *dump_mem_start, uint32_t dump_length, uint8_t dump_type);
void ld_util_bch_create(uint8_t *lap, uint8_t *bch);
void btdrv_fast_lock_config(bool fastlock_on);
void btdrv_ecc_config(void);

enum BT_SYNCMODE_REQ_USER_T {
   BT_SYNCMODE_REQ_USER_BT,
   BT_SYNCMODE_REQ_USER_BLE,

   BT_SYNCMODE_REQ_USER_QTY
};
void btdrv_hw_agc_stop_mode(enum BT_SYNCMODE_REQ_USER_T user, bool hw_agc_mode);


inline uint16_t co_read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}

inline uint32_t co_read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = co_read16p((uint16_t *)ptr32);
    addr_h = co_read16p((uint16_t *)ptr32 + 1);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

#ifdef __cplusplus
}
#endif

#endif
