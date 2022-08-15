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
//#include "app_thread.h"

//#include "hal_sdmmc.h"
#include "SDFileSystem.h"
#include "audioflinger.h"
#include "audiobuffer.h"
#include "app_sdmmc.h"


#define APP_TEST_PLAYBACK_BUFF_SIZE     (120 * 20)
#define APP_TEST_CAPTURE_BUFF_SIZE      (120 * 20)
extern uint8_t app_test_playback_buff[APP_TEST_PLAYBACK_BUFF_SIZE] __attribute__ ((aligned(4)));
extern uint8_t app_test_capture_buff[APP_TEST_CAPTURE_BUFF_SIZE] __attribute__ ((aligned(4)));
SDFileSystem sdfs("sd");

int sd_open()
{
    DIR *d = opendir("/sd");
    if (!d)
    {
        TRACE(0,"sd file system borked\n");
        return -1;
    }

    TRACE(0,"---------root---------\n");
    struct dirent *p;
    while ((p = readdir(d)))
    {
        int len = sizeof( dirent);
        TRACE(2,"%s %d\n", p->d_name, len);
    }
    closedir(d);
    TRACE(0,"--------root end-------\n");
}

extern uint32_t play_wav_file(char *file_path);
extern uint32_t stop_wav_file(void);
extern uint32_t wav_file_audio_more_data(uint8_t *buf, uint32_t len);

void test_wave_play(bool  on)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
	uint32_t reallen;
	uint32_t totalreadsize;
    uint32_t stime, etime;

    char wave[] = "/sd/test_music.wav";

	static bool isRun =  false;

	if (isRun==on)
		return;
	else
		isRun=on;


	TRACE(2,"%s %d\n", __func__, on);
    memset(&stream_cfg, 0, sizeof(stream_cfg));
	if (on){
	    play_wav_file(wave);

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = 0x03;

        stream_cfg.handler = wav_file_audio_more_data;
        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;

	    af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
	    af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
	}else{
		stop_wav_file();
		af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
		af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
	}

    return;
}


FIL pcm_fil;
FRESULT pcm_res;
UINT pcm_num;
uint32_t pcm_save_more_data(uint8_t *buf, uint32_t len)
{
//    TRACE(2,"%s\n len:%d", __func__, len);

	audio_buffer_set_stereo2mono_16bits(buf, len, 1);
	pcm_res = f_write(&pcm_fil,(uint8_t *)buf,len>>1,&pcm_num);
	if(pcm_res != FR_OK)
	{
		TRACE(2,"[%s]:error-->res = %d", __func__, pcm_res);
	}
    return 0;
}

void  ad_tester(bool run)
{
    char filename[] = "/sd/audio_dump.bin";

    struct AF_STREAM_CONFIG_T stream_cfg;

    TRACE(2,"%s %d\n", __func__, run);

    if (run){
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        pcm_res = f_open(&pcm_fil,"test2.bin",FA_CREATE_ALWAYS | FA_WRITE);
        if (pcm_res) {
        	TRACE(2,"[%s]:Cannot creat test2.bin...%d",__func__, pcm_res);
            return;
        }

        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_2;
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;

        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.vol = 0x03;

        stream_cfg.handler = pcm_save_more_data;
        stream_cfg.data_ptr = app_test_playback_buff;
        stream_cfg.data_size = APP_TEST_PLAYBACK_BUFF_SIZE;

        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        osDelay(1000);

       	f_close(&pcm_fil);
    }
}

// if dump data into sd, buffer length should make sd card speed enough
// Bench32.exe can test sd card speed in PC, then make sure bufer length, buffer length < 16k(sd driver)
void dump_data2sd(enum APP_SDMMC_DUMP_T opt, uint8_t *buf, uint32_t len)
{
    static FIL sd_fil;
    FRESULT res;

    
    ASSERT(opt < APP_SDMMC_DUMP_NUM, "[%s] opt(%d) >= APP_SDMMC_DUMP_NUM", __func__, opt);
    
    if(opt == APP_SDMMC_DUMP_OPEN)
    {
//        res = f_open(&sd_fil,"dump.bin",FA_CREATE_ALWAYS | FA_WRITE);
        res = f_open(&sd_fil,"test.txt",FA_READ);
        
//        ASSERT(pcm_res == FR_OK,"[%s]:Cannot creat dump.bin, res = %d",__func__, pcm_res);
    }
    if(opt == APP_SDMMC_DUMP_READ)
    {
        res = f_read(&sd_fil, buf, len, &pcm_num);
        
//        ASSERT(pcm_res == FR_OK,"[%s]:Cannot creat dump.bin, res = %d",__func__, pcm_res);
    }
    else if(opt == APP_SDMMC_DUMP_WRITE)
    {
        res = f_write(&sd_fil, buf, len, &pcm_num);

//        ASSERT(pcm_res == FR_OK,"[%s]:Write dump.bin failed, res = %d", __func__, pcm_res);
    }
    else if(opt == APP_SDMMC_DUMP_CLOSE)
    {
        res = f_close(&sd_fil);  
    }

    if(res == FR_OK)
    {
        TRACE(3,"[%s] SUCESS: opt = %d, res = %d",__func__, opt, res);
    }
    else
    {
        TRACE(3,"[%s] ERROR: opt = %d, res = %d",__func__, opt, res);
    }
}
