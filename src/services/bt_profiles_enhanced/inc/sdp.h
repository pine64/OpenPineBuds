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
#ifndef __SDP_H__
#define __SDP_H__

#include "btlib_more.h"
#include "l2cap_i.h"
#include "bt_co_list.h"
#include "data_link.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDP_PRIVATE_RECORD_LIST 1
#define SDP_INVALID_HANDLE 0xFFFFFFFF

/*
    @brief PSM defination
*/
#define SDP_PSM_SDP        PSM_SDP
#define SDP_PSM_RFCOMM     PSM_RFCOMM
#define SDP_PSM_AVCTP      PSM_AVCTP
#define SDP_PSM_AVDTP      PSM_AVDTP

/*
    @brief SDP data element type_size_table defination
    bit0-5: len
    bit6: 1 -- type_length is value
    bit7: 0 -- len represent the size of data; 1 -- len represent the data size is contained
            in the additional len bytes
*/
#define SDP_DATA_LENGTH_MASK        0x3F
#define SDP_TYPE_SIZE_VALUE         0x40
#define SDP_DATA_CONTAINED_IN_BYTE  0x80

/*
    @brief PDU defination
*/
#define SDP_PDU_ID_OFFSET        0
#define SDP_TRANS_ID_OFFSET      1
#define SDP_PARAM_LEN_OFFSET     3
#define SDP_PDU_LEN              5

/*
    @brief sdp_service_search_request defination
        ServiceSearchPattern        -- varies bytes
        MaximumServiceRecordCount   -- 2 bytes
        ContinuationState           -- 1-17 bytes
*/
#define SDP_SERVICE_SEARCH_MIN_SIZE  8
#define SDP_MAX_SER_RECORD_COUNT_LEN 2

/*
    @brief sdp_service_attribute_request defination
        ServiceRecordHandle         -- 4 bytes
        MaximumAttributeByteCount   -- 2 bytes
        AttributeIDList             -- varies bytes
        ContinuationState           -- 1-17 bytes
*/
#define SDP_SERVICE_ATTR_MIN_SIZE        12
#define SDP_SER_RECORD_HANDLE_OFFSET     0
#define SDP_SER_RECORD_HANDLE_LEN        4
#define SDP_MAX_ATTR_BYTE_COUNT_OFFSET   4
#define SDP_MAX_ATTR_BYTE_COUNT_LEN      2
#define SDP_SER_ATTR_REQ_ID_LIST_OFFSET  6

/*
    @brief sdp_service_search_attribute_request defination
        ServiceSearchPattern        -- varies bytes
        MaximumAttributeByteCount   -- 2 bytes
        AttributeIDList             -- varies bytes
        ContinuationState           -- 1-17 bytes
*/
#define SDP_SERVICE_SEARCH_ATTR_MIN_SIZE    13
//#define SDP_MAX_ATTR_BYTE_COUNT_LEN  2

/*
    @brief Data element type defination
    Refer to Table3.1 in Core_v5.0.pdf
*/
#define DE_TYPE_MASK 0x1F
#define DE_TYPE_OFFSET 3
enum data_element_type {
    DE_TYPE_NIL = 0,
    DE_TYPE_UINT = 1,
    DE_TYPE_S2CMPLINT = 2,
    DE_TYPE_UUID = 3,
    DE_TYPE_TEXTSTR = 4,
    DE_TYPE_BOOL = 5,
    DE_TYPE_DESEQ = 6,
    DE_TYPE_DEALT = 7,
    DE_TYPE_URL = 8,
};

/*
    @brief Data element size index defination
    Refer to Table3.2 in Core_v5.0.pdf
*/
#define DE_SIZE_MASK    0x07
enum data_element_size {
    DE_SIZE_0 = 0,
    DE_SIZE_1 = 1,
    DE_SIZE_2 = 2,
    DE_SIZE_3 = 3,
    DE_SIZE_4 = 4,
    DE_SIZE_5 = 5,
    DE_SIZE_6 = 6,
    DE_SIZE_7 = 7,
};

