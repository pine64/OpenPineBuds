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


#ifndef __BTLIB_TYPE_H__
#define __BTLIB_TYPE_H__

#ifndef NULL
#define NULL 0
#endif

#ifndef SUCCESS
#define SUCCESS     0
#endif

#ifndef FAILURE
#define FAILURE     1
#endif

#ifndef INPROGRESS
#define INPROGRESS  2
#endif

#ifndef L2C_DISCONNECT_ITSELF
#define L2C_DISCONNECT_ITSELF  (0x5E)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef unsigned char BtStatus;

#define BT_STATUS_SUCCESS         0  /* Successful and complete */
#define BT_STATUS_FAILED          1  /* Operation failed */
#define BT_STATUS_PENDING         2  /* Successfully started but pending */
#define BT_STATUS_DISCONNECT      3  /* Link disconnected */
#define BT_STATUS_NO_LINK         4  /* No Link layer Connection exists */
#define BT_STATUS_IN_USE          5  /* Operation failed - already in use. */
/* IrDA specific return codes */
#define BT_STATUS_MEDIA_BUSY      6  /* IRDA: Media is busy */
#define BT_STATUS_MEDIA_NOT_BUSY  7  /* IRDA: Media is not busy */
#define BT_STATUS_NO_PROGRESS     8  /* IRDA: IrLAP not making progress */
#define BT_STATUS_LINK_OK         9  /* IRDA: No progress condition cleared */
#define BT_STATUS_SDU_OVERRUN    10  /* IRDA: Sent more data than current SDU size */
/* Bluetooth specific return codes */
#define BT_STATUS_BUSY              11
#define BT_STATUS_NO_RESOURCES      12
#define BT_STATUS_NOT_FOUND         13
#define BT_STATUS_DEVICE_NOT_FOUND  14
#define BT_STATUS_CONNECTION_FAILED 15
#define BT_STATUS_TIMEOUT           16
#define BT_STATUS_NO_CONNECTION     17
#define BT_STATUS_INVALID_PARM      18
#define BT_STATUS_IN_PROGRESS       19
#define BT_STATUS_RESTRICTED        20
#define BT_STATUS_INVALID_TYPE      21
#define BT_STATUS_HCI_INIT_ERR      22
#define BT_STATUS_NOT_SUPPORTED     23
#define BT_STATUS_CONTINUE          24
#define BT_STATUS_CANCELLED         25

/* The last defined status code */
#define BT_STATUS_LAST_CODE         25

#define BIT0                            0x00000001
#define BIT1                            0x00000002
#define BIT2                            0x00000004
#define BIT3                            0x00000008
#define BIT4                            0x00000010
#define BIT5                            0x00000020
#define BIT6                            0x00000040
#define BIT7                            0x00000080
#define BIT8                            0x00000100
#define BIT9                            0x00000200
#define BIT10                           0x00000400
#define BIT11                           0x00000800
#define BIT12                           0x00001000
#define BIT13                           0x00002000
#define BIT14                           0x00004000
#define BIT15                           0x00008000
#define BIT16                           0x00010000
#define BIT17                           0x00020000
#define BIT18                           0x00040000
#define BIT19                           0x00080000
#define BIT20                           0x00100000
#define BIT21                           0x00200000
#define BIT22                           0x00400000
#define BIT23                           0x00800000
#define BIT24                           0x01000000
#define BIT25                           0x02000000
#define BIT26                           0x04000000
#define BIT27                           0x08000000
#define BIT28                           0x10000000
#define BIT29                           0x20000000
#define BIT30                           0x40000000
#define BIT31                           0x80000000


typedef unsigned char  byte;                    /* Unsigned  8 bit quantity                           */
typedef unsigned char  uint8;                    /* Unsigned  8 bit quantity                           */
typedef unsigned char  uint8_t;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  int8;                    /* Signed    8 bit quantity                           */
typedef unsigned short uint16;                   /* Unsigned 16 bit quantity                           */
typedef signed   short int16;                   /* Signed   16 bit quantity                           */
typedef unsigned int   uint32;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   int32;                   /* Signed   32 bit quantity                           */

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                    /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                   /* Unsigned 16 bit quantity                           */                                                                                                  
typedef signed   short INT16S;                   /* Signed   16 bit quantity                           */
typedef unsigned int   INT32U;                   /* Unsigned 32 bit quantity                           */
typedef signed   int   INT32S;                   /* Signed   32 bit quantity                           */
typedef float          FP32;                     /* Single precision floating point                    */
typedef double         FP64;                     /* Double precision floating point                    */


typedef unsigned char  		u8;  
#if 0
typedef signed   char  		s8;  
typedef unsigned short 		u16; 
typedef signed   short		s16; 
typedef unsigned int  		u32; 
typedef signed   int  		s32; 
typedef unsigned long long 	u64; 
typedef signed   long long 	s64;
#endif

typedef uint16  CNH;



typedef   struct {
    uint8  A[6];
}  __attribute__ ((packed)) BD_ADDR;

typedef    struct{
	uint8 A[10];
}  __attribute__ ((packed)) CHANMAP;


typedef  struct {
    uint8  A[3];
}  __attribute__ ((packed))  LAP;


typedef  struct {
    uint8  A[3];
} __attribute__ ((packed))   CLASS;


typedef  struct {
    uint8  A[16];
} __attribute__ ((packed))   PIN_CODE;


