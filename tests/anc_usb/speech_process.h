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
#ifndef __SPEECH_PROCESS_H__
#define __SPEECH_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "hal_sysfreq.h"

enum HAL_CMU_FREQ_T speech_process_need_freq(void);

void speech_process_init(int tx_sample_rate, int tx_channel_num, int tx_sample_bit,
                         int rx_sample_rate, int rx_channel_num, int rx_sample_bit,
                         int tx_frame_ms, int rx_frame_ms,
                         int tx_send_channel_num, int rx_recv_channel_num);
void speech_process_deinit(void);
void speech_process_capture_run(uint8_t *buf, uint32_t *len);
void speech_process_playback_run(uint8_t *buf, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif
