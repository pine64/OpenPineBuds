/****************************************************************************
 *
 * File:
 *     $Id: overide.h 2809 2011-10-11 21:42:02Z dliechty $
 *     $Product: BES AV Profiles SDK v2.x $
 *     $Revision: 2809 $
 *
 * Description: Configuration overrides for the A2DP project.
 *
 * Created:     June 15, 2004
 *
 * Copyright 2004-2005 Extended Systems, Inc.
 * Portions copyright BES.
 * All rights reserved.  All unpublished rights reserved.
 *
 * Unpublished Confidential Information of BES.
 * Do Not Disclose.
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

#ifndef __OVERIDE_H
#define __OVERIDE_H

/* WARNING: The values in this overide.h file were selected specifically for
 * this sample application. If you change them, the sample application may fail
 * to compile or not work properly.
 */

/****************************************************************************
 *
 *          Module Selection
 *  Please select the module/profiles which are needed
 *
 ****************************************************************************/

/* Initialize the necessary modules */
#if defined (BES_AUD)
#if defined (__HSP_ENABLE__)
#define XA_LOAD_LIST        XA_MODULE(CMGR) \
                            XA_MODULE(HF) \
                            XA_MODULE(HS) \
                            XA_MODULE(AVDTP) \
                            XA_MODULE(AVDEV) \
                            XA_MODULE(A2DP) \
                            XA_MODULE(AVRCP) \
                            XA_MODULE(HID) \
                            XA_MODULE(BESAUD)
#else
#define XA_LOAD_LIST        XA_MODULE(CMGR) \
                            XA_MODULE(HF) \
                            XA_MODULE(AVDTP) \
                            XA_MODULE(AVDEV) \
                            XA_MODULE(A2DP) \
                            XA_MODULE(AVRCP) \
                            XA_MODULE(HID) \
                            XA_MODULE(BESAUD)
#endif

#else

#if defined (__HSP_ENABLE__)
#define XA_LOAD_LIST        XA_MODULE(CMGR) \
                            XA_MODULE(HF) \
                            XA_MODULE(HS) \
                            XA_MODULE(AVDTP) \
                            XA_MODULE(AVDEV) \
                            XA_MODULE(A2DP) \
                            XA_MODULE(AVRCP) \
                            XA_MODULE(HID)
#else
#define XA_LOAD_LIST        XA_MODULE(CMGR) \
                            XA_MODULE(HF) \
                            XA_MODULE(AVDTP) \
                            XA_MODULE(AVDEV) \
                            XA_MODULE(A2DP) \
                            XA_MODULE(AVRCP) \
                            XA_MODULE(HID)
#endif
#endif 
                            /*      XA_MODULE(HF) \
                         XA_MODULE(OBEX) \
                            XA_MODULE(GOEP) \
                            XA_MODULE(OPUSH) \
                            XA_MODULE(FTP) \
                                XA_MODULE(CMGR) \
                                XA_MODULE(SPPDRV) \
                                XA_MODULE(OBEX) \
                                XA_MODULE(GOEP) \
                                XA_MODULE(HF) \
                                XA_MODULE(HS) \
                        */

/*****************************************************************************
 *    Global Definition for feature selection
 *    Based on BlueSDK Porting API.pdf
 *****************************************************************************/

/*   Part 1. Global definition */
#define BT_STACK                                    XA_ENABLED
#define TCP_STACK                                   XA_DISABLED
#define IRDA_STACK                                  XA_DISABLED

#define BT_BEST_SYNC_CONFIG                         XA_ENABLED

//#define XA_DEBUG                                    XA_DISABLED /* enable debug */
#ifndef XA_DEBUG
#define XA_DEBUG                                    XA_DISABLED
#endif
#define XA_DEBUG_PRINT                              XA_DEBUG   // enable debug information output, must imprement OS_Report
#define XA_ERROR_CHECK                              XA_ENABLED /* enable error check of stack */
//#define XA_CONTEXT_PTR                                XA_ENABLED /* disable dymanical RAM allocation. The context structures are performed using "->" operand */
#define XA_CONTEXT_PTR                              XA_DISABLED /* disable dymanical RAM allocation. The context structures are performed using "->" operand */

#define XA_INTEGER_SIZE                             4
#define XA_USE_ENDIAN_MACROS                        XA_DISABLED
//#define XA_MULTITASKING                               XA_DISABLED //Need implement OS_StartTimer, OS_CancelTimer, OS_NotifyEvm, OS_LockStack, OS_UnlockStack
#define XA_MULTITASKING                             XA_ENABLED //Need implement OS_StartTimer, OS_CancelTimer, OS_NotifyEvm, OS_LockStack, OS_UnlockStack
#define XA_EVENTMGR                                 XA_ENABLED //Enable Event Manager (EVM)
//#define XA_SNIFFER                                    XA_DISABLED //to sniff stack operation, need to disable in release stack
//#define XA_DECODER                                    XA_DISABLED //to decode stack information in sniff and debug
//#define XA_STATISTICS                             XA_DISABLED /* Enable statistics */
#define XA_SNIFFER                                  XA_DISABLED //to sniff stack operation, need to disable in release stack
#define XA_DECODER                                  XA_DEBUG //to decode stack information in sniff and debug
#define XA_STATISTICS                               XA_DISABLED /* Enable statistics */

/*   Part 2. Global Configuration  */
#define BDADDR_NTOA_SIZE                            18

#define NUM_BT_DEVICES                              2
#define NUM_SCO_CONNS                               2

#define NUM_BLE_DEVICES                             2
/*   Part 3. Management Entity Configuration   */
#define BT_ALLOW_SCAN_WHILE_CON                     XA_ENABLED
#define BT_SCO_HCI_DATA                             XA_DISABLED
#define BT_SCO_HCI_NUM_PACKETS                      (8 * NUM_SCO_CONNS)
#define BT_SCO_USE_LEGACY_CONNECT                   XA_DISABLED
#define BT_SECURITY                                 XA_ENABLED
#define BT_SECURITY_TIMEOUT                         80 //seconds

