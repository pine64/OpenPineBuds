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
#ifndef __A2DP_DECODER_INTERNAL_H__
#define __A2DP_DECODER_INTERNAL_H__

#include "list.h"
#include "a2dp_decoder.h"
#ifdef A2DP_CP_ACCEL
#include "a2dp_decoder_cp.h"
#include "hal_location.h"
#endif
#include "a2dp_decoder_trace.h"

#define TEXT_A2DP_LOC_A(n, l)               __attribute__((section(#n "." #l)))
#define TEXT_A2DP_LOC(n, l)                 TEXT_A2DP_LOC_A(n, l)

#define TEXT_SBC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_sbc, __LINE__)
#define TEXT_AAC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_aac, __LINE__)
#define TEXT_SSC_LOC                        TEXT_A2DP_LOC(.overlay_a2dp_ssc, __LINE__)
#define TEXT_LDAC_LOC                       TEXT_A2DP_LOC(.overlay_a2dp_ldac, __LINE__)
#define TEXT_LHDC_LOC                       TEXT_A2DP_LOC(.overlay_a2dp_lhdc, __LINE__)

#define A2DP_DECODER_NO_ERROR              (0)
#define A2DP_DECODER_DECODE_ERROR          (-1)
#define A2DP_DECODER_BUSY_ERROR            (-2)
#define A2DP_DECODER_MEMORY_ERROR          (-3)
#define A2DP_DECODER_MTU_LIMTER_ERROR      (-4)
#define A2DP_DECODER_CACHE_UNDERFLOW_ERROR (-5)
#define A2DP_DECODER_SYNC_ERROR            (-6)
#define A2DP_DECODER_NOT_SUPPORT           (-128)

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b) ((a)<(b) ? (a):(b))
#ifdef MAX
#undef MAX
#endif
#define MAX(a,b) ((a)>(b) ? (a):(b))

typedef A2DP_AUDIO_OUTPUT_CONFIG_T AUDIO_DECODER_STREAM_INFO_T;

typedef int (*AUDIO_DECODER_INIT)(A2DP_AUDIO_OUTPUT_CONFIG_T *, void *);
typedef int (*AUDIO_DECODER_DEINIT)(void);
typedef int (*AUDIO_DECODER_DECODE_FRAME)(uint8_t *, uint32_t);
typedef int (*AUDIO_DECODER_PREPARSE_PACKET)(btif_media_header_t *, uint8_t *, uint32_t);
typedef int (*AUDIO_DECODER_STORE_PACKET)(btif_media_header_t *, uint8_t *, uint32_t);
typedef int (*AUDIO_DECODER_DISCARDS_PACKET)(uint32_t);
typedef int (*AUDIO_DECODER_SYNCHRONIZE_PACKET)(A2DP_AUDIO_SYNCFRAME_INFO_T *, uint32_t);
typedef int (*AUDIO_DECODER_SYNCHRONIZE_DEST_PACKET_MUT)(uint16_t);
typedef int (*AUDIO_DECODER_HEADFRAME_INFO_GET)(A2DP_AUDIO_HEADFRAME_INFO_T*);
typedef int (*AUDIO_DECODER_INFO_GET)(void *);
typedef void(*AUDIO_DECODER_PACKET_FREE)(void *);
typedef int (*AUDIO_DECODER_SYNCHRONIZE_CONVERT_LIST_TO_SAMPLES)(uint32_t *);
typedef int (*AUDIO_DECODER_SYNCHRONIZE_DISCARDS_SAMPLES)(uint32_t);
typedef int (*AUDIO_DECODER_CHANNEL_SELECT)(A2DP_AUDIO_CHANNEL_SELECT_E);

typedef struct {
    AUDIO_DECODER_STREAM_INFO_T stream_info;
    uint32_t auto_synchronize_support;
    AUDIO_DECODER_INIT audio_decoder_init;
    AUDIO_DECODER_DEINIT audio_decoder_deinit;
    AUDIO_DECODER_DECODE_FRAME audio_decoder_decode_frame;
    AUDIO_DECODER_PREPARSE_PACKET audio_decoder_preparse_packet;
    AUDIO_DECODER_STORE_PACKET audio_decoder_store_packet;
    AUDIO_DECODER_DISCARDS_PACKET audio_decoder_discards_packet;
    AUDIO_DECODER_SYNCHRONIZE_PACKET audio_decoder_synchronize_packet;
    AUDIO_DECODER_SYNCHRONIZE_DEST_PACKET_MUT audio_decoder_synchronize_dest_packet_mut;
    AUDIO_DECODER_SYNCHRONIZE_CONVERT_LIST_TO_SAMPLES a2dp_audio_convert_list_to_samples;
    AUDIO_DECODER_SYNCHRONIZE_DISCARDS_SAMPLES a2dp_audio_discards_samples;
    AUDIO_DECODER_HEADFRAME_INFO_GET audio_decoder_headframe_info_get;
    AUDIO_DECODER_INFO_GET audio_decoder_info_get;
    AUDIO_DECODER_PACKET_FREE audio_decoder_packet_free;
    AUDIO_DECODER_CHANNEL_SELECT audio_decoder_channel_select;
} A2DP_AUDIO_DECODER_T;

