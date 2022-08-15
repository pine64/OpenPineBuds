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

#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#ifdef MBED
#include "mbed.h"
#include "rtos.h"
#endif
#include "metadata.h"
#include "codecs.h"
#include "eq_export.h"
#include "hal_overlay.h"
#include "app_overlay.h"
#include "audioflinger.h"
#include "hal_trace.h"
#include "apps.h"

#include "rbpcmbuf.h"
#include "rbplaysd.h"
#include "app_thread.h"
#include "app_utils.h"
#include "app_key.h"
#include "rbplay.h"
#include "utils.h"

#ifdef __TWS__
#include "app_tws.h"
#endif

// TODO: remove
#define  BT_STREAM_RBCODEC     0x10    //from rockbox decoder

extern "C" {
    void flac_codec_main(int r);
    void flac_codec_run(void);
    void wav_codec_main(int r);
    void wav_codec_run(void);
    void mpa_codec_main(int r);
    void mpa_codec_run(void);
    void ape_codec_main(int r);
    void ape_codec_run(void);
    void sbc_codec_main(int r);
    void sbc_codec_run(void);
}

extern void rb_pcm_player_open(enum AUD_BITS_T bits,enum AUD_SAMPRATE_T sample_rate,enum AUD_CHANNEL_NUM_T channel_num,uint8_t vol) ;

#if defined(__TWS__)
typedef struct  _rb_tws_codec_info{
    uint8_t update_codec_info;
    int32_t sample_freq;
    uint8_t channel_num;
} rb_tws_codec_info;

rb_tws_codec_info codec_info = {1, 44100, 2};
#endif

static osThreadId rb_decode_tid = NULL;
static osThreadId rb_caller_tid = NULL;

typedef struct {
    uint32_t evt;
    uint32_t arg;
} RBTHREAD_MSG_BLOCK;

#define RBTHREAD_MAILBOX_MAX (10)
osMailQDef (rb_decode_mailbox, RBTHREAD_MAILBOX_MAX, RBTHREAD_MSG_BLOCK);
int rb_decode_mailbox_put(RBTHREAD_MSG_BLOCK* msg_src);
static osMailQId rb_decode_mailbox = NULL;

static void rb_decode_thread(void const *argument);
osThreadDef(rb_decode_thread, osPriorityAboveNormal, 1, 1024 * 2, "rb_decorder");

// rbcodec info
static int song_fd;
static int song_format;
static struct mp3entry *current_id3;
static uint8_t rbplay_loop_on;

static volatile int rb_decode_halt_flag = 1;

struct codec_api ci_api;
struct codec_api *ci = &ci_api;

// TODO
volatile osThreadId thread_tid_waiter = NULL;

void init_dsp(void);
void codec_configure(int setting, intptr_t value);

uint16_t g_rbplayer_curr_song_idx = 0;

extern void app_rbplay_exit(void);

extern  void bt_change_to_iic(APP_KEY_STATUS *status, void *param);

extern  void rb_thread_send_switch(bool next);
extern void rb_thread_send_status_change(void );

enum APP_SYSFREQ_FREQ_T rb_player_get_work_freq(void);

static void rb_player_sync_close_done(void)
{
    thread_tid_waiter = NULL;
}

