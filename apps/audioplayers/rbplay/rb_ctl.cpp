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

#ifdef MBED
#include "mbed.h"
#include "rtos.h"
#endif
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "app_audio.h"

#include "eq.h"
#include "pga.h"
#include "metadata.h"
#include "dsp_core.h"
#include "codecs.h"
#include "codeclib.h"
#include "compressor.h"
#include "channel_mode.h"
#include "audiohw.h"
#include "codec_platform.h"
#include "metadata_parsers.h"

#include "hal_overlay.h"
#include "app_overlay.h"
#include "rbpcmbuf.h"
#include "rbplay.h"
#include "app_thread.h"
#include "app_utils.h"
#include "app_key.h"

#include "rbplaysd.h"
#include "rb_ctl.h"

/* Internals */

void rb_ctl_wait_lock_thread(bool lock);
void app_rbcodec_ctl_set_play_status(bool st);
extern bool app_rbcodec_check_hfp_active(void );

#include "SDFileSystem.h"

extern "C" void hal_sysfreq_print(void);

#define _LOG_TAG "[rb_ctl] "
#undef __attribute__(a)

#define RBPLAY_DEBUG 1
#if  RBPLAY_DEBUG
#define _LOG_DBG(str,...) TRACE(0,_LOG_TAG""str, ##__VA_ARGS__)
#define _LOG_ERR(str,...) TRACE(0,_LOG_TAG"err:"""str, ##__VA_ARGS__)
#else
#define _LOG_DBG(str,...)
#define _LOG_ERR(str,...) TRACE(0,_LOG_TAG"err:"""str, ##__VA_ARGS__)
#endif

#include "rb_ctl.h"
void rb_thread_send_resume(void);

typedef struct {
    uint32_t evt;
    uint32_t arg;
} RBCTL_MSG_BLOCK;

static osThreadId rb_ctl_tid;
static void rb_ctl_thread(void const *argument);
static int rb_ctl_default_priority;
osThreadDef(rb_ctl_thread, osPriorityHigh, 1, 1024 * 4, "rb_ctl");

#define RBCTL_MAILBOX_MAX (20)
osMailQDef (rb_ctl_mailbox, RBCTL_MAILBOX_MAX, RBCTL_MSG_BLOCK);
static osMailQId rb_ctl_mailbox = NULL;

int rb_ctl_mailbox_put(RBCTL_MSG_BLOCK* msg_src);

//playlist
extern playlist_struct sd_playlist;


rb_ctl_struct rb_ctl_context;

extern void rb_thread_set_decode_vars(int fd, int type ,void* id3);
extern void rb_play_codec_run(void);
extern  void rb_codec_set_halt(int halt);
extern void rb_player_sync_set_wait_thread(osThreadId tid);
extern void rb_player_sync_wait_close(void );
extern  bool rb_pcmbuf_suspend_play_loop(void);
void app_rbplay_load_playlist(playlist_struct *list );
int rb_thread_post_msg(RB_MODULE_EVT evt,uint32_t arg );

static rb_ctl_status rb_ctl_get_status(void)
{
	return rb_ctl_context.status;
}

bool rb_ctl_parse_file(const char* file_path)
{
    /* open file */

    _LOG_DBG(1,"open file  %s\n", file_path);
    rb_ctl_context.file_handle = open((const char *)file_path, O_RDWR);
    if (rb_ctl_context.file_handle <= 0) {
        _LOG_DBG(1,"open file %s failed\n", file_path);
        return false;
    }

    if (!get_metadata(&rb_ctl_context.song_id3, rb_ctl_context.file_handle, rb_ctl_context.rb_fname)) {
        _LOG_DBG(0,"get_metadata failed\n");
        close(rb_ctl_context.file_handle);
        return false;
    }

    _LOG_DBG(4,"%s bitr:%d,saps %d, freq:%d\n",__func__,rb_ctl_context.song_id3.bitrate,rb_ctl_context.song_id3.samples,rb_ctl_context.song_id3.frequency);

    rb_thread_set_decode_vars(rb_ctl_context.file_handle, rb_ctl_context.song_id3.codectype,&rb_ctl_context.song_id3);

    return true;
}

