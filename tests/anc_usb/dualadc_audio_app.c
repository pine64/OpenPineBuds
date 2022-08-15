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
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_aud.h"
#include "string.h"
#include "pmu.h"
#include "analog.h"
#include "audioflinger.h"


static uint8_t *playback_buf;
static uint32_t playback_size;

static uint8_t *capture_buf;
static uint32_t capture_size;


/*
static void get_mic_data_max(short *buf, uint32_t len)
{
    int max0 = -32768, min0 = 32767, diff0 = 0;
    int max1 = -32768, min1 = 32767, diff1 = 0;
	
    for(uint32_t i=0; i<len/2;i+=2)
    {
        if(buf[i+0]>max0)
        {
            max0 = buf[i+0];
        }

        if(buf[i+0]<min0)
        {
            min0 = buf[i+0];
        }

        if(buf[i+1]>max1)
        {
            max1 = buf[i+1];
        }

        if(buf[i+1]<min1)
        {
            min1 = buf[i+1];
        }
    }
    //TRACE(6,"min0 = %d, max0 = %d, diff0 = %d, min1 = %d, max1 = %d, diff1 = %d", min0, max0, max0 - min0, min1, max1, max1 - min1);
    TRACE(2,"diff0 = %d,  diff1 = %d", max0 - min0, max1 - min1);
}
*/

