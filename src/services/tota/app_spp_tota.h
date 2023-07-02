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
#ifndef __APP_SPP_TOTA_H__
#define __APP_SPP_TOTA_H__
#include <map>
#include <stdint.h>
using namespace std;

#define TOTA_SPP_ID                  0
#define TOTA_SPP_MAX_PACKET_SIZE     600
#define TOTA_SPP_MAX_PACKET_NUM      5


#define TOTA_SPP_TX_BUF_SIZE        (TOTA_SPP_MAX_PACKET_SIZE*TOTA_SPP_MAX_PACKET_NUM)

#define APP_TOTA_DATA_CMD_TIME_OUT_IN_MS    5000

/**
 * @brief Type of the tota module
 *
 */
typedef enum
{
    APP_TOTA_AUDIO_DUMP = 0,
    APP_TOTA_MIC,
    APP_TOTA_FLASH,
    APP_TOTA_ANC,
    APP_TOTA_GENERAL,
    APP_TOTA_CUSTOM,

    APP_TOTA_MODULE_NONE = 0xFF
} APP_TOTA_MODULE_E;


typedef struct{
    void (*tota_spp_connected)(void);
    void (*tota_spp_disconnected)(void);
    void (*tota_spp_tx_done)(void);
    void (*tota_spp_data_receive_hanle)(uint8_t * buf, uint32_t len);
}tota_callback_func_t;


void tota_callback_module_register(APP_TOTA_MODULE_E module, tota_callback_func_t tota_callback_func);
void tota_callback_module_set(APP_TOTA_MODULE_E module);
APP_TOTA_MODULE_E tota_callback_module_get();



typedef void(*app_spp_tota_tx_done_t)(void);
void app_spp_tota_register_tx_done(app_spp_tota_tx_done_t callback);
void app_spp_tota_init(void);


uint16_t app_spp_tota_tx_buf_size(void);
void app_spp_tota_init_tx_buf(uint8_t* ptr);
uint8_t* app_spp_tota_fill_data_into_tx_buf(uint8_t* ptrData, uint32_t dataLen);

bool app_spp_tota_send_data(uint8_t* ptrData, uint16_t length);

/* for sniff */
bool spp_tota_in_progress(void);

#endif

