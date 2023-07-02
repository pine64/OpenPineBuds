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
#ifndef __APP_TOTA_CMD_HANDLER_H__
#define __APP_TOTA_CMD_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_timer.h"
#include "app_tota_cmd_code.h"

#define GET_CURRENT_TICKS()         hal_sys_timer_get()
#define GET_CURRENT_MS()            TICKS_TO_MS(GET_CURRENT_TICKS())


/**< maximum payload size of one smart voice command */
#define TOTA_MAXIMUM_DATA_PACKET_LEN        660 //672
#define APP_TOTA_CMD_MAXIMUM_PAYLOAD_SIZE   TOTA_MAXIMUM_DATA_PACKET_LEN

/**
 * @brief BLE custom command playload
 *
 */
typedef struct
{
    uint16_t    cmdCode;    /**< command code, from APP_TOTA_CMD_CODE_E */
    uint16_t    paramLen;   /**< length of the following parameter */
    uint8_t     param[APP_TOTA_CMD_MAXIMUM_PAYLOAD_SIZE - 2*sizeof(uint16_t)];
} APP_TOTA_CMD_PAYLOAD_T;

/**
 * @brief Command response parameter structure
 *
 */
typedef struct
{
    uint16_t    cmdCodeToRsp;   /**< tell which command code to response */
    uint16_t    cmdRetStatus;   /**< handling result of the command, from APP_TOTA_CMD_RET_STATUS_E */
    uint16_t    rspDataLen;     /**< length of the response data */
    uint8_t     rspData[APP_TOTA_CMD_MAXIMUM_PAYLOAD_SIZE - 5*sizeof(uint16_t)];
} APP_TOTA_CMD_RSP_T;

void                        app_tota_get_cmd_response_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen);
void                        app_tota_data_xfer_control_handler(APP_TOTA_CMD_CODE_E funcCode, uint8_t* ptrParam, uint32_t paramLen);
void                        app_tota_control_data_xfer(bool isStartXfer);
void                        app_tota_start_data_xfer_control_rsp_handler(APP_TOTA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen);
void                        app_tota_stop_data_xfer_control_rsp_handler(APP_TOTA_CMD_RET_STATUS_E retStatus, uint8_t* ptrParam, uint32_t paramLen);
APP_TOTA_CMD_RET_STATUS_E   app_tota_send_response_to_command(APP_TOTA_CMD_CODE_E responsedCmdCode, APP_TOTA_CMD_RET_STATUS_E returnStatus, 
                                uint8_t* rspData, uint32_t rspDataLen, APP_TOTA_TRANSMISSION_PATH_E path);
APP_TOTA_CMD_RET_STATUS_E   app_tota_send_command(APP_TOTA_CMD_CODE_E cmdCode, uint8_t* ptrParam, uint32_t paramLen, APP_TOTA_TRANSMISSION_PATH_E path);
void                        app_tota_cmd_handler_init(void);
APP_TOTA_CMD_INSTANCE_T*    app_tota_cmd_handler_get_entry_pointer_from_cmd_code(APP_TOTA_CMD_CODE_E cmdCode);
uint16_t                    app_tota_cmd_handler_get_entry_index_from_cmd_code(APP_TOTA_CMD_CODE_E cmdCode);
APP_TOTA_CMD_RET_STATUS_E   app_tota_cmd_received(uint8_t* ptrData, uint32_t dataLength);
#if defined(APP_ANC_TEST)
APP_TOTA_CMD_RET_STATUS_E app_anc_tota_cmd_received(uint8_t* ptrData, uint32_t dataLength);
#endif




#ifdef __cplusplus
	}
#endif

#endif // #ifndef __APP_TOTA_CMD_HANDLER_H__

