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
#ifndef __AUDIO_DUMP_H__
#define __AUDIO_DUMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AUDIO_DEBUG
#define AUDIO_DUMP
#endif

void audio_dump_init(int frame_len, int sample_bytes, int channel_num);
void audio_dump_deinit(void);
void audio_dump_clear_up(void);
void audio_dump_add_channel_data_from_multi_channels(int channel_id, void *pcm_buf, int pcm_len, int channel_num, int channel_index);
void audio_dump_add_channel_data(int channel_id, void *pcm_buf, int pcm_len);
void audio_dump_run(void);

void data_dump_init(void);
void data_dump_deinit(void);
void data_dump_run(const char *str, void *data_buf, uint32_t data_len);

#ifdef __cplusplus
}
#endif

#endif