extern void rb_check_stream_reconfig(int32_t freq, uint8_t ch);
static void f_codec_pcmbuf_insert_callback(
        const void *ch1, const void *ch2, int count)
{
    struct dsp_buffer src;
    struct dsp_buffer dst;

    src.remcount  = count;
    src.pin[0]    = (const unsigned char *)ch1;
    src.pin[1]    = (const unsigned char *)ch2;
    src.proc_mask = 0;

    if (rb_codec_running() == 0)
        return ;

#ifndef __TWS__
    while (src.remcount > 0) {
        dst.remcount = 0;
        dst.p16out = (short *)rb_pcmbuf_request_buffer(&dst.bufcount);

        if (dst.p16out == NULL) {
            warn("No pcm buffer");
            osThreadYield();
        } else {
            dsp_process(ci->dsp, &src, &dst);

            if (dst.remcount > 0) {
                rb_pcmbuf_write(dst.remcount);
            }
        }
    }
#else

    if(codec_info.update_codec_info){
        rb_set_sbc_encoder_freq_ch(codec_info.sample_freq, codec_info.channel_num); //should call this to set trigger timer
        rb_check_stream_reconfig(codec_info.sample_freq, codec_info.channel_num);
        codec_info.update_codec_info = 0;
    }

    if(tws_local_player_need_tran_2_slave()){
        rb_tws_start_master_player(BT_STREAM_RBCODEC);
    }
    while(1){
        uint8_t * pcm_buff = NULL;
        dst.remcount = 0;
        dst.bufcount = MIN(src.remcount, 128); /* Arbitrary min request */
        dst.p16out = (short *)rb_pcmbuf_request_buffer(&dst.bufcount);
        pcm_buff = (uint8_t *)dst.p16out;
        ASSERT(pcm_buff, "Should request buffer");

        dsp_process(ci->dsp, &src, &dst);

        if (dst.remcount > 0) {
            while(rb_push_pcm_in_tws_buffer(pcm_buff, dst.remcount*2*2) == 0){
                osDelay(2);
            }
        }

        if (src.remcount <= 0) {
            return; /* No input remains and DSP purged */
        }
    }
#endif
}

static void f_audio_codec_update_elapsed(unsigned long elapsed)
{
    //info("Update elapsed: %d", elapsed);
    return;
}

static size_t f_codec_filebuf_callback(void *ptr, size_t size)
{
    ssize_t ret;
    ret = read(song_fd, ptr, size);
    if(ret < 0) {
        error("File read error: %d",ret);
    }

    return ret;

}

static void * f_codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    return NULL;
}

static void * f_codec_advance_buffer_callback(size_t amount)
{
    off_t ret = lseek(song_fd, (off_t)(ci->curpos + amount), SEEK_SET);
    if(ret < 0) {
        error("File seek fail");
        return NULL;
    }

    ci->curpos += amount;
    return (void *)ci;

}

static bool f_codec_seek_buffer_callback(size_t newpos)
{
    off_t ret = lseek(song_fd, (off_t)newpos, SEEK_SET);
    if(ret < 0) {
        error("File seek fail");
        return false;
    }

    ci->curpos = newpos;
    return true;
}

static void f_codec_seek_complete_callback(void)
{
    info("Seek complete");
    dsp_configure(ci->dsp, DSP_FLUSH, 0);
}

static void f_audio_codec_update_offset(size_t offset)
{
}

static void f_codec_configure_callback(int setting, intptr_t value)
{
    dsp_configure(ci->dsp, setting, value);
#ifdef __TWS__
    if(setting == DSP_SET_FREQUENCY){
        if(codec_info.sample_freq != value)
            codec_info.update_codec_info = 1;
        codec_info.sample_freq = value;
    }
    else if(setting == DSP_SET_STEREO_MODE){
        if(codec_info.channel_num != (value == STEREO_MONO ? 1 : 2))
            codec_info.update_codec_info = 1;
        codec_info.channel_num = value == STEREO_MONO ? 1 : 2;
    }
#endif
}

static enum codec_command_action f_codec_get_command_callback(intptr_t *param)
{
    if (rb_decode_halt_flag == 1)
        return CODEC_ACTION_HALT ;

    return CODEC_ACTION_NULL;
}

static bool f_codec_loop_track_callback(void)
{
    return false;
}

static void init_ci_file(void)
{
    ci->codec_get_buffer = 0;
    ci->pcmbuf_insert    = f_codec_pcmbuf_insert_callback;
    ci->set_elapsed      = f_audio_codec_update_elapsed;
    ci->read_filebuf     = f_codec_filebuf_callback;
    ci->request_buffer   = f_codec_request_buffer_callback;
    ci->advance_buffer   = f_codec_advance_buffer_callback;
    ci->seek_buffer      = f_codec_seek_buffer_callback;
    ci->seek_complete    = f_codec_seek_complete_callback;
    ci->set_offset       = f_audio_codec_update_offset;
    ci->configure        = f_codec_configure_callback;
    ci->get_command      = f_codec_get_command_callback;
    ci->loop_track       = f_codec_loop_track_callback;
}

static void rb_play_init(void)
{
    init_dsp();

    init_ci_file();

#ifndef __TWS__
    rb_pcmbuf_init();
#endif
}

