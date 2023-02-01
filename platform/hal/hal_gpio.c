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
#include "hal_gpio.h"
#include "cmsis_nvic.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "reg_gpio.h"
#include "stdarg.h"
#include "stdio.h"
#ifdef PMU_HAS_LED_PIN
#include "pmu.h"
#endif

#define HAL_GPIO_BANK_NUM 1
#define HAL_GPIO_PORT_NUM 1

#define HAL_GPIO_PIN_NUM_EACH_PORT (32)
#define HAL_GPIO_PIN_NUM_EACH_BANK                                             \
  (HAL_GPIO_PORT_NUM * HAL_GPIO_PIN_NUM_EACH_PORT)

#define HAL_GPIO_PIN_TO_BANK(pin) ((pin) / HAL_GPIO_PIN_NUM_EACH_BANK)

#define HAL_GPIO_PIN_TO_PORT(pin)                                              \
  (((pin) % HAL_GPIO_PIN_NUM_EACH_BANK) / HAL_GPIO_PIN_NUM_EACH_PORT)

#ifdef GPIOAUX_BASE
#define HAL_GPIO_AUX_BANK_NUM 1
#define HAL_GPIO_AUX_PORT_NUM 1

#define HAL_GPIO_AUX_PIN_NUM_EACH_PORT (8)
#define HAL_GPIO_AUX_PIN_NUM_EACH_BANK                                         \
  (HAL_GPIO_AUX_PORT_NUM * HAL_GPIO_AUX_PIN_NUM_EACH_PORT)

#define HAL_GPIO_AUX_PIN_TO_BANK(pin)                                          \
  (((pin)-HAL_GPIO_PIN_P4_0) / HAL_GPIO_AUX_PIN_NUM_EACH_BANK)

#define HAL_GPIO_AUX_PIN_TO_PORT(pin)                                          \
  ((((pin)-HAL_GPIO_PIN_P4_0) % HAL_GPIO_AUX_PIN_NUM_EACH_BANK) /              \
   HAL_GPIO_AUX_PIN_NUM_EACH_PORT)
#endif

typedef void (*_HAL_GPIO_IRQ_HANDLER)(void);

struct GPIO_PORT_T {
  __IO uint32_t GPIO_DR;  // 0x00
  __IO uint32_t GPIO_DDR; // 0x04
  __IO uint32_t GPIO_CTL; // 0x08
};

struct GPIO_BANK_T {
  struct GPIO_PORT_T port[HAL_GPIO_PORT_NUM];
  struct GPIO_PORT_T _port_reserved[3];
  __IO uint32_t GPIO_INTEN;                      // 0x30
  __IO uint32_t GPIO_INTMASK;                    // 0x34
  __IO uint32_t GPIO_INTTYPE_LEVEL;              // 0x38
  __IO uint32_t GPIO_INT_POLARITY;               // 0x3C
  __I uint32_t GPIO_INTSTATUS;                   // 0x40
  __I uint32_t GPIO_RAW_INTSTATUS;               // 0x44
  __IO uint32_t GPIO_DEBOUNCE;                   // 0x48
  __IO uint32_t GPIO_PORTA_EOI;                  // 0x4C
  __I uint32_t GPIO_EXT_PORT[HAL_GPIO_PORT_NUM]; // 0x50
  __I uint32_t GPIO_EXT_PORT_reserved[3];
  __IO uint32_t GPIO_LS_SYNC; // 0x60
};

void _hal_gpio_bank0_irq_handler(void);

static struct GPIO_BANK_T *const gpio_bank[HAL_GPIO_BANK_NUM] = {
    (struct GPIO_BANK_T *)GPIO_BASE,
};

static HAL_GPIO_PIN_IRQ_HANDLER gpio_irq_handler[HAL_GPIO_PIN_NUM] = {0};
static _HAL_GPIO_IRQ_HANDLER _gpio_irq_handler[HAL_GPIO_BANK_NUM] = {
    _hal_gpio_bank0_irq_handler,
};

