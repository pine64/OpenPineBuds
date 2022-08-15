#ifndef APP_THROUGH_PUT_H_
#define APP_THROUGH_PUT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_spp.h"

#define APP_THROUGHPUT_CMD_INSTANCE_NUMBER            4

#define THROUGHPUT_BLE_CONNECTION_INTERVAL_MIN_IN_MS            50
#define THROUGHPUT_BLE_CONNECTION_INTERVAL_MAX_IN_MS            60
#define THROUGHPUT_BLE_CONNECTION_SUPERVISOR_TIMEOUT_IN_MS      20000
#define THROUGHPUT_BLE_CONNECTION_SLAVELATENCY                  0

typedef enum
{
    PATTERN_RANDOM = 0,
    PATTERN_11110000,
    PATTERN_10101010,
    PATTERN_ALL_1,
    PATTERN_ALL_0,
    PATTERN_00001111,
    PATTERN_0101,
} THROUGHPUT_TEST_DATA_PATTER_E;

typedef enum
{
    UP_STREAM = 0,
    DOWN_STREAM
} THROUGHPUT_TEST_DATA_DIRECTION_E;

typedef enum
{
    WITHOUT_RSP = 0,
    WITH_RSP
} THROUGHPUT_TEST_RESPONSE_MODEL_E;


typedef struct
{
    bool        isThroughputTestOn;
    uint8_t     conidx;
} THROUGHPUT_TEST_ENV_T;

typedef struct
{
    uint8_t     dataPattern;
    uint16_t    lastTimeInSecond;
    uint16_t    dataPacketSize;
    uint8_t     direction;
    uint8_t     responseModel;
    uint8_t     isToUseSpecificConnParameter;
    uint16_t    minConnIntervalInMs;
    uint16_t    maxConnIntervalInMs;
    uint8_t     reserve[4];
} __attribute__((__packed__)) THROUGHPUT_TEST_CONFIG_T;

/**
 * @brief The command code
 *
 */
typedef enum
{
#ifdef __GMA_VOICE__
    /* 9 */  THROUGHPUT_OP_INFORM_THROUGHPUT_TEST_CONFIG = 0x89,
    /* 10 */ THROUGHPUT_OP_THROUGHPUT_TEST_DATA = 0x8A,
    /* 11 */ THROUGHPUT_OP_THROUGHPUT_TEST_DATA_ACK = 0x8B,
    /* 12 */ THROUGHPUT_OP_THROUGHPUT_TEST_DONE = 0x8C,
#else
    /* 9 */  THROUGHPUT_OP_INFORM_THROUGHPUT_TEST_CONFIG = 0x8009,
    /* 10 */ THROUGHPUT_OP_THROUGHPUT_TEST_DATA = 0x800A,
    /* 11 */ THROUGHPUT_OP_THROUGHPUT_TEST_DATA_ACK = 0x800B,
    /* 12 */ THROUGHPUT_OP_THROUGHPUT_TEST_DONE = 0x800C,
#endif
} APP_THROUGHPUT_CMD_CODE_E;

/**
 * @brief custom command playload
 *
 */
#define THROUGHPUT_CMD_PAYLOAD_HEADER_LEN (2*sizeof(uint16_t))
 #define THROUGHPUT_DATA_MAX_SIZE (L2CAP_MTU - THROUGHPUT_CMD_PAYLOAD_HEADER_LEN)
typedef struct
{
    uint16_t    cmdCode;        /**< command code, from APP_SV_CMD_CODE_E */
    uint16_t    paramLen;       /**< length of the following parameter */
    uint8_t     param[THROUGHPUT_DATA_MAX_SIZE];
} APP_THROUGHPUT_CMD_PAYLOAD_T;

/**
 * @brief through put command definition data structure
 *
 */
typedef void (*app_through_cmd_handler_t)(APP_THROUGHPUT_CMD_CODE_E cmdCode, uint8_t* ptrParam, uint32_t paramLen);
typedef struct
{
    uint32_t                cmdCode;
    app_through_cmd_handler_t    cmdHandler;             /**< command handler function */
} APP_THROUGHPUT_CMD_INSTANCE_T;
extern APP_THROUGHPUT_CMD_INSTANCE_T through_put_table[];

#ifdef __cplusplus
extern "C" {
#endif


/*---------------------------------------------------------------------------
 *            app_throughput_cmd_send_done
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    thourghput cmd send done callback
 *
 * Parameters:
 *    param1 -- callback data 
 *    param2 -- callback data length
 *
 * Return:
 *    void
 */
uint32_t app_throughput_cmd_send_done(void *param1, uint32_t param2);

/*---------------------------------------------------------------------------
 *            app_throughput_receive_cmd
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    thourghput receive and handle cmd
 *
 * Parameters:
 *    param1 -- receive data 
 *    param2 -- receive data length
 *
 * Return:
 *    void
 */
uint32_t app_throughput_receive_cmd(void *param1, uint32_t param2);

/*---------------------------------------------------------------------------
 *            app_stop_throughput_test
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    stop throughput test
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_stop_throughput_test(void);

/*---------------------------------------------------------------------------
 *            app_throughput_test_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init throughput test
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_throughput_test_init(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_THROUGH_PUT_H_