void rb_play_codec_init(void)
{
    RBTHREAD_MSG_BLOCK msg;
    msg.evt = (uint32_t)RB_CTRL_CMD_CODEC_INIT;
    msg.arg = (uint32_t)0;
    rb_decode_mailbox_put(&msg);
}

void rb_play_codec_run(void)
{
    RBTHREAD_MSG_BLOCK msg;
    msg.evt = (uint32_t)RB_CTRL_CMD_CODEC_RUN;
    msg.arg = (uint32_t)0;
    rb_decode_mailbox_put(&msg);
}

static int rb_codec_init_desc(void )
{
    info("Init decode format: %d", song_format);

    switch (song_format) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            app_overlay_select(APP_OVERLAY_MPA);
            mpa_codec_main(CODEC_LOAD);
            break;
            // TODO: add APP_OVERLAY_APE
#if 0
        case AFMT_APE:
            app_overlay_select(APP_OVERLAY_APE);
            ape_codec_main(CODEC_LOAD);
            break;
        case AFMT_SBC:
            app_overlay_select(APP_OVERLAY_A2DP);
            sbc_codec_main(CODEC_LOAD);
            break;
        case AFMT_FLAC:
            app_overlay_select(APP_OVERLAY_FLAC);
            flac_codec_main(CODEC_LOAD);
            break;
        case AFMT_PCM_WAV:
            app_overlay_select(APP_OVERLAY_WAV);
            wav_codec_main(CODEC_LOAD);
            break;
#endif
        default:
            error("unkown codec type init\n");
            break;
    }

    return 0;
}

static int rb_codec_loop_on(void)
{
#ifdef __TWS__
    //set start transfer to slave
    tws_local_player_set_tran_2_slave_flag(1);
#endif
    switch (song_format) {
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            mpa_codec_run();
            break;
#if 0
        case AFMT_SBC:
            sbc_codec_run();
            break;
        case AFMT_FLAC:
            flac_codec_run();
            break;
        case AFMT_PCM_WAV:
            wav_codec_run();
            break;
        case AFMT_APE:
            ape_codec_run();
            break;
#endif
        default:
            error("unkown codec type run\n");
            break;
    }
    return 0;
}

static int rb_thread_process_evt(RB_CTRL_CMD_T evt)
{
    info("Decode event:%d", evt);

    switch(evt) {
        case RB_CTRL_CMD_CODEC_INIT:
            rb_decode_halt_flag = 0;

            rb_play_init();

            /* get id3 */
            /* init ci info */
            ci->filesize = filesize(song_fd);
            ci->id3 = current_id3;
            ci->curpos = 0;

            dsp_configure(ci->dsp, DSP_RESET, 0);
            dsp_configure(ci->dsp, DSP_FLUSH, 0);

            rb_codec_init_desc();
            break;
        case RB_CTRL_CMD_CODEC_RUN:
            rbplay_loop_on = 1;

            info("Play start");
            app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, rb_player_get_work_freq());
            app_stop_10_second_timer(APP_POWEROFF_TIMER_ID);
            rb_codec_loop_on();
#if defined(__BTIF_AUTOPOWEROFF__)
            app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
            song_fd = 0;
            rb_decode_halt_flag = 1;
            if(thread_tid_waiter) {
                rb_player_sync_close_done();
            } else {
                rb_thread_send_status_change();
                rb_thread_send_switch(true);
            }
#ifdef __TWS__
            //should update codec info after play one music
            codec_info.update_codec_info = 1;
#endif
            rbplay_loop_on = 0;
            info("Play end");
            break;
        default:
            error("Unkown rb cmd %d\n",evt);
            break;
    }

    return 0;
}

int rb_decode_mailbox_put(RBTHREAD_MSG_BLOCK* msg_src)
{
    osStatus status;

    RBTHREAD_MSG_BLOCK *msg_p = NULL;

    msg_p = (RBTHREAD_MSG_BLOCK*)osMailAlloc(rb_decode_mailbox, 0);
    if(!msg_p) {
        TRACE(3,"%s fail, evt:%d,arg=%d \n",__func__,msg_src->evt,msg_src->arg);
        return -1;
    }

    msg_p->evt = msg_src->evt;
    msg_p->arg = msg_src->arg;

    status = osMailPut(rb_decode_mailbox, msg_p);

    return (int)status;
}

