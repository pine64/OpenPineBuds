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
#include "hal_cmd.h"
#include "hal_iomux.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "string.h"

/*
|-----|-------|----------------------------------|
|     |  crc  |           Discription            |
|     | ----- | -------------------------------- |
|     |   -3  |      Can not find head name      |
|     | ----- | -------------------------------- |
|     |   -2  |  Can not find callback function  |
|     | ----- | -------------------------------- |
| res |   -1  |          Unknown issue           |
|     | ----- | -------------------------------- |
|     |   0   |                OK                |
|     | ----- | -------------------------------- |
|     |   1   |    Data length is mismatching    |
|     | ----- | -------------------------------- |
|     |   >1  |      Issue from application      |
|-----|-------|----------------------------------|
 */

#define HAL_CMD_PREFIX_SIZE 4
#define HAL_CMD_CRC_SIZE 4
#define HAL_CMD_NAME_SIZE 12
#define HAL_CMD_LEN_SIZE 4
#define HAL_CMD_DATA_MAX_SIZE 1024 * 3

// #define HAL_CMD_PREFIX_OFFSET           0
// #define HAL_CMD_TYPE_OFFSET     1
// #define HAL_CMD_SEQ_OFFSET      2
// #define HAL_CMD_LEN_OFFSET      3
// #define HAL_CMD_CMD_OFFSET      4
// #define HAL_CMD_DATA_OFFSET     5

// #define HAL_CMD_PREFIX          0xFE
// #define HAL_CMD_TYPE            0xA0

#define HAL_CMD_RX_BUF_SIZE                                                    \
  (HAL_CMD_PREFIX_SIZE + HAL_CMD_NAME_SIZE + HAL_CMD_CRC_SIZE +                \
   HAL_CMD_LEN_SIZE + HAL_CMD_DATA_MAX_SIZE)
#define HAL_CMD_TX_BUF_SIZE (100)

#define HAL_CMD_LIST_NUM 10

#ifdef USB_EQ_TUNING
#define STATIC
#else
#define STATIC static
#endif

#ifdef USB_EQ_TUNING
#ifdef __PC_CMD_UART__
#error "USB_EQ_TUNING can not be defined together with PC_CMD_UART"
#endif
#endif

typedef struct {
  uint32_t len;
  uint8_t data[HAL_CMD_TX_BUF_SIZE - 13];
} hal_cmd_res_payload_t;

typedef struct {
  int prefix;
  int crc;
  char name[HAL_CMD_NAME_SIZE];
} hal_cmd_res_t;

typedef struct {
  int prefix;
  int crc;
  char *name;
  uint32_t len;
  uint8_t *data;
} hal_cmd_cfg_t;

typedef struct {
  char name[HAL_CMD_NAME_SIZE];
  hal_cmd_callback_t callback;
} hal_cmd_list_t;

typedef struct {
  uint8_t uart_work;
  hal_cmd_rx_status_t rx_status;
  uint8_t cur_seq;

#ifdef __PC_CMD_UART__
  uint8_t rx_len;
#endif
  uint8_t tx_len;
#ifdef __PC_CMD_UART__
  uint8_t rx_buf[HAL_CMD_RX_BUF_SIZE];
#endif
  uint8_t tx_buf[HAL_CMD_TX_BUF_SIZE];

  hal_cmd_res_t res;
  hal_cmd_res_payload_t res_payload;

  uint32_t list_num;
  hal_cmd_list_t list[HAL_CMD_LIST_NUM];
} hal_cmd_t;

hal_cmd_t hal_cmd;
CMD_CALLBACK_HANDLER_T hal_cmd_callback = NULL;

#ifdef __PC_CMD_UART__

#define HAL_CMD_ID HAL_UART_ID_0

static const struct HAL_UART_CFG_T hal_cmd_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE, // HAL_UART_FLOW_CONTROL_RTSCTS,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_2,
    .baud = 115200,
    .dma_rx = true,
    .dma_tx = true,
    .dma_rx_stop_on_err = false,
};
#endif

#define HAL_CMD_TRACE TRACE

#ifdef __PC_CMD_UART__
STATIC int hal_cmd_list_process(uint8_t *buf);
#endif
static int hal_cmd_list_register(char *name, hal_cmd_callback_t callback);

