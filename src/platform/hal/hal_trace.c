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
#if !(defined(DEBUG) || defined(REL_TRACE_ENABLE))
// Implement a local copy of dummy trace functions for library linking (which
// might be built with DEBUG enabled)
#define TRACE_FUNC_SPEC
#endif
#include "hal_trace.h"
#include "cmsis_nvic.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_bootmode.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_codec.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_location.h"
#include "hal_memsc.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#ifdef CORE_DUMP
#include "CrashCatcherApi.h"
#endif

extern const char sys_build_info[];
extern void nv_record_flash_flush(void);

#ifdef FAULT_DUMP
void hal_trace_fault_dump(const uint32_t *regs, const uint32_t *extra,
                          uint32_t extra_len);
#ifndef __ARM_ARCH_ISA_ARM
static void hal_trace_fault_handler(void);
#endif
#endif

#if !(defined(ROM_BUILD) || defined(PROGRAMMER))
#define ASSERT_MUTE_CODEC
#define CRASH_DUMP_ENABLE
#if !(defined(NO_TRACE_TIME_STAMP) || defined(AUDIO_DEBUG_V0_1_0))
#define TRACE_TIME_STAMP
#endif
#if (defined(DUMP_LOG_ENABLE) || defined(DUMP_CRASH_ENABLE))
#define TRACE_TO_APP
#endif
#ifdef CHIP_HAS_CP
#define CP_TRACE_ENABLE
#endif
#endif

#define TRACE_IDLE_OUTPUT 0

#ifndef TRACE_BAUD_RATE
#define TRACE_BAUD_RATE (921600)
#endif

#ifndef TRACE_BUF_SIZE
#ifdef AUDIO_DEBUG
#define TRACE_BUF_SIZE (6 * 1024)
#else
#define TRACE_BUF_SIZE (4 * 1024)
#endif
#endif

#define CRASH_BUF_SIZE 100
#define CRASH_BUF_ATTR ALIGNED(4) USED

#ifndef TRACE_STACK_DUMP_PREV_WORD
#define TRACE_STACK_DUMP_PREV_WORD 16
#endif
#ifndef TRACE_STACK_DUMP_WORD
#define TRACE_STACK_DUMP_WORD 32
#endif
#ifndef TRACE_BACKTRACE_NUM
#define TRACE_BACKTRACE_NUM 20
#endif
#ifndef TRACE_BACKTRACE_SEARCH_WORD
#define TRACE_BACKTRACE_SEARCH_WORD 1024
#endif

#define STACK_DUMP_CNT_PER_LEN 4
#define STACK_DUMP_CNT_PREV                                                    \
  ((TRACE_STACK_DUMP_PREV_WORD + STACK_DUMP_CNT_PER_LEN - 1) /                 \
   STACK_DUMP_CNT_PER_LEN * STACK_DUMP_CNT_PER_LEN)
#define STACK_DUMP_CNT                                                         \
  ((TRACE_STACK_DUMP_WORD + STACK_DUMP_CNT_PER_LEN - 1) /                      \
   STACK_DUMP_CNT_PER_LEN * STACK_DUMP_CNT_PER_LEN)

#define TRACE_FLUSH_TIMEOUT MS_TO_TICKS(2000)

#define TRACE_NEAR_FULL_THRESH 200

#define TRACE_CRLF

#ifdef TRACE_CRLF
#define NEW_LINE_STR "\r\n"
#else
#define NEW_LINE_STR "\n"
#endif

#define HAL_TRACE_ASSERT_ID 0xBE57AAAA
#define HAL_TRACE_EXCEPTION_ID 0xBE57EEEE

#define HAL_MEMSC_ID_TRACE HAL_MEMSC_ID_0

#define TRACE_BUF_LOC SYNC_FLAGS_LOC

struct ASSERT_INFO_T {
  uint32_t ID;
  uint32_t CPU_ID;
  const char *FILE;
  const char *FUNC;
  uint32_t LINE;
  const char *FMT;
  uint32_t R[15];
#ifndef __ARM_ARCH_ISA_ARM
  uint32_t MSP;
  uint32_t PSP;
  uint32_t CONTROL;
#ifdef __ARM_ARCH_8M_MAIN__
  uint32_t MSPLIM;
  uint32_t PSPLIM;
#endif
#endif
};

struct EXCEPTION_INFO_T {
  uint32_t ID;
  uint32_t CPU_ID;
  const uint32_t *REGS;
#ifdef __ARM_ARCH_ISA_ARM
  const uint32_t *extra;
  uint32_t extra_len;
#else
  uint32_t MSP;
  uint32_t PSP;
  uint8_t PRIMASK;
  uint8_t FAULTMASK;
  uint8_t BASEPRI;
  uint8_t CONTROL;
  uint32_t ICSR;
  uint32_t AIRCR;
  uint32_t SCR;
  uint32_t CCR;
  uint32_t SHCSR;
  uint32_t CFSR;
  uint32_t HFSR;
  uint32_t AFSR;
  uint32_t MMFAR;
  uint32_t BFAR;
#ifdef __ARM_ARCH_8M_MAIN__
  uint32_t MSPLIM;
  uint32_t PSPLIM;
#endif
#endif
};

static CRASH_BUF_ATTR char crash_buf[CRASH_BUF_SIZE];

STATIC_ASSERT(sizeof(crash_buf) >= sizeof(((struct ASSERT_INFO_T *)0)->R),
              "crash_buf too small to hold assert registers");

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))

struct HAL_TRACE_BUF_T {
  unsigned char buf[TRACE_BUF_SIZE];
  unsigned short wptr;
  unsigned short rptr;
#if (TRACE_IDLE_OUTPUT == 0)
  unsigned short sends[2];
#endif
  unsigned short discards;
  bool sending;
  bool in_trace;
  bool wrapped;
};

STATIC_ASSERT(TRACE_BUF_SIZE <
                  (1 << (8 * sizeof(((struct HAL_TRACE_BUF_T *)0)->wptr))),
              "TRACE_BUF_SIZE is too large to fit in wptr/rptr variable");

static const struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE, // HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
    .baud = TRACE_BAUD_RATE,
#ifdef HAL_TRACE_RX_ENABLE
    .dma_rx = true,
#else
    .dma_rx = false,
#endif
#if (TRACE_IDLE_OUTPUT == 0)
    .dma_tx = true,
#else
    .dma_tx = false,
#endif
    .dma_rx_stop_on_err = false,
};

#if (TRACE_IDLE_OUTPUT == 0)
static const enum HAL_DMA_PERIPH_T uart_periph[] = {
    HAL_GPDMA_UART0_TX,
#if (CHIP_HAS_UART > 1)
    HAL_GPDMA_UART1_TX,
#endif
#if (CHIP_HAS_UART > 2)
    HAL_GPDMA_UART2_TX,
#endif
};

static const struct HAL_UART_CFG_T uart_rx_enable_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE, // HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
    .baud = TRACE_BAUD_RATE,
    .dma_rx = true,

#if (TRACE_IDLE_OUTPUT == 0)
    .dma_tx = true,
#else
    .dma_tx = false,
#endif
    .dma_rx_stop_on_err = false,
};

static struct HAL_DMA_CH_CFG_T dma_cfg;
TRACE_BUF_LOC static struct HAL_DMA_DESC_T dma_desc[2];
#endif

static enum HAL_TRACE_TRANSPORT_T trace_transport = HAL_TRACE_TRANSPORT_QTY;
static enum HAL_UART_ID_T trace_uart;

TRACE_BUF_LOC
static struct HAL_TRACE_BUF_T trace;

POSSIBLY_UNUSED
static const char newline[] = NEW_LINE_STR;

static const char discards_prefix[] = NEW_LINE_STR "LOST ";
static const uint32_t max_discards = 99999;
// 5 digits + "\r\n" = 7 chars
static char discards_buf[sizeof(discards_prefix) - 1 + 7];
static const unsigned char discards_digit_start = sizeof(discards_prefix) - 1;

static bool crash_dump_onprocess = false;

#ifdef CRASH_DUMP_ENABLE
static HAL_TRACE_CRASH_DUMP_CB_T
    crash_dump_cb_list[HAL_TRACE_CRASH_DUMP_MODULE_END];
static bool crash_handling;
#ifdef TRACE_TO_APP
static HAL_TRACE_APP_NOTIFY_T app_notify_cb = NULL;
static HAL_TRACE_APP_OUTPUT_T app_output_cb = NULL;
static HAL_TRACE_APP_OUTPUT_T app_crash_custom_cb = NULL;
static bool app_output_enabled =
#if defined(DUMP_LOG_ENABLE)
    true;
#else
    false;
#endif
#endif // TRACE_TO_APP
#endif // CRASH_DUMP_ENABLE
#ifdef CP_TRACE_ENABLE
static HAL_TRACE_APP_NOTIFY_T cp_notify_cb = NULL;
static HAL_TRACE_BUF_CTRL_T cp_buffer_cb = NULL;
#endif

#ifdef AUDIO_DEBUG_V0_1_0
static const char trace_head_buf[] = "[trace]";
#endif

static enum LOG_LEVEL_T trace_max_level;
static uint32_t trace_mod_map[(LOG_MODULE_QTY + 31) / 32];

static bool hal_trace_is_uart_transport(enum HAL_TRACE_TRANSPORT_T transport) {
  if (transport == HAL_TRACE_TRANSPORT_UART0
#if (CHIP_HAS_UART > 1)
      || transport == HAL_TRACE_TRANSPORT_UART1
#endif
#if (CHIP_HAS_UART > 2)
      || transport == HAL_TRACE_TRANSPORT_UART2
#endif
  ) {
    return true;
  }
  return false;
}

#if (TRACE_IDLE_OUTPUT == 0)

static void hal_trace_uart_send(void) {
  uint32_t wptr, rptr;
  uint32_t sends[2];
  uint32_t lock;

  lock = int_lock();

  wptr = trace.wptr;
  rptr = trace.rptr;

  // There is a race condition if we do not check s/w flag, but only check the
  // h/w status. [e.g., hal_gpdma_chan_busy(dma_cfg.ch)] When the DMA is done,
  // but DMA IRQ handler is still pending due to interrupt lock or higher
  // priority IRQ, it will have a chance to send the same content twice.
  if (!trace.sending && wptr != rptr) {
    trace.sending = true;

    sends[1] = 0;
    if (wptr > rptr) {
      sends[0] = wptr - rptr;
    } else {
      sends[0] = TRACE_BUF_SIZE - rptr;
      if (sends[0] <= HAL_DMA_MAX_DESC_XFER_SIZE) {
        sends[1] = wptr;
      }
    }
    if (sends[0] > HAL_DMA_MAX_DESC_XFER_SIZE) {
      sends[1] = sends[0] - HAL_DMA_MAX_DESC_XFER_SIZE;
      sends[0] = HAL_DMA_MAX_DESC_XFER_SIZE;
    }
    if (sends[1] > HAL_DMA_MAX_DESC_XFER_SIZE) {
      sends[1] = HAL_DMA_MAX_DESC_XFER_SIZE;
    }

    dma_cfg.src = (uint32_t)&trace.buf[rptr];
    if (sends[1] == 0) {
      dma_cfg.src_tsize = sends[0];
      hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, NULL, 1);
    } else {
      dma_cfg.src_tsize = sends[0];
      hal_gpdma_init_desc(&dma_desc[0], &dma_cfg, &dma_desc[1], 0);

      if (rptr + sends[0] < TRACE_BUF_SIZE) {
        dma_cfg.src = (uint32_t)&trace.buf[rptr + sends[0]];
      } else {
        dma_cfg.src = (uint32_t)&trace.buf[0];
      }
      dma_cfg.src_tsize = sends[1];
      hal_gpdma_init_desc(&dma_desc[1], &dma_cfg, NULL, 1);
    }
    trace.sends[0] = sends[0];
    trace.sends[1] = sends[1];

    hal_gpdma_sg_start(&dma_desc[0], &dma_cfg);
  }

  int_unlock(lock);
}

