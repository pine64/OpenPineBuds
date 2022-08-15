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
#ifndef __APP_TOTA_DATA_HANDLER_H__
#define __APP_TOTA_DATA_HANDLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "app_tota_cmd_code.h"

typedef struct
{
    uint8_t        isHasCrcCheck     :  1;
    uint8_t        reserved          :  7;
    uint8_t        reservedBytes[7];	
} APP_TOTA_START_DATA_XFER_T;

typedef struct
{
    uint8_t     isHasWholeCrcCheck  :   1;
    uint8_t     reserved            :   7;
    uint8_t     reservedBytes1[3];	
    uint32_t    wholeDataLenToCheck;
    uint32_t    crc32OfWholeData;
} APP_TOTA_STOP_DATA_XFER_T;

typedef struct
{
    uint32_t    segmentDataLen;
    uint32_t    crc32OfSegment;
    uint8_t     reserved[4];
} APP_TOTA_VERIFY_DATA_SEGMENT_T;

typedef struct
{
    uint32_t    reserved;
} APP_TOTA_START_DATA_XFER_RSP_T;

typedef struct
{
    uint32_t    reserved;
} APP_TOTA_STOP_DATA_XFER_RSP_T;

typedef struct
{
    uint32_t    reserved;
} APP_TOTA_VERIFY_DATA_SEGMENT_RSP_T;

typedef struct
{
    uint32_t    dataLenReceivedByPeerDev;
} APP_TOTA_DATA_ACK_T;

typedef void(* receive_data_callback)(uint8_t* ptrData, uint32_t dataLength);

//void app_tota_data_reset_env(void);

void app_tota_data_xfer_started(APP_TOTA_CMD_RET_STATUS_E retStatus);
void app_tota_data_xfer_stopped(APP_TOTA_CMD_RET_STATUS_E retStatus);
void app_tota_data_segment_verified(APP_TOTA_CMD_RET_STATUS_E retStatus);
//void app_tota_data_received_callback(uint8_t* ptrData, uint32_t dataLength);
void app_tota_send_data(APP_TOTA_TRANSMISSION_PATH_E path, uint8_t* ptrData, uint32_t dataLength);
void app_tota_send_data_stream(APP_TOTA_TRANSMISSION_PATH_E path, uint8_t* ptrData, uint32_t dataLength);
void app_tota_start_data_xfer(APP_TOTA_TRANSMISSION_PATH_E path, APP_TOTA_START_DATA_XFER_T* req);
void app_tota_stop_data_xfer(APP_TOTA_TRANSMISSION_PATH_E path, APP_TOTA_STOP_DATA_XFER_T* req);
APP_TOTA_CMD_RET_STATUS_E app_tota_data_received(uint8_t* ptrData, uint32_t dataLength);
bool app_tota_data_is_data_transmission_started(void);
void app_tota_handle_received_data(uint8_t* buffer, uint16_t maxBytes);
void app_tota_kickoff_dataxfer(void);
void app_tota_stop_dataxfer(void);
void app_tota_data_ack_received(uint32_t dataLength);
void app_tota_data_received_callback_handler_register(receive_data_callback cb );

#if defined(APP_ANC_TEST)
void app_anc_tota_send_data(APP_TOTA_TRANSMISSION_PATH_E path, uint8_t* ptrData, uint32_t dataLength);
#endif

#ifdef __cplusplus
	}
#endif

#endif // #ifndef __APP_TOTA_DATA_HANDLER_H__