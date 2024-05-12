/**
 * @file ota_common.h
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

#ifndef __OTA_COMMON_H__
#define __OTA_COMMON_H__

#ifdef __cplusplus
extern "C"{
#endif

/*****************************header include********************************/
#include "cmsis_os.h"
#include "cqueue.h"

/******************************macro defination*****************************/
/**
 * @brief Flash base address.
 * 
 */
#define OTA_FLASH_LOGIC_ADDR (FLASH_NC_BASE)

/**
 * @brief max version string length supported by ota common layer.
 * 
 */
#define MAX_VERSION_LEN 20


/**
 * @brief boot up flag
 * 
 */
#ifndef NORMAL_BOOT
#define NORMAL_BOOT 0xBE57EC1C
#endif

/**
 * @brief This flag is used to inform OTA bin to copy the new image to APP
 * image section.
 * 
 */
#ifndef COPY_NEW_IMAGE
#define COPY_NEW_IMAGE 0x5A5A5A5A
#endif


/**
 * @brief offset in flash to write the new image.
 * 
 */
#ifndef NEW_IMAGE_FLASH_OFFSET
#define NEW_IMAGE_FLASH_OFFSET    0x180000
#endif


/**
 * @brief flash sector size
 * 
 * 4K is the page size of flash
 * 
 */
#ifndef FLASH_SECTOR_SIZE_IN_BYTES
#define FLASH_SECTOR_SIZE_IN_BYTES 4096
#endif


/**
 * @brief cache buffer size for OTA data.
 * 
 */
#ifndef OTA_DATA_CACHE_BUFFER_SIZE
#define OTA_DATA_CACHE_BUFFER_SIZE FLASH_SECTOR_SIZE_IN_BYTES
#endif


/**
 * @brief this value is used for configure the norflash write buffer.
 * 
 */
#define OTA_NORFLASH_BUFFER_LEN (OTA_DATA_CACHE_BUFFER_SIZE * 2)


/**
 * @brief this flag is used to mark if platform support automatic OTA.
 * 
 * If Target indicates the device supports Automatic OTA, then the new firmware
 * image does not have to be applied immediately after firmware transfer has
 * completed. Instead the device can choose an opportune moment to perform the
 * Apply.
 * 
 */
#define SUPPORT_AUTOMATIC_OTA false


#ifdef IBRT
/// max data size per packet of tws relay
/// this value should equal to @see APP_TWS_CTRL_BUFFER_MAX_LEN
#define TWS_RELAY_DATA_MAX_SIZE 512

/// max data size per packet of OTA APP
/// NOTE: this value depends on specific OTA spec
#define OTA_MAX_MTU 700

#define OTA_RELAY_PACKET_MAGIC_CODE_INVALID 0x44454144

/// mark the whole packet data from APP is completely transmitted
#define OTA_RELAY_PACKET_MAGIC_CODE_COMPLETE 0x28269395

/// mark the whole packet data from APP is not completely transmitted
#define OTA_RELAY_PACKET_MAGIC_CODE_INCOMPLETE 0x17188284

/// prefix of the gsuond_tws_data see @OTA_TWS_DATA_T
#define OTA_TWS_HEAD_SIZE (sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t))

/// payload max length is tws_buffer_size minimize the prefix, see @GSOUND_OTA_TWS_T
#define OTA_TWS_PAYLOAD_MAX_LEN (TWS_RELAY_DATA_MAX_SIZE - OTA_TWS_HEAD_SIZE)

#define OTA_TWS_RX_SIGNAL (1 << 10)
#define OTA_TWS_TX_SIGNAL (1 << 11)

#define OTA_TWS_RELAY_WAITTIME 2000
#endif // #ifdef IBRT


#ifdef OTA_NVRAM
#define BD_ADDR_LENGTH              6
#define NAME_LENGTH                 32
#endif

/******************************type defination******************************/
/**
 * @brief Boot up info structure.
 * 
 */
typedef struct
{
    uint32_t magicNumber;
    uint32_t imageSize;
    uint32_t imageCrc;
}__attribute__ ((__packed__)) OTA_BOOT_INFO_T;

/**
 * @brief enum for OTA command.
 * 
 */
