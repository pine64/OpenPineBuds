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
#ifndef __APP_TOTA_H__
#define __APP_TOTA_H__

#include "app_tota_cmd_code.h"

#define APP_TOTA_PATH_TYPE_SPP	(1 << 0)

#define APP_TOTA_PATH_TYPE		(APP_TOTA_PATH_TYPE_SPP)
	
#define APP_TOTA_CONNECTED             (1 << 0)    
#define APP_TOTA_DISCONNECTED          (~(1 << 0))

#define TOTA_SHARE_TX_RX_BUF    1  

#ifdef __cplusplus
extern "C" {
#endif
#define TOTA_SERVICE_CMD_PARAM_LENGTH   128
#if 0//if support write flash block cmd,the buffer size must be 4k
#define TOTA_SERVICE_DATA_WRITE_LENGTH  4*1024
#define TOTA_SERVICE_DATA_READ_LENGTH   4*1024
#else
#define TOTA_SERVICE_DATA_WRITE_LENGTH  4*32
#define TOTA_SERVICE_DATA_READ_LENGTH   4*32
#endif
#define FLASH_SECTOR_SIZE_IN_BYTES 4*1024
#define TOTA_DATA_BUFFER_SIZE_FOR_BURNING FLASH_SECTOR_SIZE_IN_BYTES
#define TOTA_SERVICE_SYNC_WORD  0x54534542
#define TOTA_SERVICE_SYNC_DATA_1    0xbe
#define TOTA_SERVICE_SYNC_DATA_2    0xca
#define TOTA_SERVICE_XFER_OUT_DATA_SIZE    500
#define BT_LOCAL_NAME_LEN 64
#define DANGLE_TRANS_MAX_SIZE 248
#define DUMP_BYTES 32

enum{
    TOTA_SERVICE_DATA_XFER_ADDR_WRITE,
    TOTA_SERVICE_DATA_XFER_ADDR_READ,
    TOTA_SERVICE_DATA_XFER_ADDR_NONE,
};


typedef enum{
    TOTA_SERVICE_GENERAL_CMD_PARSER_NONE,
    TOTA_SERVICE_GENERAL_CMD_STATE_PARSER_SYNC,
    TOTA_SERVICE_GENERAL_CMD_STATE_PARSER_DATA,
    TOTA_SERVICE_GENERAL_CMD_STATE_PARSER_EXTRAL,
}TOTA_SERVICE_GENERAL_CMD_PARSER_STATE;


typedef enum{
    TOTA_SERVICE_INIT_SYNC = 0,             /*valid payload: |2 bytes sync word |                                               */
    TOTA_SERVICE_CMD_WRITE_FLASH,           /*valid payload: |4 bytes address|4 bytes write data length|                        */
    TOTA_SERVICE_CMD_READ_FLASH,            /*valid payload: |1 byte start_or_stop|4 bytes address|4 bytes read data length|  */
    TOTA_SERVICE_CMD_WRITE_AI_ENV_DATA,     /*valid payload: |4 bytes write data length|                                        */
    TOTA_SERVICE_CMD_VERIFY_AI_ENV_DATA,    /*valid payload: |4 bytes magic code |4 bytes CRC|                                  */
    TOTA_SERVICE_CMD_READ_AI_ENV_DATA,      /*valid payload: |1 byte start_or_stop|4 bytes read data length |                  */
    TOTA_SERVICE_CMD_READ_ACK,              /*valid rsp payload: |1 bytes|                                                      */
    TOTA_SERVICE_CMD_GET_BAT,               /*valid rsp payload: |4 bytes|                                                      */
    TOTA_SERVICE_CMD_GET_FW_VERSION,        /*valid rsp payload: |4 bytes|                                                      */
    TOTA_SERVICE_CMD_GET_BT_LOCAL_NAME,     /*valid rsp payload: |x bytes|                                                      */
    TOTA_SERVICE_CMD_GET_BT_ADDR,           /*valid rsp payload: |6 bytes|                                                      */
    TOTA_SERVICE_CMD_CLEAR_PAIRING_INFO,
    TOTA_SERVICE_CMD_GENERAL_TEST,
    TOTA_SERVICE_CMD_SHUTDOWM,
    TOTA_SERVICE_CMD_MIC_TEST_ON,
    TOTA_SERVICE_CMD_MIC_TEST_OFF,
    TOTA_SERVICE_CMD_MIC_SWITCH,
    TOTA_SERVICE_CMD_COUNT = 0xff,          /*valid payload                                                                     */
}TOTA_GENERAL_CMD_E;

typedef enum{
    TOTA_SERVICE_SEND_STATUS_NO_ERROR = 0,
    TOTA_SERVICE_SEND_STATUS_INVALID_CMD = 1,
    TOTA_SERVICE_SEND_STATUS_PARAM_ERR = 2,
    TOTA_SERVICE_SEND_STATUS_END = 3,
}TOTA_SERVICE_SEND_STATUS_E;


typedef struct{
    uint16_t cmd;
    uint16_t len;
}TOTA_SERVICE_CMD_RSP_HEADER_T;

typedef struct{
    uint8_t is_to_start;
    uint32_t start_addr;
    uint32_t len;
}__attribute__ ((__packed__)) TOTA_SERVICE_CMD_READ_FLASH_BODY_STRUCT_T;

typedef struct{
    uint32_t start_addr;
    uint32_t len;
}TOTA_SERVICE_CMD_WRITE_FLASH_BODY_STRUCT_T;

typedef struct{
    uint8_t is_to_start;
    uint32_t start_addr;
    uint32_t len;
    uint8_t read_ai_env_ack;
}__attribute__ ((__packed__)) TOTA_SERVICE_CMD_READ_AI_ENV_DATA_BODY_STRUCT_T;

typedef struct{
    uint32_t len;
}TOTA_SERVICE_CMD_WRITE_AI_ENV_DATA_BODY_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t ack_val;
}__attribute__ ((__packed__)) TOTA_SERVICE_CMD_SYNC_ACK;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t write_ai_env_ack;
} __attribute__ ((__packed__)) TOTA_SERVICE_CMD_WRITE_AI_ENV_DATA_ACK_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t verify_ai_env_ack;
} __attribute__ ((__packed__)) TOTA_SERVICE_CMD_VERIFY_AI_ENV_DATA_ACK_STRUCT_T;

