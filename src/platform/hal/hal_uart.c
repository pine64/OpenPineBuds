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
#ifdef CHIP_HAS_UART

#include "hal_uart.h"
#include "cmsis_nvic.h"
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "reg_uart.h"
#include "string.h"

#define UART_FLUSH_DELAY_DEFAULT 2

enum UART_RX_DMA_MODE_T {
  UART_RX_DMA_MODE_NORMAL,
  UART_RX_DMA_MODE_PINGPANG,
  UART_RX_DMA_MODE_STREAM,
  UART_RX_DMA_MODE_BUF_LIST,
};

struct HAL_UART_HW_DESC_T {
  struct UART_T *base;
  IRQn_Type irq;
  enum HAL_CMU_MOD_ID_T mod;
  enum HAL_CMU_MOD_ID_T apb;
  enum HAL_DMA_PERIPH_T rx_periph;
  enum HAL_DMA_PERIPH_T tx_periph;
};

static const struct HAL_UART_HW_DESC_T uart[HAL_UART_ID_QTY] = {
    {
        .base = (struct UART_T *)UART0_BASE,
        .irq = UART0_IRQn,
        .mod = HAL_CMU_MOD_O_UART0,
        .apb = HAL_CMU_MOD_P_UART0,
        .rx_periph = HAL_GPDMA_UART0_RX,
        .tx_periph = HAL_GPDMA_UART0_TX,
    },
#if (CHIP_HAS_UART > 1)
    {
        .base = (struct UART_T *)UART1_BASE,
        .irq = UART1_IRQn,
        .mod = HAL_CMU_MOD_O_UART1,
        .apb = HAL_CMU_MOD_P_UART1,
        .rx_periph = HAL_GPDMA_UART1_RX,
        .tx_periph = HAL_GPDMA_UART1_TX,
    },
#endif
#if (CHIP_HAS_UART > 2)
    {
        .base = (struct UART_T *)UART2_BASE,
        .irq = UART2_IRQn,
        .mod = HAL_CMU_MOD_O_UART2,
        .apb = HAL_CMU_MOD_P_UART2,
        .rx_periph = HAL_GPDMA_UART2_RX,
        .tx_periph = HAL_GPDMA_UART2_TX,
    },
#endif
#ifdef BT_UART
    {
        .base = (struct UART_T *)BT_UART_BASE,
        .irq = INVALID_IRQn,
        .mod = HAL_CMU_MOD_QTY,
        .apb = HAL_CMU_MOD_QTY,
        .rx_periph = HAL_DMA_PERIPH_NULL,
        .tx_periph = HAL_DMA_PERIPH_NULL,
    },
#endif
};

static bool init_done = false;

static HAL_UART_IRQ_HANDLER_T irq_handler[HAL_UART_ID_QTY] = {NULL};

static HAL_UART_IRQ_RXDMA_HANDLER_T rxdma_handler[HAL_UART_ID_QTY] = {NULL};
static HAL_UART_IRQ_TXDMA_HANDLER_T txdma_handler[HAL_UART_ID_QTY] = {NULL};

static uint8_t recv_dma_chan[HAL_UART_ID_QTY];
static uint8_t send_dma_chan[HAL_UART_ID_QTY];

static uint32_t recv_dma_size[HAL_UART_ID_QTY];
static uint32_t send_dma_size[HAL_UART_ID_QTY];

static union HAL_UART_IRQ_T recv_mask[HAL_UART_ID_QTY];

static enum UART_RX_DMA_MODE_T recv_dma_mode[HAL_UART_ID_QTY];

static const char *const err_invalid_id = "Invalid UART ID: %d";
static const char *const err_recv_dma_api =
    "%s: Set RT irq in hal_uart_dma_recv_mask... to avoid data lost";

static const struct HAL_UART_CFG_T default_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_4,
    .baud = 921600,
    .dma_rx = false,
    .dma_tx = false,
    .dma_rx_stop_on_err = false,
};

static void hal_uart_irq_handler(void);

static void set_baud_rate(enum HAL_UART_ID_T id, uint32_t rate) {
  uint32_t mod_clk;
  uint32_t ibrd, fbrd;
  uint32_t div;

  mod_clk = 0;
#ifdef PERIPH_PLL_FREQ
  if (PERIPH_PLL_FREQ / 2 > 2 * hal_cmu_get_crystal_freq()) {
    // Init to OSC_X2
    mod_clk = 2 * hal_cmu_get_crystal_freq() / 16;
    if (cfg->baud > mod_clk) {
      mod_clk = PERIPH_PLL_FREQ / 2 / 16;

      if (id == HAL_UART_ID_0) {
        hal_cmu_uart0_set_div(2);
#if (CHIP_HAS_UART > 1)
      } else if (id == HAL_UART_ID_1) {
        hal_cmu_uart1_set_div(2);
#endif
#if (CHIP_HAS_UART > 2)
      } else if (id == HAL_UART_ID_2) {
        hal_cmu_uart2_set_div(2);
#endif
      }
    } else {
      mod_clk = 0;
    }
  }
#endif
  if (mod_clk == 0) {
    enum HAL_CMU_PERIPH_FREQ_T periph_freq;

    // Init to OSC
    mod_clk = hal_cmu_get_crystal_freq() / 16;
    if (rate > mod_clk) {
      mod_clk *= 2;
      periph_freq = HAL_CMU_PERIPH_FREQ_52M;
    } else {
      periph_freq = HAL_CMU_PERIPH_FREQ_26M;
    }

    if (id == HAL_UART_ID_0) {
      hal_cmu_uart0_set_freq(periph_freq);
#if (CHIP_HAS_UART > 1)
    } else if (id == HAL_UART_ID_1) {
      hal_cmu_uart1_set_freq(periph_freq);
#endif
#if (CHIP_HAS_UART > 2)
    } else if (id == HAL_UART_ID_2) {
      hal_cmu_uart2_set_freq(periph_freq);
#endif
    }
  }

  div = (mod_clk * 64 + rate / 2) / rate;
  ibrd = div / 64;
  fbrd = div % 64;
  if (ibrd == 0 || ibrd >= 65535) {
    ASSERT(false, "Invalid baud param: %d", rate);
  }

  uart[id].base->UARTIBRD = ibrd;
  uart[id].base->UARTFBRD = fbrd;

  return;
}