static void hal_trace_uart_xfer_done(uint8_t chan, uint32_t remain_tsize,
                                     uint32_t error,
                                     struct HAL_DMA_DESC_T *lli) {
  uint32_t sends[2];
  uint32_t lock;

  lock = int_lock();

  sends[0] = trace.sends[0];
  sends[1] = trace.sends[1];

  if (error) {
    if (lli || sends[1] == 0) {
      if (sends[0] > remain_tsize) {
        sends[0] -= remain_tsize;
      } else {
        sends[0] = 0;
      }
      sends[1] = 0;
    } else {
      if (sends[1] > remain_tsize) {
        sends[1] -= remain_tsize;
      } else {
        sends[1] = 0;
      }
    }
  }

  trace.rptr += sends[0] + sends[1];
  if (trace.rptr >= TRACE_BUF_SIZE) {
    trace.rptr -= TRACE_BUF_SIZE;
  }
  trace.sends[0] = 0;
  trace.sends[1] = 0;
  trace.sending = false;

  hal_trace_uart_send();

  int_unlock(lock);
}

static void hal_trace_send(void) {
#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    return;
  }
#endif

  if (hal_trace_is_uart_transport(trace_transport)) {
    hal_trace_uart_send();
  }
}

#else // TRACE_IDLE_OUTPUT

static void hal_trace_uart_idle_send(void) {
  int i;
  uint32_t lock;
  unsigned short wptr, rptr;

  lock = int_lock();
  wptr = trace.wptr;
  rptr = trace.rptr;
  int_unlock(lock);

  if (wptr == rptr) {
    return;
  }

  if (wptr < rptr) {
    for (i = rptr; i < TRACE_BUF_SIZE; i++) {
      hal_uart_blocked_putc(trace_uart, trace.buf[i]);
    }
    rptr = 0;
  }

  for (i = rptr; i < wptr; i++) {
    hal_uart_blocked_putc(trace_uart, trace.buf[i]);
  }

  trace.rptr = wptr;
  if (trace.rptr >= TRACE_BUF_SIZE) {
    trace.rptr -= TRACE_BUF_SIZE;
  }
}

void hal_trace_idle_send(void) {
  if (hal_trace_is_uart_transport(trace_transport)) {
    hal_trace_uart_idle_send();
  }
}

#endif // TRACE_IDLE_OUTPUT

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport) {
  int ret;

  crash_dump_onprocess = false;

#if (CHIP_HAS_UART > 1)
#ifdef FORCE_TRACE_UART1
  transport = HAL_TRACE_TRANSPORT_UART1;
#endif
#endif

#if (CHIP_HAS_UART > 2)
#ifdef FORCE_TRACE_UART2
  transport = HAL_TRACE_TRANSPORT_UART2;
#endif
#endif

  if (transport >= HAL_TRACE_TRANSPORT_QTY) {
    return 1;
  }
#ifdef CHIP_HAS_USB
  if (transport == HAL_TRACE_TRANSPORT_USB) {
    return 1;
  }
#endif

  if (trace_transport != HAL_TRACE_TRANSPORT_QTY) {
    return hal_trace_switch(transport);
  }

  trace_max_level = LOG_LEVEL_INFO;
  for (int i = 0; i < ARRAY_SIZE(trace_mod_map); i++) {
    trace_mod_map[i] = ~0;
  }

  memcpy(discards_buf, discards_prefix, discards_digit_start);

  trace.wptr = 0;
  trace.rptr = 0;
  trace.discards = 0;
  trace.sending = false;
  trace.in_trace = false;
  trace.wrapped = false;

  if (hal_trace_is_uart_transport(transport)) {
    trace_uart = HAL_UART_ID_0 + (transport - HAL_TRACE_TRANSPORT_UART0);
    ret = hal_uart_open(trace_uart, &uart_cfg);
    if (ret) {
      return ret;
    }

#if (TRACE_IDLE_OUTPUT == 0)
    trace.sends[0] = 0;
    trace.sends[1] = 0;

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    dma_cfg.dst = 0; // useless
    dma_cfg.dst_bsize = HAL_DMA_BSIZE_8;
    dma_cfg.dst_periph = uart_periph[trace_uart - HAL_UART_ID_0];
    dma_cfg.dst_width = HAL_DMA_WIDTH_BYTE;
    dma_cfg.handler = hal_trace_uart_xfer_done;
    dma_cfg.src_bsize = HAL_DMA_BSIZE_32;
    dma_cfg.src_periph = 0; // useless
    dma_cfg.src_width = HAL_DMA_WIDTH_BYTE;
    dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;
    dma_cfg.try_burst = 0;
    dma_cfg.ch = hal_gpdma_get_chan(dma_cfg.dst_periph, HAL_DMA_HIGH_PRIO);

    ASSERT(dma_cfg.ch != HAL_DMA_CHAN_NONE, "Failed to get DMA channel");
#endif
  }

#ifdef FAULT_DUMP
#ifdef __ARM_ARCH_ISA_ARM
  GIC_SetFaultDumpHandler(hal_trace_fault_dump);
#else
  NVIC_SetDefaultFaultHandler(hal_trace_fault_handler);
#endif
#endif

  trace_transport = transport;

#ifdef HAL_TRACE_RX_ENABLE
  hal_trace_rx_open();
#endif

  // Show build info
  static const char dbl_new_line[] = NEW_LINE_STR NEW_LINE_STR;
  hal_trace_output((unsigned char *)dbl_new_line, sizeof(dbl_new_line));
  hal_trace_output((unsigned char *)sys_build_info, strlen(sys_build_info) + 1);

  char buf[50];
  int len;
  len = snprintf(buf, sizeof(buf),
                 NEW_LINE_STR NEW_LINE_STR "------" NEW_LINE_STR
                                           "METAL_ID: %d" NEW_LINE_STR
                                           "------" NEW_LINE_STR NEW_LINE_STR,
                 hal_get_chip_metal_id());
  hal_trace_output((unsigned char *)buf, len + 1);

  return 0;
}

int hal_trace_switch(enum HAL_TRACE_TRANSPORT_T transport) {
  uint32_t POSSIBLY_UNUSED lock;
  int ret;

#if (CHIP_HAS_UART > 1)
#ifdef FORCE_TRACE_UART1
  transport = HAL_TRACE_TRANSPORT_UART1;
#endif
#endif

#if (CHIP_HAS_UART > 2)
#ifdef FORCE_TRACE_UART2
  transport = HAL_TRACE_TRANSPORT_UART2;
#endif
#endif

#ifdef CHIP_HAS_USB
  if (transport == HAL_TRACE_TRANSPORT_USB) {
    return 1;
  }
#endif
  if (transport >= HAL_TRACE_TRANSPORT_QTY) {
    return 1;
  }
  if (trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
    return 1;
  }
  if (trace_transport == transport) {
    return 0;
  }

  ret = 0;

#if (CHIP_HAS_UART > 1)

  lock = int_lock();

  if (hal_trace_is_uart_transport(trace_transport)) {
#if (TRACE_IDLE_OUTPUT == 0)
    if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
      hal_gpdma_cancel(dma_cfg.ch);
    }
#endif
    hal_uart_close(trace_uart);
  }

  if (hal_trace_is_uart_transport(transport)) {
    trace_uart = HAL_UART_ID_0 + (transport - HAL_TRACE_TRANSPORT_UART0);
#if (TRACE_IDLE_OUTPUT == 0)
    dma_cfg.dst_periph = uart_periph[trace_uart - HAL_UART_ID_0];
    trace.sends[0] = 0;
    trace.sends[1] = 0;
#endif
    ret = hal_uart_open(trace_uart, &uart_cfg);
    if (ret) {
#if (TRACE_IDLE_OUTPUT == 0)
      hal_gpdma_free_chan(dma_cfg.ch);
      dma_cfg.ch = HAL_DMA_CHAN_NONE;
#endif
      trace_transport = HAL_TRACE_TRANSPORT_QTY;
      goto _exit;
    }
  }

  trace.sending = false;

  trace_transport = transport;

_exit:
  int_unlock(lock);

#endif // CHIP_HAS_UART > 1

  return ret;
}

int hal_trace_close(void) {
  if (trace_transport >= HAL_TRACE_TRANSPORT_QTY) {
    goto _exit;
  }
#ifdef CHIP_HAS_USB
  if (trace_transport == HAL_TRACE_TRANSPORT_USB) {
    goto _exit;
  }
#endif

  if (hal_trace_is_uart_transport(trace_transport)) {
#if (TRACE_IDLE_OUTPUT == 0)
    if (dma_cfg.ch != HAL_DMA_CHAN_NONE) {
      hal_gpdma_cancel(dma_cfg.ch);
      hal_gpdma_free_chan(dma_cfg.ch);
      dma_cfg.ch = HAL_DMA_CHAN_NONE;
    }
#endif
    hal_uart_close(trace_uart);
  }

_exit:
  trace_transport = HAL_TRACE_TRANSPORT_QTY;

  return 0;
}

int hal_trace_enable_log_module(enum LOG_MODULE_T module) {
  if (module >= LOG_MODULE_QTY) {
    return 1;
  }

  trace_mod_map[module >> 5] |= (1 << (module & 0x1F));
  return 0;
}

int hal_trace_disable_log_module(enum LOG_MODULE_T module) {
  if (module >= LOG_MODULE_QTY) {
    return 1;
  }

  trace_mod_map[module >> 5] &= ~(1 << (module & 0x1F));
  return 0;
}

int hal_trace_set_log_module(const uint32_t *map, uint32_t word_cnt) {
  if (map == NULL || word_cnt == 0) {
    return 1;
  }

  if (word_cnt > ARRAY_SIZE(trace_mod_map)) {
    word_cnt = ARRAY_SIZE(trace_mod_map);
  }
  for (int i = 0; i < word_cnt; i++) {
    trace_mod_map[i] = map[i];
  }
  return 0;
}

int hal_trace_set_log_level(enum LOG_LEVEL_T level) {
  if (level >= LOG_LEVEL_QTY) {
    return 1;
  }

  trace_max_level = level;
  return 0;
}

void hal_trace_get_history_buffer(const unsigned char **buf1,
                                  unsigned int *len1,
                                  const unsigned char **buf2,
                                  unsigned int *len2) {
  uint32_t lock;
  uint8_t *b1, *b2;
  uint32_t l1, l2;

  b1 = b2 = NULL;
  l1 = l2 = 0;

  lock = int_lock();

  if (TRACE_BUF_SIZE > trace.wptr) {
    if (trace.wrapped) {
      b1 = &trace.buf[trace.wptr];
      l1 = TRACE_BUF_SIZE - trace.wptr;
      b2 = &trace.buf[0];
      l2 = trace.wptr;
    } else {
      b1 = &trace.buf[0];
      l1 = trace.wptr;
      b2 = NULL;
      l2 = 0;
    }
  }

  int_unlock(lock);

  if (buf1) {
    *buf1 = b1;
  }
  if (len1) {
    *len1 = l1;
  }
  if (buf2) {
    *buf2 = b2;
  }
  if (len2) {
    *len2 = l2;
  }
}

