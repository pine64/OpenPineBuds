/*******************************************************************************
** namer：	speech_eq
** description:	fir and iir eq manager
** version：V0.9
** author： Yunjie Huo
** modify：	2016.12.26
** todo: 	1.
*******************************************************************************/
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "string.h"
#include "hw_filter_codec_iir.h"
#include "stdbool.h"
#include "hal_location.h"
#include "hw_codec_iir_process.h"
#include "tgt_hardware.h"

#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV                    CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif

struct hw_filter_codec_iir_state_
{
    // public
    /* Basic info */
    int32_t     sample_rate;
    // int32_t     frame_size;
    int32_t     channel_num;
    int32_t     bits;

    /* configure info */ 
    int32_t     bypass;
    int32_t     need_value;
    HW_CODEC_IIR_TYPE_T iir_device;

    // private
    /* Parameters */
    int32_t     iir_device_opened;

    /* DSP-related arrays */

    /* Misc */
    // input and output buffer

};

// just support 1 st
hw_filter_codec_iir_state iir_st;

hw_filter_codec_iir_state *hw_filter_codec_iir_create(int32_t sample_rate, int32_t channel_num, int32_t bits, hw_filter_codec_iir_cfg *cfg)
{
    hw_filter_codec_iir_state *st = &iir_st;

    TRACE(4,"[%s] sample_rate = %d, channel_num = %d, bits = %d", __func__, sample_rate, channel_num, bits);

    // Set parameters
    st->sample_rate = sample_rate;
    st->channel_num  = channel_num;
    st->bits  = bits;

    // Initialize internal variables
    st->iir_device_opened = 0;
    
    // Malloc dynamic memory
    
    // Initialize dynamic memory

    hw_filter_codec_iir_set_config(st, cfg);

    return st;
}

int32_t hw_filter_codec_iir_destroy(hw_filter_codec_iir_state *st)
{
    if (st->iir_device_opened)
    {
        hw_codec_iir_close(st->iir_device);
    }

    st->bypass = 0;
    st->iir_device = HW_CODEC_IIR_NOTYPE;
    st->iir_device_opened = 0;

    return 0;
}

int32_t hw_filter_codec_iir_set_config(hw_filter_codec_iir_state *st, hw_filter_codec_iir_cfg *cfg)
{
    st->bypass = cfg->bypass;
    st->iir_device = cfg->iir_device;

    TRACE(2,"[%s] iir device = %d", __func__, st->iir_device);

    if (!st->iir_device_opened)
    {
        hw_codec_iir_open((enum AUD_SAMPRATE_T)st->sample_rate, st->iir_device,CODEC_OUTPUT_DEV);
    }

#if 1
    hw_codec_iir_set_coefs(&cfg->iir_cfg, st->iir_device); 
#else
    HW_CODEC_IIR_CFG_T *hw_iir_cfg=NULL; 

    hw_iir_cfg = hw_codec_iir_get_cfg((enum AUD_SAMPRATE_T)st->sample_rate, &cfg->iir_cfg);
    ASSERT(hw_iir_cfg != NULL, "[%s] codec IIR parameter error!", __func__);
    hal_codec_iir_dump(hw_iir_cfg);
    hw_codec_iir_set_cfg(hw_iir_cfg, (enum AUD_SAMPRATE_T)st->sample_rate, st->iir_device);
#endif

    

    return 0;
}

// int32_t hw_filter_codec_iir_ctl(hw_filter_codec_iir_state *st, int32_t ctl, void *ptr)
// {
//     switch(ctl)
//     {
//     case HW_FILTER_CODEC_IIR_SET_AAA:
//         st->aaa = (*(int32_t*)ptr);
//         break; 

//     case HW_FILTER_CODEC_IIR_GET_AAA:
//         (*(int32_t*)ptr) = (int32_t)st->aaa;
//         break;

//     default:
//         TRACE(2,"[%s] ctl(%d) is not valid", __func__, ctl);
//         return -1;
//     }

//     return 0;    
// }

int32_t hw_filter_codec_iir_dump(hw_filter_codec_iir_state *st)
{
    TRACE(1,"[%s]: ", __func__);

    // Add dump info

    // End
    
    return 0;
}