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
#ifndef __APP_SPP_TOTA_GEN_H__
#define __APP_SPP_TOTA_GEN_H__

void app_spp_tota_gen_init(void);

uint16_t app_spp_tota_gen_tx_buf_size(void);
void app_spp_tota_gen_init_tx_buf(uint8_t* ptr);
void app_tota_gen_send_cmd_via_spp(uint8_t* ptrData, uint32_t length);
void app_tota_gen_send_data_via_spp(uint8_t* ptrData, uint32_t length);

#endif

