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

#ifndef __BT_SYS_CFG_H__
#define __BT_SYS_CFG_H__

#include "cobt.h"

/* for debug usage */
#if !defined(DEBUG)
#define DEBUG 0
#endif

#define ESCO_ENABLE 1

#define ADS_IDE    0
#define BA_GCC     1

#if DEBUG == 1
#define DBG_DEBUG_PRINT_ENABLE                  1
#define DBG_COBUF_NEED_STATISTIC                1
#else  /*must NOT change the following macro*/
#define DBG_DEBUG_PRINT_ENABLE                  0
#define DBG_COBUF_NEED_STATISTIC                0
#endif

#define DBG_PPBUFF_NEED_STATISTIC               1

/* config memory block size and count */
#define COBUF_SIZE_N1    8
#define COBUF_SIZE_N2    16
#define COBUF_SIZE_N3    32
#define COBUF_SIZE_N4    64
#define COBUF_SIZE_N5    128
#define COBUF_SIZE_N6    ((L2CAP_DEFAULT_MTU+3)/4*4)

#if defined(CHIP_BEST1402) || defined(CHIP_BEST1400)
#define COBUF_NUMS_N1    32
#define COBUF_NUMS_N2    45
#define COBUF_NUMS_N3    39
#define COBUF_NUMS_N4    20
#define COBUF_NUMS_N5    10
#define COBUF_NUMS_N6    6
#else
#define COBUF_NUMS_N1    64
#define COBUF_NUMS_N2    90
#define COBUF_NUMS_N3    78
#define COBUF_NUMS_N4    40
#define COBUF_NUMS_N5    20
#define COBUF_NUMS_N6    18
#endif

/* SCO */
#ifdef CVSD_BYPASS
#define BTM_SYNC_CONN_AUDIO_SETTING_DEFAULT            0x0040
#else
#define BTM_SYNC_CONN_AUDIO_SETTING_DEFAULT            0x0060
#endif
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_CODING_LINEAR   0x0000   /* Linear */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_CODING_ULAW     0x0100   /* u-law */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_CODING_ALAW     0x0200   /* a-law */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_DATA_ONES       0x0000   /* 1's complement */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_DATA_TWOS       0x0040   /* 2's complement */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_DATA_SM         0x0080   /* Sign-Magnitude */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_SAMPLE_8BIT     0x0000   /* 8 bit */
#define BTM_SYNC_CONN_AUDIO_SETTING_IN_SAMPLE_16BIT    0x0020   /* 16 bit */
#define BTM_SYNC_CONN_AUDIO_SETTING_CVSD               0x0000   /* CVSD */
#define BTM_SYNC_CONN_AUDIO_SETTING_ULAW               0x0001   /* u-LAW */
#define BTM_SYNC_CONN_AUDIO_SETTING_ALAW               0x0002   /* A-LAW */
#define BTM_SYNC_CONN_AUDIO_SETTING_TRANSPNT 	       0x0003   /* msbc */
#define BTM_SYNC_CONN_AUDIO_SETTING_MSBC               0x0060

#define BTM_SYNC_CONN_AUDIO_PARAM_SCO      1
#define BTM_SYNC_CONN_AUDIO_PARAM_S1       2
#define BTM_SYNC_CONN_AUDIO_PARAM_S2       3
#define BTM_SYNC_CONN_AUDIO_PARAM_S3       4
#define BTM_SYNC_CONN_AUDIO_PARAM_S4       5
#define BTM_SYNC_CONN_AUDIO_PARAM_CUSTOM   6
#define BTM_SYNC_CONN_AUDIO_PARAM_T1       7
#define BTM_SYNC_CONN_AUDIO_PARAM_T2       8

#define BTM_SYNC_CONN_AUDIO_DEFAULT_PARMS           BTM_SYNC_CONN_AUDIO_PARAM_S4

/* HFP */
#define HFP_CMD_FLOW_CONTROL_ENABLE        1
#define HFP_CMD_SYST_TX_TIMEOUT_VAL_MS     3000

