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
#ifndef __REG_SENSOR_ENG_BEST2300P_H__
#define __REG_SENSOR_ENG_BEST2300P_H__

#include "plat_types.h"

struct SENSOR_ENG_T {
    __IO uint32_t SLV0_CONFIG_REG;  // 0000
    __IO uint32_t SLV1_CONFIG_REG;  // 0004
    __IO uint32_t SLV2_CONFIG_REG;  // 0008
    __IO uint32_t SLV3_CONFIG_REG;  // 000C
    __IO uint32_t GLOBAL_CFG0_REG;  // 0010
    __IO uint32_t GLOBAL_CFG1_REG;  // 0014
    __IO uint32_t GLOBAL_CFG2_REG;  // 0018
    __IO uint32_t GLOBAL_CFG3_REG;  // 001C
    __IO uint32_t SLV0_INTR_MASK;   // 0020
    __IO uint32_t SLV1_INTR_MASK;   // 0024
    __IO uint32_t SLV2_INTR_MASK;   // 0028
    __IO uint32_t SLV3_INTR_MASK;   // 002C
    __IO uint32_t SENSOR_INTR_CLR;  // 0030
    __I  uint32_t SENSOR_INTR_STS;  // 0034
    __IO uint32_t TIMER_INTVL_NUM;  // 0038
    __IO uint32_t SENSOR_STATUS;    // 003C
    __IO uint32_t I2C0_BASE_ADDR;   // 0040
    __IO uint32_t I2C1_BASE_ADDR;   // 0044
    __IO uint32_t SPI0_BASE_ADDR;   // 0048
    __IO uint32_t SPI1_BASE_ADDR;   // 004C
    __IO uint32_t SPI2_BASE_ADDR;   // 0050
};

// 0000 - 000C
#define SLV_SPI_RXDS                        (1 << 15)
#define SLV_RDN_WR                          (1 << 14)
#define SLV_I2C_SPI_SEL_SHIFT               11
#define SLV_I2C_SPI_SEL_MASK                (0x7 << SLV_I2C_SPI_SEL_SHIFT)
#define SLV_I2C_SPI_SEL(n)                  BITFIELD_VAL(SLV_I2C_SPI_SEL, n)
#define SLV_SLV_DEV_ID_SHIFT                1
#define SLV_SLV_DEV_ID_MASK                 (0x3FF << SLV_SLV_DEV_ID_SHIFT)
#define SLV_SLV_DEV_ID(n)                   BITFIELD_VAL(SLV_SLV_DEV_ID, n)
#define SLV_ENABLE                          (1 << 0)

// 0010
#define GBL_CODEC_SX_SEL_SHIFT              5
#define GBL_CODEC_SX_SEL_MASK               (0x3 << GBL_CODEC_SX_SEL_SHIFT)
#define GBL_CODEC_SX_SEL(n)                 BITFIELD_VAL(GBL_CODEC_SX_SEL, n)
#define GBL_TRI_INORDER                     (1 << 4)
#define GBL_SENSORHUB_EN                    (1 << 3)
#define GBL_RESERVED                        (1 << 2)
#define GBL_TIMER_SEL                       (1 << 1)
#define GBL_TRIGGER_TYPE                    (1 << 0)

// 0014
#define NUM_OF_RX_LLI3_SHIFT                24
#define NUM_OF_RX_LLI3_MASK                 (0xFF << NUM_OF_RX_LLI3_SHIFT)
#define NUM_OF_RX_LLI3(n)                   BITFIELD_VAL(NUM_OF_RX_LLI3, n)
#define NUM_OF_RX_LLI2_SHIFT                16
#define NUM_OF_RX_LLI2_MASK                 (0xFF << NUM_OF_RX_LLI2_SHIFT)
#define NUM_OF_RX_LLI2(n)                   BITFIELD_VAL(NUM_OF_RX_LLI2, n)
#define NUM_OF_RX_LLI1_SHIFT                8
#define NUM_OF_RX_LLI1_MASK                 (0xFF << NUM_OF_RX_LLI1_SHIFT)
#define NUM_OF_RX_LLI1(n)                   BITFIELD_VAL(NUM_OF_RX_LLI1, n)
#define NUM_OF_RX_LLI0_SHIFT                0
#define NUM_OF_RX_LLI0_MASK                 (0xFF << NUM_OF_RX_LLI0_SHIFT)
#define NUM_OF_RX_LLI0(n)                   BITFIELD_VAL(NUM_OF_RX_LLI0, n)

// 001C
#define NUM_OF_TX_LLI3_SHIFT                24
#define NUM_OF_TX_LLI3_MASK                 (0xFF << NUM_OF_TX_LLI3_SHIFT)
#define NUM_OF_TX_LLI3(n)                   BITFIELD_VAL(NUM_OF_TX_LLI3, n)
#define NUM_OF_TX_LLI2_SHIFT                16
#define NUM_OF_TX_LLI2_MASK                 (0xFF << NUM_OF_TX_LLI2_SHIFT)
#define NUM_OF_TX_LLI2(n)                   BITFIELD_VAL(NUM_OF_TX_LLI2, n)
#define NUM_OF_TX_LLI1_SHIFT                8
#define NUM_OF_TX_LLI1_MASK                 (0xFF << NUM_OF_TX_LLI1_SHIFT)
#define NUM_OF_TX_LLI1(n)                   BITFIELD_VAL(NUM_OF_TX_LLI1, n)
#define NUM_OF_TX_LLI0_SHIFT                0
#define NUM_OF_TX_LLI0_MASK                 (0xFF << NUM_OF_TX_LLI0_SHIFT)
#define NUM_OF_TX_LLI0(n)                   BITFIELD_VAL(NUM_OF_TX_LLI0, n)

// 0030
#define SENSOR_INTR_CLEAR                   (1 << 0)

// 0034
#define EXT_SENSOR3_INTR                    (1 << 4)
#define EXT_SENSOR2_INTR                    (1 << 3)
#define EXT_SENSOR1_INTR                    (1 << 2)
#define EXT_SENSOR0_INTR                    (1 << 1)
#define SENSOR_WKUP_INTR                    (1 << 0)

// 0038
#define TIMER_INTERVAL_NUM_SHIFT            0
#define TIMER_INTERVAL_NUM_MASK             (0xFF << TIMER_INTERVAL_NUM_SHIFT)
#define TIMER_INTERVAL_NUM(n)               BITFIELD_VAL(TIMER_INTERVAL_NUM, n)

// 003C
#define SENSOR_ENG_FSM_SHIFT                8
#define SENSOR_ENG_FSM_MASK                 (0xF << SENSOR_ENG_FSM_SHIFT)
#define SENSOR_ENG_FSM(n)                   BITFIELD_VAL(SENSOR_ENG_FSM, n)
#define SENSOR3_BUSY                        (1 << 7)
#define SENSOR2_BUSY                        (1 << 6)
#define SENSOR1_BUSY                        (1 << 5)
#define SENSOR0_BUSY                        (1 << 4)

#endif

