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
#ifdef CHIP_BEST1000

#include "cmsis.h"
#include "cmsis_os.h"
#include "hal_trace.h"
#include "fmdec.h"
#include "hal_dma.h"
#include "hal_timer.h"
#include "hal_cmu.h"
#include "hal_analogif.h"
#include "hal_chipid.h"
#include "audioflinger.h"
#include "audiobuffer.h"
#include "cqueue.h"
#include "app_audio.h"
#include "app_utils.h"
#include "app_overlay.h"
#include "string.h"
#include "pmu.h"

//#define FM_DEBUG 1

#define FM_DIGITAL_REG(a) *(volatile uint32_t *)(a)
#define fm_read_rf_reg(reg,val)  hal_analogif_reg_read(reg,val)
#define fm_write_rf_reg(reg,val) hal_analogif_reg_write(reg,val)

#define FM_FRAME_NUM 4
#define FM_SAMPLE_NUM NUMOFSAMPLE

#ifdef ATAN2_HARDWARE
#ifdef FM_NEWMODE
#define FM_SAMPLE_BUFFER_SIZE (FM_FRAME_NUM*FM_SAMPLE_NUM*4)
#else
#define FM_SAMPLE_BUFFER_SIZE (FM_FRAME_NUM*FM_SAMPLE_NUM/2*4)
#endif
#else
#define FM_SAMPLE_BUFFER_SIZE (FM_FRAME_NUM*FM_SAMPLE_NUM*4)
#endif
#define FM_AUDIO_BUFFER_SIZE (4096)

extern int app_bt_stream_local_volume_get(void);
static int32_t *fm_sample_buffer_p;

static void fm_handler(uint8_t chan, uint32_t remains, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    static int cnt = 0;
    int16_t fm_decbuf[(FM_SAMPLE_NUM/9)];
    FmDemodulate((int16_t *)(fm_sample_buffer_p +((cnt%FM_FRAME_NUM)*FM_SAMPLE_NUM)), fm_decbuf,FM_SAMPLE_NUM);
    cnt++;
    app_audio_pcmbuff_put((uint8_t *)fm_decbuf, (FM_SAMPLE_NUM/9)<<1);
    FmDemodulate((int16_t *)(fm_sample_buffer_p +((cnt%FM_FRAME_NUM)*FM_SAMPLE_NUM)), fm_decbuf,FM_SAMPLE_NUM);
    cnt++;
    app_audio_pcmbuff_put((uint8_t *)fm_decbuf, (FM_SAMPLE_NUM/9)<<1);

#ifdef FM_DEBUG
    {
        static uint32_t preTicks;
        uint32_t diff_ticks = 0;
        uint32_t cur_ticks;

        cur_ticks = hal_sys_timer_get();
        if (!preTicks){
            preTicks = cur_ticks;
        }else{
            diff_ticks = TICKS_TO_MS(cur_ticks - preTicks);
            preTicks = cur_ticks;
        }
        TRACE(3,"[fm_handler]       diff=%d add:%d remain:%d input", diff_ticks, (FM_SAMPLE_NUM/9)<<1, app_audio_pcmbuff_length());
    }
#endif
}

uint32_t fm_pcm_more_data(uint8_t *buf, uint32_t len)
{
        app_audio_pcmbuff_get(buf, len);
#ifdef FM_DEBUG
    {
        static uint32_t preTicks;
        uint32_t diff_ticks = 0;
        uint32_t cur_ticks= hal_sys_timer_get();
        if (!preTicks){
            preTicks = cur_ticks;
        }else{
            diff_ticks = TICKS_TO_MS(cur_ticks - preTicks);
            preTicks = cur_ticks;
        }

        TRACE(5,"[fm_pcm_more_data] diff=%d get:%d remain:%d output isr:0x%08x cnt:%d", diff_ticks, len/2, app_audio_pcmbuff_length(), FM_DIGITAL_REG(0x40160020), FM_DIGITAL_REG(0x40160028));
    }
#endif
    return 0;
}

uint32_t fm_capture_more_data(uint8_t *buf, uint32_t len)
{
    fm_handler(0,0,0,NULL);
    return len;
}