#define HFP_HF_CHANNEL 7

#define HFP_HF_FEAT_ECNR        0x00000001
#define HFP_HF_FEAT_3WAY        0x00000002
#define HFP_HF_FEAT_CLI         0x00000004
#define HFP_HF_FEAT_VR          0x00000008
#define HFP_HF_FEAT_RVC         0x00000010
#define HFP_HF_FEAT_ECS         0x00000020
#define HFP_HF_FEAT_ECC         0x00000040
#define HFP_HF_FEAT_CODEC       0x00000080
#define HFP_HF_FEAT_HF_IND      0x00000100
#define HFP_HF_FEAT_ESCO_S4_T2  0x00000200

#define HFP_HF_SDP_FEAT_MASK    0x001f
#define HFP_HF_SDP_FEAT_WBS     0x0020

#define HFP_AG_FEAT_3WAY        0x00000001
#define HFP_AG_FEAT_ECNR        0x00000002
#define HFP_AG_FEAT_VR          0x00000004
#define HFP_AG_FEAT_INBAND      0x00000008
#define HFP_AG_FEAT_VTAG        0x00000010
#define HFP_AG_FEAT_REJ_CALL    0x00000020
#define HFP_AG_FEAT_ECS         0x00000040
#define HFP_AG_FEAT_ECC         0x00000080
#define HFP_AG_FEAT_EXT_ERR     0x00000100
#define HFP_AG_FEAT_CODEC       0x00000200
#define HFP_AG_FEAT_HF_IND      0x00000400
#define HFP_AG_FEAT_ESCO_S4_T2  0x00000800

#define HFP_AG_SDP_FEAT_MASK    0x001f
#define HFP_AG_SDP_FEAT_WBS     0x0020

#if defined(HFP_1_6_ENABLE)
#define HFP_HF_FEATURES ( \
                HFP_HF_FEAT_CLI   | \
                HFP_HF_FEAT_RVC   | \
                HFP_HF_FEAT_ECS   | \
                HFP_HF_FEAT_ECC   | \
                HFP_HF_FEAT_CODEC | \
                HFP_HF_FEAT_ECNR  | \
                HFP_HF_FEAT_3WAY  | \
                HFP_HF_FEAT_VR    | \
                HFP_HF_FEAT_ESCO_S4_T2 \
                )
#define HFP_HF_SDP_FEATURES ((HFP_HF_FEATURES & HFP_HF_SDP_FEAT_MASK) | HFP_HF_SDP_FEAT_WBS)

#define HFP_AG_FEATURES ( \
                HFP_AG_FEAT_3WAY     | \
                HFP_AG_FEAT_ECNR     | \
                HFP_AG_FEAT_REJ_CALL | \
                HFP_AG_FEAT_ECS      | \
                HFP_AG_FEAT_ECC      | \
                HFP_AG_FEAT_CODEC    | \
                HFP_AG_FEAT_ESCO_S4_T2 \
                )
#define HFP_AG_SDP_FEATURES ((HFP_AG_FEATURES & HFP_AG_SDP_FEAT_MASK) | HFP_AG_SDP_FEAT_WBS)
#else
#define HFP_HF_FEATURES ( \
                HFP_HF_FEAT_CLI   | \
                HFP_HF_FEAT_RVC   | \
                HFP_HF_FEAT_ECS   | \
                HFP_HF_FEAT_ECC     \
                )
#define HFP_HF_SDP_FEATURES (HFP_HF_FEATURES & HFP_HF_SDP_FEAT_MASK)

#define HFP_AG_FEATURES ( \
                HFP_AG_FEAT_REJ_CALL | \
                HFP_AG_FEAT_ECS      | \
                HFP_AG_FEAT_ECC      | \
                )
#define HFP_AG_SDP_FEATURES ((HFP_AG_FEATURES & HFP_AG_SDP_FEAT_MASK) | HFP_AG_SDP_FEAT_WBS)
#endif