static void hal_trace_print_discards(uint32_t discards) {
  static const uint8_t base = 10;
  char digit[5], *d, *out;
  uint16_t len;
  uint16_t size;

  if (discards > max_discards) {
    discards = max_discards;
  }

  d = &digit[0];
  do {
    *d++ = (discards % base) + '0';
  } while (discards /= base);

  out = &discards_buf[discards_digit_start];
  do {
    *out++ = *--d;
  } while (d > &digit[0]);
#ifdef TRACE_CRLF
  *out++ = '\r';
#endif
  *out++ = '\n';
  len = out - &discards_buf[0];

  size = TRACE_BUF_SIZE - trace.wptr;
  if (size >= len) {
    size = len;
  }
  memcpy(&trace.buf[trace.wptr], &discards_buf[0], size);
  if (size < len) {
    memcpy(&trace.buf[0], &discards_buf[size], len - size);
  }
  trace.wptr += len;
  if (trace.wptr >= TRACE_BUF_SIZE) {
    trace.wptr -= TRACE_BUF_SIZE;
  }
}

#ifdef AUDIO_DEBUG_V0_1_0
static void hal_trace_print_head(void) {
  uint16_t len;
  uint16_t size;

  len = sizeof(trace_head_buf) - 1;

  size = TRACE_BUF_SIZE - trace.wptr;
  if (size >= len) {
    size = len;
  }
  memcpy(&trace.buf[trace.wptr], &trace_head_buf[0], size);
  if (size < len) {
    memcpy(&trace.buf[0], &trace_head_buf[size], len - size);
  }
  trace.wptr += len;
  if (trace.wptr >= TRACE_BUF_SIZE) {
    trace.wptr -= TRACE_BUF_SIZE;
  }
}
#endif

int hal_trace_output(const unsigned char *buf, unsigned int buf_len) {
  int ret;
  uint32_t lock;
  uint32_t avail;
  uint32_t out_len;
  uint16_t size;

  ret = 0;

  lock = int_lock();
#ifdef CP_TRACE_ENABLE
  while (hal_memsc_lock(HAL_MEMSC_ID_TRACE) == 0)
    ;
#endif

  // Avoid troubles when NMI occurs during trace
  if (!trace.in_trace) {
    trace.in_trace = true;

    if (trace.wptr >= trace.rptr) {
      avail = TRACE_BUF_SIZE - (trace.wptr - trace.rptr) - 1;
    } else {
      avail = (trace.rptr - trace.wptr) - 1;
    }

    out_len = buf_len;
#ifdef AUDIO_DEBUG_V0_1_0
    out_len += sizeof(trace_head_buf) - 1;
#endif
    if (trace.discards) {
      out_len += sizeof(discards_buf);
    }

    if (avail < out_len) {
      ret = 1;
      if (trace.discards < (1 << (sizeof(trace.discards) * 8)) - 1) {
        trace.discards++;
      }
#ifdef CP_TRACE_ENABLE
#if (TRACE_IDLE_OUTPUT == 0)
      hal_trace_send();
#endif
#endif
    } else {
#ifdef AUDIO_DEBUG_V0_1_0
      hal_trace_print_head();
#endif

      if (trace.discards) {
        hal_trace_print_discards(trace.discards);
        trace.discards = 0;
      }

      size = TRACE_BUF_SIZE - trace.wptr;
      if (size >= buf_len) {
        size = buf_len;
      }
      memcpy(&trace.buf[trace.wptr], &buf[0], size);
      if (size < buf_len) {
        memcpy(&trace.buf[0], &buf[size], buf_len - size);
      }
      trace.wptr += buf_len;
      if (trace.wptr >= TRACE_BUF_SIZE) {
        trace.wptr -= TRACE_BUF_SIZE;
        trace.wrapped = true;
      }
#if (TRACE_IDLE_OUTPUT == 0)
      hal_trace_send();
#endif
    }

#ifdef CP_TRACE_ENABLE
    if (get_cpu_id()) {
      if (cp_buffer_cb) {
        if (avail < out_len) {
          cp_buffer_cb(HAL_TRACE_BUF_STATE_FULL);
        } else if (avail - out_len < TRACE_NEAR_FULL_THRESH) {
          cp_buffer_cb(HAL_TRACE_BUF_STATE_NEAR_FULL);
        }
      }
    }
#endif

    trace.in_trace = false;

#ifdef CRASH_DUMP_ENABLE
#ifdef TRACE_TO_APP
    if (app_output_cb && app_output_enabled) {
      bool saved_output_state;

      saved_output_state = app_output_enabled;
      app_output_enabled = false;

      app_output_cb(buf, buf_len);

      app_output_enabled = saved_output_state;
    }
#endif
#endif
  }

#ifdef CP_TRACE_ENABLE
  hal_memsc_unlock(HAL_MEMSC_ID_TRACE);
#endif
  int_unlock(lock);

  return ret ? 0 : buf_len;
}
#ifdef USE_TRACE_ID
// define USE_CRC_CHECK
//#define LITE_VERSION

typedef struct {
  uint32_t crc : 6;
  uint32_t count : 4;
  uint32_t tskid : 5;
  uint32_t addr : 17; // 127 KB trace space support
} trace_info_t;

typedef struct {
  uint32_t crc : 8;
  uint32_t timestamp : 24; // 4 hours support
} trace_head_t;

typedef struct {
#ifndef LITE_VERSION
  trace_head_t trace_head;
#endif
  trace_info_t trace_info;
} __attribute__((packed)) LOG_DATA_T;

extern const char *unkonw_str;