typedef enum
{
    OTA_COMMAND_BEGIN = 0, //!< OTA_BEGIN command identifier
    OTA_COMMAND_DATA  = 1, //!< OTA_DATA command identifier
    OTA_COMMAND_APPLY = 2, //!< OTA_APPLY command identifier
    OTA_COMMAND_ABORT = 3, //!< OTA_ABORT command identifier
#ifdef IBRT
    OTA_COMMAND_RSP   = 4, //!< used to response the tws OTA data relay
#endif

    OTA_COMMAND_NUM, //!< number of valid OTA command
}OTA_COMMAND_E;

/**
 * @brief enum for OTA user.
 * 
 */
typedef enum
{
    OTA_USER_BES            = 0, //!< OTA user: BES
    OTA_USER_COLORFUL       = 1, //!< OTA user: GOOGLE
    OTA_USER_ORANGE         = 2, //!< OTA user: ALI
    OTA_USER_RED            = 3, //!< OTA user: HUAWEI
    OTA_USER_GREEN          = 4, //!< OTA user: OPPO
    // and other possible OTA user here

    OTA_USER_NUM, //!< OTA user number
} OTA_USER_E;

/**
 * @brief enum for OTA path.
 * 
 */
typedef enum
{
    OTA_PATH_INVALID    = 0, //!< invalid OTA path
    OTA_PATH_BT         = 1, //!< OTA path: BT
    OTA_PATH_BLE        = 2, //!< OTA path: BLE
} OTA_PATH_E;

/**
 * @brief enum for OTA device.
 * 
 */
typedef enum
{
    OTA_DEVICE_APP      = 0,
    OTA_DEVICE_HOTWORD  = 1,
    // OTA_DEVICE_BOX      = 2,

    OTA_DEVICE_NUM,
}OTA_DEVICE_E;

/**
 * @brief enum for OTA status
 * 
 */
typedef enum
{
    OTA_STATUS_OK                   = 0, //!< status OK
    OTA_STATUS_ERROR                = 1, //!< status error
    OTA_STATUS_ERROR_RELAY_TIMEOUT  = 2, //!< tws data relay timeout
    OTA_STATUS_ERROR_CHECKSUM       = 3, //!< checksum error
    OTA_STATUS_ERROR_NOT_ALLOWED    = 4, //!< ota not allowed error

    OTA_STATUS_NUM,
}OTA_STATUS_E;

/**
 * @brief enum for OTA stage
 * 
 */
typedef enum
{
    OTA_STAGE_IDLE      = 0, //!< OTA is in idle state
    OTA_STAGE_ONGOING   = 1, //!< OTA is ongoing
    OTA_STAGE_DONE      = 2, //!< OTA is done
    OTA_STAGE_APPLY     = 3, //!< OTA is ready to apply,
                             //!< will apply at convenience

    OTA_STAGE_NUM, //!< OTA stage number
}OTA_STAGE_E;

typedef void(*CUSTOM_INITIALIZER_T)(void);

/**
 * @brief the data structure of OTA_BEGIN command.
 * 
 * common used information for OTA_BEGIN command in OTA common layer
 * the param 'customize' is used to pass the user-specific info which
 * will be handled in the function registored with
 * @see ota_common_registor_command_handler.
 * 
 */
typedef struct
{
    OTA_PATH_E path; //!< used to mark the OTA path
    OTA_USER_E user; //!< used to mark the OTA user
    OTA_DEVICE_E device; //!< used to mark the OTA device
    CUSTOM_INITIALIZER_T initializer; //!< used to init the custom OTA context
    uint32_t imageSize; //!< total size of the upgrade image
    uint32_t flashOffset; //!< flash offset to write the new image
    uint32_t startOffset; //!< offset in upgrade image to start update
    char version[MAX_VERSION_LEN]; //!< string of version to update
    uint8_t versionLen; //!< length of the version string
    void *customize; //!< customized information pointer
    uint32_t customizeLen; //!< customized information length
} OTA_BEGIN_PARAM_T;

/**
 * @brief the data structure of OTA_DATA command.
 * 
 * common used information for OTA_DATA command in OTA common layer
 * the param 'customize' is used to pass the user-specific info which
 * will be handled in the function registored with
 * @see ota_common_registor_command_handler.
 * 
 */
