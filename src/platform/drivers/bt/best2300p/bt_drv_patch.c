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
#include "bt_drv.h"
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "hal_chipid.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "plat_types.h"

extern void btdrv_write_memory(uint8_t wr_type, uint32_t address,
                               const uint8_t *value, uint8_t length);

/// enable m4 patch func
#define BTDRV_PATCH_EN_REG 0xe0002000

// set m4 patch remap adress
#define BTDRV_PATCH_REMAP_REG 0xe0002004

// instruction patch compare src address
#define BTDRV_PATCH_INS_COMP_ADDR_START 0xe0002008

#define BTDRV_PATCH_INS_REMAP_ADDR_START 0xc0000100

// data patch compare src address
#define BTDRV_PATCH_DATA_COMP_ADDR_START 0xe00020e8

#define BTDRV_PATCH_DATA_REMAP_ADDR_START 0xc00001e0

#define BTDRV_PATCH_ACT 0x1
#define BTDRV_PATCH_INACT 0x0

typedef struct {
  uint8_t patch_index;   // patch position
  uint8_t patch_state;   // is patch active
  uint16_t patch_length; // patch length 0:one instrution replace  other:jump to
                         // ram to run more instruction
  uint32_t patch_remap_address; // patch occured address
  uint32_t patch_remap_value;   // patch replaced instuction
  uint32_t patch_start_address; // ram patch address for lenth>0
  uint8_t *patch_data;          // ram patch date for length >0

} BTDRV_PATCH_STRUCT;

#ifdef IBRT
/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1302 T3 (CHIP ID=2)
 *
 *  IBRT function
 *
 * patch  2: 0xC00069a8--->0xC00069b0
 * patch  4: 0xC00069b8--->0xC00069d0
 * patch  5: 0xC00069d4--->0xC00069f0
 * patch  6: 0xC00069f8--->0xC0006a08
 * patch  7: 0xC0006a10--->0xC0006a24
 * patch  9: 0xC0006a2c--->0xC0006a4c
 * patch 10: 0xC0006a54--->0xC0006a64
 * patch 11: 0xC0006a68--->0xC0006a90
 * patch 13: 0xC0006a98--->0xC0006b2c
 * patch 15: 0xC0006b3c--->0xC0006b88
 * patch 16: 0xC0006b90--->0xC0006ba0
 * patch 17: 0xC0006ba4--->0xC0006bc0
 * patch 14: 0xC0006bc8--->0xC0006bdc
 * patch  0:  0xC0006be4--->0xC0006c04
 * patch  1:  0xC0006c0c--->0xC0006c24
 * patch  2:  0xC0006c34--->0xC0006c80
 * patch  3:  0xC0006c88--->0xC0006c94
 * lmp_ext_sniffer_fast_cfm_tab: 0xC0006c9c --->0xC0006ca4

 * patch 18: 0xC0006cbc--->0xC0006d10 DEBUG_PATCH
 * patch 19: 0xC0006d14--->0xC0006dbc
 * patch 20: 0xC0006dc4--->0xC0006df4
 * patch 21: 0xC0006e00--->0xC0006e2c
 * patch 22: 0xC0006e34--->0xC0006e38
 * lmp_ext_sniffer_filter_tab: 0xC0006e40 --->0xC0006e54
 * patch 2a: 0xC0006e5c--->0xC0006e70
 * patch 24: 0xC0006e78--->0xC0006e88
 * patch 25: 0xC0006e94--->0xC0006ea4
 * patch 29: 0xC0006eb0--->0xC0006ec4
 * patch 30: 0xC0006ecc--->0xC0006ef0
 * patch 31: 0xC0006ef8--->0xC0006f08
 * patch 32: 0xC0006f0c--->0xC0006f18
 * patch 33: 0xc0006f20--->0xc0006f50
 * patch 34: 0xc0006f58--->0xc0006f64
 * patch 35: 0xc0006f70--->0xc0006f90
 * patch 49: 0xc0006f98--->0xc0006fac
 * patch 36: 0xc0006fc0--->0xc0007034
 * patch 37: 0xc0007050--->0xc0007328
 * patch 37: 0xc000732c--->0xc0007348
 * patch 44: 0xc0007460--->0xc0007500

 patch_ld_ibrt_sco_req_filter: c0007670--->c0007670
 ****************************************************************************/
//#define BT_CONTROLLER_DEBUG  1

const uint32_t bes2300p_patch0_ibrt_ins_data_t3[] = {
    0x400fe92d, /*0xc0007758L*/
    0xf0004630, /*0xc000775cL*/
    0xe8bdf805, /*0xc0007760L*/
    0xf634400f, /*0xc0007764L*/
    0x0000bb51, /*0xc0007768L*/
    0x4604b510, /*0xc000776cL*/
    0x47984b0c, /*0xc0007770L*/
    0x60184b0c, /*0xc0007774L*/
    0x781b3304, /*0xc0007778L*/
    0x2b03b2db, /*0xc000777cL*/
    0x2c02d005, /*0xc0007780L*/
    0x2c10d009, /*0xc0007784L*/
    0x2004d10a, /*0xc0007788L*/
    0x4b07e006, /*0xc000778cL*/
    0xb2db781b, /*0xc0007790L*/
    0x70134a06, /*0xc0007794L*/
    0x2002e7f3, /*0xc0007798L*/
    0x47984b05, /*0xc000779cL*/
    0xbf00bd10, /*0xc00077a0L*/
    0xa0026ded, /*0xc00077a4L*/
    0xc000653c, /*0xc00077a8L*/
    0xc00064da, /*0xc00077acL*/
    0xc0006541, /*0xc00077b0L*/
    0xa003bcc1, /*0xc00077b4L*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch0 = {
    0,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch0_ibrt_ins_data_t3),
    0x0003bdc0,
    0xbccaf1cb,
    0xc0007758,
    (uint8_t *)
        bes2300p_patch0_ibrt_ins_data_t3}; // ibrt sco auto accept (slave)

const uint32_t bes2300p_patch1_ibrt_ins_data_t3[] = {
    0x681b4b08, 0xf632b933, 0x2800ffdb, 0x2001bf0c, 0xe0002004,
    0xf6352001, 0x2000f84d, 0x60184b01, 0xb809f60a, 0xc0007434, /*6c30*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch1_ibrt_ins_data_t3),
    0x00010c3c,
    0xbfe6f1f5,
    0xc0006c0c,
    (uint8_t *)
        bes2300p_patch1_ibrt_ins_data_t3}; // ibrt sco auto accept (master)

const uint32_t bes2300p_patch2_ibrt_ins_data_t3[] = {
    0x4b04b508, 0x2b01681b, 0x2001d102, 0xfb00f5fa, 0xbf00bd08,
    0xc0007668, 0xfd56f60d, 0xfff0f7ff, 0xbe7af60f, /*6c58*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch2 = {
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch2_ibrt_ins_data_t3),
    0x0001694c,
    0xb980f1f0,
    0xc0006c38,
    (uint8_t *)bes2300p_patch2_ibrt_ins_data_t3}; // tws unsniff update ke state

const uint32_t bes2300p_patch3_ibrt_ins_data_t3[] = {
    0xbf004628, 0xfdc4f60d, 0x21144628, 0xba4af60f, /*6c94*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch3_ibrt_ins_data_t3),
    0x00016128,
    0xbdaef1f0,
    0xc0006c88,
    (uint8_t *)bes2300p_patch3_ibrt_ins_data_t3}; // start lmp to before sending
                                                  // LMP_HostConnectionReq

const uint32_t bes2300p_patch4_ibrt_ins_data_t3[] = {
    0xf633b940, /*69b8*/
    0x2801f86d, 0xf633d104, 0x2800f903,
    0xbf00d102, 0xbd84f62b, 0xbe02f62b, /*69d0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch4 = {
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch4_ibrt_ins_data_t3),
    0x000324d4,
    0xba70f1d4,
    0xc00069b8,
    (uint8_t *)bes2300p_patch4_ibrt_ins_data_t3}; // filter afh map

const uint32_t bes2300p_patch5_ibrt_ins_data_t3[] = {
    0x79ab4688, /*69d4*/
    0x6383ea6f, 0x4578f023, 0xf8d39b09, 0x9906308c,
    0x402b440b, 0xbf004641, 0xb8bdf62d, /*69f0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch5 = {
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch5_ibrt_ins_data_t3),
    0x00033b60,
    0xbf38f1d2,
    0xc00069d4,
    (uint8_t *)bes2300p_patch5_ibrt_ins_data_t3}; // sco param init

const uint32_t bes2300p_patch6_ibrt_ins_data_t3[] = {
    0x4a033304,                                     /*69f8*/
    0xbf008153, 0x12c4f640, 0xbeb9f632, 0xc00062c0, /*6a08*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch6 = {
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch6_ibrt_ins_data_t3),
    0x00039770,
    0xb942f1cd,
    0xc00069f8,
    (uint8_t *)bes2300p_patch6_ibrt_ins_data_t3}; // pscan duration

const uint32_t bes2300p_patch7_ibrt_ins_data_t3[] = {
    0x71ab2300,                                                 /*6a10*/
    0x0000f8d9, 0x380cb110, 0xf8f6f5fa, 0xbf002300, 0xbf3ef608, /*6a24*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch7 = {
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch7_ibrt_ins_data_t3),
    0x0000f8a0,
    0xb8b6f1f7,
    0xc0006a10,
    (uint8_t *)
        bes2300p_patch7_ibrt_ins_data_t3}; // lm_page_scan_end_ind free msg

const uint32_t bes2300p_patch8_ibrt_ins_data_t3[] = {
    0xf8862001,
    0xf8860060,
    0xbf000061,
    0xbba3f611,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch8 = {
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch8_ibrt_ins_data_t3),
    0x00018f1c,
    0xbc62f1ee,
    0xc00077e4,
    (uint8_t *)bes2300p_patch8_ibrt_ins_data_t3}; // ignore 0x5d sm

const uint32_t bes2300p_patch9_ibrt_ins_data_t3[] = {
    0xf62a4628, /*6a2c*/
    0x4620fef7, 0xff76f60f, 0xf8534b04, 0x46203025,
    0x135ef893, 0xfc68f5fa, 0xbd702000, 0xc0005de4, /*6a4c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch9 = {
    9,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch9_ibrt_ins_data_t3),
    0x0002347c,
    0xbad6f1e3,
    0xc0006a2c,
    (uint8_t *)bes2300p_patch9_ibrt_ins_data_t3};

const uint32_t bes2300p_patch10_ibrt_ins_data_t3[] = {
    0x0818f104,
    0xbb9ff634,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch10 = {
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch10_ibrt_ins_data_t3),
    0x0003b184,
    0xbc66f1cb,
    0xc0006a54,
    (uint8_t *)bes2300p_patch10_ibrt_ins_data_t3}; // set afh

const uint32_t bes2300p_patch11_ibrt_ins_data_t3[] = {
    0xf8934b09, /*6a68*/
    0xf0133084, 0xd0050f06, 0xf6374620, 0x4601f805, 0xe0052400,
    0xf8934b03, 0xbf003084, 0xb941f636, 0xb987f636, 0xc0006568, /*6a90*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch11 = {
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch11_ibrt_ins_data_t3),
    0x0003cd08,
    0xbeaef1c9,
    0xc0006a68,
    (uint8_t *)bes2300p_patch11_ibrt_ins_data_t3}; // set ble adv data

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch12 = {
    12,  BTDRV_PATCH_ACT, 0, 0x0003cbf0, 0xbf00e004, 0,
    NULL}; // set ble scan response data

const uint32_t bes2300p_patch13_ibrt_ins_data_t3[] = {
    /*6a98*/ 0x99049803,
    /*6a9c*/ 0x4b12b470,
    /*6aa0*/ 0x010e5c5a,
    /*6aa4*/ 0x19734d11,
    /*6aa8*/ 0x78244c11,
    /*6aac*/ 0xd20342a2,
    /*6ab0*/ 0x32015498,
    /*6ab4*/ 0xe001b2d2,
    /*6ab8*/ 0x22015570,
    /*6abc*/ 0x5442480a,
    /*6ac0*/ 0x461ab14c,
    /*6ac4*/ 0x23001919,
    /*6ac8*/ 0x0b01f912,
    /*6acc*/ 0xbf004403,
    /*6ad0*/ 0xd1f9428a,
    /*6ad4*/ 0x2300e000,
    /*6ad8*/ 0xf0f4fb93,
    /*6adc*/ 0xbc70b240,
    /*6ae0*/ 0x22019904,
    /*6ae4*/ 0xba00f626, // jump a0206ae4 -> a002cee8
    /*6ae8*/ 0xc0006af8, // addr of rssi_idx_for_agc
    /*6aec*/ 0xc0006afc, // addr of rssi_set_for_agc
    /*6af0*/ 0xc0006af4, // addr of N
    /*6af4*/ 0x00000005, // N : 1 ~ 16
    /*6af8*/ 0x00000000, // rssi_idx_for_agc[3]
    /*6afc*/ 0xbabababa, // rssi_set_for_agc[3*16]
    /*6b00*/ 0xbabababa,
    /*6b04*/ 0xbabababa,
    /*6b08*/ 0xbabababa,
    /*6b0c*/ 0xbabababa,
    /*6b10*/ 0xbabababa,
    /*6b14*/ 0xbabababa,
    /*6b18*/ 0xbabababa,
    /*6b1c*/ 0xbabababa,
    /*6b20*/ 0xbabababa,
    /*6b24*/ 0xbabababa,
    /*6b28*/ 0xbabababa,
    /*6b2c*/ 0xbabababa,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch13 = {
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch13_ibrt_ins_data_t3),
    0x0002cee4,
    0xbdd8f1d9, // jump a002cee4 -> a0206a98
    0xc0006a98,
    (uint8_t *)bes2300p_patch13_ibrt_ins_data_t3}; // ld_acl_rx() average rssi

const uint32_t bes2300p_patch14_ibrt_ins_data_t3[] = {
    0x305af893, 0xd1032b00, 0x12f4f240,
    0xbf00e002, 0x32b8f640, 0xbe30f60d, /*6bdc*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch14 = {
    14,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch14_ibrt_ins_data_t3),
    0x0001483c,
    0xb9c4f1f2,
    0xc0006bc8,
    (uint8_t *)bes2300p_patch14_ibrt_ins_data_t3}; // lmp to 5s before
                                                   // connection cmp sent

const uint32_t bes2300p_patch15_ibrt_ins_data_t3[] = {
    0x2028f843, /*6b3c*/
    0xf8434b08, 0x4b052028, 0x4b0e701a, 0x2201701a, 0xf8434b08,
    0xbf002028, 0xba32f624, 0xc0006b60, 0x00000000, 0xc0006b68,
    0x00000000, 0x00000000, 0x00000000, 0xc0006b78, 0x00000001,
    0x00000001, 0x00000001, 0xc0006b88, 0x00000000, /*6b88*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch15 = {
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch15_ibrt_ins_data_t3),
    0x0002afbc,
    0xbdbef1db,
    0xc0006b3c,
    (uint8_t *)bes2300p_patch15_ibrt_ins_data_t3}; // sync clear count

const uint32_t bes2300p_patch16_ibrt_ins_data_t3[] = {
    0x0046f894,                                     /*6b90*/
    0xfe94f625, 0xd8012802, 0xba3ef629, 0xba57f629, /*6ba0*/
};
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch16 = {
    16,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch16_ibrt_ins_data_t3),
    0x00030014,
    0xbdbcf1d6,
    0xc0006b90,
    (uint8_t *)bes2300p_patch16_ibrt_ins_data_t3}; // ld_sco_evt_start_cbk

const uint32_t bes2300p_patch17_ibrt_ins_data_t3[] = {
    0xf6062002, /*6ba4*/
    0x2802f937, 0xf898d108, 0xf62500b2, 0x2802fe85,
    0xbf00d802, 0xbc93f629, 0xbc98f629, /*6bc0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch17 = {
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch17_ibrt_ins_data_t3),
    0x000304d4,
    0xbb66f1d6,
    0xc0006ba4,
    (uint8_t *)
        bes2300p_patch17_ibrt_ins_data_t3}; // ld_calculate_event_timestamp

#ifdef BT_CONTROLLER_DEBUG
const uint32_t bes2300p_patch18_ibrt_ins_data_t3[] = {
    0x68124a0e, 0x2084f8c3, 0x68124a0d, 0x2088f8c3, 0x68124a0c, 0x208cf8c3,
    0x68124a0b, 0x2090f8c3, 0x68124a0a, 0x2094f8c3, 0x68124a09, 0x2098f8c3,
    0x68124a08, 0x209cf8c3, 0xbf004770, 0xe000ed04, 0xe000ed0c, 0xe000ed10,
    0xe000ed14, 0xe000ed24, 0xe000ed3c, 0xe000ed34, /*6d10*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch18 = {
    18,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch18_ibrt_ins_data_t3),
    0x000077fc,
    0xba5ef1ff,
    0xc0006cbc,
    (uint8_t *)bes2300p_patch18_ibrt_ins_data_t3}; // correct crash dump
#else
const uint32_t bes2300p_patch18_ibrt_ins_data_t3[] = {
    0x2271f240, /*6cc0*/
    0x4080f8b0, /*6cc4*/
    0xb9f9f63f,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch18 = {
    18,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch18_ibrt_ins_data_t3),
    0x000460ac,
    0xbe08f1c0,
    0xc0006cc0,
    (uint8_t *)bes2300p_patch18_ibrt_ins_data_t3

}; // BT avoid ble
#endif

const uint32_t bes2300p_patch19_ibrt_ins_data_t3[] = {
    0x4603b508, 0x8210f3ef, 0x0f01f012, 0xb672d112, 0xf5a34811, 0x3b285343,
    0xfba14910, 0xf3c32303, 0xeb032387, 0xeb000343, 0x31940183, 0xfdd0f5fc,
    0xbd08b662, 0xf5a34808, 0x3b285343, 0xfba14907, 0xf3c32303, 0xeb032387,
    0xeb000343, 0x31940183, 0xfdbef5fc, 0xbf00bd08, 0xc0005b4c, 0xff00ff01,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch19 = {
    19,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch19_ibrt_ins_data_t3),
    0x00008b24,
    0xb8f8f1fe,
    0xc0006d18,
    (uint8_t *)bes2300p_patch19_ibrt_ins_data_t3}; // add lock interrupt when
                                                   // free acl buf

const uint32_t bes2300p_patch20_ibrt_ins_data_t3[] = {
    0x460cb510, 0xfafaf5fa, 0xd9042870, 0x007ef1b4, 0x2001bf18,
    0x2000bd10, 0xbf00bd10, 0xf7ff4621, 0x2800ffef, 0x4628d104,
    0xfae8f5fa, 0xbd9af61c, 0xb85ff61d, /*6df4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch20 = {
    20,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch20_ibrt_ins_data_t3),
    0x00023924,
    0xba5cf1e3,
    0xc0006dc4,
    (uint8_t *)bes2300p_patch20_ibrt_ins_data_t3}; // protect ext3 lmp tx cfm

const uint32_t bes2300p_patch21_ibrt_ins_data_t3[] = {
    0x2040f897, /*6e00*/
    0xd1112a01, 0x0109f106, 0xf5fc4608, 0x2813fd4b, 0x221ed802, 0xbc54f614,
    0x2a017a32, 0xf614d801, 0x221ebbf6, 0xbc4cf614, 0xbc47f614, /*6e2c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch21 = {
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch21_ibrt_ins_data_t3),
    0x0001b604,
    0xbbfcf1eb,
    0xc0006e00,
    (uint8_t *)
        bes2300p_patch21_ibrt_ins_data_t3}; // reject set afh lmp when
                                            // channel number less than 20

const uint32_t bes2300p_patch22_ibrt_ins_data_t3[] = {
    0x2202729a, /*6e34*/
    0xbd38719a, /*6e38*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch22 = {
    22,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch22_ibrt_ins_data_t3),
    0x00011c90,
    0xb8d0f1f5,
    0xc0006e34,
    (uint8_t *)bes2300p_patch22_ibrt_ins_data_t3}; // update sco sync type

const uint32_t bes2300p_patch2a_ibrt_ins_data_t3[] = {
    /*6e5c*/ 0xb10b4b04,
    /*6e60*/ 0xbb84f615, // jump a001c56c
    /*6e64*/ 0x783b2300,
    /*6e68*/ 0x0301f003,
    /*6e6c*/ 0xbb5ef615, // jump a001c52c
    /*6e70*/ 0x00000000, // reject sniff
};                       // LMP_MSG_HANDLER(sniff_req) LC_CONNECTED

