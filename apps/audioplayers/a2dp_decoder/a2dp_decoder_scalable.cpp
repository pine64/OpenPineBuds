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
#include "cmsis.h"
#include "plat_types.h"
#include <string.h>
#include "heap_api.h"
#include "hal_location.h"
#include "a2dp_decoder_internal.h"
#include "app_audio.h"
#if defined(A2DP_SCALABLE_ON)
#include "heap_api.h"
#include "ssc.h"

#define SCALABLE_MTU_LIMITER (32)
#define  SCALABLE_MEMPOOL_SIZE  1024

#define SCALABLE_OUTPUT_FRAME_SAMPLES (SCALABLE_FRAME_SIZE)

typedef void *HANDLE_DECODER;
static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_OUTPUT_CONFIG_T output_config;

static unsigned char *scalable_decoder_place = NULL;
static unsigned char *scalable_decoder_temp_buf = NULL;
static short ss_pcm_buff[SCALABLE_FRAME_SIZE*4];
static int scalable_uhq_flag __attribute__((unused)) = 0;

static HANDLE_DECODER scalableDec_handle = NULL;

static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T lastframe_info;


typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint8_t *buffer;
    int buffer_len;
} a2dp_audio_scalable_decoder_frame_t;

static void ss_to_24bit_buf(int32_t * out, int32_t * in, int size)
{
    for (int i = 0; i < size; i++) {
    out[i] = in[i];
    }
}

static void a2dp_audio_scalable_decoder_init(void)
{
    if (scalableDec_handle == NULL) {
        scalableDec_handle = scalable_decoder_place;


        ssc_decoder_init(scalableDec_handle, output_config.num_channels, output_config.sample_rate);

    }
}

static int scalableDecoder_Close(HANDLE_DECODER handle)
{
    if (handle) {
    a2dp_audio_heap_free(handle);
    a2dp_audio_heap_free(scalable_decoder_temp_buf);
    }

    return 0;
}

static void a2dp_audio_scalable_decoder_deinit(void)
{
    if (scalableDec_handle) {
    scalableDecoder_Close(scalableDec_handle);
    scalableDec_handle = NULL;
    }
}

static void a2dp_audio_scalable_decoder_reinit(void)
{
    if (scalableDec_handle) {
    a2dp_audio_scalable_decoder_deinit();
    }
    a2dp_audio_scalable_decoder_init();
}

static bool is_valid_frame(a2dp_audio_scalable_decoder_frame_t * decoder_frame_p)
{
    int hw_tmp, len, bitrate_bps, frame_len, frame_size;

    int sampling_rate = 44100;
    unsigned char *input_buf = decoder_frame_p->buffer;
    if (decoder_frame_p->buffer_len < SCALABLE_HEAD_SIZE) {
        TRACE_A2DP_DECODER_E("invalid scalable a2dp frame, length < SCALABLE_HEAD_SIZE !!!!!!!");
        return false;
    }

    scalable_uhq_flag = 0;

    switch ((input_buf[3]&0xf7)) {
        case 0xF0:
            bitrate_bps = 88000;
            break;
        case 0xF1:
            bitrate_bps = 96000;
            break;
        case 0xF2:
            bitrate_bps = 128000;
            break;
        case 0xF3:
            bitrate_bps = 192000;
            break;
        case 0xF4:
            bitrate_bps = 229000;
            break;
        case 0xF5:
            scalable_uhq_flag = 1;
            bitrate_bps = 328000;
            sampling_rate = 96000;
            break;
        default:
            bitrate_bps = 192000;
            break;
    }

    frame_size = SCALABLE_FRAME_SIZE;

    len = bitrate_bps * frame_size / sampling_rate / 8;
    if (scalable_uhq_flag == 0) {
    hw_tmp = (len * 3) >> 7;
    len = hw_tmp + len;
    len = len + ((len & 1) ^ 1);
    } else {
    len = 369;      //744/2-4+1
    }
    TRACE_A2DP_DECODER_D
    ("scalable a2dp frame, length:%d bitrate:%d sampling_rate:%d",
     decoder_frame_p->buffer_len, bitrate_bps, sampling_rate);
    frame_len = SCALABLE_HEAD_SIZE + len - 1;

    if (decoder_frame_p->buffer_len < frame_len) {
    TRACE_A2DP_DECODER_E
        ("invalid scalable a2dp frame, length:%d  <  %d !!!!!!!", decoder_frame_p->buffer_len, frame_len);
    return false;
    }
    return true;

}