/*
    @brief SDP pdu id
    Refer to 4.2 in Core_v5.0.pdf
*/
enum sdp_pdu_id {
    SDP_PDU_ErrorResponse = 0x01,
    SDP_PDU_ServiceSearchRequest = 0x02,
    SDP_PDU_ServiceSearchResponse = 0x03,
    SDP_PDU_ServiceAttributeRequest = 0x04,
    SDP_PDU_ServiceAttributeResponse = 0x05,
    SDP_PDU_ServiceSearchAttributeRequest = 0x06,
    SDP_PDU_ServiceSearchAttributeResponse = 0x07,
};

/*
    @brief SDP Error Code
    Refer to 4.4.1 SDP_ErrorResponse PDU
*/
enum sdp_error_code {
    SDP_Reserved_for_future_use = 0x0000,
    SDP_Invalid_SDP_version = 0x0001,
    SDP_Invalid_Service_Record_Handle = 0x0002,
    SDP_Invalid_request_syntax = 0x0003,
    SDP_Invalid_PDU_Size = 0x0004,
    SDP_Invalid_Continuation_State = 0x0005,
    SDP_Insufficient_Resources_to_satisfy_Request = 0x0006,
};

/*
    @brief SDP Attribute ID Type
*/
#define ATTR_ID_TYPE_RECORED_HANDLE \
    0x00,0x00

/*
    @brief SDP callback event
*/
enum sdp_event {
    SDP_EVT_TX_HANDLED = 0,
    SDP_EVT_RESPONSE_ERROR,
    SDP_EVT_CHANNEL_CLOSE,
    SDP_EVT_TX_TIMEOUT,
    SDP_EVT_RESPONSE,
    SDP_EVT_UNKNOWN_ERROR,
};

/* 
    @brief SDP callback params
*/
struct sdp_callback_params {
    enum sdp_error_code error_code;
    struct sdp_control_t *sdp_ctl;
    struct {
        uint8 *data;
        uint32 data_len;
        enum sdp_pdu_id response_pdu_id;
        union {
            struct {
                uint8 *service_record_handle_list_buff;
                uint32 service_record_handle_list_buff_len;
                uint16 total_service_record_count;
                uint16 current_service_record_count;
                uint8 cont_state_len;
            }service_search_response;
            struct {
                uint8 *attribute_list_buff;
                uint32 attribute_list_buff_len;
                uint16 attribute_list_bytes_count;
                uint8 cont_state_len;
            }service_search_attr_response;
        };
    } response;
};

/* 
    @brief SDP connection role
*/
enum sdp_connection_role {
    SDP_ROLE_CLIENT = 0,
    SDP_ROLE_SERVER,
};

/*
    @brief SDP connection state 
*/
enum sdp_connection_state {
    SDP_ST_STANDBY = 0,
    SDP_ST_CONNECTING,
    SDP_ST_CONNECTED,
    SDP_ST_CLT_QUERING,
    SDP_ST_DISCONNETING,
};

/*
    @brief SDP tx state 
*/
enum sdp_tx_state {
    SDP_TX_IDLE = 0,
    SDP_TX_BUSY,
};

/*
    @brief SDP ctl state
*/
enum sdp_ctl_state {
    SDP_CTL_FREE,
    SDP_CTL_INUSE
};

/*
    @brief SDP server record attribute
*/
struct sdp_server_record_attr {
    uint16 attr_id;
    uint16 data_len;
    const uint8 *data;
    uint16 resv;
} __attribute__ ((__packed__));

/*
    @brief SDP server record
*/
struct sdp_server_record {
    struct list_node node;
    uint32 record_handle;
    uint32 attr_list_count;
    struct sdp_server_record_attr *attr_list;
};

/*
    @brief SDP Request Callback
*/
typedef void (*T_sdp_request_callback)(enum sdp_event event, struct sdp_callback_params *params, void *priv);

/*
    @brief SDP Request
*/
struct sdp_request {
    struct list_node node;
    void *priv;
    uint32 request_len;
    uint8 *request_data;
    struct bdaddr_t remote;
    enum sdp_pdu_id type;
    T_sdp_request_callback callback;
};

