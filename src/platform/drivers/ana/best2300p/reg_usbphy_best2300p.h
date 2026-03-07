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
#ifndef __REG_USBPHY_BEST2300P_H__
#define __REG_USBPHY_BEST2300P_H__

#include "plat_types.h"

// REG_00
#define REG_00_REVID_SHIFT                  0
#define REG_00_REVID_MASK                   (0xF << REG_00_REVID_SHIFT)
#define REG_00_REVID(n)                     BITFIELD_VAL(REG_00_REVID, n)
#define REG_00_CHIPID_SHIFT                 4
#define REG_00_CHIPID_MASK                  (0xFFF << REG_00_CHIPID_SHIFT)
#define REG_00_CHIPID(n)                    BITFIELD_VAL(REG_00_CHIPID, n)

// REG_01
#define REG_01_CFG_ANAPHY_RESETN            (1 << 0)
#define REG_01_CFG_CKCDR_EN                 (1 << 1)
#define REG_01_CFG_CKPLL_EN                 (1 << 2)
#define REG_01_CFG_EN_CLKMAC                (1 << 3)
#define REG_01_CFG_POL_CDRCLK               (1 << 4)
#define REG_01_CFG_RESETN_MAC               (1 << 5)
#define REG_01_CFG_RESETNCDR                (1 << 6)
#define REG_01_CFG_RESETNPLL                (1 << 7)
#define REG_01_CFG_BYPASS_CDR_ENDRESETN     (1 << 8)
#define REG_01_CFG_INTR_EN_ALL              (1 << 9)
#define REG_01_CFG_LOW_SPEED_MODE           (1 << 10)
#define REG_01_CFG_POL_CLKPLL               (1 << 11)
#define REG_01_CFG_BYPASS_RDY               (1 << 12)
#define REG_01_CFG_MODE_HS_LINES_TX         (1 << 13)
#define REG_01_CFG_MODE_FS_LINES_TX         (1 << 14)
#define REG_01_CFG_ENUM_MODE                (1 << 15)

// REG_02
#define REG_02_CFG_TARGET_SYNC_TOUT_SHIFT   0
#define REG_02_CFG_TARGET_SYNC_TOUT_MASK    (0xF << REG_02_CFG_TARGET_SYNC_TOUT_SHIFT)
#define REG_02_CFG_TARGET_SYNC_TOUT(n)      BITFIELD_VAL(REG_02_CFG_TARGET_SYNC_TOUT, n)
#define REG_02_CFG_SEL_SYNCPATTERN_SHIFT    4
#define REG_02_CFG_SEL_SYNCPATTERN_MASK     (0x3 << REG_02_CFG_SEL_SYNCPATTERN_SHIFT)
#define REG_02_CFG_SEL_SYNCPATTERN(n)       BITFIELD_VAL(REG_02_CFG_SEL_SYNCPATTERN, n)
#define REG_02_CFG_BYPASS_SQL_VALID         (1 << 6)
#define REG_02_CFG_EN_SYNCTOUT              (1 << 7)
#define REG_02_CFG_FORCERX                  (1 << 8)
#define REG_02_CFG_RXRESET                  (1 << 9)
#define REG_02_CFG_EBUF_THRD_SHIFT          10
#define REG_02_CFG_EBUF_THRD_MASK           (0x1F << REG_02_CFG_EBUF_THRD_SHIFT)
#define REG_02_CFG_EBUF_THRD(n)             BITFIELD_VAL(REG_02_CFG_EBUF_THRD, n)
#define REG_02_CFG_EOP_MODE_RX              (1 << 15)

// REG_03
#define REG_03_CFG_TARGET_TAIL_SHIFT        0
#define REG_03_CFG_TARGET_TAIL_MASK         (0xFF << REG_03_CFG_TARGET_TAIL_SHIFT)
#define REG_03_CFG_TARGET_TAIL(n)           BITFIELD_VAL(REG_03_CFG_TARGET_TAIL, n)
#define REG_03_CFG_TXPATTERN_SHIFT          8
#define REG_03_CFG_TXPATTERN_MASK           (0xFF << REG_03_CFG_TXPATTERN_SHIFT)
#define REG_03_CFG_TXPATTERN(n)             BITFIELD_VAL(REG_03_CFG_TXPATTERN, n)

// REG_04
#define REG_04_CFG_EN_HSTSOF                (1 << 0)
#define REG_04_CFG_FORCETX                  (1 << 1)
#define REG_04_CFG_MODE_BITEN               (1 << 2)
#define REG_04_CFG_TXSTATE_RESET            (1 << 3)
#define REG_04_CFG_EMPTY_DLY_SEL_SHIFT      4
#define REG_04_CFG_EMPTY_DLY_SEL_MASK       (0x7 << REG_04_CFG_EMPTY_DLY_SEL_SHIFT)
#define REG_04_CFG_EMPTY_DLY_SEL(n)         BITFIELD_VAL(REG_04_CFG_EMPTY_DLY_SEL, n)
#define REG_04_RESERVED_0                   (1 << 7)
#define REG_04_CFG_TP_SEL_SHIFT             8
#define REG_04_CFG_TP_SEL_MASK              (0xFF << REG_04_CFG_TP_SEL_SHIFT)
#define REG_04_CFG_TP_SEL(n)                BITFIELD_VAL(REG_04_CFG_TP_SEL, n)