void fm_radio_digit_init(void)
{
    FM_DIGITAL_REG(0xd0350244) = (FM_DIGITAL_REG(0xd0350244) & ~0x01fff) | 0x20f; //-890k -> 0 if_shift, for 110.5292m adc

//    FM_DIGITAL_REG(0x40180e0c) = 0x34;
   //FM_DIGITAL_REG(0x4000a050) = (FM_DIGITAL_REG(0x4000a050) & ~0x18000) | 0x18000;




#ifdef ATAN2_HARDWARE


    FM_DIGITAL_REG(0xd0330038) |= (1 << 11);
    FM_DIGITAL_REG(0xd0330038) |= (1 << 17);
    FM_DIGITAL_REG(0xd0350248) = 0x80c00000;
//    FM_DIGITAL_REG(0x40160030) = 1;
 //   FM_DIGITAL_REG(0x40160000) = 0x21;


#else



    FM_DIGITAL_REG(0xd0330038) |= (1 << 11);
    FM_DIGITAL_REG(0xd0350248) = 0x80c00000;
//    FM_DIGITAL_REG(0x40160030) = 1;
//    FM_DIGITAL_REG(0x40160000) = 1;

#endif



#ifdef SINGLECHANLE
   //0x4000a010  bit2 写0   单channel dac
    FM_DIGITAL_REG(0x4000a010) = (1 << 5) |(1<<4);
#else

    FM_DIGITAL_REG(0x4000a010) = (1 << 5) | (1 << 2)|(1<<4);
#endif



    FM_DIGITAL_REG(0x4000a020) = ~0UL;
    FM_DIGITAL_REG(0x4000a02c) = 4;
    FM_DIGITAL_REG(0x4000a030) = 4;

    FM_DIGITAL_REG(0x4000a034) = (1 << 2) | (1 << 1) | (1 << 0);

    // Start DAC
   // FM_DIGITAL_REG(0x4000a010) |= (1 << 1);

#if 0
//52M
    FM_DIGITAL_REG(0x40000060) |= (1 << 21);


    FM_DIGITAL_REG(0x40000060) |= (1 << 24);
    FM_DIGITAL_REG(0x40000060) |= (1 << 27);
    FM_DIGITAL_REG(0x40000060) =(FM_DIGITAL_REG(0x40000060) &  ~ (1 << 20));








    FM_DIGITAL_REG(0x40000060) |= (1 << 29);
//    FM_DIGITAL_REG(0x40000060) |= (1 << 27);
    FM_DIGITAL_REG(0x40000064) = (FM_DIGITAL_REG(0x40000064) & ~0xFF) | 0x7A | (1 << 10) | (1<<30);
#endif

    FM_DIGITAL_REG(0x4000a040) = 0xc0810000;
    FM_DIGITAL_REG(0x4000a044) = 0x08040c04;
    FM_DIGITAL_REG(0x4000a048) = 0x0e01f268;
    FM_DIGITAL_REG(0x4000a04c) = 0x00005100;
    //    FM_DIGITAL_REG(0x40010010) = 0;
    //    FM_DIGITAL_REG(0x40010014) = 0x03a80005;
    //FM_DIGITAL_REG(0x40010018) = 0x00200019;
    FM_DIGITAL_REG(0x4000a050) = 0x24200000; //for adc_div_3_6 bypass
    FM_DIGITAL_REG(0x4000a050) = (FM_DIGITAL_REG(0x4000a050) & ~0x780) | 0x380; // for channel 1 adc volume, bit10~7
    FM_DIGITAL_REG(0x4000a050) = (FM_DIGITAL_REG(0x4000a050) & ~0x18000) | 0x18000; // for dual channel adc/dac

#ifdef SINGLECHANLE
    //0x4000a050  bit16  写0   单channel dac for codec
    FM_DIGITAL_REG(0x4000a050) =(FM_DIGITAL_REG(0x4000a050) &  ~ (1 << 16));
#endif


    FM_DIGITAL_REG(0x4000a048) = (FM_DIGITAL_REG(0x4000a048) & ~0x00000f00) | 0x40000900; //set for sdm gain
    FM_DIGITAL_REG(0x4000a044) = (FM_DIGITAL_REG(0x4000a044) & ~0x60000000) | 0x60000000; //for adc en, and dac en

    // Start DAC
    FM_DIGITAL_REG(0x4000a010) |= (1 << 1);



// Delay 2 ms
//    for (volatile int kk = 0; kk < 1000/64; kk++);
    osDelay(2);

    //hal_sys_timer_delay(MS_TO_TICKS(2));
    // Start ADC
    // FM_DIGITAL_REG(0x4000a010) |= (1 << 0);


#ifdef ATAN2_HARDWARE


#ifdef FM_NEWMODE
    FM_DIGITAL_REG(0x40160030) = 1;
   FM_DIGITAL_REG(0x40160000) = 0x1;

#else
    FM_DIGITAL_REG(0x40160030) = 1;
   FM_DIGITAL_REG(0x40160000) = 0x21;
#endif

#else
    //start FM
    FM_DIGITAL_REG(0x40160030) = 1;
    FM_DIGITAL_REG(0x40160000) =1;

#endif
}