#define SDP_RESPONSE_SIZE (1024)
#define SDP_RESPONSE_HANDLES_SIZE (14)

#define SDP_PACKET_HEADER_LEN (5)
#define SDP_PACKET_CONT_FIELD_LEN (1 + sizeof(SDP_CONT_STATE))
#define SDP_PACKET_MAX_RSP_SDU_SIZE (128)

/*
    @brief SDP control struct
*/
struct sdp_control_t
{
    struct list_node node;
    enum sdp_connection_state conn_state;
    enum sdp_ctl_state ctl_state;
    enum sdp_connection_role role;
    enum sdp_tx_state tx_state;
    uint32 l2cap_handle;
    struct bdaddr_t remote;

    // for server
    uint32 max_attr_bytes_count;
    uint16 response_trans_id;
#if defined(SDP_PRIVATE_RECORD_LIST)
    struct list_node record_list; 
#endif
    uint16 refer_count;
    uint8 response_pdu_header[SDP_PACKET_HEADER_LEN];
    uint32 response_handle_buff[SDP_RESPONSE_HANDLES_SIZE];
    uint8 response_handle_count;
    uint8 response_handle_offset;
    uint8 response_buff[SDP_RESPONSE_SIZE];
    uint32 response_buff_offset;
    uint32 response_buff_len;
    uint8 response_cont_data_len;
    uint8 response_cont_data[16+1]; // 1 byte more to store cont data len
    struct data_link response_data_list;
    struct data_link response_head;
    struct data_link response_data;
    struct data_link response_cont;    

    // for client
    struct list_node request_list; 
    struct sdp_request *curr_request; 
    struct sdp_request *pending_request; 

    uint16 request_trans_id;
    uint8 request_cont_data_len;
    uint8 request_cont_data[16+1]; // 1 byte more to store cont data len
    uint8 request_pdu_header[5];
    struct data_link request_data_list;
    struct data_link request_head;
    struct data_link request_data;
    struct data_link request_cont;
};


/*
    @brief Service Attribute ID
*/
#define SERV_ATTRID_SERVICE_RECORD_HANDLE               0x0000
#define SERV_ATTRID_SERVICE_CLASS_ID_LIST               0x0001
#define SERV_ATTRID_SERVICE_RECORD_STATE                0x0002
#define SERV_ATTRID_SERVICE_ID                          0x0003
#define SERV_ATTRID_PROTOCOL_DESC_LIST                  0x0004
#define SERV_ATTRID_BROWSE_GROUP_LIST                   0x0005
#define SERV_ATTRID_LANG_BASE_ID_LIST                   0x0006
#define SERV_ATTRID_SERVICE_INFO_TIME_TO_LIVE           0x0007
#define SERV_ATTRID_SERVICE_AVAILABILITY                0x0008
#define SERV_ATTRID_BT_PROFILE_DESC_LIST                0x0009
#define SERV_ATTRID_DOC_URL                             0x000a
#define SERV_ATTRID_CLIENT_EXEC_URL                     0x000b
#define SERV_ATTRID_ICON_URL                            0x000c
#define SERV_ATTRID_ADDITIONAL_PROT_DESC_LISTS          0x000d
#define SERV_ATTRID_SERVICE_NAME                        0X0100
#define SERV_ATTRID_PROVIDER_NAME                       0X0102
#define SERV_ATTRID_SDP_VERSION_NUMBER_LIST             0x0200
#define SERV_ATTRID_SUPPORTED_FEATURES                  0x0311