#ifdef A2DP_CP_ACCEL
struct A2DP_CP_scalable_IN_FRM_INFO_T {
    uint16_t sequenceNumber;
    uint32_t timestamp;
};

struct A2DP_CP_scalable_OUT_FRM_INFO_T {
    struct A2DP_CP_scalable_IN_FRM_INFO_T in_info;
    uint16_t frame_samples;
    uint16_t decoded_frames;
    uint16_t frame_idx;
    uint16_t pcm_len;
};

static bool cp_codec_reset;
extern uint32_t app_bt_stream_get_dma_buffer_samples(void);

TEXT_SSC_LOC static int a2dp_cp_scalable_mcu_decode(uint8_t * buffer, uint32_t buffer_bytes)
{
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;
    list_node_t *node = NULL;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    int ret, dec_ret;
    struct A2DP_CP_scalable_IN_FRM_INFO_T in_info;
    struct A2DP_CP_scalable_OUT_FRM_INFO_T *p_out_info;
    uint8_t *out;
    uint32_t out_len;
    uint32_t out_frame_len;
    uint32_t cp_buffer_frames_max = 0;

    cp_buffer_frames_max = app_bt_stream_get_dma_buffer_samples()/2;
    if (cp_buffer_frames_max %(lastframe_info.frame_samples) ){
        cp_buffer_frames_max =  cp_buffer_frames_max /(lastframe_info.frame_samples) +1;
    }else{
        cp_buffer_frames_max =  cp_buffer_frames_max /(lastframe_info.frame_samples);
    }

    out_frame_len = sizeof(*p_out_info) + buffer_bytes;
    ret = a2dp_cp_decoder_init(out_frame_len, cp_buffer_frames_max * 2);

    ASSERT(ret == 0, "%s: a2dp_cp_decoder_init() failed: ret=%d", __func__, ret);

    while ((node = a2dp_audio_list_begin(list)) != NULL) {
        decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
        a2dp_audio_list_node(node);
        
        if (false == is_valid_frame(decoder_frame_p)) {
            return A2DP_DECODER_DECODE_ERROR;
        }

        in_info.sequenceNumber = decoder_frame_p->sequenceNumber;
        in_info.timestamp = decoder_frame_p->timestamp;

        ret = a2dp_cp_put_in_frame(&in_info, sizeof(in_info), decoder_frame_p->buffer, decoder_frame_p->buffer_len);
        if (ret) {
            break;
        }

        a2dp_audio_list_remove(list, decoder_frame_p);
    }

    ret = a2dp_cp_get_full_out_frame((void **)&out, &out_len);
    if (ret) {
        return A2DP_DECODER_DECODE_ERROR;
    }
    if (out_len == 0) {
        memset(buffer, 0, buffer_bytes);
        a2dp_cp_consume_full_out_frame();
        return A2DP_DECODER_NO_ERROR;
    }
    ASSERT(out_len == out_frame_len, "%s: Bad out len %u (should be %u)", __func__, out_len, out_frame_len);

    p_out_info = (struct A2DP_CP_scalable_OUT_FRM_INFO_T *)out;
    if (p_out_info->pcm_len) {
        lastframe_info.sequenceNumber = p_out_info->in_info.sequenceNumber;
        lastframe_info.timestamp = p_out_info->in_info.timestamp;
        lastframe_info.curSubSequenceNumber = 0;
        lastframe_info.totalSubSequenceNumber = 0;
        lastframe_info.frame_samples = p_out_info->frame_samples;
        lastframe_info.decoded_frames += p_out_info->decoded_frames;
        lastframe_info.undecode_frames =
            a2dp_audio_list_length(list) + a2dp_cp_get_in_frame_cnt_by_index(p_out_info->frame_idx) - 1;
        a2dp_audio_decoder_internal_lastframe_info_set(&lastframe_info);
    }

    if (p_out_info->pcm_len == buffer_bytes) {
        memcpy(buffer, p_out_info + 1, p_out_info->pcm_len);
        dec_ret = A2DP_DECODER_NO_ERROR;
    } else {
        dec_ret = A2DP_DECODER_DECODE_ERROR;
    }

    ret = a2dp_cp_consume_full_out_frame();
    ASSERT(ret == 0, "%s: a2dp_cp_consume_full_out_frame() failed: ret=%d", __func__, ret);

    return dec_ret;
}

