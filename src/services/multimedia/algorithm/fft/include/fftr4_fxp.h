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

#ifndef FFTR4_FXP_H
#define FFTR4_FXP_H

#define FFTR4_TWIDDLE_WIDTH 16
#define FFTR4_DATA_WIDTH    16
#define FFTR4_SCALE          6

/* Q1.15 */
#define FFTR4_INPUT_FORMAT_X   1
#define FFTR4_INPUT_FORMAT_Y   15
#define FFTR4_INPUT_FORMAT     (FFTR4_INPUT_FORMAT_X+FFTR4_INPUT_FORMAT_Y)

/* Q1.15 */
#define FFTR4_OUTPUT_FORMAT_X   1
#define FFTR4_OUTPUT_FORMAT_Y   15
#define FFTR4_OUTPUT_FORMAT     (FFTR4_OUTPUT_FORMAT_X+FFTR4_OUTPUT_FORMAT_Y)

typedef struct {
	int re;
	int im;
} FftData_t;

typedef struct {
	int re;
	int im;
} FftTwiddle_t;

typedef enum 
{
    FFT_MODE = 0,
    IFFT_MODE
}FftMode_t;

void make_symmetric_twiddles(FftTwiddle_t w[], int N, int width);
void fftr4(int N, FftData_t x[], FftTwiddle_t w[], int twiddleWidth, int dataWidth, FftMode_t ifft);
void dibit_reverse_array(FftData_t *vector);
//void dibit_reverse_array(FftData_t *vector);
unsigned int dibit_reverse_int(unsigned int x, unsigned int N);

#endif