int fm_radio_analog_init(void)
{
    int ret;


    /*

    // fm initial
    rfspi_wvalue( 8'h2c , 16'b0111_0000_0101_1100 ) ; // dig_vtoi_en
    rfspi_wvalue( 8'h01 , 16'b1010_1101_1111_1111 ) ; // power on fm lna
    rfspi_wvalue( 8'h02 , 16'b1000_0000_1001_0100 ) ; // reg_fm_lna_pu_mixersw

    rfspi_wvalue( 8'h1a , 16'b0101_0000_1011_0000 ) ; // reg_bt_vco_fm_buff_vctrl_dr=1

    rfspi_wvalue( 8'h18 , 16'b0000_0110_1000_0000 ) ; // power on vco

    rfspi_wvalue( 8'h19 , 16'b0110_0100_0100_0000 ) ; // reg_bt_vco_fm_buff_vctrl
    rfspi_wvalue( 8'h1d , 16'b0111_1000_1010_0100 ) ; // reg_bt_rfpll_pu_dr
    rfspi_wvalue( 8'h1c , 16'b0000_0000_1100_1000 ) ; // reg_bt_vco_fm_lo_en reg_bt_vco_fm_div_ctrl=8

    rfspi_wvalue( 8'h0a , 16'b0001_0010_0010_1111 ) ; // reg_btfm_flt_fm_en

    rfspi_wvalue( 8'h2d , 16'b0000_0111_1000_0010 ) ; // bb ldo on reg_bb_ldo_pu_vddr15a_dr
    rfspi_wvalue( 8'h07 , 16'b0000_0010_1011_1001 ) ; // reg_btfm_flt_pu_dr

    rfspi_wvalue( 8'h2a , 16'b0001_0110_1100_0000 ) ; // reg_bt_rfpll_sdm_freq_dr
    rfspi_wvalue( 8'h26 , 16'b0000_0000_0000_0000 ) ; // vco freq[31:16] ( 2400 + x )*2^25/26MHZ*N (2400+x= frf)
    rfspi_wvalue( 8'h25 , 16'b0000_0000_0000_0000 ) ; // vco freq[15:00] fm_freq = frf/(4*reg_bt_vco_fm_div_ctrl)
    rfspi_wvalue( 8'h17 , 16'b1000_0000_0000_0000 ) ; // reg_bt_vco_calen

    */






    fm_write_rf_reg( 0x2c , 0b0111000001011100 ) ; // dig_vtoi_en
    fm_write_rf_reg( 0x01 , 0b1010110111111111 ) ; // power on fm lna
    fm_write_rf_reg( 0x02 , 0b1000000010010100 ) ; // reg_fm_lna_pu_mixersw

    fm_write_rf_reg( 0x1a , 0b0101000010110000 ) ; // reg_bt_vco_fm_buff_vctrl_dr=1

    fm_write_rf_reg( 0x18 , 0b0000011010000000 ) ; // power on vco

    fm_write_rf_reg( 0x19 , 0b0110010001000000 ) ; // reg_bt_vco_fm_buff_vctrl
    fm_write_rf_reg( 0x1d , 0b0111100010100100 ) ; // reg_bt_rfpll_pu_dr
    fm_write_rf_reg( 0x1c , 0b0000000011001000 ) ; // reg_bt_vco_fm_lo_en reg_bt_vco_fm_div_ctrl=8

    fm_write_rf_reg( 0x0a , 0b0001001000101111 ) ; // reg_btfm_flt_fm_en

    fm_write_rf_reg( 0x2d , 0b0000011110000010 ) ; // bb ldo on reg_bb_ldo_pu_vddr15a_dr
    fm_write_rf_reg( 0x07 , 0b0000001010111001 ) ; // reg_btfm_flt_pu_dr

    fm_write_rf_reg( 0x2a , 0b0001011011000000 ) ; // reg_bt_rfpll_sdm_freq_dr
    fm_write_rf_reg( 0x26 , 0b0000000000000000 ) ; // vco freq[31:16] ( 2400 + x )*2^25/26MHZ*N (2400+x= frf)
    fm_write_rf_reg( 0x25 , 0b0000000000000000 ) ; // vco freq[15:00] fm_freq = frf/(4*reg_bt_vco_fm_div_ctrl)
    fm_write_rf_reg( 0x17 , 0b1000000000000000 ) ; // reg_bt_vco_calen







    //adc也要开的话，需要配 cmu
    //0x40000060[29] = 1  最好先读再写，否则把别的bit冲掉了。




    //需要配置的spi寄存器：ana interface:

    //0x05 = 0xFCB1 // Audio Pll
    //0x06 = 0x881C
    //0x31 = 0x0100 // audio_freq_en
    //0x37 = 0x1000 // codec_bbpll1_fm_adc_clk_en
    //0x31 = 0x0130 // codec_tx_en_ldac codec_tx_en_rdac


    ret = fm_write_rf_reg(0x05 , 0xfcb1);
    if (ret) {
        return ret;
    }
    ret = fm_write_rf_reg(0x06 , 0x881c);
    if (ret) {
        return ret;
    }

    ret = fm_write_rf_reg(0x3a , 0xe644);
    if (ret) {
        return ret;
    }
    ret = fm_write_rf_reg(0x31 , 0x0100);
    if (ret) {
        return ret;
    }
    ret = fm_write_rf_reg(0x37 , 0x1000);
    if (ret) {
        return ret;
    }
    ret = fm_write_rf_reg(0x31 , 0x01f0);
    if (ret) {
        return ret;
    }


    //delay 32ms
    osDelay(32);


    ret = fm_write_rf_reg(0x31 , 0x0130);
    if (ret) {
        return ret;
    }

    //[FM_RX]
    fm_write_rf_reg(0x01,0x91ff); //pu fm
    fm_write_rf_reg(0x2d,0x07fa); //ldo on
    fm_write_rf_reg(0x2e,0x6aaa); //tune fm filter IF
    fm_write_rf_reg(0x02,0xe694);
    fm_write_rf_reg(0x03,0xfe3a);
    fm_write_rf_reg(0x04,0x52a8);
    fm_write_rf_reg(0x07,0x02b9);
    fm_write_rf_reg(0x0a,0x1a2c);
    fm_write_rf_reg(0x0b,0x402b);
    fm_write_rf_reg(0x0c,0x7584);
    fm_write_rf_reg(0x0e,0x0000);
    fm_write_rf_reg(0x0f,0x2e18);
    fm_write_rf_reg(0x10,0x02b4);
    fm_write_rf_reg(0x13,0x0a48);

    //[vco init]
    fm_write_rf_reg(0x18,0x077f);
    fm_write_rf_reg(0x19,0x3ff8);
    fm_write_rf_reg(0x1a,0xc090);
    fm_write_rf_reg(0x1b,0x0f88);
    fm_write_rf_reg(0x1c,0x04c6); //[3:0] 5,6,7,8 --> vco/2

    return 0;
}