const uint32_t bes2300p_patch23_ibrt_ins_data_t3[] = {
    0xf9c6f632,                         /*7744*/
    0x02194603, 0x417ff401, 0xbeeff632, /*7750*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch23 = {
    23,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch23_ibrt_ins_data_t3),
    0x0003a52c,
    0xb90af1cd,
    0xc0007744,
    (uint8_t *)bes2300p_patch23_ibrt_ins_data_t3}; // fix sco success index

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch24 = {
    24, BTDRV_PATCH_ACT, 0, 0x00002490, 0xbf00e012, 0, NULL,
}; // stop ibrt directly

const uint32_t bes2300p_patch25_ibrt_ins_data_t3[] = {
    0x2a020a02, 0xbf00d804, 0x427ff400, 0xbf44f61c, 0xb807f61d,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch25 = {
    25,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch25_ibrt_ins_data_t3),
    0x00023d28,
    0xb8b4f1e3,
    0xc0006e94,
    (uint8_t *)bes2300p_patch25_ibrt_ins_data_t3}; // enc update2 idx error

#if 1
const uint32_t bes2300p_patch26_ibrt_ins_data_t3[] = {
    0x68134a05, 0x2380f043, 0xf5a26013, 0x68137206,
    0x0301f043, 0xbd386013, 0xd0220400,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch26 = {
    26,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch26_ibrt_ins_data_t3),
    0x00048200,
    0xba8ef1bf,
    0xc0007720,
    (uint8_t *)bes2300p_patch26_ibrt_ins_data_t3}; // enable hw spiin BLE
                                                   // lld_evt_rx_isr

#elif 0
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch26 = {
    26, BTDRV_PATCH_ACT, 0, 0x00013f44, 0xbf00bd38, 0, NULL}; // assert
#else
const uint32_t bes2300p_patch26_ibrt_ins_data_t3[] = {
    0xf6324607, 0xb120fa87, 0x63a6f640, 0xf63e827b,
    0x4638bd94, 0x2080f8d0, 0xbd86f63e,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch26 = {
    26,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch26_ibrt_ins_data_t3),
    0x000461dc,
    0xba6cf1c1,
    0xc00076b8,
    (uint8_t *)bes2300p_patch26_ibrt_ins_data_t3}; // ble con slave duration in
                                                   // snoop mode
#endif

#if defined(SYNC_BT_CTLR_PROFILE)
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch27 = {
    27,  BTDRV_PATCH_ACT, 0, 0x000187b8, 0x4628b123, 0,
    NULL}; // send profile via ble
#else
const uint32_t bes2300p_patch27_ibrt_ins_data_t3[] = {
    0xfe88f632, 0xffe8f7ff, 0xb9a2f625, /*6c68*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch27 = {
    27,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch27_ibrt_ins_data_t3),
    0x0002bfac,
    0xbe58f1da,
    0xc0006c60,
    (uint8_t *)bes2300p_patch27_ibrt_ins_data_t3}; // disconnect update ke state
#endif

#if 1
// for ble master update con param with large winsize
const uint32_t bes2300p_patch28_ibrt_ins_data_t3[] = {
    0x4630bf00, 0x109ef894, 0xf958f63f, 0x309cf894,
    0x2271f240, 0x0003fb02, 0xbcf1f63f,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch28 = {
    28,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch28_ibrt_ins_data_t3),
    0x000471e4,
    0xbb08f1c0,
    0xc00077f8,
    (uint8_t *)bes2300p_patch28_ibrt_ins_data_t3};
#else
// only for search UI
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch28 = {
    28,  BTDRV_PATCH_ACT, 0, 0x0000e47a, 0xe0027b20, 0,
    NULL}; // lm_inq_res_ind_handler()  rssi
#endif

const uint32_t bes2300p_patch29_ibrt_ins_data_t3[] = {
    0xf8934b04, 0x2b0231c2, 0x230fbf0c,
    0x75ab2314, 0xbac2f629, 0xc0005ea8, /*6ec4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch29 = {
    29,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch29_ibrt_ins_data_t3),
    0x00030444,
    0xbd34f1d6,
    0xc0006eb0,
    (uint8_t *)bes2300p_patch29_ibrt_ins_data_t3}; /// fix acl interv priority

const uint32_t bes2300p_patch30_ibrt_ins_data_t3[] = {
    0xd80a2802, 0x4b05b508, 0x3020f853, 0xf893b123, 0x220510c4,
    0xfe86f61d, 0x4770bd08, 0xc00009d4, 0xffeef7ff, 0xbda8f634, /*6ef0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch30 = {
    30,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch30_ibrt_ins_data_t3),
    0x0003ba40,
    0xba54f1cb,
    0xc0006ecc,
    (uint8_t *)bes2300p_patch30_ibrt_ins_data_t3}; /// send max slot req when
                                                   /// start ibrt

const uint32_t bes2300p_patch31_ibrt_ins_data_t3[] = {
    0xf632b920, 0x2800fdc1, 0xbf00d002, 0xbb3af629, 0xbba2f629, /*6f08*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch31 = {
    31,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch31_ibrt_ins_data_t3),
    0x00030578,
    0xbcbef1d6,
    0xc0006ef8,
    (uint8_t *)bes2300p_patch31_ibrt_ins_data_t3}; // modify start ibrt timing

#if 0 // def BT_CONTROLLER_DEBUG
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch32=
{
    32,
    BTDRV_PATCH_ACT,
    0,
    0x00000348,
    0xbf00e005,
    0,
    NULL
};///lmp trace
#else

const uint32_t bes2300p_patch32_ibrt_ins_data_t3[] = {
    0xfe4af5f9, 0xbf002300, 0x30c8f884, 0xbaf8f624, /*6f18*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch32 = {
    32,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch32_ibrt_ins_data_t3),
    0x0002b508,
    0xbd00f1db,
    0xc0006f0c,
    (uint8_t *)bes2300p_patch32_ibrt_ins_data_t3}; /// role switch fail
#endif

const uint32_t bes2300p_patch33_ibrt_ins_data_t3[] = {
    0x2300b430, 0x4d072103, 0x4290b2da, 0xf855d004, 0x2c004023,
    0x4611bf18, 0x2b033301, 0x4608d1f4, 0x4770bc30, 0xc00009d4, /*6f50*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch33 = {
    33,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch33_ibrt_ins_data_t3),
    0x0002c8c0,
    0xbb2ef1da,
    0xc0006f20,
    (uint8_t *)bes2300p_patch33_ibrt_ins_data_t3}; // find_other_acl_link

const uint32_t bes2300p_patch34_ibrt_ins_data_t3[] = {
    0xfe38f632, 0xd0052800, 0xbf002002,
    0xfcb0f61c, 0xbb2ff634, 0xbb25f634, /*6f6c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch34 = {
    34,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch34_ibrt_ins_data_t3),
    0x0003b5b4,
    0xbcd0f1cb,
    0xc0006f58,
    (uint8_t *)bes2300p_patch34_ibrt_ins_data_t3}; // do not trans afh after
                                                   // IBRT switch

const uint32_t bes2300p_patch35_ibrt_ins_data_t3[] = {
    0xfd92f632, /*6f70*/
    0xd1092801, 0xfe28f632, 0xd0052800, 0xfd9cf632,
    0xd10142a0, 0xbc77f611, 0x68294620, 0xbc70f611, /*6f90*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch35 = {
    35,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch35_ibrt_ins_data_t3),
    0x00018870,
    0xbb7ef1ee,
    0xc0006f70,
    (uint8_t *)bes2300p_patch35_ibrt_ins_data_t3}; // acl data tx cfm

const uint32_t bes2300p_patch36_ibrt_ins_data_t3[] = {
    0x460db538, 0x78034614, 0x7842085b, 0x2b7f78c1, 0x2a01d10a,
    0x290dd11e, 0xf632d11c, 0x4601fd45, 0xf6342000, 0xe015ff1f,
    0xd1132b03, 0xd1112a18, 0xd80f2d02, 0xf6104628, 0xb158fcb3,
    0x3047f890, 0xd1072b02, 0xf62a4628, 0x0228fc0b, 0x0001f040,
    0xfc88f60f, 0x000cf1a4, 0xfdf8f5f9, 0xbf00bd38, 0xd1072800,
    0x4620bf00, 0x46524639, 0xffc8f7ff, 0xbbd8f61a, 0xbb62f61a, /*7034*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch36 = {
    36,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch36_ibrt_ins_data_t3),
    0x000216f8,
    0xbc92f1e5,
    0xc0006fc0,
    (uint8_t *)bes2300p_patch36_ibrt_ins_data_t3}; // lmp filter

const uint32_t bes2300p_patch37_ibrt_ins_data_t3[] = {
    0x47f0e92d, /*7050*/
    0x4b97b082, 0x236e5c1e, 0xf300fb03, 0x5a9b4a95, 0x4f00f413, 0x8120f040,
    0x46044689, 0x1800ea4f, 0xeb084f91, 0x4b910507, 0xa020f853, 0xf8524a90,
    0x33013020, 0x3020f842, 0xf860f635, 0x42a04601, 0x8088f040, 0xf8534b8a,
    0x4b8a2024, 0x429a681b, 0xf8dad817, 0xebc22084, 0xf0230309, 0xf1b34378,
    0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a82425b, 0x0024f852, 0x68124a81,
    0xf200fb02, 0xf2404293, 0x4b7f80eb, 0x3000f993, 0x4b7eb9c3, 0xc000f893,
    0x0f00f1bc, 0x462ad00a, 0x0e0ceb05, 0xf9122300, 0x44030b01, 0xd1fa4572,
    0xe0014662, 0x46132200, 0xfcfcfb93, 0xf8804872, 0xe001c000, 0x46132200,
    0xf890486f, 0x4870e000, 0x9000f890, 0xf890486f, 0xfa4fc000, 0xebc9f08e,
    0xfa4f0000, 0x4550fa8c, 0xebc9dd05, 0x48660e0e, 0xe000f880, 0x4864e002,
    0xc000f880, 0x0e00f04f, 0xf840485d, 0xf8dfe024, 0xf85ee178, 0x30010024,
    0x0024f84e, 0x7800485d, 0xd2054286, 0x783f4f5a, 0x360155af, 0xe004b2f6,
    0x78364e57, 0x6007f808, 0x4f4d2601, 0x4282553e, 0x442ada06, 0xf9124405,
    0x44234b01, 0xd1fa42aa, 0xfb934a53, 0x9200f0f0, 0xb2406a14, 0x23002201,
    0xe08147a0, 0xf8534b46, 0x4b4e2024, 0x429a681b, 0xf8dad816, 0xebc22084,
    0xf0230309, 0xf1b34378, 0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a3e425b,
    0x1024f852, 0x68124a44, 0xf201fb02, 0xd9634293, 0xf9934b42, 0xb9c33000,
    0xf8934b3a, 0xf1bee000, 0xd00a0f00, 0xeb05462a, 0x2300000e, 0x1b01f912,
    0x4282440b, 0x4672d1fa, 0x2200e001, 0xfb934613, 0x4936fefe, 0xe000f881,
    0x2200e001, 0x49334613, 0x49337808, 0xc000f891, 0xf8914932, 0xb241e000,
    0x0101ebcc, 0xf98efa4f, 0xdd044549, 0x0000ebcc, 0x7008492a, 0x4929e002,
    0xe000f881, 0x491c2000, 0x0024f841, 0xf850481c, 0x31011024, 0x1024f840,
    0x7808491c, 0xd2054286, 0x78094920, 0x360155a9, 0xe004b2f6, 0x7809491d,
    0x1007f808, 0x490c2601, 0x4282550e, 0x442ada06, 0xf9124405, 0x440b1b01,
    0xd1fa42aa, 0xfb934a12, 0x9200f0f0, 0xb2406a15, 0x22014621, 0x47a82300,
    0xe8bdb002, 0xbf0087f0,
    0xc0006af8, // rssi_idx_for_agc[link_id]  shenxin
    0xd02111f8, // EM_BT_RD(EM_BT_RXBIT_ADDR
    0xc0006afc, // rssi_set_for_agc   shenxin
    0xc00009d4, // ld_acl_env[link_id]
    0xc0006b68, // link_no_sync_count[link_id]
    0xc00072fc, // link_agc_thd_tws
    0xc0006b78, // link_no_sync_count_time[link_id]
    0xc0007300, // link_agc_thd_tws_time
    0xc0006b60, // rssi_store_tws
    0xc0006af4, // n   shenxin
    0xc0007304, // rssi_tws_step
    0xc0007308, // rssi_min_value_tws
    0xc0004268, // rwip_rf.rxgain_cntl
    0xc000730c, // link_agc_thd_mobile
    0xc0007310, // link_agc_thd_mobile_time
    0xc0006b88, // rssi_store_mobile
    0xc0007314, // rssi_mobile_step
    0xc0007318, // rssi_min_value_mobile
    0xffffffff, // link_agc_thd_tws
    0x00000030, // link_agc_thd_tws_time
    0x00000014, // rssi_tws_step
    0x0000009c, // rssi_min_value_tws
    0xffffffff, // link_agc_thd_mobile
    0x00000030, // link_agc_thd_mobile_time
    0x00000014, // rssi_mobile_step
    0x000000a1, // rssi_min_value_mobile
    0x99029804, 0xfe96f7ff, 0xfc48f632, 0xb898f626, /*7328*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch37 = {
    37,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch37_ibrt_ins_data_t3),
    0x0002d458,
    0xbf60f1d9,
    0xc0007050,
    (uint8_t *)bes2300p_patch37_ibrt_ins_data_t3}; // swagc no sync

#if 0
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch38 =
{
    38,
    BTDRV_PATCH_ACT,
    0,
    0x000340b0,
    0xbf00e003,
    0,
    NULL,
};//ld_sco_stop
#elif 1
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch38 = {
    38, BTDRV_PATCH_ACT, 0, 0x00039ecc, 0xbf00e001, 0, NULL,
}; // remove lc_start_lmp_to
#else
const uint32_t bes2300p_patch38_ibrt_ins_data_t3[] = {
    0x30d8f8c4, 0xfed6f61f, 0xd2014283, 0xbca7f62a, 0xbcbaf62a, /*7048*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch38 = {
    38,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch38_ibrt_ins_data_t3),
    0x000319a4,
    0xbb48f1d5,
    0xc0007038,
    (uint8_t *)
        bes2300p_patch38_ibrt_ins_data_t3}; // ble first connection anchor
#endif

const uint32_t bes2300p_patch39_ibrt_ins_data_t3[] = {
    0x681b4b05, /*732c*/
    0xd1032b00, 0x781a9b12, 0xbeb8f60b, 0xbec3f60b, 0xbf00bf00,

    0xc0007348, // sniffer_sco_auto_accept
    0x00000000, /*7348*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch39 = {
    39,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch39_ibrt_ins_data_t3),
    0x000130a8,
    0xb940f1f4,
    0xc000732c,
    (uint8_t *)bes2300p_patch39_ibrt_ins_data_t3}; // lm_get_sync_param_hl_rsp

const uint32_t bes2300p_patch40_ibrt_ins_data_t3[] = {
    0xfba2f632, 0xd11d2801, 0xfc38f632, 0x4b33b1d0, 0x2b00681b, 0xf632d013,
    0x4606fba9, 0xd10e42b8, 0xfb98f632, 0xd15f2800, 0xfb76f632, 0x7a224605,
    0x42937a63, 0x4620d103,
    0xf99af000, //
    0x2000b9d0, 0xbdeff614, 0xfb80f632, 0xd10d2802, 0xfc16f632, 0x4b22b150,
    0x2b00681b, 0x4620d006,
    0xf988f000, //
    0x4b1fb910, 0x601a2201, 0xf0037823, 0xbf000301, 0xbda9f614, 0xf8852602,
    0x79a36054, 0x3055f885, 0xf844f5fc, 0x7c4279e3, 0xd802429a, 0xf83ef5fc,
    0xf8857c43, 0x79633056, 0x3057f885, 0xf3c37923, 0xf8850340, 0x7a233058,
    0x3059f885, 0xf8a58963, 0x7ba3305a, 0x305cf885, 0xf88578e3, 0x78a3305d,
    0x305ff885, 0xfb28f632, 0x46384603, 0xf1052110, 0xf6340252, 0x2000fc65,
    0xbda3f614,
    0xc0007348, // sniffer_sco_auto_accept
    0xc0007434, // sco_param_filter_status
    0x00000000, 0xfc3cf634, 0xb2c33801, 0xd8022b02, 0xf6342004, 0x2000fc3b,
    0xbd91f614, 0x3c260cff, 0x5a3712ff, 0xffffffff, 0xffffffff,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch40 = {
    40,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch40_ibrt_ins_data_t3),
    0x0001bf10,
    0xba1ef1eb,
    0xc0007350,
    (uint8_t *)
        bes2300p_patch40_ibrt_ins_data_t3}; // LMP_MSG_HANDLER(esco_link_req)

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch41 = {
    41, BTDRV_PATCH_ACT, 0, 0x0003b818, 0xfb1be001, 0, NULL,
}; // config fastack none mode

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch42 = {
    42,  BTDRV_PATCH_ACT, 0, 0x0002ceb4, 0xe00b0f0f, 0,
    NULL}; // ld_acl_rx() average rssi

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch43 = {
    43, BTDRV_PATCH_ACT, 0, 0x0002a51c, 0x3300f443, 0, NULL,
}; // bt_e_scoltcntl_retxnb_setf 2

const uint32_t bes2300p_patch44_ibrt_ins_data_t3[] = {
    0xbf00686b, 0xd00f42b3, 0x4a212100, 0x6863e004, 0x42b34629, 0x4625d00a,
    0xd9264293, 0xd8242e01, 0x462c461e, 0xe001460d, 0x2500462c, 0x8008f8d4,
    0xa00cf894, 0x900df894, 0x46384f16, 0x46224629, 0xfb12f5fc, 0x0014f1a7,
    0xf5fc4621, 0xf1b8fa1d, 0xd0050f00, 0x46514630, 0x2300464a, 0xe00847c0,
    0x21ff480d, 0xfffaf5ff, 0x682ce003, 0xd1ce2c00, 0xb924e000, 0xf44f4808,
    0xf5ff7185, 0xe8bdffef, 0xbf0087f0, 0xc0000954, 0xd02200bc, 0xa00508ec,
    0x07fffffc, 0xc0000968, 0xa00508b4,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch44 = {
    44,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch44_ibrt_ins_data_t3),
    0x0002706c,
    0xb9f8f1e0,
    0xc0007460,
    (uint8_t *)bes2300p_patch44_ibrt_ins_data_t3};

const uint32_t bes2300p_patch45_ibrt_ins_data_t3[] = {
    0x4b05b508, 0x2b01681b, 0x2200d104, 0x601a4b02,
    0xfe6cf633, 0xbf00bd08, 0xc00076a4, 0x00000000,
    0xffeef7ff, 0xd1012b04, 0xbff0f5ff, 0xbff5f5ff, /*76b4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch45 = {
    45,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch45_ibrt_ins_data_t3),
    0x00007690,
    0xb80af200,
    0xc0007688,
    (uint8_t *)bes2300p_patch45_ibrt_ins_data_t3}; // dbg state

const uint32_t bes2300p_patch46_ibrt_ins_data_t3[] = {
    0x4b06b508, 0x02007a98, 0x0001f040, 0xfb70f5fa, 0xbf183801, 0x30fff04f,
    0xbf00bd08, 0xc00064d0, 0xffeef7ff, 0x781bb918, 0xf6342b02, 0xbd08bc89,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch46 = {
    46,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch46_ibrt_ins_data_t3),
    0x0003b60c,
    0xbb70f1cb,
    0xc0006cd0,
    (uint8_t *)bes2300p_patch46_ibrt_ins_data_t3};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch47 = {
    47,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch2a_ibrt_ins_data_t3),
    0x0001c528,
    0xbc98f1ea, // a001c528 -> a0206e5c
    0xc0006e5c,
    (uint8_t *)bes2300p_patch2a_ibrt_ins_data_t3}; // LMP_MSG_HANDLER(sniff_req)
                                                   // LC_CONNECTED

const uint32_t bes2300p_patch48_ibrt_ins_data_t3[] = {
    0x681b4b1d, 0xd1352b00, 0xb083b530, 0x8310f3ef, 0x0f01f013, 0xb672d117,
    0x25004c18, 0x95019500, 0xf1044620, 0x2208013c, 0xf5fc2307, 0x9500f9ed,
    0xf5049501, 0xf5047080, 0x2208718a, 0xf5fc2307, 0xb662f9e3, 0x4c0de014,
    0x95002500, 0x46209501, 0x013cf104, 0x23072208, 0xf9d6f5fc, 0x95019500,
    0x7080f504, 0x718af504, 0x23072208, 0xf9ccf5fc, 0xbd30b003, 0xbf004770,
    0xc0005df0, 0xc0005b24, 0xffbef7ff, 0xe8bdb004, 0xbf0081f0, /*75a4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch48 = {
    48,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch48_ibrt_ins_data_t3),
    0x000013b0,
    0xb8f2f206,
    0xc0007518,
    (uint8_t *)bes2300p_patch48_ibrt_ins_data_t3};
#if 1
const uint32_t bes2300p_patch49_ibrt_ins_data_t3[] = {
    0x78004804, 0xbf00b120, 0xfd8cf632, 0xba1af609, 0xba1af609, 0xc0006221,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch49 = {
    49,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch49_ibrt_ins_data_t3),
    0x000103d8,
    0xbddef1f6,
    0xc0006f98,
    (uint8_t *)bes2300p_patch49_ibrt_ins_data_t3}; // lc_sco_ind
#else
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch49 = {
    49, BTDRV_PATCH_ACT, 0, 0x000103d8, 0xbf00e002, 0, NULL,
}; // lc_sco_ind
#endif

const uint32_t bes2300p_patch50_ibrt_ins_data_t3[] = {
    0xf632d806, 0x2801fb0b, 0x0209d002, 0xbbeef632, 0xbf00bd08, /*75d0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch50 = {
    50,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch50_ibrt_ins_data_t3),
    0x00039d98,
    0xbc0af1cd,
    0xc00075b0,
    (uint8_t *)bes2300p_patch50_ibrt_ins_data_t3}; // do not trans stop SCO

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch51 = {
    51, BTDRV_PATCH_ACT, 0, 0x0002ea00, 0x8ff8e8bd, 0, NULL,
}; // remove lmp flush

const uint32_t bes2300p_patch52_ibrt_ins_data_t3[] = {
    0x4605b5f8, 0xf8544c19, 0xf6327020, 0x4606fb05, 0x4020f854,
    0xfaecf632, 0xf632b320, 0x42a8fa61, 0xf897d120, 0x2b023047,
    0x2e02d11c, 0xb1ccd81a, 0x3047f894, 0xd1152b02, 0x3040f894,
    0x0234b973, 0x0401f044, 0x4620b2a4, 0xfeccf5f9, 0xd0052836,
    0x60a2f44f, 0x46224621, 0xfadaf5f9, 0x4b042001, 0xbdf86018,
    0x4b022000, 0xbdf86018, 0xc0005de4, 0xc0007668, 0x4638bf00,
    0xffc2f7ff, 0x4628d104, 0xfeb0f5f9, 0xfc38f614, 0xbc84f614,
    0x00000000, /*7668*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch52 = {
    52,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch52_ibrt_ins_data_t3),
    0x0001bed0,
    0xbbbef1eb,
    0xc00075dc,
    (uint8_t *)bes2300p_patch52_ibrt_ins_data_t3}; // delay mobile unsniff lmp

const uint32_t bes2300p_patch53_ibrt_ins_data_t3[] = {
    0x681b4b03, /*76fc*/
    0xd1012b01, /*7700*/
    0xfb06f632, /*7704*/
    0x81f0e8bd, /*7708*/
    0xc0007434, /*770c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch53 = {
    53,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch53_ibrt_ins_data_t3),
    0x0003a088,
    0xbb38f1cd,
    0xc00076fc,
    (uint8_t *)
        bes2300p_patch53_ibrt_ins_data_t3}; // don not trans start ibrt sco

const uint32_t bes2300p_patch54_ibrt_ins_data_t3[] = {
    0xbf000a28, 0xffc6f626, 0x0a210a28, 0xb8a4f611, /*767c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch54 = {
    54,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch54_ibrt_ins_data_t3),
    0x000187c4,
    0xbf54f1ee,
    0xc0007670,
    (uint8_t *)bes2300p_patch54_ibrt_ins_data_t3}; // protect tws tx type

#if 0
const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch55 =
{
    55,
    BTDRV_PATCH_ACT,
    0,
    0x00030758,
    0xbf00e03a,
    0,
    NULL,
};//decrease the connection time of TWS link in SCO
#else
const uint32_t bes2300p_patch55_ibrt_ins_data_t3[] = {
    0x68134a06, 0x2380f023, 0xf5a26013, 0x68137206,
    0x0301f023, 0xbf006013, 0xbdaff63e, 0xd0220400, /*77dc*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ibrt_ins_patch55 = {
    55,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch55_ibrt_ins_data_t3),
    0x00046330,
    0xba46f1c1,
    0xc00077c0,
    (uint8_t *)bes2300p_patch55_ibrt_ins_data_t3}; // disable hw spi in BLE push
#endif

/////2300p t3 ibrt patch (chip id =2)
static const uint32_t best2300p_t3_ibrt_ins_patch_config[] = {
    56,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch0,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch1,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch2,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch3,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch4,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch5,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch6,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch7,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch8,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch9,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch10,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch11,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch12,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch13,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch14,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch15,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch16,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch17,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch18,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch19,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch20,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch21,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch22,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch23,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch24,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch25,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch26,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch27,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch28,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch29,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch30,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch31,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch32,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch33,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch34,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch35,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch36,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch37,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch38,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch39,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch40,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch41,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch42,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch43,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch44,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch45,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch46,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch47,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch48,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch49,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch50,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch51,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch52,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch53,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch54,
    (uint32_t)&bes2300p_t3_ibrt_ins_patch55,
};
#else

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch0 = {
    0,   BTDRV_PATCH_ACT, 0, 0x000324d4, 0xbf00bf00, 0,
    NULL}; // afh filter disable

const uint32_t bes2300p_patch1_ins_data_t3[] = {
    0xf8934b09, 0xf0133084, 0xd0050f06, 0xf6374620, 0x4601f839, 0xe0052400,
    0xf8934b03, 0xbf003084, 0xb975f636, 0xb9bbf636, 0xc0006568,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch1_ins_data_t3),
    0x0003cd08,
    0xbe7af1c9,
    0xc0006a00,
    (uint8_t *)bes2300p_patch1_ins_data_t3}; // set ble adv data

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch2 = {
    2,   BTDRV_PATCH_ACT, 0, 0x0003cbf0, 0xbf00e004, 0,
    NULL}; // set ble scan response data

const uint32_t bes2300p_patch3_ins_data_t3[] = {
    /*6a98*/ 0x99049803,
    /*6a9c*/ 0x4b12b470,
    /*6aa0*/ 0x010e5c5a,
    /*6aa4*/ 0x19734d11,
    /*6aa8*/ 0x78244c11,
    /*6aac*/ 0xd20342a2,
    /*6ab0*/ 0x32015498,
    /*6ab4*/ 0xe001b2d2,
    /*6ab8*/ 0x22015570,
    /*6abc*/ 0x5442480a,
    /*6ac0*/ 0x461ab14c,
    /*6ac4*/ 0x23001919,
    /*6ac8*/ 0x0b01f912,
    /*6acc*/ 0xbf004403,
    /*6ad0*/ 0xd1f9428a,
    /*6ad4*/ 0x2300e000,
    /*6ad8*/ 0xf0f4fb93,
    /*6adc*/ 0xbc70b240,
    /*6ae0*/ 0x22019904,
    /*6ae4*/ 0xba00f626, // jump a0206ae4 -> a002cee8
    /*6ae8*/ 0xc0006af8, // addr of rssi_idx_for_agc
    /*6aec*/ 0xc0006afc, // addr of rssi_set_for_agc
    /*6af0*/ 0xc0006af4, // addr of N
    /*6af4*/ 0x00000005, // N : 1 ~ 16
    /*6af8*/ 0x00000000, // rssi_idx_for_agc[3]
    /*6afc*/ 0xbabababa, // rssi_set_for_agc[3*16]
    /*6b00*/ 0xbabababa,
    /*6b04*/ 0xbabababa,
    /*6b08*/ 0xbabababa,
    /*6b0c*/ 0xbabababa,
    /*6b10*/ 0xbabababa,
    /*6b14*/ 0xbabababa,
    /*6b18*/ 0xbabababa,
    /*6b1c*/ 0xbabababa,
    /*6b20*/ 0xbabababa,
    /*6b24*/ 0xbabababa,
    /*6b28*/ 0xbabababa,
    /*6b2c*/ 0xbabababa,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch3_ins_data_t3),
    0x0002cee4,
    0xbdd8f1d9, // jump a002cee4 -> a0206a98
    0xc0006a98,
    (uint8_t *)bes2300p_patch3_ins_data_t3}; // ld_acl_rx() average rssi

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch4 = {
    4,   BTDRV_PATCH_ACT, 0, 0x0002ceb4, 0xe00b0f0f, 0,
    NULL}; // ld_acl_rx() average rssi