//#define BT_DEFAULT_PAGE_SCAN_WINDOW                   0
//#define BT_DEFAULT_PAGE_SCAN_INTERVAL             0
//#define BT_DEFAULT_INQ_SCAN_WINDOW                    0
//#define BT_DEFAULT_INQ_SCAN_INTERVAL              0

//#define BT_DEFAULT_PAGE_SCAN_WINDOW                   0x20
//#define BT_DEFAULT_PAGE_SCAN_INTERVAL             0x40
#define BT_DEFAULT_PAGE_SCAN_WINDOW                 0x12
#define BT_DEFAULT_PAGE_SCAN_INTERVAL               0x800
#define BT_DEFAULT_INQ_SCAN_WINDOW                  0x12
#define BT_DEFAULT_INQ_SCAN_INTERVAL                0x800
#define BT_DEFAULT_ACCESS_MODE_NC                   BAM_GENERAL_ACCESSIBLE
#define BT_DEFAULT_ACCESS_MODE_C                    BAM_NOT_ACCESSIBLE

#define BT_DEFAULT_ACCESS_MODE_PAIR                 BAM_GENERAL_ACCESSIBLE
#define BT_DEFAULT_ACCESS_MODE_NCON                 BAM_CONNECTABLE_ONLY
#define BT_DEFAULT_ACCESS_MODE_1C                   BAM_CONNECTABLE_ONLY
#define BT_DEFAULT_ACCESS_MODE_2C                   BAM_NOT_ACCESSIBLE

#define BT_HCI_NUM_INIT_RETRIES                     0x80
#define BT_DEFAULT_PAGE_TIMEOUT						0x2000 //0x2000=5.12s
#define BT_DEFAULT_PAGE_TIMEOUT_IN_MS				5000
#define BT_PACKET_HEADER_LEN                        25
#define NUM_KNOWN_DEVICES                           10
#define DS_NUM_SERVICES                             8


/*    Part 4. HCI      */
#define HCI_RESET_TIMEOUT                           10000 //=10s
#define HCI_NUM_PACKETS                             (2 * NUM_BT_DEVICES)
#define HCI_NUM_COMMANDS                            1
#define HCI_NUM_EVENTS                              10
#define HCI_CMD_PARM_LEN                            248
#define HCI_HOST_FLOW_CONTROL                       XA_ENABLED //IMPORTANT

#define HCI_MAX_COMPLETED_PKTS                      8
#define HCI_SCO_FLOW_CONTROL                        XA_DISABLED //IMPORTANT
#define HCI_ALLOW_PRESCAN                           XA_DISABLED
#define HCI_NUM_ACL_TX_RESERVE                      0
#define HCI_NUM_UNCONN_RESERVE                      0
#if defined(A2DP_SCALABLE_ON)
#define HCI_ACL_DATA_SIZE                           1000
#else
#if defined(__3M_PACK__)
#define HCI_ACL_DATA_SIZE                           1000
#else
#define HCI_ACL_DATA_SIZE                           800
#endif
//#define HCI_ACL_DATA_SIZE                         350
#endif
#ifndef _SCO_BTPCM_CHANNEL_        
#define HCI_SCO_DATA_SIZE                           180
#else
#define HCI_SCO_DATA_SIZE                           4
#endif
//#define HCI_NUM_ACL_BUFFERS                           (L2CAP_ERTM_RX_WIN_SIZE * L2CAP_NUM_ENHANCED_CHANNELS)
#define HCI_NUM_ACL_BUFFERS                         6
#ifndef _SCO_BTPCM_CHANNEL_        
#define HCI_NUM_SCO_BUFFERS                         6
#else
#define HCI_NUM_SCO_BUFFERS                         1
#endif

#define HCI3W_SLIDING_WINDOW                        (3)
#define HCI3W_MAX_PAYLOAD                           (0x0FFF)
#define HCI3W_MEM_POOL_SIZE                         (0x8000)
#define HCI3W_UART_MULTIPLIER                       (1)
#define HCI3W_TXQ_MAX                               (8)
#define HCI3W_CRC                                   XA_ENABLED
#define HCI3W_OOF                                   XA_ENABLED
#define HCI3W_DEBUG_TXQ                             XA_DISABLED

#define BT_EXPOSE_BCCMD                             XA_DISABLED


/* Part 5. Connection Manager (CMGR)   */
#define CMGR_DEFAULT_SNIFF_EXIT_POLICY      CMGR_SNIFF_EXIT_ON_AUDIO
#define CMGR_DEFAULT_SNIFF_TIMER            CMGR_SNIFF_DONT_CARE
//#define CMGR_AUDIO_DEFAULT_PARMS          CMGR_AUDIO_PARMS_S3
//#define CMGR_AUDIO_DEFAULT_PARMS          CMGR_AUDIO_PARMS_SCO
#define CMGR_AUDIO_DEFAULT_PARMS            CMGR_AUDIO_PARMS_S4
#define CMGR_MEMORY_EXTERNAL                XA_DISABLED
#define CMGR_SNIFF_MIN_INTERVAL             800
#define CMGR_SNIFF_MAX_INTERVAL             800
#define CMGR_SNIFF_ATTEMPT                  3
#define CMGR_SNIFF_TIMEOUT                  1


/* Part 6. L2CAP definitions */
//#define L2CAP_NUM_CHANNELS                10
//#define L2CAP_NUM_ENHANCED_CHANNELS       3
#define L2CAP_NUM_ENHANCED_CHANNELS         0

#if defined(__3M_PACK__)
#define L2CAP_MTU                           1021
#else
#define L2CAP_MTU                           679
#endif
#if L2CAP_NUM_ENHANCED_CHANNELS
#define L2CAP_MPS                           (L2CAP_MTU-7)
#else
#define L2CAP_MPS                           L2CAP_MTU
#endif

#ifdef __ACC_FRAGMENT_COMPATIBLE__
#define L2CAP_MTU_FOR_ACC                   1024
#endif

#define L2CAP_NUM_PROTOCOLS                 12
#define L2CAP_PING_SUPPORT                  XA_DISABLED
#define L2CAP_FCS_OPTION                    L2FCS_16BIT