extern uint32_t __trc_str_start__[];
extern uint32_t __trc_str_end__[];
uint8_t crc8(uint8_t *data, uint32_t length) {
  uint8_t i;
  uint8_t crc = 0; // Initial value
  while (length--) {
    crc ^= *data++; // crc ^= *data; data++;
    for (i = 0; i < 8; i++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}

uint8_t crc6(uint8_t *data, uint32_t length) {
  uint8_t i;
  uint8_t crc = 0; // Initial value
  while (length--) {
    crc ^= *data++; // crc ^= *data; data++;
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0x30; // 0x30 = (reverse 0x03)>>(8-6)
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}

static int hal_trace_format_id(uint32_t attr, char *buf, uint32_t size,
                               const char *fmt, va_list ap) {
  uint8_t num;
  unsigned int value[10];
  LOG_DATA_T trace;

  if (size < sizeof(trace) + sizeof(value)) {
    return -1;
  }

  num = GET_BITFIELD(attr, LOG_ATTR_ARG_NUM);
  if (num > 10) {
    num = 10;
  }
  for (int i = 0; i < num; i++) {
    value[i] = va_arg(ap, unsigned long);
  }

  // memset(buf, 0, size);

  trace.trace_info.count = num;
  trace.trace_info.addr =
      (uint32_t)fmt -
      (uint32_t)0xFFFC0000; //(uint32_t)fmt-(uint32_t)__trc_str_start__;
  trace.trace_info.tskid = osGetThreadIntId();
  trace.trace_info.crc = 0x2A;
#ifndef LITE_VERSION
  trace.trace_head.timestamp = TICKS_TO_MS(hal_sys_timer_get());
#ifdef USE_CRC_CHECK
  trace.trace_head.crc = crc8(((uint8_t *)&trace) + 1, 7);
#else
  trace.trace_head.crc = 0xBE;
#endif
#else
  trace.trace_info.crc = crc6(((uint8_t *)&trace) + 1, 3);
#endif
  memcpy(buf, &trace, sizeof(trace));
  if (num != 0) {
    memcpy(buf + sizeof(trace), value, 4 * num);
  }

  return sizeof(trace) + 4 * num;
}
#endif

static int hal_trace_print_time(enum LOG_LEVEL_T level,
                                enum LOG_MODULE_T module, char *buf,
                                unsigned int size) {
#ifdef TRACE_TIME_STAMP
  static const char level_ch[] = {
      'C', 'E', 'W', 'N', 'I', 'D', 'V', '-',
  };
  char ctx[10];
  int len;
  int i;
  const char *mod_name;

#ifdef CRASH_DUMP_ENABLE
  if (crash_handling) {
    return 0;
  }
#endif

  if (0) {
#ifdef CP_TRACE_ENABLE
  } else if (get_cpu_id()) {
    ctx[0] = ' ';
    ctx[1] = 'C';
    ctx[2] = 'P';
    ctx[3] = '\0';
#endif
  } else if (in_isr()) {
    len = snprintf(ctx, sizeof(ctx), "%2d", (int8_t)NVIC_GetCurrentActiveIRQ());
    if (len + 1 < ARRAY_SIZE(ctx)) {
      ctx[len] = 'E';
      ctx[len + 1] = '\0';
    } else {
      ctx[ARRAY_SIZE(ctx) - 2] = '.';
      ctx[ARRAY_SIZE(ctx) - 1] = '\0';
    }
  } else {
#ifdef RTOS
#ifdef KERNEL_RTX5
    /* const char *thread_name = osGetThreadName(); */
    /* snprintf(ctx, sizeof(ctx), "%.9s", thread_name ? (char *)thread_name :
     * "NULL"); */
    ctx[0] = ' ';
    ctx[1] = ' ';
    ctx[2] = 't';
    ctx[3] = '\0';
#else
    snprintf(ctx, sizeof(ctx), "%3d", osGetThreadIntId());
#endif
#else
    ctx[0] = ' ';
    ctx[1] = ' ';
    ctx[2] = '0';
    ctx[3] = '\0';
#endif
  }
  ctx[ARRAY_SIZE(ctx) - 1] = '\0';
  len = 0;
  len += snprintf(&buf[len], size - len, "%9u/",
                  (unsigned)TICKS_TO_MS(hal_sys_timer_get()));
  if (size > len + 2) {
    buf[len++] = level_ch[level];
    buf[len++] = '/';
  }
  if (size > len + 7) {
    mod_name = hal_trace_get_log_module_desc(module);
    for (i = 0; i < 6; i++) {
      if (mod_name[i] == '\0') {
        break;
      }
      buf[len++] = mod_name[i];
    }
    for (; i < 6; i++) {
      buf[len++] = ' ';
    }
    buf[len++] = '/';
  }
  len += snprintf(&buf[len], size - len, "%s | ", ctx);
  return len;
#else  // !TRACE_TIME_STAMP
  return 0;
#endif // !TRACE_TIME_STAMP
}

static inline int hal_trace_format_va(uint32_t attr, char *buf,
                                      unsigned int size, const char *fmt,
                                      va_list ap) {
  int len;

  len = vsnprintf(&buf[0], size, fmt, ap);
  if ((attr & LOG_ATTR_NO_LF) == 0) {
#ifdef TRACE_CRLF
    if (len + 2 < size) {
      buf[len++] = '\r';
    }
#endif
    if (len + 1 < size) {
      buf[len++] = '\n';
    }
  }
  // if (len < size) buf[len] = 0;

  return len;
}

static int hal_trace_printf_internal(uint32_t attr, const char *fmt,
                                     va_list ap) {
#ifdef USE_TRACE_ID
  char buf[60];
#else
  char buf[120];
#endif
  int len = 0;
  enum LOG_LEVEL_T level;
  enum LOG_MODULE_T module;

  level = GET_BITFIELD(attr, LOG_ATTR_LEVEL);
  module = GET_BITFIELD(attr, LOG_ATTR_MOD);

#ifdef CRASH_DUMP_ENABLE
  if (!crash_handling)
#endif
  {
    if (level > trace_max_level) {
      return 0;
    }
    if (level > LOG_LEVEL_CRITICAL &&
        (trace_mod_map[module >> 5] & (1 << (module & 0x1F))) == 0) {
      return 0;
    }
  }

#ifdef USE_TRACE_ID
  if ((attr & LOG_ATTR_NO_ID) ||
      (len = hal_trace_format_id(attr, buf, sizeof(buf), fmt, ap)) < 0)
#endif
  {
    len = 0;
    if ((attr & LOG_ATTR_NO_TS) == 0) {
      len += hal_trace_print_time(level, module, &buf[len], sizeof(buf) - len);
    }
    len += hal_trace_format_va(attr, &buf[len], sizeof(buf) - len, fmt, ap);
  }

  return hal_trace_output((unsigned char *)buf, len);
}

int hal_trace_printf(uint32_t attr, const char *fmt, ...) {
  int ret;
  va_list ap;

  if (attr & LOG_ATTR_IMM) {
    hal_trace_flush_buffer();
  }

  va_start(ap, fmt);
  ret = hal_trace_printf_internal(attr, fmt, ap);
  va_end(ap);

  if (attr & LOG_ATTR_IMM) {
    hal_trace_flush_buffer();
  }

  return ret;
}

int hal_trace_dump(const char *fmt, unsigned int size, unsigned int count,
                   const void *buffer) {
  char buf[255] = {0};
  int len = 0, n = 0, i = 0;

  switch (size) {
  case sizeof(uint32_t):
    while (i < count && len < sizeof(buf)) {
      len += snprintf(&buf[len], sizeof(buf) - len, fmt,
                      *(uint32_t *)((uint32_t *)buffer + i));
      i++;
    }
    break;
  case sizeof(uint16_t):
    while (i < count && len < sizeof(buf)) {
      len += snprintf(&buf[len], sizeof(buf) - len, fmt,
                      *(int16_t *)((int16_t *)buffer + i));
      i++;
    }
    break;
  case sizeof(uint8_t):
  default:
    while (i < count && len < sizeof(buf)) {
      len += snprintf(&buf[len], sizeof(buf) - len, fmt,
                      *(uint8_t *)((uint8_t *)buffer + i));
      i++;
    }
    break;
  }

#ifdef TRACE_CRLF
  if (len + 2 < sizeof(buf)) {
    buf[len++] = '\r';
  }
#endif
  if (len + 1 < sizeof(buf)) {
    buf[len++] = '\n';
  }

  n = hal_trace_output((unsigned char *)buf, len + 1);

  return n;
}

int hal_trace_busy(void) {
  union HAL_UART_FLAG_T flag;

  if (hal_trace_is_uart_transport(trace_transport)) {
    if (hal_uart_opened(trace_uart)) {
      flag = hal_uart_get_flag(trace_uart);
      return flag.BUSY;
    }
  }
  return 0;
}

int hal_trace_pause(void) {
  if (hal_trace_is_uart_transport(trace_transport)) {
    return hal_uart_pause(trace_uart);
  }
  return 1;
}

int hal_trace_continue(void) {
  if (hal_trace_is_uart_transport(trace_transport)) {
    return hal_uart_continue(trace_uart);
  }
  return 1;
}

int hal_trace_flush_buffer(void) {
  uint32_t lock;
  uint32_t time;
  int ret;
  enum HAL_DMA_RET_T dma_ret;

  if (!hal_trace_is_uart_transport(trace_transport)) {
    return -1;
  }

#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    if (cp_buffer_cb) {
      cp_buffer_cb(HAL_TRACE_BUF_STATE_FLUSH);
    }
    return 0;
  }
#endif

  hal_uart_continue(trace_uart);

  lock = int_lock();

  time = hal_sys_timer_get();
  while (trace.wptr != trace.rptr &&
         hal_sys_timer_get() - time < TRACE_FLUSH_TIMEOUT) {
#if (TRACE_IDLE_OUTPUT == 0)
    while (hal_gpdma_chan_busy(dma_cfg.ch))
      ;
    dma_ret = hal_gpdma_irq_run_chan(dma_cfg.ch);
    if (dma_ret != HAL_DMA_OK) {
      hal_trace_send();
    }
#else
    hal_trace_idle_send();
#endif
  }

  ret = (trace.wptr == trace.rptr) ? 0 : 1;

  int_unlock(lock);

  return ret;
}

uint32_t hal_trace_get_backtrace_addr(uint32_t addr) {
  if (!hal_trace_address_executable(addr)) {
    return 0;
  }

#ifndef __ARM_ARCH_ISA_ARM
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
  // BL Instruction
  // OFFSET: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00 15 14 13 12 11 10
  // 09 08 07 06 05 04 03 02 01 00 VALUE :  1  1  1  1  0  -  -  -  -  -  -  -
  // -  -  -  -  1  1  -  1  -  -  -  -  -  -  -  -  -  -  -  -

  // BLX Instruction
  // OFFSET: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
  // VALUE :  0  1  0  0  0  1  1  1  1  -  -  -  -  -  -  -

  uint16_t val;
  uint32_t new_addr;

  new_addr = (addr & ~1) - 2;

  val = *(uint16_t *)new_addr;
  if ((val & 0xFF80) == 0x4780) {
    // BLX
    return new_addr;
  } else if ((val & 0xD000) == 0xD000) {
    new_addr -= 2;
    val = *(uint16_t *)new_addr;
    if ((val & 0xF800) == 0xF000) {
      // BL
      return new_addr;
    }
  }
#else
#error "Only ARMv7-M/ARMv8-M function can be checked for BL/BLX instructions"
#endif
#endif

  return 0;
}

#ifndef __ARM_ARCH_ISA_ARM
void hal_trace_print_special_stack_registers(void) {
  int len;

  hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

  len =
      snprintf(crash_buf, sizeof(crash_buf), "MSP=%08X, PSP=%08X" NEW_LINE_STR,
               (unsigned)__get_MSP(), (unsigned)__get_PSP());
  hal_trace_output((unsigned char *)crash_buf, len);

#ifdef __ARM_ARCH_8M_MAIN__
  len = snprintf(crash_buf, sizeof(crash_buf),
                 "MSPLIM=%08X, PSPLIM=%08X" NEW_LINE_STR,
                 (unsigned)__get_MSPLIM(), (unsigned)__get_PSPLIM());
  hal_trace_output((unsigned char *)crash_buf, len);
#endif
}
#endif

void hal_trace_print_common_registers(const uint32_t *regs) {
  int len;
  int i;
  int index;

  hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);

  for (i = 0; i < 3; i++) {
    index = i * 4;
    len = snprintf(
        crash_buf, sizeof(crash_buf),
        "R%-2d=%08X, R%-2d=%08X, R%-2d=%08X, R%-2d=%08X" NEW_LINE_STR, index,
        (unsigned)regs[index], index + 1, (unsigned)regs[index + 1], index + 2,
        (unsigned)regs[index + 2], index + 3, (unsigned)regs[index + 3]);
    hal_trace_output((unsigned char *)crash_buf, len);
  }
  len = snprintf(crash_buf, sizeof(crash_buf),
                 "R12=%08X, SP =%08X, LR =%08X" NEW_LINE_STR,
                 (unsigned)regs[12], (unsigned)regs[13], (unsigned)regs[14]);
  hal_trace_output((unsigned char *)crash_buf, len);
}

void hal_trace_print_stack(uint32_t addr) {
  static const char stack_title[] = "Stack:" NEW_LINE_STR;
  int len = 0;
  int i;
  int pos;
  uint32_t *stack;

  addr &= ~3;
  if (!hal_trace_address_writable(addr)) {
    return;
  }

  hal_trace_output((const unsigned char *)newline, sizeof(newline) - 1);
  hal_trace_output((const unsigned char *)stack_title, sizeof(stack_title) - 1);

  stack = (uint32_t *)addr - STACK_DUMP_CNT_PREV;
  for (i = 0; i < (STACK_DUMP_CNT_PREV + STACK_DUMP_CNT); i++) {
    if (!hal_trace_address_writable((uint32_t)&stack[i])) {
      break;
    }
    pos = (i % STACK_DUMP_CNT_PER_LEN);
    if (pos == 0) {
      len = snprintf(crash_buf, sizeof(crash_buf), "%c %08X: %08X",
                     (i == STACK_DUMP_CNT_PREV) ? '*' : ' ',
                     (unsigned)&stack[i], (unsigned)stack[i]);
    } else {
      len += snprintf(crash_buf + len, sizeof(crash_buf) - len, " %08X",
                      (unsigned)stack[i]);
      if (pos == (STACK_DUMP_CNT_PER_LEN - 1)) {
#ifdef TRACE_CRLF
        if (len + 2 < sizeof(crash_buf)) {
          crash_buf[len++] = '\r';
        }
#endif
        if (len + 1 < sizeof(crash_buf)) {
          crash_buf[len++] = '\n';
        }
        hal_trace_output((unsigned char *)crash_buf, len);
        hal_trace_flush_buffer();
      }
    }
  }
}

void hal_trace_print_backtrace(uint32_t addr, uint32_t search_cnt,
                               uint32_t print_cnt) {
  static const char bt_title[] = "Possible Backtrace:" NEW_LINE_STR;
  int i, j;
  int len;
  uint32_t *stack;
  uint32_t call_addr;

  addr &= ~3;
  if (!hal_trace_address_writable(addr)) {
    return;
  }

  hal_trace_output((const unsigned char *)newline, sizeof(newline));
  hal_trace_output((const unsigned char *)bt_title, sizeof(bt_title));

  stack = (uint32_t *)addr;
  for (i = 0, j = 0; i < search_cnt && j < print_cnt; i++) {
    if (!hal_trace_address_writable((uint32_t)&stack[i])) {
      break;
    }
    call_addr = hal_trace_get_backtrace_addr(stack[i]);
    if (call_addr) {
      len = snprintf(crash_buf, sizeof(crash_buf), "%8X" NEW_LINE_STR,
                     (unsigned)call_addr);
      hal_trace_output((unsigned char *)crash_buf, len + 1);
      j++;
    }
  }
}

uint32_t hal_trace_get_baudrate(void) { return uart_cfg.baud; }

bool hal_trace_crash_dump_onprocess(void) { return crash_dump_onprocess; }

int hal_trace_crash_dump_register(enum HAL_TRACE_CRASH_DUMP_MODULE_T module,
                                  HAL_TRACE_CRASH_DUMP_CB_T cb) {
#ifdef CRASH_DUMP_ENABLE
  ASSERT(module < HAL_TRACE_CRASH_DUMP_MODULE_END, "%s module %d", __func__,
         module);
  crash_dump_cb_list[module] = cb;
#endif
  return 0;
}

#ifdef CRASH_DUMP_ENABLE
static void hal_trace_crash_dump_callback(void) {
  int i;

  crash_handling = true;

  for (i = 0; i < ARRAY_SIZE(crash_dump_cb_list); i++) {
    if (crash_dump_cb_list[i]) {
      crash_dump_cb_list[i]();
    }
  }
}
#endif

