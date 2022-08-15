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
/***
 * besbt.h
 */

#ifndef BESBT_H
#define BESBT_H

enum BESBT_HOOK_USER_T {
    BESBT_HOOK_USER_0 = 0,
    BESBT_HOOK_USER_1,
    BESBT_HOOK_USER_2,    
    BESBT_HOOK_USER_3,
    BESBT_HOOK_USER_QTY
};

typedef void (*BESBT_HOOK_HANDLER)(void);

#ifdef __cplusplus
extern "C" {
#endif

void BesbtInit(void);
void BesbtThread(void const *argument);
int Besbt_hook_handler_set(enum BESBT_HOOK_USER_T user, BESBT_HOOK_HANDLER handler);
unsigned char *bt_get_local_address(void);
void bt_set_local_address(unsigned char* btaddr);
unsigned char *bt_get_ble_local_address(void);
const char *bt_get_local_name(void);
void bt_set_local_name(const char* name);
const char *bt_get_ble_local_name(void);
void gen_bt_addr_for_debug(void);
void bt_set_ble_local_address(uint8_t* bleAddr);

#ifdef __cplusplus
}
#endif
#endif /* BESBT_H */
