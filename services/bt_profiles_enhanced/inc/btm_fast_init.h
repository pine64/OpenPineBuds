#ifndef __BTM_FAST_INIT__
#define __BTM_FAST_INIT__
#include <stdbool.h>
#include "btlib_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

void btm_fast_init(uint8_t* bt_addr, uint8_t* ble_addr);

#ifdef __cplusplus 
}
#endif
#endif//__BTM_FAST_INIT__