int hal_uart_open(enum HAL_UART_ID_T id, const struct HAL_UART_CFG_T *cfg) {
  uint32_t cr, lcr, dmacr;
  int i;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  if (!init_done) {
    init_done = true;
    for (i = HAL_UART_ID_0; i < HAL_UART_ID_QTY; i++) {
      recv_dma_chan[i] = HAL_DMA_CHAN_NONE;
      send_dma_chan[i] = HAL_DMA_CHAN_NONE;
    }
  }

  if (hal_uart_opened(id)) {
    hal_uart_close(id);
  }

  if (cfg == NULL) {
    cfg = &default_cfg;
  }

  hal_cmu_clock_enable(uart[id].mod);
  hal_cmu_clock_enable(uart[id].apb);
  hal_cmu_reset_clear(uart[id].mod);
  hal_cmu_reset_clear(uart[id].apb);

  cr = lcr = 0;

  switch (cfg->parity) {
  case HAL_UART_PARITY_NONE:
    break;
  case HAL_UART_PARITY_ODD:
    lcr |= UARTLCR_H_PEN;
    break;
  case HAL_UART_PARITY_EVEN:
    lcr |= UARTLCR_H_PEN | UARTLCR_H_EPS;
    break;
  case HAL_UART_PARITY_FORCE1:
    lcr |= UARTLCR_H_PEN | UARTLCR_H_SPS;
    break;
  case HAL_UART_PARITY_FORCE0:
    lcr |= UARTLCR_H_PEN | UARTLCR_H_EPS | UARTLCR_H_SPS;
    break;
  default:
    ASSERT(false, "Invalid parity param: %d", cfg->parity);
    break;
  }

  if (cfg->stop == HAL_UART_STOP_BITS_2) {
    lcr |= UARTLCR_H_STP2;
  } else if (cfg->stop != HAL_UART_STOP_BITS_1) {
    ASSERT(false, "Invalid stop bits param: %d", cfg->stop);
  }

  switch (cfg->data) {
  case HAL_UART_DATA_BITS_5:
    lcr |= UARTLCR_H_WLEN_5;
    break;
  case HAL_UART_DATA_BITS_6:
    lcr |= UARTLCR_H_WLEN_6;
    break;
  case HAL_UART_DATA_BITS_7:
    lcr |= UARTLCR_H_WLEN_7;
    break;
  case HAL_UART_DATA_BITS_8:
    lcr |= UARTLCR_H_WLEN_8;
    break;
  default:
    ASSERT(false, "Invalid data bits param: %d", cfg->data);
    break;
  }

  switch (cfg->flow) {
  case HAL_UART_FLOW_CONTROL_NONE:
    break;
  case HAL_UART_FLOW_CONTROL_RTS:
    cr |= UARTCR_RTSEN;
    break;
  case HAL_UART_FLOW_CONTROL_CTS:
    cr |= UARTCR_CTSEN;
    break;
  case HAL_UART_FLOW_CONTROL_RTSCTS:
    cr |= UARTCR_RTSEN | UARTCR_CTSEN;
    break;
  default:
    ASSERT(false, "Invalid flow control param: %d", cfg->flow);
    break;
  }

  lcr |= UARTLCR_H_FEN | UARTLCR_H_DMA_RT_CNT(9);
  cr |= UARTCR_UARTEN | UARTCR_TXE | UARTCR_RXE;

  dmacr = 0;
  if (cfg->dma_rx) {
    dmacr |= UARTDMACR_RXDMAE;
  }
  if (cfg->dma_tx) {
    dmacr |= UARTDMACR_TXDMAE;
  }
  if (cfg->dma_rx_stop_on_err) {
    dmacr |= UARTDMACR_DMAONERR;
  }

  // Disable UART
  uart[id].base->UARTCR &= ~UARTCR_UARTEN;
  // Empty FIFO
  uart[id].base->UARTLCR_H &= ~UARTLCR_H_FEN;
  // Wait until UART becomes idle
  while (((uart[id].base->UARTFR) & UARTFR_BUSY) != 0)
    ;
  // Clear previous errors
  uart[id].base->UARTECR = 1;
  // Clear previous IRQs
  uart[id].base->UARTIMSC = 0;
  uart[id].base->UARTICR = ~0UL;
  // Configure UART
  set_baud_rate(id, cfg->baud);
  uart[id].base->UARTLCR_H = lcr;
  uart[id].base->UARTDMACR = dmacr;
  uart[id].base->UARTIFLS = UARTIFLS_TXFIFO_LEVEL(cfg->tx_level) |
                            UARTIFLS_RXFIFO_LEVEL(cfg->rx_level);
  uart[id].base->UARTCR = cr;

  if (uart[id].irq != INVALID_IRQn) {
    NVIC_SetVector(uart[id].irq, (uint32_t)hal_uart_irq_handler);
    // The priority should be the same as DMA's
    NVIC_SetPriority(uart[id].irq, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(uart[id].irq);
    NVIC_EnableIRQ(uart[id].irq);
  }

  return 0;
}

int hal_uart_reopen(enum HAL_UART_ID_T id, const struct HAL_UART_CFG_T *cfg) {
  uint32_t cr, dmacr;
  int i;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  if (!init_done) {
    init_done = true;
    for (i = HAL_UART_ID_0; i < HAL_UART_ID_QTY; i++) {
      recv_dma_chan[i] = HAL_DMA_CHAN_NONE;
      send_dma_chan[i] = HAL_DMA_CHAN_NONE;
    }
  }

  cr = 0;
  switch (cfg->flow) {
  case HAL_UART_FLOW_CONTROL_NONE:
    break;
  case HAL_UART_FLOW_CONTROL_RTS:
    cr |= UARTCR_RTSEN;
    break;
  case HAL_UART_FLOW_CONTROL_CTS:
    cr |= UARTCR_CTSEN;
    break;
  case HAL_UART_FLOW_CONTROL_RTSCTS:
    cr |= UARTCR_RTSEN | UARTCR_CTSEN;
    break;
  default:
    ASSERT(false, "Invalid flow control param: %d", cfg->flow);
    break;
  }

  dmacr = 0;
  if (cfg->dma_rx) {
    dmacr |= UARTDMACR_RXDMAE;
  }
  if (cfg->dma_tx) {
    dmacr |= UARTDMACR_TXDMAE;
  }
  if (cfg->dma_rx_stop_on_err) {
    dmacr |= UARTDMACR_DMAONERR;
  }

  // Configure UART
  uart[id].base->UARTDMACR = dmacr;
  uart[id].base->UARTCR =
      (uart[id].base->UARTCR & ~(UARTCR_RTSEN | UARTCR_CTSEN)) | cr;

  if (uart[id].irq != INVALID_IRQn) {
    NVIC_SetVector(uart[id].irq, (uint32_t)hal_uart_irq_handler);
    // The priority should be the same as DMA's
    NVIC_SetPriority(uart[id].irq, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(uart[id].irq);
    NVIC_EnableIRQ(uart[id].irq);
  }

  return 0;
}

int hal_uart_close(enum HAL_UART_ID_T id) {
  uint32_t lock;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  if (uart[id].irq != INVALID_IRQn) {
    NVIC_DisableIRQ(uart[id].irq);
  }

  lock = int_lock();
  if (recv_dma_chan[id] != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(recv_dma_chan[id]);
    hal_gpdma_free_chan(recv_dma_chan[id]);
    recv_dma_chan[id] = HAL_DMA_CHAN_NONE;
  }
  if (send_dma_chan[id] != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(send_dma_chan[id]);
    hal_gpdma_free_chan(send_dma_chan[id]);
    send_dma_chan[id] = HAL_DMA_CHAN_NONE;
  }
  int_unlock(lock);

  // Disable UART
  uart[id].base->UARTCR &= ~UARTCR_UARTEN;
  // Empty FIFO
  uart[id].base->UARTLCR_H &= ~UARTLCR_H_FEN;

  hal_cmu_reset_set(uart[id].apb);
  hal_cmu_reset_set(uart[id].mod);
  hal_cmu_clock_disable(uart[id].apb);
  hal_cmu_clock_disable(uart[id].mod);

  return 0;
}

int hal_uart_opened(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  if (uart[id].apb < HAL_CMU_MOD_QTY &&
      hal_cmu_clock_get_status(uart[id].apb) != HAL_CMU_CLK_ENABLED) {
    return 0;
  }
  if (uart[id].base->UARTCR & UARTCR_UARTEN) {
    return 1;
  }
  return 0;
}

int hal_uart_change_baud_rate(enum HAL_UART_ID_T id, uint32_t rate) {
  union HAL_UART_FLAG_T flag;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  if (!hal_uart_opened(id)) {
    return 1;
  }

  flag.reg = uart[id].base->UARTFR;
  if (flag.BUSY) {
    return 2;
  }

  uart[id].base->UARTCR &= ~UARTCR_UARTEN;

  set_baud_rate(id, rate);

  uart[id].base->UARTLCR_H = uart[id].base->UARTLCR_H;
  uart[id].base->UARTCR |= UARTCR_UARTEN;

  return 0;
}

int hal_uart_pause(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  if (hal_uart_opened(id)) {
    uart[id].base->UARTCR &= ~(UARTCR_TXE | UARTCR_RXE);
    return 0;
  }
  return 1;
}

int hal_uart_continue(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  if (hal_uart_opened(id)) {
    uart[id].base->UARTCR |= (UARTCR_TXE | UARTCR_RXE);
    return 0;
  }
  return 1;
}

int hal_uart_readable(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  return (uart[id].base->UARTFR & UARTFR_RXFE) == 0;
}

int hal_uart_writable(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  return (uart[id].base->UARTFR & UARTFR_TXFF) == 0;
}

uint8_t hal_uart_getc(enum HAL_UART_ID_T id) {
  uint32_t c;
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  ASSERT((uart[id].base->UARTDMACR & UARTDMACR_RXDMAE) == 0,
         "RX-DMA configured on UART %d", id);
  c = uart[id].base->UARTDR;
  return (c & 0xFF);
}

int hal_uart_putc(enum HAL_UART_ID_T id, uint8_t c) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  ASSERT((uart[id].base->UARTDMACR & UARTDMACR_TXDMAE) == 0,
         "TX-DMA configured on UART %d", id);
  uart[id].base->UARTDR = c;
  return 0;
}

uint8_t hal_uart_blocked_getc(enum HAL_UART_ID_T id) {
  while (hal_uart_readable(id) == 0)
    ;
  return hal_uart_getc(id);
}

int hal_uart_blocked_putc(enum HAL_UART_ID_T id, uint8_t c) {
  while (hal_uart_writable(id) == 0)
    ;
  return hal_uart_putc(id, c);
}

union HAL_UART_FLAG_T hal_uart_get_flag(enum HAL_UART_ID_T id) {
  union HAL_UART_FLAG_T flag;
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  flag.reg = uart[id].base->UARTFR;
  return flag;
}

union HAL_UART_STATUS_T hal_uart_get_status(enum HAL_UART_ID_T id) {
  union HAL_UART_STATUS_T status;
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  status.reg = uart[id].base->UARTRSR;
  return status;
}

void hal_uart_clear_status(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  uart[id].base->UARTECR = 0;
}

void hal_uart_break_set(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  uart[id].base->UARTLCR_H |= UARTLCR_H_BRK;
}

void hal_uart_break_clear(enum HAL_UART_ID_T id) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  uart[id].base->UARTLCR_H &= ~UARTLCR_H_BRK;
}