#ifdef __PC_CMD_UART__
void hal_cmd_break_irq_handler(void) {
  HAL_CMD_TRACE(1, "%s", __func__);

  // gElCtx.sync_step = EL_SYNC_ERROR;
}

void hal_cmd_dma_rx_irq_handler(uint32_t xfer_size, int dma_error,
                                union HAL_UART_IRQ_T status) {
  //	uint8_t prefix	= gpElUartCtx->rx_buf[EL_PREFIX_OFFSET];
  //	uint8_t type	= gpElUartCtx->rx_buf[EL_TYPE_OFFSET];
  //	uint8_t len		= gpElUartCtx->rx_buf[EL_LEN_OFFSET];
  //	uint8_t cmd	= gpElUartCtx->rx_buf[EL_CMD_OFFSET];
  HAL_CMD_TRACE(5,
                "%s: xfer_size[%d], dma_error[%x], status[%x], rx_status[%d]",
                __func__, xfer_size, dma_error, status.reg, hal_cmd.rx_status);

  if (status.BE) {
    hal_cmd_break_irq_handler();
    return;
  }

  if (hal_cmd.rx_status != HAL_CMD_RX_START) {
    return;
  }

  if (dma_error || status.FE || status.OE || status.PE || status.BE) {
    // TODO:
    ;
  } else {
    // mask.RT = 1
    hal_cmd.rx_len = xfer_size;
    hal_cmd.rx_status = HAL_CMD_RX_DONE;
    hal_cmd_callback(hal_cmd.rx_status);
  }
}
#endif

int hal_cmd_init(void) {
#ifdef __PC_CMD_UART__
  HAL_CMD_TRACE(1, "[%s]", __func__);
  if (HAL_CMD_ID == HAL_UART_ID_0) {
    hal_iomux_set_uart0();
  } else if (HAL_CMD_ID == HAL_UART_ID_1) {
    hal_iomux_set_uart1();
  }

#endif

#ifdef USB_AUDIO_APP
  hal_cmd_set_callback(hal_cmd_run);
#endif

  return 0;
}

void hal_cmd_set_callback(CMD_CALLBACK_HANDLER_T handler) {
  hal_cmd_callback = handler;
}

#ifdef __PC_CMD_UART__
static union HAL_UART_IRQ_T mask;
#endif

int hal_cmd_open(void) {
#ifdef __PC_CMD_UART__
  int ret = -1;

  ret = hal_uart_open(HAL_CMD_ID, &hal_cmd_cfg);
  ASSERT(!ret, "!!%s: UART open failed (%d)!!", __func__, ret);

  hal_uart_irq_set_dma_handler(HAL_CMD_ID, hal_cmd_dma_rx_irq_handler, NULL);

  // Do not enable tx and rx interrupt, use dma
  mask.reg = 0;
  mask.BE = 1;
  mask.FE = 1;
  mask.OE = 1;
  mask.PE = 1;
  mask.RT = 1;
  hal_uart_irq_set_mask(HAL_CMD_ID, mask);

  hal_cmd.uart_work = 1;
  hal_cmd.rx_status = HAL_CMD_RX_STOP;
  hal_cmd_callback(hal_cmd.rx_status);
#endif

  return 0;
}

int hal_cmd_close(void) {
#ifdef __PC_CMD_UART__
  mask.reg = 0;
  hal_uart_irq_set_mask(HAL_CMD_ID, mask);
  hal_uart_irq_set_dma_handler(HAL_CMD_ID, NULL, NULL);
  hal_uart_close(HAL_CMD_ID);

  hal_cmd.uart_work = 0;
#endif

  return 0;
}

int hal_cmd_register(char *name, hal_cmd_callback_t callback) {
  int ret = -1;

  ASSERT(strlen(name) < HAL_CMD_NAME_SIZE,
         "[%s] strlen(%s) = %d >= HAL_CMD_NAME_SIZE", __func__, name,
         strlen(name));

  ret = hal_cmd_list_register(name, callback);

  return ret;
}

