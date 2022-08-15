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

#ifndef __TOTA_STREAM_DATA_TRANSFER_H__
#define __TOTA_STREAM_DATA_TRANSFER_H__

#include <stdint.h>


#define MAX_SPP_PACKET_SIZE     666
#define STREAM_HEADER_SIZE      2


void app_tota_stream_data_transfer_init();

/* interface for streaming */
void app_tota_stream_data_start(uint16_t set_module = 0);
void app_tota_stream_data_end();
bool app_tota_send_stream_data(uint8_t * pdata, uint32_t dataLen);
void app_tota_stream_data_flush();
void app_tota_stream_data_clean();

/* interface for send data */
bool app_tota_send_data_via_spp(uint8_t* ptrData, uint32_t length);

/* interface for app_tota */
bool is_stream_data_running();
void app_tota_tx_done_callback();

#endif