/* btm */
/*
btm upper layers (l2cap,profiles) use ppb to pass incoming data.
we cannot change all usage now.
it is a bad idea for alloc ppb (only one time used in callback) for incoming data.
so to setup a fix buffer to alloc ppb inplace to avoid alloc ppb dynamicly failure.
*/
#define HCIBUFF_STATISTIC_ENABLE 1

#define CFG_BTM_USE_INPLACE_BUFFER_FOR_ACL       1
#define CFG_BTM_DISC_ACL_IN_BTM_TIMER            0
#define CFG_BTM_DROP_TX_ACL_WHEN_DISCONNECT_PENDING 1 // meaningless when CFG_BTM_DISC_ACL_IN_BTM_TIMER=1

#define MAX_SDP_SOCK_NUMBER                      2
#define NUM_BT_DEVICES                           2
#define NUM_SCO_CONNS                            2
#define NUM_BLE_DEVICES                          2

#define CFG_VOICE_SETTING_DEFAULT                0x0043

#if defined(__3M_PACK__)
#define CFG_HCI_ACL_DATA_SIZE                    1040
#else
#define CFG_HCI_ACL_DATA_SIZE                    800
#endif

#define CFG_HCI_EVT_DATA_SIZE                    300
#define CFG_HCI_SCO_DATA_SIZE                    180
#define CFG_HCI_NUM_ACL_BUFFERS                  6
#define CFG_HCI_NUM_SCO_BUFFERS                  6

#define BTM_DEVICEDB_SIZE                        6
#define BTM_PINCODE_MAX_LEN                      8    /*include '\0'*/
#define BTM_NAME_MAX_LEN                         248  /*include '\0'*/

#define BTM_CFG_TIMEOUT_BEFORE_LOWPOWER               5
#define BTM_INQUIRY_RESULT_UNUSED_TIMEOUT             30   /*seconds, decide the time out to destory an inquiry result structure*/
#define BTM_DISCONN_WAITING_TIME                      1    /* changed by marvin.zhu. seconds, decide the timeout be disconnect an acl link after it is not used by l2cap*/
#define BTM_HCI_HOST_FLOW_CONTROL_ENABLE              (1)
#define BTM_BLE_USE_INTERNAL_QUEUE_CACHE_ENABLE       (1)  /* if 1, for ble acl data, stack will use a internal queue to cache ble data, hci buff will be release imm */
#define BTM_FC_BLE_TX_USE_BT_BUFFER_FORCE             (0)
#define BTM_FC_HOST_TO_CONTROLLER_RESV_ACL_BUF_NUM    (1)
#define BTM_FC_HOST_TO_CONTROLLER_CMD_BUF_NUM         (24)
#define BTM_FC_HOST_TO_CONTROLLER_ACL_BUF_NUM         (10) /* this value will be overide by read buffer size command reaponse, tx queue will be this value */
#define BTM_FC_CONTROLLER_TO_HOST_ACL_BUF_NUM         (CFG_HCI_NUM_ACL_BUFFERS)
#if BTM_BLE_USE_INTERNAL_QUEUE_CACHE_ENABLE==1
#define BTM_BLE_INTERNAL_QUEUE_BUF_NUM                (BTM_FC_CONTROLLER_TO_HOST_ACL_BUF_NUM)
#endif

#if BTM_BLE_USE_INTERNAL_QUEUE_CACHE_ENABLE==1
#if (BTM_BLE_INTERNAL_QUEUE_BUF_NUM<BTM_FC_CONTROLLER_TO_HOST_ACL_BUF_NUM)
#error "BTM_BLE_INTERNAL_QUEUE_BUF_NUM should >= BTM_FC_CONTROLLER_TO_HOST_ACL_BUF_NUM"
#endif
#endif

#define BTM_FC_CONTROLLER_TO_HOST_EVT_BUF_NUM         (25)
#define BTM_FC_HOST_TO_CONTROLLER_SCO_BUF_NUM         (6)
#define BTM_FC_CONTROLLER_TO_HOST_SCO_BUF_NUM         (6)
#define BTM_CFG_CON_ACL_MAX                           (7)

