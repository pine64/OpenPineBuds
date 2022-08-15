#ifndef BESBT_STRING_H
#define BESBT_STRING_H

#include "stddef.h"
#include "stdint.h"
#include "plat_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t  memcpy_s(void *dst,size_t dstMax,const void *src, size_t srcMax);
size_t  memset_s(void *,size_t,int,size_t);

#ifdef __cplusplus
}
#endif


#endif /* BES_STRING_H */

