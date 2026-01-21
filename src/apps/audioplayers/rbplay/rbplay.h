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
/* rbplay header */
/* playback control & rockbox codec porting & codec thread */
#ifndef RBPLAY_HEADER
#define RBPLAY_HEADER

/* ---- APIs ---- */
typedef enum _RB_CTRL_CMD_T {
    RB_CTRL_CMD_NONE = 0,
    RB_CTRL_CMD_SCAN_SONGS,
    RB_CTRL_CMD_CODEC_PARSE_FILE,
    RB_CTRL_CMD_CODEC_INIT,
    RB_CTRL_CMD_CODEC_RUN,
    RB_CTRL_CMD_CODEC_PAUSE,
    RB_CTRL_CMD_CODEC_STOP,
    RB_CTRL_CMD_CODEC_DEINIT,

    RB_CTRL_CMD_CODEC_SEEK,

    RB_CTRL_CMD_CODEC_MAX,
} RB_CTRL_CMD_T;

int app_rbplay_open(void);
void rb_play_codec_init(void);
int rb_codec_running(void);

#endif /* RBPLAY_HEADER */
