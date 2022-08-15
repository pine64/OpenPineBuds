/**
 * @file ble_app_dbg.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-05-05
 * 
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */


#ifndef __BLE_APP_DBG_H__
#define __BLE_APP_DBG_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/

/******************************macro defination*****************************/
#define LOG_V(str, ...) LOG_VERBOSE(LOG_MOD(BLEAPP), str, ##__VA_ARGS__)
#define LOG_D(str, ...) LOG_DEBUG(LOG_MOD(BLEAPP), str, ##__VA_ARGS__)
#define LOG_I(str, ...) LOG_INFO(LOG_MOD(BLEAPP), str, ##__VA_ARGS__)
#define LOG_W(str, ...) LOG_WARN(LOG_MOD(BLEAPP), str, ##__VA_ARGS__)
#define LOG_E(str, ...) LOG_ERROR(LOG_MOD(BLEAPP), str, ##__VA_ARGS__)

/******************************type defination******************************/

/****************************function declearation**************************/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLE_APP_DBG_H__ */