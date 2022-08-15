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
// Standard C Included Files
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "plat_types.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "cqueue.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_ring_merge.h"
#include "tgt_hardware.h"
#include "bt_sco_chain.h"
#include "iir_resample.h"
#include "hfp_api.h"
#include "audio_prompt_sbc.h"
#ifdef TX_RX_PCM_MASK
#include "bt_drv_interface.h"
#endif

#define ENABLE_LPC_PLC

#define ENABLE_PLC_ENCODER

// BT
#include "a2dp_api.h"
#ifdef TX_RX_PCM_MASK
#include "hal_chipid.h"
#include "bt_drv_interface.h"

#endif

#include "plc_utils.h"
extern "C" {

#include "plc_8000.h"
#include "speech_utils.h"

#if defined(HFP_1_6_ENABLE)
#include "codec_sbc.h"
#if defined(ENABLE_LPC_PLC)
#include "lpc_plc_api.h"
#else
#include "plc_16000.h"
#endif
#endif

#if defined(CVSD_BYPASS)
#include "Pcm8k_Cvsd.h"
#endif

#if defined(SPEECH_TX_24BIT)
extern int32_t *aec_echo_buf;
#else
extern short *aec_echo_buf;
#endif

#if defined( SPEECH_TX_AEC ) || defined( SPEECH_TX_AEC2 ) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
static uint8_t *echo_buf_queue;
static uint16_t echo_buf_q_size;
static uint16_t echo_buf_q_wpos;
static uint16_t echo_buf_q_rpos;
static bool echo_buf_q_full;
#endif
#endif
static  void *speech_plc;
}

//#define PENDING_MSBC_DECODER_ALG

// #define SPEECH_RX_PLC_DUMP_DATA

#ifdef SPEECH_RX_PLC_DUMP_DATA
#include "audio_dump.h"
int16_t *audio_dump_temp_buf = NULL;
#endif

// app_bt_stream.cpp::bt_sco_player(), used buffer size
#define APP_BT_STREAM_USE_BUF_SIZE      (1024*2)
#if defined(SCO_OPTIMIZE_FOR_RAM)
uint8_t *sco_overlay_ram_buf = NULL;
int sco_overlay_ram_buf_len = 0;
#endif

static bool resample_needed_flag = false;
static int sco_frame_length;
static int codec_frame_length;
static int16_t *upsample_buf_for_msbc = NULL;
static int16_t *downsample_buf_for_msbc = NULL;
static IirResampleState *upsample_st = NULL;
static IirResampleState *downsample_st = NULL;

#define VOICEBTPCM_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

/* voicebtpcm_pcm queue */
#define FRAME_NUM (4)

#define VOICEBTPCM_PCM_16k_FRAME_LENGTH (SPEECH_FRAME_MS_TO_LEN(16000, SPEECH_SCO_FRAME_MS))
#define VOICEBTPCM_PCM_16K_QUEUE_SIZE (VOICEBTPCM_PCM_16k_FRAME_LENGTH*FRAME_NUM)

#define VOICEBTPCM_PCM_8k_FRAME_LENGTH (SPEECH_FRAME_MS_TO_LEN(8000, SPEECH_SCO_FRAME_MS))
#define VOICEBTPCM_PCM_8K_QUEUE_SIZE (VOICEBTPCM_PCM_8k_FRAME_LENGTH*FRAME_NUM)

// #endif
CQueue voicebtpcm_p2m_pcm_queue;
CQueue voicebtpcm_m2p_pcm_queue;

static int32_t voicebtpcm_p2m_cache_threshold;
static int32_t voicebtpcm_m2p_cache_threshold;

static int32_t voicebtpcm_p2m_pcm_cache_size;
static int32_t voicebtpcm_m2p_pcm_cache_size;

static enum APP_AUDIO_CACHE_T voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_QTY;
static enum APP_AUDIO_CACHE_T voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_QTY;

extern bool bt_sco_codec_is_msbc(void);
#define ENCODE_TEMP_PCM_LEN (120)

#define MSBC_FRAME_SIZE (60)
#if defined(HFP_1_6_ENABLE)
static btif_sbc_decoder_t msbc_decoder;
#if FPGA==1
#define CFG_HW_AUD_EQ_NUM_BANDS (8)
const int8_t cfg_hw_aud_eq_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif
static float msbc_eq_band_gain[CFG_HW_AUD_EQ_NUM_BANDS]= {0,0,0,0,0,0,0,0};

#define MSBC_ENCODE_PCM_LEN (240)

unsigned char *temp_msbc_buf;
unsigned char *temp_msbc_buf1;

#if defined(ENABLE_LPC_PLC)
LpcPlcState *msbc_plc_state = NULL;
#else
struct PLC_State msbc_plc_state;
#endif

#ifdef ENABLE_PLC_ENCODER
static btif_sbc_encoder_t *msbc_plc_encoder;
static int16_t *msbc_plc_encoder_buffer = NULL;
#define MSBC_CODEC_DELAY (73)
static uint8_t enc_tmp_buf[MSBC_FRAME_SIZE - 3];
static uint8_t dec_tmp_buf[MSBC_ENCODE_PCM_LEN];
#endif

//static sbc_encoder_t sbc_Encoder1;
//static btif_sbc_pcm_data_t PcmEncData1;
static btif_sbc_encoder_t *msbc_encoder;
static btif_sbc_pcm_data_t msbc_encoder_pcmdata;
static unsigned char msbc_counter = 0x08;
#endif

int decode_msbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len);
int decode_cvsd_frame(unsigned char *pcm_buffer, unsigned int pcm_len);

//playback flow
//bt-->store_voicebtpcm_m2p_buffer-->decode_voicebtpcm_m2p_frame-->audioflinger playback-->speaker
//used by playback, store data from bt to memory
int store_voicebtpcm_m2p_buffer(unsigned char *buf, unsigned int len)
{
    //TRACE(2,"[%s]: %d", __FUNCTION__, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
    int size;
#if defined(HFP_1_6_ENABLE)
    if (bt_sco_codec_is_msbc())
    {
        decode_msbc_frame(buf, len);
    }
    else
#endif
    {
#if defined(CVSD_BYPASS)
        decode_cvsd_frame(buf, len);
#endif
    if(speech_plc){
        speech_plc_8000((PlcSt_8000 *)speech_plc, (short *)buf, len);
        }
        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_pcm_queue, (unsigned char *)buf, len);
        UNLOCK_APP_AUDIO_QUEUE();
    }

    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_m2p_pcm_queue);

    if (size >= voicebtpcm_m2p_cache_threshold)
    {
        voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_OK;
    }

    //TRACE(2,"m2p :%d/%d", len, size);

    return 0;
}


#if defined(CVSD_BYPASS)
#define VOICECVSD_TEMP_BUFFER_SIZE 120
#define VOICECVSD_ENC_SIZE 60
static short cvsd_decode_buff[VOICECVSD_TEMP_BUFFER_SIZE*2];

int decode_cvsd_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    uint32_t r = 0, decode_len = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;

    while (decode_len < pcm_len)
    {
        LOCK_APP_AUDIO_QUEUE();
        len1 = len2 = 0;
        e1 = e2 = 0;
        r = APP_AUDIO_PeekCQueue(&voicebtpcm_m2p_pcm_queue, VOICECVSD_TEMP_BUFFER_SIZE, &e1, &len1, &e2, &len2);
        UNLOCK_APP_AUDIO_QUEUE();

        if (r == CQ_ERR)
        {
            memset(pcm_buffer, 0, pcm_len);
            TRACE(0,"cvsd spk buff underflow");
            return 0;
        }

        if (len1 != 0)
        {
            CvsdToPcm8k(e1, (short *)(cvsd_decode_buff), len1, 0);

            LOCK_APP_AUDIO_QUEUE();
            DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, len1);
            UNLOCK_APP_AUDIO_QUEUE();

            decode_len += len1*2;
        }

        if (len2 != 0)
        {
            CvsdToPcm8k(e2, (short *)(cvsd_decode_buff), len2, 0);

            LOCK_APP_AUDIO_QUEUE();
            DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, len2);
            UNLOCK_APP_AUDIO_QUEUE();

            decode_len += len2*2;
        }
    }

    memcpy(pcm_buffer, cvsd_decode_buff, decode_len);

    return decode_len;
}

int encode_cvsd_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    uint32_t r = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    uint32_t processed_len = 0;
    uint32_t remain_len = 0, enc_len = 0;

    while (processed_len < pcm_len)
    {
        remain_len = pcm_len-processed_len;

        if (remain_len>=(VOICECVSD_ENC_SIZE*2))
        {
            enc_len = VOICECVSD_ENC_SIZE*2;
        }
        else
        {
            enc_len = remain_len;
        }

        LOCK_APP_AUDIO_QUEUE();
        len1 = len2 = 0;
        e1 = e2 = 0;
        r = APP_AUDIO_PeekCQueue(&voicebtpcm_p2m_pcm_queue, enc_len, &e1, &len1, &e2, &len2);
        UNLOCK_APP_AUDIO_QUEUE();

        if (r == CQ_ERR)
        {
            memset(pcm_buffer, 0x55, pcm_len);
            TRACE(0,"cvsd spk buff underflow");
            return 0;
        }

        if (e1)
        {
            Pcm8kToCvsd((short *)e1, (unsigned char *)(pcm_buffer + processed_len), len1/2);
            LOCK_APP_AUDIO_QUEUE();
            DeCQueue(&voicebtpcm_p2m_pcm_queue, NULL, len1);
            UNLOCK_APP_AUDIO_QUEUE();
            processed_len += len1;
        }
        if (e2)
        {
            Pcm8kToCvsd((short *)e2, (unsigned char *)(pcm_buffer + processed_len), len2/2);
            LOCK_APP_AUDIO_QUEUE();
            DeCQueue(&voicebtpcm_p2m_pcm_queue, NULL, len2);
            UNLOCK_APP_AUDIO_QUEUE();
            processed_len += len2;
        }
    }

#if 0
    for (int cc = 0; cc < 32; ++cc)
    {
        TRACE(1,"%x-", e1[cc]);
    }
#endif

    TRACE(3,"%s: processed_len %d, pcm_len %d", __func__, processed_len, pcm_len);

    return processed_len;
}
#endif

#if defined(HFP_1_6_ENABLE)
inline int sco_parse_synchronization_header(uint8_t *buf, uint8_t *sn)
{
    uint8_t sn1, sn2;
    *sn = 0xff;
    if ((buf[0] != 0x01) ||
        ((buf[1]&0x0f) != 0x08) ||
        (buf[2] != 0xad)){
        return -1;
    }

    sn1 = (buf[1]&0x30)>>4;
    sn2 = (buf[1]&0xc0)>>6;
    if ((sn1 != 0) && (sn1 != 0x3)){
        return -2;
    }
    if ((sn2 != 0) && (sn2 != 0x3)){
        return -3;
    }

    *sn = (sn1&0x01)|(sn2&0x02);

    return 0;
}

#if 1
#define MSBC_LEN_FORMBT_PER_FRAME (120) //Bytes; only for BES platform.
#define SAMPLES_LEN_PER_FRAME (120)
#define MSBC_LEN_PER_FRAME (57+3)

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) || defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A) || defined(CHIP_BEST2001) 
#define MSBC_MUTE_PATTERN (0x5555)
#else
#define MSBC_MUTE_PATTERN (0x0000)
#endif

