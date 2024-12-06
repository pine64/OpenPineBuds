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
#ifndef __HAL_TRANSQ_H__
#define __HAL_TRANSQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CHIP_HAS_TRANSQ

#include "plat_types.h"

enum HAL_TRANSQ_ID_T {
    HAL_TRANSQ_ID_0 = 0,
#if (CHIP_HAS_TRANSQ > 1)
    HAL_TRANSQ_ID_1,
#endif

    HAL_TRANSQ_ID_QTY
};

enum HAL_TRANSQ_PRI_T {
    HAL_TRANSQ_PRI_NORMAL = 0,
    HAL_TRANSQ_PRI_HIGH,

    HAL_TRANSQ_PRI_QTY
};

enum HAL_TRANSQ_RET_T {
    HAL_TRANSQ_RET_OK = 0,
    HAL_TRANSQ_RET_BAD_ID,
    HAL_TRANSQ_RET_BAD_PRI,
    HAL_TRANSQ_RET_BAD_CFG,
    HAL_TRANSQ_RET_BAD_SLOT,
    HAL_TRANSQ_RET_BAD_TX_NUM,
    HAL_TRANSQ_RET_BAD_RX_NUM,
    HAL_TRANSQ_RET_BAD_MODE,
    HAL_TRANSQ_RET_RX_EMPTY,
    HAL_TRANSQ_RET_TX_FULL,
};

typedef void (*HAL_TRANSQ_RX_IRQ_HANDLER)(enum HAL_TRANSQ_PRI_T pri);
typedef void (*HAL_TRANSQ_TX_IRQ_HANDLER)(enum HAL_TRANSQ_PRI_T pri, const uint8_t *data, uint32_t len);

struct HAL_TRANSQ_SLOT_NUM_T {
    uint8_t tx_num[HAL_TRANSQ_PRI_QTY];
    uint8_t rx_num[HAL_TRANSQ_PRI_QTY];
};

struct HAL_TRANSQ_CFG_T {
    struct HAL_TRANSQ_SLOT_NUM_T slot;
    HAL_TRANSQ_RX_IRQ_HANDLER rx_handler;
    HAL_TRANSQ_TX_IRQ_HANDLER tx_handler;
};

enum HAL_TRANSQ_RET_T hal_transq_get_rx_status(enum HAL_TRANSQ_ID_T id, enum HAL_TRANSQ_PRI_T pri, bool *ready);

enum HAL_TRANSQ_RET_T hal_transq_get_tx_status(enum HAL_TRANSQ_ID_T id, enum HAL_TRANSQ_PRI_T pri, bool *done);

enum HAL_TRANSQ_RET_T hal_transq_rx_first(enum HAL_TRANSQ_ID_T id, enum HAL_TRANSQ_PRI_T pri, const uint8_t **data, uint32_t *len);

enum HAL_TRANSQ_RET_T hal_transq_rx_next(enum HAL_TRANSQ_ID_T id, enum HAL_TRANSQ_PRI_T pri, const uint8_t **data, uint32_t *len);

enum HAL_TRANSQ_RET_T hal_transq_tx(enum HAL_TRANSQ_ID_T id, enum HAL_TRANSQ_PRI_T pri, const uint8_t *data, uint32_t len);

enum HAL_TRANSQ_RET_T hal_transq_update_num(enum HAL_TRANSQ_ID_T id, const struct HAL_TRANSQ_SLOT_NUM_T *slot);

enum HAL_TRANSQ_RET_T hal_transq_open(enum HAL_TRANSQ_ID_T id, const struct HAL_TRANSQ_CFG_T *cfg);

enum HAL_TRANSQ_RET_T hal_transq_close(enum HAL_TRANSQ_ID_T id);

#endif // CHIP_HAS_TRANSQ

#ifdef __cplusplus
}
#endif

#endif
