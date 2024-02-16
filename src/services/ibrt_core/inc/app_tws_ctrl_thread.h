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
#ifndef __APP_TWS_CTRL_THREAD__
#define __APP_TWS_CTRL_THREAD__

#include "app_tws_ibrt.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_ibrt_custom_cmd.h"

#if (defined IBRT_OTA || defined __GMA_OTA_TWS__ || defined OTA_ENABLED) || defined __DUAL_MIC_RECORDING__
#define APP_TWS_CTRL_BUFFER_MAX_LEN 672
#else
#if (defined TILE_DATAPATH && defined GFPS_ENABLED)
#define APP_TWS_CTRL_BUFFER_MAX_LEN 512
#else
#define APP_TWS_CTRL_BUFFER_MAX_LEN 300  //no more buf for 1400
#endif
#endif
typedef void (*app_tws_cmd_send_function_t)(uint8_t*, uint16_t);

typedef struct
{
    uint32_t                        cmdcode;
    app_tws_cmd_send_function_t     tws_cmd_send;
} __attribute__((packed)) app_tws_cmd_send_t;

typedef enum
{
    PARA_NONE,
    PARA_CODEC_TYPE,
    PARA_PLAY_BACK,
    PARA_BUFFER,
    PARA_SYNC_TRIGGER,
} mail_para_type_e;

typedef struct
{
    uint8_t *cmd_buffer;
    uint16_t cmd_len;
} cmd_buffer_t;

typedef struct
{
    uint32_t evt;
    uint32_t arg;
    uint32_t arg1;
    uint32_t system_time;
    cmd_buffer_t cmd_buff;
} TWS_MSG_BLOCK;

#ifdef __cplusplus
extern "C" {
#endif
void tws_ctrl_thread_init(void);
void *tws_ctrl_mailbox_heap_malloc(uint32_t size);
void tws_ctrl_mailbox_heap_free(void *rmem);
int tws_ctrl_send_cmd(uint32_t cmd_code, uint8_t *p_buff, uint16_t length);
int tws_ctrl_send_rsp(uint16_t rsp_code, uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
int tws_ctrl_send_cmd_high_priority(uint32_t cmd_code, uint8_t *p_buff, uint16_t length);
#ifdef __cplusplus
}
#endif
#endif