#if !defined(PENDING_MSBC_DECODER_ALG)
short DecPcmBuf[SAMPLES_LEN_PER_FRAME];
unsigned char DecMsbcBuf[MSBC_LEN_PER_FRAME];
unsigned short DecMsbcBufAll[MSBC_LEN_FORMBT_PER_FRAME*5];
unsigned int next_frame_flag = 0;
static int msbc_find_first_sync = 0;
static unsigned int msbc_offset = 0;

static PacketLossState pld;

int decode_msbc_frame(unsigned char *msbc_btpcm_buffer, unsigned int msbc_len)
{

    btif_sbc_pcm_data_t pcm_data;
    unsigned int msbc_offset_lowdelay = 0;
    unsigned int i,j;
    unsigned short *msbc_buffer=(unsigned short *)msbc_btpcm_buffer;
    int frame_flag[6];  // 1: good frame; 0:bad frame;
    bt_status_t ret;
    unsigned int frame_counter=0;
    unsigned short byte_decode = 0;
    unsigned int msbc_offset_total = 0;
    int msbc_offset_drift[6] = {0, };

    //unsigned int timer_begin=hal_sys_timer_get();

    //TRACE(1,"decode_msbc_frame,msbc_len:%d",msbc_len);
#ifdef TX_RX_PCM_MASK
    if(btdrv_is_pcm_mask_enable() ==1)
    {
        memcpy((uint8_t *)DecMsbcBufAll,msbc_buffer,msbc_len);
    }
    else
#endif
    {
        for(i =0; i<msbc_len/2; i++)
        {
            DecMsbcBufAll[i+(MSBC_LEN_FORMBT_PER_FRAME/2)]=msbc_buffer[i];
        }
    }

    /*
    for(i =0; i<msbc_len/2; i=i+10)
    {
       TRACE(10,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,",
       DecMsbcBufAll[i],DecMsbcBufAll[i+1],DecMsbcBufAll[i+2],DecMsbcBufAll[i+3],DecMsbcBufAll[i+4],
       DecMsbcBufAll[i+5],DecMsbcBufAll[i+6],DecMsbcBufAll[i+7],DecMsbcBufAll[i+8],DecMsbcBufAll[i+9]);
    }
    */

    for(j =0; j<msbc_len/MSBC_LEN_FORMBT_PER_FRAME; j++)
    {
        frame_flag[j]=0;
    }

    if(msbc_find_first_sync==0)
    {
        for(i =0; i<(msbc_len/2); i++)
        {
            if((DecMsbcBufAll[i]==0x0100)&&((DecMsbcBufAll[i+1]&0x0f00)==0x0800)&&(DecMsbcBufAll[i+2]==0xad00))break;
        }

        TRACE(1,"sync......:%d",i);
        DUMP16("0x%04x, ", &DecMsbcBufAll[0], msbc_len / 2);

        if(i<(msbc_len/2))
        {
            msbc_find_first_sync=1;
            msbc_offset=i%(MSBC_LEN_FORMBT_PER_FRAME/2);
#ifdef TX_RX_PCM_MASK
            if(btdrv_is_pcm_mask_enable() ==0)
#endif
            {
                msbc_offset_total=i;
            }
            TRACE(2,"[%s] msbc header found, offset %d", __FUNCTION__, msbc_offset);
        }
        else
        {
            for (j = 0; j < msbc_len / MSBC_LEN_FORMBT_PER_FRAME; j++)
            {
                frame_flag[j] = 1;
            }
        }
    }

    if(msbc_find_first_sync==1)
    {
#if 1//def TX_RX_PCM_MASK

            int value=0;
            unsigned short headerm1 = 0;
            unsigned short header0 = 0;
            unsigned short header1 = 0;
            unsigned short header2 = 0;
            unsigned short header3 = 0;
            //unsigned short tail0 = 0;
#endif

        if(msbc_offset==0||msbc_offset==1)
        {
            msbc_offset_lowdelay=msbc_offset+60;
            //msbc_offset_lowdelay=msbc_offset;
        }
         else
        {
            msbc_offset_lowdelay=msbc_offset;
        }

        //check again
        for(j =0; j<(msbc_len/MSBC_LEN_FORMBT_PER_FRAME); j++)
        {

#ifdef TX_RX_PCM_MASK
            if(btdrv_is_pcm_mask_enable() ==1)
            {
                if((DecMsbcBufAll[j*60]==0x0100)&&((DecMsbcBufAll[j*60+1]&0x0f00)==0x0800)&&(DecMsbcBufAll[j*60+2]==0xad00))
                {
                    frame_flag[j] = 0;
                }
                else
                {
                    frame_flag[j] = 1;
                }
            }
            else
#endif
            {

                if (next_frame_flag == 1)
                {
                    next_frame_flag = 0;
                    frame_flag[j] = 1;
                    continue;
                }
                if(msbc_offset_lowdelay==0&&j==0)
                {
                    headerm1=0x0100;
                }
                else
                {
                    headerm1 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2)-1];
                }

                header0 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2)];
                header1 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 1];
                header2 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 2];
                header3 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 3];

                /*if ((headerm1 == 0x0100) && ((header0 & 0x0f00) == 0x0800) && (header1 == 0xad00) ||
                    (header0 == 0x0100) && ((header1 & 0x0f00) == 0x0800) && (header2 == 0xad00) ||
                    (header1 == 0x0100) && ((header2 & 0x0f00) == 0x0800) && (header3 == 0xad00))
                {
                    frame_flag[j] = 0;
                }*/

                if ((headerm1 == 0x0100) && ((header0 & 0x0f00) == 0x0800) && (header1 == 0xad00))
                {
                    frame_flag[j] = 0;
                    // It seems that offset is reduced by 1
                    msbc_offset_drift[j] = -1;
                    TRACE(1,"[%s] msbc_offset is reduced by 1", __FUNCTION__);
        /*          tail0 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 59 - 1];
                    if (tail0 == 0x0000 || tail0 == 0x0100|| tail0==0xff00)
                    {
                        frame_flag[j] = 0;
                    }
                    else
                    {
                        frame_flag[j] = 1;
                        next_frame_flag = 1;
                    }*/
                }
                else if ((header0 == 0x0100) && ((header1 & 0x0f00) == 0x0800) && (header2 == 0xad00))
                {
                frame_flag[j] = 0;
/*              tail0 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 59];
                    if (tail0 == 0x0000 || tail0 == 0x0100|| tail0==0xff00)
                    {
                        frame_flag[j] = 0;
                    }
                    else
                    {
                        frame_flag[j] = 1;
                        next_frame_flag = 1;
                    }
                    */
                }
                else if ((header1 == 0x0100) && ((header2 & 0x0f00) == 0x0800) && (header3 == 0xad00))
                {
                frame_flag[j] = 0;
                msbc_offset_drift[j] = 1;
                TRACE(1,"[%s] msbc_offset is increased by 1", __FUNCTION__);
/*              tail0 = DecMsbcBufAll[msbc_offset_lowdelay + j*(MSBC_LEN_FORMBT_PER_FRAME / 2) + 59 + 1];
                    if (tail0 == 0x0000 || tail0==0x0100|| tail0==0xff00)
                    {
                        frame_flag[j] = 0;
                    }
                    else
                    {
                        frame_flag[j] = 1;
                        next_frame_flag = 1;
                    }*/
                }
                else
                {
                    if ((header0 == MSBC_MUTE_PATTERN)&& ((header1 & 0x0f00) == MSBC_MUTE_PATTERN) && (header2 == MSBC_MUTE_PATTERN))
                    {
                        frame_flag[j]=1;
                    }
                    else
                    {
                        if((msbc_offset_lowdelay+j*(MSBC_LEN_FORMBT_PER_FRAME / 2))>=msbc_offset_total)
                            frame_flag[j]=3;
                        else
                            frame_flag[j]=1;
                    }
                }
            }

        }
#ifdef TX_RX_PCM_MASK
        if(btdrv_is_pcm_mask_enable() ==0)
