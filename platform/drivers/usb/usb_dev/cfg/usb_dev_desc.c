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
#include "usb_dev_desc.h"
#include "hal_usb.h"
#include "usb_descriptor.h"
#include "tgt_hardware.h"

#ifdef USB_HIGH_SPEED
#define DEV_QUAL_USB_VER                        0x0200
#define USB_CDC_USB_VER                         0x0200
#define USB_AUDIO_USB_VER                       0x0201
#define USB_CDC_PRODUCT_ID_BASE                 0x0120
#ifdef USB_AUDIO_UAC2
#define USB_AUDIO_PRODUCT_ID_BASE               0x0400
#else
#define USB_AUDIO_PRODUCT_ID_BASE               0x0300
#endif
#else
#define DEV_QUAL_USB_VER                        0x0200
#define USB_CDC_USB_VER                         0x0200 //0x0110
#define USB_AUDIO_USB_VER                       0x0200 //0x0110
#define USB_CDC_PRODUCT_ID_BASE                 0x0100
#define USB_AUDIO_PRODUCT_ID_BASE               0x0200
#ifdef USB_AUDIO_UAC2
#error "USB_AUDIO_UAC2 should run on USB_HIGH_SPEED"
#endif
#endif

#ifndef USB_CDC_VENDOR_ID
#define USB_CDC_VENDOR_ID                       0xBE57
#endif

#ifndef USB_CDC_PRODUCT_ID
#define USB_CDC_PRODUCT_ID                      (USB_CDC_PRODUCT_ID_BASE + 0x01)
#endif

#ifndef USB_AUDIO_VENDOR_ID
#define USB_AUDIO_VENDOR_ID                     0xBE57
#endif

#ifndef USB_AUDIO_PRODUCT_ID

#ifdef USB_AUDIO_DYN_CFG

#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x31)
#ifdef USB_AUDIO_32BIT
#define USB_AUDIO_PRODUCT_32BIT_FLAG            0x0004
#else
#define USB_AUDIO_PRODUCT_32BIT_FLAG            0
#endif
#ifdef USB_AUDIO_24BIT
#define USB_AUDIO_PRODUCT_24BIT_FLAG            0x0002
#else
#define USB_AUDIO_PRODUCT_24BIT_FLAG            0
#endif
#ifdef USB_AUDIO_16BIT
#define USB_AUDIO_PRODUCT_16BIT_FLAG            0x0001
#else
#define USB_AUDIO_PRODUCT_16BIT_FLAG            0
#endif

#define USB_AUDIO_PRODUCT_ID                    (USB_AUDIO_PRODUCT_ID_STEM + USB_AUDIO_PRODUCT_32BIT_FLAG + \
                                                USB_AUDIO_PRODUCT_24BIT_FLAG + USB_AUDIO_PRODUCT_16BIT_FLAG)

#else // !USB_AUDIO_DYN_CFG

#ifdef USB_AUDIO_384K
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x08)
#elif defined(USB_AUDIO_352_8K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x07)
#elif defined(USB_AUDIO_192K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x06)
#elif defined(USB_AUDIO_176_4K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x05)
#elif defined(USB_AUDIO_96K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x04)
#elif defined(USB_AUDIO_44_1K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x03)
#elif defined(USB_AUDIO_16K)
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x02)
#else // 48K
#define USB_AUDIO_PRODUCT_ID_STEM               (USB_AUDIO_PRODUCT_ID_BASE + 0x01)
#endif

#define USB_AUDIO_PRODUCT_32BIT_FLAG            0x0020
#define USB_AUDIO_PRODUCT_24BIT_FLAG            0x0010

#ifdef USB_AUDIO_32BIT
#define USB_AUDIO_PRODUCT_ID                    (USB_AUDIO_PRODUCT_ID_STEM + USB_AUDIO_PRODUCT_32BIT_FLAG)
#elif defined(USB_AUDIO_24BIT)
#define USB_AUDIO_PRODUCT_ID                    (USB_AUDIO_PRODUCT_ID_STEM + USB_AUDIO_PRODUCT_24BIT_FLAG)
#else
#define USB_AUDIO_PRODUCT_ID                    (USB_AUDIO_PRODUCT_ID_STEM)
#endif

#endif // !USB_AUDIO_DYN_CFG

#endif // !USB_AUDIO_PRODUCT_ID

//----------------------------------------------------------------
// USB device common string descriptions
//----------------------------------------------------------------

static const uint8_t stringLangidDescriptor[] = {
    0x04,               /*bLength*/
    STRING_DESCRIPTOR,  /*bDescriptorType 0x03*/
    0x09,0x04,          /*bString Lang ID - 0x0409 - English*/
};

static const uint8_t stringIconfigurationDescriptor[] = {
    0x06,               /*bLength*/
    STRING_DESCRIPTOR,  /*bDescriptorType 0x03*/
    '0',0,'1',0,        /*bString iConfiguration - 01*/
};

