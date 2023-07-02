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
#ifndef FRAME_RESIZE_H
#define FRAME_RESIZE_H

#include "stdint.h"

struct FrameResizeState_;

typedef struct FrameResizeState_ FrameResizeState;

typedef int32_t(*CAPTURE_HANDLER_T)(void *, void *, int32_t *);

typedef int32_t(*PLAYBACK_HANDLER_T)(void *, int32_t *);

#ifdef __cplusplus
extern "C" {
#endif

FrameResizeState *frame_resize_create(int codec_frame_size,
                                    int codec_capture_channel, 
                                    int vqe_frame_size,
                                    int capture_sample_size,
                                    int playback_sample_size,
                                    int aec_enable, 
                                    const CAPTURE_HANDLER_T capture_handler,
                                    const PLAYBACK_HANDLER_T playback_handler);

void frame_resize_destroy(FrameResizeState *st);

void frame_resize_process_capture(FrameResizeState *st, void *pcm_buf, void *ref_buf, int32_t *pcm_len);

void frame_resize_process_playback(FrameResizeState *st, void *pcm_buf, int32_t *pcm_len);

#ifdef __cplusplus
}
#endif

#endif