#endif
        {
            for(j =0; j<msbc_len/MSBC_LEN_FORMBT_PER_FRAME; j++)
            {
                value=value|frame_flag[j];
            }
            //abnormal msbc packet.
            if(value>1)msbc_find_first_sync=0;
        }
    }

    while(frame_counter<msbc_len/MSBC_LEN_FORMBT_PER_FRAME)
    {
        //TRACE(3,"[%s] decoding, offset %d, offset drift %d", __FUNCTION__, msbc_offset, msbc_offset_drift[frame_counter]);
        // skip first byte when msbc_offset == 0 and msbc_offset_drift == -1
        unsigned int start_idx = 0;
        if (msbc_offset_lowdelay == 0 && msbc_offset_drift[frame_counter] == -1) {
            start_idx = 1;
            DecMsbcBuf[0] = 0x01;
        }
        for(i = start_idx; i<MSBC_LEN_PER_FRAME; i++)
        {
            //DecMsbcBuf[i]=DecMsbcBufAll[i+msbc_offset_lowdelay+frame_counter*(MSBC_LEN_FORMBT_PER_FRAME/2)+2]>>8;
            DecMsbcBuf[i]=DecMsbcBufAll[i+msbc_offset_lowdelay+msbc_offset_drift[frame_counter]+frame_counter*(MSBC_LEN_FORMBT_PER_FRAME/2)]>>8;
        }

        //TRACE(1,"msbc header:0x%x",DecMsbcBuf[0]);

#ifdef SPEECH_RX_PLC_DUMP_DATA
        audio_dump_add_channel_data(2, (short *)DecMsbcBuf, MSBC_LEN_PER_FRAME/2);
#endif

        plc_type_t plc_type = packet_loss_detection_process(&pld, DecMsbcBuf);

        if(plc_type != PLC_TYPE_PASS) {
            memset(DecPcmBuf, 0, sizeof(DecPcmBuf));
            goto do_plc;
        }

        pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
        pcm_data.numChannels = 1;
        pcm_data.dataLen = 0;
        pcm_data.data = (uint8_t *)DecPcmBuf;

        ret = btif_sbc_decode_frames(&msbc_decoder,
                (unsigned char *)DecMsbcBuf,
                MSBC_LEN_PER_FRAME, &byte_decode,
                &pcm_data, SAMPLES_LEN_PER_FRAME*2,
                msbc_eq_band_gain);

        //ASSERT(ret == BT_STS_SUCCESS, "[%s] msbc decoder should never fail", __FUNCTION__);
        if (ret != BT_STS_SUCCESS) {
            plc_type = PLC_TYPE_DECODER_ERROR;
            packet_loss_detection_update_histogram(&pld, plc_type);
        }

do_plc:
        if (plc_type == PLC_TYPE_PASS) {
#if defined(ENABLE_LPC_PLC)
            lpc_plc_save(msbc_plc_state, DecPcmBuf);
#else
            PLC_good_frame(&msbc_plc_state, DecPcmBuf, DecPcmBuf);
#endif
#ifdef SPEECH_RX_PLC_DUMP_DATA
            audio_dump_add_channel_data(0, (short *)DecPcmBuf, MSBC_ENCODE_PCM_LEN/2);
#endif
        }
        else
        {
            TRACE(1,"PLC bad frame, plc_type: %d", plc_type);
#if defined(PLC_DEBUG_PRINT_DATA)
            DUMP8("0x%02x, ", DecMsbcBuf, 60);
#endif
#ifdef SPEECH_RX_PLC_DUMP_DATA
            for(uint32_t i=0; i< MSBC_ENCODE_PCM_LEN/2; i++)
            {
                  audio_dump_temp_buf[i] = (plc_type - 1) * 5000;
            }
            audio_dump_add_channel_data(0, audio_dump_temp_buf, MSBC_ENCODE_PCM_LEN/2);
#endif
#if defined(ENABLE_LPC_PLC)
            lpc_plc_generate(msbc_plc_state, DecPcmBuf,
#if defined(ENABLE_PLC_ENCODER)
                msbc_plc_encoder_buffer
#else
                NULL
#endif
            );

#if defined(ENABLE_PLC_ENCODER)
            pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
            pcm_data.numChannels = 1;
            pcm_data.dataLen = MSBC_ENCODE_PCM_LEN;
            pcm_data.data = (uint8_t *)(msbc_plc_encoder_buffer + MSBC_CODEC_DELAY);

            uint16_t encoded_bytes = 0, buf_len = MSBC_FRAME_SIZE - 3;
            ret = btif_sbc_encode_frames(msbc_plc_encoder, &pcm_data, &encoded_bytes, enc_tmp_buf, &buf_len, 0xFFFF);
            ASSERT(ret == BT_STS_SUCCESS, "[%s] plc encoder must success", __FUNCTION__);
            ASSERT(encoded_bytes == MSBC_ENCODE_PCM_LEN, "[%s] plc encoder encoded bytes error", __FUNCTION__);
            ASSERT(buf_len == MSBC_FRAME_SIZE - 3, "[%s] plc encoder encoded stream length error", __FUNCTION__);

            pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
            pcm_data.numChannels = 1;
            pcm_data.dataLen = 0;
            pcm_data.data = dec_tmp_buf;
            ret = btif_sbc_decode_frames(&msbc_decoder, enc_tmp_buf, MSBC_FRAME_SIZE - 3, &byte_decode, &pcm_data, MSBC_ENCODE_PCM_LEN, msbc_eq_band_gain);
            ASSERT(ret == BT_STS_SUCCESS, "[%s] plc decoder must success", __FUNCTION__);
            ASSERT(byte_decode == MSBC_FRAME_SIZE - 3, "[%s] plc decoder decoded bytes error", __FUNCTION__);
#endif

#else
            pcm_data.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
            pcm_data.numChannels = 1;
            pcm_data.dataLen = 0;
            pcm_data.data = (uint8_t *)DecPcmBuf;

            ret = btif_sbc_decode_frames(&msbc_decoder,
                    (unsigned char *)indices0,
                    MSBC_LEN_PER_FRAME, &byte_decode,
                    &pcm_data, SAMPLES_LEN_PER_FRAME*2,
                    msbc_eq_band_gain);

            PLC_bad_frame(&msbc_plc_state, DecPcmBuf, DecPcmBuf);

            ASSERT(ret == BT_STS_SUCCESS, "[%s] msbc decoder should never fail", __FUNCTION__);
#endif
        }

#ifdef SPEECH_RX_PLC_DUMP_DATA
        audio_dump_add_channel_data(1, (short *)DecPcmBuf, MSBC_ENCODE_PCM_LEN/2);
        audio_dump_run();
#endif

        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_pcm_queue, (unsigned char *)DecPcmBuf, (unsigned int)(SAMPLES_LEN_PER_FRAME*2));
        UNLOCK_APP_AUDIO_QUEUE();
        frame_counter++;
    }
#ifdef TX_RX_PCM_MASK
    if(btdrv_is_pcm_mask_enable() ==0)
#endif
    {
        for(i =0; i<MSBC_LEN_FORMBT_PER_FRAME/2; i++)
        {
            DecMsbcBufAll[i]=DecMsbcBufAll[i+(msbc_len/2)];
        }
    }
    //TRACE(1,"msbc + plc:%d", (hal_sys_timer_get()-timer_begin));

    return 0;
}

#else
static uint8_t *msbc_buf_before_decode;

short DecPcmBuf[SAMPLES_LEN_PER_FRAME];
unsigned char DecMsbcBuf[MSBC_LEN_PER_FRAME];
unsigned short DecMsbcBufAll[MSBC_LEN_FORMBT_PER_FRAME*5];
unsigned char DecVerifyMsbcBuf[MSBC_LEN_PER_FRAME*2];
unsigned int next_frame_flag = 0;
uint8_t last_msbc_sync_num = 0;
CQueue msbc_temp_queue;

__attribute__((section(".fast_text_sram")))  int decode_msbc_frame(unsigned char *msbc_btpcm_buffer, unsigned int msbc_len)
{
    btif_sbc_pcm_data_t pcm_data;
    uint16_t msbc_queue_len = 0, msbc_sync_offset = 0;
    unsigned char *e1 = NULL, *e2 = NULL;
    unsigned int len1 = 0, len2 = 0;
    int r = 0;
    uint8_t sync_num = 0, verify_result = 0;
    uint8_t frame_decode_num = 0;
    int plc_num = 0;
    bt_status_t ret;
    unsigned short byte_decode = 0;

    pcm_data.data = (unsigned char*)DecPcmBuf;
    pcm_data.dataLen = 0;

    //put msbc in queue
    for(int j = 0; j < (int)msbc_len; j += MSBC_LEN_PER_FRAME*2){
        for(int i = 0;  i < MSBC_LEN_PER_FRAME; i ++){
            DecMsbcBuf[i] = msbc_btpcm_buffer[2*i + 1 + j];
        }
        APP_AUDIO_EnCQueue(&msbc_temp_queue, DecMsbcBuf, MSBC_LEN_PER_FRAME);
    }

msbc_find_sync_again:
    msbc_queue_len = APP_AUDIO_LengthOfCQueue(&msbc_temp_queue);
    TRACE(1,"length = %d", msbc_queue_len);
    if(msbc_queue_len > MSBC_LEN_PER_FRAME*2)
        msbc_queue_len = MSBC_LEN_PER_FRAME*2;
    else if(msbc_queue_len < MSBC_LEN_PER_FRAME)
        return 0;

    r = APP_AUDIO_PeekCQueue(&msbc_temp_queue, msbc_queue_len, &e1, &len1, &e2, &len2);
    if(r == CQ_OK)
    {
        if (len1)
        {
            memcpy(DecVerifyMsbcBuf, e1, len1);
        }
        if (len2 != 0)
        {
            memcpy(DecVerifyMsbcBuf + len1, e2, len2);
        }
    }else{
        //can't happen

    }

    //DUMP8("%02x", DecVerifyMsbcBuf, msbc_queue_len);
    //find sync head
    for(int i = 0; i < (msbc_queue_len - 3); i ++){
        //verify sync success
        if(sco_parse_synchronization_header(DecVerifyMsbcBuf + i, &sync_num) >= 0){
            verify_result = 1;
            msbc_sync_offset = i;
            break;
        }
    }
    TRACE(3,"%02x %02x %02x", (DecVerifyMsbcBuf +msbc_sync_offset)[0],
                                                (DecVerifyMsbcBuf +msbc_sync_offset)[1],
                                                (DecVerifyMsbcBuf +msbc_sync_offset)[2]);
    TRACE(5,"msbc:%d %d %d %d %d\n",
                            verify_result,
                            msbc_queue_len,
                            msbc_sync_offset,
                            last_msbc_sync_num,
                            sync_num);

    if(!verify_result){
        //remove msbc frame remain 3 byted as it may include syn head
        APP_AUDIO_DeCQueue(&msbc_temp_queue, 0, msbc_queue_len - 3);
        goto msbc_find_sync_again;
    }else{
        //remove invalid msbc frame space
        APP_AUDIO_DeCQueue(&msbc_temp_queue, 0, msbc_sync_offset);
        if(msbc_sync_offset >= MSBC_LEN_PER_FRAME)
            goto msbc_find_sync_again;
    }

    //cal plc number
    if((last_msbc_sync_num + 1)%4 == sync_num){
        plc_num = 0;
    }else{
        for(int i = 0; i < 4;  i ++){
            if((last_msbc_sync_num + i)%4 == sync_num){
                break;
            }
            plc_num ++;
        }
    }
    last_msbc_sync_num = sync_num;

    //failed check sync should do plc
    if(plc_num > 0){
        for(int i = 0; i < plc_num; i ++){
            ret = btif_sbc_decode_frames(&msbc_decoder,
                (unsigned char *)indices0,
                MSBC_LEN_PER_FRAME, &byte_decode,
                &pcm_data, SAMPLES_LEN_PER_FRAME*2,
                msbc_eq_band_gain);

            if (ret == BT_STS_SUCCESS)
            {
                TRACE(1,"PLC bad frame:%d\n", ret);
                //timer_begin=hal_sys_timer_get();
                PLC_bad_frame(&msbc_plc_state, DecPcmBuf, DecPcmBuf);
                //TRACE(1,"msbc + plc:%d", (hal_sys_timer_get()-timer_begin));
                pcm_data.data = (unsigned char*)DecPcmBuf;
                pcm_data.dataLen = 0;
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_EnCQueue(&voicebtpcm_m2p_pcm_queue,
                                                     (unsigned char *)DecPcmBuf,
                                                     (unsigned int)(SAMPLES_LEN_PER_FRAME*2));
                UNLOCK_APP_AUDIO_QUEUE();
            }
            else
            {
                ASSERT(0, "ERROR inices0", __func__);
            }
        }
    }

    {
        ret = btif_sbc_decode_frames(&msbc_decoder,
                (unsigned char *)DecVerifyMsbcBuf + msbc_sync_offset + 2,
                MSBC_LEN_PER_FRAME, &byte_decode,
                &pcm_data, SAMPLES_LEN_PER_FRAME*2,
                msbc_eq_band_gain);

        if (ret == BT_STS_SUCCESS)
        {
            PLC_good_frame(&msbc_plc_state, DecPcmBuf, DecPcmBuf);
            pcm_data.data = (unsigned char*)DecPcmBuf;
            pcm_data.dataLen = 0;
        }
        else
        {
            TRACE(1,"PLC bad frame:%d\n");
            PLC_bad_frame(&msbc_plc_state, DecPcmBuf, DecPcmBuf);
            pcm_data.data = (unsigned char*)DecPcmBuf;
            pcm_data.dataLen = 0;
            TRACE(1,"ERROR msbc frame!ret:%d\n",ret);
        }
        //remove msbc frame
        APP_AUDIO_DeCQueue(&msbc_temp_queue, 0, MSBC_LEN_PER_FRAME);
        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_EnCQueue(&voicebtpcm_m2p_pcm_queue,
                                                (unsigned char *)DecPcmBuf,
                                                (unsigned int)(SAMPLES_LEN_PER_FRAME*2));
        UNLOCK_APP_AUDIO_QUEUE();
        frame_decode_num ++;
        if(frame_decode_num <= (msbc_len/(MSBC_LEN_PER_FRAME*2) - 1))
            goto msbc_find_sync_again;
    }
    return 0;
}
#endif  // #if !defined(PENDING_MSBC_DECODER_ALG)

