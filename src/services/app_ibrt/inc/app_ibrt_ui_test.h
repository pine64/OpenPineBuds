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
#ifndef __APP_IBRT_UI_TEST_H__
#define __APP_IBRT_UI_TEST_H__
#include "app_key.h"
#include <stdint.h>
#if defined(IBRT)

typedef void (*app_uart_test_function_handle)(void);

typedef struct {
  const char *string;
  app_uart_test_function_handle function;
} app_uart_handle_t;

app_uart_test_function_handle app_ibrt_ui_find_uart_handle(unsigned char *buf);

void app_ibrt_ui_test_key_init(void);

void app_ibrt_ui_test_init(void);

int app_ibrt_ui_test_config_load(void *config);

void app_ibrt_ui_sync_status(uint8_t status);

void app_ibrt_ui_test_voice_assistant_key(APP_KEY_STATUS *status, void *param);

void app_ibrt_search_ui_gpio_key_handle(APP_KEY_STATUS *status, void *param);
void pwrkey_detinit(void);
void startpwrkey_det(int ms);
#endif
#endif
