/**
 * @file ota_common.cpp
 * @author BES AI team
 * @version 0.1
 * @date 2020-04-17
 *
 * @copyright Copyright (c) 2015-2020 BES Technic.
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
 */

/*****************************header include********************************/
#include "ota_common.h"
#include "app.h"
#include "app_bt.h"
#include "app_utils.h"
#include "apps.h"
#include "bt_drv_reg_op.h"
#include "cmsis.h"
#include "crc32.h"
#include "hal_timer.h"
#include "norflash_api.h"
#include "norflash_drv.h"
#include "nvrecord_ota.h"
#include "ota_dbg.h"
#include "string.h"

#ifdef IBRT
#include "app_ibrt_if.h"
#include "app_ibrt_ota_cmd.h"
#include "app_ibrt_ui.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt.h"
#include "app_tws_if.h"
#endif

/*********************external function declearation************************/

/************************private macro defination***************************/
#define OTA_BOOT_INFO_FLASH_OFFSET 0x1000
#define OTA_BREAKPOINT_STORE_GRANULARITY (256 * 1024) // must be 4KB aligned
#define LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC 512
#define INVALID_VERSION_STR "BESTECHNIC"

#define NORFLASH_API_MODE_ASYNC true
#define OTA_DEFAULT_NORFLASH_API_MODE NORFLASH_API_MODE_ASYNC

#define CASES(prefix, item)                                                    \
  case prefix##item:                                                           \
    str = #item;                                                               \
    break;

/************************private type defination****************************/

/**********************private function declearation************************/

/************************private variable defination************************/
__attribute__((unused)) static const char *imageSanityKeyWord =
    "CRC32_OF_IMAGE=0x";
__attribute__((unused)) static const char *oldImageSanityKeyWord =
    "CRC32_OF_IMAGE=";

#ifdef IBRT
static void _ota_tws_thread(const void *arg);
osThreadDef(_ota_tws_thread, osPriorityNormal, 1, 1024, "ota_tws");

static osEvent evt;

/// tws relay OTA command/data TX thread ID
osThreadId txThreadId = NULL;

/// tws relay OTA command/data RX thread ID
osThreadId rxThreadId;

osMutexDef(twsTxQueueMutex);
osMutexDef(twsRxQueueMutex);

/// tws relay data TX queue mutex
osMutexId txQueueMutexID = NULL;

/// tws relay data RX queue mutex
osMutexId rxQueueMutexID = NULL;

uint8_t relayBuf[TWS_RELAY_DATA_MAX_SIZE];
#endif

OTA_COMMON_ENV_T otaEnv;

/****************************function defination****************************/

/**
 * @brief Convert OTA command to string
 *
 * @param cmd           @see OTA_COMMAND_E
 * @return char*        string of OTA_COMMAND
 */
static char *_cmd2str(OTA_COMMAND_E cmd) {
  const char *str = NULL;

  switch (cmd) {
    CASES(OTA_COMMAND_, BEGIN);
    CASES(OTA_COMMAND_, APPLY);
    CASES(OTA_COMMAND_, DATA);
    CASES(OTA_COMMAND_, ABORT);
#ifdef IBRT
    CASES(OTA_COMMAND_, RSP);
#endif
    // CASES(OTA_COMMAND_, );

  default:
    str = "INVALID";
    break;
  }

  return (char *)str;
}

/**
 * @brief Convert OTA user to string
 *
 * @param user          @see OTA_USER_E
 * @return char*        OTA_USER string
 */
static char *_user2str(OTA_USER_E user) {
  const char *str = NULL;

  switch (user) {
    CASES(OTA_USER_, BES);
    CASES(OTA_USER_, COLORFUL);
    CASES(OTA_USER_, RED);
    CASES(OTA_USER_, ORANGE);
    CASES(OTA_USER_, GREEN);
    // CASES(OTA_USER_, );

  default:
    str = "INVALID";
    break;
  }

  return (char *)str;
}

/**
 * @brief Convert OTA path into string
 *
 * @param path          @see OTA_PATH_E
 * @return char*        OTA_PATH string
 */
static char *_path2str(OTA_PATH_E path) {
  const char *str = NULL;

  switch (path) {
    CASES(OTA_PATH_, BT);
    CASES(OTA_PATH_, BLE);
    // CASES(OTA_PATH_, );

  default:
    str = "INVALID";
    break;
  }

  return (char *)str;
}

/**
 * @brief Convert OTA stage into string
 *
 * @param stage         @see OTA_STAGE_E
 * @return char*        OTA_STAGE string
 */
static char *_stage2str(OTA_STAGE_E stage) {
  const char *str = NULL;

  switch (stage) {
    CASES(OTA_STAGE_, IDLE);
    CASES(OTA_STAGE_, ONGOING);
    CASES(OTA_STAGE_, DONE);
    // CASES(OTA_STAGE_, );

  default:
    str = "INVALID";
    break;
  }

  return (char *)str;
}

static char *_sts2str(OTA_STATUS_E sts) {
  const char *str = NULL;

  switch (sts) {
    CASES(OTA_STATUS_, OK);
    CASES(OTA_STATUS_, ERROR);
    CASES(OTA_STATUS_, ERROR_RELAY_TIMEOUT);
    CASES(OTA_STATUS_, ERROR_CHECKSUM);
    CASES(OTA_STATUS_, ERROR_NOT_ALLOWED);
    // CASES(OTA_STATUS_, );

  default:
    str = "INVALID";
    break;
  }

  return (char *)str;
}

/**
 * @brief Find key word in upgrade data.
 *
 * This function is used to find out the key word at the end of upgrade data,
 * these key words are generated by python script by calculate the crc of whole
 * upgrade image and write the result at the end of the image in string format.
 * This is used to do sanity check.
 *
 * @param tgtArray      source array to search the key word
 * @param tgtArrayLen   source array length
 * @param keyArray      key word array
 * @param keyArrayLen   key word length
 * @return int32_t      index of the key word in dstArry
 */
__attribute__((unused)) static int32_t _find_key_word(uint8_t *tgtArray,
                                                      uint32_t tgtArrayLen,
                                                      uint8_t *keyArray,
                                                      uint32_t keyArrayLen) {
  if ((keyArrayLen > 0) && (tgtArrayLen >= keyArrayLen)) {
    uint32_t index = 0, targetIndex = 0;
    for (targetIndex = 0; targetIndex < tgtArrayLen; targetIndex++) {
      for (index = 0; index < keyArrayLen; index++) {
        if (tgtArray[targetIndex + index] != keyArray[index]) {
          break;
        }
      }

      if (index == keyArrayLen) {
        return targetIndex;
      }
    }

    return -1;
  } else {
    return -1;
  }
}

/**
 * @brief Convert ASCII code into hex format.
 *
 * @param asciiCode     ASCII code to convert
 * @return uint8_t      convert result
 */