/*
    @brief UUID
*/
#define SERV_UUID_GENERIC_AUDIO            0x12,0x03
#define SERV_UUID_HandsFree                0x11,0x1E
#define SERV_UUID_HandsFreeAudioGateway    0x11,0x1F
#define SERV_UUID_SDP                      0x00,0x01
#define SERV_UUID_UDP                      0x00,0x02
#define SERV_UUID_RFCOMM                   0x00,0x03
#define SERV_UUID_TCP                      0x00,0x04
#define SERV_UUID_TCS_BIN                  0x00,0x05
#define SERV_UUID_TCS_AT                   0x00,0x06
#define SERV_UUID_ATT                      0x00,0x07
#define SERV_UUID_OBEX                     0x00,0x08
#define SERV_UUID_IP                       0x00,0x09
#define SERV_UUID_FTP                      0x00,0x0A
#define SERV_UUID_HTTP                     0x00,0x0C
#define SERV_UUID_WSP                      0x00,0x0E
#define SERV_UUID_BNEP                     0x00,0x0F
#define SERV_UUID_UPNP                     0x12,0x00
#define SERV_UUID_HID_PROTOCOL             0x00,0x11
#define SERV_UUID_HID_CTRL                 0x00,0x11
#define SERV_UUID_HID_INTR                 0x00,0x13
#define SERV_UUID_HardcopyControlChannel   0x00,0x12
#define SERV_UUID_HardcopyDataChannel      0x00,0x14
#define SERV_UUID_HardcopyNotification     0x00,0x16
#define SERV_UUID_AVCTP                    0x00,0x17
#define SERV_UUID_AVDTP                    0x00,0x19
#define SERV_UUID_CMTP                     0x00,0x1B
#define SERV_UUID_MCAPControlChannel       0x00,0x1E
#define SERV_UUID_MCAPDataChannel          0x00,0x1F
#define SERV_UUID_L2CAP                    0x01,0x00
#define SERV_UUID_SPP                      0x11,0x01
#define SERV_UUID_AUDIOSOURCE              0x11,0x0A
#define SERV_UUID_AUDIOSINK                0x11,0x0B
#define SERV_UUID_HID                      0x11,0x24
#define SERV_UUID_MAP                      0x11,0x34
#define SERV_UUID_AdvancedAudioDistribution                0x11,0x0D
#define SERV_UUID_AV_REMOTE_CONTROL        0x11,0x0E
#define SERV_UUID_AV_REMOTE_CONTROL_TARGET 0x11,0x0C
#define SERV_UUID_AV_REMOTE_CONTROL_CONTROLLER 0x11,0x0F

/* 
    @brief Service Record defination helper macros
*/
#define _U8VALUE(v) ((v)&0xFF)

#define SDP_SPLIT_16BITS_BE(v) \
    _U8VALUE(v>>8),_U8VALUE(v)
#define SDP_SPLIT_32BITS_BE(v) \
    _U8VALUE(v>>24),_U8VALUE(v>>16),_U8VALUE(v>>8),_U8VALUE(v)

#define X_SDP_COMBINE_16BITS_BE(a,b) \
    (_U8VALUE(a)<<8) | _U8VALUE(b)
#define SDP_COMBINE_16BITS_BE(...) \
    X_SDP_COMBINE_16BITS_BE(__VA_ARGS__)

/*
    @brief NIL
*/
#define SDP_DE_NIL_H1_D0 \
    DE_TYPE_NIL<<3

/*
    @brief Unsigned Integer
*/
#define SDP_DE_UINT(size_index) \
    DE_TYPE_UINT<<3|size_index

#define SDP_DE_UINT_H1_D1 \
    SDP_DE_UINT(DE_SIZE_0)

#define SDP_DE_UINT_H1_D2 \
    SDP_DE_UINT(DE_SIZE_1)

#define SDP_DE_UINT_H1_D4 \
    SDP_DE_UINT(DE_SIZE_2)

#define SDP_DE_UINT_H1_D8 \
    SDP_DE_UINT(DE_SIZE_3)

#define SDP_DE_UINT_H1_D16 \
    SDP_DE_UINT(DE_SIZE_4)

/*
    @brief Signed twos-complement integer
*/
#define SDP_DE_S2CMPLINT(size_index) \
    DE_TYPE_S2CMPLINT<<3|size_index

#define SDP_DE_S2CMPLINT_H1_D1 \
    SDP_DE_S2CMPLINT(DE_SIZE_0)

#define SDP_DE_S2CMPLINT_H1_D2 \
    SDP_DE_S2CMPLINT(DE_SIZE_1)