// REG_05
#define REG_05_CFG_ADP_PRB                  (1 << 0)
#define REG_05_CFG_ADP_SNS                  (1 << 1)
#define REG_05_CFG_RID_A                    (1 << 2)
#define REG_05_CFG_RID_B                    (1 << 3)
#define REG_05_CFG_RID_C                    (1 << 4)
#define REG_05_CFG_RID_FLOAT                (1 << 5)
#define REG_05_CFG_RID_GND                  (1 << 6)
#define REG_05_CFG_UTMIOTG_AVALID           (1 << 7)
#define REG_05_CFG_UTMISRP_BVALID           (1 << 8)
#define REG_05_CFG_UTMIOTG_VBUSVALID        (1 << 9)
#define REG_05_CFG_UTMISRP_SESSEND          (1 << 10)
#define REG_05_RESERVED_5_1_SHIFT           11
#define REG_05_RESERVED_5_1_MASK            (0x1F << REG_05_RESERVED_5_1_SHIFT)
#define REG_05_RESERVED_5_1(n)              BITFIELD_VAL(REG_05_RESERVED_5_1, n)

// REG_06
#define REG_06_CFG_HS_SRC_SEL               (1 << 0)
#define REG_06_CFG_HS_TX_EN                 (1 << 1)
#define REG_06_CFG_CHIRP_EN                 (1 << 2)
#define REG_06_CFG_HS_MODE                  (1 << 3)
#define REG_06_CFG_EN_RHS                   (1 << 4)
#define REG_06_CFG_HS_DRV_SEL_SHIFT         5
#define REG_06_CFG_HS_DRV_SEL_MASK          (0xF << REG_06_CFG_HS_DRV_SEL_SHIFT)
#define REG_06_CFG_HS_DRV_SEL(n)            BITFIELD_VAL(REG_06_CFG_HS_DRV_SEL, n)
#define REG_06_CFG_RHS_TRIM_SHIFT           9
#define REG_06_CFG_RHS_TRIM_MASK            (0xF << REG_06_CFG_RHS_TRIM_SHIFT)
#define REG_06_CFG_RHS_TRIM(n)              BITFIELD_VAL(REG_06_CFG_RHS_TRIM, n)
#define REG_06_CFG_HS_RCV_PD                (1 << 13)
#define REG_06_CFG_EN_HOLD_LAST             (1 << 14)
#define REG_06_CFG_EN_HS_S2P                (1 << 15)

// REG_07
#define REG_07_CFG_RST_INTP                 (1 << 0)
#define REG_07_CFG_EN_ZPS                   (1 << 1)
#define REG_07_CFG_RES_GN_SHIFT             2
#define REG_07_CFG_RES_GN_MASK              (0xF << REG_07_CFG_RES_GN_SHIFT)
#define REG_07_CFG_RES_GN(n)                BITFIELD_VAL(REG_07_CFG_RES_GN, n)
#define REG_07_CFG_ISEL_RCV_SHIFT           6
#define REG_07_CFG_ISEL_RCV_MASK            (0x7 << REG_07_CFG_ISEL_RCV_SHIFT)
#define REG_07_CFG_ISEL_RCV(n)              BITFIELD_VAL(REG_07_CFG_ISEL_RCV, n)
#define REG_07_CFG_ISEL_SQ_SHIFT            9
#define REG_07_CFG_ISEL_SQ_MASK             (0x7 << REG_07_CFG_ISEL_SQ_SHIFT)
#define REG_07_CFG_ISEL_SQ(n)               BITFIELD_VAL(REG_07_CFG_ISEL_SQ, n)
#define REG_07_CFG_INT_FLT_SHIFT            12
#define REG_07_CFG_INT_FLT_MASK             (0x3 << REG_07_CFG_INT_FLT_SHIFT)
#define REG_07_CFG_INT_FLT(n)               BITFIELD_VAL(REG_07_CFG_INT_FLT, n)
#define REG_07_CFG_PI_GN_SHIFT              14
#define REG_07_CFG_PI_GN_MASK               (0x3 << REG_07_CFG_PI_GN_SHIFT)
#define REG_07_CFG_PI_GN(n)                 BITFIELD_VAL(REG_07_CFG_PI_GN, n)

// REG_08
#define REG_08_CFG_CDR_GN_SHIFT             0
#define REG_08_CFG_CDR_GN_MASK              (0x3 << REG_08_CFG_CDR_GN_SHIFT)
#define REG_08_CFG_CDR_GN(n)                BITFIELD_VAL(REG_08_CFG_CDR_GN, n)
#define REG_08_CFG_SEL_HS_GAIN              (1 << 2)
#define REG_08_CFG_SEL_TERR                 (1 << 3)
#define REG_08_CFG_SEL_HS_LSTATE            (1 << 4)
#define REG_08_CFG_LOOP_BACK                (1 << 5)
#define REG_08_CFG_XCVR_SELECT              (1 << 6)
#define REG_08_CFG_XTERM_SELECT             (1 << 7)
#define REG_08_CFG_USPENDM                  (1 << 8)
#define REG_08_CFG_DISCON_DET_EN            (1 << 9)
#define REG_08_CFG_DISCON_VTHSET_SHIFT      10
#define REG_08_CFG_DISCON_VTHSET_MASK       (0x7 << REG_08_CFG_DISCON_VTHSET_SHIFT)
#define REG_08_CFG_DISCON_VTHSET(n)         BITFIELD_VAL(REG_08_CFG_DISCON_VTHSET, n)
#define REG_08_CFG_FS_TX_ON                 (1 << 13)
#define REG_08_CFG_FS_EN_RFS                (1 << 14)
#define REG_08_CFG_FS_EN_R15K               (1 << 15)

