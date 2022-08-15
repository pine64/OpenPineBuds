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

#ifndef __APP_TOTA_ANC_H__
#define __APP_TOTA_ANC_H__


/* from other files */
extern unsigned char *g_buf;
extern size_t g_len;


extern "C" void bulk_read_done(void);
extern "C" void reset_programmer_state(unsigned char **buf, size_t *len);
extern "C" void send_sync_cmd_to_tool();
extern "C" int anc_handle_received_data(uint8_t* ptrParam, uint32_t paramLen);
extern "C" int get_send_sync_flag(void);



/* interface */
void app_tota_anc_init();

#endif