void  fm_radio_poweron(void)

{
    hal_cmu_reset_clear(HAL_CMU_MOD_BTCPU);
    osDelay(2000);

    {
        //wakp interface
        unsigned short read_val;

        fm_read_rf_reg(0x50, &read_val);
    }

    pmu_fm_config(1);

    fm_write_rf_reg(0x0c, 0x3584);
        if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2 || hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_3) ////
        {
            FM_DIGITAL_REG(0xc00003b4)=0x00060020;//turn off bt sleep
        }
        else if(hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_4)
        {
            FM_DIGITAL_REG(0xc00003b0)=0x00060020;//turn off bt sleep
        }
        else
        {
        FM_DIGITAL_REG(0xc00003ac)=0x00060020;//turn off bt sleep
        }
    FM_DIGITAL_REG(0xd0330038) = 0x00008D0D;
    FM_DIGITAL_REG(0xd0340020)=0x010E01C0;// open ana rxon for open adc clk
    //fm_write_rf_reg(0x02, 0xe694);
}

void* fm_radio_get_ext_buff(int size)
{
    uint8_t *pBuff = NULL;
    size = size+size%4;
    app_audio_mempool_get_buff(&pBuff, size);
    return (void*)pBuff;
}

int fm_radio_player(bool on)
{
    static struct AF_STREAM_CONFIG_T stream_cfg;
    static bool isRun =  false;
    uint8_t *buff = NULL;

    TRACE(2,"fm_radio_player work:%d op:%d", isRun, on);
    if (isRun==on)
        return 0;

    if (on){
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
        app_audio_mempool_init();
        fm_radio_poweron();
        fm_radio_analog_init();
        fm_radio_digit_init();
        osDelay(200);
        buff = (uint8_t *)fm_radio_get_ext_buff(FM_AUDIO_BUFFER_SIZE*2);
        app_audio_pcmbuff_init(buff, FM_AUDIO_BUFFER_SIZE*2);
        fm_sample_buffer_p = (int32_t *)fm_radio_get_ext_buff(FM_SAMPLE_BUFFER_SIZE);
#if FPGA==0
        app_overlay_select(APP_OVERLAY_FM);
#endif
        memset(&stream_cfg, 0, sizeof(stream_cfg));
        stream_cfg.vol = app_bt_stream_local_volume_get();
        stream_cfg.handler = fm_capture_more_data;
        stream_cfg.data_ptr = (uint8_t *)fm_sample_buffer_p;
        stream_cfg.data_size =  FM_SAMPLE_BUFFER_SIZE;
        stream_cfg.device = AUD_STREAM_USE_DPD_RX;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);

        memset(&stream_cfg, 0, sizeof(stream_cfg));
        buff = (uint8_t *)fm_radio_get_ext_buff(FM_AUDIO_BUFFER_SIZE);
        stream_cfg.bits = AUD_BITS_16;
        stream_cfg.channel_num = AUD_CHANNEL_NUM_1;
        stream_cfg.sample_rate = AUD_SAMPRATE_48000;
#if FPGA==0
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
#else
        stream_cfg.device = AUD_STREAM_USE_EXT_CODEC;
#endif
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.vol = app_bt_stream_local_volume_get();
        stream_cfg.handler = fm_pcm_more_data;
        stream_cfg.data_ptr = buff;
        stream_cfg.data_size = FM_AUDIO_BUFFER_SIZE;
        af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);

    }else{
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        af_stream_close(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    }

    isRun=on;
    return 0;
}

