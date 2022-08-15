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
#include "cmsis_os.h"
#include "hal_trace.h"
#include "resources.h"
#include "app_bt_stream.h"
#include "app_media_player.h"
#include "app_factory.h"
#include "string.h"

// for audio
#include "audioflinger.h"
#include "app_audio.h"
#include "app_utils.h"

#include "app_factory_audio.h"


#ifdef __FACTORY_MODE_SUPPORT__

#define BT_AUDIO_FACTORMODE_BUFF_SIZE    	(1024*2)
static enum APP_AUDIO_CACHE_T a2dp_cache_status = APP_AUDIO_CACHE_QTY;
static int16_t *app_audioloop_play_cache = NULL;

static uint32_t app_factorymode_data_come(uint8_t *buf, uint32_t len)
{
    app_audio_pcmbuff_put(buf, len);
    if (a2dp_cache_status == APP_AUDIO_CACHE_QTY){
        a2dp_cache_status = APP_AUDIO_CACHE_OK;
    }
    return len;
}

static uint32_t app_factorymode_more_data(uint8_t *buf, uint32_t len)
{
    if (a2dp_cache_status != APP_AUDIO_CACHE_QTY){
        app_audio_pcmbuff_get((uint8_t *)app_audioloop_play_cache, len/2);
        app_bt_stream_copy_track_one_to_two_16bits((int16_t *)buf, app_audioloop_play_cache, len/2/2);
    }
    return len;
}

int app_factorymode_audioloop(bool on, enum APP_SYSFREQ_FREQ_T freq)
{
    uint8_t *buff_play = NULL;
    uint8_t *buff_capture = NULL;
    uint8_t *buff_loop = NULL;
    struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    APP_FACTORY_TRACE(3,"app_factorymode_audioloop work:%d op:%d freq:%d", isRun, on, freq);

    if (isRun==on)
        return 0;

    if (on){
        if (freq < APP_SYSFREQ_52M) {
            freq = APP_SYSFREQ_52M;
        }
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, freq);

        a2dp_cache_status = APP_AUDIO_CACHE_QTY;
        app_audio_mempool_init();
        app_audio_mempool_get_buff(&buff_capture, BT_AUDIO_FACTORMODE_BUFF_SIZE);
        app_audio_mempool_get_buff(&buff_play, BT_AUDIO_FACTORMODE_BUFF_SIZE*2);
        app_audio_mempool_get_buff((uint8_t **)&app_audioloop_play_cache, BT_AUDIO_FACTORMODE_BUFF_SIZE*2/2/2);
        app_audio_mempool_get_buff(&buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<2);
        app_audio_pcmbuff_init(buff_loop, BT_AUDIO_FACTORMODE_BUFF_SIZE<<2);
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.bits = AUD_BITS_16;
        //stream_cfg.channel_num = AUD_CHANNEL_NUM_1;

#ifdef SPEECH_TX_AEC_CODEC_REF
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
#else
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
#endif
#if defined(__AUDIO_RESAMPLE__) && defined(SW_CAPTURE_RESAMPLE)
        stream_cfg.sample_rate = AUD_SAMPRATE_8463;
#else
        stream_cfg.sample_rate = AUD_SAMPRATE_8000;
#endif
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.vol = TGT_VOLUME_LEVEL_15;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.handler = app_factorymode_data_come;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_capture);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.handler = app_factorymode_more_data;

        stream_cfg.data_ptr = BT_AUDIO_CACHE_2_UNCACHE(buff_play);
        stream_cfg.data_size = BT_AUDIO_FACTORMODE_BUFF_SIZE*2;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        APP_FACTORY_TRACE(0,"app_factorymode_audioloop on");
    } else {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        APP_FACTORY_TRACE(0,"app_factorymode_audioloop off");

        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

int app_factorymode_output_pcmpatten(audio_test_pcmpatten_t *pcmpatten, uint8_t *buf, uint32_t len)
{
    uint32_t remain_size = len;
    uint32_t curr_size = 0;

    if (remain_size > pcmpatten->len)
    {
        do{
            if (pcmpatten->cuur_buf_pos)
            {
                curr_size = pcmpatten->len-pcmpatten->cuur_buf_pos;
                memcpy(buf,&(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]), curr_size);
                remain_size -= curr_size;
                pcmpatten->cuur_buf_pos = 0;
            }
            else if (remain_size>pcmpatten->len)
            {
                memcpy(buf+curr_size, pcmpatten->buf, pcmpatten->len);
                curr_size += pcmpatten->len;
                remain_size -= pcmpatten->len;
            }
            else
            {
                memcpy(buf+curr_size,pcmpatten->buf, remain_size);
                pcmpatten->cuur_buf_pos = remain_size;
                remain_size = 0;
            }
        }while(remain_size);
    }
    else
    {
        if ((pcmpatten->len - pcmpatten->cuur_buf_pos) >= len)
        {
            memcpy(buf, &(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]),len);
            pcmpatten->cuur_buf_pos += len;
        }
        else 
        {
            curr_size = pcmpatten->len-pcmpatten->cuur_buf_pos;
            memcpy(buf, &(pcmpatten->buf[pcmpatten->cuur_buf_pos/2]),curr_size);
            pcmpatten->cuur_buf_pos = len - curr_size;
            memcpy(buf+curr_size, pcmpatten->buf, pcmpatten->cuur_buf_pos);                
        }
    }
    
    return 0;
}

