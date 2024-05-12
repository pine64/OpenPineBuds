#ifndef _OTA_CONTROL_H_
#define _OTA_CONTROL_H_

#include "co_bt_defines.h"

#define LOG_TAG "[OTA_CONTROL] "

#define OTA_BES_CONTROL_DEBUG

#ifdef OTA_BES_CONTROL_DEBUG
#define LOG_DBG(num,str,...)   TRACE(num,LOG_TAG""str, ##__VA_ARGS__)             // DEBUG OUTPUT
#define LOG_MSG(num,str,...)   TRACE(num,LOG_TAG""str, ##__VA_ARGS__)             // MESSAGE OUTPUT
#define LOG_ERR(num,str,...)   TRACE(num,LOG_TAG"err:"""str, ##__VA_ARGS__)       // ERROR OUTPUT

#define LOG_FUNC_LINE()    TRACE(2,LOG_TAG"%s:%d\n", __FUNCTION__, __LINE__)
#define LOG_FUNC_IN()      TRACE(1,LOG_TAG"%s ++++\n", __FUNCTION__)
#define LOG_FUNC_OUT()     TRACE(1,LOG_TAG"%s ----\n", __FUNCTION__)

#define LOG_DUMP           DUMP8
#else
#define LOG_DBG(num,str,...)
#define LOG_MSG(num,str,...)   TRACE(num,LOG_TAG""str, ##__VA_ARGS__)
#define LOG_ERR(num,str,...)   TRACE(num,LOG_TAG"err:"""str, ##__VA_ARGS__)

#define LOG_FUNC_LINE()
#define LOG_FUNC_IN()
#define LOG_FUNC_OUT()

#define LOG_DUMP
#endif

#define ERASE_LEN_UNIT          4096

#define OTA_SW_VERSION          1
#define OTA_HW_VERSION          1

#define DATA_PATH_BLE           1
#define DATA_PATH_SPP           2

#ifndef NORMAL_BOOT
#define NORMAL_BOOT             0xBE57EC1C
#endif
#ifndef COPY_NEW_IMAGE
#define COPY_NEW_IMAGE          0x5a5a5a5a
#endif

#define OTA_TWS_INFO_SIZE 128

#define OTA_BUF_SIZE            1024

#define OTA_WATCH_DOG_PING_TIMER_ID 0

#ifndef NEW_IMAGE_FLASH_OFFSET
#define NEW_IMAGE_FLASH_OFFSET    0x180000
#endif

#define OTA_FLASH_LOGIC_ADDR        (FLASH_NC_BASE)

#define DATA_ACK_FOR_SPP_DATAPATH_ENABLED 1
#define PUYA_FLASH_ERASE_LIMIT 0

#define OTA_RS_INFO_MASTER_SEND_RS_REQ_CMD   1
#define OTA_RS_INFO_MASTER_DISCONECT_CMD     2
#define OTAUPLOG_HEADSIZE (sizeof(otaUpgradeLog.randomCode) + sizeof(otaUpgradeLog.totalImageSize) + sizeof(otaUpgradeLog.crc32OfImage))
#define FLASH_SECTOR_SIZE_IN_BYTES          4096
#define OTA_OFFSET                          0x1000
#define OTA_INFO_IN_OTA_BOOT_SEC            (FLASHX_BASE+OTA_OFFSET)
#define OTA_DATA_BUFFER_SIZE_FOR_BURNING    FLASH_SECTOR_SIZE_IN_BYTES

#define BES_OTA_START_MAGIC_CODE            0x54534542  // BEST

#define BES_OTA_NAME_LENGTH                 32
#define BES_OTA_BLE_DATA_PACKET_MAX_SIZE    512
#define BES_OTA_BT_DATA_PACKET_MAX_SIZE     L2CAP_MTU

#define IMAGE_RECV_FLASH_CHECK              1  // It's best to turn it on durning development and not a big deal off in the release.

#define MAX_IMAGE_SIZE                      ((uint32_t)(NEW_IMAGE_FLASH_OFFSET - __APP_IMAGE_FLASH_OFFSET__))

