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
#ifndef __SYS_BT_CFG_H__
#define __SYS_BT_CFG_H__

#define BTIF_DISABLED  0
#define BTIF_ENABLED 1

#define BTIF_AVRCP_ADVANCED_CONTROLLER

#define BTIF_AV_WORKER  BTIF_ENABLED

#define SYS_MAX_A2DP_STREAMS    14

#define BTIF_SBC_ENCODER   BTIF_ENABLED
#define BTIF_SBC_DECODER   BTIF_ENABLED

#define SYS_MAX_AVRCP_CHNS  2

#define BTIF_AVRCP_NUM_PLAYER_SETTINGS 4

#define BTIF_AVRCP_NUM_MEDIA_ATTRIBUTES        7

#define  BTIF_AVRCP_VERSION_1_3_ONLY   BTIF_DISABLED

#define  BTIF_L2CAP_PRIORITY BTIF_DISABLED

#define  BTIF_XA_STATISTICS

#define  BTIF_L2CAP_NUM_ENHANCED_CHANNELS 0

#define  BTIF_BT_BEST_SYNC_CONFIG   BTIF_ENABLED

#define  BTIF_HCI_HOST_FLOW_CONTROL BTIF_ENABLED

#define  BTIF_DEFAULT_ACCESS_MODE_PAIR   BTIF_BAM_GENERAL_ACCESSIBLE

#define BTIF_BT_DEFAULT_PAGE_SCAN_WINDOW 0x12

#define BTIF_MAP_SESSION_NUM 2

/*---------------------------------------------------------------------------
 * BT_DEFAULT_PAGE_SCAN_INTERVAL constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL 0x800

/*---------------------------------------------------------------------------
 * BT_DEFAULT_INQ_SCAN_WINDOW constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_INQ_SCAN_WINDOW 0x12

/*---------------------------------------------------------------------------
 * BT_DEFAULT_INQ_SCAN_INTERVAL constant
 *
 *     See BT_DEFAULT_PAGE_SCAN_WINDOW.
 */
#define BTIF_BT_DEFAULT_INQ_SCAN_INTERVAL 0x800

#define BTIF_BT_DEFAULT_PAGE_TIMEOUT_IN_MS              5000

#define BTIF_SPP_CLIENT BTIF_ENABLED

#define BTIF_SPP_SERVER BTIF_ENABLED

#define BTIF_RF_SEND_CONTROL  BTIF_DISABLED

#define BTIF_MULTITASKING

#define BTIF_SECURITY
#define BTIF_BLE_APP_DATAPATH_SERVER

#if defined (__AI_VOICE__) || defined (BISTO_ENABLED)
#define BTIF_DIP_DEVICE
#endif

//#define HF_CUSTOM_FEATURE_RESERVED          (0x01 << 0)
#define BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT    (0x03 << 0)
#define BTIF_HF_CUSTOM_FEATURE_DOCK              (0x01 << 2)
#define BTIF_HF_CUSTOM_FEATURE_SIRI_REPORT       (0x01 << 3)
#define BTIF_HF_CUSTOM_FEATURE_NR_REPORT         (0x01 << 4)


#ifndef BTIF_SUPPORT_SIRI
#define BTIF_SUPPORT_SIRI
#endif
//#define HF_CUSTOM_FEATURE_SUPPORT           (HF_CUSTOM_FEATURE_BATTERY_REPORT | HF_CUSTOM_FEATURE_SIRI_REPORT)
#ifndef BTIF_HF_CUSTOM_FEATURE_SUPPORT
#ifdef BTIF_SUPPORT_SIRI
#define BTIF_HF_CUSTOM_FEATURE_SUPPORT           (BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT | BTIF_HF_CUSTOM_FEATURE_SIRI_REPORT)
#else
#define BTIF_HF_CUSTOM_FEATURE_SUPPORT           (BTIF_HF_CUSTOM_FEATURE_BATTERY_REPORT)
#endif /*SUPPORT_SIRI*/
#endif /*HFt _CUSTOM_FEATURE_SUPPORT*/



/*
  *  default product  features
*/

#define  __BTIF_EARPHONE__

#define  __BTIF_AUTOPOWEROFF__

#if !defined(BLE_ONLY_ENABLED)
#define  __BTIF_BT_RECONNECT__
#endif

#define __BTIF_SNIFF__

#define BTIF_NUM_BT_DEVICES          2

#endif /*__SYS_BT_CFG_H__*/

