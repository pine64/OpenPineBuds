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
#ifndef _COREDUMP_SECTION_H
#define _COREDUMP_SECTION_H
#define COREDUMP_SECTOR_SIZE             0x1000
#define COREDUMP_SECTOR_SIZE_MASK        0xFFF
#define COREDUMP_BUFFER_LEN              (COREDUMP_SECTOR_SIZE*2)
#define COREDUMP_NORFALSH_BUFFER_LEN     (COREDUMP_BUFFER_LEN)
#if defined(__cplusplus)
extern "C" {
#endif

void coredump_to_flash_init();
void core_dump_erase_section();
int32_t core_dump_write_large(const uint8_t* ptr,uint32_t len);
int32_t core_dump_write(const uint8_t* ptr,uint32_t len);

#if defined(__cplusplus)
}
#endif

#endif