#ifdef GPIOAUX_BASE
void _hal_gpio_aux_bank0_irq_handler(void);
static struct GPIO_BANK_T *const gpio_aux_bank[HAL_GPIO_BANK_NUM] = {
    (struct GPIO_BANK_T *)GPIOAUX_BASE,
};
#if defined(CHIP_BEST2000) && defined(PMU_HAS_LED_PIN)
static struct GPIO_BANK_T *const aon_gpio_aux_bank[HAL_GPIO_BANK_NUM] = {
    (struct GPIO_BANK_T *)AON_GPIOAUX_BASE,
};
#endif
static _HAL_GPIO_IRQ_HANDLER _gpio_aux_irq_handler[HAL_GPIO_AUX_BANK_NUM] = {
    _hal_gpio_aux_bank0_irq_handler,
};
#endif

static const char *const err_bad_pin = "Bad GPIO pin %u (should < %u)";

enum HAL_GPIO_DIR_T hal_gpio_pin_get_dir(enum HAL_GPIO_PIN_T pin) {
  int pin_offset = 0;
  int bank = 0, port = 0;
  enum HAL_GPIO_DIR_T dir = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#ifdef PMU_HAS_LED_PIN
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (0) {
  }
#ifdef PMU_HAS_LED_PIN
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
    return pmu_led_get_direction(pin);
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    if (gpio_aux_bank[bank]->port[port].GPIO_DDR & (0x1 << pin_offset))
      dir = HAL_GPIO_DIR_OUT;
    else
      dir = HAL_GPIO_DIR_IN;
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    if (gpio_bank[bank]->port[port].GPIO_DDR & (0x1 << pin_offset))
      dir = HAL_GPIO_DIR_OUT;
    else
      dir = HAL_GPIO_DIR_IN;
  }

  return dir;
}

void hal_gpio_pin_set_dir(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_DIR_T dir,
                          uint8_t val_for_out) {
  int pin_offset = 0;
  int bank = 0, port = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#ifdef PMU_HAS_LED_PIN
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (dir == HAL_GPIO_DIR_OUT) {
    if (val_for_out) {
      hal_gpio_pin_set(pin);
    } else {
      hal_gpio_pin_clr(pin);
    }
  }

  if (0) {
  }
#ifdef PMU_HAS_LED_PIN
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
    pmu_led_set_direction(pin, dir);
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    if (dir == HAL_GPIO_DIR_OUT)
      gpio_aux_bank[bank]->port[port].GPIO_DDR |= 0x1 << pin_offset;
    else
      gpio_aux_bank[bank]->port[port].GPIO_DDR &= ~(0x1 << pin_offset);
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    if (dir == HAL_GPIO_DIR_OUT)
      gpio_bank[bank]->port[port].GPIO_DDR |= 0x1 << pin_offset;
    else
      gpio_bank[bank]->port[port].GPIO_DDR &= ~(0x1 << pin_offset);
  }
}

void hal_gpio_pin_set(enum HAL_GPIO_PIN_T pin) {
  int pin_offset = 0;
  int bank = 0, port = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#ifdef PMU_HAS_LED_PIN
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (0) {
  }
#ifdef PMU_HAS_LED_PIN
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
#ifdef CHIP_BEST2000
    pin_offset = pin - HAL_GPIO_PIN_LED1 + 6;

    aon_gpio_aux_bank[0]->port[0].GPIO_DR |= 0x1 << pin_offset;
#else
    pmu_led_set_value(pin, 1);
#endif
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    gpio_aux_bank[bank]->port[port].GPIO_DR |= 0x1 << pin_offset;
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    gpio_bank[bank]->port[port].GPIO_DR |= 0x1 << pin_offset;
  }
}

void hal_gpio_pin_clr(enum HAL_GPIO_PIN_T pin) {
  int pin_offset = 0;
  int bank = 0, port = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#ifdef PMU_HAS_LED_PIN
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (0) {
  }
#ifdef PMU_HAS_LED_PIN
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
#ifdef CHIP_BEST2000
    pin_offset = pin - HAL_GPIO_PIN_LED1 + 6;

    aon_gpio_aux_bank[0]->port[0].GPIO_DR &= ~(0x1 << pin_offset);
#else
    pmu_led_set_value(pin, 0);
#endif
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    gpio_aux_bank[bank]->port[port].GPIO_DR &= ~(0x1 << pin_offset);
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    gpio_bank[bank]->port[port].GPIO_DR &= ~(0x1 << pin_offset);
  }
}