void hal_uart_flush(enum HAL_UART_ID_T id, uint32_t ticks) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  if (!hal_uart_opened(id)) {
    return;
  }
  if ((uart[id].base->UARTLCR_H & UARTLCR_H_FEN) == 0) {
    return;
  }

  // Disable the UART
  uart[id].base->UARTCR &= ~UARTCR_UARTEN;
  // Wait for the end of transmission or reception of the current character
  hal_sys_timer_delay((ticks > 0) ? ticks : UART_FLUSH_DELAY_DEFAULT);

  // Flush FIFO
  uart[id].base->UARTLCR_H &= ~UARTLCR_H_FEN;
  uart[id].base->UARTLCR_H |= UARTLCR_H_FEN;

  // Enable the UART
  uart[id].base->UARTCR |= UARTCR_UARTEN;
}

union HAL_UART_IRQ_T hal_uart_get_raw_irq(enum HAL_UART_ID_T id) {
  union HAL_UART_IRQ_T irq;

  irq.reg = uart[id].base->UARTRIS;

  return irq;
}

void hal_uart_clear_irq(enum HAL_UART_ID_T id, union HAL_UART_IRQ_T irq) {
  uart[id].base->UARTICR = irq.reg;
}

union HAL_UART_IRQ_T hal_uart_irq_get_mask(enum HAL_UART_ID_T id) {
  union HAL_UART_IRQ_T mask;