/* audio connection config */
#define HCI_CFG_SYNC_TX_BANDWIDTH            0x00001F40
#define HCI_CFG_SYNC_RX_BANDWIDTH            0x00001F40
#define HCI_CFG_SYNC_MAX_LATENCY             0xffff
#define HCI_CFG_SYNC_RETX_EFFORT             0x2
//#define HCI_CFG_SYNC_PKT_TYPE              HCI_PKT_TYPE_HV3
#if ESCO_ENABLE
#define HCI_CFG_SYNC_SCO_PKTS                (PACKET_TYPE_HV1 | PACKET_TYPE_HV2 | PACKET_TYPE_HV3)
#define HCI_CFG_SYNC_PKT_TYPE                (PACKET_TYPE_HV1 | PACKET_TYPE_HV2 | PACKET_TYPE_HV3 | PACKET_TYPE_EV3)
#else
#define HCI_CFG_SYNC_PKT_TYPE                (PACKET_TYPE_HV1_FLAG | PACKET_TYPE_HV2_FLAG | PACKET_TYPE_HV3_FLAG)
#endif
#define HCI_CFG_INPUT_CODING                 INPUT_CODING_LINEAR
#define HCI_CFG_INPUT_DATA_FORMAT            INPUT_DATA_FORMAT_1S
#define HCI_CFG_INPUT_SAMPLE_SIZE            INPUT_SAMPLE_SIZE_8BITS
#if defined(HFP_1_6_ENABLE)
#define HCI_CFG_AIR_CODING_FORMAT            AIR_CODING_FORMAT_MSBC
#else
#define HCI_CFG_AIR_CODING_FORMAT            AIR_CODING_FORMAT_CVSD
#endif
#define HCI_CFG_LINEAR_PCM_BITPOS            0x00

#define HCI_CFG_VOICE_SETTING               (HCI_CFG_INPUT_CODING&  \
                                             HCI_CFG_INPUT_DATA_FORMAT&  \
                                             HCI_CFG_INPUT_SAMPLE_SIZE&  \
                                             HCI_CFG_AIR_CODING_FORMAT& \
                                             (HCI_CFG_LINEAR_PCM_BITPOS<<2))



#define HCI_CFG_SNIFF_MAX_INTERVAL           0x0320
#define HCI_CFG_SNIFF_MIN_INTERVAL           0x0300
#define HCI_CFG_SNIFF_ATTEMPT                0x08
#define HCI_CFG_SNIFF_TIMEOUT                0x00

#define HCI_CFG_LINK_POLICY_LOWPOWER         HCI_LP_ENABLE_ROLE_SWITCH_MASK \
                                             |HCI_LP_ENABLE_SNIFF_MODE_MASK 


/* 10*1.28 seconds */
#define HCI_CFG_INQUIRY_TIMEOUT              10
/* unlimited */
#define HCI_CFG_INQUIRY_RESPONSE_NUM         0

#define CFG_DEFAULT_PAGE_TIMEOUT             0x2000