#else
int decode_msbc_frame(unsigned char *pcm_buffer, unsigned int pcm_len)
{
    int ttt = 0;
    //int t = 0;
    uint8_t underflow = 0;
#if defined(MSBC_PLC_ENABLE)
    uint8_t plc_type = 0;
    uint8_t need_check_pkt = 1;
    uint8_t msbc_raw_sn = 0xff;
    static uint8_t msbc_raw_sn_pre;
    static uint8_t msbc_raw_sn_pre2;
    static bool msbc_find_first_sync = 0;
#endif
    int r = 0;
    unsigned char *e1 = NULL, *e2 = NULL, *msbc_buff = NULL;
    unsigned int len1 = 0, len2 = 0;
    static btif_sbc_pcm_data_t pcm_data;
    static unsigned int msbc_next_frame_size;
    bt_status_t ret = BT_STS_SUCCESS;
    unsigned short byte_decode = 0;

    unsigned int pcm_offset = 0;
    unsigned int pcm_processed = 0;

#if defined(MSBC_PLC_ENABLE)
    pcm_data.data = (unsigned char*)msbc_buf_before_plc;
#else
    pcm_data.data = (unsigned char*)pcm_buffer;
#endif
    if (!msbc_next_frame_size)
    {
        msbc_next_frame_size = MSBC_FRAME_SIZE;
    }

//reinit:
    if(need_init_decoder)
    {
        TRACE(0,"init msbc decoder\n");
        pcm_data.data = (unsigned char*)(pcm_buffer + pcm_offset);
        pcm_data.dataLen = 0;

        btif_sbc_init_decoder(&msbc_decoder);

        msbc_decoder.streamInfo.mSbcFlag = 1;
        msbc_decoder.streamInfo.bitPool = 26;
        msbc_decoder.streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
        msbc_decoder.streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;
        msbc_decoder.streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
        /* Number of blocks used to encode the stream (4, 8, 12, or 16) */
        msbc_decoder.streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
        /* The number of subbands in the stream (4 or 8) */
        msbc_decoder.streamInfo.numSubBands = 8;
        msbc_decoder.streamInfo.numChannels = 1;
#if defined(MSBC_PLC_ENCODER)
       btif_sbc_init_encoder(&sbc_Encoder1);
       sbc_Encoder1.streamInfo.mSbcFlag = 1;
       sbc_Encoder1.streamInfo.numChannels = 1;
       sbc_Encoder1.streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;

       sbc_Encoder1.streamInfo.bitPool   = 26;
       sbc_Encoder1.streamInfo.sampleFreq  = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
       sbc_Encoder1.streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
       sbc_Encoder1.streamInfo.numBlocks     = BTIF_MSBC_BLOCKS;
       sbc_Encoder1.streamInfo.numSubBands = 8;

#endif
#if defined(MSBC_PLC_ENABLE)
        InitPLC(&msbc_plc_state);
        msbc_need_check_sync_header = 0;
        msbc_raw_sn_pre = 0xff;
        msbc_raw_sn_pre2 = 0xff;
        msbc_find_first_sync = true;
#endif
    }

#if defined(MSBC_PLC_ENABLE)
    need_check_pkt = 1;
#endif
    msbc_buff = msbc_buf_before_decode;

get_again:
    LOCK_APP_AUDIO_QUEUE();
    len1 = len2 = 0;
    e1 = e2 = 0;
    r = APP_AUDIO_PeekCQueue(&voicebtpcm_m2p_pcm_queue, msbc_next_frame_size, &e1, &len1, &e2, &len2);
    UNLOCK_APP_AUDIO_QUEUE();

    if (r == CQ_ERR)
    {
        pcm_processed = pcm_len;
        memset(pcm_buffer, 0, pcm_len);
        TRACE(0,"msbc spk buff underflow");
        goto exit;
    }

    if (!len1)
    {
        TRACE(2,"len1 underflow %d/%d\n", len1, len2);
        goto get_again;
    }

    if (len1 > 0 && e1)
    {
        memcpy(msbc_buff, e1, len1);
    }
    if (len2 > 0 && e2)
    {
        memcpy(msbc_buff + len1, e2, len2);
    }

    if (msbc_find_first_sync){

        for(uint8_t i=0;i<MSBC_FRAME_SIZE-3;i++){
            if(!sco_parse_synchronization_header(&msbc_buff[i], &msbc_raw_sn)){
                TRACE(2,"1 msbc find sync sn:%d offset:%d", msbc_raw_sn, i);
                msbc_find_first_sync = false;
                goto start_decoder;
            }
        }
        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, MSBC_FRAME_SIZE-3);
        UNLOCK_APP_AUDIO_QUEUE();
        memset(pcm_buffer,0,pcm_len);
        pcm_processed = pcm_len;
        goto exit;
    }
    start_decoder:

    //DUMP8("%02x ", msbc_buf_before_decode, 8);
#if defined(MSBC_PLC_ENABLE)
    // [0]-1,msbc sync byte
    // [1]-seqno
    // code_frame(57 bytes)
    // padding(1 byte)
    if (need_check_pkt)
    {

#if 1
        if(!sco_parse_synchronization_header(msbc_buf_before_decode, &msbc_raw_sn))
        {
            if (msbc_raw_sn_pre2 == 0xff){
                // do nothing
                msbc_raw_sn_pre2 = msbc_raw_sn;
            }else{
                if (((msbc_raw_sn_pre2+1)%4) == msbc_raw_sn){
                    // do nothing
                    msbc_raw_sn_pre2 = msbc_raw_sn;
                }else if (msbc_raw_sn == 0xff){
                    TRACE(1,"xxxxxx: sbchd err:%d", MSBC_FRAME_SIZE-3);
                }else{
                    TRACE(2,"xxxxxx: seq err:%d/%d", msbc_raw_sn, msbc_raw_sn_pre2);
                    msbc_raw_sn_pre2 = (msbc_raw_sn_pre2+1)%4;
#if 1
                    memset(msbc_buf_before_decode, 0, MSBC_FRAME_SIZE);
#else
                    if (msbc_raw_sn_pre2 == 0)
                    {
                        msbc_buf_before_decode[1] = 0x08;
                    }
                    else if (msbc_raw_sn_pre2 == 1)
                    {
                        msbc_buf_before_decode[1] = 0x38;
                    }
                    else if (msbc_raw_sn_pre2 == 2)
                    {
                        msbc_buf_before_decode[1] = 0xc8;
                    }
                    else if (msbc_raw_sn_pre2 == 3)
                    {
                        msbc_buf_before_decode[1] = 0xf8;
                    }
                    else
                    {
                        ASSERT(0, "msbc_raw_sn_pre2(%d) error", __func__);
                    }
#endif
                }
            }
        }
#endif
        unsigned int sync_offset = 0;
        if(msbc_need_check_sync_header)
        {
            do
            {
                if(!sco_parse_synchronization_header(&msbc_buff[sync_offset], &msbc_raw_sn)){
                    break;
                }
                sync_offset ++;
            }
            while(sync_offset < (MSBC_FRAME_SIZE - 3));

            msbc_need_check_sync_header = 0;
            if(sync_offset > 0 && sync_offset != (MSBC_FRAME_SIZE - 3))
            {
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, sync_offset);
                UNLOCK_APP_AUDIO_QUEUE();
                TRACE(1,"fix sync:%d", sync_offset);
                msbc_raw_sn_pre = (msbc_raw_sn+3)%4;    // Find pre sn
                msbc_raw_sn_pre2 = msbc_raw_sn_pre;
                goto get_again;
                //TRACE(4,"***msbc_need_check_sync %d :%x %x %x\n", sync_offset, msbc_buf_before_decode[0],
                //        msbc_buf_before_decode[1],
                //        msbc_buf_before_decode[2]);
            }
            else if(sync_offset == (MSBC_FRAME_SIZE - 3))
            {
                //APP_AUDIO_DeCQueue(&voicebtpcm_m2p_queue, 0, sync_offset);
                //just jump sync length + padding length(1byte)
                //will in plc again, so can check sync again
                //TRACE(4,"***msbc_need_check_sync %d :%x %x %x\n", sync_offset, msbc_buf_before_decode[0],
                //         msbc_buf_before_decode[1],
                //         msbc_buf_before_decode[2]);
                msbc_next_frame_size = (MSBC_FRAME_SIZE - 3) - 1;
            }else if (!sync_offset){

            }else{
                TRACE(1,"3 msbc_need_check_sync %d\n", sync_offset);
            }
        }
        // 0 - normal decode without plc proc, 1 - normal decode with plc proc, 2 - special decode with plc proc
        plc_type = 0;

        if (msbc_can_plc)
        {
            if (sco_parse_synchronization_header(msbc_buf_before_decode, &msbc_raw_sn)){
                plc_type = 2;
                msbc_need_check_sync_header = 1;
            }else{
                plc_type = 1;
            }
        }
        else
        {
            plc_type = 0;
        }

        if (msbc_raw_sn_pre == 0xff){
            // do nothing
            msbc_raw_sn_pre = msbc_raw_sn;
        }else{
            if (((msbc_raw_sn_pre+1)%4) == msbc_raw_sn){
                // do nothing
                msbc_raw_sn_pre = msbc_raw_sn;
            }else if (msbc_raw_sn == 0xff){
                TRACE(1,"sbchd err:%d", MSBC_FRAME_SIZE-3);
                msbc_need_check_sync_header = 1;
                LOCK_APP_AUDIO_QUEUE();
                APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, MSBC_FRAME_SIZE-3);
                UNLOCK_APP_AUDIO_QUEUE();
                msbc_raw_sn_pre = (msbc_raw_sn_pre+1)%4;
                plc_type = 2;
            }else{
                TRACE(2,"seq err:%d/%d", msbc_raw_sn, msbc_raw_sn_pre);
                msbc_need_check_sync_header = 1;
                plc_type = 2;
                msbc_raw_sn_pre = (msbc_raw_sn_pre+1)%4;
            }
        }

        need_check_pkt = 0;
    }
    //TRACE(3,"type %d, seqno 0x%x, q_space %d\n", plc_type, cur_pkt_seqno, APP_AUDIO_AvailableOfCQueue(&voicebtpcm_m2p_queue));
#endif

    //DUMP8("%02x ", msbc_buf_before_decode, msbc_next_frame_size);
    //TRACE(0,"\n");

