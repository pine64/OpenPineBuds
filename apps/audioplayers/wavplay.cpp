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
#ifdef MBED
#include "mbed.h"
#endif
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "hal_uart.h"
#include "hal_trace.h"
#include "hal_timer.h"

/*!
 *  * @brief Standard Winodws PCM wave file header length
 *   */
#define WAVE_FILE_HEADER_SIZE   0x2CU

typedef struct wave_header
{
    uint8_t  riff[4];
    uint32_t size;
    uint8_t  waveFlag[4];
    uint8_t  fmt[4];
    uint32_t fmtLen;
    uint16_t tag;
    uint16_t channels;
    uint32_t sampFreq;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitSamp;
    uint8_t  dataFlag[4];
    uint32_t length;
} wave_header_t;

/*!
 *  * @brief Wave file structure
 *   */
typedef struct wave_file
{
    wave_header_t header;
    uint32_t *data;
}wave_file_t;

/* player */
static unsigned int g_total_play_count = 0;
static unsigned int g_curr_play_index = 0;

/* wave */
wave_file_t g_wave_file_info;
static char g_wav_header[WAVE_FILE_HEADER_SIZE];
FILE *g_wave_file_handle = NULL;
static int32_t (*wav_file_palyback_callback)(int32_t ) = NULL;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////
void wav_file_set_playeback_cb(int32_t (* cb)(int32_t))
{
    wav_file_palyback_callback = cb;
}
bool wav_file_isplaydone(void)
{
    return (g_curr_play_index >= g_total_play_count)? true : false;
}
uint32_t wav_file_audio_more_data(uint8_t *buf, uint32_t len)
{
//    static uint32_t g_preIrqTime = 0;
    uint32_t reallen = 0;
//    int32_t stime,etime;
    int32_t status;

    /* play done ? */
    if(wav_file_isplaydone()) {
        memset(buf, 0, len);
        status = 0;
        if (wav_file_palyback_callback)
             wav_file_palyback_callback(status);
        return (len);
    }
//    stime = hal_sys_timer_get();
    /* read file */
    if (g_wave_file_handle)
        reallen = fread(buf, 1, len, g_wave_file_handle);
//    etime = hal_sys_timer_get();
    if (reallen != len){
        memset(buf, 0, len);
        status = -1;
        if (wav_file_palyback_callback)
             wav_file_palyback_callback(status);
        return (len);
    }

//  TRACE(5,"wav_file_audio_more_data irqDur:%d fsSpend:%d, readbuff:0x%08x %d/%d\n ", TICKS_TO_MS(stime - g_preIrqTime),TICKS_TO_MS(etime - stime),buf,reallen,len);
//    g_preIrqTime = stime;

    /* walk index */
    g_curr_play_index += reallen;

    return reallen;
}

uint32_t get_wav_data(wave_file_t *waveFile)
{
    uint8_t *dataTemp = (uint8_t *)waveFile->data;

    // check for RIFF
    memcpy(waveFile->header.riff, dataTemp, 4);
    dataTemp += 4;
    if( memcmp( (uint8_t*)waveFile->header.riff, "RIFF", 4) )
    {
        return 0;
    }

    // Get size
    memcpy(&waveFile->header.size, dataTemp, 4);
    dataTemp += 4;

    TRACE(1,"WAV header size [%d]\n", waveFile->header.size);

    // .wav file flag
    memcpy(waveFile->header.waveFlag, dataTemp, 4);
    dataTemp += 4;
    if( memcmp( (uint8_t*)waveFile->header.waveFlag, "WAVE", 4) )
    {
        return 0;
    }

    // fmt
    memcpy(waveFile->header.fmt, dataTemp, 4);
    dataTemp += 4;
    if( memcmp( (uint8_t*)waveFile->header.fmt, "fmt ", 4) )
    {
        return 0;
    }

    // fmt length
    memcpy(&waveFile->header.fmtLen, dataTemp, 4);
    dataTemp += 4;

    // Tag: PCM or not
    memcpy(&waveFile->header.tag, dataTemp, 4);
    dataTemp += 2;

    // Channels
    memcpy(&waveFile->header.channels, dataTemp, 4);
    dataTemp += 2;

    TRACE(1,"WAV channels [%d]\n", waveFile->header.channels);

    // Sample Rate in Hz
    memcpy(&waveFile->header.sampFreq, dataTemp, 4);
    dataTemp += 4;
    memcpy(&waveFile->header.byteRate, dataTemp, 4);
    dataTemp += 4;

    TRACE(1,"WAV sample_rate [%d]\n", waveFile->header.sampFreq);
    TRACE(1,"WAV byteRate [%d]\n", waveFile->header.byteRate);

    // quantize bytes for per samp point
    memcpy(&waveFile->header.blockAlign, dataTemp, 4);
    dataTemp += 2;
    memcpy(&waveFile->header.bitSamp, dataTemp, 4);
    dataTemp += 2;

    TRACE(1,"WAV bitSamp [%d]\n", waveFile->header.bitSamp);

    // Data
    memcpy(waveFile->header.dataFlag, dataTemp, 4);
    dataTemp += 4;
    if( memcmp( (uint8_t*)waveFile->header.dataFlag, "data ", 4) )
    {
        return 0;
    }
    memcpy(&waveFile->header.length, dataTemp, 4);
    dataTemp += 4;

    return 0;
}


void audio_wav_init(wave_file_t *newWav)
{
    get_wav_data(newWav);

    // Configure the play audio g_format
    //g_format.bits = newWav->header.bitSamp;
    //g_format.sample_rate = newWav->header.sampFreq;
    //g_format.mclk = 256 * g_format.sample_rate ;
    //g_format.mono_streo = (sai_mono_streo_t)((newWav->header.channels) - 1);
}

uint32_t play_wav_file(char *file_path)
{
    uint32_t bytesToRead    = 0;

    wave_file_t *newWav = &g_wave_file_info;

    memset(&g_wave_file_info, 0, sizeof(g_wave_file_info));

    g_wave_file_handle = fopen(file_path, "rb");

    if(g_wave_file_handle == NULL) {
        TRACE(1,"WAV file %s open fail\n", file_path);
        return 1;
    }

    fread(&g_wav_header, WAVE_FILE_HEADER_SIZE, 1, g_wave_file_handle);

    newWav->data = (uint32_t *)&g_wav_header;

    audio_wav_init(newWav);

    // Remove header size from byte count
    // Adjust note duration by divider value, wav tables in pcm_data.h are 200ms by default
    bytesToRead = (newWav->header.length - WAVE_FILE_HEADER_SIZE);

    g_curr_play_index  = 0;
    g_total_play_count = bytesToRead;

    return newWav->header.sampFreq;
}

uint32_t stop_wav_file(void)
{
    memset(&g_wave_file_info, 0, sizeof(g_wave_file_info));
    g_curr_play_index = 0;
    g_total_play_count = 0;
    if (g_wave_file_handle){
        fclose(g_wave_file_handle);
        g_wave_file_handle = NULL;
    }
    if (wav_file_palyback_callback)
        wav_file_palyback_callback = NULL;

    return 0;
}