void rb_ctl_vol_operation(bool inc )
{
    uint32_t ret;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;

    _LOG_DBG(2,"%s inc:%d , ",__func__,inc);

    if(inc ) {
        if(rb_ctl_context.rb_player_vol < 16 ) {
            rb_ctl_context.rb_player_vol++;

        }
    } else {
        if(rb_ctl_context.rb_player_vol > 1 ) {
            rb_ctl_context.rb_player_vol--;
        }
    }
    if(rb_ctl_context.status != RB_CTL_PLAYING)
        return ;

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->vol = rb_ctl_context.rb_player_vol;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }
}

bool rb_ctl_is_playing(void)
{
    return (rb_ctl_context.status == RB_CTL_PLAYING);
}

void rb_ctl_set_vol(uint32_t vol)
{
    uint32_t ret;
    struct AF_STREAM_CONFIG_T *stream_cfg = NULL;

    _LOG_DBG(2,"%s set vol as :%d , ",__func__,vol);

    if(rb_ctl_context.status != RB_CTL_PLAYING)
        return ;

    vol = (vol > 16)?16:vol;
    rb_ctl_context.rb_player_vol = vol;

    ret = af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg, true);
    if (ret == 0) {
        stream_cfg->vol = rb_ctl_context.rb_player_vol;
        af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, stream_cfg);
    }
}