// REG_09
#define REG_09_CFG_LS_EN_RLS                (1 << 0)
#define REG_09_CFG_LS_MODE                  (1 << 1)
#define REG_09_CFG_ATEST_EN_REG             (1 << 2)
#define REG_09_CFG_ATEST_SELX_REG_SHIFT     3
#define REG_09_CFG_ATEST_SELX_REG_MASK      (0x3 << REG_09_CFG_ATEST_SELX_REG_SHIFT)
#define REG_09_CFG_ATEST_SELX_REG(n)        BITFIELD_VAL(REG_09_CFG_ATEST_SELX_REG, n)
#define REG_09_CFG_DTESTEN1_REG             (1 << 5)
#define REG_09_CFG_DTESTEN2_REG             (1 << 6)
#define REG_09_CFG_DTEST_SEL_REG_SHIFT      7
#define REG_09_CFG_DTEST_SEL_REG_MASK       (0x3 << REG_09_CFG_DTEST_SEL_REG_SHIFT)
#define REG_09_CFG_DTEST_SEL_REG(n)         BITFIELD_VAL(REG_09_CFG_DTEST_SEL_REG, n)
#define REG_09_CFG_ATEST_EN_DISCON_REG      (1 << 9)
#define REG_09_CFG_ATEST_SELX_DISCON_REG    (1 << 10)
#define REG_09_CFG_MODE_LINESTATE           (1 << 11)
#define REG_09_RESERVED_8_6_SHIFT           12
#define REG_09_RESERVED_8_6_MASK            (0x7 << REG_09_RESERVED_8_6_SHIFT)
#define REG_09_RESERVED_8_6(n)              BITFIELD_VAL(REG_09_RESERVED_8_6, n)
#define REG_09_CFG_BISTEN                   (1 << 15)

// REG_0A
#define REG_0A_CFG_DR_HOSTMODE              (1 << 0)
#define REG_0A_CFG_DR_FSLS_SEL              (1 << 1)
#define REG_0A_CFG_DR_SDATA                 (1 << 2)
#define REG_0A_CFG_DR_TERMSEL               (1 << 3)
#define REG_0A_CFG_DR_XCVRSEL               (1 << 4)
#define REG_0A_CFG_DR_SON                   (1 << 5)
#define REG_0A_CFG_DR_OPMODE                (1 << 6)
#define REG_0A_RESERVED_9                   (1 << 7)
#define REG_0A_CFG_DR_HS_TXEN               (1 << 8)
#define REG_0A_CFG_DR_FSTXEN                (1 << 9)
#define REG_0A_CFG_DR_FSTXD                 (1 << 10)
#define REG_0A_CFG_DR_FSTXDB                (1 << 11)
#define REG_0A_CFG_FS_LS_SEL                (1 << 12)
#define REG_0A_CFG_CLK480M_EDGE_SEL         (1 << 13)
#define REG_0A_RESERVED_13_10_SHIFT         12
#define REG_0A_RESERVED_13_10_MASK          (0xF << REG_0A_RESERVED_13_10_SHIFT)
#define REG_0A_RESERVED_13_10(n)            BITFIELD_VAL(REG_0A_RESERVED_13_10, n)

// REG_0B
#define REG_0B_CFG_REG_HOSTMODE             (1 << 0)
#define REG_0B_CFG_REG_FSLS_SEL             (1 << 1)
#define REG_0B_CFG_REG_SDATA                (1 << 2)
#define REG_0B_CFG_REG_TERM                 (1 << 3)
#define REG_0B_CFG_REG_XCVRSEL              (1 << 4)
#define REG_0B_CFG_REG_SON                  (1 << 5)
#define REG_0B_CFG_REG_OPMODE_SHIFT         6
#define REG_0B_CFG_REG_OPMODE_MASK          (0x3 << REG_0B_CFG_REG_OPMODE_SHIFT)
#define REG_0B_CFG_REG_OPMODE(n)            BITFIELD_VAL(REG_0B_CFG_REG_OPMODE, n)
#define REG_0B_CFG_REG_HS_TXEN              (1 << 8)
#define REG_0B_CFG_REG_FSTXEN               (1 << 9)
#define REG_0B_CFG_REG_FSTXD                (1 << 10)
#define REG_0B_CFG_REG_FSTXDB               (1 << 11)
#define REG_0B_RESERVED_17_14_SHIFT         12
#define REG_0B_RESERVED_17_14_MASK          (0xF << REG_0B_RESERVED_17_14_SHIFT)
#define REG_0B_RESERVED_17_14(n)            BITFIELD_VAL(REG_0B_RESERVED_17_14, n)

