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

#ifndef __APP_TOTA_FLASH_PROGRAM_H__
#define __APP_TOTA_FLASH_PROGRAM_H__


//TODO:
#define MAX_FLASH_HANDLE_SIZE 650
#define TOTA_FLASH_TEST_ADDR                          (0x200000+FLASH_NC_BASE+4096)


void app_tota_flash_init();

void tota_write_flash_test(uint32_t startAddr, uint8_t * dataBuf, uint32_t dataLen);

/*
**  4 + 650 + 2 + 4 = 660
*/
typedef struct{
    uint32_t    address;
    uint16_t    dataLen;
    uint32_t    dataCrc;
    uint8_t     dataBuf[MAX_FLASH_HANDLE_SIZE];
}TOTA_WRITE_FLASH_STRUCT_T;


typedef struct{
    uint32_t    address;
    uint16_t    length;
}TOTA_ERASE_FLASH_STRUCT_T;


#endif
