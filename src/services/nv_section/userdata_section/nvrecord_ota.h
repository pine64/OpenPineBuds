/**
 * @file nvrecord_ota.h
 * @author BES AI team
 * @version 0.1
 * @date 2020-04-21
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


#ifndef __NVRECORD_OTA_H__
#define __NVRECORD_OTA_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "nvrecord_extension.h"

/******************************macro defination*****************************/

/******************************type defination******************************/

/****************************function declearation**************************/
/**
 * @brief Initialize the nvrecord pointer.
 * 
 */
void nv_record_ota_init(void);

/**
 * @brief Get the pointer of OTA info saved in flash.
 * 
 * NOTE: OTA support mutiple devices(currently APP and hotword model file), so
 * you need to use the device index and the pointer from this function you get
 * to get the device specific OTA info.
 * 
 * @param ptr           pointer used to get the flash pointer
 */
void nv_record_ota_get_ptr(void **ptr);

/**
 * @brief Update the break point for OTA progress.
 * 
 * @param user          current user
 * @param deviceIndex   current deivce index
 * @param otaOffset     break point to update
 */
void nv_record_ota_update_breakpoint(uint8_t user,
                                     uint8_t deviceIndex,
                                     uint32_t otaOffset);

/**
 * @brief Update the OTA info.
 * 
 * @param user          current OTA user
 * @param deviceIndex   current device in OTA progress
 * @param status        OTA status(stage) to update,
 *                      @see OTA_STAGE_E to get more details
 * @param imageSize     image size
 * @param session       OTA version string pointer
 * @return true         OTA info update successfully
 * @return false        OTA info update failed
 */
bool nv_record_ota_update_info(uint8_t user,
                               uint8_t deviceIndex,
                               uint8_t status,
                               uint32_t imageSize,
                               const char *session);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __NVRECORD_OTA_H__ */