#ifdef USB_HIGH_SPEED
static const uint8_t deviceQualifierDescriptor[] = {
    10,
    QUALIFIER_DESCRIPTOR,
    LSB(DEV_QUAL_USB_VER), MSB(DEV_QUAL_USB_VER),
    0,                    // bDeviceClass
    0,                    // bDeviceSubClass
    0,                    // bDeviceProtocol
    USB_MAX_PACKET_SIZE_CTRL, // bMaxPacketSize0
    0,
    0,
};

static const uint8_t bosDescriptor[] = {
    5,
    BOS_DESCRIPTOR,
    12, 0,                  // wTotalLength
    1,                      // bNumDeviceCaps
    7,                      // bLength
    0x10,                   // bDescriptorType
    0x02,                   // bDevCapabilityType
#ifdef USB_LPM
#ifdef USB_LPM_DEEP_BESL
    0x1E,                   // bmAttributes: LPM, BESL, Baseline BESL, Deep BESL
#else
    0x0E,                   // bmAttributes: LPM, BESL, Baseline BESL
#endif
    (USB_L1_DEEP_SLEEP_BESL << 4) | USB_L1_LIGHT_SLEEP_BESL,
#else
    0x00,                   // bmAttributes: none
    0x00,
#endif
    0x00,
    0x00
};
#endif

//----------------------------------------------------------------
// USB CDC device description and string descriptions
//----------------------------------------------------------------

const uint8_t *cdc_dev_desc(uint8_t type)
{
    static const uint8_t deviceDescriptor[] = {
        18,                   // bLength
        DEVICE_DESCRIPTOR,    // bDescriptorType
        LSB(USB_CDC_USB_VER), MSB(USB_CDC_USB_VER), // bcdUSB
        2,                    // bDeviceClass
        0,                    // bDeviceSubClass
        0,                    // bDeviceProtocol
        USB_MAX_PACKET_SIZE_CTRL,  // bMaxPacketSize0
        LSB(USB_CDC_VENDOR_ID), MSB(USB_CDC_VENDOR_ID), // idVendor
        LSB(USB_CDC_PRODUCT_ID), MSB(USB_CDC_PRODUCT_ID), // idProduct
        0x00, 0x01,           // bcdDevice
        STRING_OFFSET_IMANUFACTURER, // iManufacturer
        STRING_OFFSET_IPRODUCT, // iProduct
        STRING_OFFSET_ISERIAL, // iSerialNumber
        1                     // bNumConfigurations
    };

    if (type == DEVICE_DESCRIPTOR) {
        return deviceDescriptor;
#ifdef USB_HIGH_SPEED
    } else if (type == QUALIFIER_DESCRIPTOR) {
        return deviceQualifierDescriptor;
    } else if (type == BOS_DESCRIPTOR) {
        return bosDescriptor;
#endif
    } else {
        return NULL;
    }
}

const uint8_t *cdc_string_desc(uint8_t index)
{
    static const uint8_t stringImanufacturerDescriptor[] =
#ifdef USB_CDC_STR_DESC_MANUFACTURER
        USB_CDC_STR_DESC_MANUFACTURER;
#else
    {
        0x16,                                            /*bLength*/
        STRING_DESCRIPTOR,                               /*bDescriptorType 0x03*/
        'b',0,'e',0,'s',0,'t',0,'e',0,'c',0,'h',0,'n',0,'i',0,'c',0 /*bString iManufacturer - bestechnic*/
    };
#endif

    static const uint8_t stringIserialDescriptor[] =
#ifdef USB_CDC_STR_DESC_SERIAL
        USB_CDC_STR_DESC_SERIAL;
#else
    {
        0x16,                                                           /*bLength*/
        STRING_DESCRIPTOR,                                              /*bDescriptorType 0x03*/
        '2',0,'0',0,'1',0,'5',0,'1',0,'0',0,'0',0,'6',0,'.',0,'1',0,    /*bString iSerial - 20151006.1*/
    };
#endif

    static const uint8_t stringIinterfaceDescriptor[] =
#ifdef USB_CDC_STR_DESC_INTERFACE
        USB_CDC_STR_DESC_INTERFACE;
#else
    {
        0x08,
        STRING_DESCRIPTOR,
        'C',0,'D',0,'C',0,
    };
#endif

    static const uint8_t stringIproductDescriptor[] =
#ifdef USB_CDC_STR_DESC_PRODUCT
        USB_CDC_STR_DESC_PRODUCT;
#else
    {
        0x16,
        STRING_DESCRIPTOR,
        'C',0,'D',0,'C',0,' ',0,'D',0,'E',0,'V',0,'I',0,'C',0,'E',0
    };
#endif

    const uint8_t *data = NULL;

    switch (index)
    {
        case STRING_OFFSET_LANGID:
            data = stringLangidDescriptor;
            break;
        case STRING_OFFSET_IMANUFACTURER:
            data = stringImanufacturerDescriptor;
            break;
        case STRING_OFFSET_IPRODUCT:
            data = stringIproductDescriptor;
            break;
        case STRING_OFFSET_ISERIAL:
            data = stringIserialDescriptor;
            break;
        case STRING_OFFSET_ICONFIGURATION:
            data = stringIconfigurationDescriptor;
            break;
        case STRING_OFFSET_IINTERFACE:
            data = stringIinterfaceDescriptor;
            break;
    }

    return data;
}