  mask.reg = uart[id].base->UARTIMSC;

  return mask;
}

union HAL_UART_IRQ_T hal_uart_irq_set_mask(enum HAL_UART_ID_T id,
                                           union HAL_UART_IRQ_T mask) {
  union HAL_UART_IRQ_T old_mask;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  if (mask.RT) {
    ASSERT(recv_dma_chan[id] == HAL_DMA_CHAN_NONE, err_recv_dma_api,
           __FUNCTION__);
  }
  old_mask.reg = uart[id].base->UARTIMSC;
  uart[id].base->UARTIMSC = mask.reg;

  return old_mask;
}

HAL_UART_IRQ_HANDLER_T
hal_uart_irq_set_handler(enum HAL_UART_ID_T id,
                         HAL_UART_IRQ_HANDLER_T handler) {
  HAL_UART_IRQ_HANDLER_T old_handler;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  old_handler = irq_handler[id];
  irq_handler[id] = handler;

  return old_handler;
}

static void dma_mode_uart_irq_handler(enum HAL_UART_ID_T id,
                                      union HAL_UART_IRQ_T status) {
  uint32_t xfer = 0;
  uint32_t lock;

  if (status.RT || status.FE || status.OE || status.PE || status.BE) {
    if (recv_dma_mode[id] == UART_RX_DMA_MODE_NORMAL) {
      // Restore the traditional RT behaviour
      lock = int_lock();
      uart[id].base->UARTLCR_H &= ~UARTLCR_H_DMA_RT_EN;
      int_unlock(lock);
    }

    if (rxdma_handler[id]) {
      if (recv_dma_chan[id] != HAL_DMA_CHAN_NONE) {
        if (recv_dma_mode[id] == UART_RX_DMA_MODE_NORMAL) {
          xfer = hal_uart_stop_dma_recv(id);
          if (recv_dma_size[id] > xfer) {
            xfer = recv_dma_size[id] - xfer;
          } else {
            xfer = 0;
          }
        }
      }
      rxdma_handler[id](xfer, 0, status);
    }
  }
}

void hal_uart_irq_set_dma_handler(enum HAL_UART_ID_T id,
                                  HAL_UART_IRQ_RXDMA_HANDLER_T rxdma,
                                  HAL_UART_IRQ_TXDMA_HANDLER_T txdma) {
  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  rxdma_handler[id] = rxdma;
  txdma_handler[id] = txdma;
  irq_handler[id] = dma_mode_uart_irq_handler;
}

static void recv_dma_irq_handler(uint8_t chan, uint32_t remain_tsize,
                                 uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_UART_ID_T id;
  uint32_t xfer;
  uint32_t lock;
  union HAL_UART_IRQ_T status;

  lock = int_lock();
  for (id = HAL_UART_ID_0; id < HAL_UART_ID_QTY; id++) {
    if (recv_dma_chan[id] == chan) {
      if (recv_dma_mode[id] == UART_RX_DMA_MODE_NORMAL) {
        recv_dma_chan[id] = HAL_DMA_CHAN_NONE;
      }
      break;
    }
  }
  int_unlock(lock);

  if (id == HAL_UART_ID_QTY) {
    return;
  }

  if (recv_dma_mode[id] == UART_RX_DMA_MODE_NORMAL) {
    // Get remain xfer size
    xfer = hal_gpdma_get_sg_remain_size(chan);

    hal_gpdma_free_chan(chan);
  } else {
    xfer = 0;
    status.reg = 0;
  }

  if (rxdma_handler[id]) {
    if (recv_dma_mode[id] == UART_RX_DMA_MODE_NORMAL) {
      // Already get xfer size
      if (recv_dma_size[id] > xfer) {
        xfer = recv_dma_size[id] - xfer;
      } else {
        xfer = 0;
      }
    } else if (recv_dma_mode[id] == UART_RX_DMA_MODE_PINGPANG) {
      xfer = recv_dma_size[id] / 2;
    }
    rxdma_handler[id](xfer, error, status);
  }
}