// REG_0C
#define REG_0C_CFG_RXINTR_MSK_SHIFT         0
#define REG_0C_CFG_RXINTR_MSK_MASK          (0xFFFF << REG_0C_CFG_RXINTR_MSK_SHIFT)
#define REG_0C_CFG_RXINTR_MSK(n)            BITFIELD_VAL(REG_0C_CFG_RXINTR_MSK, n)

// REG_0D
#define REG_0D_CFG_HSHOST_DISC_MSK          (1 << 0)
#define REG_0D_CFG_POL_CKSIE60              (1 << 1)
#define REG_0D_CFG_RESETN_HSTXP             (1 << 2)
#define REG_0D_CFG_CKSIE60_EN               (1 << 3)
#define REG_0D_CFG_TXP_MODE                 (1 << 4)
#define REG_0D_CFG_CKOSC_EN                 (1 << 5)
#define REG_0D_CFG_POL_OSC                  (1 << 6)
#define REG_0D_CFG_CKPRE_EN                 (1 << 7)
#define REG_0D_CFG_RX_P_SEL                 (1 << 8)
#define REG_0D_CFG_RERESTN_HSRXP            (1 << 9)
#define REG_0D_CFG_POL_RXP                  (1 << 10)
#define REG_0D_CFG_CKRXP_EN                 (1 << 11)
#define REG_0D_CFG_PEBUF_THRD_SHIFT         12
#define REG_0D_CFG_PEBUF_THRD_MASK          (0xF << REG_0D_CFG_PEBUF_THRD_SHIFT)
#define REG_0D_CFG_PEBUF_THRD(n)            BITFIELD_VAL(REG_0D_CFG_PEBUF_THRD, n)

// REG_0E
#define REG_0E_RB_RXERRSTATUS_RAW_SHIFT     0
#define REG_0E_RB_RXERRSTATUS_RAW_MASK      (0xFFFF << REG_0E_RB_RXERRSTATUS_RAW_SHIFT)
#define REG_0E_RB_RXERRSTATUS_RAW(n)        BITFIELD_VAL(REG_0E_RB_RXERRSTATUS_RAW, n)

// REG_0F
#define REG_0F_RB_RXERRSTATUS_MSKD_SHIFT    0
#define REG_0F_RB_RXERRSTATUS_MSKD_MASK     (0xFFFF << REG_0F_RB_RXERRSTATUS_MSKD_SHIFT)
#define REG_0F_RB_RXERRSTATUS_MSKD(n)       BITFIELD_VAL(REG_0F_RB_RXERRSTATUS_MSKD, n)

// REG_10
#define REG_10_RB_HSHOST_DISC_RAW           (1 << 0)

// REG_11
#define REG_11_RB_HSHOST_DISC_MSKED         (1 << 0)

// REG_12
#define REG_12_CFG_EN_SETERR_RX_SHIFT       0
#define REG_12_CFG_EN_SETERR_RX_MASK        (0xFFFF << REG_12_CFG_EN_SETERR_RX_SHIFT)
#define REG_12_CFG_EN_SETERR_RX(n)          BITFIELD_VAL(REG_12_CFG_EN_SETERR_RX, n)

// REG_13
#define REG_13_CFG_TXSTART_DLY_SEL_SHIFT    0
#define REG_13_CFG_TXSTART_DLY_SEL_MASK     (0x7 << REG_13_CFG_TXSTART_DLY_SEL_SHIFT)
#define REG_13_CFG_TXSTART_DLY_SEL(n)       BITFIELD_VAL(REG_13_CFG_TXSTART_DLY_SEL, n)
#define REG_13_RESERVED_38_26_SHIFT         3
#define REG_13_RESERVED_38_26_MASK          (0x1FFF << REG_13_RESERVED_38_26_SHIFT)
#define REG_13_RESERVED_38_26(n)            BITFIELD_VAL(REG_13_RESERVED_38_26, n)

// REG_14
#define REG_14_CALIB_TIME_15_0_SHIFT        0
#define REG_14_CALIB_TIME_15_0_MASK         (0xFFFF << REG_14_CALIB_TIME_15_0_SHIFT)
#define REG_14_CALIB_TIME_15_0(n)           BITFIELD_VAL(REG_14_CALIB_TIME_15_0, n)

// REG_15
#define REG_15_CALIB_TIME_19_16_SHIFT       0
#define REG_15_CALIB_TIME_19_16_MASK        (0xF << REG_15_CALIB_TIME_19_16_SHIFT)
#define REG_15_CALIB_TIME_19_16(n)          BITFIELD_VAL(REG_15_CALIB_TIME_19_16, n)
#define REG_15_RESERVED_49_39_SHIFT         4
#define REG_15_RESERVED_49_39_MASK          (0x7FF << REG_15_RESERVED_49_39_SHIFT)
#define REG_15_RESERVED_49_39(n)            BITFIELD_VAL(REG_15_RESERVED_49_39, n)
#define REG_15_CALIB_START                  (1 << 15)