#if defined(MSBC_PLC_ENABLE)
    if (plc_type == 1)
    {
        ret = btif_sbc_decode_frames(&msbc_decoder, (unsigned char *)msbc_buf_before_decode,
                                    msbc_next_frame_size, &byte_decode,
                                    &pcm_data, pcm_len - pcm_offset,
                                    msbc_eq_band_gain);

        ttt = hal_sys_timer_get();
#if defined(MSBC_PCM_PLC_ENABLE)
        speech_plc_16000_AddToHistory((PlcSt_16000 *)speech_plc, (short *)pcm_data.data, pcm_len/2);
        memcpy(pcm_buffer, pcm_data.data, pcm_len);
#else
        PLC_good_frame(&msbc_plc_state, (short *)pcm_data.data, (short *)pcm_buffer);
#endif
#ifdef SPEECH_RX_PLC_DUMP_DATA
        audio_dump_add_channel_data(0, (short *)pcm_buffer, pcm_len/2);
#endif
    }
    else if (plc_type == 2)
    {
#if defined(MSBC_PCM_PLC_ENABLE)
        ret = BT_STS_SUCCESS;
        ttt = hal_sys_timer_get();
#if defined(MSBC_PLC_ENCODER)
        speech_plc_16000_Dofe((PlcSt_16000 *)speech_plc, (short *)pcm_buffer, EncInBuf, pcm_len/2);
#else
        speech_plc_16000_Dofe((PlcSt_16000 *)speech_plc, (short *)pcm_buffer, NULL, pcm_len/2);
#endif
#else
        PLC_bad_frame(&msbc_plc_state, (short *)pcm_data.data, (short *)pcm_buffer);
#endif
#if defined(MSBC_PLC_ENCODER)
        {
         uint16_t bytes_encoded = 0, buf_len = MSBC_ENCODE_PCM_LEN;
         PcmEncData1.data = (uint8_t *)(EncInBuf+MSBC_ENC_BUFFER_OFFSET);
         PcmEncData1.dataLen = pcm_len;
         PcmEncData1.numChannels = 1;
         PcmEncData1.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
         btif_sbc_encode_frames(&sbc_Encoder1, &PcmEncData1, &bytes_encoded, Encodedbuf1, &buf_len, 0xFFFF);
         #ifdef __SBC_FUNC_IN_ROM__
             ret = SBC_ROM_FUNC.sbc_frames_decode(&msbc_decoder, Encodedbuf1, MSBC_FRAME_SIZE-3, &byte_decode,
                                                         &pcm_data, pcm_len - pcm_offset, msbc_eq_band_gain);
         #else
             ret = btif_sbc_decode_frames(&msbc_decoder, Encodedbuf1, MSBC_FRAME_SIZE-3, &byte_decode,
                                            &pcm_data, pcm_len - pcm_offset, msbc_eq_band_gain);
         #endif

        }
#endif
        TRACE(2,"b t:%d ret:%d\n", (hal_sys_timer_get()-ttt), ret);
#ifdef SPEECH_RX_PLC_DUMP_DATA
        for(uint32_t i=0; i< pcm_len/2; i++)
        {
            audio_dump_temp_buf[i] = 32767;
        }
        audio_dump_add_channel_data(0, audio_dump_temp_buf, pcm_len/2);
#endif
    }
    else
#endif
    {
        ret = btif_sbc_decode_frames(&msbc_decoder, (unsigned char *)msbc_buf_before_decode,
                                    msbc_next_frame_size, &byte_decode,
                                    &pcm_data, pcm_len - pcm_offset,
                                    msbc_eq_band_gain);
#if defined(MSBC_PCM_PLC_ENABLE)
        speech_plc_16000_AddToHistory((PlcSt_16000 *)speech_plc, (short *)pcm_data.data, pcm_len/2);
        memcpy(pcm_buffer, pcm_data.data, pcm_len);
#endif
#ifdef SPEECH_RX_PLC_DUMP_DATA
        for(uint32_t i=0; i< pcm_len/2; i++)
        {
            audio_dump_temp_buf[i] = -32767;
        }
        audio_dump_add_channel_data(0, audio_dump_temp_buf, pcm_len/2);
#endif
    }

#ifdef SPEECH_RX_PLC_DUMP_DATA
    audio_dump_add_channel_data(1, (short *)pcm_buffer, pcm_len/2);
    audio_dump_run();
#endif

#if 0
    TRACE(1,"[0] %x", msbc_buf_before_decode[0]);
    TRACE(1,"[1] %x", msbc_buf_before_decode[1]);
    TRACE(1,"[2] %x", msbc_buf_before_decode[2]);
    TRACE(1,"[3] %x", msbc_buf_before_decode[3]);
    TRACE(1,"[4] %x", msbc_buf_before_decode[4]);
#endif

    //TRACE(2,"sbcd ret %d %d\n", ret, byte_decode);

    if(ret == BT_STS_CONTINUE)
    {
        need_init_decoder = false;
        LOCK_APP_AUDIO_QUEUE();
        VOICEBTPCM_TRACE(2,"000000 byte_decode =%d, current_len1 =%d",byte_decode, msbc_next_frame_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        msbc_next_frame_size = (MSBC_FRAME_SIZE -byte_decode)>0?(MSBC_FRAME_SIZE -byte_decode):MSBC_FRAME_SIZE;
        goto get_again;
    }

    else if(ret == BT_STS_SUCCESS)
    {
        need_init_decoder = false;
        pcm_processed = pcm_data.dataLen;
        pcm_data.dataLen = 0;

        LOCK_APP_AUDIO_QUEUE();
#if defined(MSBC_PLC_ENABLE)
        if (plc_type == 0)
        {
            byte_decode += 1;//padding
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, byte_decode);
        }
        else
        {
#if defined(MSBC_PCM_PLC_ENABLE)
        if (plc_type == 2){
            pcm_processed = pcm_len;
        }else{
            if(msbc_next_frame_size < MSBC_FRAME_SIZE)
                msbc_next_frame_size += 1; //padding
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, msbc_next_frame_size);
        }
#else
            if(msbc_next_frame_size < MSBC_FRAME_SIZE)
                msbc_next_frame_size += 1; //padding
            APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, msbc_next_frame_size);
#endif
        }
#else
        if(msbc_next_frame_size < MSBC_FRAME_SIZE)
            msbc_next_frame_size += 1; //padding
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, msbc_next_frame_size);
#endif
        UNLOCK_APP_AUDIO_QUEUE();

        msbc_next_frame_size = MSBC_FRAME_SIZE;

#if defined(MSBC_PLC_ENABLE)
        // plc after a good frame
        if (!msbc_can_plc)
        {
            msbc_can_plc = true;
        }
#endif
    }
    else if(ret == BT_STS_FAILED)
    {
        need_init_decoder = true;
        pcm_processed = pcm_len;
        pcm_data.dataLen = 0;

        memset(pcm_buffer, 0, pcm_len);
        TRACE(1,"err mutelen:%d\n",pcm_processed);

        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        msbc_next_frame_size = MSBC_FRAME_SIZE;

        /* leave */
    }
    else if(ret == BT_STS_NO_RESOURCES)
    {
        need_init_decoder = true;
        pcm_processed = pcm_len;
        pcm_data.dataLen = 0;

        memset(pcm_buffer, 0, pcm_len);
        TRACE(1,"no_res mutelen:%d\n",pcm_processed);

        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, byte_decode);
        UNLOCK_APP_AUDIO_QUEUE();

        msbc_next_frame_size = MSBC_FRAME_SIZE;
    }

exit:
    if (underflow||need_init_decoder)
    {
        TRACE(2,"media_msbc_decoder underflow len:%d,need_init_decoder=%d\n ", pcm_len,need_init_decoder);
    }

//    TRACE(1,"pcm_processed %d", pcm_processed);

    return pcm_processed;
}
#endif

#endif

//capture flow
//mic-->audioflinger capture-->store_voicebtpcm_p2m_buffer-->get_voicebtpcm_p2m_frame-->bt
//used by capture, store data from mic to memory
int store_voicebtpcm_p2m_buffer(unsigned char *buf, unsigned int len)
{
    int POSSIBLY_UNUSED size;
    unsigned int avail_size = 0;
    LOCK_APP_AUDIO_QUEUE();
//    merge_two_trace_to_one_track_16bits(0, (uint16_t *)buf, (uint16_t *)buf, len>>1);
//    r = APP_AUDIO_EnCQueue(&voicebtpcm_p2m_queue, buf, len>>1);
    avail_size = APP_AUDIO_AvailableOfCQueue(&voicebtpcm_p2m_pcm_queue);
    if (len <= avail_size)
    {
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_pcm_queue, buf, len);
    }
    else
    {
        VOICEBTPCM_TRACE(2,"mic buff overflow %d/%d", len, avail_size);
        APP_AUDIO_DeCQueue(&voicebtpcm_p2m_pcm_queue, 0, len - avail_size);
        APP_AUDIO_EnCQueue(&voicebtpcm_p2m_pcm_queue, buf, len);
    }
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_pcm_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    VOICEBTPCM_TRACE(2,"p2m :%d/%d", len, size);

    return 0;
}

#if defined(HFP_1_6_ENABLE)
unsigned char get_msbc_counter(void)
{
    if (msbc_counter == 0x08)
    {
        msbc_counter = 0x38;
    }
    else if (msbc_counter == 0x38)
    {
        msbc_counter = 0xC8;
    }
    else if (msbc_counter == 0xC8)
    {
        msbc_counter = 0xF8;
    }
    else if (msbc_counter == 0xF8)
    {
        msbc_counter = 0x08;
    }

    return msbc_counter;
}
#endif

#if defined(TX_RX_PCM_MASK)
CQueue Tx_esco_queue;
CQueue Rx_esco_queue;
CQueue* get_tx_esco_queue_ptr()
{
    return &Tx_esco_queue;
}
CQueue* get_rx_esco_queue_ptr()
{
    return &Rx_esco_queue;
}

#endif

