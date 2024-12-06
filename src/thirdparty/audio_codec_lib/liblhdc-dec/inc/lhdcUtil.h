/*
 * lhdcUtil.h
 *
 *  Created on: 2001/1/3
 *      Author: eric.lee
 */

#ifndef LHDC_UTIL_H
#define LHDC_UTIL_H

#include "bluetooth.h"


// Define for LHDC stream type.
typedef enum {
    LHDC_STRM_TYPE_COMBINE,
    LHDC_STRM_TYPE_SPLIT
}LHDC_STRM_TYPE;

typedef enum {
  VERSION_2 = 200,
  VERSION_3 = 300
}lhdc_ver_t;

typedef enum {
  LHDCV2_BLOCK_SIZE = 512,
  LHDCV3_BLOCK_SIZE = 256,
}lhdc_block_size_t;

typedef struct bes_bt_local_info_t{
    uint8_t bt_addr[BTIF_BD_ADDR_SIZE];
    const char *bt_name;
    uint8_t bt_len;
    uint8_t ble_addr[BTIF_BD_ADDR_SIZE];
    const char *ble_name;
    uint8_t ble_len;
}bes_bt_local_info;


#define A2DP_LHDC_HDR_LATENCY_LOW   0x00
#define A2DP_LHDC_HDR_LATENCY_MID   0x01
#define A2DP_LHDC_HDR_LATENCY_HIGH  0x02
#define A2DP_LHDC_HDR_LATENCY_MASK  (A2DP_LHDC_HDR_LATENCY_MID | A2DP_LHDC_HDR_LATENCY_HIGH)

#define A2DP_LHDC_HDR_FRAME_NO_MASK 0xfc

typedef int (*LHDC_GET_BT_INFO)(bes_bt_local_info * bt_info);

void lhdcInit(uint32_t bitPerSample, uint32_t sampleRate, uint32_t scaleTo16Bits, lhdc_ver_t version);
bool lhdcReadyForInput(void);
uint32_t lhdcPutData(uint8_t * pInpBuf, uint32_t NumBytes);
uint32_t lhdcDecodeProcess(uint8_t * pOutBuf);
void lhdcDestroy();
bool lhdcSetLicenseKeyTable(uint8_t * licTable, LHDC_GET_BT_INFO pFunc);
char * getVersionCode();
#endif /* End of LHDC_UTIL_H */