typedef  struct {
    uint8  A[16];
} __attribute__ ((packed))   LINK_KEY;


typedef  struct {
    uint8  A[4];
}  __attribute__ ((packed))  SRES;


typedef  struct {
    uint8  A[12];
} __attribute__ ((packed))   ACO;



typedef  struct {
    uint8   A[8];
} __attribute__ ((packed))  FEATURES;


typedef  struct {
    uint8  A[14];
} __attribute__ ((packed))   NAME_VEC;



struct bdaddr_t {
    uint8 addr[6];
}__attribute__ ((packed));

 struct class_of_device_t {
    uint8  A[3];
}__attribute__ ((packed));

 struct link_key_t{
    uint8  A[16];
}__attribute__ ((packed));

#define STR_BE32(buff,num) ( (((U8*)buff)[0] = (U8) ((num)>>24)),  \
                              (((U8*)buff)[1] = (U8) ((num)>>16)),  \
                              (((U8*)buff)[2] = (U8) ((num)>>8)),   \
                              (((U8*)buff)[3] = (U8) (num)) )

#define STR_BE16(buff,num) ( (((U8*)buff)[0] = (U8) ((num)>>8)),    \
                              (((U8*)buff)[1] = (U8) (num)) )

#define BEtoHost16(ptr)  (U16)( ((U16) *((U8*)(ptr)) << 8) | \
                                ((U16) *((U8*)(ptr)+1)) )                              

#define BEtoHost32(ptr)  (U32)( ((U32) *((U8*)(ptr)) << 24)   | \
                                ((U32) *((U8*)(ptr)+1) << 16) | \
                                ((U32) *((U8*)(ptr)+2) << 8)  | \
                                ((U32) *((U8*)(ptr)+3)) )                                

/* Store value into a buffer in Little Endian format */
#define StoreLE16(buff,num) ( ((buff)[1] = (U8) ((num)>>8)),    \
                              ((buff)[0] = (U8) (num)) )

#define StoreLE32(buff,num) ( ((buff)[3] = (U8) ((num)>>24)),  \
                              ((buff)[2] = (U8) ((num)>>16)),  \
                              ((buff)[1] = (U8) ((num)>>8)),   \
                              ((buff)[0] = (U8) (num)) )

/* Store value into a buffer in Big Endian format */
#define StoreBE16(buff,num) ( ((buff)[0] = (U8) ((num)>>8)),    \
                              ((buff)[1] = (U8) (num)) )

#define StoreBE32(buff,num) ( ((buff)[0] = (U8) ((num)>>24)),  \
                              ((buff)[1] = (U8) ((num)>>16)),  \
                              ((buff)[2] = (U8) ((num)>>8)),   \
                              ((buff)[3] = (U8) (num)) )


#define LEtoHost16(ptr)  (U16)(((U16) *((U8*)(ptr)+1) << 8) | \
        (U16) *((U8*)(ptr)))                                

#define BDADDR_ANY   (&(bdaddr_t) {{0, 0, 0, 0, 0, 0}})
#define BDADDR_ALL   (&(bdaddr_t) {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}})
#define BDADDR_LOCAL (&(bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}})

#define CTX_INIT(buff) \
    POSSIBLY_UNUSED unsigned int __offset = 2; \
    POSSIBLY_UNUSED unsigned char *__buff = buff;

#define CTX_STR_BUF(buff,len) \
    memcpy(__buff+__offset, buff, len); \
    __offset += len;

#define CTX_LDR_BUF(buff,len) \
    memcpy(buff, __buff+__offset, len); \
    __offset += len;

#define CTX_STR_VAL8(v) \
    __buff[__offset] = v&0xFF; \
    __offset += 1;

#define CTX_LDR_VAL8(v) \
    v = __buff[__offset]; \
    __offset += 1;

#define CTX_STR_VAL16(v) \
    __buff[__offset] = v&0xFF; \
    __buff[__offset+1] = (v>>8)&0xFF; \
    __offset += 2;

#define CTX_LDR_VAL16(v) \
    v = __buff[__offset]; \
    v |= __buff[__offset+1]<<8; \
    __offset += 2;

#define CTX_STR_VAL32(v) \
    __buff[__offset] = v&0xFF; \
    __buff[__offset+1] = (v>>8)&0xFF; \
    __buff[__offset+2] = (v>>16)&0xFF; \
    __buff[__offset+3] = (v>>24)&0xFF; \
    __offset += 4;

#define CTX_LDR_VAL32(v) \
    v = __buff[__offset]; \
    v |= __buff[__offset+1]<<8; \
    v |= __buff[__offset+2]<<16; \
    v |= __buff[__offset+3]<<24; \
    __offset += 4;

#define CTX_GET_BUF_CURR() __buff

#define CTX_GET_BUF_HEAD() __buff

#define CTX_GET_OFFSET() __offset

#define CTX_GET_DATA_LEN() (__buff[0] | __buff[1]<<8)

#define CTX_GET_TOTAL_LEN() (CTX_GET_DATA_LEN()+2)

#define CTX_SAVE_UPDATE_DATA_LEN() \
   __buff[0] = (__offset-2)&0xFF; \
   __buff[1] = ((__offset-2)>>8)&0xFF;

struct ctx_content {
    uint8 *buff;
    uint32 buff_len;
};

#endif /* __BTLIB_TYPE_H__ */