typedef struct
{
    uint32_t offset; //!< offset in the whole upgrade image of this packet of data
    uint16_t len; //!< length of upgrade data
    const uint8_t *data; //!< pointer of upgrade data
    uint32_t customizeLen; //!< customized information length
    void *customize; //!< customized information pointer
} OTA_DATA_PARAM_T;

/**
 * @brief the data structure of OTA_APPLY command.
 * 
 * common used information for OTA_APPLY command in OTA common layer
 * the param 'customize' is used to pass the user-specific info which
 * will be handled in the function registored with
 * @see ota_common_registor_command_handler.
 * 
 */
typedef struct
{
    bool applyNow; //!< mark if force apply the upgrade image
    uint32_t customizeLen; //!< customized information length
    void *customize; //!< customized information pointer
} OTA_APPLY_PARAM_T;

/**
 * @brief the data structure of OTA_ABORT command.
 * 
 * common used information for OTA_ABORT command in OTA common layer
 * the param 'customize' is used to pass the user-specific info which
 * will be handled in the function registored with
 * @see ota_common_registor_command_handler.
 * 
 */
typedef struct
{
    uint32_t customizeLen; //!< customized information length
    void *customize; //!< customized information pointer
} OTA_ABORT_PARAM_T;

/**
 * @brief the data structure of OTA_RESPONSE command.
 * 
 * common used information for OTA_RESPONSE command in OTA common layer
 * 
 */
typedef struct
{
    OTA_STATUS_E status; //!< status of received data
}OTA_RESPONSE_PARAM_T;


#ifdef OTA_NVRAM
typedef struct
{
    uint32_t lengthOfFollowingData; //!< 
    uint32_t newImageOffsetInFlash; //!< the offset of the flash to start writing the image

    uint32_t clearUserData  : 1; //!< flag to mark if need to clear the user data section
    uint32_t updateBTName   : 1; //!< flag to mark if need to update the BT name
    uint32_t updateLEName   : 1; //!< flag to mark if need to update the BLE name
    uint32_t updateBTAddr   : 1; //!< flag to mark if need to update the BT address
    uint32_t updateBLEAddr  : 1; //!< flag to mark if need to update the BLE address
    uint32_t reserve        : 27; //!< reserved for future use

    uint8_t  newBTName[NAME_LENGTH]; //!< BT name to update
    uint8_t  newLEName[NAME_LENGTH]; //!< BLE name to update
    uint8_t  newBTAddr[BD_ADDR_LENGTH]; //!< BT address to update
    uint8_t  newLEAddr[BD_ADDR_LENGTH]; //!< BLE address to update

    uint32_t crc; //!< CRC of data in this structure(except for crc itself)
}__attribute__ ((__packed__))OTA_FLOW_CONFIGURATION_T;
#endif

#ifdef IBRT
/**
 * @brief OTA relay packet type
 * 
 */
typedef enum
{
    OTA_RELAY_PACKET_TYPE_BEGIN = 0,
    OTA_RELAY_PACKET_TYPE_DATA  = 1,
    OTA_RELAY_PACKET_TYPE_APPLY = 2,
    OTA_RELAY_PACKET_TYPE_ABORT = 3,
    OTA_RELAY_PACKET_TYPE_RSP   = 4,

    OTA_RELAY_PACKET_TYPE_NUM,
} OTA_RELAY_PACKET_TYPE_E;

/**
 * @brief OTA relay packet data structure.
 * 
 */
typedef struct
{
    OTA_COMMAND_E cmdType;
    uint32_t magicCode;
    uint16_t length;
    uint8_t data[OTA_TWS_PAYLOAD_MAX_LEN];
} __attribute__((__packed__)) OTA_TWS_DATA_T;

/**
 * @brief Used for customer to determain if relay command/data needed.
 * 
 */
typedef bool(*CUSTOM_RELAY_NEEDED_FUNC_T)(void);

/**
 * @brief Used for peer command handling
 * 
 */