static uint32_t dualadc_audio_data_playback(uint8_t *buf, uint32_t len)
{

TRACE(0,"play");
#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - playback_buf;
    if (pos >= playback_size) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE(4,"%s irqDur:%d Len:%d pos:%d", __func__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif
    return 0;
}

#if 1
#define MWSPT_NSEC 3
//static const int NL[MWSPT_NSEC] = { 1, 3, 1 };
static const float NUM[MWSPT_NSEC][3] = {
	{
		0.0002616526908, 0, 0
	},
	{
		1, 2, 1
	},
	{
		1, 0, 0
	}
};


//static const int DL[MWSPT_NSEC] = { 1, 3, 1 };
static const float DEN[MWSPT_NSEC][3] = {
	{
		1, 0, 0
	},
	{
		1, -1.953727961, 0.9547745585
	},
	{
		1, 0, 0
	}
};

#else 

#define MWSPT_NSEC 3
static const int NL[MWSPT_NSEC] = { 1,3,1 };
static const float NUM[MWSPT_NSEC][3] = {
  {
  0.0002616526908,              0,              0 
  },
  {
                1,              2,              1 
  },
  {
                1,              0,              0 
  }
};
static const int DL[MWSPT_NSEC] = { 1,3,1 };
static const float DEN[MWSPT_NSEC][3] = {
  {
                1,              0,              0 
  },
  {
                1,   -1.953727961,   0.9547745585 
  },
  {
                1,              0,              0 
  }
};

#endif

static uint32_t dualadc_audio_data_capture(uint8_t *buf, uint32_t len)
{

TRACE(0,"capture");



#if 0
    if(buf==capture_buf)
    {
		short *BufSrcL = (short *)capture_buf;
		short *BufSrcR = (short *)(capture_buf+2);
		short *BufDstL = (short *)playback_buf;
		short *BufDstR = (short *)(playback_buf+2);

		for(int i=0,j=0;i<playback_size/4;i=i+2,j=j+4)
		{
			BufDstL[i]=BufSrcL[j];
			BufDstR[i]=BufSrcR[j];
			
		}

    }
   else
   {
		short *BufSrcL = (short *)(capture_buf+capture_size/2);
		short *BufSrcR = (short *)(capture_buf+capture_size/2+2);
		short *BufDstL = (short *)(playback_buf+playback_size/2);
		short *BufDstR = (short *)(playback_buf+playback_size/2+2);

		for(int i=0,j=0;i<playback_size/4;i=i+2,j=j+4)
		{
			BufDstL[i]=BufSrcL[j];
			BufDstR[i]=BufSrcR[j];
			
		}
   
   }


#else

	short *BufSrcL;
	short *BufSrcR;
	short *BufDstL;
	short *BufDstR;


	int32_t PcmValue;
	int32_t OutValue;


	//get_mic_data_max(buf,len);

	    if(buf==capture_buf)
	    {
			BufSrcL = (short *)(capture_buf);
			BufSrcR = (short *)(capture_buf+2);
			BufDstL = (short *)playback_buf;
			BufDstR = (short *)(playback_buf+2);

	    }
	   else
	   {
			BufSrcL = (short *)(capture_buf+capture_size/2);
			BufSrcR = (short *)(capture_buf+capture_size/2+2);
			BufDstL = (short *)(playback_buf+playback_size/2);
			BufDstR = (short *)(playback_buf+playback_size/2+2);

	   
	   }
		


	for(int i=0,j=0;i<capture_size/4;i=i+4,j=j+2)
	{
		PcmValue = BufSrcL[i];

		if (PcmValue>32600)
		{
			OutValue = ((int)BufSrcR[i]) << 6;
		}
		else if (PcmValue>(32600 / 2) && PcmValue<32600)
		{
			if (BufSrcR[i]>512)
			{
				OutValue = PcmValue*(32600 - PcmValue) + (((int)BufSrcR[i]) << 6)*(PcmValue - (32600 / 2));
				OutValue = OutValue / (32600 / 2);
			}
			else
			{
				OutValue = PcmValue;
			}
		}
		else if (PcmValue<-32700)
		{
			OutValue = ((int)BufSrcR[i]) << 6;
		}
		else if (PcmValue >-32700 && PcmValue < -(32700 / 2))
		{

			if (BufSrcR[i] <-512)
			{
				OutValue = PcmValue*(-PcmValue - (32700 / 2)) + (((int)BufSrcR[i]) << 6)*(32700 + PcmValue);
				OutValue= OutValue/ (32700 / 2);
			}
			else
			{
				OutValue = PcmValue;
			}

		}
		else
		{
			OutValue= PcmValue;
		}	

		OutValue=OutValue>>6;
	//	OutValue=BufSrcR[i];


		{
			static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;

			x0 = (OutValue* NUM[0][0]);

			y0 = x0*NUM[1][0] + x1*NUM[1][1] + x2*NUM[1][2] - y1*DEN[1][1] - y2*DEN[1][2];

			y2 = y1;
			y1 = y0;
			x2 = x1;
			x1 = x0;

			if (y0 > 32767.0f)
			{
				y0 = 32767.0f;
			}

			if (y0 < -32768.0f)
			{
				y0 = -32768.0f;
			}

			OutValue = (short)y0;
		}			
		

		BufDstL[j]=OutValue;
		BufDstR[j]=OutValue;
	}		


#endif






#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - capture_buf;
    if (pos >= capture_size) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE(4,"%s irqDur:%d Len:%d pos:%d", __func__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif
    return 0;
}



void dualadc_audio_app(bool on)
{

    struct AF_STREAM_CONFIG_T stream_cfg;
    enum AUD_SAMPRATE_T sample_rate_play;
    enum AUD_SAMPRATE_T POSSIBLY_UNUSED sample_rate_capture;

    static bool isRun = false;

    if (isRun==on)
        return;
    else
        isRun=on;

    TRACE(2,"%s: on=%d", __FUNCTION__, on);


    sample_rate_play = AUD_SAMPRATE_192000;
    sample_rate_capture = AUD_SAMPRATE_192000;


    if (on){

        memset(&stream_cfg, 0, sizeof(stream_cfg));


        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate_play;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = 0x06;

        stream_cfg.handler = dualadc_audio_data_playback;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;

        stream_cfg.data_ptr = playback_buf;
        stream_cfg.data_size = playback_size;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);

		
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = sample_rate_capture;
        stream_cfg.vol = 0x01;

        stream_cfg.handler = dualadc_audio_data_capture;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;

        stream_cfg.data_ptr = capture_buf;
        stream_cfg.data_size = capture_size;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);

        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

    }else{

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
}

void dualadc_audio_app_init(uint8_t *play_buf, uint32_t play_size, uint8_t *cap_buf, uint32_t cap_size)
{
    playback_buf = play_buf;
    playback_size = play_size;
    capture_buf = cap_buf;
    capture_size = cap_size;
}

void dualadc_audio_app_term(void)
{
    playback_buf = NULL;
    playback_size = 0;
    capture_buf = NULL;
    capture_size = 0;
}