// REG_16
#define REG_16_INTR_MASK_SHIFT              0
#define REG_16_INTR_MASK_MASK               (0x3 << REG_16_INTR_MASK_SHIFT)
#define REG_16_INTR_MASK(n)                 BITFIELD_VAL(REG_16_INTR_MASK, n)
#define REG_16_RESERVED_63_50_SHIFT         2
#define REG_16_RESERVED_63_50_MASK          (0x3FFF << REG_16_RESERVED_63_50_SHIFT)
#define REG_16_RESERVED_63_50(n)            BITFIELD_VAL(REG_16_RESERVED_63_50, n)

// REG_17
#define REG_17_O_CNT_PLL_15_0_SHIFT         0
#define REG_17_O_CNT_PLL_15_0_MASK          (0xFFFF << REG_17_O_CNT_PLL_15_0_SHIFT)
#define REG_17_O_CNT_PLL_15_0(n)            BITFIELD_VAL(REG_17_O_CNT_PLL_15_0, n)

// REG_18
#define REG_18_O_CNT_PLL_24_16_SHIFT        0
#define REG_18_O_CNT_PLL_24_16_MASK         (0x1FF << REG_18_O_CNT_PLL_24_16_SHIFT)
#define REG_18_O_CNT_PLL_24_16(n)           BITFIELD_VAL(REG_18_O_CNT_PLL_24_16, n)

// REG_19
#define REG_19_O_INTR_STATUS_SHIFT          0
#define REG_19_O_INTR_STATUS_MASK           (0x3 << REG_19_O_INTR_STATUS_SHIFT)
#define REG_19_O_INTR_STATUS(n)             BITFIELD_VAL(REG_19_O_INTR_STATUS, n)

// REG_1A
#define REG_1A_HS_TEST                      (1 << 0)
#define REG_1A_FS_TEST                      (1 << 1)
#define REG_1A_USBINSERT_DET_EN             (1 << 2)
#define REG_1A_USBINSERT_INTR_EN            (1 << 3)
#define REG_1A_INTR_CLR                     (1 << 4)
#define REG_1A_POL_USB_RX_DP                (1 << 5)
#define REG_1A_POL_USB_RX_DM                (1 << 6)
#define REG_1A_DEBOUNCE_EN                  (1 << 7)
#define REG_1A_NOLS_MODE                    (1 << 8)
#define REG_1A_USB_INSERT_INTR_MSK          (1 << 9)
#define REG_1A_PHY_TEST_GOON                (1 << 10)

// REG_1B
#define REG_1B_CFG_HS_PDATA_SHIFT           0
#define REG_1B_CFG_HS_PDATA_MASK            (0xFF << REG_1B_CFG_HS_PDATA_SHIFT)
#define REG_1B_CFG_HS_PDATA(n)              BITFIELD_VAL(REG_1B_CFG_HS_PDATA, n)
#define REG_1B_CFG_HS_P_ON_SHIFT            8
#define REG_1B_CFG_HS_P_ON_MASK             (0xFF << REG_1B_CFG_HS_P_ON_SHIFT)
#define REG_1B_CFG_HS_P_ON(n)               BITFIELD_VAL(REG_1B_CFG_HS_P_ON, n)

// REG_20
#define REG_20_RB_BISTERR_CAUSE_SHIFT       0
#define REG_20_RB_BISTERR_CAUSE_MASK        (0x7 << REG_20_RB_BISTERR_CAUSE_SHIFT)
#define REG_20_RB_BISTERR_CAUSE(n)          BITFIELD_VAL(REG_20_RB_BISTERR_CAUSE, n)

// REG_21
#define REG_21_RB_BIST_FAIL                 (1 << 0)
#define REG_21_RB_BIST_DONE                 (1 << 1)
#define REG_21_RB_CHIRP_ON                  (1 << 2)
#define REG_21_RB_ADP_SNS_EN                (1 << 3)
#define REG_21_RB_ADP_PRB_EN                (1 << 4)
#define REG_21_RB_ADP_DISCHRG               (1 << 5)
#define REG_21_RB_ADP_CHRG                  (1 << 6)
#define REG_21_RB_UTMISRP_DISCHRGVBUS       (1 << 7)
#define REG_21_RB_UTMISRP_CHRGVBUS          (1 << 8)
#define REG_21_RB_UTMIOTG_IDPULLUP          (1 << 9)
#define REG_21_RB_UTMIOTG_DPPULLDOWN        (1 << 10)
#define REG_21_RB_UTMIOTG_DMPULLDOWN        (1 << 11)
#define REG_21_RB_UTMIOTG_DRVVBUS           (1 << 12)

// REG_22
#define REG_22_CFG_RESETNTX                 (1 << 0)
#define REG_22_CFG_RESETNRX                 (1 << 1)
#define REG_22_CFG_CKTX_EN                  (1 << 2)
#define REG_22_CFG_CKRX_EN                  (1 << 3)
#define REG_22_CFG_FORCE_TXCK               (1 << 4)
#define REG_22_CFG_FORCE_RXCK               (1 << 5)
#define REG_22_CFG_FORCE_CDRCK              (1 << 6)
#define REG_22_CFG_MODE_S_ON                (1 << 7)
#define REG_22_CFG_RESETNFRE                (1 << 8)
#define REG_22_CFG_RESETNOSC                (1 << 9)
#define REG_22_CFG_DIG_LOOP                 (1 << 10)
#define REG_22_RESERVED_83_78_SHIFT         11
#define REG_22_RESERVED_83_78_MASK          (0x1F << REG_22_RESERVED_83_78_SHIFT)
#define REG_22_RESERVED_83_78(n)            BITFIELD_VAL(REG_22_RESERVED_83_78, n)