int fm_tune(uint32_t freqkhz)
{
    uint32_t reg;
    unsigned long long tmp = 0;


    //[rfpll_cal]
    fm_write_rf_reg(0x21,0x3979); // ref sel 52MHz
    fm_write_rf_reg(0x22,0x7A22); // doubler setting
    fm_write_rf_reg(0x23,0x0380);
    fm_write_rf_reg(0x2b,0x32a0); // sdm
    fm_write_rf_reg(0x2a,0x12d1); // cal ini

    //(freq(Mhz)-0.89(Mhz))*(2^28)*3/26
    tmp = freqkhz;
    reg =(((tmp-890))<<27)*3/13/1000;

    fm_write_rf_reg(0x25, (reg&0xffff0000)>>16);
    fm_write_rf_reg(0x26, reg&0x0000ffff);

    fm_write_rf_reg(0x1d,0x58e4); // pll_cal_en
    fm_write_rf_reg(0xf7,0x5597); // rst and enable pll_cal clk
    fm_write_rf_reg(0xf7,0x55d7); // rst and enable pll_cal clk
    fm_write_rf_reg(0x1d,0x7ae4); // pll cal start
    fm_write_rf_reg(0xff,0x0000); // wait 100us

    osDelay(20);


    fm_write_rf_reg(0x1d,0x7ac4); // close pll loop

    return 0;
}

void fm_test_main(void)
{
    fm_radio_player(true);
    osDelay(20);
    fm_tune(90500);
}

#endif