void rb_ctl_stop_play(void )
{
    _LOG_DBG(1,"%s  \n",__func__);

    rb_codec_set_halt(1);
//close file
    _LOG_DBG(0," af  stream stop \n");
    af_stream_stop(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
    _LOG_DBG(0," af	stream close \n");
    af_stream_close(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
    _LOG_DBG(0," close file \n");
    if(rb_ctl_context.file_handle != -1)
        close(rb_ctl_context.file_handle );
    rb_ctl_context.file_handle = -1;
//release frequency
    _LOG_DBG(0," release freq  \n");
    
    return ;
}

void rb_ctl_sync_stop_play(void )
{
    rb_player_sync_set_wait_thread(osThreadGetId());

    rb_ctl_stop_play();

    rb_player_sync_wait_close();
    //close file
    _LOG_DBG(0," close file \n");
    if(rb_ctl_context.file_handle != -1)
        close(rb_ctl_context.file_handle );
    rb_ctl_context.file_handle = -1;
}

void rb_ctl_pause_playing(void )
{
    af_stream_stop(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
    _LOG_DBG(0," af   stream close \n");
    af_stream_close(AUD_STREAM_ID_0,AUD_STREAM_PLAYBACK);
//wait decoder suspend
    //osDelay(200);
   // while(!rb_pcmbuf_suspend_play_loop()) {
        //osThreadYield();
    //    osDelay(1);
   // }
	
    _LOG_DBG(1,"%s sucessed ",__func__);
}

void rb_ctl_resume_playing(void )
{
#ifndef __TWS__
    rb_pcmbuf_init();
#endif
}


void rb_ctl_set_priority(int priority)
{
    osThreadSetPriority(rb_ctl_tid, (osPriority)priority);
}

int rb_ctl_get_priority(void)
{
    return (int)osThreadGetPriority(rb_ctl_tid);
}

int rb_ctl_get_default_priority(void)
{
    return rb_ctl_default_priority;
}

void rb_ctl_reset_priority(void)
{
    osThreadSetPriority(rb_ctl_tid, (osPriority)rb_ctl_default_priority);
}

#ifdef __IAG_BLE_INCLUDE__
extern "C" void app_ble_inform_music_switch(uint16_t index);
#endif
static int rb_ctl_handle_event(RBCTL_MSG_BLOCK *msg_body)
{
    RB_MODULE_EVT evt =(RB_MODULE_EVT) msg_body->evt;
    uint32_t arg = msg_body->arg;
//    hal_sysfreq_print();


    switch(evt) {       
        case RB_MODULE_EVT_PLAY:
            if(rb_ctl_context.status != RB_CTL_IDLE) {
                if(rb_ctl_context.status == RB_CTL_SUSPEND)
                    rb_thread_send_resume();
                else
                    _LOG_DBG(2,"%s  st  %d not in idle \n",__func__,rb_ctl_context.status);
                break;
            }
			_LOG_DBG(3," %s start %d/%d ",__func__, rb_ctl_context.curr_song_idx ,sd_playlist.total_songs);
            if(sd_playlist.total_songs > 0 ) {
                playlist_item *it;

                it = app_rbplay_get_playitem(rb_ctl_context.curr_song_idx) ;

                if(it == NULL) {
                    _LOG_DBG(2," %s get item fail idx %d",__func__,rb_ctl_context.curr_song_idx);
                }

                _LOG_DBG(2,"%s  start songidx %d \n",__func__,rb_ctl_context.curr_song_idx);

                memcpy(rb_ctl_context.rb_audio_file,it->file_path,sizeof(uint16_t)*FILE_PATH_LEN - 4);
                memcpy(rb_ctl_context.rb_fname,it->file_name,sizeof(rb_ctl_context.rb_fname));

                if(rb_ctl_parse_file((const char*)rb_ctl_context.rb_audio_file)) {
                    _LOG_DBG(2,"%s  start init, the tid 0x%x\n",__func__,osThreadGetId());
                    rb_play_codec_init();
                    _LOG_DBG(1,"%s  start run\n",__func__);
                    rb_play_codec_run();
                    rb_ctl_context.status = RB_CTL_PLAYING;
					app_rbcodec_ctl_set_play_status(true);
                    break;
                } else {
                    _LOG_DBG(2,"%s  evt  %d parse fail,find next\n",__func__,msg_body->evt);
					rb_thread_post_msg(RB_MODULE_EVT_PLAY_NEXT,true);
                }

            } else {
                _LOG_DBG(2,"%s  evt  %d no songs\n",__func__,msg_body->evt);
            }
            break;
        case RB_MODULE_EVT_STOP:
            if(rb_ctl_context.status == RB_CTL_IDLE) {
                _LOG_DBG(2,"%s  st  %d in idle \n",__func__,rb_ctl_context.status);
                break;
            }
            if(rb_ctl_context.status == RB_CTL_SUSPEND) {
				//osDelay(100);
				rb_ctl_resume_playing();
				rb_ctl_context.status = RB_CTL_PLAYING;
            }
            rb_ctl_sync_stop_play();
            rb_ctl_context.status = RB_CTL_IDLE;
			app_rbcodec_ctl_set_play_status(false);
            break;
        case RB_MODULE_EVT_SUSPEND:
            if (rb_ctl_context.status != RB_CTL_PLAYING) {
                _LOG_DBG(2,"%s  st  %d not running \n",__func__,rb_ctl_context.status);
                break;
            }
            rb_ctl_pause_playing();
            rb_ctl_context.status = RB_CTL_SUSPEND;
            break;
        case RB_MODULE_EVT_RESUME:
            if (rb_ctl_context.status != RB_CTL_SUSPEND) {
                _LOG_DBG(2,"%s  st  %d not suspend \n",__func__,rb_ctl_context.status);
                break;
            }
            rb_ctl_resume_playing();
            rb_ctl_context.status = RB_CTL_PLAYING;
            break;
        case RB_MODULE_EVT_PLAY_NEXT:
            if( rb_ctl_context.status == RB_CTL_PLAYING) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
            } else if( rb_ctl_context.status == RB_CTL_SUSPEND) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
            } else {

            }
            if(arg == 0) {
                rb_ctl_context.curr_song_idx--;

                if(rb_ctl_context.curr_song_idx == 0xffff)
                    rb_ctl_context.curr_song_idx = sd_playlist.total_songs -1;

            } else {

                rb_ctl_context.curr_song_idx++;

                if(rb_ctl_context.curr_song_idx >= sd_playlist.total_songs)
                    rb_ctl_context.curr_song_idx = 0;
            }
            
        #ifdef __IAG_BLE_INCLUDE__
            app_ble_inform_music_switch(rb_ctl_context.curr_song_idx);
        #endif
        
            rb_thread_post_msg(RB_MODULE_EVT_PLAY,0 );
            break;
        case RB_MODULE_EVT_CHANGE_VOL:
            rb_ctl_vol_operation(arg);
            break;
        case RB_MODULE_EVT_SET_VOL:
            rb_ctl_set_vol(arg);
            break;
        case RB_MODULE_EVT_CHANGE_IDLE:
			rb_ctl_stop_play();
            rb_ctl_context.status = RB_CTL_IDLE;        
            break;
        case RB_MODULE_EVT_PLAY_IDX:
            if(arg > sd_playlist.total_songs -1 ) {
                break;
            }
            if( rb_ctl_context.status == RB_CTL_PLAYING) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
            } else if( rb_ctl_context.status == RB_CTL_SUSPEND) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
            } else {

            }

            rb_ctl_context.curr_song_idx = (uint16_t)arg;

            rb_thread_post_msg(RB_MODULE_EVT_PLAY,0 );
            break;
//for voice cocah
#if 0
        case SBCREADER_ACTION_INIT:
            //prepare fs
            voiceCocah_prepare_fs();
            //init the data buff
            voiceCocah_read_sbc_data();
            break;
        case SBCREADER_ACTION_RUN:
            //fill sbc queue
            voiceCocah_read_sbc_data();
            break;
        case SBCREADER_ACTION_STOP:
            //release the res
            voiceCocah_stop_clean();
            rb_ctl_reset_priority();
            break;
#endif

#ifdef __TWS__
        case RB_MODULE_EVT_RESTORE_DUAL_PLAY:
            extern void rb_restore_dual_play(uint8_t stream_type);
            rb_restore_dual_play(arg);
        break;
#endif
		case RB_MODULE_EVT_DEL_FILE:
			static rb_ctl_status prev_status = rb_ctl_context.status;
            if( rb_ctl_context.status == RB_CTL_PLAYING) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
				rb_thread_post_msg(RB_MODULE_EVT_DEL_FILE,arg);
            } else if( rb_ctl_context.status == RB_CTL_SUSPEND) {
                rb_thread_post_msg(RB_MODULE_EVT_STOP,0 );
				rb_thread_post_msg(RB_MODULE_EVT_DEL_FILE,arg);
            } else {
				app_ctl_remove_file(arg);
				if(prev_status == RB_CTL_PLAYING) {
					rb_thread_post_msg(RB_MODULE_EVT_PLAY,0 );
				}
            }
	
        break;