void hal_trace_app_register(HAL_TRACE_APP_NOTIFY_T notify_cb,
                            HAL_TRACE_APP_OUTPUT_T output_cb) {
#ifdef TRACE_TO_APP
  app_notify_cb = notify_cb;
  app_output_cb = output_cb;
#endif
}

void hal_trace_app_custom_register(HAL_TRACE_APP_NOTIFY_T notify_cb,
                                   HAL_TRACE_APP_OUTPUT_T output_cb,
                                   HAL_TRACE_APP_OUTPUT_T crash_custom_cb) {
#ifdef TRACE_TO_APP
  hal_trace_app_register(notify_cb, output_cb);
  app_crash_custom_cb = crash_custom_cb;
#endif
}

void hal_trace_cp_register(HAL_TRACE_APP_NOTIFY_T notify_cb,
                           HAL_TRACE_BUF_CTRL_T buf_cb) {
#ifdef CP_TRACE_ENABLE
  cp_notify_cb = notify_cb;
  cp_buffer_cb = buf_cb;
#endif
}

#else // !(DEBUG || REL_TRACE_ENABLE)

int hal_trace_open(enum HAL_TRACE_TRANSPORT_T transport) {
#ifdef FAULT_DUMP
#ifdef __ARM_ARCH_ISA_ARM
  GIC_SetFaultDumpHandler(hal_trace_fault_dump);
#else
  NVIC_SetDefaultFaultHandler(hal_trace_fault_handler);
#endif
#endif

  return 0;
}

#endif // !(DEBUG || REL_TRACE_ENABLE)

int hal_trace_open_cp(void) {
#ifdef CP_TRACE_ENABLE
#ifdef FAULT_DUMP
  NVIC_SetDefaultFaultHandler_cp(hal_trace_fault_handler);
#endif
#endif
  return 0;
}

int hal_trace_address_writable(uint32_t addr) {
  if (RAM_BASE < addr && addr < RAM_BASE + RAM_SIZE) {
    return 1;
  }
#ifdef PSRAM_BASE
  if (PSRAM_BASE < addr && addr < PSRAM_BASE + PSRAM_SIZE) {
    return 1;
  }
#endif
#ifdef PSRAM_NC_BASE
  if (PSRAM_NC_BASE < addr && addr < PSRAM_NC_BASE + PSRAM_SIZE) {
    return 1;
  }
#endif
#ifdef PSRAMUHS_BASE
  if (PSRAMUHS_BASE < addr && addr < PSRAMUHS_BASE + PSRAMUHS_SIZE) {
    return 1;
  }
#endif
#ifdef PSRAMUHS_NC_BASE
  if (PSRAMUHS_NC_BASE < addr && addr < PSRAMUHS_NC_BASE + PSRAMUHS_SIZE) {
    return 1;
  }
#endif
#ifdef RAMRET_BASE
  if (RAMRET_BASE < addr && addr < RAMRET_BASE + RAMRET_SIZE) {
    return 1;
  }
#endif
#ifdef RAMCP_BASE
  if (RAMCP_BASE < addr && addr < RAMCP_BASE + RAMCP_SIZE) {
    return 1;
  }
#endif
  return 0;
}

int hal_trace_address_executable(uint32_t addr) {
// Avoid reading out of valid memory region when parsing instruction content
#define X_ADDR_OFFSET 0x10

  // Check thumb code
  if ((addr & 1) == 0) {
    return 0;
  }
  // Check location
  if (RAMX_BASE + X_ADDR_OFFSET < addr && addr < RAMX_BASE + RAM_SIZE) {
    return 1;
  }
  if (FLASHX_BASE + X_ADDR_OFFSET < addr && addr < FLASHX_BASE + FLASH_SIZE) {
    return 1;
  }
#ifdef PSRAMX_BASE
  if (PSRAMX_BASE + X_ADDR_OFFSET < addr && addr < PSRAMX_BASE + PSRAM_SIZE) {
    return 1;
  }
#endif
#ifdef PSRAMUHSX_BASE
  if (PSRAMUHSX_BASE + X_ADDR_OFFSET < addr &&
      addr < PSRAMUHSX_BASE + PSRAMUHS_SIZE) {
    return 1;
  }
#endif
#ifdef RAMXRET_BASE
  if (RAMXRET_BASE < addr && addr < RAMXRET_BASE + RAMRET_SIZE) {
    return 1;
  }
#endif

//#define CHECK_ROM_CODE
#ifdef CHECK_ROM_CODE
#ifndef USED_ROM_SIZE
#define USED_ROM_SIZE ROM_SIZE
#endif
  if (ROM_BASE + (NVIC_USER_IRQ_OFFSET * 4) < addr &&
      addr < ROM_BASE + USED_ROM_SIZE) {
    return 1;
  }
#endif

#if 0
    if (FLASHX_NC_BASE < addr && addr < FLASHX_NC_BASE + FLASH_SIZE) {
        return 1;
    }
#ifdef PSRAMX_NC_BASE
    if (PSRAMX_NC_BASE < addr && addr < PSRAMX_NC_BASE + PSRAM_SIZE) {
        return 1;
    }
#endif
#ifdef PSRAMUHSX_NC_BASE
    if (PSRAMUHSX_NC_BASE < addr && addr < PSRAMUHSX_NC_BASE + PSRAMUHS_SIZE) {
        return 1;
    }
#endif
#endif
  return 0;
}

int hal_trace_address_readable(uint32_t addr) {
  if (hal_trace_address_writable(addr)) {
    return 1;
  }
  if (hal_trace_address_executable(addr)) {
    return 1;
  }
  if (FLASH_BASE < addr && addr < FLASH_BASE + FLASH_SIZE) {
    return 1;
  }
  if (FLASH_NC_BASE < addr && addr < FLASH_NC_BASE + FLASH_SIZE) {
    return 1;
  }
#ifdef PSRAM_NC_BASE
  if (PSRAM_NC_BASE < addr && addr < PSRAM_NC_BASE + PSRAM_SIZE) {
    return 1;
  }
#endif
#ifdef PSRAMUHS_NC_BASE
  if (PSRAMUHS_NC_BASE < addr && addr < PSRAMUHS_NC_BASE + PSRAMUHS_SIZE) {
    return 1;
  }
#endif
  return 0;
}

static void NORETURN hal_trace_crash_end(void) {
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
  hal_trace_flush_buffer();
  hal_sys_timer_delay(MS_TO_TICKS(5));
#endif

  // Tag failure for simulation environment
  hal_cmu_simu_fail();
#ifndef PROGRAMMER
#ifndef __BES_OTA_MODE__
  nv_record_flash_flush();
#endif
#endif
#ifdef CRASH_REBOOT

  hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT |
                      HAL_SW_BOOTMODE_REBOOT_FROM_CRASH);
  hal_cmu_sys_reboot();
#else
  hal_iomux_set_analog_i2c();
  hal_iomux_set_jtag();
  hal_cmu_jtag_clock_enable();
#endif

  SAFE_PROGRAM_STOP();
}

__STATIC_FORCEINLINE uint32_t get_cpu_id_tag(void) {
  uint32_t cpu_id;
#ifdef CP_TRACE_ENABLE
  cpu_id = get_cpu_id();
#elif defined(__ARM_ARCH_ISA_ARM)
  cpu_id = 0xAAAA0000 | (get_cpu_id() & 0xFFFF);
#else
  cpu_id = 0;
#endif
  return cpu_id;
}

static void NORETURN USED hal_trace_assert_dump_internal(ASSERT_DUMP_ARGS) {
  const uint32_t *p_regs;
  struct ASSERT_INFO_T info;
  int i;

  // MUST SAVE REGISTERS FIRST!
  p_regs = (const uint32_t *)crash_buf;
  for (i = 0; i < ARRAY_SIZE(info.R); i++) {
    info.R[i] = p_regs[i];
  }

  int_lock_global();

  info.ID = HAL_TRACE_ASSERT_ID;
  info.CPU_ID = get_cpu_id_tag();
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) &&                           \
    defined(ASSERT_SHOW_FILE_FUNC)
  info.FILE = file;
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FILE)
  info.FILE = scope;
#else
  info.FILE = NULL;
#endif
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) &&                           \
    defined(ASSERT_SHOW_FILE_FUNC)
  info.FUNC = func;
#elif (defined(DEBUG) || defined(REL_TRACE_ENABLE)) && defined(ASSERT_SHOW_FUNC)
  info.FUNC = scope;
#else
  info.FUNC = NULL;
#endif
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE)) &&                           \
    (defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE) ||            \
     defined(ASSERT_SHOW_FUNC))
  info.LINE = line;
#else
  info.LINE = 0;
#endif
  info.FMT = fmt;
#ifndef __ARM_ARCH_ISA_ARM
  info.MSP = __get_MSP();
  info.PSP = __get_PSP();
  info.CONTROL = __get_CONTROL();
#ifdef __ARM_ARCH_8M_MAIN__
  info.MSPLIM = __get_MSPLIM();
  info.PSPLIM = __get_PSPLIM();
#endif
#endif

  *(volatile uint32_t *)RAM_BASE = (uint32_t)&info;

#ifdef ASSERT_MUTE_CODEC
  bool crash_mute = true;
#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    // Forbid CP to access CODEC
    crash_mute = false;
  }
#endif
  if (crash_mute) {
    hal_codec_crash_mute();
  }
#endif

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))

  static const char POSSIBLY_UNUSED desc_file[] = "FILE    : ";
  static const char POSSIBLY_UNUSED desc_func[] = "FUNCTION: ";
  static const char POSSIBLY_UNUSED desc_line[] = "LINE    : ";
  int len;
  va_list ap;

#ifdef CRASH_DUMP_ENABLE
  bool full_dump = true;

#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    full_dump = false;
    if (cp_notify_cb) {
      cp_notify_cb(HAL_TRACE_STATE_CRASH_ASSERT_START);
    }
  }
#endif

  if (full_dump) {
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
      app_notify_cb(HAL_TRACE_STATE_CRASH_ASSERT_START);
    }
    app_output_enabled = true;
#endif

    crash_dump_onprocess = true;
    for (uint8_t i = 0; i < 10; i++) {
      REL_TRACE_IMM(0, "                                                       "
                       "                 ");
      REL_TRACE_IMM(0, "                                                       "
                       "                 " NEW_LINE_STR);
      hal_sys_timer_delay(MS_TO_TICKS(50));
    }
  }
#endif

  hal_trace_flush_buffer();

  hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_52M);

  len = hal_trace_print_time(LOG_LEVEL_CRITICAL, LOG_MODULE_NONE, &crash_buf[0],
                             sizeof(crash_buf));
  if (len > 0) {
    hal_trace_output((const unsigned char *)newline, sizeof(newline));
    hal_trace_output((unsigned char *)crash_buf, len);
  }
  len = snprintf(&crash_buf[0], sizeof(crash_buf),
                 NEW_LINE_STR "### ASSERT @ 0x%08X ###" NEW_LINE_STR,
                 (unsigned)info.R[14]);
  hal_trace_output((unsigned char *)crash_buf, len);

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE) ||             \
    defined(ASSERT_SHOW_FUNC)
  const char separate_line[] =
      "----------------------------------------" NEW_LINE_STR;
#ifdef ASSERT_SHOW_FILE
  const char *file = scope;
#elif defined(ASSERT_SHOW_FUNC)
  const char *func = scope;
