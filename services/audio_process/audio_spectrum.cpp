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
#ifdef SPEECH_LIB

#include "spectrum_fix.h"
#include "speech_memory.h"
#include "hal_aud.h"
#include "hal_trace.h"

#define AUDIO_SPECTRUM_FRAME_SIZE (256)

struct AudioSpectrum {
    SpectrumFixState *state;
    enum AUD_BITS_T bits;
    int16_t *frame;
    int16_t *data;
};

static AudioSpectrum audio_spectrum;

extern const SpectrumFixConfig audio_spectrum_cfg;

void audio_spectrum_open(int sample_rate, enum AUD_BITS_T sample_bits)
{
    uint8_t *speech_buf = NULL;
    int speech_buf_size = 1024 * 5;

    syspool_get_buff((uint8_t **)&(audio_spectrum.frame), AUDIO_SPECTRUM_FRAME_SIZE * sizeof(int16_t));
    syspool_get_buff((uint8_t **)&(audio_spectrum.data), audio_spectrum_cfg.freq_num * sizeof(int16_t));
    syspool_get_buff(&speech_buf, speech_buf_size);
    speech_heap_init(speech_buf, speech_buf_size);

    audio_spectrum.state = spectrum_fix_init(sample_rate, AUDIO_SPECTRUM_FRAME_SIZE, &audio_spectrum_cfg);
    audio_spectrum.bits = sample_bits;
}

void audio_spectrum_close(void)
{
    spectrum_fix_destroy(audio_spectrum.state);

    size_t total = 0, used = 0, max_used = 0;
    speech_memory_info(&total, &used, &max_used);
    TRACE(3,"SPEECH MALLOC MEM: total - %d, used - %d, max_used - %d.", total, used, max_used);
    ASSERT(used == 0, "[%s] used != 0", __func__);
}

static inline int16_t convertTo16Bit(int16_t x)
{
    return x;
}

static inline int16_t convertTo16Bit(int32_t x)
{
    return (x >> 8);
}


// convert stream to 16bit mono stream
template<typename DataType>
static void convertToMono16Bit(int16_t *out, DataType *in, int frame_size)
{
    for (int i = 0; i < frame_size; i++) {
        out[i] = convertTo16Bit(in[i * 2]) / 2 + convertTo16Bit(in[i * 2 + 1]) / 2;
    }
}

template<typename DataType>
void audio_spectrum_run_impl(const uint8_t *buf, int len)
{
    int frame_size = len / sizeof(DataType);
    DataType *pBuf = (DataType *)buf;

    ASSERT(frame_size % (2 * AUDIO_SPECTRUM_FRAME_SIZE) == 0,
        "[%s] only support N*%d frame size", __FUNCTION__, AUDIO_SPECTRUM_FRAME_SIZE);

    int audio_spectrum_block_cnt = frame_size / 2 / AUDIO_SPECTRUM_FRAME_SIZE;

    for (int i = 0; i < audio_spectrum_block_cnt; i++) {
        //stereo to mono, 24bit to 16bit
        convertToMono16Bit(audio_spectrum.frame, pBuf + i * AUDIO_SPECTRUM_FRAME_SIZE * 2, AUDIO_SPECTRUM_FRAME_SIZE);

        //TRACE(0,"pcm:");
        //DUMP16("0x%x, ", audio_spectrum.frame, 8);

        spectrum_fix_analysis(audio_spectrum.state, audio_spectrum.frame);
        if (i == 0) {
            spectrum_fix_process(audio_spectrum.state, audio_spectrum.data, audio_spectrum_cfg.freq_num);
            //TRACE(1,"spectrum: %d", sizeof(DataType));
            //DUMP16("0x%x, ", audio_spectrum.data, audio_spectrum_cfg.freq_num);
        }
    }
}

void audio_spectrum_run(const uint8_t *buf, int len)
{
    if (audio_spectrum.bits == AUD_BITS_16)
        audio_spectrum_run_impl<int16_t>(buf, len);
    else if (audio_spectrum.bits == AUD_BITS_24)
        audio_spectrum_run_impl<int32_t>(buf, len);
    else
        TRACE(1,"[%s] warning not suitable callback available", __func__);
}

#endif