#define SDP_DE_S2CMPLINT_H1_D4 \
    SDP_DE_S2CMPLINT(DE_SIZE_2)

#define SDP_DE_S2CMPLINT_H1_D8 \
    SDP_DE_S2CMPLINT(DE_SIZE_3)

#define SDP_DE_S2CMPLINT_H1_D16 \
    SDP_DE_S2CMPLINT(DE_SIZE_4)

/*
    @brief UUID
*/
#define SDP_DE_UUID(size_index) \
    DE_TYPE_UUID<<3|size_index

#define SDP_DE_UUID_H1_D2 \
    SDP_DE_UUID(DE_SIZE_1)

#define SDP_DE_UUID_H1_D4 \
    SDP_DE_UUID(DE_SIZE_2)

#define SDP_DE_UUID_H1_D16 \
    SDP_DE_UUID(DE_SIZE_4)

/*
    @brief Text string
*/
#define SDP_DE_TEXTSTR_8BITSIZE_H2_D(size) \
    DE_TYPE_TEXTSTR<<3|DE_SIZE_5,_U8VALUE(size)

#define SDP_DE_TEXTSTR_16BITSIZE_H3_D(size) \
    DE_TYPE_TEXTSTR<<3|DE_SIZE_6,_U8VALUE(size),_U8VALUE(size>>8)

#define SDP_DE_TEXTSTR_32BITSIZE_H5_D(size) \
    DE_TYPE_TEXTSTR<<3|DE_SIZE_7,_U8VALUE(size),U*VALUE(size>>8),_U8VALUE(size>>16),_U8VALUE(size>>24)

/*
    @brief Boolean
*/
#define SDP_DE_BOOL_H1_D1 \
    DE_TYPE_BOOL<<3|DE_SIZE_0

/*
    @brief Data element sequence
*/
#define SDP_DE_DESEQ_8BITSIZE_H2_D(size) \
    DE_TYPE_DESEQ<<3|DE_SIZE_5,_U8VALUE(size)

#define SDP_DE_DESEQ_16BITSIZE_H3_D(size) \
    DE_TYPE_DESEQ<<3|DE_SIZE_6,_U8VALUE(size),_U8VALUE(size>>8)

#define SDP_DE_DESEQ_32BITSIZE_H5_D(size) \
    DE_TYPE_DESEQ<<3|DE_SIZE_7,_U8VALUE(size),U*VALUE(size>>8),_U8VALUE(size>>16),_U8VALUE(size>>24)

/*
    @brief Data element alternative
*/
#define SDP_DE_DEALT_8BITSIZE_H2_D(size) \
    DE_TYPE_DEALT<<3|DE_SIZE_5,_U8VALUE(size)

#define SDP_DE_DEALT_16BITSIZE_H3_D(size) \
    DE_TYPE_DEALT<<3|DE_SIZE_6,_U8VALUE(size),_U8VALUE(size>>8)

#define SDP_DE_DEALT_32BITSIZE_H5_D(size) \
    DE_TYPE_DEALT<<3|DE_SIZE_7,_U8VALUE(size),_U8VALUE(size>>8),_U8VALUE(size>>16),_U8VALUE(size>>24)

/*
    @brief URL
*/
#define SDP_DE_URL_8BITSIZE_H2_D(size) \
    DE_TYPE_URL<<3|DE_SIZE_5,_U8VALUE(size)

#define SDP_DE_URL_16BITSIZE_H3_D(size) \
    DE_TYPE_URL<<3|DE_SIZE_6,_U8VALUE(size),_U8VALUE(size>>8)

#define SDP_DE_URL_32BITSIZE_H5_D(size) \
    DE_TYPE_URL<<3|DE_SIZE_7,_U8VALUE(size),_U8VALUE(size>>8),_U8VALUE(size>>16),_U8VALUE(size>>24)


/*
    @brief Define a attribute
*/
#define SDP_DEF_ATTRIBUTE(attr_id,attrs) \
    { \
        attr_id,sizeof(attrs),attrs,0x0, \
    }