#define L2CAP_NUM_GROUPS                    0  //8
#define L2CAP_GET_INFO_SUPPORT              XA_DISABLED
#define L2CAP_FLEXIBLE_CONFIG               XA_DISABLED
#define L2CAP_RTX_TIMEOUT                   30 //seconds
#define L2CAP_ERTX_TIMEOUT                  150 //seconds
#define L2CAP_ERTM_TX_WIN_SIZE              5
#define L2CAP_ERTM_RX_WIN_SIZE              5
#define L2CAP_ERTM_MAX_TRANSMIT             10
#define L2CAP_ERTM_RETRANS_TIMEOUT          2000 //milliseconds
#define L2CAP_ERTM_MONITOR_TIMEOUT          12000 //MILLISECONDS
#define L2CAP_ERTM_ACK_TIMEOUT              (L2CAP_ERTM_RETRANS_TIMEOUT/2)
#define L2CAP_ERTM_SREJ_ENABLE              XA_DISABLED
#define L2CAP_ERTM_FRAGMENTS                XA_DISABLED
#define L2CAP_ERTM_IMMEDIATE                XA_DISABLED
#define L2CAP_FLOW_CONTROL                  XA_DISABLED
#define L2CAP_PRIORITY                      XA_DISABLED
//#define L2CAP_PRIORITY                        XA_ENABLED
#define L2CAP_DEREGISTER_FUNC               XA_DISABLED
//#define L2CAP_NUM_SIGNAL_PACKETS          (NUM_BT_DEVICES * L2CAP_NUM_ENHANCED_CHANNELS *4)
#define L2CAP_NUM_SIGNAL_PACKETS            (NUM_BT_DEVICES * 2)
#define L2CAP_NUM_TX_PACKETS                ((L2CAP_ERTM_TX_WIN_SIZE * L2CAP_NUM_ENHANCED_CHANNELS) + 2)
#define L2CAP_PRELUDE_SIZE                  7
#define L2CAP_VIOLATE_SPEC_MTU_NEG          XA_DISABLED

//Not documented
//#define L2CAP_DEFAULT_MTU                 672
//#define L2CAP_DEFAULT_MTU                 335
#define L2CAP_MINIMUM_MTU                   0x0030
#define L2CAP_MAXIMUM_MTU                   L2CAP_MTU
#define L2CAP_MANGLER_TESTING               XA_DISABLED
#define L2CAP_ENHANCED_IOP_TESTING          XA_DISABLED

/* Part 7. SDP               */
#define SDP_CLIENT_SUPPORT                  XA_ENABLED
#define SDP_SERVER_SUPPORT                  XA_ENABLED
#define SDP_CLIENT_LOCAL_MTU                L2CAP_MTU
#define SDP_PARSING_FUNCS                   XA_ENABLED
#define SDP_NUM_CLIENTS                     NUM_BT_DEVICES
#define SDP_ACTIVE_CLIENTS                  (NUM_BT_DEVICES)
#define SDP_SERVER_SEND_SIZE                128
#define SDP_SERVER_MAX_LEVEL                4
#define SDP_SERVER_LOCAL_MTU                L2CAP_MTU
#define SDP_SERVER_MIN_REMOTE_MTU           48

/* Part 8. RFCOMM              */
#define RFCOMM_PROTOCOL                     XA_ENABLED
#define RF_SECURITY                         XA_ENABLED
#define RF_SEND_TEST                        XA_DISABLED  //XA_ENABLED
#define RF_SEND_CONTROL                     XA_DISABLED  //XA_ENABLED
#define NUM_RF_SERVERS                      30
#define NUM_RF_CHANNELS                     (NUM_RF_SERVERS * 2)
#define RF_MAX_FRAME_SIZE                   L2CAP_MTU-5
#define RF_CONNECT_TIMEOUT                  60000 //milliseconds
#define RF_T1_TIMEOUT                       20000  //milliseconds
#define RF_T2_TIMEOUT                       20000  //milliseconds

#define RF_MIN_FRAME_SIZE                   23
#define RF_DEFAULT_FRAMESIZE                127
#define RF_DEFAULT_PRIORITY                 0
#define RF_LOWEST_PRIORITY                  63

/* Part 9. SPP                */
#define SPP_SERVER                          XA_ENABLED
#define SPP_CLIENT                          XA_ENABLED

/* Part 10. Unplugfest testing */
#define UPF_TWEAKS                          XA_DISABLED

/*BES AUD data path*/
#ifdef BES_AUD
#define BESAUD_DEVICE                       XA_ENABLED
#else
#define BESAUD_DEVICE                       XA_DISABLED
#endif

/**********************************************************************************
 *
 *   AV Profile SDK
 *
 **********************************************************************************/
/* A2DP definitions */

#if defined(APP_LINEIN_A2DP_SOURCE)||defined(APP_I2S_A2DP_SOURCE)
#define A2DP_SOURCE                         XA_ENABLED
#else
#define A2DP_SOURCE                         XA_DISABLED
#endif
#define A2DP_SINK                           XA_ENABLED
#define A2DP_MINOR_DEVICE_CLASS             COD_MINOR_AUDIO_HIFIAUDIO
#define A2DP_SRC_FEATURES                   A2DP_SRC_FEATURE_PLAYER
#define A2DP_SNK_FEATURES                   (A2DP_SNK_FEATURE_SPEAKER | A2DP_SNK_FEATURE_HEADPHONE)
#define A2DP_MAX_STREAMINFOS                8

//#define A2DP_APP_USE_WMSDK                    XA_ENABLED  //for Windows only
#define A2DP_APP_USE_WMSDK                  XA_DISABLED

/* VDP                 */
#define VDP_SOURCE                          XA_DISABLED
#define VDP_SINK                            XA_DISABLED
#define VDP_MINOR_DEVICE_CLASS              COD_MINOR_AUDIO_VIDEOCAMERA
#define VDP_MAX_STREAMINFOS                 8