__attribute__((unused)) static uint8_t _ascii2hex(uint8_t asciiCode) {
  if ((asciiCode >= '0') && (asciiCode <= '9')) {
    return asciiCode - '0';
  } else if ((asciiCode >= 'a') && (asciiCode <= 'f')) {
    return asciiCode - 'a' + 10;
  } else if ((asciiCode >= 'A') && (asciiCode <= 'F')) {
    return asciiCode - 'A' + 10;
  } else {
    return 0xff;
  }
}

/**
 * @brief Update the stage of OTA progress.
 *
 * @param stage         OTA stage to update,
 *                      @see OTA_STAGE_E to get more details.
 */
static void _set_ota_stage(OTA_STAGE_E stage) {
  LOG_D("stage update:%s->%s", _stage2str(otaEnv.currentStage),
        _stage2str(stage));

  otaEnv.currentStage = stage;
}

static void _update_ota_user(OTA_USER_E user) {
  ASSERT((OTA_USER_NUM == otaEnv.currentUser) || (OTA_USER_NUM == user),
         "try to set a ota user while the current user is not null");

  LOG_I("ota user update:%s->%s", _user2str(otaEnv.currentUser),
        _user2str(user));

  otaEnv.currentUser = user;
}

/**
 * @brief Enter OTA state handler.
 *
 * This function is used to require the relative system resources to gurantee
 * the performance for both OTA progress and other functionalities
 *
 * @param path          Current OTA path,
 *                      @see OTA_PATH_E to get more details.
 */
static void _enter_ota_state(OTA_PATH_E path) {
  LOG_I("%s", __func__);

  if (otaEnv.isInOtaState) {
    LOG_W("ALREADY in OTA state");
  } else {
    /// 1. guarantee performance->switch to the highest freq
    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_104M);

#ifdef IBRT
    /// 2. guarantee performance->decrease the communication interval of TWS
    /// connection
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    btdrv_reg_op_set_tws_link_duration(IBRT_TWS_LINK_LARGE_DURATION);
    btdrv_reg_op_set_private_tws_poll_interval(
        p_ibrt_ctrl->config.default_private_poll_interval,
        p_ibrt_ctrl->config.default_private_poll_interval_in_sco);

    /// 3. guarantee performance->exit bt sniff mode
    app_ibrt_ui_judge_link_policy(OTA_START_TRIGGER, BTIF_BLP_DISABLE_ALL);

    if (app_tws_ibrt_tws_link_connected() &&
        (p_ibrt_ctrl->nv_role == IBRT_MASTER) &&
        p_ibrt_ctrl->p_tws_remote_dev) {
      btif_me_stop_sniff(p_ibrt_ctrl->p_tws_remote_dev);
    }

    app_ibrt_if_tws_sniff_block(15);
#else
    /// 3. guarantee performance->exit bt sniff mode
    app_bt_active_mode_set(ACTIVE_MODE_KEEPER_OTA,
                           UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif

    /// update the OTA path
    LOG_I("OTA path update:%s->%s", _path2str(otaEnv.currentPath),
          _path2str(path));
    otaEnv.currentPath = path;

    /// 4. guarantee performance->decrease the BLE connection interval
#ifdef BLE_ENABLE
    if (OTA_PATH_BLE == otaEnv.currentPath) {
      app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA, true);
    }
#endif
  }
}

/**
 * @brief Exit the OTA state handler.
 *
 * This function is used to release the system resources which are required
 * when enter the OTA state
 *
 */
__attribute__((unused)) static void _exit_ota_state(void) {
  LOG_I("%s", __func__);

  if (otaEnv.isInOtaState) {
#ifdef IBRT
    app_ibrt_ui_judge_link_policy(OTA_STOP_TRIGGER, BTIF_BLP_SNIFF_MODE);
#else
    app_bt_active_mode_clear(ACTIVE_MODE_KEEPER_OTA,
                             UPDATE_ACTIVE_MODE_FOR_ALL_LINKS);
#endif

    /// release the short TWS communication interval
#ifdef IBRT
    ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
    btdrv_reg_op_set_tws_link_duration(IBRT_TWS_LINK_DEFAULT_DURATION);
    btdrv_reg_op_set_private_tws_poll_interval(
        p_ibrt_ctrl->config.long_private_poll_interval,
        p_ibrt_ctrl->config.default_private_poll_interval_in_sco);
#endif

    /// release the short BLE connection interval
#ifdef BLE_ENABLE
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA, false);
    app_ble_update_conn_param_mode(BLE_CONN_PARAM_MODE_OTA_SLOWER, false);
#endif

    /// update the OTA path to invalid
    otaEnv.currentPath = OTA_PATH_INVALID;

    /// release the system frequency
    app_sysfreq_req(APP_SYSFREQ_USER_OTA, APP_SYSFREQ_32K);

    otaEnv.isInOtaState = false;
  } else {
    LOG_W("NOT in OTA STATE");
  }
}

#ifdef IBRT

/**
 * @brief Do the sanity check for the upgrade file.
 *
 * Currently, this function uses the CRC that was inserted by the build process.
 * This CRC check does not verify the last x bytes of the image.  x varies
 * depending on order of the key/value pair order but is < 512 bytes.
 *
 * @return true         Sanity check success.
 * @return false        Sanity check failed.
 */
static bool _image_sanity_check(void) {
  // find the location of the CRC key word string
  uint8_t *ptrOfTheLast4KImage =
      (uint8_t *)(OTA_FLASH_LOGIC_ADDR + NEW_IMAGE_FLASH_OFFSET +
                  otaEnv.totalImageSize - LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC);

  uint32_t sanityCrc32;
  uint32_t crc32ImageOffset;
  int32_t sanity_crc_location =
      _find_key_word(ptrOfTheLast4KImage, LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC,
                     (uint8_t *)imageSanityKeyWord, strlen(imageSanityKeyWord));
  if (-1 == sanity_crc_location) {
    sanity_crc_location = _find_key_word(
        ptrOfTheLast4KImage, LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC,
        (uint8_t *)oldImageSanityKeyWord, strlen(oldImageSanityKeyWord));
    if (-1 == sanity_crc_location) {
      // if no sanity crc, fail the check
      return false;
    } else {
      crc32ImageOffset = sanity_crc_location + otaEnv.totalImageSize -
                         LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC +
                         strlen(oldImageSanityKeyWord);
      sanityCrc32 = *(uint32_t *)(OTA_FLASH_LOGIC_ADDR +
                                  NEW_IMAGE_FLASH_OFFSET + crc32ImageOffset);
    }
  } else {
    crc32ImageOffset = sanity_crc_location + otaEnv.totalImageSize -
                       LEN_OF_IMAGE_TAIL_TO_FIND_SANITY_CRC +
                       strlen(imageSanityKeyWord);

    sanityCrc32 = 0;
    uint8_t *crcString = (uint8_t *)(OTA_FLASH_LOGIC_ADDR +
                                     NEW_IMAGE_FLASH_OFFSET + crc32ImageOffset);

    for (uint8_t index = 0; index < 8; index++) {
      sanityCrc32 |= (_ascii2hex(crcString[index]) << (28 - 4 * index));
    }
  }

  LOG_D("Bytes to generate crc32 is %d", crc32ImageOffset);
  LOG_D("sanity_crc_location is %d", sanity_crc_location);

  LOG_D("sanityCrc32 is 0x%x", sanityCrc32);

  // generate the CRC from image data
  uint32_t calculatedCrc32 = 0;
  calculatedCrc32 =
      crc32(calculatedCrc32,
            (uint8_t *)(OTA_FLASH_LOGIC_ADDR + NEW_IMAGE_FLASH_OFFSET),
            crc32ImageOffset);

  LOG_D("calculatedCrc32 is 0x%x", calculatedCrc32);

  if (sanityCrc32 == calculatedCrc32) {
    return true;
  }

  return false;
}