uint8_t hal_gpio_pin_get_val(enum HAL_GPIO_PIN_T pin) {
  int pin_offset = 0;
  int bank = 0, port = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#ifdef PMU_HAS_LED_PIN
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (0) {
  }
#ifdef PMU_HAS_LED_PIN
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
#ifdef CHIP_BEST2000
    pin_offset = pin - HAL_GPIO_PIN_LED1 + 6;

    return (((aon_gpio_aux_bank[0]->GPIO_EXT_PORT[0]) & (0x1 << pin_offset)) >>
            pin_offset);
#else
    return pmu_led_get_value(pin);
#endif
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    /* when as input : read back outside signal value */
    /* when as output: read back DR register value ,same as read back DR
     * register */

    return (
        ((gpio_aux_bank[bank]->GPIO_EXT_PORT[port]) & (0x1 << pin_offset)) >>
        pin_offset);
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    /* when as input : read back outside signal value */
    /* when as output: read back DR register value ,same as read back DR
     * register */

    return (((gpio_bank[bank]->GPIO_EXT_PORT[port]) & (0x1 << pin_offset)) >>
            pin_offset);
  }
}

void _hal_gpio_bank0_irq_handler(void) {
  uint32_t raw_status = 0, bank = 0, pin_offset = 0;

  raw_status = gpio_bank[bank]->GPIO_RAW_INTSTATUS;

  if (raw_status == 0) {
    return;
  }

  /* clear irq */
  gpio_bank[bank]->GPIO_PORTA_EOI = raw_status;

  while (raw_status) {
    if (raw_status & 0x1) {
      if (gpio_irq_handler[pin_offset]) {
        gpio_irq_handler[pin_offset](pin_offset +
                                     bank * HAL_GPIO_PIN_NUM_EACH_BANK);
      }
    }
    raw_status >>= 1;
    ++pin_offset;
  }
}

#ifdef GPIOAUX_BASE
void _hal_gpio_aux_bank0_irq_handler(void) {
  uint32_t raw_status = 0, bank = 0, pin_offset = 0;

  raw_status = 0;
  bank = 0;
  pin_offset = 0;

  raw_status = gpio_aux_bank[bank]->GPIO_RAW_INTSTATUS;

  if (raw_status == 0)
    return;

  /* clear irq */
  gpio_aux_bank[bank]->GPIO_PORTA_EOI = raw_status;

  while (raw_status) {
    if (raw_status & 0x1) {
      if (gpio_irq_handler[pin_offset + HAL_GPIO_PIN_P4_0]) {
        gpio_irq_handler[pin_offset + HAL_GPIO_PIN_P4_0](
            pin_offset + HAL_GPIO_PIN_P4_0 + bank * HAL_GPIO_PIN_NUM_EACH_BANK);
      }
    }
    raw_status >>= 1;
    ++pin_offset;
  }
}
#endif