/* AVDTP              */
#define AVDTP_RTX_SIG_TIMEOUT               3000
#define AVDTP_MAX_CODEC_ELEM_SIZE           10
#define AVDTP_MAX_CP_VALUE_SIZE             10
#define AVDTP_NUM_TX_PACKETS                4
#define AVDTP_TX_SIGNAL_MPS                 L2CAP_MINIMUM_MTU
#define AVDTP_RX_SIGNAL_MPS                 L2CAP_MPS
#define AVDTP_TX_STREAM_MPS                 L2CAP_MPS
#define AVDTP_RX_STREAM_MPS                 L2CAP_MPS
#define BT_STATUS_ABORTED                   (BT_STATUS_LAST_CODE + 1)
#define AV_WORKER 1
/* AVRCP               */
#define AVRCP_VERSION_1_3_ONLY              XA_DISABLED //XA_DISABLED
#define AVRCP_RTX_CMD_TIMEOUT               1000
#define AVRCP_RTX_ADV_CMD_TIMEOUT           1000
#define AVRCP_RTX_BROWSE_CMD_TIMEOUT        10000
#define AVRCP_ADVANCED_TARGET               XA_DISABLED

#define AVRCP_ADVANCED_TARGET_SLIM          XA_ENABLED
#define AVRCP_ADVANCED_CONTROLLER           XA_ENABLED
#define AVRCP_PROVIDER_NAME                 'M', 'V', 'I', 'E', 'W', '\0'
#define AVRCP_PROVIDER_NAME_LEN             6
#define AVRCP_CT_SERVICE_NAME               'M', 'V', 'I', 'E', 'W', '\0'
#define AVRCP_CT_SERVICE_NAME_LEN           6
#define AVRCP_TG_SERVICE_NAME               'M', 'V', 'I', 'E', 'W', '\0'
#define AVRCP_TG_SERVICE_NAME_LEN           6
#define AVRCP_PANEL_COMPANY_ID              "\xFF\xFF\xFF"
#define AVRCP_SUBUNIT_OP_QUEUE_MAX          15
#define AVRCP_BLUETOOTH_COMPANY_ID          "\x00\x19\x58"
#define AVRCP_MAX_CHAR_SETS                 10
#define AVRCP_ADVANCED_RESPONSE_SIZE        128
#define AVRCP_NO_TRACK_CURRENTLY_SELECTED   0xFFFFFFFF
#define AVRCP_MAX_MEDIA_PLAYERS             10
#define AVRCP_BROWSING_TARGET               XA_DISABLED
#define AVRCP_BROWSING_CONTROLLER           XA_DISABLED
#define AVRCP_LIST_PLAYERS_ENABLED          XA_DISABLED /*((AVRCP_ADVANCED_TARGET == XA_ENABLED) && (AVRCP_VERSION_1_3_ONLY == XA_DISABLED))*/
#define AVRCP_BROWSE_AUTO_ACCEPT            XA_DISABLED
#define AVRCP_MAX_FOLDER_DEPTH              10
#define AVRCP_NUM_PLAYER_SETTINGS           4
#define AVRCP_MAX_PALYER_STRINGS            5

/* AVCTP             */


/**********************************************************************************
 *
 *   BIP
 *
 **********************************************************************************/
/* Value */
#define BIP_SUPPORTED_CAPABILITIES          0x01
#define BIP_SUPPORTED_FEATURES              0x0091
#define BIP_SUPPORTED_FUNCTIONS             0x41EB
#define BIP_IMAGING_DATA_CAPACITY           0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00

/* Constants */
#define BIP_NUM_INITIATORS                  1
#define BIP_NUM_RESPONDERS                  1
#define BIP_MAX_PASSWORD_LEN                18
#define BIP_MAX_USERID_LEN                  20
#define BIP_MAX_REALM_LEN                   20

/**********************************************************************************
 *
 *   BNEP
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define NUM_BNEP_PANUS                      1
#define BNEP_ETHERNET_EMULATION             XA_DISABLED
#define BNEP_NUM_TIMERS                     3
#define BNEP_CONTROL_TIMEOUT                10

/**********************************************************************************
 *
 *   BPP
 *
 **********************************************************************************/
/* Value */
#define BPP_1284ID \
    'M','F','G',':','E','X','T','E','N','D','E','D','-','S', \
    'Y','S','T','E','M','S',';','M','D','L',':','X','A','B', \
    'T','P','r','i','n','t','e','r',';','D','E','S',':','X', \
    'T','N','D','A','c','c','e','s','s',' ','B','l','u','e', \
    't','o','o','t','h',' ','P','r','i','n','t','e','r',';', \
    'S','N',':','1','2','3','4','5','\0'
#define BPP_1284ID_LEN                      79
#define BPP_DOC_FORMATS \
    'a','p','p','l','i','c','a','t','i','o','n','/', \
    'v','n','d','.','p','w','g','-','x','h','t','m','l','-', \
    'p','r','i','n','t','+','x','m','l',':','1','.','0',',', \
    't','e','x','t','/','p','l','a','i','n','\0'
#define BPP_DOC_FORMATS_LEN                 51
#define BPP_CHAR_REPERTOIRES \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff
#define BPP_IMG_FORMATS \
    'i','m','a','g','e','/','j','p','e','g',',', \
    'i','m','a','g','e','/','g','i','f',':','8','9','A','\0'
#define BPP_IMG_FORMATS_LEN                 25
#define BPP_COLOR_SUPPORTED                 TRUE
#define BPP_DUPLEX_SUPPORTED                TRUE
#define BPP_MEDIA_TYPES \
    's','t','a','t','i','o','e','r','y',',', \
    'c','a','r','d','s','t','o','c','k',',', \
    'e','n','v','e','l','o','p','e','\0'
#define BPP_MEDIA_TYPES_LEN                 29
#define BPP_MAX_MEDIA_WIDTH                 210
#define BPP_MAX_MEDIA_LENGTH                297


/* Constants */
#define BPP_NUM_SENDERS                     1
#define BPP_NUM_PRINTERS                    1
#define BPP_PRINT_STATUS                    XA_ENABLED
#define BPP_MAX_PASSWORD_LEN                20
#define BPP_MAX_USERID_LEN                  20
#define BPP_MAX_REALM_LEN                   20

