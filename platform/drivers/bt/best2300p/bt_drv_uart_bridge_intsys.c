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
#include "bt_drv.h"
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "hal_chipid.h"
#include "hal_intersys.h"
#include "hal_iomux.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "heap_api.h"
#include "stdbool.h"
#include "string.h"

#define BT_UART_BRIDGE_BUF_LEN (1024)

static const struct HAL_UART_CFG_T uart_cfg = {
    .parity = HAL_UART_PARITY_NONE,
    .stop = HAL_UART_STOP_BITS_1,
    .data = HAL_UART_DATA_BITS_8,
    .flow = HAL_UART_FLOW_CONTROL_NONE,
    .tx_level = HAL_UART_FIFO_LEVEL_1_2,
    .rx_level = HAL_UART_FIFO_LEVEL_1_4,
    .baud = 115200,
    .dma_rx = true,
    .dma_tx = true,
    .dma_rx_stop_on_err = false,
};

#if (DEBUG_PORT == 1)
enum HAL_UART_ID_T br_uart = HAL_UART_ID_0;
#else
enum HAL_UART_ID_T br_uart = HAL_UART_ID_1;
#endif
enum HAL_INTERSYS_ID_T br_intersys = HAL_INTERSYS_ID_0;

static volatile bool uart_rx_done = false;
static volatile bool uart_tx_done = false;
static volatile bool bt_rx_done = false;
static volatile bool bt_tx_done = false;

static unsigned char *uart_rx_data_p;
static volatile unsigned int uart_rx_len = 0;

static unsigned char *bt_rx_data_p;
static volatile unsigned int bt_rx_len = 0;

static void uart_rx(uint32_t xfer_size, int dma_error,
                    union HAL_UART_IRQ_T status) {
  if (dma_error) {
    BT_DRV_TRACE(1, "uart_rx dma error: xfer_size=%d", xfer_size);
    uart_rx_len = 0;
  } else if (status.BE || status.FE || status.OE || status.PE) {
    BT_DRV_TRACE(2, "uart_rx uart error: xfer_size=%d, status=0x%08x",
                 xfer_size, status.reg);
    uart_rx_len = 0;
  } else {
    uart_rx_len = xfer_size;
  }
  uart_rx_done = true;
}

static void uart_tx(uint32_t xfer_size, int dma_error) {
  if (dma_error) {
    BT_DRV_TRACE(1, "uart_tx dma error: xfer_size=%d", xfer_size);
  }
  uart_tx_done = true;
}

#ifdef VCO_TEST_TOOL
static unsigned int bt_rx(const unsigned char *data, unsigned int len);

void vco_test_simu_bt_rx(void) {
  const unsigned char hci_bt_vco_test_event[] = {0x04, 0x0e, 0x04, 0x05,
                                                 0xaa, 0xfc, 0x00};
  bt_rx(hci_bt_vco_test_event, sizeof(hci_bt_vco_test_event));
}
#endif

static unsigned int bt_rx(const unsigned char *data, unsigned int len) {
  unsigned int processed = len;

  hal_intersys_stop_recv(br_intersys);
  if (bt_rx_len + processed > BT_UART_BRIDGE_BUF_LEN) {
    BT_DRV_TRACE(3, "bt_rx data too long: bt_rx_len=%d, len=%d, limit=%d",
                 bt_rx_len, processed, sizeof(bt_rx_data_p));
    processed = BT_UART_BRIDGE_BUF_LEN - bt_rx_len;
  }
  if (processed > 0) {
    memcpy(bt_rx_data_p + bt_rx_len, data, processed);
    if ((data[0] == 0x4) && (data[2] == 0xc) && (data[4] == 0x1) &&
        (data[5] == 0x10)) {
      *(uint8_t *)(bt_rx_data_p + bt_rx_len + 7) = 0x09;
      *(uint8_t *)(bt_rx_data_p + bt_rx_len + 10) = 0x09;
    }
    if (data[0] == 0x4 && data[1] == 0xe && data[4] == 0x3 && data[5] == 0xc) {
      memset((uint32_t *)BT_EM_ADDR_BASE, 0, BT_EM_SIZE / 4);
      *(uint32_t *)0xd02201e4 = 0;
    }
    BT_DRV_TRACE(0, "[TX]:");
    DUMP8("%02x ", bt_rx_data_p + bt_rx_len, processed);
    bt_rx_len += processed;
    bt_rx_done = true;
  }
  return len;
}