static void recv_dma_start_callback(uint8_t chan) {
  enum HAL_UART_ID_T id;

  for (id = HAL_UART_ID_0; id < HAL_UART_ID_QTY; id++) {
    if (recv_dma_chan[id] == chan) {
      break;
    }
  }

  if (id == HAL_UART_ID_QTY) {
    return;
  }

  uart[id].base->UARTIMSC = recv_mask[id].reg;
}

static int fill_buf_list_dma_desc(struct HAL_DMA_DESC_T *desc, uint32_t cnt,
                                  struct HAL_DMA_CH_CFG_T *cfg,
                                  const struct HAL_UART_BUF_T *ubuf,
                                  uint32_t ucnt, uint32_t step) {
  enum HAL_DMA_RET_T ret;
  struct HAL_DMA_DESC_T *last_desc;
  int tc_irq;
  int i;
  int u;
  uint32_t remain;
  uint32_t dlen;

  last_desc = NULL;

  u = 0;
  remain = ubuf[0].len;

  for (i = 0, u = 0; i < cnt - 1; i++) {
    if (ubuf[u].loop_hdr && last_desc == NULL) {
      last_desc = &desc[i];
    }
    if (remain <= step) {
      dlen = remain;
      tc_irq = ubuf[u].irq;
    } else {
      dlen = step;
      tc_irq = 0;
    }
    cfg->src_tsize = dlen;
    ret = hal_gpdma_init_desc(&desc[i], cfg, &desc[i + 1], tc_irq);
    if (ret != HAL_DMA_OK) {
      return 1;
    }
    remain -= dlen;
    if (remain) {
      cfg->dst += dlen;
    } else {
      u++;
      if (u >= ucnt) {
        return 2;
      }
      cfg->dst = (uint32_t)ubuf[u].buf;
      remain = ubuf[u].len;
    }
  }

  cfg->src_tsize = remain;
  ret = hal_gpdma_init_desc(&desc[i], cfg, last_desc, ubuf[u].irq);
  if (ret != HAL_DMA_OK) {
    return 1;
  }

  return 0;
}

static int fill_dma_desc(struct HAL_DMA_DESC_T *desc, uint32_t cnt,
                         struct HAL_DMA_CH_CFG_T *cfg, uint32_t len,
                         enum UART_RX_DMA_MODE_T mode, uint32_t step) {
  enum HAL_DMA_RET_T ret;
  struct HAL_DMA_DESC_T *last_desc;
  int tc_irq;
  int i;

  for (i = 0; i < cnt - 1; i++) {
    cfg->src_tsize = step;
    tc_irq = 0;
    if (mode == UART_RX_DMA_MODE_PINGPANG) {
      tc_irq = (i == cnt / 2 - 1) ? 1 : 0;
    } else if (mode == UART_RX_DMA_MODE_STREAM) {
      tc_irq = 1;
    }
    ret = hal_gpdma_init_desc(&desc[i], cfg, &desc[i + 1], tc_irq);
    if (ret != HAL_DMA_OK) {
      return 1;
    }
    cfg->dst += step;
  }

  cfg->src_tsize = len - (step * i);
  last_desc = NULL;
  if (mode == UART_RX_DMA_MODE_PINGPANG || mode == UART_RX_DMA_MODE_STREAM) {
    last_desc = &desc[0];
  }
  ret = hal_gpdma_init_desc(&desc[i], cfg, last_desc, 1);
  if (ret != HAL_DMA_OK) {
    return 1;
  }

  return 0;
}