// REG_23
#define REG_23_O_CNT_OSC_15_0_SHIFT         0
#define REG_23_O_CNT_OSC_15_0_MASK          (0xFFFF << REG_23_O_CNT_OSC_15_0_SHIFT)
#define REG_23_O_CNT_OSC_15_0(n)            BITFIELD_VAL(REG_23_O_CNT_OSC_15_0, n)

// REG_24
#define REG_24_O_CNT_OSC_19_16_SHIFT        0
#define REG_24_O_CNT_OSC_19_16_MASK         (0x1F << REG_24_O_CNT_OSC_19_16_SHIFT)
#define REG_24_O_CNT_OSC_19_16(n)           BITFIELD_VAL(REG_24_O_CNT_OSC_19_16, n)

// REG_25
#define REG_25_USB_STATUS_RX_DP             (1 << 0)
#define REG_25_USB_STATUS_RX_DM             (1 << 1)

// REG_30
#define REG_30_REV_REG30_SHIFT              0
#define REG_30_REV_REG30_MASK               (0xFFFF << REG_30_REV_REG30_SHIFT)
#define REG_30_REV_REG30(n)                 BITFIELD_VAL(REG_30_REV_REG30, n)

// REG_31
#define REG_31_REV_REG31_SHIFT              0
#define REG_31_REV_REG31_MASK               (0xFFFF << REG_31_REV_REG31_SHIFT)
#define REG_31_REV_REG31(n)                 BITFIELD_VAL(REG_31_REV_REG31, n)

// REG_32
#define REG_32_REV_REG32_SHIFT              0
#define REG_32_REV_REG32_MASK               (0xFFFF << REG_32_REV_REG32_SHIFT)
#define REG_32_REV_REG32(n)                 BITFIELD_VAL(REG_32_REV_REG32, n)

// REG_33
#define REG_33_CFG_ANA_REV_SHIFT            0
#define REG_33_CFG_ANA_REV_MASK             (0xFFFF << REG_33_CFG_ANA_REV_SHIFT)
#define REG_33_CFG_ANA_REV(n)               BITFIELD_VAL(REG_33_CFG_ANA_REV, n)

// REG_61
#define REG_61_USB_LDO_VOUT_SET_SHIFT       0
#define REG_61_USB_LDO_VOUT_SET_MASK        (0xF << REG_61_USB_LDO_VOUT_SET_SHIFT)
#define REG_61_USB_LDO_VOUT_SET(n)          BITFIELD_VAL(REG_61_USB_LDO_VOUT_SET, n)
#define REG_61_USB_LDO_OCP                  (1 << 4)
#define REG_61_USB_LDO_SOFT_START           (1 << 5)
#define REG_61_USB_LDO_PULL_DOWN            (1 << 6)
#define REG_61_USB_LDO_BYPASS               (1 << 7)
#define REG_61_USB_LDO_EN                   (1 << 8)
#define REG_61_USB_LDO_SOFT_CNT_SHIFT       9
#define REG_61_USB_LDO_SOFT_CNT_MASK        (0xF << REG_61_USB_LDO_SOFT_CNT_SHIFT)
#define REG_61_USB_LDO_SOFT_CNT(n)          BITFIELD_VAL(REG_61_USB_LDO_SOFT_CNT, n)

// REG_62
#define REG_62_PLL_LDO_PU                   (1 << 0)
#define REG_62_PLL_LDO_VREF_SHIFT           1
#define REG_62_PLL_LDO_VREF_MASK            (0x7 << REG_62_PLL_LDO_VREF_SHIFT)
#define REG_62_PLL_LDO_VREF(n)              BITFIELD_VAL(REG_62_PLL_LDO_VREF, n)
#define REG_62_PLL_LDO_PRECHARGE            (1 << 4)
#define REG_62_PLL_PFD_DNDLY_SHIFT          5
#define REG_62_PLL_PFD_DNDLY_MASK           (0x7 << REG_62_PLL_PFD_DNDLY_SHIFT)
#define REG_62_PLL_PFD_DNDLY(n)             BITFIELD_VAL(REG_62_PLL_PFD_DNDLY, n)
#define REG_62_PLL_PFD_RST                  (1 << 8)
#define REG_62_PLL_PFD_UPDLY_SHIFT          9
#define REG_62_PLL_PFD_UPDLY_MASK           (0x7 << REG_62_PLL_PFD_UPDLY_SHIFT)
#define REG_62_PLL_PFD_UPDLY(n)             BITFIELD_VAL(REG_62_PLL_PFD_UPDLY, n)
#define REG_62_PLL_LPF_BW_SEL               (1 << 12)
#define REG_62_PLL_UP_CP                    (1 << 13)