const uint32_t bes2300p_patch5_ins_data_t3[] = {
    0x2028f843, /*6b3c*/
    0xf8434b08, 0x4b052028, 0x4b0e701a, 0x2201701a, 0xf8434b08,
    0xbf002028, 0xba32f624, 0xc0006b60, 0x00000000, 0xc0006b68,
    0x00000000, 0x00000000, 0x00000000, 0xc0006b78, 0x00000001,
    0x00000001, 0x00000001, 0xc0006b88, 0x00000000, /*6b88*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch5 = {
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch5_ins_data_t3),
    0x0002afbc,
    0xbdbef1db,
    0xc0006b3c,
    (uint8_t *)bes2300p_patch5_ins_data_t3}; // sync clear count

const uint32_t bes2300p_patch6_ins_data_t3[] = {
    0x47f0e92d, /*7050*/
    0x4b97b082, 0x236e5c1e, 0xf300fb03, 0x5a9b4a95, 0x4f00f413, 0x8120f040,
    0x46044689, 0x1800ea4f, 0xeb084f91, 0x4b910507, 0xa020f853, 0xf8524a90,
    0x33013020, 0x3020f842, 0xf860f635, 0x42a04601, 0x8088f040, 0xf8534b8a,
    0x4b8a2024, 0x429a681b, 0xf8dad817, 0xebc22084, 0xf0230309, 0xf1b34378,
    0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a82425b, 0x0024f852, 0x68124a81,
    0xf200fb02, 0xf2404293, 0x4b7f80eb, 0x3000f993, 0x4b7eb9c3, 0xc000f893,
    0x0f00f1bc, 0x462ad00a, 0x0e0ceb05, 0xf9122300, 0x44030b01, 0xd1fa4572,
    0xe0014662, 0x46132200, 0xfcfcfb93, 0xf8804872, 0xe001c000, 0x46132200,
    0xf890486f, 0x4870e000, 0x9000f890, 0xf890486f, 0xfa4fc000, 0xebc9f08e,
    0xfa4f0000, 0x4550fa8c, 0xebc9dd05, 0x48660e0e, 0xe000f880, 0x4864e002,
    0xc000f880, 0x0e00f04f, 0xf840485d, 0xf8dfe024, 0xf85ee178, 0x30010024,
    0x0024f84e, 0x7800485d, 0xd2054286, 0x783f4f5a, 0x360155af, 0xe004b2f6,
    0x78364e57, 0x6007f808, 0x4f4d2601, 0x4282553e, 0x442ada06, 0xf9124405,
    0x44234b01, 0xd1fa42aa, 0xfb934a53, 0x9200f0f0, 0xb2406a14, 0x23002201,
    0xe08147a0, 0xf8534b46, 0x4b4e2024, 0x429a681b, 0xf8dad816, 0xebc22084,
    0xf0230309, 0xf1b34378, 0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a3e425b,
    0x1024f852, 0x68124a44, 0xf201fb02, 0xd9634293, 0xf9934b42, 0xb9c33000,
    0xf8934b3a, 0xf1bee000, 0xd00a0f00, 0xeb05462a, 0x2300000e, 0x1b01f912,
    0x4282440b, 0x4672d1fa, 0x2200e001, 0xfb934613, 0x4936fefe, 0xe000f881,
    0x2200e001, 0x49334613, 0x49337808, 0xc000f891, 0xf8914932, 0xb241e000,
    0x0101ebcc, 0xf98efa4f, 0xdd044549, 0x0000ebcc, 0x7008492a, 0x4929e002,
    0xe000f881, 0x491c2000, 0x0024f841, 0xf850481c, 0x31011024, 0x1024f840,
    0x7808491c, 0xd2054286, 0x78094920, 0x360155a9, 0xe004b2f6, 0x7809491d,
    0x1007f808, 0x490c2601, 0x4282550e, 0x442ada06, 0xf9124405, 0x440b1b01,
    0xd1fa42aa, 0xfb934a12, 0x9200f0f0, 0xb2406a15, 0x22014621, 0x47a82300,
    0xe8bdb002, 0xbf0087f0,
    0xc0006af8, // rssi_idx_for_agc[link_id]  shenxin
    0xd02111f8, // EM_BT_RD(EM_BT_RXBIT_ADDR
    0xc0006afc, // rssi_set_for_agc   shenxin
    0xc00009d4, // ld_acl_env[link_id]
    0xc0006b68, // link_no_sync_count[link_id]
    0xc00072fc, // link_agc_thd_tws
    0xc0006b78, // link_no_sync_count_time[link_id]
    0xc0007300, // link_agc_thd_tws_time
    0xc0006b60, // rssi_store_tws
    0xc0006af4, // n   shenxin
    0xc0007304, // rssi_tws_step
    0xc0007308, // rssi_min_value_tws
    0xc0004268, // rwip_rf.rxgain_cntl
    0xc000730c, // link_agc_thd_mobile
    0xc0007310, // link_agc_thd_mobile_time
    0xc0006b88, // rssi_store_mobile
    0xc0007314, // rssi_mobile_step
    0xc0007318, // rssi_min_value_mobile
    0xffffffff, // link_agc_thd_tws
    0x00000030, // link_agc_thd_tws_time
    0x00000014, // rssi_tws_step
    0x0000009c, // rssi_min_value_tws
    0xffffffff, // link_agc_thd_mobile
    0x00000030, // link_agc_thd_mobile_time
    0x00000014, // rssi_mobile_step
    0x000000a1, // rssi_min_value_mobile
    0x99029804, 0xfe96f7ff, 0xfc48f632, 0xb898f626, /*7328*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch6 = {
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch6_ins_data_t3),
    0x0002d458,
    0xbf60f1d9,
    0xc0007050,
    (uint8_t *)bes2300p_patch6_ins_data_t3}; // swagc no sync

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch7 = {
    7,   BTDRV_PATCH_ACT, 0, 0x0000e47a, 0xe0027b20, 0,
    NULL}; // lm_inq_res_ind_handler()  rssi

const uint32_t bes2300p_patch8_ins_data_t3[] = {
    0x2300b430, 0x4d072103, 0x4290b2da, 0xf855d004, 0x2c004023,
    0x4611bf18, 0x2b033301, 0x4608d1f4, 0x4770bc30, 0xc00009d4,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch8 = {
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch8_ins_data_t3),
    0x0002c8c0,
    0xbb2ef1da,
    0xc0006f20,
    (uint8_t *)bes2300p_patch8_ins_data_t3}; // find_other_acl_link

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch9 = {
    9,   BTDRV_PATCH_ACT, 0, 0x000123e8, 0xf7f1bf00, 0,
    NULL}; // ignore lm_get_nb_acl(MASTER_FLAG|SLAVE_FLAG) > 1 when cal wesco

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch10 = {
    10, BTDRV_PATCH_ACT, 0, 0x0002ff48, 0xd9590f05, 0, NULL,
}; // ignore bld_sco_retransmit_dec function

const uint32_t bes2300p_patch11_ins_data_t3[] = {
    0x304bf996,
    0x44233302,
    0xbbfcf629,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch11 = {
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch11_ins_data_t3),
    0x00030748,
    0xbbfef1d6,
    0xc0006f48,
    (uint8_t *)bes2300p_patch11_ins_data_t3}; // ld_calculate_timestamp

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch12 = {
    12, BTDRV_PATCH_ACT, 0, 0x0001bcf4, 0xbf00e004, 0, NULL,
}; // LMP_MSG_HANDLER(max_slot_req)

const uint32_t bes2300p_patch13_ins_data_t3[] = {
    0x3040f894, 0xd1032b00, 0x222a210b, 0xbe00f616, 0xbd06f616};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch13 = {
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch13_ins_data_t3),
    0x0001d63c,
    0xb9f8f1e9,
    0xc0006a30,
    (uint8_t *)bes2300p_patch13_ins_data_t3}; // LMP_MSG_HANDLER(aurand)

const uint32_t bes2300p_patch14_ins_data_t3[] = {
    0xf8534b04, 0x46283026, 0x135ef893, 0xfc62f5fa, 0xbd55f616, 0xc0005de4};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch14 = {
    14,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch14_ins_data_t3),
    0x0001d4f4,
    0xbaa6f1e9,
    0xc0006a44,
    (uint8_t *)bes2300p_patch14_ins_data_t3}; // LMP_MSG_HANDLER(aurand)

const uint32_t bes2300p_patch15_ins_data_t3[] = {
    0x3040f897, 0xd1032b00, 0xbbacf617,
    0x21234620, 0xf836f610, 0xbc8bf617 /*0xc0006a74*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch15 = {
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch15_ins_data_t3),
    0x0001e1c0,
    0xbc4ef1e8,
    0xc0006a60,
    (uint8_t *)bes2300p_patch15_ins_data_t3}; // LMP_MSG_HANDLER(not_accepted)

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch16 = {
    16, BTDRV_PATCH_ACT, 0, 0x0002ea00, 0x8ff8e8bd, 0, NULL,
}; // remove lmp flush

const uint32_t bes2300p_patch17_ins_data_t3[] = {
    0xfe4af5f9, 0xbf002300, 0x30c8f884, 0xbaf8f624, /*6f18*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch17 = {
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch17_ins_data_t3),
    0x0002b508,
    0xbd00f1db,
    0xc0006f0c,
    (uint8_t *)bes2300p_patch17_ins_data_t3}; /// role switch fail

const uint32_t bes2300p_patch18_ins_data_t3[] = {
    0xbf004628, 0xfdc4f60d, 0x21144628, 0xba4af60f, /*6c94*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch18 = {
    18,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch18_ins_data_t3),
    0x00016128,
    0xbdaef1f0,
    0xc0006c88,
    (uint8_t *)bes2300p_patch18_ins_data_t3}; // start lmp to before sending
                                              // LMP_HostConnectionReq

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch19 = {
    19, BTDRV_PATCH_ACT, 0, 0x00020640, 0xe00dbf00, 0, NULL,
}; // detach directly

const uint32_t bes2300p_patch20_ins_data_t3[] = {
    0x305af893, 0xd1032b00, 0x12f4f240,
    0xbf00e002, 0x32b8f640, 0xbe30f60d, /*6bdc*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch20 = {
    20,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch20_ins_data_t3),
    0x0001483c,
    0xb9c4f1f2,
    0xc0006bc8,
    (uint8_t *)bes2300p_patch20_ins_data_t3}; // lmp to 5s before connection cmp
                                              // sent

const uint32_t bes2300p_patch21_ins_data_t3[] = {
    0x4603b508, 0x8210f3ef, 0x0f01f012, 0xb672d112, 0xf5a34811, 0x3b285343,
    0xfba14910, 0xf3c32303, 0xeb032387, 0xeb000343, 0x31940183, 0xfdd0f5fc,
    0xbd08b662, 0xf5a34808, 0x3b285343, 0xfba14907, 0xf3c32303, 0xeb032387,
    0xeb000343, 0x31940183, 0xfdbef5fc, 0xbf00bd08, 0xc0005b4c, 0xff00ff01,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch21 = {
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch21_ins_data_t3),
    0x00008b24,
    0xb8f8f1fe,
    0xc0006d18,
    (uint8_t *)bes2300p_patch21_ins_data_t3}; // add lock interrupt when free
                                              // acl buf

const uint32_t bes2300p_patch22_ins_data_t3[] = {
    0x2040f897, /*6e00*/
    0xd1112a01, 0x0109f106, 0xf5fc4608, 0x2813fd4b, 0x221ed802, 0xbc54f614,
    0x2a017a32, 0xf614d801, 0x221ebbf6, 0xbc4cf614, 0xbc47f614, /*6e2c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch22 = {
    22,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch22_ins_data_t3),
    0x0001b604,
    0xbbfcf1eb,
    0xc0006e00,
    (uint8_t *)bes2300p_patch22_ins_data_t3}; // reject set afh lmp when channel
                                              // number less than 20

/*patch 23,24 for HiSilicon,BCS4.2,ver0x608 and Broadcom,BCS4.2,ver0x6106
 * Enhanced Data Rate problem*/
const uint32_t bes2300p_patch23_ins_data_t3[] = {
    0x0a00b538, /*c0006b90*/
    0xf8534b08, 0xb2c45020, 0xf6274620, 0x4620fdaf,
    0xf895210b, 0xf61d2046, 0x2201ff63, 0x701a4b02,
    0xbf00bd38, 0xc0005de4, 0xc0006bc0, 0x00000000 /*c0006bc0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch23 = {
    23,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch23_ins_data_t3),
    0x000144fc,
    0xbb48f1f2,
    0xc0006b90,
    (uint8_t *)bes2300p_patch23_ins_data_t3}; // table request

const uint32_t bes2300p_patch24_ins_data_t3[] = {
    0x0a00b538, /*c0006be0*/
    0xf8534b0f, 0xb2c45020, 0x104df895, 0x31004620, 0x2101bf18,
    0xfab8f628, 0x8d298ea8, 0xf61d2201, 0x4601f9f7, 0xf6284620,
    0x2301fae5, 0x3062f885, 0xf6274620, 0x2200fd7f, 0x701a4b02,
    0xbf00bd38, 0xc0005de4, 0xc0006bc0, 0x78244c04, 0x4628b114,
    0xffd4f7ff, 0xb00d2000, 0x83f0e8bd, 0xc0006bc0 /*c0006c40*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch24 = {
    24,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch24_ins_data_t3),
    0x00023eb8,
    0xbeb8f1e2,
    0xc0006be0,
    (uint8_t *)bes2300p_patch24_ins_data_t3}; // table request configure
/*end patch 23,24 for HiSilicon,BCS4.2,ver0x608 and Broadcom,BCS4.2,ver0x6106
 * Enhanced Data Rate problem*/

const uint32_t bes2300p_patch25_ins_data_t3[] = {
    0xfa80f5fc, /*c0006f58*/
    0xb9737f43, 0x2b01786b, 0x2b03d01a, 0x2b05d010, 0xf896d00e,
    0x22203044, 0x4620212e, 0xfd90f61d, 0xf896e010, 0x212e2044,
    0xf61d4620, 0xe009fd65, 0xfd70f60c, 0xd1ec2800, 0x325ef896,
    0xd1e82b00, 0xbeb0f614, 0xbec1f614, /*c0006fa0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch25 = {
    25,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch25_ins_data_t3),
    0x0001bcb8,
    0xb94ef1eb,
    0xc0006f58,
    (uint8_t *)bes2300p_patch25_ins_data_t3}; // LMP_MSG_HANDLER(max_slot_req)

const uint32_t bes2300p_patch26_ins_data_t3[] = {
    0x681b4b16, 0xd0222b00, 0xf8b34b16, 0xf01331c0, 0xd1000f18, 0x4b13e020,
    0x71c2f893, 0xff12f61f, 0xd0192f02, 0xf8534b10, 0x2b003027, 0x6b5ad014,
    0xd9074282, 0xf8931a10, 0xeb033042, 0x42980343, 0x2200d80a, 0x46204611,
    0xfa62f629, 0x4620e004, 0xbf002100, 0xbe0cf629, 0xbe0df629,

    0xc0007008, // sco_start_delay_flag
    0x00000000,
    0xc0005ea8, // ld_env
    0xc00009e4, // ld_sco_env

};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch26 = {
    26,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch26_ins_data_t3),
    0x00030c14,
    0xb9c8f1d6,
    0xc0006fa8,
    (uint8_t *)bes2300p_patch26_ins_data_t3}; // ld_acl_sched

const uint32_t bes2300p_patch27_ins_data_t3[] = {
    0xf0234413, 0x63634378, 0x68094904, 0x4904b121,
    0xfb016809, 0x63633300, 0xba5af62c,

    0xc0007008, // sco_start_delay_flag
    0xc000703c, // sco_anchor_start_add
    0x0000000a,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch27 = {
    27,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch27_ins_data_t3),
    0x000334e0,
    0xbd9af1d3,
    0xc0007018,
    (uint8_t *)bes2300p_patch27_ins_data_t3}; // ld_sco_start

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch28 = {
    28, BTDRV_PATCH_ACT, 0, 0x00019f54, 0xbf00e00b, 0, NULL,
}; // lc_sync_ind
// send max_slot anyway

const uint32_t bes2300p_patch29_ins_data_t3[] = {
    0xf8862001,
    0xf8860060,
    0xbf000061,
    0xbba3f611,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch29 = {
    29,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch29_ins_data_t3),
    0x00018f1c,
    0xbc62f1ee,
    0xc00077e4,
    (uint8_t *)bes2300p_patch29_ins_data_t3}; // ignore 0x5d sm

// for ble master update con param with large winsize
const uint32_t bes2300p_patch30_ins_data_t3[] = {
    0x4630bf00, 0x109ef894, 0xf958f63f, 0x309cf894,
    0x2271f240, 0x0003fb02, 0xbcf1f63f,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch30 = {
    30,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch30_ins_data_t3),
    0x000471e4,
    0xbb08f1c0,
    0xc00077f8,
    (uint8_t *)bes2300p_patch30_ins_data_t3};

static const uint32_t best2300p_t3_ins_patch_config[] = {
    31,
    (uint32_t)&bes2300p_t3_ins_patch0,
    (uint32_t)&bes2300p_t3_ins_patch1,
    (uint32_t)&bes2300p_t3_ins_patch2,
    (uint32_t)&bes2300p_t3_ins_patch3,
    (uint32_t)&bes2300p_t3_ins_patch4,
    (uint32_t)&bes2300p_t3_ins_patch5,
    (uint32_t)&bes2300p_t3_ins_patch6,
    (uint32_t)&bes2300p_t3_ins_patch7,
    (uint32_t)&bes2300p_t3_ins_patch8,
    (uint32_t)&bes2300p_t3_ins_patch9,
    (uint32_t)&bes2300p_t3_ins_patch10,
    (uint32_t)&bes2300p_t3_ins_patch11,
    (uint32_t)&bes2300p_t3_ins_patch12,
    (uint32_t)&bes2300p_t3_ins_patch13,
    (uint32_t)&bes2300p_t3_ins_patch14,
    (uint32_t)&bes2300p_t3_ins_patch15,
    (uint32_t)&bes2300p_t3_ins_patch16,
    (uint32_t)&bes2300p_t3_ins_patch17,
    (uint32_t)&bes2300p_t3_ins_patch18,
    (uint32_t)&bes2300p_t3_ins_patch19,
    (uint32_t)&bes2300p_t3_ins_patch20,
    (uint32_t)&bes2300p_t3_ins_patch21,
    (uint32_t)&bes2300p_t3_ins_patch22,
    (uint32_t)&bes2300p_t3_ins_patch23,
    (uint32_t)&bes2300p_t3_ins_patch24,
    (uint32_t)&bes2300p_t3_ins_patch25,
    (uint32_t)&bes2300p_t3_ins_patch26,
    (uint32_t)&bes2300p_t3_ins_patch27,
    (uint32_t)&bes2300p_t3_ins_patch28,
    (uint32_t)&bes2300p_t3_ins_patch29,
    (uint32_t)&bes2300p_t3_ins_patch30,
};
#endif

#ifndef IBRT

/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1302 T2(CHIP ID=1)
 *
 * __BT_ONE_BRING_TWO__ function
 *
 ****************************************************************************/

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch0 = {
    0,   BTDRV_PATCH_ACT, 0, 0x00030d48, 0xbf00bf00, 0,
    NULL}; // set lsto when there is no connect

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch1 = {
    1, BTDRV_PATCH_ACT, 0, 0x00019acc, 0xbf00e003, 0, NULL}; ////iocap rsp

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch2 = {
    2,   BTDRV_PATCH_ACT, 0, 0x0001a7a4, 0xbf00e003, 0,
    NULL}; // pwer rsp in wrong state

const uint32_t bes2300p_patch3_ins_data_t2[] = {
    0xf5fa4620,                                     /*6900*/
    0x285dfd4b, 0xf240d10e, 0x46215005, 0xf84cf5fa, /*6910*/
    0xf8534b05, 0x46203026, 0x135ef893, 0xfcd0f5fa, /*6920*/
    0xb9f3f617, 0xb9ebf617, 0xc0005cd8,             /*692c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch3_ins_data_t2),
    0x0001dcfc,
    0xbe00f1e8,
    0xc0006900,
    (uint8_t *)bes2300p_patch3_ins_data_t2}; // not accept

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch4 = {
    4,   BTDRV_PATCH_ACT, 0, 0x0002a4a4, 0xbf8c7f80, 0,
    NULL}; // LD_ACL_STOP_NOTIF_BIT_OFF_THR

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch5 = {
    5,   BTDRV_PATCH_ACT, 0, 0x0002a7e4, 0x7f80f5b3, 0,
    NULL}; // LD_ACL_STOP_NOTIF_BIT_OFF_THR

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch6 = {
    6,   BTDRV_PATCH_ACT, 0, 0x0002a69c, 0xbf947f80, 0,
    NULL}; // LD_ACL_STOP_NOTIF_BIT_OFF_THR

const uint32_t bes2300p_patch7_ins_data_t2[] = {
    0xd01e0fdb,                                     /*6930*/
    0x6307f44f, 0xf304fb03, 0x48168f6a, 0x93011810, /*6940*/
    0x49154418, 0xfea0f644, 0x58fb4b14, 0x4301f3c3, /*6950*/
    0xf1028f2a, 0xf5024250, 0xf2021205, 0x8f6942fc, /*6960*/
    0x0241eb02, 0x440a9901, 0x600a490d, 0xf44fe010, /*6970*/
    0xfb036307, 0x9301f304, 0x18184806, 0x8f6a4906, /*6980*/
    0xfe82f644, 0x58fb4b05, 0x4381f3c3, 0xba27f62e, /*6990*/
    0xba2df62e,

    0xd02144fc, 0xc000593c, 0xd0220144, /*69a0*/
    0xc000005c,                         /*69a4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch7 = {
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch7_ins_data_t2),
    0x00034d80,
    0xbdd6f1d1,
    0xc0006930,
    (uint8_t *)bes2300p_patch7_ins_data_t2}; // ld_sco_frm_isr

const uint32_t bes2300p_patch8_ins_data_t2[] = {
    0xf6052002,                                     /*69a8*/
    0x2802ffbf, 0xf898d110,                         /*69b0*/
    0x230120b2, 0x4a074093, 0x21c0f8b2, 0x0303ea22, /*69c0*/
    0x0341f3c3, 0xf8524a04, 0xbf003023, 0xbbecf625, /*69d0*/
    0xbbeef625,

    0xc0005d9c, // ld_env
    0xc0000998, // ld_acl_env /*69dc*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch8 = {
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch8_ins_data_t2),
    0x0002c188,
    0xbc0ef1da,
    0xc00069a8,
    (uint8_t *)bes2300p_patch8_ins_data_t2}; // ld_event_timestamp_calculate

const uint32_t bes2300p_patch9_ins_data_t2[] = {
    0xf8934b07,                                     /*69e0*/
    0xf0133084, 0xd0070f06, 0xf6364620, 0x4601fa4b, /*69f0*/
    0xbf002400, 0xbbd2f635, 0xbb91f635,

    0xc0006460, // llm_le_env /*6a00*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch9 = {
    9,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch9_ins_data_t2),
    0x0003c11c,
    0xbc60f1ca,
    0xc00069e0,
    (uint8_t *)bes2300p_patch9_ins_data_t2}; // hci_le_set_adv_data_cmd_handler

const uint32_t bes2300p_patch10_ins_data_t2[] = {
    0x123ff240,                                     /*6a04*/
    0xd006421a, 0x2b007aab, 0x2201d003,             /*6a10*/
    0x601a4b05, 0x2201e002, 0x601a4b03, 0xf240892b, /*6a20*/
    0xbf00123f, 0xbc1af63e,

    0xc0006a30, // le_wrong_packet
    0x00000000, /*6a30*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch10 = {
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch10_ins_data_t2),
    0x0004525c,
    0xbbd2f1c1,
    0xc0006a04,
    (uint8_t *)bes2300p_patch10_ins_data_t2}; // lld_pdu_rx_handler

const uint32_t bes2300p_patch11_ins_data_t2[] = {
    0xf894b952,                                     /*6a34*/
    0xf01330ae, 0xd1050f09, 0x209ff894,             /*6a40*/
    0x4a03b912, 0xb10a6812, 0xbc05f63f, 0xbc09f63f, /*6a50*/

    0xc0006a30, /*6a54*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch11 = {
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch11_ins_data_t2),
    0x00046248,
    0xbbf4f1c0,
    0xc0006a34,
    (uint8_t *)bes2300p_patch11_ins_data_t2}; // lld_evt_restart

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch12 = // from bes2300p_t2_ibrt_ins_patch2
    {12,  BTDRV_PATCH_ACT, 0, 0x00007024, 0xbf00e011, 0,
     NULL}; // dont call rwip_assert_err when assert error

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch13 = // from bes2300p_t2_ibrt_ins_patch4
    {13,  BTDRV_PATCH_ACT, 0, 0x0002be90, 0xbd702000, 0,
     NULL}; // ld_check_if_need_protect_sniff retrun 0

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch14 = // from bes2300p_t2_ibrt_ins_patch9
    {
        14, BTDRV_PATCH_ACT, 0, 0x0002d82c, 0x2b006013, 0, NULL,
}; // ld_acl_rsw_rx

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch15 = // from bes2300p_t2_ibrt_ins_patch10
    {
        15, BTDRV_PATCH_ACT, 0, 0x0002fe60, 0xbf00e001, 0, NULL,
}; // ld_acl_flow_on

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch16 = // from bes2300p_t2_ibrt_ins_patch12
    {16,  BTDRV_PATCH_ACT, 0, 0x0002a41c, 0xbf000f01, 0,
     NULL}; // mode == SYNC2_SCO_MODE

const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch17 = // from bes2300p_t2_ibrt_ins_patch17
    {
        17,         BTDRV_PATCH_ACT,
        0,
        0x00007010, // a0007010
        0xb9bef000, // jump a0007390
        0,          NULL,
}; // DEF_IRQHandler

const uint32_t bes2300p_patch18_ins_data_t2[] = {
    0x4681b083, /*6b30*/
    0x4b022200, 0x2020f843, 0xbddcf623, 0xc0006b44,
    0x00000000, 0x00000000, 0x00000000, /*6b4c*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch18 = // from bes2300p_t2_ibrt_ins_patch22
    {18,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch18_ins_data_t2),
     0x0002a6f4,
     0xba1cf1dc,
     0xc0006b30,
     (uint8_t *)bes2300p_patch18_ins_data_t2}; // sync clear count

const uint32_t bes2300p_patch19_ins_data_t2[] = {
    /*6ba0*/ 0xbf0080c6,
    /*6ba4*/ 0x71022200,
    /*6ba8*/ 0xbb2af619, // jump a0020200
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch19 = // from bes2300p_t2_ibrt_ins_patch29
    {19,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch19_ins_data_t2),
     0x000201f8,
     0xbcd2f1e6, // jump a00201f8 -> a0206ba0
     0xc0006ba0,
     (uint8_t *)
         bes2300p_patch19_ins_data_t2}; // KE_MSG_HANDLER(lc_op_loc_sniff_req,
                                        // void)

const uint32_t bes2300p_patch20_ins_data_t2[] = {
    /*6bac*/ 0x46286825,
    /*6bb0*/ 0x303ef895,
    /*6bb4*/ 0xd0012b02,
    /*6bb8*/ 0xbda2f622, // jump a0029700
    /*6bbc*/ 0xbf002300,
    /*6bc0*/ 0x303ef885,
    /*6bc4*/ 0xfea6f622, // call a0029914
    /*6bc8*/ 0xbdc1f622, // jump a002974e
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch20 = // from bes2300p_t2_ibrt_ins_patch30
    {20,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch20_ins_data_t2),
     0x000296fc,
     0xba56f1dd, // jump a00296fc -> a0206bac
     0xc0006bac,
     (uint8_t *)bes2300p_patch20_ins_data_t2}; // ld_pscan_frm_cbk

const uint32_t bes2300p_patch21_ins_data_t2[] = {
    0x4620461d,                         /*6d00*/
    0x463a4629, 0xf5f94633, 0xbdf8fc2b, /*6d0c*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch21 = // from bes2300p_t2_ibrt_ins_patch0
    {21,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch21_ins_data_t2),
     0x000070ec,
     0xbe08f1ff,
     0xc0006d00,
     (uint8_t *)bes2300p_patch21_ins_data_t2}; // assert warn

const uint32_t bes2300p_patch22_ins_data_t2[] = {
    0x4630461f,                         /*6d14*/
    0x462a4639, 0xf5f94623, 0xbdf8fc21, /*6d20*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch22 = // from bes2300p_t2_ibrt_ins_patch1
    {22,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch22_ins_data_t2),
     0x00007070,
     0xbe50f1ff,
     0xc0006d14,
     (uint8_t *)bes2300p_patch22_ins_data_t2}; // assert param

const uint32_t bes2300p_patch23_ins_data_t2[] = {
    0x2a140c92,                         /*6d30*/
    0x2214bf28, 0xbf004691, 0xbc44f623, /*6d3c*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch23 = // from bes2300p_t2_ibrt_ins_patch40
    {23,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch23_ins_data_t2),
     0x0002a5c4,
     0xbbb4f1dc,
     0xc0006d30,
     (uint8_t *)bes2300p_patch23_ins_data_t2}; // max_drift limit

const uint32_t bes2300p_patch24_ins_data_t2[] = {
    /*6d9c*/ 0x4b179a0b,
    /*6da0*/ 0xd023429a,
    /*6da4*/ 0x20b5f894,
    /*6da8*/ 0xb9fab2d2,
    /*6dac*/ 0xf9b4b22e,
    /*6db0*/ 0x1b9b3098,
    /*6db4*/ 0xf9b24a0f,
    /*6db8*/ 0x42932000,
    /*6dbc*/ 0x4251da05,
    /*6dc0*/ 0xdd02428b,
    /*6dc4*/ 0x7088f8d4,
    /*6dc8*/ 0x4293e00f,
    /*6dcc*/ 0xf8d4dd05,
    /*6dd0*/ 0x3f017088,
    /*6dd4*/ 0x4778f027,
    /*6dd8*/ 0x4252e007,
    /*6ddc*/ 0xda044293,
    /*6de0*/ 0x7088f8d4,
    /*6de4*/ 0xf0273701,
    /*6de8*/ 0xbf004778,
    /*6dec*/ 0x708cf8c4,
    /*6df0*/ 0xbce4f623, // jump a002a7bc
    /*6df4*/ 0xc0006df8,
    /*6df8*/ 0x00000032, // 50
    /*6dfc*/ 0xa003477b, // ld_acl_sniff_frm_isr lr
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch24 = // from bes2300p_t2_ibrt_ins_patch46
    {24,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch24_ins_data_t2),
     0x0002a7b8,
     0xbaf0f1dc, // jump a002a7b8 -> a0206d9c
     0xc0006d9c,
     (uint8_t *)bes2300p_patch24_ins_data_t2}; // ld_acl_rx_sync clk_off cal
                                               // active mode

const uint32_t bes2300p_patch25_ins_data_t2[] = {
    /*6e10*/ 0xbf00669a,
    /*6e14*/ 0x66da466a, // mov   r2, sp; str   r2, [r3, #108]
    /*6e18*/ 0x8208f3ef, // mrs   r2, MSP
    /*6e1c*/ 0xbf00671a, // str   r2, [r3, #112]
    /*6e20*/ 0x8209f3ef, // mrs   r2, PSP
    /*6e24*/ 0xbf00675a, // str   r2, [r3, #116]
    /*6e28*/ 0xbf00bf00,
    /*6e2c*/ 0xbf00bf00,
    /*6e30*/ 0xbf00bf00,
    /*6e34*/ 0xbf00bf00,
    /*6e38*/ 0xbf00bf00,
    /*6e3c*/ 0xbf00bf00,
    /*6e40*/ 0xbf00bf00,
    /*6e44*/ 0xbf00bf00,
    /*6e48*/ 0xbf00bf00,
    /*6e4c*/ 0xbf00bf00,
    /*6e50*/ 0xba90f600, // jump a0206e50 -> a0007374
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch25 = // from bes2300p_t2_ibrt_ins_patch15
    {25,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch25_ins_data_t2),
     0x00007360,
     0xbd56f1ff, // jump a0007360 -> a0206e10
     0xc0006e10,
     (uint8_t *)bes2300p_patch25_ins_data_t2}; // bt_assert_crash_dump

#define PATCH_IN_EXTRA_SPACE 1
#if PATCH_IN_EXTRA_SPACE
const uint32_t bes2300p_patch26_ins_data_t2[] = {
    0xd8092b01,                                     /*70d0*/
    0xfb03236e, 0x4a04f308, 0xf4135a9b, 0xd1016f00, /*70e0*/
    0xb919f629, 0xb9aef629, 0xd0211200,             /*70ec*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch26 = // from bes2300p_t2_ibrt_ins_patch44
    {26,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch26_ins_data_t2),
     0x00030314,
     0xbedcf1d6,
     0xc00070d0,
     (uint8_t *)bes2300p_patch26_ins_data_t2}; // lmp tx for tx desc error

const uint32_t bes2300p_patch27_ins_data_t2[] = {
    0xfeecf5f8,                                     /*7300*/
    0xbf00bf00, 0x8310f3ef, 0x0401f013, 0xb672d108, /*7310*/
    0xff22f5f8, 0xbf30b110, 0xd1ef2c00, 0xe7edb662, /*7320*/
    0xff1af5f8, 0xd1f62800, 0xbf00e7e8,             /*732c*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch27 = // from bes2300p_t2_ibrt_ins_patch13
    {27,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch27_ins_data_t2),
     0x0000721c,
     0xb870f200,
     0xc0007300,
     (uint8_t *)bes2300p_patch27_ins_data_t2};

const uint32_t bes2300p_patch28_ins_data_t2[] = {
    0xf885b2db,                                     /*7338*/
    0xf89530bd, 0x2a0220b4,                         /*7340*/
    0xb98bd114, 0xf8534b0a, 0xf6430024, 0xf895fd15, /*7350*/
    0x2b0430b5, 0x4620d103, 0xf6232116, 0x4620fc61, /*7360*/
    0xf6242116, 0xbf00f961, 0xbc6bf62d, 0xbc53f62d, /*7370*/
    0xc0000998,                                     /*7374*/
};
const BTDRV_PATCH_STRUCT
    bes2300p_t2_ins_patch28 = // from bes2300p_t2_ibrt_ins_patch15
    {28,
     BTDRV_PATCH_ACT,
     sizeof(bes2300p_patch28_ins_data_t2),
     0x00034c14,
     0xbb90f1d2,
     0xc0007338,
     (uint8_t *)bes2300p_patch28_ins_data_t2}; // no disconnect evt
#endif                                         /* PATCH_IN_EXTRA_SPACE */

/////2300p t2 patch
static const uint32_t best2300p_t2_ins_patch_config[] = {
    29,
    (uint32_t)&bes2300p_t2_ins_patch0,
    (uint32_t)&bes2300p_t2_ins_patch1,
    (uint32_t)&bes2300p_t2_ins_patch2,
    (uint32_t)&bes2300p_t2_ins_patch3,
    (uint32_t)&bes2300p_t2_ins_patch4,
    (uint32_t)&bes2300p_t2_ins_patch5,
    (uint32_t)&bes2300p_t2_ins_patch6,
    (uint32_t)&bes2300p_t2_ins_patch7,
    (uint32_t)&bes2300p_t2_ins_patch8,
    (uint32_t)&bes2300p_t2_ins_patch9,
    (uint32_t)&bes2300p_t2_ins_patch10,
    (uint32_t)&bes2300p_t2_ins_patch11,
    (uint32_t)&bes2300p_t2_ins_patch12,
    (uint32_t)&bes2300p_t2_ins_patch13,
    (uint32_t)&bes2300p_t2_ins_patch14,
    (uint32_t)&bes2300p_t2_ins_patch15,
    (uint32_t)&bes2300p_t2_ins_patch16,
    (uint32_t)&bes2300p_t2_ins_patch17,
    (uint32_t)&bes2300p_t2_ins_patch18,
    (uint32_t)&bes2300p_t2_ins_patch19,
    (uint32_t)&bes2300p_t2_ins_patch20,
    (uint32_t)&bes2300p_t2_ins_patch21,
    (uint32_t)&bes2300p_t2_ins_patch22,
    (uint32_t)&bes2300p_t2_ins_patch23,
    (uint32_t)&bes2300p_t2_ins_patch24,
    (uint32_t)&bes2300p_t2_ins_patch25,
    (uint32_t)&bes2300p_t2_ins_patch26,
    (uint32_t)&bes2300p_t2_ins_patch27,
    (uint32_t)&bes2300p_t2_ins_patch28,
};

#else
/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1302 T2 (CHIP ID=1)
 *
 *  IBRT function
 *
 * patch  3: 0xC0006900--->0xC0006910
 * patch  45:0xC0006930--->0xC0006940
 * patch  5: 0xC000694c--->0xC0006980
 * patch 14: 0xC000698c--->0xC00069cc
 * patch 16: 0xC0006a18--->0xC0006a28//REMOVE
 * patch 19: 0xC0006a2c--->0xC0006a9c
 * patch 20: 0xC0006aa4--->0xC0006ab0
 * patch 21: 0xC0006ab4--->0xC0006ad0
 * patch  2: 0xC0006ae8--->0xC0006b04
 * patch 22: 0xC0006b30--->0xC0006b4c
 * patch 22: 0xC0006b54--->0xC0006b7c
 * patch 29: 0xC0006ba0--->0xC0006ba8
 * patch 30: 0xC0006bac--->0xC0006bc8
 * patch 32: 0xC0006be8--->0xC0006c14
 * patch 33: 0xC0006c1c--->0xC0006c2c
 * patch 34: 0xC0006c30--->0xC0006c44
 * patch 35: 0xC0006c48--->0xC0006c84
 * patch 36: 0xC0006c88--->0xC0006c98
 * patch 37: 0xC0006ca0--->0xC0006cb8
 * patch 38: 0xC0006cc0--->0xC0006cf8
 * patch  0: 0xC0006d00--->0xC0006d0c
 * patch  1: 0xC0006d14--->0xC0006d20
 * patch 44: 0xC0006d24--->0xC0006d2c (via ble)
 * patch 40: 0xC0006d30--->0xC0006d40
 * patch 45: 0xC0006d50--->0xC0006d90
 * patch 46: 0xC0006d9c--->0xC0006dfc
 * patch 47: 0xC0006e00--->0xC0006e28
 * patch 48: 0xC0006e30--->0xC0006e98
 * patch 49: 0xC0006ec0--->0xC0006ed0
 * patch 50: 0xC0006ed8--->0xC0006f2c
 * patch 51: 0xC0006f30--->0xC0006f5c
 * patch 53: 0xC0006f74--->0xC0006f8c
 * patch 25: 0xC0006f90--->0xc0007074
 * patch 55: 0xC00070a4--->0xC00070c4
 * patch 44: 0xC00070d0--->0xC00070ec
 * sniffer_filter_tab: 0xC00070f4--->0xC0007130
 * patch  7: 0xC0007138--->0xC0007164
 * patch 23: 0xC0007180--->0xC000720c
 * patch 17: 0xC00072b4--->0xC00072ec
 * patch 13: 0xC0007300--->0xC0007330
 * patch 15: 0xC0007338--->0xC0007374
 * patch 31: 0xC000737c--->0xC0007a4
 * patch 54: 0xC00073cc--->0XC00074E4(new)
 ****************************************************************************/
//#define ECC_LL_MONITOR

const uint32_t bes2300p_patch0_ibrt_ins_data_t2[] = {
    /*6d00*/ 0x4673b408,
    /*6d04*/ 0x0f20f013,
    /*6d08*/ 0xbc08d002,
    /*6d0c*/ 0xb9e5f600, // jump a00070da
    /*6d10*/ 0xbf00bc08,
    /*6d14*/ 0x4b014699,
    /*6d18*/ 0xbaeaf600, // jump a00072f0
    /*6d1c*/ 0xc0000050,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch0 = {
    0,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch0_ibrt_ins_data_t2),
    0x000072ec,
    0xbd08f1ff, // jump a00072ec -> a0206d00
    0xc0006d00,
    (uint8_t *)bes2300p_patch0_ibrt_ins_data_t2}; // assert warn, assert param

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch1 = {
    1,          BTDRV_PATCH_ACT,
    0,          0x0001ff2c,
    0xb81af000,        // jump a001ff2c -> a001ff64
    0,          NULL}; // KE_MSG_HANDLER(lc_op_loc_unsniff_req)

#if 0
const uint32_t bes2300p_patch2_ibrt_ins_data_t2[] =
{
    0x460a4603,/*6ae8*/
    0x46194802,
    0xfcb6f5fc,
    0xbf00bd38,
    0xc0006afc,/*6af8*/
    0x73617243,
    0x73252c68,
    0x0064252c,/*6b04*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch2 =
{
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch2_ibrt_ins_data_t2),
    0x00007018,
    0xbd66f1ff,
    0xc0006ae8,
    (uint8_t *)bes2300p_patch2_ibrt_ins_data_t2
};//"Crash,%s,%d", file, line
#elif 1
#if 0
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch2 =
{
    2,
    BTDRV_PATCH_ACT,
    0,
    0x00007024,
    0xbf00e011,
    0,
    NULL
};//dont call rwip_assert_err when assert error
#else
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch2 = {
    2,          BTDRV_PATCH_ACT,
    0,
    0x0002d8b0, // 0xa002d8b0
    0xbf00bf00, 0,
    NULL}; // ld_acl_rsw_frm_isr
#endif
#else
const uint32_t bes2300p_patch2_ibrt_ins_data_t2[] = {
    0xfc12f631, /*7780*/
    0xb1337843, 0x0518f100, 0xf5fb4628, 0x2813fed9,
    0xf605d804, 0xbf00fba1, 0xba24f60c, 0xba23f60c /*77a0*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch2 = {
    2,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_patch2_ibrt_ins_data_t2),
    0x00013be4,
    0xbdccf1f3,
    0xc0007780,
    (uint8_t *)bes2300p_patch2_ibrt_ins_data_t2}; // lc_afh_set use mobile afh
#endif

const uint32_t bes2300p_patch3_ibrt_ins_data_t2[] = {
    0xd1052800,                                     /*6900*/
    0x000cf1aa, 0xf97af5fa, 0xbc48f61a, 0xbbd2f61a, /*6910*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch3_ibrt_ins_data_t2),
    0x000210b4,
    0xbc24f1e5,
    0xc0006900,
    (uint8_t *)bes2300p_patch3_ibrt_ins_data_t2}; // lmp filter free heap

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch4 = {
    4,   BTDRV_PATCH_ACT, 0, 0x0002be90, 0xbd702000, 0,
    NULL}; // ld_check_if_need_protect_sniff retrun 0

#if 0
const uint32_t bes2300p_patch5_ibrt_ins_data_t2[] =
{
    /*6950*/ 0x0f01f013,
    /*6954*/ 0x4b06d00a,
    /*6958*/ 0x104ff893,
    /*695c*/ 0xd8052902,
    /*6960*/ 0x2a027a9a,
    /*6964*/ 0xbf00d802,
    /*6968*/ 0xbc73f632,
    /*696c*/ 0xbf00bd08,
    /*6970*/ 0xc00063c8,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch5 =
{
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch5_ibrt_ins_data_t2),
    0x0003924c,
    0xbb80f1cd, // jump a00384dc -> a0206950
    0xc0006950,
    (uint8_t *)bes2300p_patch5_ibrt_ins_data_t2
}; //protect link id when send bitoffset

const uint32_t bes2300p_patch6_ibrt_ins_data_t2[] =
{
    /*6978*/ 0x5023f852,
    /*697c*/ 0xd0012d00,
    /*6980*/ 0xbc04f611,// jump a0206980 -> a001818c
    /*6984*/ 0xbd382000,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch6 =
{
    6,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_patch6_ibrt_ins_data_t2),
    0x00018188,
    0xbbf6f1ee, // jump a0018188 -> a0206978
    0xc0006978,
    (uint8_t *)bes2300p_patch6_ibrt_ins_data_t2
};//protect null ptr when send afh
#else
const uint32_t bes2300p_patch5_ibrt_ins_data_t2[] = {
    /*694c*/ 0xf632b1c0,
    /*6950*/ 0xb1a8fc1b,
    /*6954*/ 0x70b2f898,
    /*6958*/ 0xfc2af632,
    /*695c*/ 0xd10d4287,
    /*6960*/ 0xfbfaf634,
    /*6964*/ 0xbf142804,
    /*6968*/ 0x22012200,
    /*696c*/ 0x46294640,
    /*6970*/ 0xf6259b03,
    /*6974*/ 0x2001fae5,
    /*6978*/ 0xbecef625,
    /*697c*/ 0xbc6cf625,
    /*6980*/ 0xbcc9f625,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch5 = {
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch5_ibrt_ins_data_t2),
    0x0002c230,
    0xbb8cf1da, // jump a002c230 -> a020694c
    0xc000694c,
    (uint8_t *)bes2300p_patch5_ibrt_ins_data_t2}; // ld_cal_timestamp

#if 1
const uint32_t bes2300p_patch6_ibrt_ins_data_t2[] = {
    0x79ab4688, 0x6383ea6f, 0x4578f023, 0xf8d39b09,
    0x9906308c, 0x402b440b, 0xbf004641, 0xbb87f62c,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch6 = {
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch6_ibrt_ins_data_t2),
    0x000330f4,
    0xbc6ef1d3, // jump a00330f4 -> a02069d4
    0xc00069d4,
    (uint8_t *)bes2300p_patch6_ibrt_ins_data_t2}; // sco param init
#else
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch6 = {
    6,          BTDRV_PATCH_ACT,
    0,
    0x00007010, // a0007010
    0xb9bef000, // jump a0007390
    0,          NULL,
}; // DEF_IRQHandler
#endif

#endif
const uint32_t bes2300p_patch7_ibrt_ins_data_t2[] = {
    /*7138*/ 0x28024604,
    /*713c*/ 0xf89ad812,
    /*7140*/ 0x28020046,
    /*7144*/ 0xf610d803,
    /*7148*/ 0x2800fb1b,
    /*714c*/ 0x2112d106,
    /*7150*/ 0xf61b20ff,
    /*7154*/ 0x2000fec9,
    /*7158*/ 0xbb0af633,
    /*715c*/ 0xbf004620,
    /*7160*/ 0xba4ef633,
    /*7164*/ 0xbad8f633,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch7 = {
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch7_ibrt_ins_data_t2),
    0x0003a5f8,
    0xbd9ef1cc, // jump a003a5f8 -> a0207138
    0xc0007138,
    (uint8_t *)
        bes2300p_patch7_ibrt_ins_data_t2}; // start simu link check linkid

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch8 = {
    8, BTDRV_PATCH_ACT, 0, 0x0002e720, 0xf8a62308, 0, NULL,
}; // ld_acl_start t_poll

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch9 = {
    9, BTDRV_PATCH_ACT, 0, 0x0002d82c, 0x2b006013, 0, NULL,
}; // ld_acl_rsw_rx

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch10 = {
    10, BTDRV_PATCH_ACT, 0, 0x0002fe60, 0xbf00e001, 0, NULL,
}; // ld_acl_flow_on

#ifdef ECC_LL_MONITOR
////patch 11 12 13 used for ecc count by set it to ll monitor parameter
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch11 = {
    11, BTDRV_PATCH_ACT, 0, 0x0002dc94, 0x7000f400, 0, NULL}; ////wrong flag

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch12 = {
    12, BTDRV_PATCH_ACT, 0, 0x0002dc98, 0x2800bf00, 0, NULL};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch13 = {
    13, BTDRV_PATCH_ACT, 0, 0x0002f78c, 0x46288113, 0, NULL};
#else
const uint32_t bes2300p_patch11_ibrt_ins_data_t2[] = {
    0x0034f104, 0x220a4649, 0xfd86f644, 0xf632b940, 0x2801fa39, 0xf632d104,
    0x2800fb0d, 0xbf00d104, 0x00c5f894, 0xb97cf62b, 0xb9eaf62b,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch11 = {
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch11_ibrt_ins_data_t2),
    0x00031e70,
    0xbe70f1d4,
    0xc0006b54,
    (uint8_t *)bes2300p_patch11_ibrt_ins_data_t2}; // filter afh map

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch12 = {
    12,  BTDRV_PATCH_ACT, 0, 0x0002a41c, 0xbf000f01, 0,
    NULL}; // mode == SYNC2_SCO_MODE

const uint32_t bes2300p_patch13_ibrt_ins_data_t2[] = {
    0xfeecf5f8, 0xbf00bf00, 0x8310f3ef, 0x0401f013, 0xb672d108, 0xff22f5f8,
    0xbf30b110, 0xd1ef2c00, 0xe7edb662, 0xff1af5f8, 0xd1f62800, 0xbf00e7e8,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch13 = {
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch13_ibrt_ins_data_t2),
    0x0000721c,
    0xb870f200,
    0xc0007300,
    (uint8_t *)bes2300p_patch13_ibrt_ins_data_t2};
#endif

const uint32_t bes2300p_patch14_ibrt_ins_data_t2[] = {
    /*698c*/ 0x4604b510,
    /*6990*/ 0xfc0ef632,
    /*6994*/ 0xd00542a0,
    /*6998*/ 0x7a984b03,
    /*699c*/ 0xbf181b00,
    /*69a0*/ 0xbd102001,
    /*69a4*/ 0xbd102000,
    /*69a8*/ 0xc00063c8,
    /*69ac*/ 0x461f7a9e,
    /*69b0*/ 0x46104625,
    /*69b4*/ 0xffeaf7ff,
    /*69b8*/ 0xd1072800,
    /*69bc*/ 0x46392004,
    /*69c0*/ 0xfc74f634,
    /*69c4*/ 0x462c2003,
    /*69c8*/ 0xbd68f634,
    /*69cc*/ 0xbf00bdf8,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch14 = {
    14,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch14_ibrt_ins_data_t2),
    0x0003b498,
    0xba88f1cb, // jump a003b498 -> a02069ac
    0xc000698c,
    (uint8_t *)
        bes2300p_patch14_ibrt_ins_data_t2}; // protect idx when snoop disconnect

#if 0
const uint32_t bes2300p_patch15_ibrt_ins_data_t2[] =
{
    /*69D0*/ 0xbf00669a,
    /*69D4*/ 0x66da466a,//mov   r2, sp; str   r2, [r3, #108]
    /*69D8*/ 0x8208f3ef,//mrs   r2, MSP
    /*69DC*/ 0xbf00671a,//str   r2, [r3, #112]
    /*69E0*/ 0x8209f3ef,//mrs   r2, PSP
    /*69E4*/ 0xbf00675a,//str   r2, [r3, #116]
    /*69E8*/ 0xbf00bf00,
    /*69EC*/ 0xbf00bf00,
    /*69F0*/ 0xbf00bf00,
    /*69F4*/ 0xbf00bf00,
    /*69F8*/ 0xbf00bf00,
    /*69FC*/ 0xbf00bf00,
    /*6A00*/ 0xbf00bf00,
    /*6A04*/ 0xbf00bf00,
    /*6A08*/ 0xbf00bf00,
    /*6A0C*/ 0xbf00bf00,
    /*6A10*/ 0xbcb0f600,//jump a0206a10 -> a0007374
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch15 =
{
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch15_ibrt_ins_data_t2),
    0x00007360,
    0xbb36f1ff, // jump a0007360 -> a02069d0
    0xc00069d0,
    (uint8_t *)bes2300p_patch15_ibrt_ins_data_t2
};//bt_assert_crash_dump
#else

const uint32_t bes2300p_patch15_ibrt_ins_data_t2[] = {
    0xf885b2db,                                     /*7338*/
    0xf89530bd, 0x2a0220b4, 0xb98bd114, 0xf8534b0a, /*7348*/
    0xf6430024, 0xf895fd15, 0x2b0430b5, 0x4620d103, /*7358*/
    0xf6232116, 0x4620fc61, 0xf6242116, 0xbf00f961, /*7368*/
    0xbc6bf62d, 0xbc53f62d, 0xc0000998,             /*7374*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch15 = {
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch15_ibrt_ins_data_t2),
    0x00034c14,
    0xbb90f1d2,
    0xc0007338,
    (uint8_t *)bes2300p_patch15_ibrt_ins_data_t2}; // no disconnect evt
#endif

const uint32_t bes2300p_patch16_ibrt_ins_data_t2[] = {
    0x71ab2300, 0x0000f8d9, 0x380cb110, 0xf96cf5fa, 0xbf002300, 0xbce2f608,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch16 = {
    16,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch16_ibrt_ins_data_t2),
    0x0000f2f0,
    0xbb12f1f7,
    0xc0006918,
    (uint8_t *)
        bes2300p_patch16_ibrt_ins_data_t2}; // lm_page_scan_end_ind free msg

#if 0
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch17 =
{
    17,
    BTDRV_PATCH_ACT,
    0,
    0x00007010, //a0007010
    0xb9bef000, //jump a0007390
    0,
    NULL,
};//DEF_IRQHandler
#elif 1
const uint32_t bes2300p_patch17_ibrt_ins_data_t2[] = {
    0x4605b538,                                     /*72b4*/
    0xfe76f631, 0x7a834604, 0x2f15ebb3, 0x7803d10a, /*72c4*/
    0xd1072b02, 0xff5cf631, 0xf894b120, 0x2101004f, /*72d4*/
    0xfc78f61d, 0xbf00bd38, 0x3312f886, 0xbf004620, /*72e4*/
    0xffe4f7ff, 0xbcfaf614,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch17 = {
    17,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch17_ibrt_ins_data_t2),
    0x0001bce0,
    0xbafef1eb,
    0xc00072b4,
    (uint8_t *)bes2300p_patch17_ibrt_ins_data_t2}; // send unsniff cmd

#else
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch17 = {
    17, BTDRV_PATCH_ACT, 0, 0x0002e674, 0x53faf44f, 0, NULL,
}; // lc start lsto 5s
#endif

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch18 = {
    18, BTDRV_PATCH_ACT, 0, 0x0002f264, 0xbf00e001, 0, NULL,
}; // close tx seq force set after start IBRT

const uint32_t bes2300p_patch19_ibrt_ins_data_t2[] = {
    0xd01e0fdb, /*6a2c*/
    0x6307f44f, 0xf304fb03, 0x48168f6a, 0x93011810, 0x49154418, 0xfe22f644,
    0x58fb4b14, 0x4301f3c3, 0xf1028f2a, 0xf5024250, 0xf2021205, 0x8f6942fc,
    0x0241eb02, 0x440a9901, 0x600a490d, 0xf44fe010, 0xfb036307, 0x9301f304,
    0x18184806, 0x8f6a4906, 0xfe04f644, 0x58fb4b05, 0x4381f3c3, 0xb9a9f62e,
    0xb9aff62e, 0xd02144fc, 0xc000593c, 0xd0220144, 0xc000005c, /*6a9c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch19 = {
    19,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch19_ibrt_ins_data_t2),
    0x00034d80,
    0xbe54f1d1,
    0xc0006a2c,
    (uint8_t *)bes2300p_patch19_ibrt_ins_data_t2}; // ld_sco_frm_isr

const uint32_t bes2300p_patch20_ibrt_ins_data_t2[] = {
    0xbf002004, /*6aa4*/
    0xfb5cf634, /*6aa8*/
    0xb8c4f618, /*6aac*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch20 = {
    20,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch20_ibrt_ins_data_t2),
    0x0001ec10,
    0xbf48f1e7,
    0xc0006aa4,
    (uint8_t *)bes2300p_patch20_ibrt_ins_data_t2}; // ibrt sco status update

const uint32_t bes2300p_patch21_ibrt_ins_data_t2[] = {
    0xf6414631, 0x4620fc1b, 0xf6064631, 0x4a03faaf,
    0xf4436813, 0x60130380, 0xbdddf633, 0xd0220468,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch21 = {
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch21_ibrt_ins_data_t2),
    0x0003a684,
    0xba16f1cc, // jump a003a684 -> a0206ab4
    0xc0006ab4,
    (uint8_t *)
        bes2300p_patch21_ibrt_ins_data_t2}; // register address at LM layer and
                                            // open fa en register

#if 0
const uint32_t bes2300p_patch22_ibrt_ins_data_t2[] =
{
    0x4681b083,/*6b30*/
    0x4b022200,
    0x2020f843,
    0xbddcf623,
    0xc0006b44,
    0x00000000,
    0x00000000,
    0x00000000,/*6b4c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch22 =
{
    22,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch22_ibrt_ins_data_t2),
    0x0002a6f4,
    0xba1cf1dc,
    0xc0006b30,
    (uint8_t *)bes2300p_patch22_ibrt_ins_data_t2
};//sync clear count
#else
const uint32_t bes2300p_patch22_ibrt_ins_data_t2[] = {
    0x4681b083, 0x4b042200, 0x2020f843, 0x601a4b0d,
    0x601a4b0d, 0xb8b0f623, 0xc000759c,
    0x00000000, // link_no_sync_count
    0x00000000, 0x00000000,
    0x00000018, // link_agc_thd
    0x00000000, // rssi_store_tws
    0x00000006, // rssi_tws_step
    0x00000000, // rssi_store_mobile
    0x00000006, // rssi_mobile_step
    0x0000009c, // rssi_min_value_tws
    0x0000009c, // rssi_min_value_mobile
    0xc00075ac, // rssi_store_tws
    0xc00075b4, // rssi_store_mobile

};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch22 = {
    22,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch22_ibrt_ins_data_t2),
    0x0002a6f4,
    0xbf44f1dc,
    0xc0007580,
    (uint8_t *)bes2300p_patch22_ibrt_ins_data_t2}; // sync clear count
#endif

#if 0
const uint32_t bes2300p_patch23_ibrt_ins_data_t2[] =
{
    0x6020f853,/*6b50*/
    0xf8524a0d,
    0x33013020,
    0x3020f842,
    0x68124a0b,
    0xd90f4293,
    0x4a082300,
    0x3020f842,
    0xb082b402,
    0x92004a08,
    0xf06f6a14,
    0x46290059,
    0x47a02201,
    0xbc02b002,
    0xbce4f623,
    0xc0006b44,
    0xc0006b94,
    0x00000050,
    0xc00041b4,/*6b98*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch23 =
{
    23,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch23_ibrt_ins_data_t2),
    0x0002a550,
    0xbafef1dc,
    0xc0006b50,
    (uint8_t *)bes2300p_patch23_ibrt_ins_data_t2
};////swagc no sync
#elif 0

const uint32_t bes2300p_patch23_ibrt_ins_data_t2[] = {
    0xf6324620, 0xb480fce9, 0xfb07276e, 0x4a1ff704, 0xf837bf00, 0xf4133002,
    0xd12b4f00, 0xf8524a17, 0x33013024, 0x3024f842, 0x68124a15, 0xd8124293,
    0x3084f8d5, 0xf0221af2, 0xf1b14178, 0xd9066f80, 0xf0231b9b, 0x425b4378,
    0xdc042b64, 0xf022e012, 0x2a644278, 0x2300dd0e, 0xf8424a08, 0xb0823024,
    0x92004a09, 0xc020f8d2,
    //    0x0036f06f,
    0x004cf06f, 0x22014621, 0xb00247e0, 0xbf00bc80, 0xbbeff62d, 0xc0006b44,
    0xc0007204, 0x0000003c, 0xc00041b4,
    0xd02111f8, // 720c
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch23 = {
    23,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch23_ibrt_ins_data_t2),
    0x000349d4,
    0xbbd4f1d2,
    0xc0007180,
    (uint8_t *)bes2300p_patch23_ibrt_ins_data_t2}; ////swagc no sync
#else
const uint32_t bes2300p_patch23_ibrt_ins_data_t2[] = {
    0xfb03236e, 0x4a3ef300, 0xf4135a9b, 0xd1764f00, 0xb082b570, 0xf8534b3b,
    0x4a3b4020, 0x3020f852, 0xf8423301, 0x4a393020, 0x42936812, 0xf8d4d812,
    0x1aca3084, 0x4578f022, 0x6f80f1b5, 0x1a5bd906, 0x4378f023, 0x2b64425b,
    0xe04ddc04, 0x4278f022, 0xdd492a64, 0x681b4b2e, 0xd11d4298, 0xf9934b2d,
    0xb93b3000, 0x30acf9b4, 0x20aef894, 0xf3f2fb93, 0x70134a28, 0x78134a27,
    0x78094927, 0xb2db1a5b, 0x4a267013, 0xb25b7812, 0x428bb251, 0x4b21bfbc,
    0x4b20701a, 0xe01c781e, 0xf9934b21, 0xb93b3000, 0x30acf9b4, 0x20aef894,
    0xf3f2fb93, 0x70134a1c, 0x78134a1b, 0x7809491b, 0xb2db1a5b, 0x4a1a7013,
    0xb25b7812, 0x428bb251, 0x4b15bfbc, 0x4b14701a, 0x4601781e, 0x4a0c2300,
    0x3020f842, 0x92004a13, 0xb2706a15, 0x47a82201, 0x30aef894, 0xd9042b95,
    0xf8a42300, 0xf88430ac, 0xb00230ae, 0x4770bd70, 0xd02111f8, 0xc0000998,
    0xc000759c, 0xc00075a8, 0xc0006e2c, 0xc00075ac, 0xc00075b0, 0xc00075bc,
    0xc00075b4, 0xc00075b8, 0xc00075c0, 0xc00041b4, 0xf6324620, 0x4620f9f7,
    0xf7ff4631, 0xbf00ff61, 0xb931f62d};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch23 = {
    23,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch23_ibrt_ins_data_t2),
    0x000349d4,
    0xbec6f1d2,
    0xc0007634,
    (uint8_t *)bes2300p_patch23_ibrt_ins_data_t2}; ////swagc no sync

#endif

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch24 = {
    24, BTDRV_PATCH_ACT, 0, 0x0000fe0c, 0xf029bf00, 0, NULL,
}; // filter tws link lc_sco_ind ke message

const uint32_t bes2300p_patch25_ibrt_ins_data_t2[] = {
    /*6f90*/ 0xbf009904,
    /*6f94*/ 0x4b12b470,
    /*6f98*/ 0x010e5c5a,
    /*6f9c*/ 0x19734d11,
    /*6fa0*/ 0x78244c11,
    /*6fa4*/ 0xd20342a2,
    /*6fa8*/ 0x32015498,
    /*6fac*/ 0xe001b2d2,
    /*6fb0*/ 0x22015570,
    /*6fb4*/ 0x5442480a,
    /*6fb8*/ 0x461ab14c,
    /*6fbc*/ 0x23001919,
    /*6fc0*/ 0x0b01f912,
    /*6fc4*/ 0xbf004403,
    /*6fc8*/ 0xd1f9428a,
    /*6fcc*/ 0x2300e000,
    /*6fd0*/ 0xf0f4fb93,
    /*6fd4*/ 0xbc70b240,
    /*6fd8*/ 0x22019904,
    /*6fdc*/ 0xbb8cf628, // jump a002f6f8
    /*6fe0*/ 0xc0006ff0, // addr of rssi_idx_for_agc
    /*6fe4*/ 0xc0006ff4, // addr of rssi_set_for_agc
    /*6fe8*/ 0xc0006fec, // addr of N
    /*6fec*/ 0x00000005, // N : 1 ~ 16
    /*6ff0*/ 0x00000000, // rssi_idx_for_agc[3]
    /*6ff4*/ 0xbabababa, // rssi_set_for_agc[3*16]
    /*6ff8*/ 0xbabababa,
    /*6ffc*/ 0xbabababa,
    /*7000*/ 0xbabababa,
    /*7004*/ 0xbabababa,
    /*7008*/ 0xbabababa,
    /*700c*/ 0xbabababa,
    /*7010*/ 0xbabababa,
    /*7014*/ 0xbabababa,
    /*7018*/ 0xbabababa,
    /*701c*/ 0xbabababa,
    /*7020*/ 0xbabababa,
    /*7024*/ 0xbabababa,
    /*7028*/ 0xbabababa,
    /*702c*/ 0xbabababa,
}; // 0xc0006f90--->0xc0007074

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch25 = {
    25,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch25_ibrt_ins_data_t2),
    0x0002f6f4,
    0xbc4cf1d7, // jump a002f6f4 -> a0206f90
    0xc0006f90,
    (uint8_t *)bes2300p_patch25_ibrt_ins_data_t2}; // ld_acl_rx() average rssi