#define MIN_SEG_ALIGN                       256
typedef void (*ota_transmit_data_t)(uint8_t* ptrData, uint32_t dataLen);
typedef void (*ota_transmission_done_t)(void);

enum
{
    OTA_RESULT_ERR_RECV_SIZE = 2,
    OTA_RESULT_ERR_FLASH_OFFSET,
    OTA_RESULT_ERR_SEG_VERIFY,
    OTA_RESULT_ERR_BREAKPOINT,
    OTA_RESULT_ERR_IMAGE_SIZE,
};

/**
 * @brief The format of the start OTA control packet
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_COMMAND_START
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
    uint32_t    imageSize;      // total image size
    uint32_t    crc32OfImage;   // crc32 of the whole image
} __attribute__ ((__packed__)) OTA_CONTROL_START_T;

/**
 * @brief The format of the start OTA response packet
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_RSP_START
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
    uint16_t    swVersion;
    uint16_t    hwVersion;
    uint16_t    MTU;            // MTU exchange result, central will send images in the unit of MTU
} __attribute__ ((__packed__)) OTA_START_RSP_T;

typedef struct
{
    uint32_t    lengthOfFollowingData;
    uint32_t    startLocationToWriteImage;    // the offset of the flash to start writing the image
    uint32_t    isToClearUserData   : 1;
    uint32_t    isToRenameBT        : 1;
    uint32_t    isToRenameBLE       : 1;
    uint32_t    isToUpdateBTAddr    : 1;
    uint32_t    isToUpdateBLEAddr   : 1;
    uint32_t    reserve             : 27;
    uint8_t     newBTName[BES_OTA_NAME_LENGTH];
    uint8_t     newBLEName[BES_OTA_NAME_LENGTH];
    uint8_t     newBTAddr[BD_ADDR_LEN];
    uint8_t     newBLEAddr[BD_ADDR_LEN];
    uint32_t    crcOfConfiguration; // CRC of data from lengthOfFollowingData to newBLEAddr
} __attribute__ ((__packed__)) OTA_FLOW_CONFIGURATION_T;

/**
 * @brief The format of the start OTA control packet
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_COMMAND_CONFIG_OTA
    OTA_FLOW_CONFIGURATION_T config;
} __attribute__ ((__packed__)) OTA_COMMAND_CONFIG_T;

/**
 * @brief The format of the OTA configuration response packet
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_RSP_CONFIG
    uint8_t     isConfigurationDone;    // 1 if the configuration has been done successfully, otherwise, 0
} __attribute__ ((__packed__)) OTA_RSP_CONFIG_T;


/**
 * @brief The format of the segment verification request
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_COMMAND_SEGMENT_VERIFY
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
    uint32_t    crc32OfSegment;          // crc32 of the 1% segment
} __attribute__ ((__packed__)) OTA_CONTROL_SEGMENT_VERIFY_T;

/**
 * @brief The format of the segment verification request
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_RSP_SEGMENT_VERIFY
    uint8_t     isVerificationPassed;
} __attribute__ ((__packed__)) OTA_RSP_SEGMENT_VERIFY_T;


/**
 * @brief The format of the OTA result response
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_RSP_RESULT
    uint8_t     isVerificationPassed;
} __attribute__ ((__packed__)) OTA_RSP_OTA_RESULT_T;

/**
 * @brief The format of the OTA reading flash content command
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_READ_FLASH_CONTENT
    uint8_t     isToStart;      // true to start, false to stop
    uint32_t    startAddr;
    uint32_t    lengthToRead;
} __attribute__ ((__packed__)) OTA_READ_FLASH_CONTENT_REQ_T;

/**
 * @brief The format of the OTA reading flash content command
 *
 */
typedef struct
{
    uint8_t     packetType;     // should be OTA_READ_FLASH_CONTENT
    uint8_t     isReadingReqHandledSuccessfully;
} __attribute__ ((__packed__)) OTA_READ_FLASH_CONTENT_RSP_T;