typedef struct {    
    list_t *input_raw_packet_list;
    list_t *output_pcm_packet_list;
} A2DP_AUDIO_DATAPATH_T;

typedef struct {    
    bool  enalbe;
    void *semaphore;
} AUDIO_BUFFER_SEMAPHORE_T;

enum A2DP_AUDIO_DECODER_STATUS {
    A2DP_AUDIO_DECODER_STATUS_NULL,
    A2DP_AUDIO_DECODER_STATUS_READY,
    A2DP_AUDIO_DECODER_STATUS_START,
    A2DP_AUDIO_DECODER_STATUS_STOP
};

enum A2DP_AUDIO_DECODER_STORE_PACKET_STATUS {
    A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_IDLE,
    A2DP_AUDIO_DECODER_STORE_PACKET_STATUS_BUSY,
};

enum A2DP_AUDIO_DECODER_PLAYBACK_STATUS {
    A2DP_AUDIO_DECODER_PLAYBACK_STATUS_IDLE,
    A2DP_AUDIO_DECODER_PLAYBACK_STATUS_BUSY,
};

typedef struct {
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint16_t curSubSequenceNumber;
    uint16_t totalSubSequenceNumber;
    uint32_t frame_samples;
    uint32_t list_samples;
    uint32_t decoded_frames;
    uint32_t undecode_frames;
    uint32_t check_sum;
    A2DP_AUDIO_OUTPUT_CONFIG_T stream_info;
} A2DP_AUDIO_DECODER_LASTFRAME_INFO_T;

typedef struct {
    A2DP_AUDIO_SYNC_PID_T pid;
    uint32_t tick;
    uint32_t cnt;
} A2DP_AUDIO_SYNC_T;

typedef struct {    
    A2DP_AUDIO_OUTPUT_CONFIG_T output_cfg;
    float init_factor_reference;
    A2DP_AUDIO_CHANNEL_SELECT_E chnl_sel;
    uint16_t dest_packet_mut;
    float average_packet_mut;
    A2DP_AUDIO_DECODER_T audio_decoder;
    A2DP_AUDIO_DATAPATH_T audio_datapath;
    list_t *input_raw_packet_list;
    list_t *output_pcm_packet_list;
    bool need_detect_first_packet;
    bool underflow_onporcess;
    uint32_t skip_frame_cnt_after_no_cache;
    uint32_t mute_frame_cnt_after_no_cache;
    AUDIO_BUFFER_SEMAPHORE_T audio_semaphore;
    void *audio_buffer_mutex;
    void *audio_status_mutex;
    void *audio_stop_mutex;
    enum A2DP_AUDIO_DECODER_STATUS audio_decoder_status;
    enum A2DP_AUDIO_DECODER_STORE_PACKET_STATUS store_packet_status;
    enum A2DP_AUDIO_DECODER_PLAYBACK_STATUS playback_status;
    A2DP_AUDIO_SYNC_T audio_sync;
#if A2DP_DECODER_HISTORY_SEQ_SAVE
    uint16_t historySeq[A2DP_DECODER_HISTORY_SEQ_SAVE];
#ifdef A2DP_DECODER_HISTORY_LOCTIME_SAVE
    uint32_t historyLoctime[A2DP_DECODER_HISTORY_SEQ_SAVE];
#endif
#ifdef A2DP_DECODER_HISTORY_CHECK_SUM_SAVE
    uint32_t historyChecksum[A2DP_DECODER_HISTORY_SEQ_SAVE];
#endif
    uint8_t historySeq_idx;
#endif
} A2DP_AUDIO_CONTEXT_T;

#ifdef __cplusplus
extern "C" {
#endif

void *a2dp_audio_heap_malloc(uint32_t size);
void *a2dp_audio_heap_cmalloc(uint32_t size);
void *a2dp_audio_heap_realloc(void *rmem, uint32_t newsize);
void a2dp_audio_heap_free(void *rmem);

list_node_t *a2dp_audio_list_begin(const list_t *list);
list_node_t *a2dp_audio_list_end(const list_t *list);
uint32_t a2dp_audio_list_length(const list_t *list);
void *a2dp_audio_list_node(const list_node_t *node);
list_node_t *a2dp_audio_list_next(const list_node_t *node);
bool a2dp_audio_list_remove(list_t *list, void *data);
bool a2dp_audio_list_append(list_t *list, void *data);
void a2dp_audio_list_clear(list_t *list);
void a2dp_audio_list_free(list_t *list);
list_t *a2dp_audio_list_new(list_free_cb callback, list_mempool_zmalloc zmalloc, list_mempool_free free);

int a2dp_audio_semaphore_wait(uint32_t timeout_ms);
int a2dp_audio_semaphore_release(void);
int a2dp_audio_decoder_internal_lastframe_info_set(A2DP_AUDIO_DECODER_LASTFRAME_INFO_T *lastframe_info);
uint32_t a2dp_audio_get_passed(uint32_t curr_ticks, uint32_t prev_ticks, uint32_t max_ticks);
uint32_t a2dp_audio_decoder_internal_check_sum_generate(const uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif//__A2DPPLAY_H__

