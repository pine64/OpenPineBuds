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
#ifndef __APP_IBRT_OTA_CMD__
#define __APP_IBRT_OTA_CMD__

#include "app_ibrt_custom_cmd.h"

#define RESEND_TIME 2

#define app_ibrt_ota_image_buff_cmd_rsp_timeout_handler_null   (0)
#define app_ibrt_ota_image_buff_cmd_rsp_handler_null           (0)

typedef enum
{
#ifdef IBRT_OTA
    IBRT_OTA_TWS_GET_VERSION_CMD     = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x01,
    IBRT_OTA_TWS_SELECT_SIDE_CMD     = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x02,
    IBRT_OTA_TWS_BP_CHECK_CMD        = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x03,
    IBRT_OTA_TWS_START_OTA_CMD       = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x04,
    IBRT_OTA_TWS_OTA_CONFIG_CMD      = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x05,
    IBRT_OTA_TWS_SEGMENT_CRC_CMD     = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x06,
    IBRT_OTA_TWS_IMAGE_CRC_CMD       = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x07,
    IBRT_OTA_TWS_IMAGE_OVERWRITE_CMD = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x08,
    IBRT_OTA_TWS_ROLE_SWITCH_CMD     = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x0C,
    IBRT_OTA_TWS_ACK_CMD             = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x0D,
#endif
    IBRT_OTA_TWS_IMAGE_BUFF          = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x09,
    IBRT_COMMON_OTA                   = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x0B,
//    IBRT_OTA_TWS_IMAGE_BUFF_SLAVE    = APP_IBRT_CMD_BASE|APP_IBRT_OTA_TWS_CMD_PREFIX|0x0A,
} app_ibrt_ota_tws_cmd_code_e;

#ifdef IBRT_OTA
extern uint32_t ibrt_ota_cmd_type;
extern uint32_t twsBreakPoint;
//extern uint8_t ibrt_connect_slave;
extern uint8_t errOtaCode;
#endif

#endif