TEXT_SSC_LOC int a2dp_cp_scalable_cp_decode(void)
{
    int ret;
    enum CP_EMPTY_OUT_FRM_T out_frm_st;
    uint8_t *out;
    uint32_t out_len;
    uint8_t *dec_start;
    uint32_t dec_len;
    struct A2DP_CP_scalable_IN_FRM_INFO_T *p_in_info;
    struct A2DP_CP_scalable_OUT_FRM_INFO_T *p_out_info;
    uint8_t *in_buf;
    uint32_t in_len;
    uint32_t dec_sum;
    int error, output_samples = 0,output_byte = 0;

    if (cp_codec_reset)
    {
        cp_codec_reset      = false;
        a2dp_audio_scalable_decoder_init();
    }

    out_frm_st = a2dp_cp_get_emtpy_out_frame((void **)&out, &out_len);

    if (out_frm_st != CP_EMPTY_OUT_FRM_OK && out_frm_st != CP_EMPTY_OUT_FRM_WORKING)
    {
        return 1;
    }

    ASSERT(out_len > sizeof(*p_out_info), "%s: Bad out_len %u (should > %u)", __func__, out_len, sizeof(*p_out_info));

    p_out_info = (struct A2DP_CP_scalable_OUT_FRM_INFO_T *)out;
    if (out_frm_st == CP_EMPTY_OUT_FRM_OK)
    {
        p_out_info->pcm_len = 0;
        p_out_info->decoded_frames = 0;
    }

    ASSERT(out_len > sizeof(*p_out_info) + p_out_info->pcm_len,
       "%s: Bad out_len %u (should > %u + %u)", __func__, out_len, sizeof(*p_out_info), p_out_info->pcm_len);

    dec_start = (uint8_t *) (p_out_info + 1) + p_out_info->pcm_len;
    dec_len = out_len - (dec_start - (uint8_t *) out);
    if (!scalableDec_handle) {
        TRACE(0,"scalable_decode not ready");
        return 3;
    }

    dec_sum = 0;
    error = 0;

    while (dec_sum < dec_len && error == 0)
    {
        ret = a2dp_cp_get_in_frame((void * *) &in_buf, &in_len);

        if (ret)
        {
            break;
        }

        ASSERT(in_len > sizeof(*p_in_info), "%s: Bad in_len %u (should > %u)", __func__, in_len, sizeof(*p_in_info));
        p_in_info = (struct A2DP_CP_scalable_IN_FRM_INFO_T *)in_buf;
        in_buf += sizeof(*p_in_info);
        in_len -= sizeof(*p_in_info);
        /* decode one SSC frame */
        output_samples = ssc_decode(scalableDec_handle,(const unsigned char*)in_buf, ss_pcm_buff, SCALABLE_FRAME_SIZE, 0, 2);
        if (0 == output_samples)
        {  
            a2dp_audio_scalable_decoder_reinit();
            TRACE(0, "scalable_decode reinin codec \n");
            error = A2DP_DECODER_DECODE_ERROR;
            goto exit;
        }

        ss_to_24bit_buf((int32_t *) (dec_start+dec_sum), (int32_t *) ss_pcm_buff, output_samples);
        output_byte = output_samples * 4;
        dec_sum += output_byte;
exit:
        memcpy(&p_out_info->in_info, ss_pcm_buff, sizeof(*p_in_info));
        p_out_info->decoded_frames++;
        p_out_info->frame_samples = SCALABLE_OUTPUT_FRAME_SAMPLES;
        p_out_info->frame_idx = a2dp_cp_get_in_frame_index();

        ret = a2dp_cp_consume_in_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_in_frame() failed: ret=%d", __func__, ret);
    }

    p_out_info->pcm_len += dec_sum;

    if (error || out_len <= sizeof(*p_out_info) + p_out_info->pcm_len) {
        ret = a2dp_cp_consume_emtpy_out_frame();
        ASSERT(ret == 0, "%s: a2dp_cp_consume_emtpy_out_frame() failed: ret=%d", __func__, ret);
    }

    return error;
}
#endif

