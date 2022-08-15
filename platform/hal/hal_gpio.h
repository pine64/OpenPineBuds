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
#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_iomux.h"

enum HAL_GPIO_DIR_T {
    HAL_GPIO_DIR_IN = 0,
    HAL_GPIO_DIR_OUT = 1,
};

enum HAL_GPIO_IRQ_TYPE_T {
    HAL_GPIO_IRQ_TYPE_LEVEL_SENSITIVE = 0,
    HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE,
};

enum HAL_GPIO_IRQ_POLARITY_T {
    HAL_GPIO_IRQ_POLARITY_LOW_FALLING = 0,
    HAL_GPIO_IRQ_POLARITY_HIGH_RISING,
};

typedef void (* HAL_GPIO_PIN_IRQ_HANDLER)(enum HAL_GPIO_PIN_T pin);

struct HAL_GPIO_IRQ_CFG_T {
    uint8_t irq_enable:1;
    uint8_t irq_debounce:1;
    enum HAL_GPIO_IRQ_TYPE_T irq_type;
    enum HAL_GPIO_IRQ_POLARITY_T irq_polarity;
    HAL_GPIO_PIN_IRQ_HANDLER irq_handler;
};

enum HAL_GPIO_DIR_T hal_gpio_pin_get_dir(enum HAL_GPIO_PIN_T pin);
void hal_gpio_pin_set_dir(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_DIR_T dir, uint8_t val_for_out);
void hal_gpio_pin_set(enum HAL_GPIO_PIN_T pin);
void hal_gpio_pin_clr(enum HAL_GPIO_PIN_T pin);
uint8_t hal_gpio_pin_get_val(enum HAL_GPIO_PIN_T pin);
uint8_t hal_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, const struct HAL_GPIO_IRQ_CFG_T *cfg);

#ifdef __cplusplus
}
#endif

#endif
