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
#ifndef __A2DP_DECODER_CP_H__
#define __A2DP_DECODER_CP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"

enum CP_PROC_DELAY_T {
    CP_PROC_DELAY_0_FRAME = 0,
    CP_PROC_DELAY_1_FRAME,
    CP_PROC_DELAY_2_FRAMES,
    CP_PROC_DELAY_3_FRAMES,

    CP_PROC_DELAY_QTY,
};

enum CP_EMPTY_OUT_FRM_T {
    CP_EMPTY_OUT_FRM_OK,
    CP_EMPTY_OUT_FRM_WORKING,
    CP_EMPTY_OUT_FRM_ERR,
};

typedef int (*A2DP_CP_DECODE_T)(void);

int a2dp_cp_init(A2DP_CP_DECODE_T decode_func, enum CP_PROC_DELAY_T delay);

int a2dp_cp_deinit(void);

int a2dp_cp_decoder_init(uint32_t out_frame_len, uint8_t max_frames);

int a2dp_cp_put_in_frame(const void *buf1, uint32_t len1, const  void *buf2, uint32_t len2);

int a2dp_cp_get_in_frame(void **p_buf, uint32_t *p_len);

int a2dp_cp_consume_in_frame(void);

uint32_t a2dp_cp_get_in_frame_index(void);

uint32_t a2dp_cp_get_in_frame_cnt_by_index(uint32_t ridx);

void a2dp_cp_reset_frame(void);

bool a2dp_cp_get_in_frame_reset_status(void);

enum CP_EMPTY_OUT_FRM_T a2dp_cp_get_emtpy_out_frame(void **p_buf, uint32_t *p_len);

int a2dp_cp_consume_emtpy_out_frame(void);

int a2dp_cp_get_full_out_frame(void **p_buf, uint32_t *p_len);

int a2dp_cp_consume_full_out_frame(void);

unsigned int set_cp_reset_flag(uint8_t evt);

bool is_cp_need_reset(void);

bool is_cp_init_done(void);

#ifdef __cplusplus
}
#endif

#endif