typedef struct
{
    uint8_t     packetType;     // should be OTA_CONTROL_RESUME_VERIFY
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
    uint8_t     randomCode[32];
    uint32_t    segmentSize;
    uint32_t    crc32;          // CRC32 of randomCode and segment size
} __attribute__ ((__packed__)) OTA_CONTROL_RESUME_VERIFY_T;

typedef struct
{
    uint8_t     packetType;     // should be OTA_RSP_RESUME_VERIFY
    uint32_t    breakPoint;
    uint8_t     randomCode[32];
    uint32_t    crc32;          // CRC32 of breakPoint and randomCode
} __attribute__ ((__packed__)) OTA_RSP_RESUME_VERIFY_T;

typedef struct
{
    uint8_t     packetType;     // should be
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
} __attribute__ ((__packed__)) OTA_GET_VERSION_REQ_T;

typedef struct
{
    uint8_t     packetType;     // should be
    uint32_t     magicCode;
    uint8_t    deviceType;
    uint8_t    leftVersion[4];    //or stereo version
    uint8_t    rightVersion[4];    //stereo not needed
} __attribute__ ((__packed__)) OTA_RSP_OTA_VERSION_T;

typedef struct
{
    uint8_t     packetType;     // should be
    uint8_t    success;
} __attribute__ ((__packed__)) OTA_RSP_SELECTION_T;


typedef struct
{
    uint8_t     packetType;     // should be
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
} __attribute__ ((__packed__)) OTA_FORCE_REBOOT_T;

typedef struct
{
    uint8_t     packetType;     // should be
    uint32_t    magicCode;      // should be BES_OTA_START_MAGIC_CODE
} __attribute__ ((__packed__)) OTA_IMAGE_APPLY_REQ_T;

typedef struct
{
    uint8_t     packetType;     // should be
       uint8_t    success;
} __attribute__ ((__packed__)) OTA_RSP_APPLY_T;

typedef struct
{
    uint8_t     packetType;
} __attribute__ ((__packed__)) OTA_START_ROLE_SWITCH_T;

typedef struct
{
    uint8_t     packetType;
} __attribute__ ((__packed__)) OTA_ROLE_SWITCH_COMPLETE_T;

/**
 * @brief The OTA handling data structure
 *
 */
typedef struct
{
    uint8_t*    dataBufferForBurning;
    uint32_t    dstFlashOffsetForNewImage;
    uint32_t    offsetInDataBufferForBurning;
    uint32_t    offsetInFlashToProgram;
    uint32_t    totalImageSize;
    uint32_t    alreadyReceivedDataSizeOfImage;
    uint32_t    offsetInFlashOfCurrentSegment;
    uint32_t    offsetOfImageOfCurrentSegment;
    uint32_t    crc32OfImage;
    uint32_t    crc32OfSegment;
    uint8_t     isOTAInProgress;
    uint8_t     isPendingForReboot;
    uint32_t    flasehOffsetOfUserDataPool;
    uint32_t    flasehOffsetOfFactoryDataPool;

    // configuration of the OTA
    OTA_FLOW_CONFIGURATION_T    configuration;
    uint32_t    AlreadyReceivedConfigurationLength;

    uint16_t     dataPacketSize;
    ota_transmit_data_t  transmitHander;
    uint32_t    offsetInFlashToRead;
    uint32_t    leftSizeOfFlashContentToRead;

    uint8_t     dataPathType;

    bool        resume_at_breakpoint;
    uint32_t    breakPoint;
    uint32_t    i_log;
} OTA_CONTROL_ENV_T;

typedef struct
{
    uint32_t magicNumber;    // NORMAL_BOOT or COPY_NEW_IMAGE
    uint32_t imageSize;
    uint32_t imageCrc;
} FLASH_OTA_BOOT_INFO_T;

typedef struct
{
    uint8_t     randomCode[32];
    uint32_t    totalImageSize;
    uint32_t    crc32OfImage;
    uint32_t    upgradeSize[(ERASE_LEN_UNIT - 32 - 2*sizeof(uint32_t)) / 4];
}FLASH_OTA_UPGRADE_LOG_FLASH_T;

