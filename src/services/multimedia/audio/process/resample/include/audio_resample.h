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
#ifndef __AUDIO_RESAMPLER_H__
#define __AUDIO_RESAMPLER_H__

#error "audio_resample.h/c have been obsolete. Please use audio_resample_ex.h/c instead"

#if 0
#ifdef __cplusplus
extern "C" {
#endif

enum audio_resample_status_t{
    AUDIO_RESAMPLE_STATUS_SUCESS = 0,
    AUDIO_RESAMPLE_STATUS_CONTINUE,
    AUDIO_RESAMPLE_STATUS_FAILED,
};

enum resample_id_t{
	RESAMPLE_ID_8TO8P4 = 0,
	RESAMPLE_ID_8P4TO8,
	RESAMPLE_ID_NUM
};

#define RESAMPLE_TYPE_DOWN_STEREO 0x1
#define RESAMPLE_TYPE_UP_MONO   0x2
#define RESAMPLE_TYPE_DOWN_MONO 0x4

enum audio_resample_status_t audio_resample_open(int type, enum resample_id_t resample_id_up, 
        enum resample_id_t resample_id_down, uint8_t *up_buf, uint8_t *down_buf, void* (* alloc_ext)(int));
enum audio_resample_status_t audio_resample_cfg(int type, short *OutBuf, int OutLen);
enum audio_resample_status_t audio_resample_run(int type, short *InBuf, int InLen);
void audio_resample_close(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