#define CFG_COD_LIMITED_DISCOVERABLE_MODE    0x00002000
#define CFG_COD_POSITIONING                  0x00010000
#define CFG_COD_NETWORKING                   0x00020000
#define CFG_COD_RENDERING                    0x00040000
#define CFG_COD_CAPTURING                    0x00080000
#define CFG_COD_OBJECT_TRANSFER              0x00100000
#define CFG_COD_AUDIO                        0x00200000
#define CFG_COD_TELEPHONY                    0x00400000
#define CFG_COD_INFORMATION                  0x00800000
#define CFG_COD_MAJOR_MISCELLANEOUS          0x00000000
#define CFG_COD_MAJOR_COMPUTER               0x00000100
#define CFG_COD_MAJOR_PHONE                  0x00000200
#define CFG_COD_MAJOR_LAN_ACCESS_POINT       0x00000300
#define CFG_COD_MAJOR_AUDIO                  0x00000400
#define CFG_COD_MAJOR_PERIPHERAL             0x00000500
#define CFG_COD_MAJOR_IMAGING                0x00000600
#define CFG_COD_MAJOR_UNCLASSIFIED           0x00001F00
#define CFG_COD_MINOR_AUDIO_UNCLASSIFIED     0x00000000
#define CFG_COD_MINOR_AUDIO_HEADSET          0x00000004
#define CFG_COD_MINOR_AUDIO_HANDSFREE        0x00000008
#define CFG_COD_MINOR_AUDIO_MICROPHONE       0x00000010
#define CFG_COD_MINOR_AUDIO_LOUDSPEAKER      0x00000014
#define CFG_COD_MINOR_AUDIO_HEADPHONES       0x00000018
#define CFG_COD_MINOR_AUDIO_PORTABLEAUDIO    0x0000001C
#define CFG_COD_MINOR_AUDIO_CARAUDIO         0x00000020
#define CFG_COD_MINOR_AUDIO_SETTOPBOX        0x00000024
#define CFG_COD_MINOR_AUDIO_HIFIAUDIO        0x00000028
#define CFG_COD_MINOR_AUDIO_VCR              0x0000002C
#define CFG_COD_MINOR_AUDIO_VIDEOCAMERA      0x00000030
#define CFG_COD_MINOR_AUDIO_CAMCORDER        0x00000034
#define CFG_COD_MINOR_AUDIO_VIDEOMONITOR     0x00000038
#define CFG_COD_MINOR_AUDIO_VIDEOSPEAKER     0x0000003C
#define CFG_COD_MINOR_AUDIO_CONFERENCING     0x00000040
#define CFG_COD_MINOR_AUDIO_GAMING           0x00000048
#define CFG_CLASS_OF_DEVICE                  (CFG_COD_MAJOR_AUDIO|CFG_COD_MINOR_AUDIO_HEADSET|CFG_COD_AUDIO|CFG_COD_RENDERING)

// Minor device class for CFG_COD_MAJOR_PERIPHERAL
#define CFG_COD_MINOR_PERIPH_KEYBOARD       0x00000040
#define CFG_COD_MINOR_PERIPH_POINTING       0x00000080
#define CFG_COD_MINOR_PERIPH_COMBOKEY       0x000000C0

#ifdef _SCO_BTPCM_CHANNEL_
#define CFG_SYNC_CONFIG_PATH                 (0<<8|1<<4|1<<0) /* all links use hci */
#else
#define CFG_SYNC_CONFIG_PATH                 (0<<8|0<<4|0<<0) /* all links use hci */
#endif
#define CFG_SYNC_CONFIG_MAX_BUFFER           (0) /* (e)sco use Packet size */
#ifdef _CVSD_BYPASS_
#define CFG_SYNC_CONFIG_CVSD_BYPASS          (1) /* use pcm over hci */
#else
#define CFG_SYNC_CONFIG_CVSD_BYPASS          (0) /* use pcm over hci */
#endif

#define CFG_LP_SNIFF_MODE                    0x0000

#define EDR_ENABLED 0

#define CFG_BT_DEFAULT_PAGE_SCAN_WINDOW      0x12
#define CFG_BT_DEFAULT_PAGE_SCAN_INTERVAL    0x800
#define CFG_BT_DEFAULT_INQ_SCAN_WINDOW       0x12
#define CFG_BT_DEFAULT_INQ_SCAN_INTERVAL     0x800

#define CFG_INQ_TYPE    1
#define CFG_PAGE_TYPE   1

/* l2cap */
#define SSP_RECONNECT              1
#define L2CAP_AUTH_NEED_ENCRYPTION 1/* default value in spec */

#if defined(__3M_PACK__)
#define L2CAP_CFG_MTU                                               1021
#else
#define L2CAP_CFG_MTU                                               679
#endif
#define L2CAP_DEFAULT_MTU 	                                        L2CAP_CFG_MTU
#define L2CAP_MIN_MTU                                               48

