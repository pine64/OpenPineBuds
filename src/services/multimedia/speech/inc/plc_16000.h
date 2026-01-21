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
/********************************************************
SBC Example PLC ANSI-C Source Code
File: sbcplc.h
*****************************************************************************/
#ifndef SBCPLC_H
#define SBCPLC_H

#define FS 120 /* Frame Size */
#define N 256 /* 16ms - Window Length for pattern matching */
#define M 64 /* 4ms - Template for matching */
#define LHIST (N+FS-1) /* Length of history buffer required */
#define SBCRT 60 /* SBC Reconvergence Time (samples) */
#define OLAL 60 /* OverLap-Add Length (samples) */

//SBCRT + OLAL must be <=FS

/* PLC State Information */
struct PLC_State
{
	short hist[LHIST + OLAL + FS + SBCRT + OLAL];
	short bestlag;
	int nbf;
};
extern unsigned char indices0[57];
/* Prototypes */
void InitPLC(struct PLC_State *plc_state);
void PLC_bad_frame(struct PLC_State *plc_state, short *ZIRbuf, short *out);
void PLC_good_frame(struct PLC_State *plc_state, short *in, short *out);
#endif /* SBCPLC_H */

