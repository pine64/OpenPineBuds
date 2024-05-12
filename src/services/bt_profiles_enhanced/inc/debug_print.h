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

/*
usage:

0.make sure globle cfg control DBG_DEBUG_PRINT_ENABLE is 1

1.set your  module in your source code file at the top(MUST).
   #undef MOUDLE
   #define MOUDLE BB
   #include "debug_print.h"
   
2.define the module debug level in debug_cfg.h, 
  NOTICE,if no define, then no print
  
   #define BB_LEVEL INFO_LEVEL
   
3.use DEBUG_INFO,DEBUG_WARNING,and DEBUG_ERROR in your module's codes

4.DEBUG_ASSERT always work.

*/
#ifndef __DEBUG_PRINT_H__
#define __DEBUG_PRINT_H__

#include "bt_common.h"
#include "debug_cfg.h"

#ifdef ENABLE_COMPRESS_LOG
#define TRACE_BTPRF_V(num,str, ...) LOG_VERBOSE(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)), str, ##__VA_ARGS_
#define TRACE_BTPRF_I(num,str, ...) LOG_INFO(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)), str, ##__VA_ARGS__)
#define TRACE_BTPRF_W(num,str, ...) LOG_WARN(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)), str, ##__VA_ARGS__)
#define TRACE_BTPRF_E(num,str, ...) LOG_ERROR(LOG_ATTR_ARG_NUM(COUNT_ARG_NUM(unused, ##__VA_ARGS__)), str, ##__VA_ARGS__)__)
#else
#define TRACE_BTPRF_D(num,str, ...) LOG_DEBUG(LOG_MOD(BTPRF), str, ##__VA_ARGS__)
#define TRACE_BTPRF_I(num,str, ...) LOG_INFO(LOG_MOD(BTPRF), str, ##__VA_ARGS__)
#define TRACE_BTPRF_W(num,str, ...) LOG_WARN(LOG_MOD(BTPRF), str, ##__VA_ARGS__)
#define TRACE_BTPRF_E(num,str, ...) LOG_ERROR(LOG_MOD(BTPRF), str, ##__VA_ARGS__)
#endif

// used for global cfg control, to set if debug_print feature enable
#if DBG_DEBUG_PRINT_ENABLE == 1

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define _NAME(a) #a
#define NAME(a) _NAME(a)

#define WHERESTR  "[module %s, line %d]: "
#define WHEREARG  NAME(MOUDLE), __LINE__
#define DEBUGPRINT2(num,...)       TRACE(num,__VA_ARGS__)

#define CO_DEBUG_PRINTF(num,level,...)  do{ \
										DEBUGPRINT2(num,__VA_ARGS__);\
									}while(0);



#if ( CONCAT(MOUDLE, _LEVEL) >= DBG_INFO_LEVEL )
#define DEBUG_INFO(num,...) CO_DEBUG_PRINTF(num,"INF", __VA_ARGS__)
#else
#define DEBUG_INFO(...)
#endif


#if ( CONCAT(MOUDLE, _LEVEL) >= DBG_WARNING_LEVEL )
#define DEBUG_WARNING(num,...)  CO_DEBUG_PRINTF(num,"WAR", __VA_ARGS__)
#else
#define DEBUG_WARNING(...)
#endif


#if ( CONCAT(MOUDLE, _LEVEL) >= DBG_ERROR_LEVEL )
#define DEBUG_ERROR(num,...) CO_DEBUG_PRINTF(num,"ERR", __VA_ARGS__)
#else
#define DEBUG_ERROR(...)
#endif

#if 0
#define DEBUG_ASSERT(expr)  do{\
							 if (expr)\
							 	{ ; }\
							 else \
							 	{CO_DEBUG_PRINTF("ASSERT","%s",#expr)}\
							 }while(0);
#endif

/*only dump at info level*/
#if ( CONCAT(MOUDLE, _LEVEL) >= INFO_LEVEL )
void _debug_print_dump_data(char *mem, int mem_size);
#define DEBUG_DUMP_DATA(memaddr, memlen) do{ \
											DEBUGPRINT2(2,"\r\n"  WHERESTR , WHEREARG);\
											DEBUGPRINT2(0,"\r\n");\
											Plt_DUMP8("%02x ", (char *)memaddr,memlen);\
										   }while(0);

#else
#define DEBUG_DUMP_DATA(...) 
#endif


#define DEBUG_RAW(...) DEBUG_PRINT(__VA_ARGS__)

#define DEBUG_IMM(...) TRACE_IMM(__VA_ARGS__)

#else

#define DEBUG_IMM(...)
#define DEBUG_INFO(...)
#define DEBUG_WARNING(...)
#define DEBUG_ERROR(...)
#define DEBUG_ASSERT(expr)
#define DEBUG_DUMP_DATA(...)  
#define DEBUG_RAW(...) 
#endif

#endif /* __DEBUG_PRINT_H__ */