#define L2CAP_DEFAULT_FLUSH_TO	                                    0xFFFF

#define L2CAP_DEFAULT_QOS_SERVICE_TYPE                              L2CAP_QOS_BEST_EFFORT 
#define L2CAP_DEFAULT_QOS_TOKEN_RATE                                0x00000000
#define L2CAP_DEFAULT_QOS_TOKEN_BUCKET_SIZE                         0x00000000
#define L2CAP_DEFAULT_QOS_PEEK_BANDWIDTH                            0x00000000
#define L2CAP_DEFAULT_QOS_LATENCY                                   0xffffffff
#define L2CAP_DEFAULT_QOS_DELAY_VARIATION                           0xffffffff


#define L2CAP_DEFAULT_RFC_MODE                                      L2CAP_MODE_BASE                  
#define L2CAP_DEFAULT_RFC_TXWINDOW                                  32      /*1 to 32*/
#define L2CAP_DEFAULT_RFC_MAXTRANSMIT                               32
#define L2CAP_DEFAULT_RFC_RETRANSMISSION_TIMEOUT                    1000
#define L2CAP_DEFAULT_RFC_MONITOR_TIMEOUT                           1000
#define L2CAP_DEFAULT_RFC_MPS                                       0xFFFF

#define L2CAP_DEFAULT_FCS_TYPE		L2CAP_FCS_TYPE_16_BIT

/* used for pp_buff */
#define L2CAP_PPB_HEAD_RESERVE      4       /*len+cid+control+sdulen  2+2+2+2*/
#define L2CAP_PPB_TAIL_RESERVE	    0        /* fcs 2 */
#define L2CAP_PPB_RESERVE  (L2CAP_PPB_HEAD_RESERVE + L2CAP_PPB_TAIL_RESERVE)

#define L2CAP_CFG_FLUSH_TO	                                         L2CAP_DEFAULT_FLUSH_TO

#define L2CAP_CFG_QOS_SERVICE_TYPE                                   L2CAP_DEFAULT_QOS_SERVICE_TYPE 
#define L2CAP_CFG_QOS_TOKEN_RATE                                     0x00000000
#define L2CAP_CFG_QOS_TOKEN_BUCKET_SIZE                              0x00000000
#define L2CAP_CFG_QOS_PEEK_BANDWIDTH                                 0x00000000
#define L2CAP_CFG_QOS_LATENCY                                        0xffffffff
#define L2CAP_CFG_QOS_DELAY_VARIATION                                0xffffffff

#define L2CAP_CFG_RFC_MODE                                           L2CAP_DEFAULT_RFC_MODE                   /*L2CAP_MODE_BASE*/
/* below is only available when rfc mode is not base mode*/
#define L2CAP_CFG_RFC_TXWINDOW                                       32      /*1 to 32*/
#define L2CAP_CFG_RFC_MAXTRANSMIT                                    32
#define L2CAP_CFG_RFC_RETRANSMISSION_TIMEOUT                         1000
#define L2CAP_CFG_RFC_MONITOR_TIMEOUT                                1000
#define L2CAP_CFG_RFC_MPS                                            0xFFFF

/* L2CAP_RTX: The Responsive Timeout eXpired timer is used to terminate
   the channel when the remote endpoint is unresponsive to signalling
   requests (min 1s, max 60s) */
#define L2CAP_CFG_RTX 60
/* L2CAP_RTX_MAXN: Maximum number of Request retransmissions before
   terminating the channel identified by the request. The decision
   should be based on the flush timeout of the signalling link. If the
   flush timeout is infinite, no retransmissions should be performed */
#define L2CAP_CFG_RTX_MAXN 0
/* L2CAP_ERTX: The Extended Response Timeout eXpired timer is used in
   place of the RTC timer when a L2CAP_ConnectRspPnd event is received
   (min 60s, max 300s) */
#define L2CAP_CFG_ERTX 300


#endif /* __BT_SYS_CFG_H__ */