#if 0
//ECC patch 1
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch26 =
{
    26,
    BTDRV_PATCH_ACT,
    0,
    0x00034790,
    0xbf00bf00,
    0,
    NULL,
};//ld_acl_sniff_frm_isr ld_get_rx_isr_counter  ignore
#else
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch26 = {
    26, BTDRV_PATCH_ACT, 0, 0x00029d50, 0x3300f443, 0, NULL,
}; // bt_e_scoltcntl_retxnb_setf
#endif

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch27 = {
    27, BTDRV_PATCH_ACT, 0, 0x000236e0, 0xf0172004, 0, NULL,
}; // LC_WAIT_EXCHANGE_SCO_STATUS_CFM

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch28 = {
    28,  BTDRV_PATCH_ACT, 0, 0x00039144, 0x47702001, 0,
    NULL}; // ld_get_sniffer_enter_sniff_flag() forece return 1

#if 0
const uint32_t bes2300p_patch29_ibrt_ins_data_t2[] =
{
    /*6ba0*/ 0xbf0080c6,
    /*6ba4*/ 0x71022200,
    /*6ba8*/ 0xbb2af619, //jump a0020200
};
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch29 =
{
    29,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch29_ibrt_ins_data_t2),
    0x000201f8,
    0xbcd2f1e6, //jump a00201f8 -> a0206ba0
    0xc0006ba0,
    (uint8_t *)bes2300p_patch29_ibrt_ins_data_t2
};//KE_MSG_HANDLER(lc_op_loc_sniff_req, void)
#else

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch29 = {
    29, BTDRV_PATCH_ACT, 0, 0x00020024, 0xe00dbf00, 0, NULL,
}; // detach directly
#endif