static int a2dp_audio_scalable_init(A2DP_AUDIO_OUTPUT_CONFIG_T * config, void *context)
{
    TRACE_A2DP_DECODER_D("%s", __func__);

    TRACE(0,"\n\nA2DP SSC-LC INIT\n");

    a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *) context;

    memcpy(&output_config, config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));

    memset(&lastframe_info, 0, sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));
    lastframe_info.stream_info = output_config;
    lastframe_info.frame_samples = SCALABLE_FRAME_SIZE;
    lastframe_info.list_samples = SCALABLE_FRAME_SIZE;
    a2dp_audio_decoder_internal_lastframe_info_set(&lastframe_info);

    ASSERT(a2dp_audio_context_p->dest_packet_mut < SCALABLE_MTU_LIMITER,
       "%s MTU OVERFLOW:%u/%u", __func__, a2dp_audio_context_p->dest_packet_mut, SCALABLE_MTU_LIMITER);

    int decoder_size;

    decoder_size = ssc_decoder_get_size(output_config.num_channels); //todo:  get size with codec capability

    TRACE(0, "decoder size %d", decoder_size);
    
    scalable_decoder_place = (unsigned char *)a2dp_audio_heap_malloc(decoder_size);
    ASSERT_A2DP_DECODER(scalable_decoder_place, "no memory resource for scalable_decoder_place");

    scalable_decoder_temp_buf = (unsigned char *)a2dp_audio_heap_malloc(SCALABLE_FRAME_SIZE * 16);
    ASSERT_A2DP_DECODER(scalable_decoder_temp_buf, "no memory resource for scalable_decoder_temp_buf");

#ifdef A2DP_CP_ACCEL
    int ret;
    cp_codec_reset = true;
    ret = a2dp_cp_init(a2dp_cp_scalable_cp_decode, CP_PROC_DELAY_1_FRAME);
    ASSERT(ret == 0, "%s: a2dp_cp_init() failed: ret=%d", __func__, ret);
#else
    a2dp_audio_scalable_decoder_init();
#endif

    return A2DP_DECODER_NO_ERROR;
}