/**
 * @brief The control packet type
 *
 */
typedef enum
{
    OTA_COMMAND_START = 0x80,   // from central to device, to let OTA start
    OTA_RSP_START,              // from device to centrl, to let it know that it has been ready for OTA
    OTA_COMMAND_SEGMENT_VERIFY, // from central to device, sent per 1% of image, inform device to do CRC check on those 1%
    OTA_RSP_SEGMENT_VERIFY,
    OTA_RSP_RESULT,
    OTA_DATA_PACKET,
    OTA_COMMAND_CONFIG_OTA,     // from central to device, to configure the following OTA flow
    OTA_RSP_CONFIG,
    OTA_COMMAND_GET_OTA_RESULT,
    OTA_READ_FLASH_CONTENT,
    OTA_FLASH_CONTENT_DATA = 0x8A,
    OTA_DATA_ACK,
    OTA_COMMAND_RESUME_VERIFY,
    OTA_RSP_RESUME_VERIFY,
    OTA_COMMAND_GET_VERSION,
    OTA_RSP_VERSION,
    OTA_COMMAND_SIDE_SELECTION = 0x90,
    OTA_RSP_SIDE_SELECTION,
    OTA_COMMAND_IMAGE_APPLY,
    OTA_RSP_IMAGE_APPLY,
    OTA_RSP_START_ROLE_SWITCH = 0x95,
    OTA_RSP_ROLE_SWITCH_COMPLETE,
} OTA_CONTROL_PACKET_TYPE_E;

typedef struct
{
    uint8_t     typeCode;
    uint16_t    rsp_seq;
    uint16_t    length;
    uint8_t     p_buff[OTA_TWS_INFO_SIZE];
}OTA_IBRT_TWS_CMD_EXECUTED_RESULT_FROM_SLAVE_T;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Format of the data xfer handler function, to send data to central
 *
 * @param ptrData    Pointer of the data to send
 * @param dataLen     Length of the data to send
 *
 */

extern void app_ibrt_ota_segment_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_start_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_config_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_image_crc_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_get_version_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_select_side_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);
extern void app_ibrt_ota_image_overwrite_cmd_send_handler(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void ota_control_register_transmitter(ota_transmit_data_t transmit_handle);
void ota_control_update_MTU(uint16_t mtu);
void ota_update_info(void);
void ota_check_and_reboot_to_use_new_image(void);
void ota_control_reset_env(void);
void ota_execute_the_final_operation(void);
bool ota_is_in_progress(void);
void ota_control_set_datapath_type(uint8_t datapathType);
void ibrt_ota_send_start_response(bool isViaBle);
void ibrt_ota_send_configuration_response(bool isDone);
void ota_control_image_apply_rsp(uint8_t success);
void ibrt_ota_send_segment_verification_response(bool isPass);
void ibrt_ota_send_result_response(uint8_t isSuccessful);
void ota_randomCode_log(uint8_t randomCode[]);
void ota_control_send_resume_response(uint32_t breakPoint, uint8_t randomCode[]);
void ota_bes_handle_received_data(uint8_t *otaBuf, bool isViaBle,uint16_t dataLenth);
void ota_ibrt_handle_received_data(uint8_t *otaBuf, bool isViaBle, uint16_t len);
void ota_control_side_selection_rsp(uint8_t success);
uint8_t ota_control_get_datapath_type(void);
uint32_t app_get_magic_number(void);
void ota_flash_init(void);
void Bes_exit_ota_state(void);
void ibrt_ota_send_version_rsp(void);
void ota_upgradeLog_destroy(void);
void ota_status_change(bool status);
void ota_control_send_start_role_switch(void);
void ota_control_send_role_switch_complete(void);
uint8_t app_get_bes_ota_state(void);
bool app_check_is_ota_role_switch_initiator(void);
void app_set_ota_role_switch_initiator(bool is_initiate);
void bes_ota_send_role_switch_req(void);
void app_statistics_get_ota_pkt_in_role_switch(void);
bool app_check_user_can_role_switch_in_ota(void);


#ifdef __cplusplus
}
#endif

#endif