int rb_decode_mailbox_free(RBTHREAD_MSG_BLOCK* msg_p)
{
    osStatus status;

    status = osMailFree(rb_decode_mailbox, msg_p);

    return (int)status;
}

int rb_decode_mailbox_get(RBTHREAD_MSG_BLOCK** msg_p)
{
    osEvent evt;
    evt = osMailGet(rb_decode_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (RBTHREAD_MSG_BLOCK *)evt.value.p;
        return 0;
    }
    return -1;
}

static void rb_decode_thread(void const *argument)
{
    RB_CTRL_CMD_T action;
    RBTHREAD_MSG_BLOCK* msg_p;

    while(1) {
        app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, APP_SYSFREQ_32K);
        //  evt = osSignalWait(0, osWaitForever);
        if(0 == rb_decode_mailbox_get(&msg_p)) {
            app_sysfreq_req(APP_SYSFREQ_USER_APP_PLAYER, APP_SYSFREQ_104M);

            action = (RB_CTRL_CMD_T) msg_p->evt;
            rb_caller_tid = (osThreadId) msg_p->arg ;

            TRACE(3,"[%s] action:%d ,tid,0x%x", __func__,  action,rb_caller_tid);
            rb_thread_process_evt(action);

            rb_decode_mailbox_free(msg_p);
            if( rb_caller_tid)
                osSignalSet(rb_decode_tid, 0x1203);
            rb_caller_tid = NULL;
        }

    }
}

int app_rbplay_open(void)
{
    if (rb_decode_tid != NULL) {
        warn("Decode thread reopen");
        return -1;
    }

    rb_decode_mailbox = osMailCreate(osMailQ(rb_decode_mailbox), NULL);
    if (rb_decode_mailbox == NULL)  {
        error("Failed to Create rb_decode_mailbox");
        return -1;
    }

    rb_decode_tid = osThreadCreate(osThread(rb_decode_thread), NULL);
    if (rb_decode_tid == NULL)  {
        error("Failed to Create rb_thread \n");
        return -1;
    }

    return 0;
}

int rb_codec_running(void)
{
    return ((rb_decode_halt_flag == 0)?1:0);
}

void rb_codec_set_halt(int halt)
{
    rb_decode_halt_flag = halt;
}

void rb_thread_set_decode_vars(int fd, int type ,void* id3)
{
    song_fd =fd;
    song_format = type;
    current_id3 = (struct mp3entry *)id3;
}

void rb_player_sync_set_wait_thread(osThreadId tid)
{
    if(rbplay_loop_on)
        thread_tid_waiter = tid;
    else
        thread_tid_waiter = NULL;
}

void rb_player_sync_wait_close(void )
{
    while(NULL != thread_tid_waiter) {
        osThreadYield();
    }
}

enum APP_SYSFREQ_FREQ_T rb_player_get_work_freq(void)
{
    enum APP_SYSFREQ_FREQ_T  freq;

    hal_sysfreq_print();

    info("bitrate:%d freq:%d\n", ci->id3->bitrate, ci->id3->frequency);
#ifndef __TWS__
    enum AUD_SAMPRATE_T sample_rate = AUD_SAMPRATE_44100;
    sample_rate =(enum AUD_SAMPRATE_T ) ci->id3->frequency;
    if(sample_rate > AUD_SAMPRATE_48000)
        freq = APP_SYSFREQ_208M;
    else if (sample_rate > AUD_SAMPRATE_44100)
        freq = APP_SYSFREQ_104M;
    else
        freq = APP_SYSFREQ_52M;

    if(ci->id3->bitrate > 192)
        freq = APP_SYSFREQ_208M;
    else if (ci->id3->bitrate > 128)
        freq = APP_SYSFREQ_104M;
    else
        freq = APP_SYSFREQ_52M;

    switch( song_format ) {
        case AFMT_APE:
            freq = APP_SYSFREQ_208M;
            break;
        case AFMT_FLAC:
            freq = APP_SYSFREQ_208M;
            break;
        case AFMT_PCM_WAV:
            freq = APP_SYSFREQ_208M;
            break;
        default:
            break;
    }
#else
    freq = APP_SYSFREQ_208M;
#endif
    info("Decode thread run at: %d", freq);
    return freq;
}