#if defined(TWS_LINEIN_PLAYER)        
        case RB_MODULE_EVT_LINEIN_START:
            extern int app_linein_codec_start(void);
            app_linein_codec_start();
            break;

        case RB_MODULE_EVT_RECONFIG_STREAM:
            extern void rb_tws_reconfig_stream(uint32_t arg);
            rb_tws_reconfig_stream(arg);
            break;

        case RB_MODULE_EVT_SET_TWS_MODE:
            break;
#endif            
#ifdef SBC_RECORD_TEST

		case SBC_RECORD_ACTION_START:
		{
			rb_ctl_context.sbc_record_on = true;
			app_rbplay_set_store_flag(true);
			app_rbplay_open_sbc_file();

		}
		break;
			
		case SBC_RECORD_ACTION_DATA_IND:
		{
			if(rb_ctl_context.sbc_record_on) {
				app_rbplay_process_sbc_data();
			}
		}
		break;
				
		case SBC_RECORD_ACTION_STOP:
		{
			rb_ctl_context.sbc_record_on = false;
			app_rbplay_set_store_flag(false);
			app_rbplay_close_sbc_file();
		}
		break;
#endif					
        default:
            break;
    }
	if(SBC_RECORD_ACTION_DATA_IND !=msg_body->evt)
    _LOG_DBG(3,"%s  rbcodec evt  %d  ,st %d ended\n",__func__,msg_body->evt,rb_ctl_context.status);
    return 0;
}