uint8_t hal_gpio_setup_irq(enum HAL_GPIO_PIN_T pin,
                           const struct HAL_GPIO_IRQ_CFG_T *cfg) {
  int pin_offset = 0;
  int bank = 0, port = 0;

  enum HAL_GPIO_PIN_T max_pin = HAL_GPIO_PIN_NUM;
#if defined(PMU_HAS_LED_PIN) && defined(PMU_HAS_LED_IRQ)
  max_pin = HAL_GPIO_PIN_LED_NUM;
#endif
  ASSERT(pin < max_pin, err_bad_pin, pin, max_pin);

  if (0) {
  }
#if defined(PMU_HAS_LED_PIN) && defined(PMU_HAS_LED_IRQ)
  else if (pin == HAL_GPIO_PIN_LED1 || pin == HAL_GPIO_PIN_LED2) {
    return pmu_gpio_setup_irq(pin, cfg);
  }
#endif
#ifdef GPIOAUX_BASE
  else if (pin >= HAL_GPIO_PIN_P4_0) {
    bank = HAL_GPIO_AUX_PIN_TO_BANK(pin);
    port = HAL_GPIO_AUX_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_AUX_PIN_NUM_EACH_PORT;

    /* only port A support irq */
    if (port != 0)
      return 0;

    if (cfg->irq_enable) {
      gpio_aux_bank[bank]->GPIO_INTMASK |= (0x1 << pin_offset);

      if (cfg->irq_debounce)
        gpio_aux_bank[bank]->GPIO_DEBOUNCE |= 0x1 << pin_offset;
      else
        gpio_aux_bank[bank]->GPIO_DEBOUNCE &= ~(0x1 << pin_offset);

      if (cfg->irq_type == HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE)
        gpio_aux_bank[bank]->GPIO_INTTYPE_LEVEL |= 0x1 << pin_offset;
      else
        gpio_aux_bank[bank]->GPIO_INTTYPE_LEVEL &= ~(0x1 << pin_offset);

      if (cfg->irq_polarity == HAL_GPIO_IRQ_POLARITY_HIGH_RISING)
        gpio_aux_bank[bank]->GPIO_INT_POLARITY |= 0x1 << pin_offset;
      else
        gpio_aux_bank[bank]->GPIO_INT_POLARITY &= ~(0x1 << pin_offset);

      gpio_irq_handler[pin] = cfg->irq_handler;

      NVIC_SetVector(GPIOAUX_IRQn, (uint32_t)_gpio_aux_irq_handler[bank]);
      NVIC_SetPriority(GPIOAUX_IRQn, IRQ_PRIORITY_NORMAL);
      NVIC_EnableIRQ(GPIOAUX_IRQn);

      gpio_aux_bank[bank]->GPIO_INTMASK &= ~(0x1 << pin_offset);
      gpio_aux_bank[bank]->GPIO_INTEN |= 0x1 << pin_offset;
    } else {
      gpio_aux_bank[bank]->GPIO_INTMASK |= (0x1 << pin_offset);
      gpio_aux_bank[bank]->GPIO_INTEN &= ~(0x1 << pin_offset);
      gpio_irq_handler[pin] = 0;
    }
  }
#endif
  else {
    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin % HAL_GPIO_PIN_NUM_EACH_PORT;

    /* only port A support irq */
    if (port != 0)
      return 0;

    if (cfg->irq_enable) {
      gpio_bank[bank]->GPIO_INTMASK |= (0x1 << pin_offset);

      if (cfg->irq_debounce)
        gpio_bank[bank]->GPIO_DEBOUNCE |= 0x1 << pin_offset;
      else
        gpio_bank[bank]->GPIO_DEBOUNCE &= ~(0x1 << pin_offset);

      if (cfg->irq_type == HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE)
        gpio_bank[bank]->GPIO_INTTYPE_LEVEL |= 0x1 << pin_offset;
      else
        gpio_bank[bank]->GPIO_INTTYPE_LEVEL &= ~(0x1 << pin_offset);

      if (cfg->irq_polarity == HAL_GPIO_IRQ_POLARITY_HIGH_RISING)
        gpio_bank[bank]->GPIO_INT_POLARITY |= 0x1 << pin_offset;
      else
        gpio_bank[bank]->GPIO_INT_POLARITY &= ~(0x1 << pin_offset);

      gpio_irq_handler[pin] = cfg->irq_handler;

      NVIC_SetVector(GPIO_IRQn, (uint32_t)_gpio_irq_handler[bank]);
      NVIC_SetPriority(GPIO_IRQn, IRQ_PRIORITY_NORMAL);
      NVIC_EnableIRQ(GPIO_IRQn);

      gpio_bank[bank]->GPIO_INTMASK &= ~(0x1 << pin_offset);
      gpio_bank[bank]->GPIO_INTEN |= 0x1 << pin_offset;
    } else {
      gpio_bank[bank]->GPIO_INTMASK |= (0x1 << pin_offset);
      gpio_bank[bank]->GPIO_INTEN &= ~(0x1 << pin_offset);
      gpio_irq_handler[pin] = 0;
    }
  }
  return 0;
}