#ifdef __PC_CMD_UART__
static int hal_cmd_send(void) {
  int ret = -1;

  if (!hal_cmd.uart_work) {
    hal_cmd_open();
  }

  ret =
      hal_uart_dma_send(HAL_CMD_ID, hal_cmd.tx_buf, hal_cmd.tx_len, NULL, NULL);

  HAL_CMD_TRACE(4, "%s: %d - %d - %d", __func__, hal_cmd.tx_len,
                hal_cmd.uart_work, ret);

  return ret;
}

static int hal_cmd_rx_start(void) {
  int ret = -1;

  if (!hal_cmd.uart_work) {
    hal_cmd_open();
  }

  ret = hal_uart_dma_recv_mask(HAL_CMD_ID, hal_cmd.rx_buf, HAL_CMD_RX_BUF_SIZE,
                               NULL, NULL, &mask); // HAL_CMD_RX_BUF_SIZE

  ASSERT(!ret, "!!%s: UART recv failed (%d)!!", __func__, ret);

  hal_cmd.rx_status = HAL_CMD_RX_START;
  hal_cmd_callback(hal_cmd.rx_status);
  return ret;
}

static int hal_cmd_rx_process(void) {
  int ret = -1;

  HAL_CMD_TRACE(1, "[%s] start...", __func__);

  ret = hal_cmd_list_process(hal_cmd.rx_buf);

  hal_cmd.rx_status = HAL_CMD_RX_STOP;
  hal_cmd_callback(hal_cmd.rx_status);
  return ret;
}
#endif

void hal_cmd_set_res_playload(uint8_t *data, int len) {
  hal_cmd.res_payload.len = len;
  memcpy(hal_cmd.res_payload.data, data, len);
}

#ifdef __PC_CMD_UART__
static int hal_cmd_tx_process(void) {
  int ret = -1;

  HAL_CMD_TRACE(1, "[%s] start...", __func__);

#if 0
    // Test loop
    hal_cmd.tx_len = hal_cmd.rx_len;
    memcpy(hal_cmd.tx_buf, hal_cmd.rx_buf, hal_cmd.rx_len);
#endif

#if 1
  hal_cmd.tx_len = sizeof(hal_cmd.res);
  TRACE(5, "[%s] len : %d, %c, %d, %s", __func__, hal_cmd.tx_len,
        hal_cmd.res.prefix, hal_cmd.res.crc, hal_cmd.res.name);
  memcpy(hal_cmd.tx_buf, &hal_cmd.res, hal_cmd.tx_len);

  if (hal_cmd.res_payload.len) {
    memcpy(hal_cmd.tx_buf + hal_cmd.tx_len, &hal_cmd.res_payload.len,
           sizeof(hal_cmd.res_payload.len));
    hal_cmd.tx_len += sizeof(hal_cmd.res_payload.len);

    memcpy(hal_cmd.tx_buf + hal_cmd.tx_len, hal_cmd.res_payload.data,
           hal_cmd.res_payload.len);
    hal_cmd.tx_len += hal_cmd.res_payload.len;

    memset(&hal_cmd.res_payload, 0, sizeof(hal_cmd.res_payload));
  }
#else
  char send_string[] = "791,";
  hal_cmd.tx_len = sizeof(send_string);
  memcpy(hal_cmd.tx_buf, send_string, hal_cmd.tx_len);
#endif

  hal_cmd_send();

  return ret;
}
#endif

#ifdef USB_EQ_TUNING

void hal_cmd_tx_process(uint8_t **ppbuf, uint16_t *plen) {
  hal_cmd.tx_len = sizeof(hal_cmd.res);

  memcpy(hal_cmd.tx_buf, &hal_cmd.res, hal_cmd.tx_len);

  if (hal_cmd.res_payload.len) {
    memcpy(hal_cmd.tx_buf + hal_cmd.tx_len, &hal_cmd.res_payload.len,
           sizeof(hal_cmd.res_payload.len));
    hal_cmd.tx_len += sizeof(hal_cmd.res_payload.len);

    memcpy(hal_cmd.tx_buf + hal_cmd.tx_len, hal_cmd.res_payload.data,
           hal_cmd.res_payload.len);
    hal_cmd.tx_len += hal_cmd.res_payload.len;

    memset(&hal_cmd.res_payload, 0, sizeof(hal_cmd.res_payload));
  }

  *ppbuf = hal_cmd.tx_buf;
  *plen = hal_cmd.tx_len;
}

#endif