/**********************************************************************************
 *
 *   HFP
 *
 **********************************************************************************/
/* Value */

/* Constants */
//wangjianjun
#if defined(HFP_1_6_ENABLE)
#define HF_VERSION_1_6                  XA_ENABLED
#else
#define HF_VERSION_1_6                  XA_DISABLED
#endif
//#define HF_VERSION_1_6                    XA_ENABLED
#define HF_VREC                             XA_ENABLED
#define HF_FEATURE_ECHO_NOISE               0x00000001
#define HF_FEATURE_CALL_WAITING             0x00000002
#define HF_FEATURE_CLI_PRESENTATION         0x00000004
#define HF_FEATURE_VOICE_RECOGNITION        0x00000008
#define HF_FEATURE_VOLUME_CONTROL           0x00000010

#if  HF_VERSION_1_6 == XA_ENABLED
#define HF_FEATURE_CODEC_NEGOTIATION        0x00000020 //wind band speech
#define HF_FEATURE_ENHANCED_CALL_STATUS         0x00000040
#define HF_FEATURE_ENHANCED_CALL_CTRL       0x00000080
#else
#define HF_FEATURE_ENHANCED_CALL_STATUS         0x00000020
#define HF_FEATURE_ENHANCED_CALL_CTRL       0x00000040
#endif

//zjq add for hfp1.7 hf_indicators
#define HF_FEATURE_HF_INDICATORS                 0x00000100
#define HF_FEATURE_ESCO_S4                 0x00000200

//only 5 bits avirable, no useful!!!
#if HF_VERSION_1_6 == XA_ENABLED
#define HF_SDK_FEATURES (HF_FEATURE_ECHO_NOISE  \
                            | HF_FEATURE_CALL_WAITING   \
                            | HF_FEATURE_CLI_PRESENTATION   \
                            | HF_FEATURE_VOICE_RECOGNITION  \
                            | HF_FEATURE_VOLUME_CONTROL \
                            | HF_FEATURE_CODEC_NEGOTIATION  \
                            | HF_FEATURE_ENHANCED_CALL_STATUS   \
                            | HF_FEATURE_ENHANCED_CALL_CTRL \
                            | HF_FEATURE_ESCO_S4)
#else

#define HF_SDK_FEATURES (HF_FEATURE_ECHO_NOISE          \
                            | HF_FEATURE_CALL_WAITING   \
                            | HF_FEATURE_CLI_PRESENTATION   \
                            | HF_FEATURE_VOICE_RECOGNITION  \
                            | HF_FEATURE_VOLUME_CONTROL \
                            | HF_FEATURE_ENHANCED_CALL_STATUS   \
                            | HF_FEATURE_ENHANCED_CALL_CTRL)
#endif

#define HF_COMMAND_TIMEOUT                  10000
#define HF_USE_CALL_MANAGER                 XA_DISABLED  //XA_ENABLED
#define HF_POLL_TIMEOUT                     120000
#define HF_POLL_TIMEOUT_ACTIVE              4000
#define HF_RING_TIMEOUT                     10000
#define HF_TX_BUFFER_SIZE                   128   //1024
#define HF_RECV_BUFFER_SIZE                 HCI_ACL_DATA_SIZE  //1024
#define HF_USE_PHONEBOOK_COMMANDS           XA_DISABLED
#define HF_USE_MESSAGING_COMMANDS           XA_DISABLED
#define HF_USE_IIA                          XA_DISABLED
//#define HF_USE_RESP_HOLD                  XA_ENABLED
#define HF_USE_RESP_HOLD                    XA_DISABLED
#define HF_SNIFF_MIN_INTERVAL               0x0040
#define HF_SNIFF_MAX_INTERVAL               0x0800
#define HF_SNIFF_ATTEMPT                    0x0160
#define HF_SNIFF_TIMEOUT                    0x0160
#define HF_MEMORY_EXTERNAL                  XA_DISABLED
//#define HF_SECURITY_SETTINGS              (BSL_AUTHORIZATION_IN | BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT )
#define HF_SECURITY_SETTINGS                (BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT )
#define HF_CBSZ                             (256)
#define HF_MAX_BIA_STRING                   (40)
#define HF_DELAY_CHUP_OK_POLL               XA_DISABLED
#define HF_DELAY_IND_SETUP_POLL             XA_DISABLED


/**********************************************************************************
 *
 *   HFP_AG
 *
 **********************************************************************************/
/* Value */

/* Constants */
//wangjianjun for hfp AG
#define HFG_FEATURE_THREE_WAY_CALLS         0x00000001
#define HFG_FEATURE_ECHO_NOISE              0x00000002
#define HFG_FEATURE_VOICE_RECOGNITION       0x00000004
#define HFG_FEATURE_RING_TONE               0x00000008
#define HFG_FEATURE_VOICE_TAG               0x00000010
#define HFG_FEATURE_REJECT                  0x00000020
#define HFG_FEATURE_ENHANCED_CALL_STATUS    0x00000040
#define HFG_FEATURE_ENHANCED_CALL_CTRL      0x00000080
#define HFG_FEATURE_EXTENDED_ERRORS         0x00000100
#if HF_VERSION_1_6 == XA_ENABLED
#define HFG_FEATURE_CODEC_NEGOTATION            0x00000200
#endif

#define HFG_TX_BUFFER_SIZE                  1024
#define HFG_RECV_BUFFER_SIZE                1024
#define HFG_USE_RESP_HOLD                   XA_DISABLED  //XA_ENABLED
#define HFG_USE_IIA                         XA_ENABLED
#define HFG_SNIFF_TIMER                     2000
#define HFG_SNIFF_MIN_INTERVAL              800
#define HFG_SNIFF_MAX_INTERVAL              8000
#define HFG_SNIFF_ATTEMPT                   1600
#define HFG_SNIFF_TIMEOUT                   1600
#define HFG_MEMORY_EXTERNAL                 XA_DISABLED
#define HFG_SECURITY_SETTINGS               (BSL_AUTHORIZATION_IN | BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT )
#define HFG_II_DESC_MAX                     128