// REG_63
#define REG_63_PLL_CP_REN_SHIFT             0
#define REG_63_PLL_CP_REN_MASK              (0xF << REG_63_PLL_CP_REN_SHIFT)
#define REG_63_PLL_CP_REN(n)                BITFIELD_VAL(REG_63_PLL_CP_REN, n)
#define REG_63_PLL_CP_SWRC_SHIFT            4
#define REG_63_PLL_CP_SWRC_MASK             (0x3 << REG_63_PLL_CP_SWRC_SHIFT)
#define REG_63_PLL_CP_SWRC(n)               BITFIELD_VAL(REG_63_PLL_CP_SWRC, n)
#define REG_63_PLL_PRECHARGE                (1 << 6)
#define REG_63_PLL_PU_VCO                   (1 << 7)
#define REG_63_PLL_VAA_SWRA_SHIFT           8
#define REG_63_PLL_VAA_SWRA_MASK            (0x3 << REG_63_PLL_VAA_SWRA_SHIFT)
#define REG_63_PLL_VAA_SWRA(n)              BITFIELD_VAL(REG_63_PLL_VAA_SWRA, n)
#define REG_63_PLL_VAA_SPD_SHIFT            10
#define REG_63_PLL_VAA_SPD_MASK             (0x7 << REG_63_PLL_VAA_SPD_SHIFT)
#define REG_63_PLL_VAA_SPD(n)               BITFIELD_VAL(REG_63_PLL_VAA_SPD, n)
#define REG_63_PLL_PU_DIG                   (1 << 13)
#define REG_63_PLL_SDM_CLK_SET              (1 << 14)
#define REG_63_PLL_TST_EN                   (1 << 15)

// REG_64
#define REG_64_PLL_DIG_SWRC_SHIFT           0
#define REG_64_PLL_DIG_SWRC_MASK            (0x3 << REG_64_PLL_DIG_SWRC_SHIFT)
#define REG_64_PLL_DIG_SWRC(n)              BITFIELD_VAL(REG_64_PLL_DIG_SWRC, n)
#define REG_64_PLL_DIV_DELAY_SHIFT          2
#define REG_64_PLL_DIV_DELAY_MASK           (0xF << REG_64_PLL_DIV_DELAY_SHIFT)
#define REG_64_PLL_DIV_DELAY(n)             BITFIELD_VAL(REG_64_PLL_DIV_DELAY, n)
#define REG_64_PLL_DIV_FREE_SHIFT           6
#define REG_64_PLL_DIV_FREE_MASK            (0x3 << REG_64_PLL_DIV_FREE_SHIFT)
#define REG_64_PLL_DIV_FREE(n)              BITFIELD_VAL(REG_64_PLL_DIV_FREE, n)
#define REG_64_PLL_DIV_INT_SHIFT            8
#define REG_64_PLL_DIV_INT_MASK             (0x7F << REG_64_PLL_DIV_INT_SHIFT)
#define REG_64_PLL_DIV_INT(n)               BITFIELD_VAL(REG_64_PLL_DIV_INT, n)

// REG_65
#define REG_65_PLL_DIV_SET_SYNCLK_SHIFT     0
#define REG_65_PLL_DIV_SET_SYNCLK_MASK      (0x7 << REG_65_PLL_DIV_SET_SYNCLK_SHIFT)
#define REG_65_PLL_DIV_SET_SYNCLK(n)        BITFIELD_VAL(REG_65_PLL_DIV_SET_SYNCLK, n)
#define REG_65_PLL_CAL_EN                   (1 << 3)
#define REG_65_PLL_RETB                     (1 << 4)
#define REG_65_PLL_FORCE_1K                 (1 << 5)
#define REG_65_PLL_1K_ERR23                 (1 << 6)
#define REG_65_PLL_1K_LONG                  (1 << 7)
#define REG_65_PLL_1K_RSTB                  (1 << 8)
#define REG_65_PLL_1K_WIN_SHIFT             9
#define REG_65_PLL_1K_WIN_MASK              (0x7 << REG_65_PLL_1K_WIN_SHIFT)
#define REG_65_PLL_1K_WIN(n)                BITFIELD_VAL(REG_65_PLL_1K_WIN, n)

// REG_66
#define REG_66_REG_USBPLL_RSTN_DR           (1 << 0)
#define REG_66_REG_USBPLL_RSTN              (1 << 1)
#define REG_66_REG_USBPLL_CLK_FBC_EDGE      (1 << 2)
#define REG_66_REG_USBPLL_INT_DEC_SEL_SHIFT 3
#define REG_66_REG_USBPLL_INT_DEC_SEL_MASK  (0x7 << REG_66_REG_USBPLL_INT_DEC_SEL_SHIFT)
#define REG_66_REG_USBPLL_INT_DEC_SEL(n)    BITFIELD_VAL(REG_66_REG_USBPLL_INT_DEC_SEL, n)
#define REG_66_REG_USBPLL_DITHER_BYPASS     (1 << 6)
#define REG_66_REG_USBPLL_PRESCALER_DEL_SEL_SHIFT 7
#define REG_66_REG_USBPLL_PRESCALER_DEL_SEL_MASK (0xF << REG_66_REG_USBPLL_PRESCALER_DEL_SEL_SHIFT)
#define REG_66_REG_USBPLL_PRESCALER_DEL_SEL(n) BITFIELD_VAL(REG_66_REG_USBPLL_PRESCALER_DEL_SEL, n)
#define REG_66_REG_USBPLL_PU_PLL_DR         (1 << 11)
#define REG_66_REG_USBPLL_PU_PLL            (1 << 12)

