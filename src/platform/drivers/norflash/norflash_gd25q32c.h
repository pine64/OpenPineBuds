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
#ifndef NORFLASH_GD25Q32C_H
#define NORFLASH_GD25Q32C_H

#include "plat_types.h"

/* bytes */
#define GD25Q32C_PAGE_SIZE (256)
#define GD25Q32C_SECTOR_SIZE (4096)
#define GD25Q32C_BLOCK_SIZE (32*1024)
#define GD25Q32C_TOTAL_SIZE (4*1024*1024)

#define P25Q32L_TOTAL_SIZE (4*1024*1024)

#define P25Q64L_TOTAL_SIZE (8*1024*1024)

#define P25Q128L_TOTAL_SIZE (16*1024*1024)

#define XM25QH16C_TOTAL_SIZE (2*1024*1024)

#define XM25QH80B_TOTAL_SIZE (1*1024*1024)

/* device cmd */
#define GD25Q32C_CMD_ID 0x9F
#define GD25Q32C_CMD_WRITE_ENABLE 0x06
#define GD25Q32C_CMD_PAGE_PROGRAM 0x02
#define GD25Q32C_CMD_DUAL_PAGE_PROGRAM 0xA2
#define GD25Q32C_CMD_QUAD_PAGE_PROGRAM 0x32
#define GD25Q32C_CMD_BLOCK_ERASE_32K 0x52
#define GD25Q32C_CMD_BLOCK_ERASE_64K 0xD8
#define GD25Q32C_CMD_BLOCK_ERASE GD25Q32C_CMD_BLOCK_ERASE_32K
#define GD25Q32C_CMD_SECTOR_ERASE 0x20
#define GD25Q32C_CMD_CHIP_ERASE 0x60
#define GD25Q32C_CMD_READ_STATUS_S0_S7 0x05
#define GD25Q32C_CMD_READ_STATUS_S8_S15 0x35
#define GD25Q32C_CMD_WRITE_STATUS_S0_S7 0x01
#define GD25Q32C_CMD_WRITE_STATUS_S8_S15 0x31

#define GD25Q32C_CMD_FAST_QUAD_IO_READ 0xEB
#define GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ 0x6B
#define GD25Q32C_CMD_FAST_DUAL_IO_READ 0xBB
#define GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ 0x3B
#define GD25Q32C_CMD_STANDARD_READ 0x03
#define GD25Q32C_CMD_STANDARD_FAST_READ 0x0B
#define GD25Q32C_CMD_DEEP_POWER_DOWN 0xB9
#define GD25Q32C_CMD_RELEASE_FROM_DP 0xAB
#define GD25Q32C_CMD_HIGH_PERFORMANCE 0xA3
#define GD25Q32C_CMD_SET_BURST_WRAP 0x77
#define GD25Q32C_CMD_UNIQUE_ID 0x4B
#define GD25Q32C_CMD_ENABLE_RESET 0x66
#define GD25Q32C_CMD_RESET 0x99
#define GD25Q32C_CMD_PROGRAM_ERASE_SUSPEND 0x75
#define GD25Q32C_CMD_PROGRAM_ERASE_RESUME 0x7A
#define GD25Q32C_CMD_SECURITY_REGISTER_ERASE 0x44
#define GD25Q32C_CMD_SECURITY_REGISTER_PROGRAM 0x42
#define GD25Q32C_CMD_SECURITY_REGISTER_READ 0x48

#define PUYA_FLASH_CMD_PAGE_ERASE 0x81

/* status register _S0_S7*/
#define GD25Q32C_WIP_BIT_SHIFT 0
#define GD25Q32C_WIP_BIT_MASK ((0x1)<<GD25Q32C_WIP_BIT_SHIFT)
#define GD25Q32C_WEL_BIT_SHIFT 1
#define GD25Q32C_WEL_BIT_MASK ((0x1)<<GD25Q32C_WEL_BIT_SHIFT)
#define GD25Q32C_BP0_4_BIT_SHIFT 2
#define GD25Q32C_BP0_4_BIT_MASK ((0x1F)<<GD25Q32C_WEL_BIT_SHIFT)
#define GD25Q32C_BP0_4_BIT(n) (((n) & 0x1F)<<GD25Q32C_WEL_BIT_SHIFT)
/* status register _S8_S15*/
#define GD25Q32C_QE_BIT_SHIFT 1
#define GD25Q32C_QE_BIT_MASK ((0x1)<<GD25Q32C_QE_BIT_SHIFT)
#define GD25Q32C_CMP_BIT_SHIFT 6
#define GD25Q32C_CMP_BIT_MASK ((0x1)<<GD25Q32C_CMP_BIT_SHIFT)

#endif /* NORFLASH_GD25Q32C_H */
