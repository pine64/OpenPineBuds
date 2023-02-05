#pragma once 
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
bool Curr_Is_Master(void);
uint8_t get_curr_role(void);
bool Curr_Is_Slave(void);
uint8_t get_nv_role(void);

#ifdef __cplusplus
};
#endif

