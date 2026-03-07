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
#ifndef AUDIO_SPECTRUM_H
#define AUDIO_SPECTRUM_H

void audio_spectrum_open(int sample_rate, enum AUD_BITS_T sample_bits);

void audio_spectrum_close(void);

void audio_spectrum_run(const uint8_t *buf, int len);

#endif
