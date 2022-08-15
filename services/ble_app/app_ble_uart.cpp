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
//#include "mbed.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hal_uart.h"
#include "hal_trace.h"

#define APP_BLE_UART_BUFF_SIZE  50  //16

enum HAL_UART_ID_T app_ble_uart_id = HAL_UART_ID_0;

extern "C" {
typedef void (*GATT_UART_HANDLER_T)(uint8_t *buf, uint32_t len);
extern void gatt_uart_register_write_request_cb(GATT_UART_HANDLER_T handler);
extern int gatt_uart_write(uint8_t *buffer, uint32_t length);
extern int gatt_loop_write(uint8_t *buffer, uint32_t length);
}

static uint32_t app_ble_debug_cnt = 0;

void app_ble_send_data(uint8_t *buffer, uint32_t length)
{
    TRACE(1,"[%s]", __func__);
    gatt_uart_write(buffer, length);
//    gatt_loop_write(buffer, length);
}

typedef struct
{
    volatile bool rx_done;
    volatile bool tx_done;
    uint8_t rx_data[APP_BLE_UART_BUFF_SIZE];
    uint32_t rx_len;
} app_ble_uart_t;

app_ble_uart_t app_ble_uart;

static void uart_rx(uint32_t xfer_size, int dma_error, union HAL_UART_IRQ_T status)
{
    app_ble_debug_cnt = 4;
    if (dma_error) {
//        TRACE(1,"uart_rx dma error: xfer_size=%d", xfer_size);
        app_ble_debug_cnt = 0xfe;
        app_ble_uart.rx_len = 0;
    } else if (status.BE || status.FE || status.OE || status.PE) {
//        TRACE(2,"uart_rx uart error: xfer_size=%d, status=0x%08x", xfer_size, status.reg);
        app_ble_debug_cnt = 0xfd;
        app_ble_uart.rx_len = 0;
    } else {
        app_ble_debug_cnt = 0xfc;
        app_ble_uart.rx_len = xfer_size;
//        hal_uart_dma_recv(app_ble_uart_id, app_ble_uart.rx_data, app_ble_uart.rx_len - 5, NULL, NULL);
//        gatt_uart_write(app_ble_uart.rx_data, app_ble_uart.rx_len - 5);

//        hal_uart_dma_recv(app_ble_uart_id, app_ble_uart.rx_data, 5, NULL, NULL);
//        gatt_uart_write(app_ble_uart.rx_data, 5);

        hal_uart_dma_recv(app_ble_uart_id, app_ble_uart.rx_data, sizeof(app_ble_uart.rx_data), NULL, NULL);
//        //hal_uart_dma_send(app_ble_uart_id, app_ble_uart.rx_data, app_ble_uart.rx_len, NULL, NULL);
        gatt_uart_write(app_ble_uart.rx_data, app_ble_uart.rx_len);
    }

    app_ble_uart.rx_done = true;
}

static void uart_tx(uint32_t xfer_size, int dma_error)
{
    if (dma_error)
    {
//        TRACE(1,"uart_tx dma error: xfer_size=%d", xfer_size);
        app_ble_debug_cnt = 0xfb;
    }
    app_ble_uart.tx_done = true;
}

void app_ble_uart_write_request_cb(uint8_t *buf, uint32_t len)
{
    //send data over uart.
   hal_uart_dma_send(app_ble_uart_id, buf, len, NULL, NULL);
    //DUMP8("%x ", buf, len);

}

uint32_t app_ble_uart_open()
{
#if 1
    int ret;
    union HAL_UART_IRQ_T mask;

    struct HAL_UART_CFG_T uart_cfg;

    uart_cfg.parity = HAL_UART_PARITY_NONE,
    uart_cfg.stop = HAL_UART_STOP_BITS_1,
    uart_cfg.data = HAL_UART_DATA_BITS_8,
    uart_cfg.flow = HAL_UART_FLOW_CONTROL_NONE,
    uart_cfg.tx_level = HAL_UART_FIFO_LEVEL_1_2,
    uart_cfg.rx_level = HAL_UART_FIFO_LEVEL_1_4,
    uart_cfg.baud = 115200,
    uart_cfg.dma_rx = true,
    uart_cfg.dma_tx = true,
    uart_cfg.dma_rx_stop_on_err = false;

    app_ble_debug_cnt = 1;

    ret = hal_uart_open(app_ble_uart_id, &uart_cfg);
    if (ret) {
        app_ble_debug_cnt = 0xff;
//        TRACE(0,"Failed to open uart");
        return 1;
    }

    app_ble_debug_cnt = 2;
    hal_uart_irq_set_dma_handler(app_ble_uart_id, uart_rx, uart_tx);
//    hal_uart_dma_recv(app_ble_uart_id, app_ble_uart.rx_data, sizeof(app_ble_uart.rx_data), NULL, NULL);
    mask.reg = 0;
    mask.BE = 1;
    mask.FE = 1;
    mask.OE = 1;
    mask.PE = 1;
    mask.RT = 1;
    hal_uart_irq_set_mask(app_ble_uart_id, mask);
    app_ble_debug_cnt = 3;

    app_ble_uart.rx_done = false;
    app_ble_uart.tx_done = false;
#endif
    gatt_uart_register_write_request_cb(app_ble_uart_write_request_cb);
    return 0;
}