/**********************************************************************************
 *
 *   HCRP
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define HCRP_1284ID \
    'M','F','G',':','i','A','n','y','w','h','e','r','e',';','M','D', \
    'L',':','i','A','B','T','P','r','i','n','t','e','r',';','D','E', \
    'S',':','i','A','n','y','w','h','e','r','e',' ','B','l','u','e', \
    't','o','o','t','h',' ','P','r','i','n','t','e','r',';','S','N', \
    ':','1','2','3','4','5',';'
#define HCRP_1284ID_LEN                     71
#define HCRP_DEVICE_NAME 'i','A','B','t','P','r','i','n','t','e','r'
#define HCRP_DEVICE_NAME_LEN                11
#define HCRP_FRIENDLY_NAME \
    'i','A','n','y','w','h','e','r','e',' ','B','l','u','e','t','o', \
    'o','t','h',' ','P','r','i','n','t','e','r'
#define HCRP_FRIENDLY_NAME_LEN              27
#define HCRP_SERVER                         XA_DISABLED
#define HCRP_CLIENT                         XA_DISABLED
#define HCRP_CLIENT_CTL_MTU                 (L2CAP_MTU - 8)

/**********************************************************************************
 *
 *   HSP
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define HS_MAX_LEVEL                        15
#define HS_DEFAULT_LEVEL                    7
#define HS_SECURITY                         XA_ENABLED
#define HS_SECURITY_SETTINGS                (BSL_AUTHORIZATION_IN | BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT)
#define HS_MEMORY_EXTERNAL                  XA_DISABLED
#define HS_REMOTE_AUDIO_VOLUME_CONTROL      TRUE
 
#define HS_SNIFF_ATTEMPT	0x0160
#define HS_SNIFF_TIMEOUT	0x0160
#define HS_SNIFF_MAX_INTERVAL	0x0800
#define HS_SNIFF_MIN_INTERVAL	0x0040 
	
#define HS_RECV_BUFFER_SIZE		HCI_ACL_DATA_SIZE
#define HS_TX_BUFFER_SIZE		128


/**********************************************************************************
 *
 *   HSP_AG
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define AG_SECURITY                         XA_ENABLED
#define AG_SECURITY_SETTINGS                (BSL_AUTHORIZATION_IN | BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT)
#define AG_SCO_SETTINGS                     XA_ENABLED
#define AG_RINGTIMER                        5000
#define AG_MAX_RING_COUNT                   5
#define AG_MAX_LEVEL                        15
#define AG_DEFAULT_LEVEL                    7
#define AG_DISCONNECT_REQUEST_NOTIFY        XA_ENABLED
#define AG_MEMORY_EXTERNAL                  XA_DISABLED


/**********************************************************************************
 *
 *   HID
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define HID_HOST                            XA_DISABLED
#ifndef HID_DEVICE
#define HID_DEVICE                          XA_DISABLED
#endif
#define HID_NUM_TX_PACKETS                  5
#define HID_NUM_SDP_ATTRIBUTES              24
#define DEVICE_ID_NUM_SDP_ATTRIBUTES        7
#define HID_DEVICE_RELEASE                  0x0100
#define HID_PARSER_VERSION                  0x0111
#define HID_DEVICE_SUBCLASS					((U8)(COD_MINOR_PERIPH_KEYBOARD))
#define HID_COUNTRY_CODE                    0x21
#define HID_VIRTUAL_CABLE                   TRUE
#define HID_RECONNECT_INITIATE              TRUE
#define HID_DESCRIPTOR_TYPE                 0x22
#define HID_DESCRIPTIOR_LEN                 50
#define HID_DESCRIPTOR \
        0x05, 0x01,  /* USAGE_PAGE(Generic Desktop)   */ \
        0x09, 0x02,  /* USAGE(Mouse)                  */ \
        0xA1, 0x01,  /* COLLECTION(Application)       */ \
        0x09, 0x01,  /*  USAGE(Pointer)               */ \
        0xA1, 0x00,  /*  COLLECTION(Physical)         */ \
        0x05, 0x01,  /*   USAGE_PAGE(Generic Desktop) */ \
        0x09, 0x30,  /*   USAGE(X)                    */ \
        0x09, 0x31,  /*   USAGE(Y)                    */ \
        0x15, 0x81,  /*   LOGICAL_MINIMUM(-127)       */ \
        0x25, 0x7F,  /*   LOGICAL_MAXIMUM(127)        */ \
        0x75, 0x08,  /*   REPORT_SIZE(8)              */ \
        0x95, 0x02,  /*   REPORT_COUNT(2)             */ \
        0x81, 0x06,  /*   INPUT(Data,Var,Rel)         */ \
        0xC0,        /*  END_COLLECTION               */ \
        0x05, 0x09,  /*  USAGE_PAGE(Button)           */ \
        0x19, 0x01,  /*  USAGE_MINIMUM(Button 1)      */ \
        0x29, 0x03,  /*  USAGE_MAXIMUM(Button 3)      */ \
        0x15, 0x00,  /*  LOGICAL_MINUMUM(0)           */ \
        0x25, 0x03,  /*  LOGICAL_MAXIMUM(1)           */ \
        0x95, 0x03,  /*  REPORT_COUNT(3)              */ \
        0x75, 0x01,  /*  REPORT_SIZE(1)               */ \
        0x81, 0x02,  /*  INPUT(Data,Var,Abs)          */ \
        0x95, 0x01,  /*  REPORT_COUNT(1)              */ \
        0x75, 0x05,  /*  REPORT_SIZE(5)               */ \
        0x81, 0x03,  /*  INPUT(Cnst,Var,Abs)          */ \
        0xC0         /* END_COLLECTION                */
#define HID_MAX_DESCRIPTOR_LEN              128
#define HID_BATTERY_POWER                   FALSE
#define HID_REMOTE_WAKE                     TRUE
#define HID_SUPERVISION_TIMEOUT             0x7D00
#define HID_NORMALLY_CONNECTABLE            TRUE
#define HID_BOOT_DEVICE                     TRUE
#define HID_DEVID_SPEC_ID                   0x0103
#define HID_DEVID_VENDOR_ID                 0x23A1
#define HID_DEVID_VENDOR_ID_SRC             0x0001
#define HID_DEVID_PRODUCT_ID                0x1234
/**********************************************************************************
 *
 *   DIP
 *
 **********************************************************************************/