//used by capture, get the memory data which has be stored by store_voicebtpcm_p2m_buffer()
int get_voicebtpcm_p2m_frame(unsigned char *buf, unsigned int len)
{
    int  got_len = 0;

    // TRACE(2,"[%s] pcm_len = %d", __func__, len / 2);
    if (voicebtpcm_cache_p2m_status == APP_AUDIO_CACHE_CACHEING)
    {
        app_audio_memset_16bit((short *)buf, 0, len/2);
        TRACE(1,"[%s] APP_AUDIO_CACHE_CACHEING", __func__);
        return len;
    }

    int msbc_encode_temp_len = MSBC_FRAME_SIZE * 2;

    ASSERT(len % msbc_encode_temp_len == 0 , "[%s] len(%d) is invalid", __func__, len);

    int loop_cnt = len / msbc_encode_temp_len;
    len = msbc_encode_temp_len;

    for (int cnt=0; cnt<loop_cnt; cnt++)
    {
        if (bt_sco_codec_is_msbc())
        {
#ifdef HFP_1_6_ENABLE
            uint16_t bytes_encoded = 0, buf_len = len;
            uint16_t *dest_buf = 0, offset = 0;
            int r = 0;
            unsigned char *e1 = NULL, *e2 = NULL;
            unsigned int len1 = 0, len2 = 0;

         got_len = 0;

            dest_buf = (uint16_t *)buf;

            LOCK_APP_AUDIO_QUEUE();
            r = APP_AUDIO_PeekCQueue(&voicebtpcm_p2m_pcm_queue, MSBC_ENCODE_PCM_LEN, &e1, &len1, &e2, &len2);
            UNLOCK_APP_AUDIO_QUEUE();

            if(r == CQ_OK)
            {
                if (len1)
                {
                    memcpy(temp_msbc_buf, e1, len1);
                    LOCK_APP_AUDIO_QUEUE();
                    APP_AUDIO_DeCQueue(&voicebtpcm_p2m_pcm_queue, 0, len1);
                    UNLOCK_APP_AUDIO_QUEUE();
                    got_len += len1;
                }
                if (len2 != 0)
                {
                    memcpy(temp_msbc_buf+got_len, e2, len2);
                    got_len += len2;
                    LOCK_APP_AUDIO_QUEUE();
                    APP_AUDIO_DeCQueue(&voicebtpcm_p2m_pcm_queue, 0, len2);
                    UNLOCK_APP_AUDIO_QUEUE();
                }

                //int t = 0;
                //t = hal_sys_timer_get();
                msbc_encoder_pcmdata.data = temp_msbc_buf;
                msbc_encoder_pcmdata.dataLen = MSBC_ENCODE_PCM_LEN;
                memset(temp_msbc_buf1, 0, MSBC_FRAME_SIZE);
                btif_sbc_encode_frames(msbc_encoder, &msbc_encoder_pcmdata, &bytes_encoded, temp_msbc_buf1, (uint16_t *)&buf_len, 0xFFFF);
                //TRACE(1,"enc msbc %d t\n", hal_sys_timer_get()-t);
                //TRACE(2,"encode len %d, out len %d\n", bytes_encoded, buf_len);

                dest_buf[offset++] = 1<<8;
                dest_buf[offset++] = get_msbc_counter()<<8;

                for (int i = 0; i < buf_len; ++i)
                {
                    dest_buf[offset++] = temp_msbc_buf1[i]<<8;
                }

                dest_buf[offset++] = 0; //padding

                got_len = len;
            }

#if defined(TX_RX_PCM_MASK)
            if(btdrv_is_pcm_mask_enable()==1)
            {
                int status;
                uint8_t len = 120;
                uint8_t temp_mic_buff[len];
                for(uint8_t j = 0; j < len; j++)
                {
                    temp_mic_buff[j]=buf[2*j+1];
                }
                status = APP_AUDIO_EnCQueue(&Tx_esco_queue,temp_mic_buff,len);
                if(status)
                {
                    //TRACE(0,"Tx EnC Fail");
                }
            }
#endif
#endif
        }
        else
        {
#if defined(CVSD_BYPASS)
            got_len = encode_cvsd_frame(buf, len);
#else
            int r = 0;
            unsigned char *e1 = NULL, *e2 = NULL;
            unsigned int len1 = 0, len2 = 0;
         got_len = 0;
            LOCK_APP_AUDIO_QUEUE();
            //        size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
            r = APP_AUDIO_PeekCQueue(&voicebtpcm_p2m_pcm_queue, len - got_len, &e1, &len1, &e2, &len2);
            UNLOCK_APP_AUDIO_QUEUE();

            //        VOICEBTPCM_TRACE(2,"p2m :%d/%d", len, APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue));

            if(r == CQ_OK)
            {
                if (len1)
                {
                    app_audio_memcpy_16bit((short *)buf, (short *)e1, len1/2);
                    LOCK_APP_AUDIO_QUEUE();
                    APP_AUDIO_DeCQueue(&voicebtpcm_p2m_pcm_queue, 0, len1);
                    UNLOCK_APP_AUDIO_QUEUE();
                    got_len += len1;
                }
                if (len2 != 0)
                {
                    app_audio_memcpy_16bit((short *)(buf+got_len), (short *)e2, len2/2);
                    got_len += len2;
                    LOCK_APP_AUDIO_QUEUE();
                    APP_AUDIO_DeCQueue(&voicebtpcm_p2m_pcm_queue, 0, len2);
                    UNLOCK_APP_AUDIO_QUEUE();
                }
            }
            else
            {
                VOICEBTPCM_TRACE(0,"mic buff underflow");
                app_audio_memset_16bit((short *)buf, 0, len/2);
                got_len = len;
                voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;
            }
#endif
        }
        buf += msbc_encode_temp_len;
    }


    return got_len;
}

#if 0
void get_mic_data_max(short *buf, uint32_t len)
{
    int max0 = -32768, min0 = 32767, diff0 = 0;
    int max1 = -32768, min1 = 32767, diff1 = 0;

    for(uint32_t i=0; i<len/2; i+=2)
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
    TRACE(6,"min0 = %d, max0 = %d, diff0 = %d, min1 = %d, max1 = %d, diff1 = %d", min0, max0, max0 - min0, min1, max1, max1 - min1);
}
#endif

#ifdef __BT_ANC__
extern uint8_t bt_sco_samplerate_ratio;
#define US_COEF_NUM (24)

/*
8K -->48K
single phase coef Number:=24,    upsample factor:=6,
      64, -311,  210, -320,  363, -407,  410, -356,  196,  191,-1369,30721, 3861,-2265, 1680,-1321, 1049, -823,  625, -471,  297, -327,   34,   19,
   72, -254,   75, -117,   46,   58, -256,  583,-1141, 2197,-4866,28131,10285,-4611, 2935,-2061, 1489,-1075,  755, -523,  310, -291,  -19,   31,
   65, -175,  -73,   84, -245,  457, -786, 1276,-2040, 3377,-6428,23348,17094,-6200, 3592,-2347, 1588,-1073,  703, -447,  236, -203,  -92,   49,
   49,  -92, -203,  236, -447,  703,-1073, 1588,-2347, 3592,-6200,17094,23348,-6428, 3377,-2040, 1276, -786,  457, -245,   84,  -73, -175,   65,
   31,  -19, -291,  310, -523,  755,-1075, 1489,-2061, 2935,-4611,10285,28131,-4866, 2197,-1141,  583, -256,   58,   46, -117,   75, -254,   72,
   19,   34, -327,  297, -471,  625, -823, 1049,-1321, 1680,-2265, 3861,30721,-1369,  191,  196, -356,  410, -407,  363, -320,  210, -311,   64,
*/

const static short coef_8k_upto_48k[6][US_COEF_NUM]__attribute__((section(".sram_data")))  =
{
    {64, -311,  210, -320,  363, -407,  410, -356,  196,    191,-1369,30721, 3861,-2265, 1680,-1321, 1049, -823,  625, -471,  297, -327,   34,   19},
    {72, -254,  75, -117,   46,   58, -256,  583,-1141, 2197,-4866,28131,10285,-4611, 2935,-2061, 1489,-1075,  755, -523,  310, -291,  -19,   31   },
    {65, -175,  -73,   84, -245,  457, -786, 1276,-2040, 3377,-6428,23348,17094,-6200, 3592,-2347, 1588,-1073,  703, -447,  236, -203,  -92,   49  },
    {49,  -92, -203,  236, -447,  703,-1073, 1588,-2347, 3592,-6200,17094,23348,-6428, 3377,-2040, 1276, -786,  457, -245,  84,  -73, -175,   65  },
    {31,  -19, -291,  310, -523,  755,-1075, 1489,-2061, 2935,-4611,10285,28131,-4866, 2197,-1141,  583, -256,  58,   46, -117,   75, -254,   72  },
    {19,   34, -327,  297, -471,  625, -823, 1049,-1321, 1680,-2265, 3861,30721,-1369,  191,  196, -356,  410, -407,  363, -320,  210, -311,   64 }
};

/*
16K -->48K

single phase coef Number:=24,    upsample factor:=3,
       1, -291,  248, -327,  383, -405,  362, -212, -129,  875,-2948,29344, 7324,-3795, 2603,-1913, 1418,-1031,  722, -478,  292, -220,  -86,   16,
   26, -212,    6,   45, -185,  414, -764, 1290,-2099, 3470,-6431,20320,20320,-6431, 3470,-2099, 1290, -764,  414, -185,   45,    6, -212,   26,
   16,  -86, -220,  292, -478,  722,-1031, 1418,-1913, 2603,-3795, 7324,29344,-2948,  875, -129, -212,  362, -405,  383, -327,  248, -291,    1,
*/

const static short coef_16k_upto_48k[3][US_COEF_NUM]  __attribute__((section(".sram_data"))) =
{
    {1, -291,  248, -327,  383, -405,  362, -212, -129, 875,-2948,29344, 7324,-3795, 2603,-1913, 1418,-1031,  722, -478,  292, -220,  -86,   16},
    {26, -212,   6,   45, -185,  414, -764, 1290,-2099, 3470,-6431,20320,20320,-6431, 3470,-2099, 1290, -764,  414, -185,   45,    6, -212,   26},
    {16,  -86, -220,  292, -478,  722,-1031, 1418,-1913, 2603,-3795, 7324,29344,-2948,  875, -129, -212,  362, -405,  383, -327,  248, -291,    1}
};

static short us_para_lst[US_COEF_NUM-1];

static inline short us_get_coef_para(U32 samp_idx,U32 coef_idx)
{
    if(bt_sco_samplerate_ratio == 6)
        return coef_8k_upto_48k[samp_idx][coef_idx];
    else
        return coef_16k_upto_48k[samp_idx][coef_idx];
}

void us_fir_init (void)
{
    app_audio_memset_16bit(us_para_lst, 0, sizeof(us_para_lst)/sizeof(short));
}


__attribute__((section(".fast_text_sram"))) U32 us_fir_run (short* src_buf, short* dst_buf, U32 in_samp_num)
{
    U32 in_idx, samp_idx, coef_idx, real_idx, out_idx;
    int para, out;

    for (in_idx = 0, out_idx = 0; in_idx < in_samp_num; in_idx++)
    {
        for (samp_idx = 0; samp_idx < bt_sco_samplerate_ratio; samp_idx++)
        {
            out = 0;
            for (coef_idx = 0; coef_idx < US_COEF_NUM; coef_idx++)
            {
                real_idx = coef_idx + in_idx;
                para = (real_idx < (US_COEF_NUM-1))?us_para_lst[real_idx]:src_buf[real_idx - (US_COEF_NUM-1)];
                out += para * us_get_coef_para(samp_idx,coef_idx);
            }

            dst_buf[out_idx] = (short)(out>>16);
            out_idx++;
        }
    }

    if (in_samp_num >= (US_COEF_NUM-1))
    {
        app_audio_memcpy_16bit(us_para_lst,
                               (src_buf+in_samp_num-US_COEF_NUM+1),
                               (US_COEF_NUM-1));
    }
    else
    {
        U32 start_idx = (US_COEF_NUM-1-in_samp_num);

        app_audio_memcpy_16bit(us_para_lst,
                               (us_para_lst+in_samp_num),
                               start_idx);

        app_audio_memcpy_16bit((us_para_lst + start_idx),
                               src_buf,
                               in_samp_num);
    }
    return out_idx;
}

uint32_t voicebtpcm_pcm_resample (short* src_samp_buf, uint32_t src_smpl_cnt, short* dst_samp_buf)
{
    return us_fir_run (src_samp_buf, dst_samp_buf, src_smpl_cnt);
}

#endif

static int speech_tx_aec_frame_len = 0;

int speech_tx_aec_get_frame_len(void)
{
    return speech_tx_aec_frame_len;
}

void speech_tx_aec_set_frame_len(int len)
{
    TRACE(2,"[%s] len = %d", __func__, len);
    speech_tx_aec_frame_len = len;
}