typedef OTA_STATUS_E (*PEER_CMD_RECEIVED_HANDLER_T)(OTA_COMMAND_E cmdType,
                                                    const uint8_t *data,
                                                    uint16_t len);
#endif

typedef OTA_STATUS_E(*OTA_CMD_HANDLER_T)(const void *cmd, uint16_t cmdLen);

typedef struct
{
    /// used to record the OTA command execution result
    /// NOTE: this could be used by customers
    OTA_STATUS_E status;

    /// used to mark if device currently in OTA state
    bool isInOtaState;
    
    /// used to cache OTA data
    uint8_t dataCacheBuffer[OTA_DATA_CACHE_BUFFER_SIZE];

    /// used to mark OTA stage, @see OTA_STAGE_E to get more details
    OTA_STAGE_E currentStage;

    /// current OTA user, @see OTA_USER_E to get more details
    OTA_USER_E currentUser;

    /// current device ID, @see OTA_DEVICE_E to get more details
    OTA_DEVICE_E deviceId;

    /// current OTA path, @see OTA_PATH_E to get more details
    OTA_PATH_E currentPath;

    /// sanity check enable flag
    /// 1 - sanity check enabled
    /// 2 - sanity check disabled
    bool sanityCheckEnable;

    /// total size of current OTA file
    uint32_t totalImageSize;

    /// crc32 value of whole OTA image
    uint32_t crc32OfImage;

    /// string of version info
    char version[MAX_VERSION_LEN];

    /// length of the version string
    uint8_t versionLen;

    /// received data length
    uint32_t receivedDataSize;

    /// break point of OTA progress
    uint32_t breakPoint;

    /// offset of data cached in cache-buffer
    uint32_t dataCacheBufferOffset;

    /// Flash Offset For New Image
    uint32_t newImageFlashOffset;

    /// offset of data programmed in new image flash section
    uint32_t newImageProgramOffset;

    /// offset in flash of user data section
    uint32_t userDataNvFlashOffset;

    /// flash offset of user data pool
    uint32_t flashOffsetOfUserDataPool;

    /// OTA user-specific command handlers, should registor this handler array
    /// with @see ota_common_registor_command_handler
    OTA_CMD_HANDLER_T cmdHandler[OTA_COMMAND_NUM + 1];

#ifdef OTA_NVRAM
    uint32_t flashOffsetOfFactoryDataPool;
    OTA_FLOW_CONFIGURATION_T configuration;
#endif

#ifdef IBRT
    /// tws relay data TX queue
    CQueue txQueue;

    /// tws relay data TX buffer
    uint8_t txBuf[OTA_MAX_MTU];

    /// tws relay data RX queue
    CQueue rxQueue;

    /// tws relay data RX buffer
    uint8_t rxBuf[OTA_MAX_MTU];

    /// used to retrieve data from RX queue
    uint8_t tempRxBuf[OTA_MAX_MTU];

    /// peer result of handling received relay data
    OTA_STATUS_E peerResult;

    /// current receiving magic code of relay frame
    /// used to mark if whole packet from APP is received
    uint32_t currentMagicCode;

    /// current receiving command type
    OTA_COMMAND_E currentCmdType;

    /// customized relay needed check handler
    CUSTOM_RELAY_NEEDED_FUNC_T customRelayNeededHandler;

    /// peer data received handler
    PEER_CMD_RECEIVED_HANDLER_T peerCmdReceivedHandler;
#endif
} OTA_COMMON_ENV_T;

/****************************function declearation**************************/
/**
 * @brief Initialize the common used OTA context.
 * 
 */
void ota_common_init_handler(void);

/**
 * @brief Enable/disable the sanity check.
 * 
 * @param enable        switch on/off
 */
void ota_common_enable_sanity_check(bool enable);

/**
 * @brief Init OTA used flash module.
 * 
 * @param module        flash module,
 *                      @see NORFLASH_API_MODULE_ID_T to get more details
 * @param baseAddr      base address of flash module
 * @param len           length of the flash module
 * @param imageHandler  handler
 */
void ota_common_init_flash(uint8_t module,
                           uint32_t baseAddr,
                           uint32_t len,
                           uint32_t imageHandler);