/* Value */
#ifndef DIP_DEVICE
#define DIP_DEVICE                          XA_ENABLED
#endif


/**********************************************************************************
 *
 *   MAP
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define MAP_NUM_CLIENTS                     1
#define MAP_NUM_SERVERS                     1
#define MAP_NOTIFICATION                    XA_ENABLED
#define MAP_BROWSING                        XA_ENABLED
#define MAP_DELETE                          XA_ENABLED
#define MAP_UPLOADING                       XA_ENABLED
#define MAP_MAX_APP_PARMS_LEN               255
#define MAP_EMAIL_SUPPORTED                 XA_ENABLED
#define MAP_SMS_GSM_SUPPORTED               XA_ENABLED
#define MAP_SMS_CDMA_SUPPORTED              XA_DISABLED
#define MAP_MMS_SUPPORTED                   XA_ENABLED
#define MAP_NOTIFICATION_SUPPORTED          XA_ENABLED
#define PUSH_MESSAGE_TYPE                   "x-bt/message"
#define PULL_MESSAGE_TYPE                   "x-bt/message"
#define SET_MESSAGE_STATUS_TYPE             "x-bt/messageStatus"
#define GET_MESSAGE_LISTING_TYPE            "x-bt/MAP-msg-listing"
#define GET_FOLDER_LISTING_TYPE             "x-obex/folder-listing"
#define SET_NOTIFICATION_REGISTER_TYPE      "x-bt/MAP-NotificationRegistration"
#define SEND_EVENT_TYPE                     "x-bt/MAP-event-report"
#define UPDATE_INBOX_TYPE                   "x-bt/MAP-messageUpdate"
#define MAP_UNKNOWN_OBJECT_LENGTH           0xFFFFFFFF


/**********************************************************************************
 *
 *   PAN
 *
 **********************************************************************************/
/* Value */
#define PAN_PANU_NUM_ATTRIBUTES     9
#define PAN_GN_NUM_ATTRIBUTES       11
#define PAN_NAP_NUM_ATTRIBUTES      13
#define PAN_NUM_SEARCH_ATTRIBUTES   21
#define PAN_PKT_TYPE_LIST \
    SDP_UINT_16BIT(0x0800),         /* Uint16 IPv4 */ \
    SDP_UINT_16BIT(0x0806)          /* Uint16 ARP */
#define PAN_SECURITY_LEVEL      0x0000  /* No security */
#define PAN_PANU_SERVICE_NAME \
    'P','e','r','s','o','n','a','l',' ','A','d','-','h','o','c',' ', \
    'U','s','e','r',' ','S','e','r','v','i','c','e', 0x00
#define PAN_PANU_SERVICE_DESCRIPTION \
    'P','e','r','s','o','n','a','l',' ','A','d','-','h','o','c',' ', \
    'U','s','e','r',' ','S','e','r','v','i','c','e', 0x00
#define PAN_GN_SERVICE_NAME \
    'G','r','o','u','p','A','d','-','h','o','c',' ', \
    'N','e','t','w','o','r','k',' ','S','e','r','v','i','c','e', 0x00
#define PAN_GN_SERVICE_DESCRIPTION \
    'P','e','r','s','o','n','a','l',' ','G','r','o','u','p',' ', \
    'A','d','-','h','o','c',' ','N','e','t','w','o','r','k',' ', \
    'S','e','r','v','i','c','e', 0x00
#define PAN_GN_IPV4_SUBNET '1','0','.','0','.','0','.','0','/','8'
#define PAN_GN_IPV6_SUBNET 'f','e','8','0',':',':','/','4','8'
#define PAN_NAP_SERVICE_NAME \
    'N','e','t','w','o','r','k',' ','A','c','c','e','s','s',' ', \
    'P','o','i','n','t',' ','S','e','r','v','i','c','e', 0x00
#define PAN_NAP_SERVICE_DESCRIPTION \
    'P','e','r','s','o','n','a','l',' ','A','d','-','h','o','c',' ', \
    'N','e','t','w','o','r','k',' ','S','e','r','v','i','c','e', 0x00
/*---------------------------------------------------------------------------
 *  PAN_NAP_NET_ACCESS_TYPE constant
 *      0x0000: PSTN,
 *      0x0001: ISDN,
 *      0x0002: DSL,
 *      0x0003: Cable Modem,
 *      0x0004: 10Mb Ethernet,
 *      0x0005: 100Mb Ethernet,
 *      0x0006: 4Mb Token Ring,
 *      0x0007: 16Mb Token Ring,
 *      0x0008: 100Mb Token Ring,
 *      0x0009: FDDI,
 *      0x000A: GSM,
 *      0x000B: CDMA,
 *      0x000C: GPRS,
 *      0x000D: 3G,
 *      0xFFFE: other.
 */
#define PAN_NAP_NET_ACCESS_TYPE     0x0005      /* 100Mb Ethernet */
#define PAN_NAP_MAX_NET_ACCESS_RATE 10000000    /* 10Mb/sec */
#define PAN_NAP_IPV4_SUBNET '1','0','.','0','.','0','.','0','/','8'
#define PAN_NAP_IPV6_SUBNET 'f','e','8','0',':',':','/','4','8'

/* Constants */
#define NUM_BT_PKTS                         (NUM_BNEP_PANUS * 4)
#define NUM_PAN_ARP_ADDRS                   10
#define PAN_LINK_LOCAL_SUBNET               0xA9FE0000 /*169.254.0.0*/
#define PAN_ROLE_PANU                       XA_ENABLED
#define PAN_ROLE_GN                         XA_DISABLED
#define PAN_ROLE_NAP                        XA_DISABLED


