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
/* rbplay source */
/* playback control & rockbox codec porting & codec thread */
#ifndef __RB_CTL_H__
#define __RB_CTL_H__


typedef enum {
    RB_MODULE_EVT_NONE,
	RB_MODULE_EVT_PLAY,
	RB_MODULE_EVT_PLAY_IDX,
    RB_MODULE_EVT_STOP,
    RB_MODULE_EVT_SUSPEND,
    RB_MODULE_EVT_RESUME,
    RB_MODULE_EVT_PLAY_NEXT,
	RB_MODULE_EVT_CHANGE_VOL,
	RB_MODULE_EVT_SET_VOL,
	RB_MODULE_EVT_CHANGE_IDLE,
	RB_MODULE_EVT_DEL_FILE,

	RB_MODULE_EVT_RESTORE_DUAL_PLAY,
	RB_MODULE_EVT_LINEIN_START,
	RB_MODULE_EVT_RECONFIG_STREAM,
	RB_MODULE_EVT_SET_TWS_MODE,

    SBCREADER_ACTION_NONE,
    SBCREADER_ACTION_INIT,
    SBCREADER_ACTION_RUN,
    SBCREADER_ACTION_STOP,

	SBC_RECORD_ACTION_START,
	SBC_RECORD_ACTION_DATA_IND,
	SBC_RECORD_ACTION_STOP,

    RB_MODULE_EVT_MAX
} RB_MODULE_EVT;

typedef enum {
    RB_CTL_IDLE,
    RB_CTL_PLAYING,
    RB_CTL_SUSPEND,
} rb_ctl_status;

typedef struct {
    struct mp3entry song_id3;
    char rb_fname[FILE_SHORT_NAME_LEN];
    rb_ctl_status status ; //playing,idle,pause,
    uint16_t rb_audio_file[FILE_PATH_LEN];
    uint16_t curr_song_idx ;
    uint8_t rb_player_vol ;
    int     file_handle;
	int 	init_done;
	BOOL	sbc_record_on;
} rb_ctl_struct;

extern  int app_rbmodule_post_msg(RB_MODULE_EVT evt,uint32_t arg);
extern int rb_ctl_is_audio_file(const char* file_path);

uint8_t rb_ctl_get_vol(void);
#endif