//----------------------------------------------------------------
// USB audio device description and string descriptions
//----------------------------------------------------------------

const uint8_t *uaud_dev_desc(uint8_t type)
{
    static const uint8_t deviceDescriptor[] = {
        18,                   // bLength
        DEVICE_DESCRIPTOR,    // bDescriptorType
        LSB(USB_AUDIO_USB_VER), MSB(USB_AUDIO_USB_VER), // bcdUSB
        0,                    // bDeviceClass
        0,                    // bDeviceSubClass
        0,                    // bDeviceProtocol
        USB_MAX_PACKET_SIZE_CTRL, // bMaxPacketSize0
        LSB(USB_AUDIO_VENDOR_ID), MSB(USB_AUDIO_VENDOR_ID), // idVendor
        LSB(USB_AUDIO_PRODUCT_ID), MSB(USB_AUDIO_PRODUCT_ID), // idProduct
        0x00, 0x01,           // bcdDevice
        STRING_OFFSET_IMANUFACTURER, // iManufacturer
        STRING_OFFSET_IPRODUCT, // iProduct
        STRING_OFFSET_ISERIAL, // iSerialNumber
        1                     // bNumConfigurations
    };

    if (type == DEVICE_DESCRIPTOR) {
        return deviceDescriptor;
#ifdef USB_HIGH_SPEED
    } else if (type == QUALIFIER_DESCRIPTOR) {
        return deviceQualifierDescriptor;
    } else if (type == BOS_DESCRIPTOR) {
        return bosDescriptor;
#endif
    } else {
        return NULL;
    }
}

const uint8_t *uaud_string_desc(uint8_t index)
{
    static const uint8_t stringImanufacturerDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_MANUFACTURER
        USB_AUDIO_STR_DESC_MANUFACTURER;
#else
    {
        0x16,                                            /*bLength*/
        STRING_DESCRIPTOR,                               /*bDescriptorType 0x03*/
        'b',0,'e',0,'s',0,'t',0,'e',0,'c',0,'h',0,'n',0,'i',0,'c',0 /*bString iManufacturer - bestechnic*/
    };
#endif

    static const uint8_t stringIserialDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_SERIAL
        USB_AUDIO_STR_DESC_SERIAL;
#else
    {
        0x16,                                                           /*bLength*/
        STRING_DESCRIPTOR,                                              /*bDescriptorType 0x03*/
        '2',0,'0',0,'1',0,'6',0,'0',0,'4',0,'0',0,'6',0,'.',0,'1',0,    /*bString iSerial - 20160406.1*/
    };
#endif

    static const uint8_t stringIinterfaceDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_INTERFACE
        USB_AUDIO_STR_DESC_INTERFACE;
#else
    {
        0x0c,                           //bLength
        STRING_DESCRIPTOR,              //bDescriptorType 0x03
        'A',0,'u',0,'d',0,'i',0,'o',0   //bString iInterface - Audio
    };
#endif

    static const uint8_t stringIproductDescriptor[] =
#ifdef USB_AUDIO_STR_DESC_PRODUCT
        USB_AUDIO_STR_DESC_PRODUCT;
#else
    {
        0x16,                                                       //bLength
        STRING_DESCRIPTOR,                                          //bDescriptorType 0x03
        'B',0,'e',0,'s',0,'t',0,' ',0,'A',0,'u',0,'d',0,'i',0,'o',0 //bString iProduct - Best Audio
    };
#endif

    const uint8_t *data = NULL;

    switch (index)
    {
        case STRING_OFFSET_LANGID:
            data = stringLangidDescriptor;
            break;
        case STRING_OFFSET_IMANUFACTURER:
            data = stringImanufacturerDescriptor;
            break;
        case STRING_OFFSET_IPRODUCT:
            data = stringIproductDescriptor;
            break;
        case STRING_OFFSET_ISERIAL:
            data = stringIserialDescriptor;
            break;
        case STRING_OFFSET_ICONFIGURATION:
            data = stringIconfigurationDescriptor;
            break;
        case STRING_OFFSET_IINTERFACE:
            data = stringIinterfaceDescriptor;
            break;
    }

    return data;
}


