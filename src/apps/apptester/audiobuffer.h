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
#ifndef __AUDIOBUFFER_H__
#define __AUDIOBUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_BUFFER_FRAME_SIZE (4*1024)
#define AUDIO_BUFFER_FRAME_NUM (1)
#define AUDIO_BUFFER_TOTAL_SIZE (AUDIO_BUFFER_FRAME_SIZE*AUDIO_BUFFER_FRAME_NUM)

void audio_mono2stereo_16bits(uint16_t *dst_buf, uint16_t *src_buf, uint32_t src_len);

void audio_stereo2mono_16bits(uint8_t chnlsel, uint16_t *dst_buf, uint16_t *src_buf, uint32_t src_len);

void audio_buffer_init(void);

int audio_buffer_length(void);

int audio_buffer_set(uint8_t *buff, uint16_t len);

int audio_buffer_get(uint8_t *buff, uint16_t len);

int audio_buffer_set_stereo2mono_16bits(uint8_t *buff, uint16_t len, uint8_t chnlsel);

int audio_buffer_get_mono2stereo_16bits(uint8_t *buff, uint16_t len);

#ifdef __cplusplus
	}
#endif

#endif//__FMDEC_H__