int rb_thread_post_msg(RB_MODULE_EVT evt,uint32_t arg )
{
    int ret;
    RBCTL_MSG_BLOCK msg;

	if(!rb_ctl_tid)
		return 0;
    /* APIs */
    msg.evt = (uint32_t)evt;
    msg.arg = (uint32_t)arg;
    ret = rb_ctl_mailbox_put(&msg);

    _LOG_DBG(3,"%s   ret %d evt:%d\n",__func__,ret ,evt);
    return 0;
}

void app_wait_player_stoped(void )
{
    while(rb_ctl_context.status != RB_CTL_IDLE)
        osThreadYield();
}

void app_wait_player_suspend(void )
{
	if(rb_ctl_context.status == RB_CTL_IDLE)
		return;
		
    while(rb_ctl_context.status != RB_CTL_SUSPEND)
        osThreadYield();
}

void app_wait_player_resumed(void )
{
	if(rb_ctl_context.status == RB_CTL_IDLE)
		return;
		
    while(rb_ctl_context.status != RB_CTL_PLAYING)
        osThreadYield();
}

void rb_thread_send_play(void )
{
    _LOG_DBG(1," %s ",__FUNCTION__);
    rb_thread_post_msg(RB_MODULE_EVT_PLAY, (uint32_t)osThreadGetId());
}

void rb_thread_send_stop(void )
{
    _LOG_DBG(1," %s ",__FUNCTION__);
    rb_thread_post_msg(RB_MODULE_EVT_STOP, (uint32_t)osThreadGetId());
	
	osDelay(200);
    app_wait_player_stoped();
}

void rb_thread_send_pause(void)
{
    _LOG_DBG(1," %s ",__FUNCTION__);

    rb_thread_post_msg(RB_MODULE_EVT_SUSPEND,(uint32_t)osThreadGetId());
    app_wait_player_suspend();
	
    _LOG_DBG(1," %s end",__FUNCTION__);
}

void rb_thread_send_resume(void)
{
    _LOG_DBG(1," %s ",__FUNCTION__);

    rb_thread_post_msg(RB_MODULE_EVT_RESUME,(uint32_t)osThreadGetId());
	
    _LOG_DBG(1," %s end",__FUNCTION__);
}

void rb_play_dual_play_restore(uint8_t stream_type )
{
    _LOG_DBG(2,"%s %d \n", __func__, __LINE__);

    rb_thread_post_msg(RB_MODULE_EVT_RESTORE_DUAL_PLAY, stream_type);
     _LOG_DBG(2,"%s %d \n", __func__, __LINE__);
}

void rb_thread_send_switch(bool next)
{
    _LOG_DBG(2,"%s %d \n", __func__, __LINE__);

    rb_thread_post_msg(RB_MODULE_EVT_PLAY_NEXT,next);

     _LOG_DBG(2,"%s %d \n", __func__, __LINE__);
}

void rb_play_linein_start(void )
{
    _LOG_DBG(2,"%s %d \n", __func__, __LINE__);

    rb_thread_post_msg(RB_MODULE_EVT_LINEIN_START, 0);
     _LOG_DBG(2,"%s %d \n", __func__, __LINE__);
}

void rb_play_reconfig_stream(uint32_t arg )
{
    _LOG_DBG(3,"%s %d arg:0x%x\n", __func__, __LINE__, arg);

    rb_thread_post_msg(RB_MODULE_EVT_RECONFIG_STREAM, arg);
     _LOG_DBG(2,"%s %d \n", __func__, __LINE__);
}

void rb_play_set_tws_mode(uint32_t arg )
{
    _LOG_DBG(3,"%s %d arg:0x%x\n", __func__, __LINE__, arg);

    rb_thread_post_msg(RB_MODULE_EVT_SET_TWS_MODE, arg);
     _LOG_DBG(2,"%s %d \n", __func__, __LINE__);
}