static void bt_tx(const unsigned char *data, unsigned int len) {
  bt_tx_done = true;
}

void btdrv_uart_bridge_loop(void) {
  int ret;
  union HAL_UART_IRQ_T mask;

  syspool_init();

  syspool_get_buff(&uart_rx_data_p, BT_UART_BRIDGE_BUF_LEN);
  syspool_get_buff(&bt_rx_data_p, BT_UART_BRIDGE_BUF_LEN);

  if (br_uart == HAL_UART_ID_0) {
    hal_iomux_set_uart0();
    hal_trace_switch(HAL_TRACE_TRANSPORT_UART1);
    hal_iomux_set_uart1();
  } else {
    hal_iomux_set_uart1();
    hal_trace_switch(HAL_TRACE_TRANSPORT_UART0);
    // hal_iomux_set_uart0();
  }

  ret = hal_uart_open(br_uart, &uart_cfg);
  if (ret) {
    BT_DRV_TRACE(0, "Failed to open uart");
    return;
  }

  hal_uart_irq_set_dma_handler(br_uart, uart_rx, uart_tx);
  mask.reg = 0;
  mask.BE = 1;
  mask.FE = 1;
  mask.OE = 1;
  mask.PE = 1;
  mask.RT = 1;
  hal_uart_dma_recv_mask(br_uart, uart_rx_data_p, BT_UART_BRIDGE_BUF_LEN, NULL,
                         NULL, &mask);

  ret =
      hal_intersys_open(br_intersys, HAL_INTERSYS_MSG_HCI, bt_rx, bt_tx, false);
  if (ret) {
    BT_DRV_TRACE(0, "Failed to open intersys");
    return;
  }
  hal_intersys_start_recv(br_intersys);
  BT_DRV_TRACE(0, "Enter bridge mode");

  while (1) {
    if (uart_rx_done) {
      uart_rx_done = false;
      if (uart_rx_len > 0) {
        BT_DRV_TRACE(0, "[RX]:");
        DUMP8("%02x ", uart_rx_data_p, uart_rx_len);
#ifdef LAURENT_ALGORITHM
        btdrv_ble_test_bridge_intsys_callback(uart_rx_data_p);
#endif

#ifdef VCO_TEST_TOOL
        if (btdrv_vco_test_bridge_intsys_callback(uart_rx_data_p)) {
          uart_rx_len = 0;
          continue;
        }
#endif
        hal_intersys_send(br_intersys, HAL_INTERSYS_MSG_HCI, uart_rx_data_p,
                          uart_rx_len);
      } else {
        bt_tx_done = true;
      }
    }
    if (uart_tx_done) {
      uart_tx_done = false;
      bt_rx_len = 0;
      hal_intersys_start_recv(br_intersys);
    }
    if (bt_rx_done) {
      bt_rx_done = false;
      if (bt_rx_len > 0) {
        hal_uart_dma_send(br_uart, bt_rx_data_p, bt_rx_len, NULL, NULL);
      } else {
        uart_tx_done = true;
      }
    }
    if (bt_tx_done) {
      bt_tx_done = false;
      hal_uart_dma_recv_mask(br_uart, uart_rx_data_p, BT_UART_BRIDGE_BUF_LEN,
                             NULL, NULL, &mask);
    }
#ifdef VCO_TEST_TOOL
    if (btdrv_get_vco_test_process_flag()) {
      btdrv_vco_test_process(btdrv_get_vco_test_process_flag());
      vco_test_simu_bt_rx();
    }
#endif
  }
}