const uint32_t bes2300p_patch30_ibrt_ins_data_t2[] = {
    /*6bac*/ 0x46286825,
    /*6bb0*/ 0x303ef895,
    /*6bb4*/ 0xd0012b02,
    /*6bb8*/ 0xbda2f622, // jump a0029700
    /*6bbc*/ 0xbf002300,
    /*6bc0*/ 0x303ef885,
    /*6bc4*/ 0xfea6f622, // call a0029914
    /*6bc8*/ 0xbdc1f622, // jump a002974e
};
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch30 = {
    30,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch30_ibrt_ins_data_t2),
    0x000296fc,
    0xba56f1dd, // jump a00296fc -> a0206bac
    0xc0006bac,
    (uint8_t *)bes2300p_patch30_ibrt_ins_data_t2}; // ld_pscan_frm_cbk

const uint32_t bes2300p_patch31_ibrt_ins_data_t2[] = {
    0x7021f853, 0xb12fb136, 0x017ff000, 0xf8534b06, 0xb93b3021, 0x210c20ff,
    0xfda8f61b, 0xbf002004, 0xbffcf62b, 0xbf76f62b, 0xc0005cd8,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch31 = {
    31,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch31_ibrt_ins_data_t2),
    0x0003328c,
    0xb876f1d4, // jump a003328c -> a020737c
    0xc000737c,
    (uint8_t *)bes2300p_patch31_ibrt_ins_data_t2}; // protect null ptr when acl
                                                   // tws switch