// REG_67
#define REG_67_REG_USBPLL_DIV_DR            (1 << 0)
#define REG_67_REG_USBPLL_DIV_FRAC_SHIFT    1
#define REG_67_REG_USBPLL_DIV_FRAC_MASK     (0x3 << REG_67_REG_USBPLL_DIV_FRAC_SHIFT)
#define REG_67_REG_USBPLL_DIV_FRAC(n)       BITFIELD_VAL(REG_67_REG_USBPLL_DIV_FRAC, n)
#define REG_67_REG_USBPLL_DIV_INT_SHIFT     3
#define REG_67_REG_USBPLL_DIV_INT_MASK      (0x7F << REG_67_REG_USBPLL_DIV_INT_SHIFT)
#define REG_67_REG_USBPLL_DIV_INT(n)        BITFIELD_VAL(REG_67_REG_USBPLL_DIV_INT, n)

// REG_6A
#define REG_6A_REG_USBPLL_FREQ_EN           (1 << 3)
#define REG_6A_EN960M_USB                   (1 << 5)
#define REG_6A_ENUSB_DIG                    (1 << 6)
#define REG_6A_ENUSB_PS                     (1 << 7)

// REG_6B
#define REG_6B_REG_USBPLL_FREQ_OFFSET_SHIFT 0
#define REG_6B_REG_USBPLL_FREQ_OFFSET_MASK  (0x1FFFF << REG_6B_REG_USBPLL_FREQ_OFFSET_SHIFT)
#define REG_6B_REG_USBPLL_FREQ_OFFSET(n)    BITFIELD_VAL(REG_6B_REG_USBPLL_FREQ_OFFSET, n)

// REG_6C
#define REG_6C_CHIP_ADDR_I2C_SHIFT          0
#define REG_6C_CHIP_ADDR_I2C_MASK           (0x7F << REG_6C_CHIP_ADDR_I2C_SHIFT)
#define REG_6C_CHIP_ADDR_I2C(n)             BITFIELD_VAL(REG_6C_CHIP_ADDR_I2C, n)

// REG_6D
#define REG_6D_REG_USBPLL_LDOPRECHG_TIMER_SHIFT 0
#define REG_6D_REG_USBPLL_LDOPRECHG_TIMER_MASK (0x1F << REG_6D_REG_USBPLL_LDOPRECHG_TIMER_SHIFT)
#define REG_6D_REG_USBPLL_LDOPRECHG_TIMER(n) BITFIELD_VAL(REG_6D_REG_USBPLL_LDOPRECHG_TIMER, n)
#define REG_6D_REG_USBPLL_PRECHG_TIMER_SHIFT 5
#define REG_6D_REG_USBPLL_PRECHG_TIMER_MASK (0x1F << REG_6D_REG_USBPLL_PRECHG_TIMER_SHIFT)
#define REG_6D_REG_USBPLL_PRECHG_TIMER(n)   BITFIELD_VAL(REG_6D_REG_USBPLL_PRECHG_TIMER, n)
#define REG_6D_REG_USBPLL_UP_TIMER_PD_SHIFT 10
#define REG_6D_REG_USBPLL_UP_TIMER_PD_MASK  (0x1F << REG_6D_REG_USBPLL_UP_TIMER_PD_SHIFT)
#define REG_6D_REG_USBPLL_UP_TIMER_PD(n)    BITFIELD_VAL(REG_6D_REG_USBPLL_UP_TIMER_PD, n)

enum USBPHY_REG_T {
    USBPHY_REG_00   = 0x00,
    USBPHY_REG_01,
    USBPHY_REG_02,
    USBPHY_REG_03,
    USBPHY_REG_04,
    USBPHY_REG_05,
    USBPHY_REG_06,
    USBPHY_REG_07,
    USBPHY_REG_08,
    USBPHY_REG_09,
    USBPHY_REG_0A,
    USBPHY_REG_0B,
    USBPHY_REG_0C,
    USBPHY_REG_0D,
    USBPHY_REG_0E,
    USBPHY_REG_0F,
    USBPHY_REG_10,
    USBPHY_REG_11,
    USBPHY_REG_12,
    USBPHY_REG_13,
    USBPHY_REG_14,
    USBPHY_REG_15,
    USBPHY_REG_16,
    USBPHY_REG_17,
    USBPHY_REG_18,
    USBPHY_REG_19,
    USBPHY_REG_1A,
    USBPHY_REG_1B,

    USBPHY_REG_20   = 0x20,
    USBPHY_REG_21,
    USBPHY_REG_22,
    USBPHY_REG_23,
    USBPHY_REG_24,
    USBPHY_REG_25,

    USBPHY_REG_30   = 0x30,
    USBPHY_REG_31,
    USBPHY_REG_32,

    USBPHY_REG_61   = 0x61,
    USBPHY_REG_62,
    USBPHY_REG_63,
    USBPHY_REG_64,
    USBPHY_REG_65,
    USBPHY_REG_66,
    USBPHY_REG_67,
    USBPHY_REG_68,
    USBPHY_REG_69,
    USBPHY_REG_6A,
    USBPHY_REG_6B,
    USBPHY_REG_6C,
    USBPHY_REG_6D,
};

#endif