#endif

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FILE)
  hal_trace_output((const unsigned char *)desc_file, sizeof(desc_file));
  hal_trace_output((const unsigned char *)file, strlen(file) + 1);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
#endif

#if defined(ASSERT_SHOW_FILE_FUNC) || defined(ASSERT_SHOW_FUNC)
  hal_trace_output((const unsigned char *)desc_func, sizeof(desc_func));
  hal_trace_output((const unsigned char *)func, strlen(func) + !);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
#endif

  hal_trace_output((const unsigned char *)desc_line, sizeof(desc_func));
  len = snprintf(crash_buf, sizeof(crash_buf), "%u", line);
  hal_trace_output((const unsigned char *)crash_buf, len);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  hal_trace_output((unsigned char *)separate_line, sizeof(separate_line));

  hal_trace_flush_buffer();
#endif

  va_start(ap, fmt);
  len = hal_trace_format_va(0, crash_buf, sizeof(crash_buf), fmt, ap);
  va_end(ap);

  hal_trace_output((unsigned char *)crash_buf, len);

  hal_trace_flush_buffer();

  hal_trace_print_common_registers(info.R);
  hal_trace_print_special_stack_registers();

  hal_trace_print_stack(info.R[13]);

  hal_trace_print_backtrace(info.R[13], TRACE_BACKTRACE_SEARCH_WORD,
                            TRACE_BACKTRACE_NUM);

  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  hal_trace_flush_buffer();

#ifdef CRASH_DUMP_ENABLE
  if (full_dump) {
    hal_trace_crash_dump_callback();
    hal_trace_flush_buffer();
    hal_sys_timer_delay(MS_TO_TICKS(5));

#ifdef CORE_DUMP
    AssertCatcher_Entry();
    hal_sys_timer_delay(MS_TO_TICKS(5));
#endif

#ifdef TRACE_TO_APP
    if (app_notify_cb) {
      app_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
#endif
  }

#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    if (cp_notify_cb) {
      cp_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
    SAFE_PROGRAM_STOP();
  }
#endif
#endif // CRASH_DUMP_ENABLE

#endif // DEBUG || REL_TRACE_ENABLE

  hal_trace_crash_end();
}

void NORETURN NAKED hal_trace_assert_dump(ASSERT_DUMP_ARGS) {
  asm volatile("subs sp, sp, #4*" TO_STRING(
      STACK_DUMP_CNT_PREV) ";"
                           ".cfi_def_cfa_offset 4*" TO_STRING(
                               STACK_DUMP_CNT_PREV) ";"
                                                    "push {r0, r1};"
                                                    "ldr r0, =crash_buf;"
                                                    "ldr r1, [sp];"
                                                    "str r1, [r0], 4;"
                                                    "ldr r1, [sp, 4];"
                                                    "str r1, [r0], 4;"
                                                    "stmia r0!, {r2-r12};"
                                                    "add r1, sp, "
                                                    "#4*(2+" TO_STRING(
                                                        STACK_DUMP_CNT_PREV) ")"
                                                                             ";"
                                                                             "s"
                                                                             "t"
                                                                             "r"
                                                                             " "
                                                                             "r"
                                                                             "1"
                                                                             ","
                                                                             " "
                                                                             "["
                                                                             "r"
                                                                             "0"
                                                                             "]"
                                                                             ","
                                                                             " "
                                                                             "4"
                                                                             ";"
                                                                             "s"
                                                                             "t"
                                                                             "r"
                                                                             " "
                                                                             "l"
                                                                             "r"
                                                                             ","
                                                                             " "
                                                                             "["
                                                                             "r"
                                                                             "0"
                                                                             "]"
                                                                             ","
                                                                             " "
                                                                             "4"
                                                                             ";"
                                                                             "p"
                                                                             "o"
                                                                             "p"
                                                                             " "
                                                                             "{"
                                                                             "r"
                                                                             "0"
                                                                             ","
                                                                             " "
                                                                             "r"
                                                                             "1"
                                                                             "}"
                                                                             ";"
                                                                             "b"
                                                                             "."
                                                                             "w"
                                                                             " "
                                                                             "h"
                                                                             "a"
                                                                             "l"
                                                                             "_"
                                                                             "t"
                                                                             "r"
                                                                             "a"
                                                                             "c"
                                                                             "e"
                                                                             "_"
                                                                             "a"
                                                                             "s"
                                                                             "s"
                                                                             "e"
                                                                             "r"
                                                                             "t"
                                                                             "_"
                                                                             "d"
                                                                             "u"
                                                                             "m"
                                                                             "p"
                                                                             "_"
                                                                             "i"
                                                                             "n"
                                                                             "t"
                                                                             "e"
                                                                             "r"
                                                                             "n"
                                                                             "a"
                                                                             "l"
                                                                             ";");
}

#ifdef FAULT_DUMP
static void hal_trace_fill_exception_info(struct EXCEPTION_INFO_T *info,
                                          const uint32_t *regs,
                                          const uint32_t *extra,
                                          uint32_t extra_len) {
  info->ID = HAL_TRACE_EXCEPTION_ID;
  info->CPU_ID = get_cpu_id_tag();
  info->REGS = regs;
#ifdef __ARM_ARCH_ISA_ARM
  info->extra = extra;
  info->extra_len = extra_len;
#else
  info->MSP = __get_MSP();
  info->PSP = __get_PSP();
  info->PRIMASK = regs[17];
  info->FAULTMASK = __get_FAULTMASK();
  info->BASEPRI = __get_BASEPRI();
  info->CONTROL = __get_CONTROL();
  info->ICSR = SCB->ICSR;
  info->AIRCR = SCB->AIRCR;
  info->SCR = SCB->SCR;
  info->CCR = SCB->CCR;
  info->SHCSR = SCB->SHCSR;
  info->CFSR = SCB->CFSR;
  info->HFSR = SCB->HFSR;
  info->AFSR = SCB->AFSR;
  info->MMFAR = SCB->MMFAR;
  info->BFAR = SCB->BFAR;
#ifdef __ARM_ARCH_8M_MAIN__
  info->MSPLIM = __get_MSPLIM();
  info->PSPLIM = __get_PSPLIM();
#endif
#endif
}

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
#ifdef __ARM_ARCH_ISA_ARM
static void hal_trace_print_fault_info_ca(const struct EXCEPTION_INFO_T *info) {
  const struct FAULT_REGS_T *fregs;
  const uint32_t *extra;
  // uint32_t extra_len;
  enum EXCEPTION_ID_T id;
  int len;
  uint32_t val;
  const char *desc;

  fregs = (const struct FAULT_REGS_T *)info->REGS;
  extra = info->extra;
  // extra_len = info->extra_len;

  id = (enum EXCEPTION_ID_T)extra[0];

  len = snprintf(crash_buf, sizeof(crash_buf), "PC =%08X",
                 (unsigned)fregs->r[15]);
  len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                  ", ExceptionNumber=%d" NEW_LINE_STR, id);
  hal_trace_output((unsigned char *)crash_buf, len);

  hal_trace_print_common_registers(fregs->r);

  len = snprintf(crash_buf, sizeof(crash_buf), "SPSR=%08X",
                 (unsigned)fregs->spsr);
  len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", APSR=%c%c%c%c%c",
                  (fregs->spsr & (1 << 31)) ? 'N' : 'n',
                  (fregs->spsr & (1 << 30)) ? 'Z' : 'z',
                  (fregs->spsr & (1 << 29)) ? 'C' : 'c',
                  (fregs->spsr & (1 << 28)) ? 'V' : 'v',
                  (fregs->spsr & (1 << 27)) ? 'Q' : 'q');
  len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", XC=%c%c%c%c%c",
                  (fregs->spsr & (1 << 9)) ? 'E' : 'e',
                  (fregs->spsr & (1 << 8)) ? 'A' : 'a',
                  (fregs->spsr & (1 << 7)) ? 'I' : 'i',
                  (fregs->spsr & (1 << 6)) ? 'F' : 'f',
                  (fregs->spsr & (1 << 5)) ? 'T' : 't');
  val = fregs->spsr & 0x1F;
  if (val == 0x10) {
    desc = "USR";
  } else if (val == 0x11) {
    desc = "FIQ";
  } else if (val == 0x12) {
    desc = "IRQ";
  } else if (val == 0x13) {
    desc = "SVC";
  } else if (val == 0x16) {
    desc = "MON";
  } else if (val == 0x17) {
    desc = "ABT";
  } else if (val == 0x1A) {
    desc = "HYP";
  } else if (val == 0x1B) {
    desc = "UND";
  } else if (val == 0x1F) {
    desc = "SYS";
  } else {
    desc = "UNKNOWN";
  }
  len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, ", MODE=%02X (%s)",
                  (unsigned)val, desc);
  hal_trace_output((unsigned char *)crash_buf, len);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  hal_trace_flush_buffer();

  if (id == EXCEPTION_UNDEF) {
  } else if (id == EXCEPTION_SVC) {
  } else if (id == EXCEPTION_PABT) {
  } else if (id == EXCEPTION_DABT) {
  } else {
  }
}
#else
static void hal_trace_print_fault_info_cm(const struct EXCEPTION_INFO_T *info) {
  const uint32_t *regs;
  int len;
  uint32_t val;
  uint32_t primask;

  regs = info->REGS;
  primask = regs[17];

  len = snprintf(crash_buf, sizeof(crash_buf), "PC =%08X", (unsigned)regs[15]);
  val = __get_IPSR();
  if (val == 0) {
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                    ", ThreadMode" NEW_LINE_STR);
  } else {
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                    ", ExceptionNumber=%d" NEW_LINE_STR, (int)val - 16);
  }
  hal_trace_output((unsigned char *)crash_buf, len);

  hal_trace_print_common_registers(regs);
  hal_trace_print_special_stack_registers();

  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  len = snprintf(
      crash_buf, sizeof(crash_buf),
      "PRIMASK=%02X, FAULTMASK=%02X, BASEPRI=%02X, CONTROL=%02X" NEW_LINE_STR,
      (unsigned)primask, (unsigned)__get_FAULTMASK(), (unsigned)__get_BASEPRI(),
      (unsigned)__get_CONTROL());
  hal_trace_output((unsigned char *)crash_buf, len);
  len = snprintf(crash_buf, sizeof(crash_buf), "XPSR=%08X", (unsigned)regs[16]);
  len += snprintf(
      &crash_buf[len], sizeof(crash_buf) - len, ", APSR=%c%c%c%c%c",
      (regs[16] & (1 << 31)) ? 'N' : 'n', (regs[16] & (1 << 30)) ? 'Z' : 'z',
      (regs[16] & (1 << 29)) ? 'C' : 'c', (regs[16] & (1 << 28)) ? 'V' : 'v',
      (regs[16] & (1 << 27)) ? 'Q' : 'q');
  val = regs[16] & 0xFF;
  len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                  ", EPSR=%08X, IPSR=%02X", (unsigned)(regs[16] & 0x0700FC00),
                  (unsigned)val);
  if (val == 0) {
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (NoException)");
  }
  hal_trace_output((unsigned char *)crash_buf, len);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  hal_trace_flush_buffer();

  len = snprintf(crash_buf, sizeof(crash_buf),
                 "ICSR =%08X, AIRCR=%08X, SCR  =%08X, CCR  =%08X" NEW_LINE_STR,
                 (unsigned)info->ICSR, (unsigned)info->AIRCR,
                 (unsigned)info->SCR, (unsigned)info->CCR);
  hal_trace_output((unsigned char *)crash_buf, len);

  len = snprintf(crash_buf, sizeof(crash_buf),
                 "SHCSR=%08X, CFSR =%08X, HFSR =%08X, AFSR =%08X" NEW_LINE_STR,
                 (unsigned)info->SHCSR, (unsigned)info->CFSR,
                 (unsigned)info->HFSR, (unsigned)info->AFSR);
  hal_trace_output((unsigned char *)crash_buf, len);

  len = snprintf(crash_buf, sizeof(crash_buf),
                 "MMFAR=%08X, BFAR =%08X" NEW_LINE_STR, (unsigned)info->MMFAR,
                 (unsigned)info->BFAR);
  hal_trace_output((unsigned char *)crash_buf, len);

  if (info->HFSR & (1 << 30)) {
    len = snprintf(crash_buf, sizeof(crash_buf),
                   "(Escalation HardFault)" NEW_LINE_STR);
    hal_trace_output((unsigned char *)crash_buf, len);
  }

  len = snprintf(crash_buf, sizeof(crash_buf), "FaultInfo :");
  if ((info->SHCSR & 0x3F) == 0) {
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (None)");
  } else {
    if (info->SHCSR & (1 << 0)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MemFault)");
    }
    if (info->SHCSR & (1 << 1)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BusFault)");
    }
