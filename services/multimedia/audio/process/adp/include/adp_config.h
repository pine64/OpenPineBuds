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
// Microsoft version of 'inline'

#ifdef WIN32
#define inline __inline
#endif


//#define FIXED_POINT


// In Visual Studio, _M_IX86_FP=1 means /arch:SSE was used, likewise
// _M_IX86_FP=2 means /arch:SSE2 was used.
// Also, enable both _USE_SSE and _USE_SSE2 if we're compiling for x86-64
// #if _M_IX86_FP >= 1 || defined(_M_X64)
// #define _USE_SSE
// #endif
// 
// #if _M_IX86_FP >= 2 || defined(_M_X64)
// #define _USE_SSE2
// #endif

// Visual Studio support alloca(), but it always align variables to 16-bit
// boundary, while SSE need 128-bit alignment. So we disable alloca() when
// SSE is enabled.
#define FLOATING_POINT
#define USE_SMALLFT

/* We don't support visibility on Win32 */
#define EXPORT

#define USE_STATIC_MEMORY

#define ADPFILTER_NUM 400


