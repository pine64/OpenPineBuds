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
#ifndef __APP_IBRT_CUSTOM_CMD__
#define __APP_IBRT_CUSTOM_CMD__

#define APP_IBRT_CMD_BASE           0x8000
#define APP_IBRT_CMD_MASK           0x0F00
#define APP_IBRT_CUSTOM_CMD_PREFIX  0x0100
#define APP_IBRT_OTA_CMD_PREFIX     0x0200
#define APP_IBRT_OTA_TWS_CMD_PREFIX 0x0300

int app_ibrt_customif_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);
int app_ibrt_ota_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);
int app_ibrt_ota_tws_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size);

#endif