static int a2dp_audio_scalable_deinit(void)
{
#ifdef A2DP_CP_ACCEL
    a2dp_cp_deinit();
#endif

    a2dp_audio_scalable_decoder_deinit();

    TRACE(0,"\n\nA2DP   SCALABLE  DEINIT\n");

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_scalable_mcu_decode_frame(uint8_t * buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;
    int ret = A2DP_DECODER_NO_ERROR;

    bool cache_underflow = false;
    int output_byte = 0, output_samples = 0;
    uint8_t *output = buffer;
    // TRACE(1,"bbd %d",buffer_bytes );
    if (buffer_bytes < (SCALABLE_FRAME_SIZE * output_config.num_channels * output_config.bits_depth / 8)) {
    TRACE(1,"scalable_decode pcm_len = %d \n", buffer_bytes);
    return A2DP_DECODER_NO_ERROR;
    }
    if (!scalableDec_handle) {
    TRACE(0,"scalable_decode not ready");
        return A2DP_DECODER_NO_ERROR;
    }

    while (output < buffer + buffer_bytes) {
        node = a2dp_audio_list_begin(list);
        if (!node) {
            TRACE(0,"scalable_decode cache underflow");
        cache_underflow = true;
        goto exit;
    } else {
        decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
        a2dp_audio_list_node(node);

        if (false == is_valid_frame(decoder_frame_p)) {
        TRACE_A2DP_DECODER_E("%s %d invalid a2dp frame", __func__, __LINE__);
        ret = A2DP_DECODER_DECODE_ERROR;
        goto exit;
        }

        /* decode one SSC frame */
        output_samples =
        ssc_decode(scalableDec_handle,
               decoder_frame_p->buffer, ss_pcm_buff, SCALABLE_FRAME_SIZE,0,2);

        TRACE_A2DP_DECODER_D
        ("scalable_decode seq:%d len:%d output_samples:%d",
         decoder_frame_p->sequenceNumber, decoder_frame_p->buffer_len, output_samples);

        if (0 == output_samples) {
        TRACE(0,"scalable_decode failed  !!!!!!");
        //if  failed reopen it again
        a2dp_audio_scalable_decoder_reinit();
        TRACE(0,"scalable_decode reinin codec \n");
        ret = A2DP_DECODER_DECODE_ERROR;
        goto exit;
        }
        ss_to_24bit_buf((int32_t *) output, (int32_t *) ss_pcm_buff, output_samples * 2);
        output_byte = output_samples * 8;


        output += output_byte;
        ASSERT(SCALABLE_FRAME_SIZE == output_samples, "scalable_decode output mismatch samples:%d", output_samples);
        lastframe_info.sequenceNumber = decoder_frame_p->sequenceNumber;
        lastframe_info.timestamp = decoder_frame_p->timestamp;
        lastframe_info.curSubSequenceNumber = 0;
        lastframe_info.totalSubSequenceNumber = 0;
        lastframe_info.frame_samples = SCALABLE_FRAME_SIZE;
        lastframe_info.decoded_frames++;
        lastframe_info.undecode_frames = a2dp_audio_list_length(list) - 1;
        a2dp_audio_decoder_internal_lastframe_info_set(&lastframe_info);
        a2dp_audio_list_remove(list, decoder_frame_p);

    }

    }

  exit:
    if (cache_underflow) {
    lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_internal_lastframe_info_set(&lastframe_info);
    ret = A2DP_DECODER_CACHE_UNDERFLOW_ERROR;
    }
    //TRACE(0,"abd");
    return ret;
}

static int a2dp_audio_scalable_decode_frame(uint8_t * buffer, uint32_t buffer_bytes)
{
#ifdef A2DP_CP_ACCEL
    return a2dp_cp_scalable_mcu_decode(buffer, buffer_bytes);
#else
    return a2dp_audio_scalable_mcu_decode_frame(buffer, buffer_bytes);
#endif
}

static int a2dp_audio_scalable_preparse_packet(btif_media_header_t * header, uint8_t * buffer, uint32_t buffer_bytes)
{
    lastframe_info.sequenceNumber = header->sequenceNumber;
    lastframe_info.timestamp = header->timestamp;
    lastframe_info.curSubSequenceNumber = 0;
    lastframe_info.totalSubSequenceNumber = 0;
    lastframe_info.frame_samples = SCALABLE_FRAME_SIZE;
    lastframe_info.list_samples = SCALABLE_FRAME_SIZE;
    lastframe_info.decoded_frames = 0;
    lastframe_info.undecode_frames = 0;
    a2dp_audio_decoder_internal_lastframe_info_set(&lastframe_info);

    TRACE(3,"%s seq:%d timestamp:%08x", __func__, header->sequenceNumber, header->timestamp);

    return A2DP_DECODER_NO_ERROR;
}

static void *a2dp_audio_scalable_frame_malloc(uint32_t packet_len)
{
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;
    uint8_t *buffer = NULL;

    buffer = (uint8_t *) a2dp_audio_heap_malloc(SCALABLE_READBUF_SIZE);
    decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
    a2dp_audio_heap_malloc(sizeof(a2dp_audio_scalable_decoder_frame_t));
    decoder_frame_p->buffer = buffer;
    decoder_frame_p->buffer_len = packet_len;
    return (void *)decoder_frame_p;
}

static void a2dp_audio_scalable_free(void *packet)
{
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *) packet;
    a2dp_audio_heap_free(decoder_frame_p->buffer);
    a2dp_audio_heap_free(decoder_frame_p);
}