const uint32_t bes2300p_patch32_ibrt_ins_data_t2[] = {
    /*6be8*/ 0x46514620,
    /*6bec*/ 0x4b09464a,
    /*6bf0*/ 0x3022f853,
    /*6bf4*/ 0xd10b2901,
    /*6bf8*/ 0x0184f103,
    /*6bfc*/ 0xbf002206,
    /*6c00*/ 0xfd34f644,
    /*6c04*/ 0xbf00b920,
    /*6c08*/ 0x0802f05f,
    /*6c0c*/ 0xba4af641, // jump a00480a4
    /*6c10*/ 0xbaf6f641, // jump a0048200
    /*6c14*/ 0xc0005cd8,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch32 = {
    32,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch32_ibrt_ins_data_t2),
    0x00048008,
    0xbdeef1be, // jump a0048008 -> a0206be8
    0xc0006be8,
    (uint8_t *)bes2300p_patch32_ibrt_ins_data_t2}; // hci_evt_filter_con_check

const uint32_t bes2300p_patch33_ibrt_ins_data_t2[] = {
    /*6c1c*/ 0x0024f855,
    /*6c20*/ 0xbf003038,
    /*6c24*/ 0xfd9ef643,
    /*6c28*/ 0x0024f855,
    /*6c2c*/ 0xbe9ef60c,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch33 = {
    33,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch33_ibrt_ins_data_t2),
    0x00013968,
    0xb958f1f3, // jump a0013968 -> a0206c1c
    0xc0006c1c,
    (uint8_t *)bes2300p_patch33_ibrt_ins_data_t2}; // ea_interval_remove

const uint32_t bes2300p_patch34_ibrt_ins_data_t2[] = {
    /*6c30*/ 0xf632d108,
    /*6c34*/ 0x2100f9e3,
    /*6c38*/ 0xf802f634,
    /*6c3c*/ 0xfa8cf634,
    /*6c40*/ 0xbb8bf5fb,
    /*6c44*/ 0xbbc3f5fb,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch34 = {
    34,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch34_ibrt_ins_data_t2),
    0x00002354,
    0xbc6cf204, // jump a0002354 -> a0206c30
    0xc0006c30,
    (uint8_t *)
        bes2300p_patch34_ibrt_ins_data_t2}; // close fastack when stop ibrt

const uint32_t bes2300p_patch35_ibrt_ins_data_t2[] = {
    /*6c48*/ 0x4b0a0a02,
    /*6c4c*/ 0x3022f853,
    /*6c50*/ 0xb500b173,
    /*6c54*/ 0x4601b083,
    /*6c58*/ 0xf88d230f,
    /*6c5c*/ 0x23133004,
    /*6c60*/ 0x3005f88d,
    /*6c64*/ 0xf616a801,
    /*6c68*/ 0xb003fbc1,
    /*6c6c*/ 0xfb04f85d,
    /*6c70*/ 0xbf004770,
    /*6c74*/ 0xc0005cd8,
    /*6c78*/ 0xbf000230,
    /*6c7c*/ 0x0001f040,
    /*6c80*/ 0xffe2f7ff,
    /*6c84*/ 0xbc5df634};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch35 = {
    35,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch35_ibrt_ins_data_t2),
    0x0003b538,
    0xbb9ef1cb, // jump a003b538 -> a0206c78
    0xc0006c48,
    (uint8_t *)
        bes2300p_patch35_ibrt_ins_data_t2}; // snoop disconnect lc_release

#if 0
//ECC patch 2
const uint32_t bes2300p_patch36_ibrt_ins_data_t2[] =
{
    /*6c88*/ 0x701c4b03,
    /*6c8c*/ 0x78134a01,
    /*6c90*/ 0xbcfcf62d,
    /*6c94*/ 0xc0000971,
    /*6c98*/ 0xc000032a,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch36 =
{
    36,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch36_ibrt_ins_data_t2),
    0x00034688,
    0xbafef1d2, // jump a0034688 -> a0206c88
    0xc0006c88,
    (uint8_t *)bes2300p_patch36_ibrt_ins_data_t2
};//ld_acl_rx_isr set_ecc_recheck_link_id
#else
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch36 = {
    36, BTDRV_PATCH_ACT, 0, 0x0002ddc0, 0x4a07bf00, 0, NULL,
}; // 1st ibrt ok, ignore sniffer role
#endif

const uint32_t bes2300p_patch37_ibrt_ins_data_t2[] = {
    0x4605b538,                                     /*6ca0*/
    0xf643460c, 0x6a00fc79, 0xd2034285, 0x28061b40, /*6cb0*/
    0x2402bf98, 0xb9fdf627,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch37 = {
    37,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch37_ibrt_ins_data_t2),
    0x0002e0b0,
    0xbdf6f1d8,
    0xc0006ca0,
    (uint8_t *)bes2300p_patch37_ibrt_ins_data_t2}; // decrease retx

#if 0
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch38 =
{
    38,
    BTDRV_PATCH_ACT,
    0,
    0x00000878,
    0x2300bf00,
    0,
    NULL
};//SLEEP EN
#else
const uint32_t bes2300p_patch38_ibrt_ins_data_t2[] = {
    0x4604b510,                                     /*6cc0*/
    0xf6104608, 0x2c03fd25, 0x2803d001, 0x200cd104, /*6cd0*/
    0xfa4ef61c, 0xbd102001, 0xbd102000, 0xfd18f610, /*6ce0*/
    0xf1044605, 0x46280145, 0xffe8f7ff, 0xd1012800, /*6cf0*/
    0xbeb7f633, 0xbf19f633,                         /*6cf8*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch38 = {
    38,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch38_ibrt_ins_data_t2),
    0x0003aa60,
    0xb93ef1cc, // jump a003aa60 -> a0206cc0
    0xc0006cc0,
    (uint8_t *)bes2300p_patch38_ibrt_ins_data_t2}; // set env
#endif

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch39 = {
    39,  BTDRV_PATCH_ACT, 0, 0x0003b450, 0xbf00bd08, 0,
    NULL}; // do not send ld_sniffer_connect_sco_forward

const uint32_t bes2300p_patch40_ibrt_ins_data_t2[] = {
    0x2a140c92, 0x2214bf28, 0xbf004691, 0xbc44f623, /*6d40*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch40 = {
    40,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch40_ibrt_ins_data_t2),
    0x0002a5c4,
    0xbbb4f1dc,
    0xc0006d30,
    (uint8_t *)bes2300p_patch40_ibrt_ins_data_t2}; // max_drift limit

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch41 = {
    41, BTDRV_PATCH_ACT, 0, 0x0003acd8, 0xbf00e005, 0, NULL}; /// acl tx silence

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch42 = {
    42,  BTDRV_PATCH_ACT, 0, 0x00033668, 0xbf00e001, 0,
    NULL}; /// do not clear acl env during sco stop

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch43 = {
    43,  BTDRV_PATCH_ACT, 0, 0x0002a62c, 0xe001021a, 0,
    NULL}; // rx window adjust for start ibrt

#if defined(SYNC_BT_CTLR_PROFILE)
const uint32_t bes2300p_patch44_ibrt_ins_data_t2[] = {
    /*6d24*/ 0xbf004628,
    /*6d28*/ 0xfff8f633,
    /*6d2c*/ 0xb8f7f611,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch44 = {
    44,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch44_ibrt_ins_data_t2),
    0x00017f10,
    0xbf08f1ee, // jump a0017f10 -> a0206d24
    0xc0006d24,
    (uint8_t *)bes2300p_patch44_ibrt_ins_data_t2}; // send profile via BLE
#else

const uint32_t bes2300p_patch44_ibrt_ins_data_t2[] = {
    0xd8092b01,                                     /*70d0*/
    0xfb03236e, 0x4a04f308, 0xf4135a9b, 0xd1016f00, /*70e0*/
    0xb919f629, 0xb9aef629, 0xd0211200,             /*70ec*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch44 = {
    44,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch44_ibrt_ins_data_t2),
    0x00030314,
    0xbedcf1d6,
    0xc00070d0,
    (uint8_t *)bes2300p_patch44_ibrt_ins_data_t2}; // lmp tx for tx desc error
#endif

#if 0
const uint32_t bes2300p_patch45_ibrt_ins_data_t2[] =
{
    0x20aef89b,/*6d50*/
    0xd1052aa0,
    0xf88b2200,
    0xf8ab20ae,
    0xe01320ac,/*6d60*/
    0x68194b0b,
    0xf3f1fbb2,
    0x2313fb01,
    0x4b07b963,/*6d70*/
    0x00acf9bb,
    0x6a1e9300,
    0xfb90bf00,
    0x9904f0f2,/*6d80*/
    0x2201b240,
    0x47b02300,
    0xbcb6f628,
    0xc00041b4,/*6d90*/
    0xc0006d98,
    0x0000000a,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch45 =
{
    45,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_patch45_ibrt_ins_data_t2),
    0x0002f6ec,
    0xbb30f1d7,
    0xc0006d50,
    (uint8_t *)bes2300p_patch45_ibrt_ins_data_t2
};//RSSI AVERAGE 10
#else
const uint32_t bes2300p_patch45_ibrt_ins_data_t2[] = {
    0x4a033304,                                     /*6930*/
    0xbf008153, 0x12c4f640, 0xb9bdf632, 0xc00061b8, /*6940*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch45 = {
    45,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch45_ibrt_ins_data_t2),
    0x00038cb0,
    0xbe3ef1cd,
    0xc0006930,
    (uint8_t *)bes2300p_patch45_ibrt_ins_data_t2}; // pscan duration
#endif

const uint32_t bes2300p_patch46_ibrt_ins_data_t2[] = {
    /*6d9c*/ 0x4b179a0b,
    /*6da0*/ 0xd023429a,
    /*6da4*/ 0x20b5f894,
    /*6da8*/ 0xb9fab2d2,
    /*6dac*/ 0xf9b4b22e,
    /*6db0*/ 0x1b9b3098,
    /*6db4*/ 0xf9b24a0f,
    /*6db8*/ 0x42932000,
    /*6dbc*/ 0x4251da05,
    /*6dc0*/ 0xdd02428b,
    /*6dc4*/ 0x7088f8d4,
    /*6dc8*/ 0x4293e00f,
    /*6dcc*/ 0xf8d4dd05,
    /*6dd0*/ 0x3f017088,
    /*6dd4*/ 0x4778f027,
    /*6dd8*/ 0x4252e007,
    /*6ddc*/ 0xda044293,
    /*6de0*/ 0x7088f8d4,
    /*6de4*/ 0xf0273701,
    /*6de8*/ 0xbf004778,
    /*6dec*/ 0x708cf8c4,
    /*6df0*/ 0xbce4f623, // jump a002a7bc
    /*6df4*/ 0xc0006df8,
    /*6df8*/ 0x00000032, // 50
    /*6dfc*/ 0xa003477b, // ld_acl_sniff_frm_isr lr
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch46 = {
    46,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch46_ibrt_ins_data_t2),
    0x0002a7b8,
    0xbaf0f1dc, // jump a002a7b8 -> a0206d9c
    0xc0006d9c,
    (uint8_t *)bes2300p_patch46_ibrt_ins_data_t2}; // ld_acl_rx_sync clk_off cal
                                                   // active mode

const uint32_t bes2300p_patch47_ibrt_ins_data_t2[] = {
    0xd10b2800,                                     /*6e00*/
    0x0f00f1ba, 0x4a07d10a, 0x2aff7812, 0x4615d006, /*6e10*/
    0x30b2f894, 0xd10342ab, 0xb8aff625, 0xb974f625, /*6e20*/
    0xb952f625, 0xc0006e2c, 0x000000ff,             /*6e28*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch47 = {
    47,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch47_ibrt_ins_data_t2),
    0x0002bf78,
    0xbf42f1da,
    0xc0006e00,
    (uint8_t *)bes2300p_patch47_ibrt_ins_data_t2}; /// adjust slot share for tws
                                                   /// without ibrt

const uint32_t bes2300p_patch48_ibrt_ins_data_t2[] = {
    0x781b4b13, 0xd0212bff, 0x4604b510, 0xf946f632, 0x4b0fb1d8, 0x4b0f781a,
    0x3022f853, 0x0f28f113, 0xf893d013, 0xeb0330c1, 0xb2db0342, 0x0383eb03,
    0x4a09005b, 0xf3c35a9b, 0x2b5303c9, 0x7ea2d905, 0x441368e3, 0x4378f023,
    0xbd1060e3, 0xbf004770, 0xc0006e2c, 0xc0000998, 0xd0211632, 0x0028f104,
    0xffcef7ff, 0x60a36b63, 0xb9fcf627,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch48 = {
    48,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch48_ibrt_ins_data_t2),
    0x0002e290,
    0xbdfcf1d8,
    0xc0006e30,
    (uint8_t *)bes2300p_patch48_ibrt_ins_data_t2}; // big packet send in sco v1

const uint32_t bes2300p_patch49_ibrt_ins_data_t2[] = {
    /*6ec0*/ 0xf6232100,
    /*6ec4*/ 0xf643feaf,
    /*6ec8*/ 0x2300fb69,
    /*6ecc*/ 0xbf006243,
    /*6ed0*/ 0xbcf4f626,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch49 = {
    49,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch49_ibrt_ins_data_t2),
    0x0002d88c,
    0xbb18f1d9, // jump a0018188 -> a0206978
    0xc0006ec0,
    (uint8_t *)
        bes2300p_patch49_ibrt_ins_data_t2}; /// role switch fail to disconnect

#if 0
const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch50 =
{
    50,
    BTDRV_PATCH_INACT,
    0,
    0x0003ea94,
    0xbf00e04a,
    0,
    NULL
};//ble channel map disable
#elif 0
const uint32_t bes2300p_patch50_ibrt_ins_data_t2[] = {
    0xf8948263, 0xbf0030b3, 0xbf00bf00, 0xbf00bf00, 0xbf002304, 0xb8b8f625,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch50 = {
    50,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch50_ibrt_ins_data_t2),
    0x0002c024,
    0xbf3cf1da,
    0xc0006ea0,
    (uint8_t *)bes2300p_patch50_ibrt_ins_data_t2};
#else
const uint32_t bes2300p_patch50_ibrt_ins_data_t2[] = {
    0x75a32312,                                     /*6ed8*/
    0x001bf898, 0x1088f8d1, 0x300cf8d8, 0xf023440b, /*6ee8*/
    0xea834378, 0xf8986380, 0x1a5b1019, 0x101af898, /*6ef8*/
    0xf3f1fbb3, 0xfbb3b2db, 0xfb02f1f2, 0x1ad23311, /*6f08*/
    0xf894b2d6, 0xb91330b3, 0xf8842301, 0xf5fc30bb, /*6f18*/
    0xf890fa05, 0xf5c33025, 0x3304631c, 0x23048263, /*6f28*/
    0xb87cf625,                                     /*6f2c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch50 = {
    50,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch50_ibrt_ins_data_t2),
    0x0002bfc4,
    0xbf88f1da,
    0xc0006ed8,
    (uint8_t *)
        bes2300p_patch50_ibrt_ins_data_t2}; ////ibrt connect slot adjust in sco
#endif

const uint32_t bes2300p_patch51_ibrt_ins_data_t2[] = {
    0x490a60e3,                                     /*6f30*/
    0x428f6809, 0x4620d10c, 0x22002100, 0xf914f625, /*6f40*/
    0x429d68a3, 0x60a5bf38, 0x134cf640, 0x3012f8a4, /*6f50*/
    0xbf004620, 0xbc20f624, 0xc0006e2c,             /*6f5c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch51 = {
    51,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch51_ibrt_ins_data_t2),
    0x0002b798,
    0xbbcaf1db,
    0xc0006f30,
    (uint8_t *)bes2300p_patch51_ibrt_ins_data_t2}; // sniff trans mode timing

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch52 = {
    52,         BTDRV_PATCH_ACT,
    0,          0x0001bf54,
    0xbff2f7ff,        // jump a001bf54 -> a001bf3c
    0,          NULL}; // LMP_MSG_HANDLER(switch_req) LC_CONNECTED

const uint32_t bes2300p_patch53_ibrt_ins_data_t2[] = {
    /*6f74*/ 0xf632b140,
    /*6f78*/ 0x42b0f841,
    /*6f7c*/ 0xf632d104,
    /*6f80*/ 0x2801f82b,
    /*6f84*/ 0xbf00d002,
    /*6f88*/ 0xbea8f619,
    /*6f8c*/ 0xbf08f619,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch53 = {
    53,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch53_ibrt_ins_data_t2),
    0x00020cd8,
    0xb94cf1e6, // jump a0020cd8 -> a0206f74
    0xc0006f74,
    (uint8_t *)bes2300p_patch53_ibrt_ins_data_t2}; // disconnect cmp

#if 0
const uint32_t bes2300p_patch54_ibrt_ins_data_t2[] =
{
    0xb3384606,/*6f90*/
    0x0a0388a0,
    0xd1212b05,
    0xf1034b14,
    0xb2c10528,/*6fa0*/
    0xe044f8df,
    0x428a781a,
    0x785ad114,
    0x0f01f012,/*6fb0*/
    0x88e7d006,
    0x2f020a3f,
    0xf85ed814,
    0xb18f7027,/*6fc0*/
    0x0f02f012,
    0x8922d006,
    0x2a020a12,
    0xf85ed80a,/*6fe0*/
    0xb13a2022,
    0x42ab3302,
    0xbf00d1e4,
    0xb8e5f5fa,/*6ff0*/
    0xb8dcf5fa,
    0xb8ecf5fa,
    0xc0005cd8,
    0xc0006ff4,/*7000*/
    0x0327032a,
    0x010e0326,
    0x0000010f,
    0x00000000,/*7010*/
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,/*7020*/
    0x00000000,
    0x00000000,/*7024*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch54 =
{
    54,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch54_ibrt_ins_data_t2),
    0x0000119c,
    0xbef8f205,
    0xc0006f90,
    (uint8_t *)bes2300p_patch54_ibrt_ins_data_t2
};//msg filter
#else
const uint32_t bes2300p_patch54_ibrt_ins_data_t2[] = {
    0x8885b5f8, // 73cc
    0x2b050a2b, 0x4b49d121, 0x0428f103, 0x4f48b2e9, 0x428a781a, 0x785ad116,
    0x0f01f012, 0x88c6d007, 0x2e020a36, 0xf857d86b, 0x2e006026, 0xf012d069,
    0xd0070f02, 0x0a128902, 0xd8642a02, 0x2022f857, 0xd0622a00, 0x42a33302,
    0xf240d1e2, 0x429d5335, 0x4605d15d, 0x08637b44, 0xd1142b04, 0xf5f988c0,
    0x2832ffb5, 0xf240d155, 0x88e95006, 0xfb2ef5f9, 0x0a0288e8, 0xf8534b2e,
    0xf8933022, 0xf5f9135e, 0x2401ff39, 0x2b18e050, 0x88c0d126, 0xff9ef5f9,
    0xd1402836, 0x02fef004, 0x0a3688ee, 0xf8534b24, 0xf8844026, 0xb2f62044,
    0x21184630, 0xff1cf61c, 0x4040f894, 0xd1302c01, 0xf6294630, 0x88e8fa29,
    0xf800f60f, 0x0a0288e8, 0xf8534b19, 0xf8933022, 0xf5f9135e, 0xe027ff0f,
    0xd1202b7e, 0x3b097b83, 0x2b01b2db, 0xf631d81d, 0x7a83fd77, 0xd81a2b02,
    0xf8524a0f, 0xfab44023, 0x0964f484, 0x2401e014, 0x2401e012, 0x2401e010,
    0x2401e00e, 0x2400e00c, 0x2400e00a, 0x2400e008, 0x2401e006, 0x2400e004,
    0x2400e002, 0x2401e000, 0xbdf84620, 0xa0207520, 0xc0005cd8, 0xb1484606,
    0xf7ff4620, 0x2801ff5f, 0xbf00d102, 0xbe56f5f9, 0xbe49f5f9, 0xbe40f5f9,
    0x0327032a, 0x010e0326, 0x0000010f, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch54 = {
    54,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch54_ibrt_ins_data_t2),
    0x0000119c,
    0xb9b2f206,
    0xc00073cc,
    (uint8_t *)bes2300p_patch54_ibrt_ins_data_t2}; // msg filter

#endif

const uint32_t bes2300p_patch55_ibrt_ins_data_t2[] = {
    0x49079804,                                     /*70a4*/
    0x42886809, 0xf632d103, 0x2800f86b, 0x3301d004, /*70b4*/
    0xf88bb2db, 0xbf0030bb, 0xbaf4f628, 0xc0006e2c, /*70c4*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ibrt_ins_patch55 = {
    55,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_patch55_ibrt_ins_data_t2),
    0x0002f6a4,
    0xbcfef1d7,
    0xc00070a4,
    (uint8_t *)bes2300p_patch55_ibrt_ins_data_t2}; // tws link dont rx traffic++

/////2300p t2 ibrt patch (chip id =1)
static const uint32_t best2300p_t2_ibrt_ins_patch_config[] = {
    56,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch0,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch1,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch2,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch3,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch4,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch5,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch6,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch7,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch8,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch9,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch10,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch11,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch12,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch13,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch14,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch15,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch16,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch17,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch18,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch19,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch20,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch21,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch22,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch23,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch24,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch25,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch26,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch27,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch28,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch29,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch30,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch31,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch32,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch33,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch34,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch35,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch36,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch37,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch38,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch39,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch40,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch41,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch42,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch43,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch44,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch45,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch46,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch47,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch48,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch49,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch50,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch51,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch52,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch53,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch54,
    (uint32_t)&bes2300p_t2_ibrt_ins_patch55,
};
#endif
/***************************************************************************
 *
 * instruction patch Information
 *
 * BT ROM Chip Version:1302 T1
 *
 * [IBRT]
 * patch  7: 0xC0006a50--->0xC0006a78
 * patch  8: 0xC0006a90--->0xC0006a9C
 * patch  9: in used   check ble conflict with acl
 * patch 10: 0xC0006AA0--->0xC0006AC8
 * patch 11: 0xC0006AD0--->0xC0006AEC
 * patch 12: 0xC0006AF0--->0xC0006B04
 * patch 13: 0xC0006B08--->0xC0006B18
 * patch 14: in use
 * patch 15: 0xC0006B74--->0xC0006B80
 * patch 16: in used :check afh mismatch 1
 * patch 17: in used :check afh mismatch 2
 * patch 18: 0xC0006B88--->0xC006C14    fix acl interval in sco
 * patch 19: in used
 * patch 20: in used
 * patch 21: 0xC0006C78--->0xC0006C88
 * patch 22: 0xC0006810--->0xC0006878
 * patch 23: 0xC0006880--->0xC0006890
 * patch 24: 0xC0006C90--->0xC000689C
 * patch 25: in used:ld_acl_start t_poll
 * patch 26: 0xC0006CA0--->0xC0006CC8:  unused
 * patch 27: in used: ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED
 * patch 28: in used: 0xC0006CF8--->0xC0006D0C
 * patch 29: in used: 0xC0006D14--->0xC0006D30
 ****************************************************************************/

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch0 = {
    0,   BTDRV_PATCH_ACT, 0, 0x00025ef4, 0xbf00bf00, 0,
    NULL}; /// remove the memset of ldenv

const uint32_t bes2300p_t1_patch_0a_ins_data[] = {
    /*6810*/ 0x681b4b0e,
    /*6814*/ 0x4b0eb1ab,
    /*6818*/ 0x31cbf893,
    /*681c*/ 0x4a0d011b,
    /*6820*/ 0xf3c35a9b,
    /*6824*/ 0x4a0c2340,
    /*6828*/ 0x2a025c12,
    /*682c*/ 0x4a0ad103,
    /*6830*/ 0x20005413,
    /*6834*/ 0x42934770,
    /*6838*/ 0x4a07d005,
    /*683c*/ 0x20005413,
    /*6840*/ 0x20004770,
    /*6844*/ 0x20014770,
    /*6848*/ 0xbf004770,
    /*684c*/ 0xc000685c,
    /*6850*/ 0xc0005d34,
    /*6854*/ 0xd02115f2,
    /*6858*/ 0xc0006860,
    /*685c*/ 0x00000001, // sw_seqn_filter_en
    /*6860*/ 0x00020202, // link_id_seq
    /*6864*/ 0x9805bf00,
    /*6868*/ 0xffd2f7ff, // call a0206810
    /*686c*/ 0x2019b910,
    /*6870*/ 0xbd62f628, // jump a002f338
    /*6874*/ 0x22802107,
    /*6878*/ 0xbe22f628, // jump a002f4c0
};                       // ld_acl_rx ld_sw_seqn_filter

const uint32_t bes2300p_t1_patch_0b_ins_data[] = {
    /*6880*/ 0x328af885,
    /*6884*/ 0x22024b02,
    /*6888*/ 0xbf00559a,
    /*688c*/ 0xbe90f614, // jump a001b5b0
    /*6890*/ 0xc0006860, // link_id_seq
};                       // lc_rsw_end_ind link_id_seq[link_id]=2

const uint32_t bes2300p_t1_patch_1_ins_data[] = {
    0x2c021b64, 0x2009d101, 0x3402e002, 0xbc4af63c, 0xbca6f63c,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_1_ins_data),
    0x000431a0,
    0xbbaef1c3,
    0xc0006900,
    (uint8_t *)bes2300p_t1_patch_1_ins_data}; // for ble and sco:
                                              // OVERLAP_RESERVE2 in
                                              // lld_check_conflict_with_ble