typedef uint32_t BAT_VAL_BYTES;
typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    BAT_VAL_BYTES bat_val;
}TOTA_SERVICE_CMD_GET_BAT_VAL_BODY_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t fw_version[4];
}TOTA_SERVICE_CMD_GET_FW_VERSION_BODY_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    char bt_local_name[BT_LOCAL_NAME_LEN];
}__attribute__ ((__packed__))TOTA_SERVICE_CMD_GET_BT_LOCAL_NAME_BODY_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t bt_addr[6];
}__attribute__ ((__packed__))TOTA_SERVICE_CMD_GET_BT_ADDR_BODY_STRUCT_T;

typedef struct{
    TOTA_SERVICE_CMD_RSP_HEADER_T cmd_rsp_header;
    uint8_t test_val[7];
}__attribute__ ((__packed__))TOTA_SERVICE_CMD_GET_TEST_VAL_BODY_STRUCT_T;

typedef struct{
    uint32_t prefix;
    uint16_t cmd;
    uint8_t seq;
    uint8_t len;
}msg_header_t;

typedef struct{
    msg_header_t hdr;
    unsigned char cmd_param[TOTA_SERVICE_CMD_PARAM_LENGTH];
}TOTA_GENERAL_CMD_STRUCT_T;

typedef struct{
    uint8_t* tota_write_buff_ptr;
    uint16_t tota_data_write_buf_block_lenth;
    uint16_t tota_data_written_lenth_in_block;

    uint32_t tota_data_need_write_flash_begin_addr;
    uint32_t tota_data_tota_need_write_lenth;
    uint32_t tota_data_tota_already_receive_length;
    uint32_t tota_data_written_in_flash_pos ;
    uint32_t tota_offset_in_flash_of_current_block;
    uint32_t tota_offset_of_data_of_current_block;
}TOTA_SERVICE_DATA_WRITE_STRUCT_T;

typedef struct{
    uint8_t* tota_read_buff_ptr;
    uint16_t tota_data_read_buf_block_lenth;
    uint16_t tota_data_have_read_lenth_in_block;

    uint32_t tota_data_need_read_flash_begin_addr;
    uint32_t tota_data_tota_need_read_lenth;
    uint32_t tota_data_tota_already_sent_length;
    uint32_t tota_data_have_read_in_flash_pos ;
    uint32_t tota_offset_in_flash_of_current_block;
    uint32_t tota_offset_of_data_of_current_block;
}TOTA_SERVICE_DATA_READ_STRUCT_T;


/**
 * @brief The format of the segment verification request
 *
 */
typedef struct
{
    uint32_t    magicCode;      // should be OTA_START_MAGIC_CODE
    uint32_t    crc32OfSegment;          // crc32 of the 1 segment
} __attribute__ ((__packed__)) TOTA_SEGMENT_VERIFY_T;

void app_tota_init(void);
void app_tota_connected(uint8_t connType);
void app_tota_disconnected(uint8_t disconnType);
void app_tota_general_connected(uint8_t connType);
bool app_is_in_tota_mode(void);
void app_tota_update_datapath(APP_TOTA_TRANSMISSION_PATH_E dataPath);
APP_TOTA_TRANSMISSION_PATH_E app_tota_get_datapath(void);


/* interface */
void tota_connected_handle();
void tota_disconnected_handle();
bool is_tota_connected();
void tota_set_encrypt_key_from_hash_key(uint8_t * set_key);


uint8_t * tota_encrypt_packet(uint8_t * in, uint32_t inLen, uint32_t * poutLen);
uint8_t * tota_decrypt_packet(uint8_t * in, uint32_t inLen, uint32_t * poutLen);

void tota_printf(const char * format, ...);


/*todo: for test*/
void test_aes_encode_decode();



#ifdef __cplusplus
}
#endif

#endif

