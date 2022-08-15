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
#ifndef __FIR2IIR_H__
#define __FIR2IIR_H__

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Add fir2iir_create and fir2iir_destroy functions.

// weight: length is 512
void fir2iir_process(double *RealCoeffs,double *ImagCoeffs,double iircoefaa[10][3], double iircoefbb[10][3], int numorder, int denorder, int maxiter, double *weight);

#ifdef __cplusplus
}
#endif

#endif