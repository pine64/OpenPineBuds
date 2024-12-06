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
#ifndef __ANC_PARSE_DATA_H__
#define __ANC_PARSE_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "tool_msg.h"
#include "crc32.h"
// #include "app_tws.h"

#define COMMAND_PARSER_VERSION          0x0100

#define ANC_PARSE_DATA_BUFF_SIZE ((8 * 1024 + BURN_DATA_MSG_OVERHEAD + 63) / 64 * 64)
#define ANC_PARSE_DATA_BUFF_OFFSET (PCM_BUFFER_SZ_TWS+TRANS_BUFFER_SZ_TWS+A2DP_BUFFER_SZ_TWS - ANC_PARSE_DATA_BUFF_SIZE)

#define ANC_SHARE_BUF                   0

#define BURN_BUFFER_LOC

enum PROGRAMMER_STATE{
    PROGRAMMER_NONE,
    PROGRAMMER_ERASE_BURN_START,
    PROGRAMMER_BULK_WRITE_START,
};

enum DATA_BUF_STATE {
    DATA_BUF_FREE,
    DATA_BUF_RECV,
    DATA_BUF_BURN,
    DATA_BUF_DONE,
};

enum FLASH_CMD_TYPE {
    FLASH_CMD_ERASE_SECTOR = 0x21,
    FLASH_CMD_BURN_DATA = 0x22,
    FLASH_CMD_ERASE_CHIP = 0x31,
};

enum CUST_CMD_STAGE {
    CUST_CMD_STAGE_HEADER,
    CUST_CMD_STAGE_DATA,
    CUST_CMD_STAGE_EXTRA,

    CUST_CMD_STAGE_QTY
};

struct CUST_CMD_PARAM {
    enum CUST_CMD_STAGE stage;
    const struct message_t *msg;
    int extra;
    unsigned char *buf;
    size_t expect;
    size_t size;
    unsigned int timeout;
};

typedef enum ERR_CODE (*CUST_CMD_HANDLER_T)(struct CUST_CMD_PARAM *param);

#define CUST_CMD_HDLR_TBL_LOC           __attribute__((section(".cust_cmd_hldr_tbl"), used))
#define TIMEOUT_INFINITE                ((uint32_t)-1)

extern struct message_t recv_msg;

/**
 * @brief
 * Data transmission will be slightly slower in sniff mode. PC Tool may disconnet SPP connection
 * because of command response timed out. We should always in active mode to work around this case. 
 */
extern void anc_data_buff_init(void);
extern void anc_data_buff_deinit(void);


#ifdef __cplusplus
}
#endif

#endif