#ifdef __PC_CMD_UART__

#ifdef USB_AUDIO_APP
void hal_cmd_run(hal_cmd_rx_status_t status)
#else
int hal_cmd_run(hal_cmd_rx_status_t status)
#endif
{
  int ret = -1;

  // static uint32_t pre_time_ms = 0, curr_ticks = 0, curr_time_ms = 0;

  // curr_ticks = hal_sys_timer_get();
  // curr_time_ms = TICKS_TO_MS(curr_ticks);

  // if(curr_time_ms - pre_time_ms > 1000)
  // {
  //     HAL_CMD_TRACE(1,"[%s] start...", __func__);
  //     pre_time_ms = curr_time_ms;
  // }

  if (status == HAL_CMD_RX_DONE) {
    ret = hal_cmd_rx_process();

    if (ret) {
      hal_cmd.res.crc = ret;
    }

    ret = hal_cmd_tx_process();
  }

  if (status == HAL_CMD_RX_STOP) {
    ret = hal_cmd_rx_start();
  }

#ifndef USB_AUDIO_APP
  return ret;
#endif
}
#endif

// List process
static int hal_cmd_list_get_id(char *name) {
  for (int i = 0; i < hal_cmd.list_num; i++) {
    if (!strcmp(hal_cmd.list[i].name, name)) {
      return i;
    }
  }

  TRACE(2, "[%s] rx = %s", __func__, name);
  for (int i = 0; i < hal_cmd.list_num; i++) {
    TRACE(3, "[%s] list[%d] = %s", __func__, i, hal_cmd.list[i].name);
  }
  return -1;
}

static int hal_cmd_list_add(char *name, hal_cmd_callback_t callback) {
  if (hal_cmd.list_num < HAL_CMD_LIST_NUM) {
    memcpy(hal_cmd.list[hal_cmd.list_num].name, name, strlen(name));
    hal_cmd.list[hal_cmd.list_num].callback = callback;
    hal_cmd.list_num++;

    return 0;
  } else {
    return -1;
  }
}

static int hal_cmd_list_register(char *name, hal_cmd_callback_t callback) {
  int ret = -1;

  if (hal_cmd_list_get_id(name) == -1) {
    ret = hal_cmd_list_add(name, callback);
  } else {
    ret = -1;
  }

  return ret;
}

#if defined(USB_EQ_TUNING) || defined(__PC_CMD_UART__)
static int hal_cmd_list_parse(uint8_t *buf, hal_cmd_cfg_t *cfg) {
  cfg->prefix = *((uint32_t *)buf);
  HAL_CMD_TRACE(2, "[%s] PREFIX = %c", __func__, cfg->prefix);
  buf += HAL_CMD_PREFIX_SIZE;
  hal_cmd.res.prefix = cfg->prefix;

  cfg->crc = *((uint32_t *)buf);
  HAL_CMD_TRACE(2, "[%s] crc = %d", __func__, cfg->crc);
  buf += HAL_CMD_CRC_SIZE;
  hal_cmd.res.crc = cfg->crc;

  cfg->name = (char *)buf;
  HAL_CMD_TRACE(2, "[%s] NAME = %s", __func__, cfg->name);
  buf += HAL_CMD_NAME_SIZE;
  memcpy(hal_cmd.res.name, cfg->name, HAL_CMD_NAME_SIZE);

  cfg->len = *((uint32_t *)buf);
  HAL_CMD_TRACE(2, "[%s] LEN = %d", __func__, cfg->len);
  buf += HAL_CMD_LEN_SIZE;

  cfg->data = buf;

  return 0;
}

STATIC int hal_cmd_list_process(uint8_t *buf) {
  int ret = -1;
  int id = 0;
  hal_cmd_cfg_t cfg;

  hal_cmd_list_parse(buf, &cfg);

  id = hal_cmd_list_get_id(cfg.name);

  if (id == -1) {
    TRACE(2, "[%s] %s is invalid", __func__, cfg.name);
    return -2;
  }

  if (hal_cmd.list[id].callback) {
    ret = hal_cmd.list[id].callback(cfg.data, cfg.len);
  } else {
    TRACE(2, "[%s] %s has not callback", __func__, hal_cmd.list[id].name);
    ret = -3;
  }

  return ret;
}
#endif