#if 1
//used by capture, store data from mic to memory
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len)
{
    int16_t POSSIBLY_UNUSED ret = 0;
    bool POSSIBLY_UNUSED vdt = false;
    int size = 0;

#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    uint32_t queue_len;

    uint32_t len_per_channel = len / SPEECH_CODEC_CAPTURE_CHANNEL_NUM;

    ASSERT(len_per_channel == speech_tx_aec_get_frame_len() * sizeof(short), "%s: Unmatched len: %u != %u", __func__, len_per_channel, speech_tx_aec_get_frame_len() * sizeof(short));
    ASSERT(echo_buf_q_rpos + len_per_channel <= echo_buf_q_size, "%s: rpos (%u) overflow: len=%u size=%u", __func__, echo_buf_q_rpos, len_per_channel, echo_buf_q_size);

    if (echo_buf_q_rpos == echo_buf_q_wpos) {
        queue_len = echo_buf_q_full ? echo_buf_q_size : 0;
        echo_buf_q_full = false;
    } else if (echo_buf_q_rpos < echo_buf_q_wpos) {
        queue_len = echo_buf_q_wpos - echo_buf_q_rpos;
    } else {
        queue_len = echo_buf_q_size + echo_buf_q_wpos - echo_buf_q_rpos;
    }
    ASSERT(queue_len >= len_per_channel, "%s: queue underflow: q_len=%u len=%u rpos=%u wpos=%u size=%u",
        __func__, queue_len, len_per_channel, echo_buf_q_rpos, echo_buf_q_wpos, echo_buf_q_size);

    aec_echo_buf = (int16_t *)(echo_buf_queue + echo_buf_q_rpos);
    echo_buf_q_rpos += len_per_channel;
    if (echo_buf_q_rpos >= echo_buf_q_size) {
        echo_buf_q_rpos = 0;
    }
#endif
#endif
    short   *pcm_buf = (short*)buf;
#if defined(SPEECH_TX_24BIT)
    int     pcm_len = len / sizeof(int32_t);
#else
    int     pcm_len = len / sizeof(int16_t);
#endif

    if(app_get_current_overlay() == APP_OVERLAY_HFP){
        speech_tx_process(pcm_buf, aec_echo_buf, &pcm_len);

#if defined(SPEECH_TX_24BIT)
        int32_t *buf24 = (int32_t *)pcm_buf;
        int16_t *buf16 = (int16_t *)pcm_buf;
        for (int i = 0; i < pcm_len; i++)
            buf16[i] = (buf24[i] >> 8);
#endif

        if (resample_needed_flag == true) {
            iir_resample_process(upsample_st, pcm_buf, upsample_buf_for_msbc, pcm_len);
            pcm_buf = upsample_buf_for_msbc;
            pcm_len = sco_frame_length;
        }
    }else{
        memset(buf, 0, len);
    }

    LOCK_APP_AUDIO_QUEUE();
    store_voicebtpcm_p2m_buffer((uint8_t *)pcm_buf, pcm_len * sizeof(short));
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_pcm_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    if (size >= voicebtpcm_p2m_cache_threshold)
    {
        voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_OK;
    }

    return pcm_len*2;
}
#else
//used by capture, store data from mic to memory
uint32_t voicebtpcm_pcm_audio_data_come(uint8_t *buf, uint32_t len)
{
    int16_t POSSIBLY_UNUSED ret = 0;
    bool POSSIBLY_UNUSED vdt = false;
    int size = 0;

    short *pcm_buf = (short*)buf;
    uint32_t pcm_len = len / 2;

    // TRACE(2,"[%s] pcm_len = %d", __func__, pcm_len);

    LOCK_APP_AUDIO_QUEUE();
    store_voicebtpcm_p2m_buffer((uint8_t *)pcm_buf, pcm_len*2);
    size = APP_AUDIO_LengthOfCQueue(&voicebtpcm_p2m_queue);
    UNLOCK_APP_AUDIO_QUEUE();

    if (size > VOICEBTPCM_PCM_TEMP_BUFFER_SIZE)
    {
        voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_OK;
    }

    return pcm_len*2;
}
#endif

//used by playback, play data from memory to speaker
uint32_t voicebtpcm_pcm_audio_more_data(uint8_t *buf, uint32_t len)
{
    uint32_t l = 0;
    //TRACE(3,"[%s]: pcm_len = %d, %d", __FUNCTION__, len / 2, FAST_TICKS_TO_US(hal_fast_sys_timer_get()));
    if ((voicebtpcm_cache_m2p_status == APP_AUDIO_CACHE_CACHEING)
#ifndef FPGA
        ||(app_get_current_overlay() != APP_OVERLAY_HFP)
#endif
        )
    {
        app_audio_memset_16bit((short *)buf, 0, len/2);
        l = len;
    }
    else
    {
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT) || defined(SPEECH_TX_THIRDPARTY)
#if !(defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE))
#if defined(SPEECH_TX_24BIT) && defined(SPEECH_RX_24BIT)
        memcpy(aec_echo_buf, buf, len / sizeof(int16_t) * sizeof(int32_t));
#elif defined(SPEECH_TX_24BIT) && !defined(SPEECH_RX_24BIT)
        short *buf_p=(short *)buf;
        for (uint32_t i = 0; i < len / sizeof(int16_t); i++) {
            aec_echo_buf[i] = ((int32_t)buf_p[i] << 8);
        }
#elif !defined(SPEECH_TX_24BIT) && defined(SPEECH_RX_24BIT)
        int32_t *buf32_p = (int32_t *)buf;
        for (uint32_t i = 0; i < len / sizeof(int16_t); i++) {
            aec_echo_buf[i] = (buf32_p[i] >> 8);
        }
#else
        app_audio_memcpy_16bit((int16_t *)aec_echo_buf, (int16_t *)buf, len/2);
#endif
#endif
#endif

        int decode_len = len;
        uint8_t *decode_buf = buf;

        if (resample_needed_flag == true) {
            decode_len = sco_frame_length * 2;
            decode_buf = (uint8_t *)downsample_buf_for_msbc;
        }

        unsigned int len1 = 0, len2 = 0;
        unsigned char *e1 = NULL, *e2 = NULL;
        int r = 0;

        LOCK_APP_AUDIO_QUEUE();
        len1 = len2 = 0;
        e1 = e2 = 0;
        r = APP_AUDIO_PeekCQueue(&voicebtpcm_m2p_pcm_queue, decode_len, &e1, &len1, &e2, &len2);
        UNLOCK_APP_AUDIO_QUEUE();
        if (r == CQ_ERR)
        {
            TRACE(0,"pcm buff underflow");
            memset(decode_buf, 0, decode_len);
            l = len;
            goto fail;
        }

        if (!len1)
        {
            TRACE(2,"pcm  len1 underflow %d/%d\n", len1, len2);
            memset(decode_buf, 0, decode_len);
            l = len;
            goto fail;
        }

        if (len1 > 0 && e1)
        {
            memcpy(decode_buf, e1, len1);
        }
        if (len2 > 0 && e2)
        {
            memcpy(decode_buf + len1, e2, len2);
        }
        LOCK_APP_AUDIO_QUEUE();
        APP_AUDIO_DeCQueue(&voicebtpcm_m2p_pcm_queue, 0, decode_len);
        UNLOCK_APP_AUDIO_QUEUE();
    } // if (voicebtpcm_cache_m2p_status == APP_AUDIO_CACHE_CACHEING)

    // downsample_buf_for_msbc size is len * 2
    if (resample_needed_flag == true) {
        iir_resample_process(downsample_st, downsample_buf_for_msbc, (int16_t *)buf, sco_frame_length);
    }

fail:
    short   * POSSIBLY_UNUSED pcm_buf = (short*)buf;
    int     POSSIBLY_UNUSED pcm_len = len / 2;

#if defined(SPEECH_RX_24BIT)
    int32_t *buf32 = (int32_t *)buf;
    for (int i = pcm_len - 1; i >= 0; i--) {
        buf32[i] = ((int32_t)pcm_buf[i] << 8);
    }
#endif

    speech_rx_process(pcm_buf, &pcm_len);

    buf = (uint8_t *)pcm_buf;
    len = pcm_len * sizeof(short);

#if defined(SPEECH_RX_24BIT)
    len = len / sizeof(int16_t) * sizeof(int32_t);
#endif

#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    uint32_t queue_len;

    ASSERT(len == speech_tx_aec_get_frame_len() * sizeof(short), "%s: Unmatched len: %u != %u", __func__, len, speech_tx_aec_get_frame_len() * sizeof(short));
    ASSERT(echo_buf_q_wpos + len <= echo_buf_q_size, "%s: wpos (%u) overflow: len=%u size=%u", __func__, echo_buf_q_wpos, len, echo_buf_q_size);

    if (echo_buf_q_rpos == echo_buf_q_wpos) {
        queue_len = echo_buf_q_full ? echo_buf_q_size : 0;
    } else if (echo_buf_q_rpos < echo_buf_q_wpos) {
        queue_len = echo_buf_q_wpos - echo_buf_q_rpos;
    } else {
        queue_len = echo_buf_q_size + echo_buf_q_wpos - echo_buf_q_rpos;
    }
    ASSERT(queue_len + len <= echo_buf_q_size, "%s: queue overflow: q_len=%u len=%u rpos=%u wpos=%u size=%u",
        __func__, queue_len, len, echo_buf_q_rpos, echo_buf_q_wpos, echo_buf_q_size);

    app_audio_memcpy_16bit((int16_t *)(echo_buf_queue + echo_buf_q_wpos), (int16_t *)buf, len / 2);
    echo_buf_q_wpos += len;
    if (echo_buf_q_wpos >= echo_buf_q_size) {
        echo_buf_q_wpos = 0;
    }
    if (echo_buf_q_rpos == echo_buf_q_wpos) {
        echo_buf_q_full = true;
    }
#endif
#endif

    return l;
}

void *voicebtpcm_get_ext_buff(int size)
{
    uint8_t *pBuff = NULL;
    if (size % 4)
    {
        size = size + (4 - size % 4);
    }
    app_audio_mempool_get_buff(&pBuff, size);
    VOICEBTPCM_TRACE(2,"[%s] len:%d", __func__, size);
    return (void*)pBuff;
}

int voicebtpcm_pcm_echo_buf_queue_init(uint32_t size)
{
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    echo_buf_queue = (uint8_t *)voicebtpcm_get_ext_buff(size);
    echo_buf_q_size = size;
    echo_buf_q_wpos = 0;
    echo_buf_q_rpos = 0;
    echo_buf_q_full = false;
#endif
#endif
    return 0;
}

void voicebtpcm_pcm_echo_buf_queue_reset(void)
{
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    echo_buf_q_wpos = 0;
    echo_buf_q_rpos = 0;
    echo_buf_q_full = false;
#endif
#endif
}

void voicebtpcm_pcm_echo_buf_queue_deinit(void)
{
#if defined(SPEECH_TX_AEC) || defined(SPEECH_TX_AEC2) || defined(SPEECH_TX_AEC3) || defined(SPEECH_TX_AEC2FLOAT)
#if defined(__AUDIO_RESAMPLE__) && defined(SW_SCO_RESAMPLE)
    echo_buf_queue = NULL;
    echo_buf_q_size = 0;
    echo_buf_q_wpos = 0;
    echo_buf_q_rpos = 0;
    echo_buf_q_full = false;
#endif
#endif
}

extern enum AUD_SAMPRATE_T speech_codec_get_sample_rate(void);