void rb_thread_send_play_idx(uint32_t array_subidx)
{
    _LOG_DBG(2," %s array_subidx %d, ",__FUNCTION__,array_subidx);

    rb_thread_post_msg(RB_MODULE_EVT_PLAY_IDX,array_subidx);
}


void rb_thread_send_vol(bool inc)
{
    _LOG_DBG(2," %s inc %d, ",__FUNCTION__,inc);

    rb_thread_post_msg(RB_MODULE_EVT_CHANGE_VOL,inc);
}

void rb_thread_send_status_change(void )
{
    _LOG_DBG(1," %s , ",__FUNCTION__);

    rb_thread_post_msg(RB_MODULE_EVT_CHANGE_IDLE,0);
}

void rb_thread_set_vol(uint32_t vol)
{
    _LOG_DBG(1," %s , ",__FUNCTION__);

    rb_thread_post_msg(RB_MODULE_EVT_SET_VOL, vol);
}

void rb_thread_send_del_file(uint16_t idx )
{
    _LOG_DBG(1," %s , ",__FUNCTION__);

    rb_thread_post_msg(RB_MODULE_EVT_DEL_FILE,idx);
}

void rb_thread_send_sbc_record_start(void )
{
    _LOG_DBG(1," %s , ",__FUNCTION__);

    rb_thread_post_msg(SBC_RECORD_ACTION_START,(uint32_t)osThreadGetId());
}

void rb_thread_send_sbc_record_data_ind(void )
{
	if(!rb_ctl_context.sbc_record_on)
		return ;
  //  _LOG_DBG(1," %s , ",__FUNCTION__);
    rb_thread_post_msg(SBC_RECORD_ACTION_DATA_IND,(uint32_t)osThreadGetId());
}

void rb_thread_send_sbc_record_stop(void )
{
    _LOG_DBG(1," %s , ",__FUNCTION__);

    rb_thread_post_msg(SBC_RECORD_ACTION_STOP,(uint32_t)osThreadGetId());
}


//interface for audio module
static bool user_key_pause_stream = false;
void app_rbplay_audio_reset_pause_status(void)
{
	user_key_pause_stream = false;
}

int app_rbplay_audio_onoff(bool onoff, uint16_t aud_id)
{
    _LOG_DBG(3," %s onoff %d, get status:%d",__FUNCTION__,onoff, rb_ctl_get_status());

	if(app_rbcodec_check_hfp_active()&& !onoff) {
        if( RB_CTL_PLAYING == rb_ctl_get_status()) {
            rb_thread_send_pause();
        } 
	} else if ( !onoff) {
		//rb_thread_send_stop();
		rb_thread_send_pause();
	} else {
		if(!user_key_pause_stream) {
			if( RB_CTL_SUSPEND == rb_ctl_get_status()) {
				rb_thread_send_resume();
				app_wait_player_resumed();
			} else {
    	rb_thread_send_play();
    }
		}
	}
	
    return 0;
}

void app_rbplay_pause_resume(void)
{
    _LOG_DBG(2," %s get status:%d",__func__, rb_ctl_get_status());
	
	user_key_pause_stream = true;
    if( RB_CTL_SUSPEND == rb_ctl_get_status()) {
		user_key_pause_stream = false;
        rb_thread_send_resume();
    }
    if( RB_CTL_PLAYING == rb_ctl_get_status()) {
		user_key_pause_stream = true;
        rb_thread_send_pause();
    }
}
//sbc reader run within thread

static int rb_ctl_mailbox_init(void)
{
    rb_ctl_mailbox = osMailCreate(osMailQ(rb_ctl_mailbox), NULL);
    if (rb_ctl_mailbox == NULL)  {
        TRACE(0,"Failed to Create rb_ctl_mailbox\n");
        return -1;
    }
    return 0;
}