static enum NORFLASH_API_MODULE_ID_T
_get_flash_module_from_ota_device(OTA_DEVICE_E device) {
  enum NORFLASH_API_MODULE_ID_T mod = NORFLASH_API_MODULE_ID_COUNT;

  switch (device) {
  case OTA_DEVICE_APP:
    mod = NORFLASH_API_MODULE_ID_OTA;
    break;
  case OTA_DEVICE_HOTWORD:
    mod = NORFLASH_API_MODULE_ID_HOTWORD_MODEL;
    break;

  default:
    ASSERT(0, "invalid OTA device received in %s", __func__);
    break;
  }

  return mod;
}

static void _flush_data_to_flash(uint8_t *ptrSource, uint32_t lengthToBurn,
                                 uint32_t offsetInFlashToProgram,
                                 bool synWrite) {

  LOG_D("flush %d bytes to flash offset 0x%x", lengthToBurn,
        offsetInFlashToProgram);
  enum NORFLASH_API_MODULE_ID_T mod =
      _get_flash_module_from_ota_device(otaEnv.deviceId);

  uint32_t preBytes = (FLASH_SECTOR_SIZE_IN_BYTES -
                       (offsetInFlashToProgram % FLASH_SECTOR_SIZE_IN_BYTES)) %
                      FLASH_SECTOR_SIZE_IN_BYTES;

  if (lengthToBurn < preBytes) {
    preBytes = lengthToBurn;
  }

  uint32_t middleBytes = 0;
  if (lengthToBurn > preBytes) {
    middleBytes = ((lengthToBurn - preBytes) / FLASH_SECTOR_SIZE_IN_BYTES *
                   FLASH_SECTOR_SIZE_IN_BYTES);
  }

  uint32_t postBytes = 0;
  if (lengthToBurn > (preBytes + middleBytes)) {
    postBytes =
        (offsetInFlashToProgram + lengthToBurn) % FLASH_SECTOR_SIZE_IN_BYTES;
  }

  LOG_D("Prebytes is %d middlebytes is %d postbytes is %d", preBytes,
        middleBytes, postBytes);

  if (preBytes > 0) {
    app_flash_page_program(mod, offsetInFlashToProgram, ptrSource, preBytes,
                           synWrite);

    ptrSource += preBytes;
    offsetInFlashToProgram += preBytes;
  }

  uint32_t sectorIndexInFlash =
      offsetInFlashToProgram / FLASH_SECTOR_SIZE_IN_BYTES;

  if (middleBytes > 0) {
    uint32_t sectorCntToProgram = middleBytes / FLASH_SECTOR_SIZE_IN_BYTES;
    for (uint32_t sector = 0; sector < sectorCntToProgram; sector++) {
      app_flash_page_erase(mod,
                           sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
      app_flash_page_program(mod,
                             sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES,
                             ptrSource + sector * FLASH_SECTOR_SIZE_IN_BYTES,
                             FLASH_SECTOR_SIZE_IN_BYTES, synWrite);

      sectorIndexInFlash++;
    }

    ptrSource += middleBytes;
  }

  if (postBytes > 0) {
    app_flash_page_erase(mod, sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
    app_flash_page_program(mod, sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES,
                           ptrSource, postBytes, synWrite);
  }

  app_flush_pending_flash_op(mod, NORFLASH_API_ALL);
}

/**
 * NOTE that this function is stil needed since the OTA bootloader uses the CRC
 * that is passed into it to re-verify the image and that CRC is for the entire
 * image.
 */
static bool _compute_whole_image_crc(void) {
  uint32_t processedDataSize = 0;
  uint32_t crc32Value = 0;
  uint32_t bytes_to_use = 0;
  uint32_t lock;

  while (processedDataSize < otaEnv.totalImageSize) {
    if (otaEnv.totalImageSize - processedDataSize >
        OTA_DATA_CACHE_BUFFER_SIZE) {
      bytes_to_use = OTA_DATA_CACHE_BUFFER_SIZE;
    } else {
      bytes_to_use = otaEnv.totalImageSize - processedDataSize;
    }

    lock = int_lock_global();
    norflash_sync_read(
        NORFLASH_API_MODULE_ID_OTA,
        (OTA_FLASH_LOGIC_ADDR + NEW_IMAGE_FLASH_OFFSET + processedDataSize),
        otaEnv.dataCacheBuffer, OTA_DATA_CACHE_BUFFER_SIZE);
    // memcpy(otaEnv.dataCacheBuffer, (uint8_t *)(OTA_FLASH_LOGIC_ADDR +
    // NEW_IMAGE_FLASH_OFFSET + processedDataSize),
    //    OTA_DATA_CACHE_BUFFER_SIZE);
    int_unlock_global(lock);

    if (0 == processedDataSize) {
      if (*(uint32_t *)otaEnv.dataCacheBuffer != NORMAL_BOOT) {
        LOG_D("first 32bit value is not NORMAL_BOOT");
        return false;
      } else {
        *(uint32_t *)otaEnv.dataCacheBuffer = 0xFFFFFFFF;
      }
    }

    LOG_D("bytes to verify =%d.", bytes_to_use);

    crc32Value =
        crc32(crc32Value, (uint8_t *)otaEnv.dataCacheBuffer, bytes_to_use);

    processedDataSize += bytes_to_use;
  }

  LOG_D("Computed CRC32 is 0x%x.", crc32Value);

  /* This crc value will be passed to the ota app in GSoundOtaApply(). */
  otaEnv.crc32OfImage = crc32Value;
  return true;
}

static void _update_boot_info(OTA_BOOT_INFO_T *otaBootInfo) {
  ASSERT(OTA_DEVICE_APP == otaEnv.deviceId,
         "illegal OTA device try to update boot info");
  hal_norflash_disable_protection(HAL_NORFLASH_ID_0);
  enum NORFLASH_API_MODULE_ID_T mod =
      _get_flash_module_from_ota_device(otaEnv.deviceId);
  app_flash_page_erase(mod, OTA_BOOT_INFO_FLASH_OFFSET);
  app_flash_page_program(mod, OTA_BOOT_INFO_FLASH_OFFSET,
                         (uint8_t *)otaBootInfo, sizeof(OTA_BOOT_INFO_T), true);
}

static void _update_magic_number(uint32_t newMagicNumber) {
  ASSERT(OTA_DEVICE_APP == otaEnv.deviceId,
         "illegal device %d to update magic number", otaEnv.deviceId);
  memcpy(otaEnv.dataCacheBuffer,
         (uint8_t *)(OTA_FLASH_LOGIC_ADDR + otaEnv.newImageFlashOffset),
         FLASH_SECTOR_SIZE_IN_BYTES);

  *(uint32_t *)otaEnv.dataCacheBuffer = newMagicNumber;
  enum NORFLASH_API_MODULE_ID_T mod =
      _get_flash_module_from_ota_device(otaEnv.deviceId);
  app_flash_page_erase(mod, otaEnv.newImageFlashOffset);

  app_flash_page_program(mod, otaEnv.newImageFlashOffset,
                         otaEnv.dataCacheBuffer, FLASH_SECTOR_SIZE_IN_BYTES,
                         true);

  app_flush_pending_flash_op(mod, NORFLASH_API_ALL);
}

static void _update_peer_result(OTA_STATUS_E ret) {
  LOG_D("update peer result:%s->%s", _sts2str(otaEnv.peerResult),
        _sts2str(ret));

  otaEnv.peerResult = ret;
}

OTA_STATUS_E ota_common_get_peer_result(void) { return otaEnv.peerResult; }

static bool _relay_data_needed(void) {
  bool ret = true;

  if (!app_tws_ibrt_tws_link_connected()) {
    ret = false;
  }

  if (OTA_STAGE_IDLE == otaEnv.currentStage) {
    ret = false;
  }

  /// check the customized relay_data_needed permission
  if (ret && otaEnv.customRelayNeededHandler) {
    ret = otaEnv.customRelayNeededHandler();
  }

  return ret;
}

static void _ota_relay_data(OTA_COMMAND_E cmdType, const uint8_t *dataPtr,
                            uint16_t length) {
  uint8_t frameNum = 0;
  uint16_t frameLen = 0;
  OTA_TWS_DATA_T tCmd = {
      cmdType,
  };

  ASSERT(length <= ARRAY_SIZE(tCmd.data), "ILLEGAL relay packet length");

  /// split packet into servel frame to fulfill the max TWS data
  /// transmission length requirement
  if (length % OTA_TWS_PAYLOAD_MAX_LEN) {
    frameNum = length / OTA_TWS_PAYLOAD_MAX_LEN + 1;
  } else {
    frameNum = length / OTA_TWS_PAYLOAD_MAX_LEN;
  }

  LOG_D("packet len:%d, splited frame num:%d", length, frameNum);

  /// push all data into queue
  for (uint8_t i = 0; i < frameNum; i++) {
    if ((i + 1) == frameNum) {
      tCmd.magicCode = OTA_RELAY_PACKET_MAGIC_CODE_COMPLETE;
      tCmd.length = length % OTA_TWS_PAYLOAD_MAX_LEN;
    } else {
      tCmd.magicCode = OTA_RELAY_PACKET_MAGIC_CODE_INCOMPLETE;
      tCmd.length = OTA_TWS_PAYLOAD_MAX_LEN;
    }

    frameLen = OTA_TWS_HEAD_SIZE + tCmd.length;

    memcpy(tCmd.data, dataPtr + i * OTA_TWS_PAYLOAD_MAX_LEN, tCmd.length);

    osMutexWait(txQueueMutexID, osWaitForever);
    ASSERT(CQ_OK == EnCQueue(&otaEnv.txQueue, (CQItemType *)&frameLen,
                             sizeof(frameLen)),
           "%s failed to push data to queue", __func__);
    ASSERT(CQ_OK == EnCQueue(&otaEnv.txQueue, (CQItemType *)&tCmd, frameLen),
           "%s failed to push data to queue", __func__);
    osMutexRelease(txQueueMutexID);
  }

  /// inform tx thread to handle the data to be transmitted
  osSignalSet(txThreadId, OTA_TWS_TX_SIGNAL);
}

/**
 * @brief Check the validity of received data frame.
 *
 * @param dataPtr       Pointer of received data
 * @param length        length of received data
 * @return true         Received data frame is valid
 * @return false        Received data frame is invalid
 */
static bool _tws_frame_validity_check(uint8_t *dataPtr, uint32_t length) {
  bool isValid = true;

  OTA_TWS_DATA_T *otaTwsData = (OTA_TWS_DATA_T *)dataPtr;

  ASSERT(TWS_RELAY_DATA_MAX_SIZE >= length,
         "received overloaded data packet!!!");
  if (OTA_RELAY_PACKET_MAGIC_CODE_INCOMPLETE == otaEnv.currentMagicCode &&
      otaTwsData->cmdType != otaEnv.currentCmdType) {
    ASSERT(0, "bad frame is received!!!");
  }

  if (length != otaTwsData->length + OTA_TWS_HEAD_SIZE) {
    LOG_W("INVALID packet, dataLen:%d, expected dataLen:%d", length,
          otaTwsData->length + OTA_TWS_HEAD_SIZE);
    isValid = false;
  }

  if (OTA_RELAY_PACKET_MAGIC_CODE_COMPLETE != otaTwsData->magicCode &&
      OTA_RELAY_PACKET_MAGIC_CODE_INCOMPLETE != otaTwsData->magicCode) {
    LOG_W("INVALID magic code:0x%08x", otaTwsData->magicCode);
    isValid = false;
  }

  return isValid;
}

/**
 * @brief Response the master for the OTA_COMMAND received.
 *
 * This function will response the master after a whole packet receiving is
 * done.
 *
 * @param status        Status of receiving TWS relay data
 */
static void _tws_rsp(OTA_STATUS_E status) {
  LOG_D("[%s] status:%s", __func__, _sts2str(status));

  OTA_STATUS_E rsp = status;
  _ota_relay_data(OTA_COMMAND_RSP, (const uint8_t *)&rsp, sizeof(rsp));
}
#endif

OTA_COMMON_ENV_T *ota_common_get_env(void) { return &otaEnv; }

/**
 * @brief Reset the OTA environment.
 *
 */
static void _reset_env(void) {
  LOG_D("[%s]", __func__);
  memset(&otaEnv, 0, sizeof(otaEnv));

#ifdef __APP_USER_DATA_NV_FLASH_OFFSET__
  otaEnv.userDataNvFlashOffset = __APP_USER_DATA_NV_FLASH_OFFSET__;
#else
  otaEnv.userDataNvFlashOffset =
      hal_norflash_get_flash_total_size(HAL_NORFLASH_ID_0) - 2 * 4096;
#endif

  otaEnv.flashOffsetOfUserDataPool = otaEnv.userDataNvFlashOffset;
  _update_ota_user(OTA_USER_NUM);

#ifdef OTA_NVRAM
  // gOtaCtx.cfg.clearUserData = false;
  // gOtaCtx.flashOffsetOfFactoryDataPool =
  //     otaEnv.otaCommon->userDataNvFlashOffset + FLASH_SECTOR_SIZE_IN_BYTES;
#endif

#ifdef IBRT
  /// init tws TX queue
  osMutexWait(txQueueMutexID, osWaitForever);
  InitCQueue(&(otaEnv.txQueue), ARRAY_SIZE(otaEnv.txBuf),
             (CQItemType *)otaEnv.txBuf);
  osMutexRelease(txQueueMutexID);

  /// init tws RX queue
  osMutexWait(rxQueueMutexID, osWaitForever);
  InitCQueue(&(otaEnv.rxQueue), ARRAY_SIZE(otaEnv.rxBuf),
             (CQItemType *)otaEnv.rxBuf);
  osMutexRelease(rxQueueMutexID);
#endif
}

void ota_common_init_handler(void) {
  LOG_D("[%s]", __func__);

  /// init OTA related nvrecord pointer
  nv_record_ota_init();

  /// init common used flash module
  ota_common_init_flash(
      (uint8_t)NORFLASH_API_MODULE_ID_OTA, 0,
      (OTA_FLASH_LOGIC_ADDR + (
#ifdef __APP_USER_DATA_NV_FLASH_OFFSET__
                                  __APP_USER_DATA_NV_FLASH_OFFSET__
#else
                                  (hal_norflash_get_flash_total_size(
                                       HAL_NORFLASH_ID_0) -
                                   2 * 4096)
#endif
                                  & 0xffffff)),
      0);

#ifdef IBRT
  if (txThreadId == NULL) {
    txThreadId = osThreadCreate(osThread(_ota_tws_thread), NULL);
  }

  /// init tws TX queue mutex
  if (txQueueMutexID == NULL) {
    txQueueMutexID = osMutexCreate((osMutex(twsTxQueueMutex)));
  }

  /// init tws RX queue mutex
  if (rxQueueMutexID == NULL) {
    rxQueueMutexID = osMutexCreate((osMutex(twsRxQueueMutex)));
  }
#endif

  _reset_env();
}

void ota_common_enable_sanity_check(bool enable) {
  LOG_I("sanity check enable state update:%d->%d", otaEnv.sanityCheckEnable,
        enable);
  otaEnv.sanityCheckEnable = enable;
}

void ota_common_init_flash(uint8_t module, uint32_t baseAddr, uint32_t len,
                           uint32_t imageHandler) {
  enum NORFLASH_API_RET_T ret;
  uint32_t block_size = 0;
  uint32_t sector_size = 0;
  uint32_t page_size = 0;

  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);

  LOG_I("%s module:%d, startAddr:0x%x, len:0x%x", __func__, module, baseAddr,
        len);
  ret = norflash_api_register((NORFLASH_API_MODULE_ID_T)module,
                              HAL_NORFLASH_ID_0, baseAddr, len, block_size,
                              sector_size, page_size, OTA_NORFLASH_BUFFER_LEN,
                              (NORFLASH_API_OPERA_CB)imageHandler);

  ASSERT(ret == NORFLASH_API_OK,
         "ota_init_flash: norflash_api register failed,ret = %d.", ret);

#ifdef FLASH_SUSPEND
  norflash_suspend_check_irq(AUDMA_IRQn);
  norflash_suspend_check_irq(ISDATA_IRQn);
  norflash_suspend_check_irq(ISDATA1_IRQn);
#endif
}

bool ota_common_is_in_progress(void) {
  bool ret = false;

  if (OTA_STAGE_IDLE != otaEnv.currentStage) {
    ret = true;
  }

  return ret;
}

void ota_common_registor_command_handler(OTA_COMMAND_E cmdType,
                                         void *cmdHandler) {
  if (otaEnv.cmdHandler[cmdType]) {
    LOG_W("handler for OTA command %d is not NULL", cmdType);
  }

  otaEnv.cmdHandler[cmdType] = (OTA_CMD_HANDLER_T)cmdHandler;
}

OTA_STATUS_E ota_common_command_received_handler(OTA_COMMAND_E cmdType,
                                                 void *cmdInfo,
                                                 uint16_t cmdLen) {
  LOG_D("cmd received:%s", _cmd2str(cmdType));
  otaEnv.status = OTA_STATUS_OK;
  OTA_STATUS_E temp = OTA_STATUS_OK;

  /// init OTA progress when received begin command
  if (OTA_COMMAND_BEGIN == cmdType) {
    ASSERT(OTA_STAGE_IDLE == otaEnv.currentStage,
           "Received begin command while not in IDLE stage");
    OTA_BEGIN_PARAM_T *c = (OTA_BEGIN_PARAM_T *)cmdInfo;

    /// update the ota user to ota common layer
    _update_ota_user(c->user);

    /// update the new image size according to the image size param
    LOG_I("total image size update:%d->%d", otaEnv.totalImageSize,
          c->imageSize);
    otaEnv.totalImageSize = c->imageSize;

    /// update the new image offset according to the start offset param
    ASSERT(0 == (c->startOffset % FLASH_SECTOR_SIZE_IN_BYTES),
           "Resumed start offset is not 4KB aligned!");
    LOG_I("programOffset&receivedDataSize is update:%d->%d",
          otaEnv.newImageProgramOffset, c->startOffset);
    otaEnv.newImageProgramOffset = c->startOffset;
    otaEnv.receivedDataSize = c->startOffset;

    /// update the device index
    LOG_I("deviceId update:%d->%d", otaEnv.deviceId, c->device);
    otaEnv.deviceId = c->device;

    /// update the version info and version length
    memcpy(otaEnv.version, c->version, c->versionLen);
    otaEnv.versionLen = c->versionLen;

    /// update the flash offset of new image
    LOG_I("newImageFlashOffset update:0x%02x->0x%02x",
          otaEnv.newImageFlashOffset, c->flashOffset);
    otaEnv.newImageFlashOffset = c->flashOffset;

    /// set the OTA stage to ongoing
    _set_ota_stage(OTA_STAGE_ONGOING);

    /// system layer configurations to guarentee the performance
    /// of all functionalities
    _enter_ota_state(c->path);
  }

  /// execute the custom configuration
  if (OTA_COMMAND_NUM != cmdType) {
    if (otaEnv.cmdHandler[cmdType]) {
      temp = otaEnv.cmdHandler[cmdType](cmdInfo, cmdLen);
      LOG_I("ota command %s handle done, status:%s", _cmd2str(cmdType),
            _sts2str(temp));

      if (OTA_STATUS_OK != temp) {
        otaEnv.status = temp;
      }
    } else {
      LOG_W("ota cmd %s is not handled by customor", _cmd2str(cmdType));
    }
  } else {
    LOG_E("INVALID cmd:%d", cmdType);
  }

  /// update the OTA progress
  if (OTA_STATUS_OK == otaEnv.status) {
    if (!nv_record_ota_update_info(otaEnv.currentUser, otaEnv.deviceId,
                                   otaEnv.currentStage, otaEnv.totalImageSize,
                                   otaEnv.version)) {
      LOG_W("update info failed");
      otaEnv.status = OTA_STATUS_ERROR;
    }
  }

  /// special process for OTA_ABORT&&OTA_APPLY command
  if ((OTA_COMMAND_ABORT == cmdType) || (OTA_COMMAND_APPLY == cmdType)) {
    _reset_env();
  }

  LOG_I("%s cmd process result:%s", __func__, _sts2str(otaEnv.status));
  return otaEnv.status;
}

#ifdef OTA_NVRAM
static void ota_update_nv_data(void) {
  if (otaEnv.configuration.isToClearUserData) {
    enum NORFLASH_API_MODULE_ID_T mod =
        _get_flash_module_from_ota_device(otaEnv.deviceId);
    app_flash_page_erase(mod, sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);
  }

  if (otaEnv.configuration.isToRenameBT || otaEnv.configuration.isToRenameBLE ||
      otaEnv.configuration.isToUpdateBTAddr ||
      otaEnv.configuration.isToUpdateBLEAddr) {
    uint32_t *pOrgFactoryData, *pUpdatedFactoryData;
    pOrgFactoryData = (uint32_t *)(OTA_FLASH_LOGIC_ADDR +
                                   otaEnv.flashOffsetOfFactoryDataPool);
    memcpy(otaEnv.dataCacheBuffer, (uint8_t *)pOrgFactoryData,
           FLASH_SECTOR_SIZE_IN_BYTES);
    pUpdatedFactoryData = (uint32_t *)&(otaEnv.dataCacheBuffer);

    if (NVREC_DEV_VERSION_1 == nv_record_dev_rev) {
      if (otaEnv.configuration.isToRenameBT) {
        memset((uint8_t *)(pUpdatedFactoryData + dev_name), 0,
               sizeof(uint32_t) * (dev_bt_addr - dev_name));

        memcpy((uint8_t *)(pUpdatedFactoryData + dev_name),
               (uint8_t *)(otaEnv.configuration.newBTName), NAME_LENGTH);
      }

      if (otaEnv.configuration.isToUpdateBTAddr) {
        memcpy((uint8_t *)(pUpdatedFactoryData + dev_bt_addr),
               (uint8_t *)(otaEnv.configuration.newBTAddr), BD_ADDR_LENGTH);
      }

      if (otaEnv.configuration.isToUpdateBLEAddr) {
        memcpy((uint8_t *)(pUpdatedFactoryData + dev_ble_addr),
               (uint8_t *)(otaEnv.configuration.newBLEAddr), BD_ADDR_LENGTH);
      }

      pUpdatedFactoryData[dev_crc] =
          crc32(0, (uint8_t *)(&pUpdatedFactoryData[dev_reserv1]),
                (dev_data_len - dev_reserv1) * sizeof(uint32_t));
    } else {
      if (otaEnv.configuration.isToRenameBT) {
        memset((uint8_t *)(pUpdatedFactoryData + rev2_dev_name), 0,
               sizeof(uint32_t) * (rev2_dev_bt_addr - rev2_dev_name));

        memcpy((uint8_t *)(pUpdatedFactoryData + rev2_dev_name),
               (uint8_t *)(otaEnv.configuration.newBTName), NAME_LENGTH);
      }

      if (otaEnv.configuration.isToRenameBLE) {
        memset((uint8_t *)(pUpdatedFactoryData + rev2_dev_ble_name), 0,
               sizeof(uint32_t) * (rev2_dev_section_end - rev2_dev_ble_name));

        memcpy((uint8_t *)(pUpdatedFactoryData + rev2_dev_ble_name),
               (uint8_t *)(otaEnv.configuration.newBleName),
               BLE_NAME_LEN_IN_NV);
      }

      if (otaEnv.configuration.isToUpdateBTAddr) {
        memcpy((uint8_t *)(pUpdatedFactoryData + rev2_dev_bt_addr),
               (uint8_t *)(otaEnv.configuration.newBTAddr), BD_ADDR_LENGTH);
      }

      if (otaEnv.configuration.isToUpdateBLEAddr) {
        memcpy((uint8_t *)(pUpdatedFactoryData + rev2_dev_ble_addr),
               (uint8_t *)(otaEnv.configuration.newBLEAddr), BD_ADDR_LENGTH);
      }

      pUpdatedFactoryData[dev_crc] = crc32(
          0, (uint8_t *)(&pUpdatedFactoryData[rev2_dev_section_start_reserved]),
          pUpdatedFactoryData[rev2_dev_data_len]);
    }

    enum NORFLASH_API_MODULE_ID_T mod =
        _get_flash_module_from_ota_device(otaEnv.deviceId);
    app_flash_page_erase(mod, sectorIndexInFlash * FLASH_SECTOR_SIZE_IN_BYTES);

    app_flash_page_program(mod, otaEnv.flashOffsetOfFactoryDataPool,
                           (uint8_t *)pUpdatedFactoryData,
                           FLASH_SECTOR_SIZE_IN_BYTES);
  }
}
#endif

#ifdef IBRT
void ota_common_registor_relay_needed_handler(void *handler) {
  if (otaEnv.customRelayNeededHandler) {
    LOG_W("handler for custom relay needed judge is not NULL");
  }

  otaEnv.customRelayNeededHandler = (CUSTOM_RELAY_NEEDED_FUNC_T)handler;
}

void ota_common_registor_peer_cmd_received_handler(void *handler) {
  if (otaEnv.peerCmdReceivedHandler) {
    LOG_W("handler for peer cmd received is not NULL");
  }

  otaEnv.peerCmdReceivedHandler = (PEER_CMD_RECEIVED_HANDLER_T)handler;
}

OTA_STATUS_E ota_common_relay_data_to_peer(OTA_COMMAND_E cmdType,
                                           const uint8_t *data, uint16_t len) {
  OTA_STATUS_E status = OTA_STATUS_OK;
  if (_relay_data_needed()) {
    _ota_relay_data(cmdType, (const uint8_t *)data, len);
  } else {
    status = OTA_STATUS_ERROR;
  }

  return status;
}

OTA_STATUS_E ota_common_receive_peer_rsp(void) {
  OTA_STATUS_E status = OTA_STATUS_OK;

  if (_relay_data_needed()) {
    /// get current thread ID as tws rx thread
    rxThreadId = osThreadGetId();

    /// pending current thread to waitting for slave response
    evt = osSignalWait(OTA_TWS_RX_SIGNAL, OTA_TWS_RELAY_WAITTIME);

    if (evt.status == osEventTimeout) {
      status = OTA_STATUS_ERROR_RELAY_TIMEOUT;
      LOG_W("[%s]SignalWait TIMEOUT!", __func__);
    } else if (osEventSignal == evt.status) {
      status = ota_common_get_peer_result();
    }

    ///.clear the excute result of this time
    _update_peer_result(OTA_STATUS_OK);
  } else {
    status = OTA_STATUS_ERROR;
  }

  return status;
}

OTA_STATUS_E ota_common_fw_data_write(const uint8_t *data, uint16_t len) {
  OTA_STATUS_E status = OTA_STATUS_OK;
  uint16_t leftDataSize = len;
  uint32_t offsetInReceivedRawData = 0;

  do {
    uint32_t bytesToCopy;
    // copy to data buffer
    if ((otaEnv.dataCacheBufferOffset + leftDataSize) >
        OTA_DATA_CACHE_BUFFER_SIZE) {
      bytesToCopy = OTA_DATA_CACHE_BUFFER_SIZE - otaEnv.dataCacheBufferOffset;
    } else {
      bytesToCopy = leftDataSize;
    }

    leftDataSize -= bytesToCopy;

    memcpy(&otaEnv.dataCacheBuffer[otaEnv.dataCacheBufferOffset],
           &data[offsetInReceivedRawData], bytesToCopy);
    offsetInReceivedRawData += bytesToCopy;
    otaEnv.dataCacheBufferOffset += bytesToCopy;

    ASSERT(otaEnv.dataCacheBufferOffset <= OTA_DATA_CACHE_BUFFER_SIZE,
           "bad math in %s", __func__);
    if (OTA_DATA_CACHE_BUFFER_SIZE == otaEnv.dataCacheBufferOffset) {
      _flush_data_to_flash(
          otaEnv.dataCacheBuffer, OTA_DATA_CACHE_BUFFER_SIZE,
          (otaEnv.newImageProgramOffset + otaEnv.newImageFlashOffset), false);
      otaEnv.newImageProgramOffset += OTA_DATA_CACHE_BUFFER_SIZE;
      otaEnv.dataCacheBufferOffset = 0;
    }
  } while (offsetInReceivedRawData < len);

  otaEnv.receivedDataSize += len;

  // check whether all image data has been received
  if (otaEnv.receivedDataSize == otaEnv.totalImageSize) {
    LOG_D("The final image programming and crc32 check.");

    // flush any partial buffer to flash
    if (otaEnv.dataCacheBufferOffset != 0) {
      _flush_data_to_flash(
          otaEnv.dataCacheBuffer, otaEnv.dataCacheBufferOffset,
          (otaEnv.newImageProgramOffset + otaEnv.newImageFlashOffset), true);
    }

    if (OTA_DEVICE_APP == otaEnv.deviceId) {
      bool check = true;

      /// check the sanity if required
      if (otaEnv.sanityCheckEnable) {
        check = _image_sanity_check();
      }

      if (check) {
        /// update the magic code of the application image
        _update_magic_number(NORMAL_BOOT);

        /// check the crc32 of the received image data
        if (_compute_whole_image_crc()) {
          LOG_I("Whole image verification pass.");

          /// update the OTA stage to OTA done
          _set_ota_stage(OTA_STAGE_DONE);
        } else {
          LOG_W("image verification failed @%d", __LINE__);

          /// update the OTA stage to OTA idle
          _set_ota_stage(OTA_STAGE_IDLE);
        }
      } else {
        /// sanity check failed
        LOG_W("image verification failed @%d", __LINE__);

        /// update the OTA stage to OTA idle
        _set_ota_stage(OTA_STAGE_IDLE);
      }
    } else {
      LOG_I("download finished, device:%d", otaEnv.deviceId);

      /// update the OTA stage to OTA idle
      _set_ota_stage(OTA_STAGE_DONE);
    }

    /// whole image verification failed somehow
    if (OTA_STAGE_IDLE == otaEnv.currentStage) {
      nv_record_ota_update_info(otaEnv.currentUser, otaEnv.deviceId,
                                otaEnv.currentStage, 0, INVALID_VERSION_STR);

      status = OTA_STATUS_ERROR_CHECKSUM;
    } else //!< whole image verification passed
    {
      nv_record_ota_update_info(otaEnv.currentUser, otaEnv.deviceId,
                                otaEnv.currentStage, otaEnv.totalImageSize,
                                otaEnv.version);
    }

    /// exit the OTA state
    _exit_ota_state();
  } else //!< whole image revceive not finished
  {
    LOG_D("Received image size:%d", otaEnv.receivedDataSize);

    /// update the break point if it is changed
    if ((otaEnv.receivedDataSize - otaEnv.breakPoint) >=
        OTA_BREAKPOINT_STORE_GRANULARITY) {
      otaEnv.breakPoint =
          (otaEnv.receivedDataSize / OTA_BREAKPOINT_STORE_GRANULARITY) *
          OTA_BREAKPOINT_STORE_GRANULARITY;

      LOG_I("update record offset to %d", otaEnv.breakPoint);
      nv_record_ota_update_breakpoint(otaEnv.currentUser, otaEnv.deviceId,
                                      otaEnv.breakPoint);
    }
  }

  return status;
}

void ota_common_apply_current_fw(void) {
  OTA_BOOT_INFO_T otaBootInfo = {COPY_NEW_IMAGE, otaEnv.totalImageSize,
                                 otaEnv.crc32OfImage};
  _update_boot_info(&otaBootInfo);

#ifdef OTA_NVRAM
  ota_update_nv_data();
#endif

  app_start_postponed_reset();
}

void ota_common_on_relay_data_received(uint8_t *ptrParam, uint32_t paramLen) {
  ASSERT(ptrParam, "invalid data pointer received in %s", __func__);
  ASSERT(paramLen <= TWS_RELAY_DATA_MAX_SIZE,
         "illegal parameter length received in %s", __func__);

  OTA_TWS_DATA_T *otaTwsData = (OTA_TWS_DATA_T *)ptrParam;
  LOG_D("[%s] paramLen:%d, cmdType:%d|%s", __func__, paramLen,
        otaTwsData->cmdType, _cmd2str(otaTwsData->cmdType));

  OTA_STATUS_E status = OTA_STATUS_OK;
  OTA_BEGIN_PARAM_T *beginInfo = NULL;
  uint16_t packetLen = 0;

  if (_tws_frame_validity_check(ptrParam, paramLen)) {
    /// update the magic code and command type
    otaEnv.currentMagicCode = otaTwsData->magicCode;
    otaEnv.currentCmdType = otaTwsData->cmdType;

    /// push received data into tws OTA data receive queue
    osMutexWait(rxQueueMutexID, osWaitForever);
    ASSERT(CQ_OK == EnCQueue(&otaEnv.rxQueue, (CQItemType *)otaTwsData->data,
                             (paramLen - OTA_TWS_HEAD_SIZE)),
           "%s failed to push data to queue, avaiable:%d, push:%d", __func__,
           AvailableOfCQueue(&otaEnv.rxQueue), (paramLen - OTA_TWS_HEAD_SIZE));
    osMutexRelease(rxQueueMutexID);

    /// whole packet from APP received done
    if (OTA_RELAY_PACKET_MAGIC_CODE_COMPLETE == otaEnv.currentMagicCode) {
      osMutexWait(rxQueueMutexID, osWaitForever);
      packetLen = LengthOfCQueue(&otaEnv.rxQueue);
      LOG_D("length of rx queue:%d", packetLen);
      DeCQueue(&otaEnv.rxQueue, otaEnv.tempRxBuf, packetLen);
      osMutexRelease(rxQueueMutexID);

      switch (otaEnv.currentCmdType) {
      case OTA_COMMAND_RSP:
        /// retrieve and update the status of peer
        status = ((OTA_RESPONSE_PARAM_T *)otaEnv.tempRxBuf)->status;
        _update_peer_result(status);

        /// inform the receiving thread to proceed
        osSignalSet(rxThreadId, OTA_TWS_RX_SIGNAL);
        break;

      /// add other supported command here
      case OTA_COMMAND_BEGIN:
        beginInfo = (OTA_BEGIN_PARAM_T *)otaTwsData->data;
        if (beginInfo->initializer) {
          beginInfo->initializer();
        } else {
          LOG_E("initializer not registored");
        }
      case OTA_COMMAND_DATA:
      case OTA_COMMAND_APPLY:
      case OTA_COMMAND_ABORT:
        if (otaEnv.peerCmdReceivedHandler) {
          status = otaEnv.peerCmdReceivedHandler(
              otaEnv.currentCmdType, (const uint8_t *)otaEnv.tempRxBuf,
              packetLen);
        } else {
          LOG_W("peerCmdReceivedHandler is not registored");
        }

        break;

      default:
        ASSERT(0, "INVALID cmd received");
        status = OTA_STATUS_ERROR;
        break;
      }

      /// response to master if needed
      if (OTA_COMMAND_RSP != otaTwsData->cmdType) {
        _tws_rsp(status);
      }

      otaEnv.currentMagicCode = OTA_RELAY_PACKET_MAGIC_CODE_INVALID;
      otaEnv.currentCmdType = OTA_COMMAND_NUM;
    }
  } else {
    LOG_W("Received data frame is invalid");
    status = OTA_STATUS_ERROR;

    switch (otaTwsData->cmdType) {
    case OTA_COMMAND_RSP:
      _update_peer_result(status);
      osSignalSet(rxThreadId, OTA_TWS_RX_SIGNAL);
      break;

    case OTA_COMMAND_BEGIN:
    case OTA_COMMAND_DATA:
    case OTA_COMMAND_APPLY:
    case OTA_COMMAND_ABORT:
      _tws_rsp(status);
      break;

    default:
      ASSERT(0, "INVALID command received");
      break;
    }
  }
}

POSSIBLY_UNUSED static void _ota_tws_deinit(void) {
  ResetCQueue(&otaEnv.txQueue);
  ResetCQueue(&otaEnv.rxQueue);
  memset(otaEnv.txBuf, 0, ARRAY_SIZE(otaEnv.txBuf));
  memset(otaEnv.rxBuf, 0, ARRAY_SIZE(otaEnv.rxBuf));
  memset(otaEnv.tempRxBuf, 0, ARRAY_SIZE(otaEnv.tempRxBuf));

  if (txThreadId) {
    osThreadTerminate(txThreadId);
    txThreadId = NULL;
  }

  if (txQueueMutexID != NULL) {
    osMutexDelete(txQueueMutexID);
    txQueueMutexID = NULL;
  }

  if (rxQueueMutexID != NULL) {
    osMutexDelete(rxQueueMutexID);
    rxQueueMutexID = NULL;
  }
}

static void _ota_tws_thread(const void *arg) {
  volatile uint16_t qLen = 0;
  volatile uint16_t packetLen = 0;

  while (1) {
    osMutexWait(txQueueMutexID, osWaitForever);
    qLen = LengthOfCQueue(&otaEnv.txQueue);
    osMutexRelease(txQueueMutexID);
    LOG_D("queued data len:%d", qLen);

    while (qLen) {
      /// check the validity of queue length
      /// ASSERT(qLen <= OTA_MAX_MTU, "invalid OTA relay data len");

      osMutexWait(txQueueMutexID, osWaitForever);
      /// retrive and check the validity of packet length
      DeCQueue(&otaEnv.txQueue, (CQItemType *)&packetLen, 2);
      ASSERT(packetLen <= TWS_RELAY_DATA_MAX_SIZE,
             "invalid OTA relay data len:%d", packetLen);

      /// retrieve the data to transmit
      DeCQueue(&otaEnv.txQueue, (CQItemType *)relayBuf, packetLen);
      osMutexRelease(txQueueMutexID);

      LOG_D("send data len:%d", packetLen);

      /// send data to peer
      tws_ctrl_send_cmd(IBRT_COMMON_OTA, relayBuf, packetLen);
      memset(relayBuf, 0, TWS_RELAY_DATA_MAX_SIZE);

      osMutexWait(txQueueMutexID, osWaitForever);
      qLen = LengthOfCQueue(&otaEnv.txQueue);
      osMutexRelease(txQueueMutexID);
    }

    osSignalWait(OTA_TWS_TX_SIGNAL, osWaitForever);
  }
}

static void _sync_info_prepare_handler(uint8_t *buf, uint16_t *length) {
  *length = OTA_DEVICE_CNT * sizeof(NV_OTA_INFO_T);

  void *otaInfo = NULL;
  nv_record_ota_get_ptr(&otaInfo);

  memcpy(buf, otaInfo, *length);
}

static void _sync_info_received_handler(uint8_t *buf, uint16_t length) {
  // uodate gsound info
  // TODO:
}

void ota_common_tws_sync_init(void) {
  TWS_SYNC_USER_T userOta = {
      _sync_info_prepare_handler,
      _sync_info_received_handler,
      _sync_info_prepare_handler,
      NULL,
      NULL,
  };

  app_tws_if_register_sync_user(TWS_SYNC_USER_OTA, &userOta);
}
#endif