// sco sample rate: encoder/decoder sample rate
// codec sample rate: hardware sample rate
int voicebtpcm_pcm_audio_init(int sco_sample_rate, int codec_sample_rate)
{
    uint8_t POSSIBLY_UNUSED *speech_buf = NULL;
    int POSSIBLY_UNUSED speech_len = 0;

    sco_frame_length = SPEECH_FRAME_MS_TO_LEN(sco_sample_rate, SPEECH_SCO_FRAME_MS);
    codec_frame_length = SPEECH_FRAME_MS_TO_LEN(codec_sample_rate, SPEECH_SCO_FRAME_MS);

    TRACE(3,"[%s] TX: sample rate = %d, frame len = %d", __func__, codec_sample_rate, codec_frame_length);
    TRACE(3,"[%s] RX: sample rate = %d, frame len = %d", __func__, codec_sample_rate, codec_frame_length);

    // init cqueue
    uint8_t *p2m_pcm_buff = NULL;
    uint8_t *m2p_pcm_buff = NULL;
#if defined(TX_RX_PCM_MASK)
    uint8_t *Mic_frame_buff = NULL;
    uint8_t *Rx_esco_buff = NULL;
    uint32_t pcm_buf_size= 240*sizeof(uint8_t);
#endif
    if (bt_sco_codec_is_msbc())
    {
        voicebtpcm_p2m_pcm_cache_size=VOICEBTPCM_PCM_16K_QUEUE_SIZE;
        voicebtpcm_m2p_pcm_cache_size=VOICEBTPCM_PCM_16K_QUEUE_SIZE;
    }
    else
    {
        voicebtpcm_p2m_pcm_cache_size=VOICEBTPCM_PCM_8K_QUEUE_SIZE;
        voicebtpcm_m2p_pcm_cache_size=VOICEBTPCM_PCM_8K_QUEUE_SIZE;
    }

    voicebtpcm_p2m_cache_threshold=voicebtpcm_p2m_pcm_cache_size/2;
    voicebtpcm_m2p_cache_threshold=voicebtpcm_m2p_pcm_cache_size/2;

    app_audio_mempool_get_buff(&p2m_pcm_buff, voicebtpcm_p2m_pcm_cache_size);
    app_audio_mempool_get_buff(&m2p_pcm_buff, voicebtpcm_m2p_pcm_cache_size);
#if defined(TX_RX_PCM_MASK)
    app_audio_mempool_get_buff(&Mic_frame_buff, pcm_buf_size);
    app_audio_mempool_get_buff(&Rx_esco_buff, pcm_buf_size);
#endif

    LOCK_APP_AUDIO_QUEUE();
    APP_AUDIO_InitCQueue(&voicebtpcm_p2m_pcm_queue, voicebtpcm_p2m_pcm_cache_size, p2m_pcm_buff);
    APP_AUDIO_InitCQueue(&voicebtpcm_m2p_pcm_queue, voicebtpcm_m2p_pcm_cache_size, m2p_pcm_buff);

#if defined(TX_RX_PCM_MASK)
    APP_AUDIO_InitCQueue(&Tx_esco_queue,pcm_buf_size,Mic_frame_buff);
    APP_AUDIO_InitCQueue(&Rx_esco_queue,pcm_buf_size,Rx_esco_buff);
#endif
    UNLOCK_APP_AUDIO_QUEUE();

    voicebtpcm_cache_m2p_status = APP_AUDIO_CACHE_CACHEING;
    voicebtpcm_cache_p2m_status = APP_AUDIO_CACHE_CACHEING;

#if defined(HFP_1_6_ENABLE)

    memset(DecPcmBuf,0,SAMPLES_LEN_PER_FRAME*sizeof(short));
    memset(DecMsbcBuf,0,MSBC_LEN_PER_FRAME*sizeof(unsigned char));
    memset(DecMsbcBufAll,0, sizeof(DecMsbcBufAll));

    if (bt_sco_codec_is_msbc())
    {
        app_audio_mempool_get_buff((uint8_t **)&msbc_encoder, sizeof(btif_sbc_encoder_t));
#if !defined(PENDING_MSBC_DECODER_ALG)
        app_audio_mempool_get_buff(&temp_msbc_buf, MSBC_ENCODE_PCM_LEN);
        app_audio_mempool_get_buff(&temp_msbc_buf1, MSBC_FRAME_SIZE);
#else
        app_audio_mempool_get_buff(&msbc_buf_before_decode, MSBC_FRAME_SIZE * sizeof(short));
        app_audio_mempool_get_buff(&temp_msbc_buf, MSBC_ENCODE_PCM_LEN * sizeof(short));
        app_audio_mempool_get_buff(&temp_msbc_buf1, MSBC_ENCODE_PCM_LEN * sizeof(short));
#endif
        //init msbc encoder
        btif_sbc_init_encoder(msbc_encoder);
        msbc_encoder->streamInfo.mSbcFlag = 1;
        msbc_encoder->streamInfo.numChannels = 1;
        msbc_encoder->streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;

        msbc_encoder->streamInfo.bitPool     = 26;
        msbc_encoder->streamInfo.sampleFreq  = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
        msbc_encoder->streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
        msbc_encoder->streamInfo.numBlocks   = BTIF_MSBC_BLOCKS;
        msbc_encoder->streamInfo.numSubBands = 8;

        msbc_counter = 0x08;

        //init msbc decoder
        const float EQLevel[25] =
        {
            0.0630957,  0.0794328, 0.1,       0.1258925, 0.1584893,
            0.1995262,  0.2511886, 0.3162278, 0.398107, 0.5011872,
            0.6309573,  0.794328, 1,         1.258925, 1.584893,
            1.995262,  2.5118864, 3.1622776, 3.9810717, 5.011872,
            6.309573,  7.943282, 10, 12.589254, 15.848932
        };//-12~12
        uint8_t i;

        for (i=0; i<sizeof(msbc_eq_band_gain)/sizeof(float); i++)
        {
            msbc_eq_band_gain[i] = EQLevel[cfg_aud_eq_sbc_band_settings[i]+12];
        }

        btif_sbc_init_decoder(&msbc_decoder);

        msbc_decoder.streamInfo.mSbcFlag = 1;
        msbc_decoder.streamInfo.bitPool = 26;
        msbc_decoder.streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
        msbc_decoder.streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;
        msbc_decoder.streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
        /* Number of blocks used to encode the stream (4, 8, 12, or 16) */
        msbc_decoder.streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
        /* The number of subbands in the stream (4 or 8) */
        msbc_decoder.streamInfo.numSubBands = 8;
        msbc_decoder.streamInfo.numChannels = 1;
#if defined(PENDING_MSBC_DECODER_ALG)
        last_msbc_sync_num = 0;
        APP_AUDIO_InitCQueue(&msbc_temp_queue, sizeof(DecMsbcBufAll), (uint8_t*)DecMsbcBufAll);
#endif
        //init msbc plc

#ifndef ENABLE_LPC_PLC
        InitPLC(&msbc_plc_state);
#endif

        next_frame_flag = 0;
        msbc_find_first_sync = 0;

        packet_loss_detection_init(&pld);

#if defined(ENABLE_PLC_ENCODER)
        app_audio_mempool_get_buff((uint8_t **)&msbc_plc_encoder, sizeof(btif_sbc_encoder_t));
        btif_sbc_init_encoder(msbc_plc_encoder);
        msbc_plc_encoder->streamInfo.mSbcFlag = 1;
        msbc_plc_encoder->streamInfo.bitPool = 26;
        msbc_plc_encoder->streamInfo.sampleFreq = BTIF_SBC_CHNL_SAMPLE_FREQ_16;
        msbc_plc_encoder->streamInfo.channelMode = BTIF_SBC_CHNL_MODE_MONO;
        msbc_plc_encoder->streamInfo.allocMethod = BTIF_SBC_ALLOC_METHOD_LOUDNESS;
        /* Number of blocks used to encode the stream (4, 8, 12, or 16) */
        msbc_plc_encoder->streamInfo.numBlocks = BTIF_MSBC_BLOCKS;
        /* The number of subbands in the stream (4 or 8) */
        msbc_plc_encoder->streamInfo.numSubBands = 8;
        msbc_plc_encoder->streamInfo.numChannels = 1;
        app_audio_mempool_get_buff((uint8_t **)&msbc_plc_encoder_buffer, sizeof(int16_t) * (SAMPLES_LEN_PER_FRAME + MSBC_CODEC_DELAY));
#endif
    }
    else
#endif
    {
        speech_plc = (PlcSt_8000 *)speech_plc_8000_init(voicebtpcm_get_ext_buff);
    }

#if defined(CVSD_BYPASS)
    Pcm8k_CvsdInit();
#endif

#ifdef SPEECH_RX_PLC_DUMP_DATA
    audio_dump_temp_buf = (int16_t *)voicebtpcm_get_ext_buff(sizeof(int16_t) * 120);
    audio_dump_init(120, sizeof(short), 3);
#endif

    resample_needed_flag = (sco_sample_rate == codec_sample_rate) ? 0 : 1;

    if (resample_needed_flag == true) {
        // upsample_buf_for_msbc -> uplink resampler
        // downsample_buf_for_msbc -> downlink resampler
        // store msbc pcm buffer
        upsample_buf_for_msbc = (int16_t *)voicebtpcm_get_ext_buff(sizeof(int16_t) * sco_frame_length);
        downsample_buf_for_msbc = (int16_t *)voicebtpcm_get_ext_buff(sizeof(int16_t) * sco_frame_length);
    }

#if defined(SCO_OPTIMIZE_FOR_RAM)
    sco_overlay_ram_buf_len = hal_overlay_get_text_free_size((enum HAL_OVERLAY_ID_T)APP_OVERLAY_HFP);
    sco_overlay_ram_buf = (uint8_t *)hal_overlay_get_text_free_addr((enum HAL_OVERLAY_ID_T)APP_OVERLAY_HFP);
#endif

    speech_len = app_audio_mempool_free_buff_size() - APP_BT_STREAM_USE_BUF_SIZE;
    speech_buf = (uint8_t *)voicebtpcm_get_ext_buff(speech_len);

    int tx_frame_ms = SPEECH_PROCESS_FRAME_MS;
    int rx_frame_ms = SPEECH_PROCESS_FRAME_MS;
#if !defined(SPEECH_RX_NS2FLOAT)
    rx_frame_ms = SPEECH_SCO_FRAME_MS;
#endif
    speech_init(codec_sample_rate, codec_sample_rate, tx_frame_ms, rx_frame_ms, SPEECH_SCO_FRAME_MS, speech_buf, speech_len);

    if (resample_needed_flag == true) {
        // Resample state must be created after speech init, as it uses speech heap
        upsample_st = iir_resample_init(codec_frame_length, iir_resample_choose_mode(codec_sample_rate, sco_sample_rate));
        downsample_st = iir_resample_init(sco_frame_length, iir_resample_choose_mode(sco_sample_rate, codec_sample_rate));
    }

#if defined(HFP_1_6_ENABLE) && defined(ENABLE_LPC_PLC)
    msbc_plc_state = lpc_plc_create(sco_sample_rate);
#endif

    return 0;
}

int voicebtpcm_pcm_audio_deinit(void)
{
    TRACE(1,"[%s] Close...", __func__);
    // TRACE(2,"[%s] app audio buffer free = %d", __func__, app_audio_mempool_free_buff_size());

#if defined(HFP_1_6_ENABLE) && defined(ENABLE_LPC_PLC)
    lpc_plc_destroy(msbc_plc_state);
#endif

    if (resample_needed_flag == true) {
        iir_resample_destroy(upsample_st);
        iir_resample_destroy(downsample_st);
    }

    speech_deinit();

#if defined(HFP_1_6_ENABLE)
#if !defined(PENDING_MSBC_DECODER_ALG)
    packet_loss_detection_report(&pld);
#endif
#endif

#if defined(SCO_OPTIMIZE_FOR_RAM)
    sco_overlay_ram_buf = NULL;
    sco_overlay_ram_buf_len = 0;
#endif

    voicebtpcm_pcm_echo_buf_queue_deinit();

    // TRACE(1,"Free buf = %d", app_audio_mempool_free_buff_size());

    return 0;
}