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
#ifndef __BT_DRV_2300P_INTERNAL_H__
#define __BT_DRV_2300P_INTERNAL_H__

/***************************************************************************
 *RF conig macro
 ****************************************************************************/
//#define __HW_AGC__
#define __HYBIRD_AGC__ //if define __HYBIRD_AGC__ , not define __HW_AGC__
#define __FANG_HW_AGC_CFG__
#define __FANG_LNA_CFG__
#define __PWR_FLATNESS__

#ifdef __HYBIRD_AGC__
#define __HW_AGC__
#endif

#ifdef __HW_AGC__
#define __HW_AGC_I2V_DISABLE_DC_CAL__
#endif

#define  __1302_8_AGC__

#define BT_RF_OLD_CORR_MODE
/***************************************************************************
 *BT clock gate disable
 ****************************************************************************/
#define __CLK_GATE_DISABLE__

/***************************************************************************
 *BT read controller rssi
 ****************************************************************************/
#define  BT_RSSI_MONITOR  1

/***************************************************************************
 *PCM config macro
 ****************************************************************************/
//#define APB_PCM
//#define SW_INSERT_MSBC_TX_OFFSET

/***************************************************************************
 *BT afh follow function
 ****************************************************************************/
#define  BT_AFH_FOLLOW  0

#ifdef __cplusplus
extern "C" {
#endif

void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain);
#ifdef LAURENT_ALGORITHM
void btdrv_ble_test_bridge_intsys_callback(const unsigned char *data);
void btdrv_bt_laurent_algorithm_enable(void);
void btdrv_ble_laurent_algorithm_enable(void);
#endif
void bt_drv_reg_op_for_test_mode_disable(void);
void bt_drv_reg_op_disable_swagc_nosync_count(void);

void btdrv_txpower_calib(void);

#ifdef __cplusplus
}
#endif

#endif