const uint32_t bes2300p_t1_patch_2_ins_data[] = {
    0x781b4b05, 0xd1032b09, 0x4b032200, 0x2704701a, 0x60eb1be3, 0xb8ecf627,
    0xc000693c, ////prev_conflict_type_record
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch2 = {
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_2_ins_data),
    0x0002db0c,
    0xbf08f1d8,
    0xc0006920,
    (uint8_t *)bes2300p_t1_patch_2_ins_data}; // for ble and sco:
                                              // prev_conflict_type_record in
                                              // ld_sco_avoid_ble_connect

const uint32_t bes2300p_t1_patch_3_ins_data[] = {
    0xd1084298, 0x3047f894, 0x92004a0e, 0x22024631, 0xfd42f61f, 0x4b0ce014,
    0xb143781b, 0x3047f894, 0x92004a08, 0x22024631, 0xfd36f61f, 0xf894e008,
    0x4a043047, 0x46319200, 0xbf002204, 0xfd2cf61f, 0xbaa2f62b, 0xa002fce5,
    0xc000699c, /// ble_sco_need_move_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_3_ins_data),
    0x00031ec4,
    0xbd44f1d4,
    0xc0006950,
    (uint8_t *)bes2300p_t1_patch_3_ins_data}; // for ble and sco:
                                              // ld_fm_prog_push
                                              // LD_FRAME_TYPE_ESCO when
                                              // ble_sco_need_move_flag in
                                              // ld_sco_evt_start_cbk

const uint32_t bes2300p_t1_patch_4_ins_data[] = {
    0x781b4b18, 0x2200b113, 0x701a4b16, 0x781b4b18, 0xd0232b00,
    0x4b162201, 0xbf00701a, 0xb1e84680, 0x781b4b11, 0xd1192b00,
    0x0304f1a8, 0x2b01b2db, 0xf1b8d903, 0xd1130f09, 0x2201e00c,
    0x701a4b09, 0x0f09f1b8, 0x2209d103, 0x701a4b08, 0x68eb2704,
    0xb8a6f627, 0x4b032201, 0xe7f4701a, 0xb8c2f627, 0xb8aff627,
    0xc000699c, /// ble_sco_need_move_flag
    0xc000099f,
    0xc000693c, /// prev_conflict_type_record
    0xc0006a18, /// sco_find_sync_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch4 = {
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_4_ins_data),
    0x0002db30,
    0xbf38f1d8,
    0xc00069a4,
    (uint8_t *)bes2300p_t1_patch_4_ins_data}; // for ble and sco:
                                              // ble_sco_need_move_flag =
                                              // 1,need_move=4 in
                                              // ld_sco_avoid_ble_connect

const uint32_t bes2300p_t1_patch_5_ins_data[] = {
    0xfbb24a02,
    0xf5f9f3f3,
    0xbf00bec6,
    0x9af8da00,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch5 = {
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_5_ins_data),
    0x00000650,
    0xb936f206,
    0xc00068c0,
    (uint8_t *)bes2300p_t1_patch_5_ins_data}; // lpclk

const uint32_t bes2300p_t1_patch_6_ins_data[] = {
    0xd00d2b00, 0xf8384b07, 0xf4133003, 0xd0024f00, 0x4b052201,
    0xf899701a, 0x2b0130b3, 0xba5af629, 0xbaccf629, 0xd02111f8,
    0xc0006a18, ////sco_find_sync_flag
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch6 = {
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_6_ins_data),
    0x0002feec,
    0xbd98f1d6,
    0xc0006a20,
    (uint8_t *)bes2300p_t1_patch_6_ins_data}; /// for ble and sco:
                                              /// sco_find_sync_flag = 1 in
                                              /// ld_sco_frm_isr

const uint32_t bes2300p_t1_patch_7_ins_data[] = {
    0xd10f2f03, 0xfd16f642, 0x4008f8d8, 0x46216940, 0xf6242220, 0xb128fff5,
    0x0320f104, 0x4378f023, 0x3008f8c8, 0xbf002000, 0xbc7df625,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch7 = {
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_7_ins_data),
    0x0002c32c,
    0xbb90f1da,
    0xc0006A50,
    (uint8_t *)bes2300p_t1_patch_7_ins_data}; // ble avoid role switch

const uint32_t bes2300p_t1_patch_8_ins_data[] = {
    0xf855b2e6,
    0xb1091b04,
    0xbc71f610,
    0xbc78f610,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch8 = {
    8,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_8_ins_data),
    0x00017378,
    0xbb8af1ef,
    0xc0006A90,
    (uint8_t *)bes2300p_t1_patch_8_ins_data}; // lc_check_bt_device_conected

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch9 = {
    9,   BTDRV_PATCH_ACT, 0, 0x00043238, 0xbf342c09, 0,
    NULL}; // check conflict 8 slot 1

const uint32_t bes2300p_t1_patch_10_ins_data[] = {
    0xf8934b07, 0x98053078, 0xd1074298, 0xd0052b03, 0x681b4b04, 0x2201b913,
    0x601a4b02, 0xbc3ff628, 0xc0006358, 0xc0006AC8, 0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch10 = {
    10,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_10_ins_data),
    0x0002f338,
    0xbbb2f1d7,
    0xc0006AA0,
    (uint8_t *)bes2300p_t1_patch_10_ins_data}; // ibrt snoop success

const uint32_t bes2300p_t1_patch_11_ins_data[] = {
    0xb149d208, 0x681b4b05, 0x2200b133, 0x601a4b03,
    0xbce4f632, 0xbcf2f632, 0xbcfff632, 0xc0006AC8,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch11 = {
    11,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_11_ins_data),
    0x000394a8,
    0xbb12f1cd,
    0xc0006AD0,
    (uint8_t *)bes2300p_t1_patch_11_ins_data}; // ibrt snoop success 2

const uint32_t bes2300p_t1_patch_12_ins_data[] = {
    0x46032102, 0xf9b4f633, 0xbf004618, 0x22012105, 0xfa28f61d, 0xba4ff633,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch12 = {
    12,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_12_ins_data),
    0x00039fa0,
    0xbda6f1cc,
    0xc0006AF0,
    (uint8_t *)bes2300p_t1_patch_12_ins_data}; // hack tws switch tx seqn error

const uint32_t bes2300p_t1_patch_13_ins_data[] = {
    0xfc6ef631, 0xfcbaf631, 0xbf004620, 0xfcc2f631, 0xbfb2f62a,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch13 = {
    13,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_13_ins_data),
    0x00031a7c,
    0xb844f1d5,
    0xc0006B08,
    (uint8_t *)bes2300p_t1_patch_13_ins_data}; // lmp filter enable

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch14 = {
    14, BTDRV_PATCH_ACT, 0, 0x000388a8, 0xbf00e001, 0, NULL}; // open afh update

const uint32_t bes2300p_t1_patch_15_ins_data[] = {
    0xfbd0f631,
    0x20004601,
    0xfbb8f633,
    0xba9ef618,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch15 = {
    15,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_15_ins_data),
    0x0001f0bc,
    0xbd5af1e7,
    0xc0006B74,
    (uint8_t *)bes2300p_t1_patch_15_ins_data}; // ibrt slave stop sco

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch16 = {
    16,
#if defined(IBRT)
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    0,
    0x00038a78,
    0x7a22d206,
    0,
    NULL,
}; // check afh mismatch 1

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch17 = {
    17,
#if defined(IBRT)
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    0,
    0x00038a88,
    0xbf00e00e,
    0,
    NULL,
}; // check afh mismatch 2

const uint32_t bes2300p_t1_patch_18_ins_data[] = {
    0x4606b570, 0x7e8d460c, 0xf5f5fb92, 0x2114b2e8, 0x68e175b1, 0xf023440b,
    0x7ee14378, 0x6381ea83, 0x1a5b7e61, 0xfbb37ea1, 0xb2dbf3f1, 0xf1f0fbb3,
    0x3311fb00, 0xb2c31ac0, 0xd8042a3c, 0x622ef640, 0x22048272, 0xf640e003,
    0x8272124c, 0x7ea02206, 0xfb0068e1, 0x44101003, 0x4078f020, 0x210060b0,
    0xfab4f63c, 0xd9072806, 0xb2ed7ea3, 0xfb0568b2, 0xf0252503, 0x60b54578,
    0xbf00bd70, 0x46514620, 0x2042f8b8, 0x3088f8d3, 0xffbaf7ff, 0xbfcbf624,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch18 = {
    18,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_18_ins_data),
    0x0002bad8,
    0xb894f1db, // jump a002bad8 -> A0206C04
    0xc0006B88,
    (uint8_t *)bes2300p_t1_patch_18_ins_data};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch19 = {
    19, BTDRV_PATCH_ACT, 0, 0x00039d78, 0xd13b2902, 0, NULL}; // ibrt switch op

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch20 = {
    20, BTDRV_PATCH_ACT, 0, 0x00030240, 0xe0022000, 0, NULL}; // acl end assert

const uint32_t bes2300p_t1_patch_21_ins_data[] = {
    0x781b4b03, 0xd1012b01, 0xbc5cf633, 0xbc63f633, 0xc0006358,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch21 = {
    21,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_21_ins_data),
    0x0003a528,
    0xbba6f1cc,
    0xc0006C78,
    (uint8_t *)bes2300p_t1_patch_21_ins_data}; // snoop disconnect

// __SW_SEQ_FILTER__ - add software seqn filter to protect recive identical
// repeating packages
#if defined(IBRT)
#define __SW_SEQ_FILTER__
#endif
const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch22 = {
    22,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_INACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes2300p_t1_patch_0a_ins_data),
    0x0002f334,
    0xba96f1d7, // jump a002f334 -> a0206864
    0xc0006810,
    (uint8_t *)bes2300p_t1_patch_0a_ins_data};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch23 = {
    23,
#ifdef __SW_SEQ_FILTER__
    BTDRV_PATCH_INACT,
#else
    BTDRV_PATCH_INACT,
#endif
    sizeof(bes2300p_t1_patch_0b_ins_data),
    0x0001b5ac,
    0xb968f1eb, // jump a001b5ac -> a0206880
    0xc0006880,
    (uint8_t *)
        bes2300p_t1_patch_0b_ins_data}; // lc_rsw_end_ind link_id_seq[link_id]=2

const uint32_t bes2300p_t1_patch_24_ins_data[] = {
    0x0f01f1b8,
    0x380ebf0c,
    0xb280301c,
    0xbb30f632,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch24 = {
    24,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_24_ins_data),
    0x000392d8,
    0xbcdaf1cd,
    0xc0006c90,
    (uint8_t *)bes2300p_t1_patch_24_ins_data}; // start snoop using bitoff

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch25 = {
    25,
#if defined(IBRT)
    BTDRV_PATCH_ACT,
#else
    BTDRV_PATCH_INACT,
#endif
    0,
    0x0002e09c,
    0xf8a6230a,
    0,
    NULL,
}; // ld_acl_start t_poll

const uint32_t bes2300p_t1_patch_26_ins_data[] = {
    0x5040f898, 0xd80d2d14, 0xfbecf642, 0x690068a6, 0xb2ea4631, 0xfeccf624,
    0xf106b120, 0xf0230364, 0x60a34378, 0xfbdef642, 0xbfb2f624,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch26 = {
    26,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_26_ins_data),
    0x0002bc2c,
    0xb838f1db,
    0xc0006CA0,
    (uint8_t *)bes2300p_t1_patch_26_ins_data}; // mobile link avoid tws link
                                               // when send profile

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch27 = {
    27, BTDRV_PATCH_ACT, 0, 0x0003890c, 0xbf00e008, 0, NULL,
}; // ld_sniffer_update_status: SNIFFER_EVT_SCO_CONNECTED

const uint32_t bes2300p_t1_patch_28_ins_data[] = {
    0xfa58f63f, 0x4b034a02, 0xbf00601a, 0xbb66f63f, 0xffff0009, 0xd03300f0,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch28 = {
    28,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_28_ins_data),
    0x000463d0,
    0xbc92f1c0,
    0xc0006CF8,
    (uint8_t *)bes2300p_t1_patch_28_ins_data}; // lld_evt_end_isr:SYS 2M

const uint32_t bes2300p_t1_patch_29_ins_data[] = {
    0x681b4b0a, 0x0301f033, 0xbf00d106, 0x4b044a03, 0x4a04601a, 0x601a4b04,
    0xbc96f63d, 0x04007107, 0xD0220000, 0xffff0008, 0xd03300f0, 0xd0220054,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch29 = {
    29,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_29_ins_data),
    0x00044658,
    0xbb5cf1c2,
    0xc0006d14,
    (uint8_t *)bes2300p_t1_patch_29_ins_data}; // lld_evt_schedule:SYS 1M

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch30 = {
    30, BTDRV_PATCH_ACT, 0, 0x00029e70, 0xbf8c7fb9, 0, NULL,
}; // ld_acl_rx_sync2

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch31 = {
    31, BTDRV_PATCH_ACT, 0, 0x0002a068, 0xbf947fb9, 0, NULL,
}; // ld_acl_rx_no_sync

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch32 = {
    32, BTDRV_PATCH_ACT, 0, 0x0002a1b0, 0x7fb9f5b3, 0, NULL,
}; // ld_acl_rx_sync

const uint32_t bes2300p_t1_patch_33_ins_data[] = {
    0xfdf6f624, 0x4638b920, 0xf63c2100, 0xb108fa01, 0xbf2bf628, 0xbf3df628,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch33 = {
    33,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_patch_33_ins_data),
    0x0002fba8,
    0xb8ccf1d7,
    0xc0006d44,
    (uint8_t *)bes2300p_t1_patch_33_ins_data}; // protect ble 8 slot

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch34 = {
    34, BTDRV_PATCH_ACT, 0, 0x00043248, 0xbf8c2c08, 0, NULL,
}; // check conflict 8 slot 2

const uint32_t bes2300p_t1_patch_35_ins_data[] = {
    /*6d54*/ 0xf8534b02,
    /*6d58*/ 0xb11b3020,
    /*6d5c*/ 0xbafdf610,
    /*6d60*/ 0xc0005c70,
    /*6d64*/ 0x47702000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch35 = {
    35,
    BTDRV_PATCH_INACT,
    sizeof(bes2300p_t1_patch_35_ins_data),
    0x00017354,
    0xbcfef1ef,
    0xc0006d54,
    (uint8_t *)bes2300p_t1_patch_35_ins_data}; // is_in_sniff_mode use NULL ptr

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch36 = {
    36, BTDRV_PATCH_INACT, 0, 0x00039b58, 0xf2002802, 0, NULL,
}; // ld_sniffer_start_monitored_link(link_id >= MAX_NB_ACTIVE_ACL)

/////2300p t1 patch
static const uint32_t best2300p_t1_ins_patch_config[] = {
    37,
    (uint32_t)&bes2300p_t1_ins_patch0,
    (uint32_t)&bes2300p_t1_ins_patch1,
    (uint32_t)&bes2300p_t1_ins_patch2,
    (uint32_t)&bes2300p_t1_ins_patch3,
    (uint32_t)&bes2300p_t1_ins_patch4,
    (uint32_t)&bes2300p_t1_ins_patch5,
    (uint32_t)&bes2300p_t1_ins_patch6,
    (uint32_t)&bes2300p_t1_ins_patch7,
    (uint32_t)&bes2300p_t1_ins_patch8,
    (uint32_t)&bes2300p_t1_ins_patch9,
    (uint32_t)&bes2300p_t1_ins_patch10,
    (uint32_t)&bes2300p_t1_ins_patch11,
    (uint32_t)&bes2300p_t1_ins_patch12,
    (uint32_t)&bes2300p_t1_ins_patch13,
    (uint32_t)&bes2300p_t1_ins_patch14,
    (uint32_t)&bes2300p_t1_ins_patch15,
    (uint32_t)&bes2300p_t1_ins_patch16,
    (uint32_t)&bes2300p_t1_ins_patch17,
    (uint32_t)&bes2300p_t1_ins_patch18,
    (uint32_t)&bes2300p_t1_ins_patch19,
    (uint32_t)&bes2300p_t1_ins_patch20,
    (uint32_t)&bes2300p_t1_ins_patch21,
    (uint32_t)&bes2300p_t1_ins_patch22,
    (uint32_t)&bes2300p_t1_ins_patch23,
    (uint32_t)&bes2300p_t1_ins_patch24,
    (uint32_t)&bes2300p_t1_ins_patch25,
    (uint32_t)&bes2300p_t1_ins_patch26,
    (uint32_t)&bes2300p_t1_ins_patch27,
    (uint32_t)&bes2300p_t1_ins_patch28,
    (uint32_t)&bes2300p_t1_ins_patch29,
    (uint32_t)&bes2300p_t1_ins_patch30,
    (uint32_t)&bes2300p_t1_ins_patch31,
    (uint32_t)&bes2300p_t1_ins_patch32,
    (uint32_t)&bes2300p_t1_ins_patch33,
    (uint32_t)&bes2300p_t1_ins_patch34,
    (uint32_t)&bes2300p_t1_ins_patch35,
    (uint32_t)&bes2300p_t1_ins_patch36,
};

void btdrv_ins_patch_write(BTDRV_PATCH_STRUCT *ins_patch_p) {
  uint32_t remap_addr;
  /// uint8_t i=0;
  remap_addr = ins_patch_p->patch_remap_address | 1;
  btdrv_write_memory(
      _32_Bit,
      (BTDRV_PATCH_INS_REMAP_ADDR_START + ins_patch_p->patch_index * 4),
      (uint8_t *)&ins_patch_p->patch_remap_value, 4);
  if (ins_patch_p->patch_length != 0) // have ram patch data
  {
    btdrv_memory_copy((uint32_t *)ins_patch_p->patch_start_address,
                      (uint32_t *)ins_patch_p->patch_data,
                      ins_patch_p->patch_length);
  }

  btdrv_write_memory(
      _32_Bit, (BTDRV_PATCH_INS_COMP_ADDR_START + ins_patch_p->patch_index * 4),
      (uint8_t *)&remap_addr, 4);
}

void btdrv_ins_patch_init(void) {
  const BTDRV_PATCH_STRUCT *ins_patch_p;
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    for (uint8_t i = 0; i < best2300p_t1_ins_patch_config[0]; i++) {
      ins_patch_p = (BTDRV_PATCH_STRUCT *)best2300p_t1_ins_patch_config[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)best2300p_t1_ins_patch_config[i + 1]);
    }
  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
#ifndef IBRT
    for (uint8_t i = 0; i < best2300p_t2_ins_patch_config[0]; i++) {
      ins_patch_p = (BTDRV_PATCH_STRUCT *)best2300p_t2_ins_patch_config[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)best2300p_t2_ins_patch_config[i + 1]);
    }
#else
    for (uint8_t i = 0; i < best2300p_t2_ibrt_ins_patch_config[0]; i++) {
      ins_patch_p =
          (BTDRV_PATCH_STRUCT *)best2300p_t2_ibrt_ins_patch_config[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)best2300p_t2_ibrt_ins_patch_config[i + 1]);
    }
#endif
  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
#ifdef IBRT
    for (uint8_t i = 0; i < best2300p_t3_ibrt_ins_patch_config[0]; i++) {
      ins_patch_p =
          (BTDRV_PATCH_STRUCT *)best2300p_t3_ibrt_ins_patch_config[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)best2300p_t3_ibrt_ins_patch_config[i + 1]);
    }
#else
    for (uint8_t i = 0; i < best2300p_t3_ins_patch_config[0]; i++) {
      ins_patch_p = (BTDRV_PATCH_STRUCT *)best2300p_t3_ins_patch_config[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)best2300p_t3_ins_patch_config[i + 1]);
    }
#endif
  } else {
    ASSERT(0, "%s:error chip id=%d", __func__, hal_get_chip_metal_id());
  }
}

///////////////////data  patch ..////////////////////////////////////

static const POSSIBLY_UNUSED uint32_t best2300p_t2_data_patch_config[] = {};