#ifdef __ARM_ARCH_8M_MAIN__
    if (info->SHCSR & (1 << 2)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (HardFault)");
    }
#endif
    if (info->SHCSR & (1 << 3)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (UsageFault)");
    }
#ifdef __ARM_ARCH_8M_MAIN__
    if (info->SHCSR & (1 << 4)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (SecureFault)");
    }
    if (info->SHCSR & (1 << 5)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (NMI)");
    }
#endif
  }
  hal_trace_output((unsigned char *)crash_buf, len);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  len = snprintf(crash_buf, sizeof(crash_buf), "FaultCause:");
  if (info->CFSR == 0) {
    len += snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (None)");
  } else {
    if (info->CFSR & (1 << 0)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Instruction access violation)");
    }
    if (info->CFSR & (1 << 1)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Data access violation)");
    }
    if (info->CFSR & (1 << 3)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (MemFault on unstacking for a return from exception)");
    }
    if (info->CFSR & (1 << 4)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (MemFault on stacking for exception entry)");
    }
    if (info->CFSR & (1 << 5)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                   " (MemFault during floating-point lazy state preservation)");
    }
    if (info->CFSR & (1 << 7)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (MMFAR valid)");
    }
    if (len) {
      hal_trace_output((unsigned char *)crash_buf, len);
      hal_trace_flush_buffer();
      len = 0;
    }
    if (info->CFSR & (1 << 8)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Instruction bus error)");
    }
    if (info->CFSR & (1 << 9)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Precise data bus error)");
    }
#ifndef __ARM_ARCH_8M_MAIN__
    if (info->CFSR & (1 << 10)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Imprecise data bus error)");
    }
#endif
    if (info->CFSR & (1 << 11)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (BusFault on unstacking for a return from exception)");
    }
    if (info->CFSR & (1 << 12)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (BusFault on stacking for exception entry)");
    }
    if (info->CFSR & (1 << 13)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                   " (BusFault during floating-point lazy state preservation)");
    }
    if (info->CFSR & (1 << 15)) {
      len +=
          snprintf(&crash_buf[len], sizeof(crash_buf) - len, " (BFAR valid)");
    }
    if (len) {
      hal_trace_output((unsigned char *)crash_buf, len);
      hal_trace_flush_buffer();
      len = 0;
    }
    if (info->CFSR & (1 << 16)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Undefined instruction UsageFault)");
    }
    if (info->CFSR & (1 << 17)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Invalid state UsageFault)");
    }
    if (info->CFSR & (1 << 18)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Invalid PC load by EXC_RETURN UsageFault)");
    }
    if (info->CFSR & (1 << 19)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (No coprocessor UsageFault)");
    }
#ifdef __ARM_ARCH_8M_MAIN__
    if (info->CFSR & (1 << 20)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Stack overflow UsageFault)");
    }
#endif
    if (info->CFSR & (1 << 24)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Unaligned access UsageFault)");
    }
    if (info->CFSR & (1 << 25)) {
      len += snprintf(&crash_buf[len], sizeof(crash_buf) - len,
                      " (Divide by zero UsageFault)");
    }
  }
  hal_trace_output((unsigned char *)crash_buf, len);
  hal_trace_output((const unsigned char *)newline, sizeof(newline));
}
#endif

static void hal_trace_print_fault_info(const struct EXCEPTION_INFO_T *info) {
#ifdef __ARM_ARCH_ISA_ARM
  hal_trace_print_fault_info_ca(info);
#else
  hal_trace_print_fault_info_cm(info);
#endif
}
#endif // DEBUG || REL_TRACE_ENABLE

void hal_trace_fault_dump(const uint32_t *regs, const uint32_t *extra,
                          uint32_t extra_len) {
  struct EXCEPTION_INFO_T info;

  int_lock_global();

  hal_trace_fill_exception_info(&info, regs, extra, extra_len);

  *(volatile uint32_t *)RAM_BASE = (uint32_t)&info;

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))

  static const char title[] = NEW_LINE_STR "### EXCEPTION ###" NEW_LINE_STR;
  int len;

#ifdef CRASH_DUMP_ENABLE
  bool full_dump = true;

#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    full_dump = false;
    if (cp_notify_cb) {
      cp_notify_cb(HAL_TRACE_STATE_CRASH_FAULT_START);
    }
  }
#endif

  if (full_dump) {
#ifdef TRACE_TO_APP
    if (app_notify_cb) {
      app_notify_cb(HAL_TRACE_STATE_CRASH_FAULT_START);
    }
    if (app_crash_custom_cb == NULL) {
      app_output_enabled = true;
    }
#endif

    crash_dump_onprocess = true;
    for (uint8_t i = 0; i < 10; i++) {
      REL_TRACE_IMM(0, "                                                       "
                       "                 ");
      REL_TRACE_IMM(0, "                                                       "
                       "                 " NEW_LINE_STR);
      hal_sys_timer_delay(MS_TO_TICKS(50));
    }
  }
#endif

  hal_trace_flush_buffer();

  hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_52M);

  len = hal_trace_print_time(LOG_LEVEL_CRITICAL, LOG_MODULE_NONE, &crash_buf[0],
                             sizeof(crash_buf));
  if (len > 0) {
    hal_trace_output((const unsigned char *)newline, sizeof(newline));
    hal_trace_output((unsigned char *)crash_buf, len);
  }
  hal_trace_output((unsigned char *)title, sizeof(title));

  hal_trace_print_fault_info(&info);

  hal_trace_flush_buffer();

  hal_trace_print_stack(regs[13]);

  hal_trace_print_backtrace(regs[13], TRACE_BACKTRACE_SEARCH_WORD,
                            TRACE_BACKTRACE_NUM);

  hal_trace_output((const unsigned char *)newline, sizeof(newline));

  hal_trace_flush_buffer();

#ifdef CRASH_DUMP_ENABLE
  if (full_dump) {
    hal_trace_crash_dump_callback();
    hal_trace_flush_buffer();

#ifndef __ARM_ARCH_ISA_ARM
#ifdef TRACE_TO_APP
    // Crash-Dump "Lite-Version"
    if (app_crash_custom_cb) {
      app_crash_custom_cb((unsigned char *)regs,
                          CRASH_DUMP_REGISTERS_NUM_BYTES);
      stack = (uint32_t *)(__get_MSP() & ~3);
      app_crash_custom_cb((unsigned char *)stack,
                          ((CRASH_DUMP_STACK_NUM_BYTES) / 2));
      stack = (uint32_t *)(__get_PSP() & ~3);
      app_crash_custom_cb((unsigned char *)stack,
                          ((CRASH_DUMP_STACK_NUM_BYTES) / 2));
    }
#endif

#ifdef CORE_DUMP
    {
      static CrashCatcherExceptionRegisters eregs;

      eregs.msp = info.MSP;
      eregs.psp = info.PSP;
      eregs.exceptionPSR = regs[16] & 0x0700FE00;
      eregs.r4 = regs[4];
      eregs.r5 = regs[5];
      eregs.r6 = regs[6];
      eregs.r7 = regs[7];
      eregs.r8 = regs[8];
      eregs.r9 = regs[9];
      eregs.r10 = regs[10];
      eregs.r11 = regs[11];
      eregs.exceptionLR = regs[14];
      CrashCatcher_Entry(&eregs);
    }
#endif
#endif // !__ARM_ARCH_ISA_ARM

#ifdef TRACE_TO_APP
    if (app_notify_cb) {
      app_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
#endif
  }

#ifdef CP_TRACE_ENABLE
  if (get_cpu_id()) {
    if (cp_notify_cb) {
      cp_notify_cb(HAL_TRACE_STATE_CRASH_END);
    }
    SAFE_PROGRAM_STOP();
  }
#endif
#endif // CRASH_DUMP_ENABLE

#endif // DEBUG || REL_TRACE_ENABLE

  hal_trace_crash_end();
}

#ifndef __ARM_ARCH_ISA_ARM
static void NAKED hal_trace_fault_handler(void) {
  // TODO: Save FP registers (and check lazy Floating-point context
  // preservation)
  asm volatile(
      // Check EXC_RETURN.SPSEL (bit[2])
      "tst lr, #0x04;"
      "ite eq;"
      // Using MSP
      "mrseq r3, msp;"
      // Using PSP
      "mrsne r3, psp;"
      // Check EXC_RETURN.FType (bit[4])
      "tst lr, #0x10;"
      "ite eq;"
      // FPU context saved
      "moveq r1, #1;"
      // No FPU context
      "movne r1, #0;"
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
      "mov, r0, #0;"
      // -- Check EXC_RETURN.S (bit[6])
      "tst lr, #0x40;"
      "beq _done_sec_cntx;"
      // -- Check EXC_RETURN.ES (bit[0])
      "tst lr, #0x01;"
      "bne _done_sec_cntx;"
      // -- Check EXC_RETURN.DCRS (bit[5])
      "tst lr, #0x20;"
      "bne _done_sec_cntx;"
      "mov, r0, #1;"
      "push {r4-r11};"
      "add r3, #2*4;"
      "ldm r3!, {r4-r11};"
      "_done_sec_cntx:;"
      "push {r0};"
#endif
      // Make room for r0-r15,psr,primask
      "sub sp, #18*4;"
      ".cfi_def_cfa_offset 18*4;"
      // Save r4-r11
      "add r0, sp, #4*4;"
      "stm r0, {r4-r11};"
      ".cfi_offset r4, -14*4;"
      ".cfi_offset r5, -13*4;"
      ".cfi_offset r6, -12*4;"
      ".cfi_offset r7, -11*4;"
      ".cfi_offset r8, -10*4;"
      ".cfi_offset r9, -9*4;"
      ".cfi_offset r10, -8*4;"
      ".cfi_offset r11, -7*4;"
      // Save r0-r3
      "ldm r3, {r4-r7};"
      "stm sp, {r4-r7};"
      ".cfi_offset r0, -18*4;"
      ".cfi_offset r1, -17*4;"
      ".cfi_offset r2, -16*4;"
      ".cfi_offset r3, -15*4;"
      // Save r12
      "ldr r0, [r3, #4*4];"
      "str r0, [sp, #12*4];"
      ".cfi_offset r12, -6*4;"
      // Save sp
      "teq r1, 0;"
      "itt eq;"
      "addeq r0, r3, #8*4;"
      "beq _done_stack_frame;"
      "add r0, r3, #(8+18)*4;"
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
      // -- Check EXC_RETURN.S (bit[6])
      "tst lr, #0x40;"
      "beq _done_stack_frame;"
      // -- Check FPCCR_S.TS (bit[26])
      "ldr r4, =0xE000EF34;"
      "ldr r4, [r4];"
      "tst r4, #(1 << 26);"
      "it ne;"
      "addne r3, #16*4;"
#endif
      "_done_stack_frame:;"
      // -- Check RETPSR.SPREALIGN (bit[9])
      "ldr r4, [r3, #7*4];"
      "tst r4, #(1 << 9);"
      "it ne;"
      "addne r0, #4;"
      "str r0, [sp, #13*4];"
      // Save lr
      "ldr r0, [r3, #5*4];"
      "str r0, [sp, #14*4];"
      // Save pc
      "ldr r0, [r3, #6*4];"
      "str r0, [sp, #15*4];"
      // Save PSR
      "ldr r0, [r3, #7*4];"
      "str r0, [sp, #16*4];"
      // Save primask
      "mrs r0, primask;"
      "str r0, [sp, #17*4];"
      // Save current exception lr
      "mov r4, lr;"
      ".cfi_register lr, r4;"
      // Invoke the fault handler
      "mov r0, sp;"
      "ldr r2, =hal_trace_fault_dump;"
      "blx r2;"
      // Restore current exception lr
      "mov lr, r4;"
      // Restore r4-r7
      "add r0, sp, #4*4;"
      "ldm r0, {r4-r7};"
      "mov r0, r3;"
      // Restore sp
      "add sp, #18*4;"
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
      "pop {r0};"
      "cmp r0, #1;"
      "it eq;"
      "popeq {r4-r11};"
#endif
      "bx lr;"
      "");
}
#endif
#endif