static int start_recv_dma_with_mask(enum HAL_UART_ID_T id,
                                    const struct HAL_UART_BUF_T *ubuf,
                                    uint32_t ucnt, struct HAL_DMA_DESC_T *desc,
                                    uint32_t *desc_cnt,
                                    const union HAL_UART_IRQ_T *mask,
                                    enum UART_RX_DMA_MODE_T mode,
                                    uint32_t step) {
  uint8_t *buf;
  uint32_t len;
  struct HAL_DMA_CH_CFG_T dma_cfg;
  enum HAL_DMA_RET_T ret;
  uint32_t lock;
  uint32_t cnt;
  uint32_t i;
  enum HAL_DMA_PERIPH_T periph;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  ASSERT(uart[id].irq != INVALID_IRQn, "DMA not supported on UART %d", id);
  ASSERT((uart[id].base->UARTDMACR & UARTDMACR_RXDMAE),
         "DMA not configured on UART %d", id);

  if (ucnt == 0) {
    return -21;
  }
  if (step > HAL_UART_DMA_TRANSFER_STEP || step == 0) {
    return -22;
  }

  buf = ubuf[0].buf;
  len = ubuf[0].len;

  if (buf == NULL) {
    return -23;
  }
  if (len == 0) {
    return -24;
  }

  if (0) {
  } else if (mode == UART_RX_DMA_MODE_NORMAL) {
    cnt = (len + step - 1) / step;
  } else if (mode == UART_RX_DMA_MODE_PINGPANG) {
    cnt = ((len / 2 + step - 1) / step) * 2;
    step = len / cnt;
    if (len % cnt != 0) {
      return -11;
    }
    if (step == 0) {
      return -12;
    }
  } else if (mode == UART_RX_DMA_MODE_STREAM) {
    cnt = (len + step - 1) / step;
    if (cnt == 1) {
      // cnt should >= 2
      cnt++;
    }
    step = (len + cnt - 1) / cnt;
    if (step == 0) {
      return -13;
    }
  } else if (mode == UART_RX_DMA_MODE_BUF_LIST) {
    cnt = 0;
    for (i = 0; i < ucnt; i++) {
      if (ubuf->buf == NULL) {
        return -14;
      }
      if (ubuf->len == 0) {
        return -15;
      }
      cnt += (ubuf->len + step - 1) / step;
    }
  } else {
    return -10;
  }

  // Return the required DMA descriptor count
  if (desc == NULL && desc_cnt) {
    *desc_cnt = (cnt == 1) ? 0 : cnt;
    return 0;
  }

  if (cnt > 1) {
    if (desc == NULL || desc_cnt == NULL) {
      return -1;
    }
    if (*desc_cnt < cnt) {
      return -2;
    }
  }
  if (desc_cnt) {
    *desc_cnt = (cnt == 1) ? 0 : cnt;
  }

  periph = uart[id].rx_periph;

  lock = int_lock();
  if (recv_dma_chan[id] != HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return 1;
  }
  recv_dma_chan[id] = hal_gpdma_get_chan(periph, HAL_DMA_HIGH_PRIO);
  if (recv_dma_chan[id] == HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return 2;
  }
  int_unlock(lock);

  recv_dma_mode[id] = mode;
  recv_dma_size[id] = len;

  memset(&dma_cfg, 0, sizeof(dma_cfg));
  dma_cfg.ch = recv_dma_chan[id];
  dma_cfg.dst = (uint32_t)buf;
  dma_cfg.dst_bsize = HAL_DMA_BSIZE_32;
  dma_cfg.dst_periph = 0; // useless
  dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
  dma_cfg.handler = recv_dma_irq_handler;
  dma_cfg.src = 0; // useless
  dma_cfg.src_bsize = HAL_DMA_BSIZE_8;
  dma_cfg.src_periph = periph;
  dma_cfg.src_tsize = len;
  dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
  dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
  dma_cfg.try_burst = 0;

  if (mask) {
    recv_mask[id] = *mask;
    dma_cfg.start_cb = recv_dma_start_callback;
  } else {
    union HAL_UART_IRQ_T irq_mask;

    irq_mask.reg = uart[id].base->UARTIMSC;
    ASSERT(irq_mask.RT == 0, err_recv_dma_api, __FUNCTION__);
  }

  // Activate DMA RT behaviour
  lock = int_lock();
  uart[id].base->UARTLCR_H |= UARTLCR_H_DMA_RT_EN;
  int_unlock(lock);

  if (cnt == 1) {
    ret = hal_gpdma_start(&dma_cfg);
  } else {
    if (mode == UART_RX_DMA_MODE_BUF_LIST) {
      ret = fill_buf_list_dma_desc(desc, cnt, &dma_cfg, ubuf, ucnt, step);
    } else {
      ret = fill_dma_desc(desc, cnt, &dma_cfg, len, mode, step);
    }
    if (ret) {
      goto _err_exit;
    }
    ret = hal_gpdma_sg_start(desc, &dma_cfg);
  }

  if (ret != HAL_DMA_OK) {
  _err_exit:
    // Restore the traditional RT behaviour
    lock = int_lock();
    uart[id].base->UARTLCR_H &= ~UARTLCR_H_DMA_RT_EN;
    int_unlock(lock);
    hal_gpdma_free_chan(recv_dma_chan[id]);
    recv_dma_chan[id] = HAL_DMA_CHAN_NONE;
    return 3;
  }

  return 0;
}

// Safe API to trigger receive timeout IRQ and DMA IRQ
int hal_uart_dma_recv_mask(enum HAL_UART_ID_T id, uint8_t *buf, uint32_t len,
                           struct HAL_DMA_DESC_T *desc, uint32_t *desc_cnt,
                           const union HAL_UART_IRQ_T *mask) {
  struct HAL_UART_BUF_T uart_buf;

  uart_buf.buf = buf;
  uart_buf.len = len;
  uart_buf.irq = false;
  uart_buf.loop_hdr = false;
  return start_recv_dma_with_mask(id, &uart_buf, 1, desc, desc_cnt, mask,
                                  UART_RX_DMA_MODE_NORMAL,
                                  HAL_UART_DMA_TRANSFER_STEP);
}

// Safe API to trigger receive timeout IRQ and DMA IRQ
int hal_uart_dma_recv_mask_pingpang(enum HAL_UART_ID_T id, uint8_t *buf,
                                    uint32_t len, struct HAL_DMA_DESC_T *desc,
                                    uint32_t *desc_cnt,
                                    const union HAL_UART_IRQ_T *mask,
                                    uint32_t step) {
  struct HAL_UART_BUF_T uart_buf;

  uart_buf.buf = buf;
  uart_buf.len = len;
  uart_buf.irq = false;
  uart_buf.loop_hdr = false;
  return start_recv_dma_with_mask(id, &uart_buf, 1, desc, desc_cnt, mask,
                                  UART_RX_DMA_MODE_PINGPANG, step);
}

