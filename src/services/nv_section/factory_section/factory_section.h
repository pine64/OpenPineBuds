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
#ifndef __FACTORY_SECTIONS_H__
#define __FACTORY_SECTIONS_H__
#include <stdbool.h>
#include <stdint.h>

#define ALIGN4 __attribute__((aligned(4)))
#define nvrec_mini_version 1
#define nvrec_dev_magic 0xba80
#define nvrec_current_version 2
#define FACTORY_SECTOR_SIZE 4096
typedef struct {
  unsigned short magic;
  unsigned short version;
  unsigned int crc;
  unsigned int reserved0;
  unsigned int reserved1;
} section_head_t;

typedef struct {
  unsigned char device_name[248 + 1] ALIGN4;
  unsigned char bt_address[8] ALIGN4;
  unsigned char ble_address[8] ALIGN4;
  unsigned char tester_address[8] ALIGN4;
  unsigned int xtal_fcap ALIGN4;
  unsigned int rev1_data_len;

  unsigned int rev2_data_len;
  unsigned int rev2_crc;
  unsigned int rev2_reserved0;
  unsigned int rev2_reserved1;
  unsigned int rev2_bt_name[63];
  unsigned int rev2_bt_addr[2];
  unsigned int rev2_ble_addr[2];
  unsigned int rev2_dongle_addr[2];
  unsigned int rev2_xtal_fcap;
  unsigned int rev2_ble_name[8];
} factory_section_data_t;

typedef struct {
  section_head_t head;
  factory_section_data_t data;
} factory_section_t;

#ifdef __cplusplus
extern "C" {
#endif

void factory_section_init(void);
int factory_section_open(void);
void factory_section_original_btaddr_get(uint8_t *btAddr);
int factory_section_xtal_fcap_get(unsigned int *xtal_fcap);
int factory_section_xtal_fcap_set(unsigned int xtal_fcap);
uint8_t *factory_section_get_bt_address(void);
uint8_t *factory_section_get_bt_name(void);
uint8_t *factory_section_get_ble_name(void);
uint32_t factory_section_get_version(void);

#ifdef __cplusplus
}
#endif
#endif
