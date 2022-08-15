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

static A2DP_AUDIO_CONTEXT_T *a2dp_audio_context_p = NULL;
static A2DP_AUDIO_DECODER_LASTFRAME_INFO_T a2dp_audio_ldac_example_info;
static A2DP_AUDIO_OUTPUT_CONFIG_T a2dp_audio_example_output_config;

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint8_t *buffer;
    uint32_t buffer_len;
} a2dp_audio_example_decoder_frame_t;


int a2dp_audio_example_decode_frame(uint8_t *buffer, uint32_t buffer_bytes)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_preparse_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    return A2DP_DECODER_NO_ERROR;
}


static void *a2dp_audio_example_frame_malloc(uint32_t packet_len)
{
    a2dp_audio_example_decoder_frame_t *decoder_frame_p = NULL;
    uint8_t *buffer = NULL;

    buffer = (uint8_t *)a2dp_audio_heap_malloc(packet_len);
    decoder_frame_p = (a2dp_audio_example_decoder_frame_t *)a2dp_audio_heap_malloc(sizeof(a2dp_audio_example_decoder_frame_t));
    decoder_frame_p->buffer = buffer;
    decoder_frame_p->buffer_len = packet_len;
    return (void *)decoder_frame_p;
}

void a2dp_audio_example_free(void *packet)
{
    a2dp_audio_example_decoder_frame_t *decoder_frame_p = (a2dp_audio_example_decoder_frame_t *)packet; 
    a2dp_audio_heap_free(decoder_frame_p->buffer);
    a2dp_audio_heap_free(decoder_frame_p);
}

int a2dp_audio_example_store_packet(btif_media_header_t * header, uint8_t *buffer, uint32_t buffer_bytes)
{
    list_t *list = a2dp_audio_context_p->audio_datapath.input_raw_packet_list;
    a2dp_audio_example_decoder_frame_t *decoder_frame_p = (a2dp_audio_example_decoder_frame_t *)a2dp_audio_example_frame_malloc(buffer_bytes);

    decoder_frame_p->sequenceNumber = header->sequenceNumber;
    decoder_frame_p->timestamp = header->timestamp;
    memcpy(decoder_frame_p->buffer, buffer, buffer_bytes);
    decoder_frame_p->buffer_len = buffer_bytes;
    a2dp_audio_list_append(list, decoder_frame_p);       

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_discards_packet(uint32_t packets)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_headframe_info_get(A2DP_AUDIO_HEADFRAME_INFO_T* headframe_info)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_info_get(void *info)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_init(A2DP_AUDIO_OUTPUT_CONFIG_T *config, void *context)
{
    TRACE_A2DP_DECODER_D("%s", __func__);
    a2dp_audio_context_p = (A2DP_AUDIO_CONTEXT_T *)context;

    memset(&a2dp_audio_ldac_example_info, 0, sizeof(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T));    
    memcpy(&a2dp_audio_example_output_config, config, sizeof(A2DP_AUDIO_OUTPUT_CONFIG_T));
    a2dp_audio_ldac_example_info.stream_info = &a2dp_audio_example_output_config;

    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_deinit(void)
{
    return A2DP_DECODER_NO_ERROR;
}

int  a2dp_audio_example_synchronize_packet(A2DP_AUDIO_SYNCFRAME_INFO_T *sync_info, uint32_t mask)
{
    return A2DP_DECODER_NO_ERROR;
}

int a2dp_audio_example_synchronize_dest_packet_mut(uint16_t packet_mut)
{
    return A2DP_DECODER_NO_ERROR;
}

A2DP_AUDIO_DECODER_T a2dp_audio_example_decoder_config = {
                                                        {44100, 2, 16},
                                                        0,
                                                        a2dp_audio_example_init,
                                                        a2dp_audio_example_deinit,
                                                        a2dp_audio_example_decode_frame,
                                                        a2dp_audio_example_preparse_packet,
                                                        a2dp_audio_example_store_packet,
                                                        a2dp_audio_example_discards_packet,
                                                        a2dp_audio_example_synchronize_packet,
                                                        a2dp_audio_example_synchronize_dest_packet_mut,
                                                        a2dp_audio_example_headframe_info_get,
                                                        a2dp_audio_example_info_get,
                                                        a2dp_audio_example_free,
                                                     } ;