// Safe API to trigger receive timeout IRQ and DMA IRQ
int hal_uart_dma_recv_mask_stream(enum HAL_UART_ID_T id, uint8_t *buf,
                                  uint32_t len, struct HAL_DMA_DESC_T *desc,
                                  uint32_t *desc_cnt,
                                  const union HAL_UART_IRQ_T *mask,
                                  uint32_t step) {
  struct HAL_UART_BUF_T uart_buf;

  uart_buf.buf = buf;
  uart_buf.len = len;
  uart_buf.irq = false;
  uart_buf.loop_hdr = false;
  return start_recv_dma_with_mask(id, &uart_buf, 1, desc, desc_cnt, mask,
                                  UART_RX_DMA_MODE_STREAM, step);
}

// Safe API to trigger receive timeout IRQ and DMA IRQ
int hal_uart_dma_recv_mask_buf_list(enum HAL_UART_ID_T id,
                                    const struct HAL_UART_BUF_T *ubuf,
                                    uint32_t ucnt, struct HAL_DMA_DESC_T *desc,
                                    uint32_t *desc_cnt,
                                    const union HAL_UART_IRQ_T *mask) {
  return start_recv_dma_with_mask(id, ubuf, ucnt, desc, desc_cnt, mask,
                                  UART_RX_DMA_MODE_BUF_LIST,
                                  HAL_UART_DMA_TRANSFER_STEP);
}

int hal_uart_dma_recv(enum HAL_UART_ID_T id, uint8_t *buf, uint32_t len,
                      struct HAL_DMA_DESC_T *desc, uint32_t *desc_cnt) {
  struct HAL_UART_BUF_T uart_buf;

  uart_buf.buf = buf;
  uart_buf.len = len;
  uart_buf.irq = false;
  uart_buf.loop_hdr = false;
  return start_recv_dma_with_mask(id, &uart_buf, 1, desc, desc_cnt, NULL,
                                  UART_RX_DMA_MODE_NORMAL,
                                  HAL_UART_DMA_TRANSFER_STEP);
}

int hal_uart_dma_recv_pingpang(enum HAL_UART_ID_T id, uint8_t *buf,
                               uint32_t len, struct HAL_DMA_DESC_T *desc,
                               uint32_t *desc_cnt) {
  struct HAL_UART_BUF_T uart_buf;

  uart_buf.buf = buf;
  uart_buf.len = len;
  uart_buf.irq = false;
  uart_buf.loop_hdr = false;
  return start_recv_dma_with_mask(id, &uart_buf, 1, desc, desc_cnt, NULL,
                                  UART_RX_DMA_MODE_PINGPANG,
                                  HAL_UART_DMA_TRANSFER_STEP_PINGPANG);
}

uint32_t hal_uart_get_dma_recv_addr(enum HAL_UART_ID_T id) {
  uint32_t lock;
  uint32_t addr;
  int i;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  addr = 0;

  lock = int_lock();
  if (recv_dma_chan[id] != HAL_DMA_CHAN_NONE) {
    for (i = 0; i < 2; i++) {
      addr = hal_dma_get_cur_dst_addr(recv_dma_chan[id]);
      if (addr) {
        break;
      }
    }
  }
  int_unlock(lock);

  return addr;
}

uint32_t hal_uart_stop_dma_recv(enum HAL_UART_ID_T id) {
  uint32_t remains;
  uint32_t lock;
  uint8_t chan;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  lock = int_lock();
  chan = recv_dma_chan[id];
  recv_dma_chan[id] = HAL_DMA_CHAN_NONE;
  // Restore the traditional RT behaviour
  uart[id].base->UARTLCR_H &= ~UARTLCR_H_DMA_RT_EN;
  int_unlock(lock);

  if (chan == HAL_DMA_CHAN_NONE) {
    return 0;
  }

  // Save the data in DMA FIFO
  hal_gpdma_stop(chan);
  remains = hal_gpdma_get_sg_remain_size(chan);
  hal_gpdma_free_chan(chan);

  return remains;
}

static void send_dma_irq_handler(uint8_t chan, uint32_t remain_tsize,
                                 uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_UART_ID_T id;
  uint32_t xfer;
  uint32_t lock;

  lock = int_lock();
  for (id = HAL_UART_ID_0; id < HAL_UART_ID_QTY; id++) {
    if (send_dma_chan[id] == chan) {
      send_dma_chan[id] = HAL_DMA_CHAN_NONE;
      break;
    }
  }
  int_unlock(lock);

  if (id == HAL_UART_ID_QTY) {
    return;
  }

  // Get remain xfer size
  xfer = hal_gpdma_get_sg_remain_size(chan);

  hal_gpdma_free_chan(chan);

  if (txdma_handler[id]) {
    // Get already xfer size
    if (send_dma_size[id] > xfer) {
      xfer = send_dma_size[id] - xfer;
    } else {
      xfer = 0;
    }
    txdma_handler[id](xfer, error);
  }
}