/**
 * @brief Get the pointer of otaEnv.
 * 
 * This function is used to pass the pointer of otaEnv to up-layer OTA applications
 * to use the common OTA info.
 * 
 * @return OTA_COMMON_ENV_T* pointer of the otaEnv.
 */
OTA_COMMON_ENV_T* ota_common_get_env(void);

/**
 * @brief Judge if currently OTA is in progress.
 * 
 * This function is used to judge if OTA progress is ongoing or not.
 * NOTE: this function will return true if current OTA stage is not OTA_STAGE_IDLE
 * 
 * @return true         OTA in progress
 * @return false        OTA not in progress
 */
bool ota_common_is_in_progress(void);

/**
 * @brief Registor customized OTA command handler.
 * 
 * Registor OTA uer-specific OTA command handlers, these functions are used to
 * mainten the status of specific OTA user layer and handle the user-specific
 * configurations.
 * 
 * @param cmdType       command type received,
 *                      @see OTA_COMMAND_E to get more details
 * @param cmdHandler    handler to registor,
 *                      must in type @see OTA_CMD_HANDLER_T
 */
void ota_common_registor_command_handler(OTA_COMMAND_E cmdType,
                                         void *cmdHandler);

/**
 * @brief Handler of received OTA command.
 * 
 * Handle the received OTA command and informations.
 * 
 * @param cmdType       receive command type
 * @param cmdInfo       pointer of received command information
 * @param cmdLen        length of received command information
 * @return OTA_STATUS_E result of handling received OTA command
 */
OTA_STATUS_E ota_common_command_received_handler(OTA_COMMAND_E cmdType,
                                                 void *cmdInfo,
                                                 uint16_t cmdLen);

#ifdef IBRT
/**
 * @brief Registor customized relay needed handler.
 * 
 * Relay needed is set to true by default, so the customized relay needed check
 * handler only need to return false when relay is not needed. And the customized
 * check function should in type of @see CUSTOM_RELAY_NEEDED_FUNC_T
 * 
 * @param handler       customized function handler
 * 
 */
void ota_common_registor_relay_needed_handler(void *handler);

/**
 * @brief Registor customized peer command recevied handler.
 * 
 * @param handler       customized function handler for received peer command
 */
void ota_common_registor_peer_cmd_received_handler(void *handler);

/**
 * @brief Relay data to peer handler.
 * 
 * @param cmdType       OTA command to relay, @see OTA_COMMAND_E to get more info
 * @param data          pointer of OTA data to relay
 * @param len           length of OTA data to realy
 * @return OTA_STATUS_E result of the OTA data relay operation
 */
OTA_STATUS_E ota_common_relay_data_to_peer(OTA_COMMAND_E cmdType,
                                           const uint8_t *data,
                                           uint16_t len);

/**
 * @brief Receive the OTA relay response.
 * 
 * NOTE: This function will block the current thread until response from peer
 * received or timeout.
 * 
 * @return OTA_STATUS_E OTA relay receive result.
 */
OTA_STATUS_E ota_common_receive_peer_rsp(void);

/**
 * @brief Write the received firmware data into flash.
 * 
 * @param data          pointer of received firmware data
 * @param len           lenght of received firmware data
 * @return OTA_STATUS_E Operation excution result
 */
OTA_STATUS_E ota_common_fw_data_write(const uint8_t *data, uint16_t len);

/**
 * @brief Apply current firmware handler.
 * 
 */
void ota_common_apply_current_fw(void);

/**
 * @brief Get the result of peer response to tws relay data.
 * 
 * @return OTA_STATUS_E response of peer to the relay data,
 *                      @see OTA_STATUS_E to get more details
 */
OTA_STATUS_E ota_common_get_peer_result(void);

/**
 * @brief OTA realy data received handler.
 * 
 * This function is used to handle received tws relay OTA data, transmit could be:
 * 1. master->slave: relay OTA commands
 * 2. slave->master: response to master
 * 
 * @param ptrParam      pointer of received data
 * @param paramLen      length of received data
 */
void ota_common_on_relay_data_received(uint8_t *ptrParam, uint32_t paramLen);

/**
 * @brief OTA sync tws info initializer.
 * 
 */
void ota_common_tws_sync_init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OTA_COMMON_H__ */