/*
    @brief SDP init
    @param in_ctl - control instance

    @return error code - 0 for no error
*/ 
int32 sdp_init(struct sdp_control_t *in_ctl);

/*
    @brief SDP init server record
    @param in_attr_list - attr list
    @param in_attr_list_len - attr list len in bytes
    @param out_record - output record

    @return error code - 0 for no error
*/ 
int32 sdp_init_server_record(struct sdp_server_record_attr *in_attr_list, uint32 in_attr_list_len, struct sdp_server_record *out_record);

/*
    @brief Parse an element from buffer
    @param in_buff - buffer to parse
    @param in_len - buffer length
    @param out_type - element type
    @param out_header_len - element total length
    @param out_data_len - element data length

    @return error code - 0 for no error
*/
int32 sdp_parse_data_element(uint8 *in_buff, uint32 in_len, 
            enum data_element_type *out_type, uint32 *out_header_len, uint32 *out_data_len);

/*
    @brief Gether all right size uuid in ServiceClassID attr
    @param out_buff - buffer to fill
    @param out_buff_len - buffer length
    @param in_uuid_size - uuid size to find (in bytes)
    @param out_len - output length (in bytes)
    @param out_real_len - real uuid length (in bytes)

    @return error code - 0 for no error
*/
int32 sdp_gether_global_service_uuids(uint8 in_uuid_size, uint8 *out_buff, uint32 out_buff_len, uint32 *out_len, uint32 *out_real_len);

/*
    @brief Get attribute from attribute list
    Return attribute value data element (array)
    @param in_buff - buffer to parse
    @param in_len - buffer length
    @param in_attr_id - attribute id buffer
    @param in_attr_id_len - attribute id buffer len
    @param out_attr - found attribute value buffer (array), it is a data element sequence (array)
    @param out_attr - found attribute value buffer len (array)
    @param in_out_max_attr_count - out_attr pointer count, we may found multiple attribute
    @param out_attr_count - real found out_attr ponter count

    @return error code - 0 for no error
*/
int32 sdp_get_attribute_from_attribute_list(uint8 *in_buff, uint32 in_len, 
            uint8 *in_attr_id, uint8 in_attr_id_len, uint32 *out_attr, uint32 *out_attr_len, uint32 in_out_attr_count, uint32 *out_attr_count);

/*
    @brief Iterate data element list
    These macros are used when too much loops in one function
    @param label - name for this iterate (any symbol, like L1,L2,...)
    @param in_de_list_buff - element list buffer (uint8 *)
    @param in_de_buff_len - element list buffer len (uint32)
    @param out_de_buff - found data element buffer (uint8 *)
    @param out_type - found data element type (enum data_element_type *)
    @param out_header_len - found data element header len (uint32 *)
    @param out_data_len - found data element data len (uint32 *)
    @param out_error - 0 means no error (uint32 *)
*/
#define SDP_ITERATE_DATA_ELEMENT_LIST_START(label,in_de_list_buff,in_de_buff_len,out_de_buff,out_type,out_header_len,out_data_len,out_error) \
        { \
            POSSIBLY_UNUSED int32 __ret = 0, __break = 0; \
            POSSIBLY_UNUSED uint8 *__buff = in_de_list_buff; \
            POSSIBLY_UNUSED uint32 __offset = 0, __buff_len = in_de_buff_len; \
            while (__offset < __buff_len) { \
                *out_error = 0; \
                __ret = sdp_parse_data_element(__buff, __buff_len-__offset, out_type, out_header_len, out_data_len); \
                out_de_buff = __buff; \
                if (__ret != 0) { \
                    *out_error = 1; \
                } \
            __buff += *out_header_len + *out_data_len; \
            __offset += *out_header_len + *out_data_len;

#define S_I_D_E_L_CONTINUE(label) \
        goto __finish_loop_##label;

#define S_I_D_E_L_BREAK(label) \
        __break = 1; goto __finish_loop_##label;

#define SDP_ITERATE_DATA_ELEMENT_LIST_END(label) \
            } \
        }

