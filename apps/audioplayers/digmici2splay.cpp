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
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "cqueue.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"

uint8_t digmic_buf[100*1024];
uint32_t digmic_buf_len = 0;

uint32_t dig_mic_audio_more_data(uint8_t *buf, uint32_t len)
{
    TRACE(2,"%s:%d\n", __func__, __LINE__);
    memcpy(buf, digmic_buf, len);

    return len;
}
uint32_t dig_mic_audio_data_come(uint8_t *buf, uint32_t len)
{
    TRACE(2,"%s:%d\n", __func__, __LINE__);

    memcpy(digmic_buf + digmic_buf_len, buf, len);

#if 1
    digmic_buf_len = (digmic_buf_len + len)%(100*1024);
#endif
    return len;
}