static int a2dp_audio_scalable_store_packet(btif_media_header_t * header, uint8_t * buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    int nRet = A2DP_DECODER_NO_ERROR;
    if (a2dp_audio_list_length(list) < SCALABLE_MTU_LIMITER) {
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
        a2dp_audio_scalable_frame_malloc(buffer_bytes);
    TRACE_A2DP_DECODER_D("%s seq:%d len:%d", __func__, header->sequenceNumber, buffer_bytes);
    decoder_frame_p->sequenceNumber = header->sequenceNumber;
    decoder_frame_p->timestamp = header->timestamp;
    memcpy(decoder_frame_p->buffer, buffer, buffer_bytes);
    decoder_frame_p->buffer_len = buffer_bytes;
    a2dp_audio_list_append(list, decoder_frame_p);
    nRet = A2DP_DECODER_NO_ERROR;
    } else {
    TRACE(2,"%s list full current len:%d", __func__, a2dp_audio_list_length(list));
    nRet = A2DP_DECODER_MTU_LIMTER_ERROR;
    }

    return nRet;
}

static int a2dp_audio_scalable_discards_packet(uint32_t packets)
{
    int nRet = A2DP_DECODER_MEMORY_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    if (packets <= a2dp_audio_list_length(list)) {
    for (uint8_t i = 0; i < packets; i++) {
        if ((node = a2dp_audio_list_begin(list)) != NULL){
            decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
            a2dp_audio_list_node(node);
            a2dp_audio_list_remove(list, decoder_frame_p);
        }
    }
    nRet = A2DP_DECODER_NO_ERROR;
    }

    TRACE(3,"%s packets:%d nRet:%d", __func__, packets, nRet);
    return nRet;
}

static int a2dp_audio_scalable_headframe_info_get(A2DP_AUDIO_HEADFRAME_INFO_T* headframe_info)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;

    if (a2dp_audio_list_length(list)){
        if ((node = a2dp_audio_list_begin(list)) != NULL){
            decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)a2dp_audio_list_node(node);
            headframe_info->sequenceNumber = decoder_frame_p->sequenceNumber;
            headframe_info->timestamp = decoder_frame_p->timestamp;
            headframe_info->curSubSequenceNumber = 0;
            headframe_info->totalSubSequenceNumber = 0;
        }
    }else{
        memset(headframe_info, 0, sizeof(A2DP_AUDIO_HEADFRAME_INFO_T));
    }

    return A2DP_DECODER_NO_ERROR;
}

static int a2dp_audio_scalable_info_get(void *info)
{
    return A2DP_DECODER_NO_ERROR;
}