#if 0
#define SDP_ITERATE_DATA_ELEMENT_LIST_END(label) \
            POSSIBLY_UNUSED __finish_loop_##label: \
                if (__break) break; \
            } \
        }
#endif

/*
    @brief UUID Compare
    @param uuid_a - uuid value a
    @param uuid_a_len - uuid value a len
    @param uuid_b - uuid value a
    @param uuid_b_len - uuid value b len

    @return result, 1 if equal, or 0
*/
int32 sdp_uuid_cmp(uint8 *uuid_a, uint32 uuid_a_len, uint8 *uuid_b, uint32 uuid_b_len);

/*
    @brief Find uuid in data element list
    An data elemement list means an array of data element who has buffer address and buffer length.
    It can be a data element sequence/alternative or a few data elements in an array.
    In common case, no matter sdp request or sdp response, data to be parsed in which will be a data element sequence.

    @param in_buff - buffer to parse
    @param in_buff_len - buffer length
    @param in_uuid - uuid to find
    @param in_uuid_len - uuid length

    @return result, 0 means not found, 1 means found at least one
*/
int32 sdp_find_uuid_in_data_element_list(uint8 *in_buff, uint32 in_len, uint8 *uuid, uint32 uuid_len);


/*
    @brief Walk data element list
    Details see sdp_find_uuid_in_data_element_list.

    @param in_buff - buffer to parse
    @param in_buff_len - buffer length
    @param cb - callback for caller to deal with current data element
    @param stop - stop flag,  1 to stop walking
    @param priv - priv data to pass into cb

    @return result, 0 - no error, non 0 - error
*/
typedef void (*T_sdp_walk_data_element_list_cb)(enum data_element_type type, uint8 *parent_buff, uint8 *buff, \
    uint32 header_len, uint32 data_len, uint8 *stop, uint32 *priv);
int32 sdp_walk_data_element_list(uint8 *buff, uint32 buff_len, T_sdp_walk_data_element_list_cb cb, uint8 *stop, uint32 *priv);

/*
    @brief SDP client send a request to server
    @param in_ctl - control instance
    @param in_request - request to send

    @return error code - 0 for no error
*/ 
int32 sdp_client_request(struct sdp_request *in_request);

/*
    @brief SDP close
    @param in_ctl - control instance

    @return error code - 0 for no error
*/ 
int32 sdp_close(struct sdp_control_t *in_ctl);

/*
    @brief SDP server add record
    @param in_ctl - control instance
    @param in_record - server record

    @return error code - 0 for no error
*/
#if defined(SDP_PRIVATE_RECORD_LIST)
int32 sdp_server_add_record(struct sdp_control_t *in_ctl, struct sdp_server_record *in_record);
int32 sdp_server_remove_record(struct sdp_control_t *in_ctl, struct sdp_server_record *in_record);
#endif
int32 sdp_server_add_global_record(struct sdp_server_record *in_record);
int32 sdp_server_remove_global_record(struct sdp_server_record *in_record);
uint32 sdp_serv_find_sdp_record_handle(uint32 handle);

/*
    @brief Generate an data element in given buffer
    @param in_buff - buffer to generate the data element
    @param in_len - buffer length
    @param in_type - element type
    @param in_data - data buffer
    @param in_data_len - element data length
    @param out_header_len - output header len

    @return error code - 0 for no error
*/
int32 sdp_generate_an_data_element(uint8 *in_buff, uint32 in_len, enum data_element_type type, int8 *in_data, uint32 in_data_len, uint32 *ou_header_len);

/*
    @brief Edit an data element length in given buffer
    @param in_len - buffer length
    @param in_buff - buffer to generate the data element

    @return error code - 0 for no error
*/
int32 sdp_edit_data_element_length(uint8 *in_buff, uint32 in_buff_len, uint32 in_data_len);

/*
    @brief Convert sdp event to string
    @param event - event

    @return event string or "[]"
*/
const char *sdp_event2str(enum sdp_event event);

uint16 sdp_serv_find_max_attr_id(const struct sdp_server_record *record, uint16 origin_max);


#ifdef __cplusplus
}
#endif

#endif /* __SDP_H__ */
