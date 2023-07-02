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
///////////////////////////////////////
/*      For Debug Print Module       */
/*                                   */
///////////////////////////////////////

#define DBG_ERROR_LEVEL             1
#define DBG_WARNING_LEVEL           2
#define DBG_INFO_LEVEL              3


/*NEED TO DEFINE OUTPUT METHOD*/
#define DEBUG_PRINT TRACE


/*Moudle list need to set*/
/*set debug level for each file*/

#define a2dp_LEVEL          DBG_ERROR_LEVEL
#define avctp_LEVEL         DBG_ERROR_LEVEL
#define avdtp_LEVEL         DBG_INFO_LEVEL
#define avrcp_LEVEL         DBG_ERROR_LEVEL
#define AUDIO_CODEC_LEVEL   DBG_ERROR_LEVEL
#define btm_handle_hcievent_LEVEL   DBG_ERROR_LEVEL
#define btm_LEVEL           DBG_ERROR_LEVEL
#define HCI_LEVEL           DBG_INFO_LEVEL
#define hshf_app_LEVEL      DBG_ERROR_LEVEL
#define i2s_LEVEL           DBG_ERROR_LEVEL
#define l2cap_LEVEL         DBG_INFO_LEVEL
#define LC_LEVEL            DBG_ERROR_LEVEL
#define LMP_LEVEL           DBG_ERROR_LEVEL
#define LMP_UI_LEVEL        DBG_ERROR_LEVEL
#define rfcomm_LEVEL        DBG_INFO_LEVEL
#define SBC_LEVEL           DBG_ERROR_LEVEL
#define sco_LEVEL           DBG_ERROR_LEVEL
#define sdp_LEVEL           DBG_INFO_LEVEL
#define sdp_client_LEVEL    DBG_ERROR_LEVEL
#define sdp_server_LEVEL    DBG_INFO_LEVEL
#define spp_app_LEVEL       DBG_WARNING_LEVEL
#define sppnew_LEVEL        DBG_WARNING_LEVEL


///////////////////////////////////////
/*      For Debug cpu use Module     */
/*                                   */
///////////////////////////////////////

#define EXT_LINE_SET_HIGH
#define EXT_LINE_SET_LOW


