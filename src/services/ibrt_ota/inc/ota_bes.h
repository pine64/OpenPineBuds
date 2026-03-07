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
#ifndef __APP_OTA_H__
#define __APP_OTA_H__
	
#define APP_OTA_CONNECTED             (1 << 0)    
#define APP_OTA_DISCONNECTED          (~(1 << 0))

#ifdef __cplusplus
extern "C" {
#endif

void app_ota_connected(uint8_t connType);
void app_ota_disconnected(uint8_t disconnType);
bool app_is_in_ota_mode(void);
void bes_ota_init(void);

#define BES_OTA_UUID_128 {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 }

#if (BLE_OTA)
#define ota_val_char_val_uuid_128_content  		{0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 }	
#define ATT_DECL_PRIMARY_SERVICE_UUID		{ 0x00, 0x28 }
#define ATT_DECL_CHARACTERISTIC_UUID		{ 0x03, 0x28 }
#define ATT_DESC_CLIENT_CHAR_CFG_UUID		{ 0x02, 0x29 }
#endif
#ifdef __cplusplus
}
#endif

#endif