/**********************************************************************************
 *
 *   PBAP
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define PBAP_NUM_CLIENTS                    1
#define PBAP_NUM_SERVERS                    1
#define PBAP_MAX_PASSWORD_LEN               20
#define PBAP_MAX_USERID_LEN                 20
#define PBAP_MAX_REALM_LEN                  20
#define PBAP_MAX_APP_PARMS_LEN              30
#define PBAP_LOCAL_PHONEBOOK_SUPPORTED      XA_ENABLED
#define PBAP_SIM_PHONEBOOK_SUPPORTED        XA_ENABLED
#define PULL_PHONEBOOK_TYPE                 "x-bt/phonebook"
#define VCARD_LISTING_OBJECT_TYPE           "x-bt/vcard-listing"
#define VCARD_OBJECT_TYPE                   "x-bt/vcard"
#define PBAP_UNKNOWN_OBJECT_LENGTH          0xFFFFFFFF
#define PBAP_FILTER_SIZE                    8


/**********************************************************************************
 *
 *   SIM
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define SIM_SERVER                          XA_DISABLED
#define SIM_CLIENT                          XA_DISABLED
#define SIM_MAX_MSG_SIZE                    RF_MAX_FRAME_SIZE
#define SIM_MAX_APDU                        276
#define SIM_CLIENT_SECURITY_SETTINGS        (BSL_AUTHORIZATION_IN | BSL_SECURITY_L2_IN | BSL_SECURITY_L2_OUT)
#define SIM_MAX_MSG_PARMS                   3


/**********************************************************************************
 *
 *   TCS Binary
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define TCS_CORDLESS                        XA_DISABLED
#define TCS_WUG_MASTER                      XA_DISABLED
#define NUM_TCS_CONNS                       1
#define TCS_TIMEOUT_CONNECT_VAL             (3 * 60 * 1000)
#define TCS_TIMEOUT_OUTPROCEED_VAL          (30 * 1000)
#define TCS_COI_MAX                         6
#define TCS_MDATA_MAX                       (74 + TCS_COI_MAX)


/**********************************************************************************
 *
 *   TCS Message
 *
 **********************************************************************************/
/* Value */

/* Constants */



/**********************************************************************************
 *
 *   Intercom
 *
 **********************************************************************************/
/* Value */

/* Constants */
#define ICM_SECURITY                        XA_DISABLED
#define ICM_SCO_SETTINGS                    XA_DISABLED


/**********************************************************************************
 *
 *   Cordless Profile
 *
 **********************************************************************************/
/* Value */

/* Constants */





/**********************************************************************************
 *
 *   OBEX
 *
 **********************************************************************************/
/* Value */

/* Constants */
//not documented
#define OBEX_RFCOMM_TRANSPORT               XA_ENABLED
#define OBEX_ROLE_CLIENT                    XA_ENABLED
#define OBEX_DEINIT_FUNCS                   XA_ENABLED
#define OBEX_ROLE_SERVER                    XA_ENABLED
#define OBEX_TRANSPORT_FLOW_CTRL            XA_ENABLED
#define OBEX_ALLOW_SERVER_TP_CONNECT        XA_ENABLED
#define OBEX_PROVIDE_SDP_RESULTS            XA_ENABLED
#define OBEX_SERVER_CONS_SIZE               2
#define OBEX_DYNAMIC_OBJECT_SUPPORT         XA_ENABLED
#define OBEX_PACKET_FLOW_CONTROL            XA_ENABLED
#define OBEX_BODYLESS_GET                   XA_ENABLED
#define FTP_EXPANDED_API              XA_ENABLED
#define OPUSH_EXPANDED_API            XA_ENABLED


/**********************************************************************************
 *
 *   Other defition without documentation
 *
 **********************************************************************************/

#define PME_APP_NAME "A2DP Sample Application"

/* ============================================================
 *  GOEP
 */
#define GOEP_SERVER_HB_SIZE         12
#define GOEP_MAX_TYPE_LEN           100



/* ===============================
 *  Temp add by Jimmy
 */
#define OEM_STACK           XA_DISABLED
#define AT_HANDSFREE        XA_ENABLED
#define AT_DUN              XA_DISABLED
#define AT_PHONEBOOK        XA_DISABLED
#define AT_SMS              XA_DISABLED
#define AT_ROLE_TERMINAL    XA_ENABLED
#define AT_ROLE_MOBILE      XA_DISABLED

#define HF_CUSTOM_FEATURE_NULL              (0x00)
//#define HF_CUSTOM_FEATURE_RESERVED          (0x01 << 0)
#define HF_CUSTOM_FEATURE_BATTERY_REPORT    (0x03 << 0)
#define HF_CUSTOM_FEATURE_DOCK              (0x01 << 2)
#define HF_CUSTOM_FEATURE_SIRI_REPORT       (0x01 << 3)
#define HF_CUSTOM_FEATURE_NR_REPORT         (0x01 << 4)

//#define HF_CUSTOM_FEATURE_SUPPORT           (HF_CUSTOM_FEATURE_BATTERY_REPORT | HF_CUSTOM_FEATURE_SIRI_REPORT)
#ifndef HF_CUSTOM_FEATURE_SUPPORT
#ifdef SUPPORT_SIRI
#define HF_CUSTOM_FEATURE_SUPPORT           (HF_CUSTOM_FEATURE_BATTERY_REPORT | HF_CUSTOM_FEATURE_SIRI_REPORT)
#else
#define HF_CUSTOM_FEATURE_SUPPORT           (HF_CUSTOM_FEATURE_BATTERY_REPORT)
#endif /*SUPPORT_SIRI*/
#endif /*HFt _CUSTOM_FEATURE_SUPPORT*/

#if HF_CUSTOM_FEATURE_SUPPORT
#define HF_CUSTOM_VENDOR_ID     0x0000
#define HF_CUSTOM_PRODUCT_ID    0x0000
#define HF_CUSTOM_VERSION_ID    0x0100
#endif

#define __BT_RECONNECT__
#define __BT_HFP_RECONNECT__
#define __BT_SNIFF__
//#define __BT_ONE_BRING_TWO__
//#define __BT_REAL_ONE_BRING_TWO__
#define __EARPHONE__
#define __AUTOPOWEROFF__
#endif /* __OVERIDE_H */