#include "fft128dot.h"

#define N 64
#define NFFT 128

struct mic_st_t{
    FftTwiddle_t w[N];
    FftTwiddle_t w128[N*2];
    FftData_t x[N*2];
    FftData_t data_odd[N];
    FftData_t data_even[N];
    FftData_t data_odd_d[N];
    FftData_t data_even_d[N];
    FftData_t data[N*2];
    signed long out[N];
};

int app_factorymode_mic_cancellation_run(void * mic_st, signed short *inbuf, int sample)
{
    struct mic_st_t *st = (struct mic_st_t *)mic_st;
    int i,k,jj,ii;
    //int dataWidth = 16;     // input word format is 16 bit twos complement fractional format 1.15
    int twiddleWidth = 16;  // input word format is 16 bit twos complement fractional format 2.14
    FftMode_t ifft = FFT_MODE;

    make_symmetric_twiddles(st->w,N,twiddleWidth);
    make_symmetric_twiddles(st->w128,N*2,twiddleWidth);
    // input data
    for (i=0; i<sample; i++){
        st->x[i].re = inbuf[i];
        st->x[i].im = 0;
    }

    for(ii = 0; ii < 1; ii++)
    {
      k = 0;
        for (jj = 0; jj < N*2; jj+=2)
        {
            FftData_t tmp;

            tmp.re     = st->x[jj].re;
            tmp.im     = st->x[jj].im;

              st->data_even[k].re = tmp.re;//(int) (double(tmp.re)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              st->data_even[k].im = tmp.im;//(int) (double(tmp.im)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              tmp.re     = st->x[jj+1].re;
              tmp.im     = st->x[jj+1].im;
              st->data_odd[k].re  = tmp.re;//(int) (double(tmp.re)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              st->data_odd[k].im  = tmp.im;//(int) (double(tmp.im)*double(1 << FFTR4_INPUT_FORMAT_Y)) ;
              k++;
        }

        fftr4(NFFT/2, st->data_even, st->w, FFTR4_TWIDDLE_WIDTH, FFTR4_DATA_WIDTH, ifft);
        fftr4(NFFT/2, st->data_odd, st->w, FFTR4_TWIDDLE_WIDTH, FFTR4_DATA_WIDTH, ifft);

        for (jj = 0; jj < NFFT/2; jj++)
        {

        int idx = dibit_reverse_int(jj, NFFT/2);
            st->data_even_d[jj].re = st->data_even[idx].re;
            st->data_even_d[jj].im = st->data_even[idx].im;
            st->data_odd_d[jj].re  = st->data_odd[idx].re;
            st->data_odd_d[jj].im  = st->data_odd[idx].im;
        }
        for (jj=0;jj<NFFT/2;jj++)
        {
          long long mbr,mbi;
          FftData_t ta;
          FftData_t tmp;
          double a;
      mbr = (long long)(st->data_odd_d[jj].re) * st->w128[jj].re - (long long)(st->data_odd_d[jj].im) * st->w128[jj].im;
      mbi = (long long)(st->data_odd_d[jj].im) * st->w128[jj].re + (long long)(st->data_odd_d[jj].re) * st->w128[jj].im;
      ta.re = int(mbr>>(FFTR4_TWIDDLE_WIDTH-2));
      ta.im = int(mbi>>(FFTR4_TWIDDLE_WIDTH-2));
      st->data[jj].re = (st->data_even_d[jj].re + ta.re)/2;
      st->data[jj].im = (st->data_even_d[jj].im + ta.im)/2;
      //data[jj] = sat(data[jj],FFTR4_DATA_WIDTH);
      st->data[jj+NFFT/2].re = (st->data_even_d[jj].re - ta.re)/2;
      st->data[jj+NFFT/2].im = (st->data_even_d[jj].im - ta.im)/2;
      //data[jj+NFFT/2] = sat(data[jj+NFFT/2],FFTR4_DATA_WIDTH);

      a = st->data[jj].re;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.re              = (int)a;
          a = st->data[jj].im;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.im              = (int)a;
            st->x[ii*NFFT+jj].re = (int) tmp.re;
            st->x[ii*NFFT+jj].im = (int) tmp.im;
      a = st->data[jj+NFFT/2].re;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.re              = (int)a;
          a = st->data[jj+NFFT/2].im;///double(1 << FFTR4_OUTPUT_FORMAT_Y);// * double(1 << FFTR4_SCALE);
          tmp.im              = (int)a;
          st->x[ii*NFFT+jj+NFFT/2].re = (int) tmp.re;
            st->x[ii*NFFT+jj+NFFT/2].im = (int) tmp.im;
        }
    }

    for (i=0; i<N; i++){
        st->out[i] = st->x[i].re * st->x[i].re +  st->x[i].im * st->x[i].im;
    }

    return 0;

}

void *app_factorymode_mic_cancellation_init(void* (* alloc_ext)(int))
{
    struct mic_st_t *mic_st;
    mic_st = (struct mic_st_t *)alloc_ext(sizeof(struct mic_st_t));
    return (void *)mic_st;
}

#endif

