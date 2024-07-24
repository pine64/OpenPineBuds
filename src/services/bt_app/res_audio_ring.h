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

const int16_t RES_AUD_RING_SAMPRATE_8000 [] = {
#include "res/ring/SOUND_RING_8000.txt"
};
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__

const int16_t RES_AUD_RING_SAMPRATE_16000 [] = {
#include "res/ring/SOUND_RING_16000.txt"
};
const int16_t RES_AUD_RING_SAMPRATE_44100[] = {
#include "res/ring/SOUND_RING_44100.txt"
};

const int16_t RES_AUD_RING_SAMPRATE_48000 [] = {
#include "res/ring/SOUND_RING_48000.txt"
};
#endif
