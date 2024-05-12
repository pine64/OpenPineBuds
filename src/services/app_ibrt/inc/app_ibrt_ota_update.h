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
#ifndef __APP_IBRT_OTA_UPDATE_CMD__
#define __APP_IBRT_OTA_UPDATE_CMD__
#define APP_TWS_CTRL_BUFFER_LEN 512

#include "app_ibrt_custom_cmd.h"

typedef enum
{
    APP_TWS_CMD_OTA_UPDATE_NOW = APP_IBRT_OTA_CMD_PREFIX | 0x01,
    APP_TWS_CMD_UPDATE_SECTION = APP_IBRT_OTA_CMD_PREFIX | 0x02,
    APP_TWS_CMD_CHECK_UPDATE_INFO = APP_IBRT_OTA_CMD_PREFIX | 0x03,
    APP_TWS_CMD_CHECK_UPDATE_INFO2 = APP_IBRT_OTA_CMD_PREFIX | 0x04,
    APP_TWS_CMD_SYNC_BREAKPIONT = APP_IBRT_OTA_CMD_PREFIX | 0x05,
    APP_TWS_CMD_VALIDATION_DONE = APP_IBRT_OTA_CMD_PREFIX | 0x06,
    //new customer cmd add here
} app_ibrt_ota_cmd_code_e;

typedef struct
{
    uint8_t* address;
    uint32_t length;
} __attribute__((packed))ibrt_ota_check_cmd_t;

typedef struct
{
    uint32_t local_point;
    uint32_t remote_point;
} __attribute__((packed))ibrt_ota_sync_breakpoint_cmd_t;

typedef struct
{
    uint8_t flag;
} __attribute__((packed))ibrt_ota_sync_validation_cmd_t;

typedef struct
{
    uint32_t  address;
    uint32_t length;
    uint8_t number;
    uint32_t point;
    uint32_t crc;
    uint8_t buff[APP_TWS_CTRL_BUFFER_LEN];
} __attribute__((packed))ibrt_ota_update_cmd_t;

#ifdef __cplusplus
extern "C" {
#endif

void reset_error_list();
uint32_t get_error_list();
void set_all_error_list();
void set_error_list(uint16_t list);
void app_ibrt_view_update_sector();
void app_ibrt_view_update_list();
void app_ibrt_ota_check_update_list();
void app_ibrt_ota_update_immediately();
void app_ibrt_ota_force_update();
void app_ibrt_ota_check_update_info();

#ifdef __cplusplus
}
#endif

#endif