static int a2dp_audio_scalable_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T * sync_info, uint32_t mask)
{
    int nRet = A2DP_DECODER_SYNC_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    list_node_t *node = NULL;
    int list_len;
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;

#ifdef A2DP_CP_ACCEL
    a2dp_cp_reset_frame();
#endif

    list_len = a2dp_audio_list_length(list);

    for (uint16_t i = 0; i < list_len; i++) {
        if ((node = a2dp_audio_list_begin(list)) != NULL){
            decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
                a2dp_audio_list_node(node);
            if (A2DP_AUDIO_SYNCFRAME_CHK(decoder_frame_p->sequenceNumber == sync_info->sequenceNumber, A2DP_AUDIO_SYNCFRAME_MASK_SEQ,       mask)&&
                A2DP_AUDIO_SYNCFRAME_CHK(decoder_frame_p->timestamp      == sync_info->timestamp,      A2DP_AUDIO_SYNCFRAME_MASK_TIMESTAMP, mask)) {
                nRet = A2DP_DECODER_NO_ERROR;
                break;
            }
            a2dp_audio_list_remove(list, decoder_frame_p);
        }
    }

    node = a2dp_audio_list_begin(list);
    if (node) {
    decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
        a2dp_audio_list_node(node);
    TRACE(4,"%s nRet:%d SEQ:%d timestamp:%d", __func__, nRet,
          decoder_frame_p->sequenceNumber, decoder_frame_p->timestamp);
    } else {
    TRACE(2,"%s nRet:%d", __func__, nRet);
    }

    return nRet;
}

static int a2dp_audio_scalable_synchronize_dest_packet_mut(uint16_t packet_mut)
{
    list_node_t *node = NULL;
    uint32_t list_len = 0;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_scalable_decoder_frame_t *decoder_frame_p = NULL;

    list_len = a2dp_audio_list_length(list);
    if (list_len > packet_mut) {
    do {
        node = a2dp_audio_list_begin(list);
        if (node)
        {
            decoder_frame_p = (a2dp_audio_scalable_decoder_frame_t *)
            a2dp_audio_list_node(node);
            a2dp_audio_list_remove(list, decoder_frame_p);
        }
    } while (a2dp_audio_list_length(list) > packet_mut);
    }

    TRACE(2,"%s list:%d", __func__, a2dp_audio_list_length(list));

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_scalable_convert_list_to_samples(uint32_t *samples)
{
    uint32_t list_len = 0;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;

    list_len = a2dp_audio_list_length(list);
    *samples = SCALABLE_FRAME_SIZE*list_len;

    TRACE(3, "%s list:%d samples:%d", __func__, list_len, *samples);

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_scalable_discards_samples(uint32_t samples)
{
    int nRet = A2DP_DECODER_SYNC_ERROR;
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_scalable_decoder_frame_t *scalable_decoder_frame = NULL;
    list_node_t *node = NULL;
    int need_remove_list = 0;
    uint32_t list_samples = 0;
    ASSERT(!(samples%SCALABLE_FRAME_SIZE), "%s samples err:%d", __func__, samples);

    a2dp_audio_scalable_convert_list_to_samples(&list_samples);
    if (list_samples >= samples){
        need_remove_list = samples/SCALABLE_FRAME_SIZE;
        for (int i=0; i<need_remove_list; i++){
            node = a2dp_audio_list_begin(list);
            if (node)
            {
                scalable_decoder_frame = (a2dp_audio_scalable_decoder_frame_t *)a2dp_audio_list_node(node);
                a2dp_audio_list_remove(list, scalable_decoder_frame);
            }
        }
        nRet = A2DP_DECODER_NO_ERROR;
    }

    return nRet;
}
A2DP_AUDIO_DECODER_T a2dp_audio_scalable_decoder_config = {
    {44100, 2, 16},
    1,
    a2dp_audio_scalable_init,
    a2dp_audio_scalable_deinit,
    a2dp_audio_scalable_decode_frame,
    a2dp_audio_scalable_preparse_packet,
    a2dp_audio_scalable_store_packet,
    a2dp_audio_scalable_discards_packet,
    a2dp_audio_scalable_synchronize_packet,
    a2dp_audio_scalable_synchronize_dest_packet_mut,
    a2dp_audio_scalable_convert_list_to_samples,
    a2dp_audio_scalable_discards_samples,
    a2dp_audio_scalable_headframe_info_get,
    a2dp_audio_scalable_info_get,
    a2dp_audio_scalable_free,
};
#else
A2DP_AUDIO_DECODER_T a2dp_audio_scalable_decoder_config = { 0, };
#endif