#define HAL_TRACE_RX_HEAD_SIZE 4
#define HAL_TRACE_RX_NAME_SIZE 20
#define HAL_TRACE_RX_BUF_SIZE 1024
#define HAL_TRACE_RX_ROLE_NUM 6

#define HAL_TRACE_RX_HEAD '['
#define HAL_TRACE_RX_END ']'
#define HAL_TRACE_RX_SEPARATOR ','

// static struct HAL_DMA_CH_CFG_T dma_cfg_rx;
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
static struct HAL_DMA_DESC_T dma_desc_rx;
#endif

typedef struct {
  char *name;
  uint32_t len;
  uint8_t *buf;
} HAL_TRACE_RX_CFG_T;

typedef struct {
  char name[HAL_TRACE_RX_NAME_SIZE];
  HAL_TRACE_RX_CALLBACK_T callback;
} HAL_TRACE_RX_LIST_T;

typedef struct {
  uint32_t list_num;
  HAL_TRACE_RX_LIST_T list[HAL_TRACE_RX_ROLE_NUM];

  uint32_t rx_enable;
  uint32_t pos;
  uint8_t buf[HAL_TRACE_RX_BUF_SIZE];
} HAL_TRACE_RX_T;

#if defined(_AUTO_TEST_)
extern int auto_test_prase(uint8_t *cmd);
#endif

HAL_TRACE_RX_T hal_trace_rx;

int hal_trace_rx_dump_list(void) {
  for (int i = 0; i < HAL_TRACE_RX_ROLE_NUM; i++) {
    TRACE(2, "%d: %s", i, hal_trace_rx.list[i].name);
  }
  return 0;
}

int hal_trace_rx_is_in_list(const char *name) {
  for (int i = 0; i < HAL_TRACE_RX_ROLE_NUM; i++) {
    if (!strcmp(hal_trace_rx.list[i].name, name)) {
      return i;
    }
  }
  hal_trace_rx_dump_list();
  // TRACE(1,"%s", hal_trace_rx.list[0].name);
  // TRACE(1,"%s", name);
  // TRACE(1,"%d", strlen(hal_trace_rx.list[0].name));
  // TRACE(1,"%d", strlen(name));
  // TRACE(1,"%d", strcmp(hal_trace_rx.list[0].name, name));
  return -1;
}

int hal_trace_rx_add_item_to_list(const char *name,
                                  HAL_TRACE_RX_CALLBACK_T callback) {
  for (int i = 0; i < HAL_TRACE_RX_ROLE_NUM; i++) {
    if (hal_trace_rx.list[i].name[0] == 0) {
      memcpy(hal_trace_rx.list[i].name, name, strlen(name));
      hal_trace_rx.list[i].callback = callback;
      hal_trace_rx.list_num++;
      return 0;
    }
  }

  return 1;
}

int hal_trace_rx_del_item_to_list(int id) {
  memset(hal_trace_rx.list[id].name, 0, sizeof(hal_trace_rx.list[id].name));
  hal_trace_rx.list[id].callback = NULL;
  hal_trace_rx.list_num--;

  return 0;
}

int hal_trace_rx_register(const char *name, HAL_TRACE_RX_CALLBACK_T callback) {
  TRACE(2, "[%s] Add %s", __func__, name);
  if (hal_trace_rx_is_in_list(name) == -1) {
    hal_trace_rx_add_item_to_list(name, callback);
    return 0;
  } else {
    return 0;
  }
}

int hal_trace_rx_deregister(const char *name) {
  int id = 0;

  id = hal_trace_rx_is_in_list(name);

  if (id != -1) {
    hal_trace_rx_del_item_to_list(id);
    return 0;
  } else {
    return 1;
  }
}

#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
static int hal_trace_rx_reset(void) {

  hal_trace_rx.pos = 0;
  memset(hal_trace_rx.buf, 0, HAL_TRACE_RX_BUF_SIZE);

  return 0;
}
#endif
// [test,12,102.99]
static int hal_trace_rx_parse(int8_t *buf, HAL_TRACE_RX_CFG_T *cfg) {
  // TRACE(1,"[%s] Start...", __func__);
  int pos = 0;
  int len = 0;

  for (; pos < HAL_TRACE_RX_HEAD_SIZE; pos++) {
    if (buf[pos] == HAL_TRACE_RX_HEAD) {
      buf[pos] = 0;
      break;
    }
  }

  if (pos == HAL_TRACE_RX_HEAD_SIZE) {
    return 3;
  }

  pos++;

  cfg->name = (char *)(buf + pos);
  for (; pos < HAL_TRACE_RX_NAME_SIZE + HAL_TRACE_RX_HEAD_SIZE; pos++) {
    if (buf[pos] == HAL_TRACE_RX_SEPARATOR) {
      buf[pos] = 0;
      break;
    }
  }

  // TRACE(1,"Step1: %s", cfg->name);
  // TRACE(1,"%d", strlen(cfg->name));

  if (pos == HAL_TRACE_RX_NAME_SIZE) {
    return 1;
  }

  pos++;

  len = 0;
  cfg->buf = (uint8_t *)(buf + pos);
  for (; pos < HAL_TRACE_RX_BUF_SIZE; pos++) {
    if (buf[pos] == HAL_TRACE_RX_END) {
      buf[pos] = 0;
      break;
    }
    len++;
  }
  cfg->len = len;
  if (pos == HAL_TRACE_RX_BUF_SIZE) {
    return 2;
  }

  return 0;
}

#if defined(IBRT)
void app_ibrt_peripheral_automate_test(const char *ibrt_cmd, uint32_t cmd_len);
void app_ibrt_peripheral_perform_test(const char *ibrt_cmd);
#endif

int hal_trace_rx_process(uint8_t *buf, uint32_t len) {
  HAL_TRACE_RX_CFG_T cfg;
  int id = 0;
  int res = 0;

#if defined(IBRT)
  if (buf && strlen((char *)buf) >= 10 &&
      ((strncmp((char *)buf, "auto_test:", 10) == 0) ||
       (strncmp((char *)buf, "ibrt_test:", 10) == 0))) {
#ifdef BES_AUTOMATE_TEST
    app_ibrt_peripheral_automate_test((char *)(buf + 10), len - 10);
#else
    app_ibrt_peripheral_perform_test((char *)(buf + 10));
#endif
    return 0;
  }
#endif

  res = hal_trace_rx_parse((int8_t *)buf, &cfg);

  if (res) {
    TRACE(1, "ERROR: hal_trace_rx_parse %d", res);
    return 1;
  } else {
    // TRACE(1,"%s rx OK", cfg.name);
  }

  id = hal_trace_rx_is_in_list(cfg.name);

  if (id == -1) {
    TRACE(1, "%s is invalid", cfg.name);
    return -1;
  }

  if (hal_trace_rx.list[id].callback) {
    hal_trace_rx.list[id].callback(cfg.buf, cfg.len);
  } else {
    TRACE(1, "%s has not callback", hal_trace_rx.list[id].name);
  }

  return 0;
}
#if (defined(DEBUG) || defined(REL_TRACE_ENABLE))
void hal_trace_rx_start(void) {
  uint32_t desc_cnt = 1;
  union HAL_UART_IRQ_T mask;

  mask.reg = 0;
  mask.BE = 0;
  mask.FE = 0;
  mask.OE = 0;
  mask.PE = 0;
  mask.RT = 1;

  hal_uart_dma_recv_mask(trace_uart, hal_trace_rx.buf, HAL_TRACE_RX_BUF_SIZE,
                         &dma_desc_rx, &desc_cnt, &mask);
}

void hal_trace_rx_irq_handler(uint32_t xfer_size, int dma_error,
                              union HAL_UART_IRQ_T status) {
  int res;
  // TRACE(4,"[%s] %d, %d, %d", __func__, xfer_size, dma_error, status);

  if (xfer_size) {
    hal_trace_rx.buf[xfer_size] = 0;
#if defined(_AUTO_TEST_)
    res = auto_test_prase(hal_trace_rx.buf);
    if (res) {
      TRACE(2, "%s:auto_test_prase prase data error, err_code = %d", __func__,
            res);
    }
#else
    // TRACE(2,"[%s] RX = %s", __func__, hal_trace_rx.buf);
    res = hal_trace_rx_process(hal_trace_rx.buf, xfer_size);
    if (res) {
      TRACE(2, "%s:hal_trace_rx_process prase data error, err_code = %d",
            __func__, res);
    }
#endif
    hal_trace_rx_reset();
    hal_trace_rx_start();
  }
}

uint32_t app_test_callback(unsigned char *buf, uint32_t len) {
  TRACE(2, "[%s] len = %d", __func__, len);

  // Process string
  int num_int = 0;
  int num_float = 0.0;
  TRACE(2, "[%s] %s", __func__, buf);
  hal_trace_rx_parser((char *)buf, "%d,%d", &num_int, &num_float);

  TRACE(3, "[%s] %d:%d", __func__, num_int, num_float);

  return 0;
}

int hal_trace_rx_open() {
  hal_uart_irq_set_dma_handler(trace_uart, hal_trace_rx_irq_handler, NULL);
  hal_trace_rx_start();

  hal_trace_rx_register("test", (HAL_TRACE_RX_CALLBACK_T)app_test_callback);

  return 0;
}

int hal_trace_rx_reopen() {
  hal_uart_reopen(trace_uart, &uart_rx_enable_cfg);
  hal_trace_rx_open();

  return 0;
}
#endif