int hal_uart_dma_send(enum HAL_UART_ID_T id, const uint8_t *buf, uint32_t len,
                      struct HAL_DMA_DESC_T *desc, uint32_t *desc_cnt) {
  struct HAL_DMA_CH_CFG_T dma_cfg;
  enum HAL_DMA_RET_T ret;
  uint32_t lock;
  uint32_t cnt;
  uint32_t i;
  enum HAL_DMA_PERIPH_T periph;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);
  ASSERT(uart[id].irq != INVALID_IRQn, "DMA not supported on UART %d", id);
  ASSERT((uart[id].base->UARTDMACR & UARTDMACR_TXDMAE),
         "DMA not configured on UART %d", id);

  cnt = (len + HAL_UART_DMA_TRANSFER_STEP - 1) / HAL_UART_DMA_TRANSFER_STEP;

  // Return the required DMA descriptor count
  if (desc == NULL && desc_cnt) {
    *desc_cnt = (cnt == 1) ? 0 : cnt;
    return 0;
  }

  if (cnt == 0) {
    return 0;
  }
  if (cnt > 1) {
    if (desc == NULL || desc_cnt == NULL) {
      return -1;
    }
    if (*desc_cnt < cnt) {
      return -2;
    }
  }
  if (desc_cnt) {
    *desc_cnt = (cnt == 1) ? 0 : cnt;
  }

  periph = uart[id].tx_periph;

  lock = int_lock();
  if (send_dma_chan[id] != HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return 1;
  }
  send_dma_chan[id] = hal_gpdma_get_chan(periph, HAL_DMA_HIGH_PRIO);
  if (send_dma_chan[id] == HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return 2;
  }
  int_unlock(lock);

  send_dma_size[id] = len;

  memset(&dma_cfg, 0, sizeof(dma_cfg));
  dma_cfg.ch = send_dma_chan[id];
  dma_cfg.dst = 0; // useless
  dma_cfg.dst_bsize = HAL_DMA_BSIZE_8;
  dma_cfg.dst_periph = periph;
  dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
  dma_cfg.handler = send_dma_irq_handler;
  dma_cfg.src = (uint32_t)buf;
  dma_cfg.src_bsize = HAL_DMA_BSIZE_32;
  dma_cfg.src_periph = 0; // useless
  dma_cfg.src_tsize = len;
  dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
  dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
  dma_cfg.try_burst = 0;

  if (cnt == 1) {
    ret = hal_gpdma_start(&dma_cfg);
  } else {
    for (i = 0; i < cnt - 1; i++) {
      dma_cfg.src_tsize = HAL_UART_DMA_TRANSFER_STEP;
      ret = hal_gpdma_init_desc(&desc[i], &dma_cfg, &desc[i + 1], 0);
      if (ret != HAL_DMA_OK) {
        goto _err_exit;
      }
      dma_cfg.src += HAL_UART_DMA_TRANSFER_STEP;
    }
    dma_cfg.src_tsize = len - (HAL_UART_DMA_TRANSFER_STEP * i);
    ret = hal_gpdma_init_desc(&desc[i], &dma_cfg, NULL, 1);
    if (ret != HAL_DMA_OK) {
      goto _err_exit;
    }
    ret = hal_gpdma_sg_start(desc, &dma_cfg);
  }

  if (ret != HAL_DMA_OK) {
  _err_exit:
    hal_gpdma_free_chan(send_dma_chan[id]);
    send_dma_chan[id] = HAL_DMA_CHAN_NONE;
    return 3;
  }

  return 0;
}

uint32_t hal_uart_stop_dma_send(enum HAL_UART_ID_T id) {
  uint32_t remains;
  uint32_t lock;
  uint8_t chan;

  ASSERT(id < HAL_UART_ID_QTY, err_invalid_id, id);

  lock = int_lock();
  chan = send_dma_chan[id];
  send_dma_chan[id] = HAL_DMA_CHAN_NONE;
  int_unlock(lock);

  if (chan == HAL_DMA_CHAN_NONE) {
    return 0;
  }

  // Not to keep the data in DMA FIFO
  hal_gpdma_cancel(chan);
  remains = hal_gpdma_get_sg_remain_size(chan);
  hal_gpdma_free_chan(chan);

  return remains;
}

static void hal_uart_irq_handler(void) {
  enum HAL_UART_ID_T id;
  union HAL_UART_IRQ_T state;

  for (id = HAL_UART_ID_0; id < HAL_UART_ID_QTY; id++) {
    state.reg = uart[id].base->UARTMIS;

    if (state.reg) {
      uart[id].base->UARTICR = state.reg;

      if (irq_handler[id] != NULL) {
        irq_handler[id](id, state);
      }
    }
  }
}

// ========================================================================
// Test function

#include "stdarg.h"
#include "stdio.h"

#if !defined(DEBUG_PORT) || (DEBUG_PORT == 1)
#define UART_PRINTF_ID HAL_UART_ID_0
#else
#define UART_PRINTF_ID HAL_UART_ID_1
#endif

#ifndef TRACE_BAUD_RATE
#define TRACE_BAUD_RATE (921600)
#endif

int hal_uart_printf_init(void) {
  static const struct HAL_UART_CFG_T uart_cfg = {
      .parity = HAL_UART_PARITY_NONE,
      .stop = HAL_UART_STOP_BITS_1,
      .data = HAL_UART_DATA_BITS_8,
      .flow = HAL_UART_FLOW_CONTROL_NONE, // HAL_UART_FLOW_CONTROL_RTSCTS,
      .tx_level = HAL_UART_FIFO_LEVEL_1_2,
      .rx_level = HAL_UART_FIFO_LEVEL_1_4,
      .baud = TRACE_BAUD_RATE,
      .dma_rx = false,
      .dma_tx = false,
      .dma_rx_stop_on_err = false,
  };

  if (UART_PRINTF_ID == HAL_UART_ID_0) {
    hal_iomux_set_uart0();
  } else {
    hal_iomux_set_uart1();
  }

  return hal_uart_open(UART_PRINTF_ID, &uart_cfg);
}

void hal_uart_printf(const char *fmt, ...) {
  char buf[200];
  int ret;
  int i;
  va_list ap;

  va_start(ap, fmt);
  ret = vsnprintf(buf, sizeof(buf), fmt, ap);
#ifdef TRACE_CRLF
  if (ret + 2 < sizeof(buf)) {
    buf[ret++] = '\r';
  }
#endif
  if (ret + 1 < sizeof(buf)) {
    buf[ret++] = '\n';
  }
  // buf[ret] = 0;
  va_end(ap);

  if (ret > 0) {
    for (i = 0; i < ret; i++) {
      hal_uart_blocked_putc(UART_PRINTF_ID, buf[i]);
    }
  }
}

#endif // CHIP_HAS_UART