void btdrv_data_patch_write(const BTDRV_PATCH_STRUCT *d_patch_p) {

  uint32_t remap_addr;
  uint8_t i = 0;

  remap_addr = d_patch_p->patch_remap_address | 1;
  btdrv_write_memory(
      _32_Bit, (BTDRV_PATCH_DATA_COMP_ADDR_START + d_patch_p->patch_index * 4),
      (uint8_t *)&remap_addr, 4);
  btdrv_write_memory(
      _32_Bit, (BTDRV_PATCH_DATA_REMAP_ADDR_START + d_patch_p->patch_index * 4),
      (uint8_t *)&d_patch_p->patch_remap_value, 4);

  if (d_patch_p->patch_length != 0) // have ram patch data
  {
    for (; i < (d_patch_p->patch_length - 1) / 128; i++) {
      btdrv_write_memory(_32_Bit, d_patch_p->patch_start_address + i * 128,
                         (d_patch_p->patch_data + i * 128), 128);
    }

    btdrv_write_memory(_32_Bit, d_patch_p->patch_start_address + i * 128,
                       d_patch_p->patch_data + i * 128,
                       d_patch_p->patch_length % 128);
  }
}

void btdrv_ins_patch_disable(uint8_t index) {
  uint32_t addr = 0;
  btdrv_write_memory(_32_Bit, (BTDRV_PATCH_INS_COMP_ADDR_START + index * 4),
                     (uint8_t *)&addr, 4);
}

void btdrv_data_patch_init(void) {
#if 0
    const BTDRV_PATCH_STRUCT *data_patch_p;
    if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1)
    {
        for(uint8_t i=0; i<best2300p_t2_data_patch_config[0]; i++)
        {
            data_patch_p = (BTDRV_PATCH_STRUCT *)best2300p_t2_data_patch_config[i+1];
            if(data_patch_p->patch_state == BTDRV_PATCH_ACT)
                btdrv_data_patch_write((BTDRV_PATCH_STRUCT *)best2300p_t2_data_patch_config[i+1]);
        }
    }
#endif
}

//////////////////////////////patch enable////////////////////////

void btdrv_patch_en(uint8_t en) {
  uint32_t value[2];

  // set patch enable
  value[0] = 0x2f02 | en;
  // set patch remap address  to 0xc0000100
  value[1] = 0x20000100;
  btdrv_write_memory(_32_Bit, BTDRV_PATCH_EN_REG, (uint8_t *)&value, 8);
}

/////////////2300p t3 test mode patch///////////////////////

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_0 = {
    0,   BTDRV_PATCH_ACT, 0, 0x0002ceb4, 0xe00b0f0f, 0,
    NULL}; // ld_acl_rx() average rssi

const uint32_t bes2300p_t3_ins_patch_data_testmode_1[] = {
    /*6a98*/ 0x99049803,
    /*6a9c*/ 0x4b12b470,
    /*6aa0*/ 0x010e5c5a,
    /*6aa4*/ 0x19734d11,
    /*6aa8*/ 0x78244c11,
    /*6aac*/ 0xd20342a2,
    /*6ab0*/ 0x32015498,
    /*6ab4*/ 0xe001b2d2,
    /*6ab8*/ 0x22015570,
    /*6abc*/ 0x5442480a,
    /*6ac0*/ 0x461ab14c,
    /*6ac4*/ 0x23001919,
    /*6ac8*/ 0x0b01f912,
    /*6acc*/ 0xbf004403,
    /*6ad0*/ 0xd1f9428a,
    /*6ad4*/ 0x2300e000,
    /*6ad8*/ 0xf0f4fb93,
    /*6adc*/ 0xbc70b240,
    /*6ae0*/ 0x22019904,
    /*6ae4*/ 0xba00f626, // jump a0206ae4 -> a002cee8
    /*6ae8*/ 0xc0006af8, // addr of rssi_idx_for_agc
    /*6aec*/ 0xc0006afc, // addr of rssi_set_for_agc
    /*6af0*/ 0xc0006af4, // addr of N
    /*6af4*/ 0x00000005, // N : 1 ~ 16
    /*6af8*/ 0x00000000, // rssi_idx_for_agc[3]
    /*6afc*/ 0xbabababa, // rssi_set_for_agc[3*16]
    /*6b00*/ 0xbabababa,
    /*6b04*/ 0xbabababa,
    /*6b08*/ 0xbabababa,
    /*6b0c*/ 0xbabababa,
    /*6b10*/ 0xbabababa,
    /*6b14*/ 0xbabababa,
    /*6b18*/ 0xbabababa,
    /*6b1c*/ 0xbabababa,
    /*6b20*/ 0xbabababa,
    /*6b24*/ 0xbabababa,
    /*6b28*/ 0xbabababa,
    /*6b2c*/ 0xbabababa,
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t3_ins_patch_data_testmode_1),
    0x0002cee4,
    0xbdd8f1d9, // jump a002cee4 -> a0206a98
    0xc0006a98,
    (uint8_t *)
        bes2300p_t3_ins_patch_data_testmode_1}; // ld_acl_rx() average rssi

const uint32_t bes2300p_t3_ins_patch_data_testmode_2[] = {
    0x2028f843, /*6b3c*/
    0xf8434b08, 0x4b052028, 0x4b0e701a, 0x2201701a, 0xf8434b08,
    0xbf002028, 0xba32f624, 0xc0006b60, 0x00000000, 0xc0006b68,
    0x00000000, 0x00000000, 0x00000000, 0xc0006b78, 0x00000001,
    0x00000001, 0x00000001, 0xc0006b88, 0x00000000, /*6b88*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_2 = {
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t3_ins_patch_data_testmode_2),
    0x0002afbc,
    0xbdbef1db,
    0xc0006b3c,
    (uint8_t *)bes2300p_t3_ins_patch_data_testmode_2}; // sync clear count

const uint32_t bes2300p_t3_ins_patch_data_testmode_3[] = {
    0x47f0e92d, /*7050*/
    0x4b97b082, 0x236e5c1e, 0xf300fb03, 0x5a9b4a95, 0x4f00f413, 0x8120f040,
    0x46044689, 0x1800ea4f, 0xeb084f91, 0x4b910507, 0xa020f853, 0xf8524a90,
    0x33013020, 0x3020f842, 0xf860f635, 0x42a04601, 0x8088f040, 0xf8534b8a,
    0x4b8a2024, 0x429a681b, 0xf8dad817, 0xebc22084, 0xf0230309, 0xf1b34378,
    0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a82425b, 0x0024f852, 0x68124a81,
    0xf200fb02, 0xf2404293, 0x4b7f80eb, 0x3000f993, 0x4b7eb9c3, 0xc000f893,
    0x0f00f1bc, 0x462ad00a, 0x0e0ceb05, 0xf9122300, 0x44030b01, 0xd1fa4572,
    0xe0014662, 0x46132200, 0xfcfcfb93, 0xf8804872, 0xe001c000, 0x46132200,
    0xf890486f, 0x4870e000, 0x9000f890, 0xf890486f, 0xfa4fc000, 0xebc9f08e,
    0xfa4f0000, 0x4550fa8c, 0xebc9dd05, 0x48660e0e, 0xe000f880, 0x4864e002,
    0xc000f880, 0x0e00f04f, 0xf840485d, 0xf8dfe024, 0xf85ee178, 0x30010024,
    0x0024f84e, 0x7800485d, 0xd2054286, 0x783f4f5a, 0x360155af, 0xe004b2f6,
    0x78364e57, 0x6007f808, 0x4f4d2601, 0x4282553e, 0x442ada06, 0xf9124405,
    0x44234b01, 0xd1fa42aa, 0xfb934a53, 0x9200f0f0, 0xb2406a14, 0x23002201,
    0xe08147a0, 0xf8534b46, 0x4b4e2024, 0x429a681b, 0xf8dad816, 0xebc22084,
    0xf0230309, 0xf1b34378, 0xd9046f80, 0x0302ebc9, 0x4378f023, 0x4a3e425b,
    0x1024f852, 0x68124a44, 0xf201fb02, 0xd9634293, 0xf9934b42, 0xb9c33000,
    0xf8934b3a, 0xf1bee000, 0xd00a0f00, 0xeb05462a, 0x2300000e, 0x1b01f912,
    0x4282440b, 0x4672d1fa, 0x2200e001, 0xfb934613, 0x4936fefe, 0xe000f881,
    0x2200e001, 0x49334613, 0x49337808, 0xc000f891, 0xf8914932, 0xb241e000,
    0x0101ebcc, 0xf98efa4f, 0xdd044549, 0x0000ebcc, 0x7008492a, 0x4929e002,
    0xe000f881, 0x491c2000, 0x0024f841, 0xf850481c, 0x31011024, 0x1024f840,
    0x7808491c, 0xd2054286, 0x78094920, 0x360155a9, 0xe004b2f6, 0x7809491d,
    0x1007f808, 0x490c2601, 0x4282550e, 0x442ada06, 0xf9124405, 0x440b1b01,
    0xd1fa42aa, 0xfb934a12, 0x9200f0f0, 0xb2406a15, 0x22014621, 0x47a82300,
    0xe8bdb002, 0xbf0087f0,
    0xc0006af8, // rssi_idx_for_agc[link_id]  shenxin
    0xd02111f8, // EM_BT_RD(EM_BT_RXBIT_ADDR
    0xc0006afc, // rssi_set_for_agc   shenxin
    0xc00009d4, // ld_acl_env[link_id]
    0xc0006b68, // link_no_sync_count[link_id]
    0xc00072fc, // link_agc_thd_tws
    0xc0006b78, // link_no_sync_count_time[link_id]
    0xc0007300, // link_agc_thd_tws_time
    0xc0006b60, // rssi_store_tws
    0xc0006af4, // n   shenxin
    0xc0007304, // rssi_tws_step
    0xc0007308, // rssi_min_value_tws
    0xc0004268, // rwip_rf.rxgain_cntl
    0xc000730c, // link_agc_thd_mobile
    0xc0007310, // link_agc_thd_mobile_time
    0xc0006b88, // rssi_store_mobile
    0xc0007314, // rssi_mobile_step
    0xc0007318, // rssi_min_value_mobile
    0xffffffff, // link_agc_thd_tws
    0x00000030, // link_agc_thd_tws_time
    0x00000014, // rssi_tws_step
    0x0000009c, // rssi_min_value_tws
    0xffffffff, // link_agc_thd_mobile
    0x00000030, // link_agc_thd_mobile_time
    0x00000014, // rssi_mobile_step
    0x000000a1, // rssi_min_value_mobile
    0x99029804, 0xfe96f7ff, 0xfc48f632, 0xb898f626, /*7328*/
};

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t3_ins_patch_data_testmode_3),
    0x0002d458,
    0xbf60f1d9,
    0xc0007050,
    (uint8_t *)bes2300p_t3_ins_patch_data_testmode_3}; // swagc no sync

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_4 = {
    4,   BTDRV_PATCH_ACT, 0, 0x0000e47a, 0xe0027b20, 0,
    NULL}; // lm_inq_res_ind_handler()  rssi

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_5 = {
    5,   BTDRV_PATCH_INACT, 0, 0x0002ad78, 0xbf00e009, 0,
    NULL}; // ld_acl_rx_no_sync()  rssi

const uint32_t bes2300p_t3_ins_patch_data_testmode_6[] = {
    0xf8802300, 0xf8803024, 0xbf003025, 0x30b3f894, 0xbc18f62d,
}; /* 6b90-6ba0 */

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_6 = {
    6,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t3_ins_patch_data_testmode_6),
    0x000343d0,
    0xbbdef1d2,
    0xc0006b90,
    (uint8_t *)
        bes2300p_t3_ins_patch_data_testmode_6}; // nosig test pattern problem

const BTDRV_PATCH_STRUCT bes2300p_t3_ins_patch_testmode_7 = {
    7, BTDRV_PATCH_ACT, 0, 0x00000880, 0x26004c0e, 0, NULL,
}; // rwip_env.sleep_enable=false after hci reset

static const uint32_t ins_patch_2300p_t3_config_testmode[] = {
    8,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_0,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_1,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_2,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_3,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_4,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_5,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_6,
    (uint32_t)&bes2300p_t3_ins_patch_testmode_7,
};
/////////////2300p t2 test mode patch///////////////////////

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_0 = {
    0, BTDRV_PATCH_ACT, 0, 0x00000874, 0x26004c0e, 0, NULL,
}; ////rwip_env.sleep_enable=false after hci reset

const uint32_t bes2300p_t2_ins_patch_data_testmode_1[] = {
    0x4681b083, /*6b30*/
    0x4b022200, 0x2020f843, 0xbddcf623, 0xc0006b44,
    0x00000000, 0x00000000, 0x00000000, /*6b4c*/
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_1),
    0x0002a6f4,
    0xba1cf1dc,
    0xc0006b30,
    (uint8_t *)bes2300p_t2_ins_patch_data_testmode_1}; // sync clear count

const uint32_t bes2300p_t2_ins_patch_data_testmode_2[] = {
    0x6020f853, 0xf8524a0d, 0x33013020, 0x3020f842, 0x68124a0b,
    0xd90f4293, 0x4a082300, 0x3020f842, 0xb082b402, 0x92004a08,
    0xf06f6a14, 0x46290059, 0x47a02201, 0xbc02b002, 0xbce4f623,
    0xc0006b44, 0xc0006b94, 0x00000010, 0xc00041b4,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_2 = {
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_2),
    0x0002a550,
    0xbafef1dc,
    0xc0006b50,
    (uint8_t *)bes2300p_t2_ins_patch_data_testmode_2};

const uint32_t bes2300p_t2_ins_patch_data_testmode_3[] = {
    /*6f90*/ 0xbf009904,
    /*6f94*/ 0x4b12b470,
    /*6f98*/ 0x010e5c5a,
    /*6f9c*/ 0x19734d11,
    /*6fa0*/ 0x78244c11,
    /*6fa4*/ 0xd20342a2,
    /*6fa8*/ 0x32015498,
    /*6fac*/ 0xe001b2d2,
    /*6fb0*/ 0x22015570,
    /*6fb4*/ 0x5442480a,
    /*6fb8*/ 0x461ab14c,
    /*6fbc*/ 0x23001919,
    /*6fc0*/ 0x0b01f912,
    /*6fc4*/ 0xbf004403,
    /*6fc8*/ 0xd1f9428a,
    /*6fcc*/ 0x2300e000,
    /*6fd0*/ 0xf0f4fb93,
    /*6fd4*/ 0xbc70b240,
    /*6fd8*/ 0x22019904,
    /*6fdc*/ 0xbb8cf628, // jump a002f6f8
    /*6fe0*/ 0xc0006ff0, // addr of rssi_idx_for_agc
    /*6fe4*/ 0xc0006ff4, // addr of rssi_set_for_agc
    /*6fe8*/ 0xc0006fec, // addr of N
    /*6fec*/ 0x00000005, // N : 1 ~ 16
    /*6ff0*/ 0x00000000, // rssi_idx_for_agc[3]
    /*6ff4*/ 0xbabababa, // rssi_set_for_agc[3*16]
    /*6ff8*/ 0xbabababa,
    /*6ffc*/ 0xbabababa,
    /*7000*/ 0xbabababa,
    /*7004*/ 0xbabababa,
    /*7008*/ 0xbabababa,
    /*700c*/ 0xbabababa,
    /*7010*/ 0xbabababa,
    /*7014*/ 0xbabababa,
    /*7018*/ 0xbabababa,
    /*701c*/ 0xbabababa,
    /*7020*/ 0xbabababa,
    /*7024*/ 0xbabababa,
    /*7028*/ 0xbabababa,
    /*702c*/ 0xbabababa,
}; // 0xc0006f90--->0xc0007074

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_3),
    0x0002f6f4,
    0xbc4cf1d7, // jump a002f6f4 -> a0206f90
    0xc0006f90,
    (uint8_t *)
        bes2300p_t2_ins_patch_data_testmode_3}; // ld_acl_rx() average rssi

const uint32_t bes2300p_t2_ins_patch_data_testmode_4[] = {
    0x4620461d,
    0x463a4629,
    0xf5f94633,
    0xbdf8fc2b,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_4 = {
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_4),
    0x000070ec,
    0xbe08f1ff,
    0xc0006d00,
    (uint8_t *)bes2300p_t2_ins_patch_data_testmode_4}; // assert warn

const uint32_t bes2300p_t2_ins_patch_data_testmode_5[] = {
    0x4630461f,
    0x462a4639,
    0xf5f94623,
    0xbdf8fc21,
};

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_5 = {
    5,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_5),
    0x00007070,
    0xbe50f1ff,
    0xc0006d14,
    (uint8_t *)bes2300p_t2_ins_patch_data_testmode_5}; // assert param

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_6 = {
    6, BTDRV_PATCH_ACT, 0, 0x0003bbe0, 0xf7c52100, 0, NULL,
}; /// ble tx test mode switch freq channel problem

const uint32_t bes2300p_t2_ins_patch_data_testmode_7[] = {
    0xf8802300, 0xf8803024, 0xbf003025, 0x30b3f894, 0xbe0ef62c,
}; /* 6d30-6d40 */

const BTDRV_PATCH_STRUCT bes2300p_t2_ins_patch_testmode_7 = {
    7,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t2_ins_patch_data_testmode_7),
    0x0003395c,
    0xb9e8f1d3,
    0xc0006d30,
    (uint8_t *)
        bes2300p_t2_ins_patch_data_testmode_7}; // nosig test pattern problem

static const uint32_t ins_patch_2300p_t2_config_testmode[] = {
    8,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_0,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_1,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_2,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_3,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_4,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_5,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_6,
    (uint32_t)&bes2300p_t2_ins_patch_testmode_7,
};

/////////////2300p t1 test mode patch///////////////////////

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_0 = {
    0,   BTDRV_PATCH_ACT, 0, 0x0002f440, 0x789abf00, 0,
    NULL}; /// test mode: remove rxllid's judging condition in ld_acl_rx

const uint32_t bes2300p_t1_ins_patch_data_testmode_1[] = {
    0x20fcf8d5, 0xf8928811, 0xf0088007, 0xb909080f, 0x88194b11, 0x0484eb04,
    0x3014f837, 0xf423b29b, 0x4a0f7300, 0xea438812, 0xb29b2342, 0x3014f827,
    0x3014f837, 0xf023b29b, 0xea430378, 0xf82708c8, 0x4b068014, 0xf043781b,
    0xea430304, 0xb28b01c1, 0x3014f82a, 0xbe8ef627,
    0xc000099c, ////loopback_length
    0xc000097d, ////loopback_llid
    0xc0006964, ////rxseq_flag
    0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_1 = {
    1,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_1),
    0x0002e60c,
    0xb978f1d8,
    0xc0006900,
    (uint8_t *)
        bes2300p_t1_ins_patch_data_testmode_1}; /// test mode: ld_acl_tx_prog

const uint32_t bes2300p_t1_ins_patch_data_testmode_2[] = {
    0xfbdcf5fc, 0x3060f890, 0xd01f2b00, 0x30fcf8db, 0xd01b2b00, 0x0f03f1b9,
    0xf1b9d018, 0xd0150f08, 0xf8934b0b, 0x011b31cb, 0x5a9b4a0a, 0x7f80f413,
    0x4b09d10a, 0x4b0a781a, 0x4b08601a, 0x4b09881a, 0x2201601a, 0x601a4b08,
    0xbc32f628, 0xbc2ef628, 0xc0005d34, 0xd02115f2,
    0xc000097d, ////loopback_llid
    0xc000099c, ////loopback_length
    0xc0006a8c, ////rxllid_flag
    0xc0006a90, ////rxlength_flag
    0xc0006a94, ////unack_seqerr_flag
    0x00000000, 0x00000000, 0x00000000,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_2 = {
    2,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_2),
    0x0002f2c8,
    0xbbaaf1d7,
    0xc0006a20,
    (uint8_t *)bes2300p_t1_ins_patch_data_testmode_2}; /// test mode: skip rx
                                                       /// seq err in ld_acl_rx

#if 0
const uint32_t bes2300p_t1_ins_patch_data_testmode_3[] =
{
    0xf015d002,
    0xd0010f24,
    0xbc1af628,
    0xbcfcf628,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_3 =
{
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_3),
    0x0002f2d4,
    0xbbe0f1d7,
    0xc0006a98,
    (uint8_t*)bes2300p_t1_ins_patch_data_testmode_3
};///test mode: skip crc err in ld_acl_rx
#else
const uint32_t bes2300p_t1_ins_patch_data_testmode_3[] = {
    0xf015d006, 0xd0050f24, 0x0f03f1b9, 0xbf00d002, 0xbc16f628, 0xbcf8f628,
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_3 = {
    3,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_3),
    0x0002f2d4,
    0xbbe0f1d7,
    0xc0006a98,
    (uint8_t *)bes2300p_t1_ins_patch_data_testmode_3}; /// test mode: skip crc
                                                       /// err in ld_acl_rx

#endif

const uint32_t bes2300p_t1_ins_patch_data_testmode_4[] = {
    0x700d4914, 0xf891490c, 0x010911cb, 0x5a8a4a0b, 0x2240f3c2, 0x6002480a,
    0x6800480a, 0x2100b150, 0x60014808, 0x68014808, 0x80014808, 0x68014808,
    0x70014808, 0xbcbaf628, 0xc0005d34, 0xd02115f2,
    0xc0006964, ////rxseq_flag
    0xc0006a94, ////unack_seqerr_flag
    0xc0006a90, ////rxlength_flag
    0xc000099c, ////loopback_length
    0xc0006a8c, ////rxllid_flag
    0xc000097d, ////loopback_llid
};

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_4 = {
    4,
    BTDRV_PATCH_ACT,
    sizeof(bes2300p_t1_ins_patch_data_testmode_4),
    0x0002f464,
    0xbb2af1d7,
    0xc0006abc,
    (uint8_t *)bes2300p_t1_ins_patch_data_testmode_4}; /// test mode: ld_acl_rx

const BTDRV_PATCH_STRUCT bes2300p_t1_ins_patch_testmode_5 = {
    5, BTDRV_PATCH_ACT, 0, 0x00000100, 0x20004b15, 0, NULL,
}; ////rwip_env.sleep_enable=false after hci reset

static const uint32_t ins_patch_2300p_t1_config_testmode[] = {
    6,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_0,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_1,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_2,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_3,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_4,
    (uint32_t)&bes2300p_t1_ins_patch_testmode_5,
};

void btdrv_ins_patch_test_init(void) {
  const BTDRV_PATCH_STRUCT *ins_patch_p;

  btdrv_patch_en(0);

  for (uint8_t i = 0; i < 56; i++) {
    btdrv_ins_patch_disable(i);
  }
  bt_drv_reg_op_for_test_mode_disable();

  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    for (uint8_t i = 0; i < ins_patch_2300p_t1_config_testmode[0]; i++) {
      ins_patch_p =
          (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t1_config_testmode[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t1_config_testmode[i + 1]);
    }
  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    for (uint8_t i = 0; i < ins_patch_2300p_t2_config_testmode[0]; i++) {
      ins_patch_p =
          (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t2_config_testmode[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t2_config_testmode[i + 1]);
    }

  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    for (uint8_t i = 0; i < ins_patch_2300p_t3_config_testmode[0]; i++) {
      ins_patch_p =
          (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t3_config_testmode[i + 1];
      if (ins_patch_p->patch_state == BTDRV_PATCH_ACT)
        btdrv_ins_patch_write(
            (BTDRV_PATCH_STRUCT *)ins_patch_2300p_t3_config_testmode[i + 1]);
    }
    bt_drv_reg_op_disable_swagc_nosync_count();
  } else {
    ASSERT(0, "%s:error chip id=%d", __func__, hal_get_chip_metal_id());
  }

  btdrv_patch_en(1);
}