int rb_ctl_mailbox_put(RBCTL_MSG_BLOCK* msg_src)
{
    osStatus status;

    RBCTL_MSG_BLOCK *msg_p = NULL;

    msg_p = (RBCTL_MSG_BLOCK*)osMailAlloc(rb_ctl_mailbox, 0);
    if(!msg_p) {
        TRACE(3,"%s fail, evt:%d,arg=%d \n",__func__,msg_src->evt,msg_src->arg);
        return -1;
    }

    ASSERT(msg_p, "osMailAlloc error");

    msg_p->evt = msg_src->evt;
    msg_p->arg = msg_src->arg;

    status = osMailPut(rb_ctl_mailbox, msg_p);

    return (int)status;
}

int rb_ctl_mailbox_free(RBCTL_MSG_BLOCK* msg_p)
{
    osStatus status;

    status = osMailFree(rb_ctl_mailbox, msg_p);

    return (int)status;
}

int rb_ctl_mailbox_get(RBCTL_MSG_BLOCK** msg_p)
{
    osEvent evt;
    evt = osMailGet(rb_ctl_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (RBCTL_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

#if 0
void rb_ctl_action_init(void)
{

    bool ind = voiceCocah_get_indication();
    TRACE(2," %s Cocah %d",__func__,ind);

    if( !ind) return ;

    //  rb_ctl_set_priority(osPriorityHigh);

    rb_thread_post_msg(SBCREADER_ACTION_INIT,(uint32_t)osThreadGetId());
}

void rb_ctl_action_run(void)
{
    bool ind = voiceCocah_get_indication();
    TRACE(2," %s Cocah %d",__func__,ind);

    if( !ind) return ;

    rb_thread_post_msg(SBCREADER_ACTION_RUN,(uint32_t)osThreadGetId());
}

void rb_ctl_action_stop(void)
{
    bool ind = voiceCocah_get_indication();
    TRACE(2," %s Cocah %d",__func__,ind);

    if( !ind) return ;

    rb_thread_post_msg(SBCREADER_ACTION_STOP,(uint32_t)osThreadGetId());

}
#endif
static void rb_ctl_thread(void const *argument)
{
    osEvent evt;
#if 1
    app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, APP_SYSFREQ_208M);

    app_rbplay_load_playlist(&sd_playlist);
#endif
    memset(&rb_ctl_context,0x0,sizeof(rb_ctl_struct));
    rb_ctl_context.rb_player_vol = 6;
    rb_ctl_context.status = RB_CTL_IDLE;


    rb_ctl_context.curr_song_idx = 0;
    //load playlist here

    //voiceCocah_init();
	
	rb_ctl_context.init_done = true;
	
    while(1) {
        RBCTL_MSG_BLOCK *msg_p = NULL;

        app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, APP_SYSFREQ_32K);

        if (!rb_ctl_mailbox_get(&msg_p)) {
            app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, APP_SYSFREQ_104M);

            rb_ctl_handle_event(msg_p);

            rb_ctl_mailbox_free(msg_p);
        }
    }
}


int rb_ctl_init(void)
{
    _LOG_DBG(1,"%s \n",__func__);

	rb_ctl_context.init_done = false;
	rb_ctl_context.sbc_record_on = false;
	
	app_rbplay_open();

	rb_ctl_mailbox_init();

	rb_ctl_tid = osThreadCreate(osThread(rb_ctl_thread), NULL);
    rb_ctl_default_priority = osPriorityAboveNormal;

	if (rb_ctl_tid == NULL)  {
		TRACE(0,"Failed to Create rb_ctl_thread\n");
		return 0;
	}
	_LOG_DBG(1,"Leave %s \n",__func__);
	return 0;
}

uint8_t rb_ctl_get_vol(void)
{
    return rb_ctl_context.rb_player_vol;
}

bool rb_ctl_is_init_done(void)
{
	return rb_ctl_context.init_done;
}

bool rb_ctl_is_paused(void)
{
	return ( RB_CTL_SUSPEND == rb_ctl_get_status());
}

uint16_t rb_ctl_songs_count(void)
{
	return sd_playlist.total_songs;
}

