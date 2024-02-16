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
#include "aud_section.h"
#include "cmsis.h"
#include "crc32.h"
#include "hal_norflash.h"
#include "hal_trace.h"
#include "string.h"
#include "tgt_hardware.h"
#include <stdio.h>
#ifdef __ARMCC_VERSION
#include "link_sym_armclang.h"
#endif

extern uint32_t __aud_start[];

#ifndef ANC_COEF_LIST_NUM
#define ANC_COEF_LIST_NUM 0
#endif

#define MAGIC_NUMBER 0xBE57
#define USERDATA_VERSION 0x0001

static uint32_t section_device_length[AUDIO_SECTION_DEVICE_NUM] = {
    AUDIO_SECTION_LENGTH_ANC,
    AUDIO_SECTION_LENGTH_AUDIO,
    AUDIO_SECTION_LENGTH_SPEECH,
};

static uint32_t audio_section_get_device_addr_offset(uint32_t device) {
  ASSERT(device < AUDIO_SECTION_DEVICE_NUM,
         "[%s] device(%d) >= AUDIO_SECTION_DEVICE_NUM", __func__, device);

  uint32_t addr_offset = 0;

  for (uint32_t i = 0; i < device; i++) {
    addr_offset += section_device_length[i];
  }

  return addr_offset;
}

static audio_section_t *audio_section_get_device_ptr(uint32_t device) {
  uint8_t *section_ptr = (uint8_t *)__aud_start;
  section_ptr += audio_section_get_device_addr_offset(device);

  return (audio_section_t *)section_ptr;
}

int audio_section_store_cfg(uint32_t device, uint8_t *cfg, uint32_t len) {
  audio_section_t *section_ptr = (audio_section_t *)cfg;
  uint32_t addr_start = 0;
  uint32_t crc = 0;

  section_ptr->head.magic = MAGIC_NUMBER;
  section_ptr->head.version = USERDATA_VERSION;
  section_ptr->device = device;
  section_ptr->cfg_len = len;

  // calculate crc
  crc = crc32(0, (unsigned char *)section_ptr + AUDIO_SECTION_CFG_RESERVED_LEN,
              len - AUDIO_SECTION_CFG_RESERVED_LEN);
  section_ptr->head.crc = crc;

  addr_start =
      (uint32_t)__aud_start + audio_section_get_device_addr_offset(device);

#ifdef AUDIO_SECTION_DEBUG
  TRACE(2, "[%s] len = %d", __func__, len);
  TRACE(2, "[%s] addr_start = 0x%x", __func__, addr_start);
  TRACE(2, "[%s] block length = 0x%x", __func__, section_device_length[device]);
#endif

  // FIXME: CHECK return value
  enum HAL_NORFLASH_RET_T flash_opt_res;
  uint32_t flag = int_lock();
  flash_opt_res = hal_norflash_erase(HAL_NORFLASH_ID_0, addr_start,
                                     section_device_length[device]);
  int_unlock(flag);

  if (flash_opt_res) {
    TRACE(2, "[%s] ERROR: erase flash res = %d", __func__, flash_opt_res);
    return flash_opt_res;
  }

  flag = int_lock();
  flash_opt_res = hal_norflash_write(HAL_NORFLASH_ID_0, addr_start,
                                     (uint8_t *)section_ptr, len);
  int_unlock(flag);

  if (flash_opt_res) {
    TRACE(2, "[%s] ERROR: write flash res = %d", __func__, flash_opt_res);
    return flash_opt_res;
  }

#ifdef AUDIO_SECTION_DEBUG
  TRACE(1, "********************[%s]********************", __func__);
  TRACE(1, "magic:       0x%x", section_ptr->head.magic);
  TRACE(1, "version:     0x%x", section_ptr->head.version);
  TRACE(1, "crc:         0x%x", section_ptr->head.crc);
  TRACE(1, "device:      %d", section_ptr->device);
  TRACE(1, "cfg_len:    %d", section_ptr->cfg_len);
  TRACE(0, "********************END********************");
#endif

  // audio_section_t *section_read_ptr = audio_section_get_device_ptr(device);
  // check

  return 0;
}

int audio_section_load_cfg(uint32_t device, uint8_t *cfg, uint32_t len) {
  audio_section_t *section_ptr = audio_section_get_device_ptr(device);
  uint32_t crc = 0;

#ifdef AUDIO_SECTION_DEBUG
  TRACE(1, "********************[%s]********************", __func__);
  TRACE(1, "magic:       0x%x", section_ptr->head.magic);
  TRACE(1, "version:     0x%x", section_ptr->head.version);
  TRACE(1, "crc:         0x%x", section_ptr->head.crc);
  TRACE(1, "device:      %d", section_ptr->device);
  TRACE(1, "cfg_len:    %d", section_ptr->cfg_len);
  TRACE(0, "********************END********************");
#endif

  if (section_ptr->head.magic != MAGIC_NUMBER) {
    TRACE(3, "[%s] WARNING: Different magic number (%x != %x)", __func__,
          section_ptr->head.magic, MAGIC_NUMBER);
    return -1;
  }

  // Calculate crc and check crc value
  crc = crc32(0, (unsigned char *)section_ptr + AUDIO_SECTION_CFG_RESERVED_LEN,
              len - AUDIO_SECTION_CFG_RESERVED_LEN);

  if (section_ptr->head.crc != crc) {
    TRACE(3, "[%s] WARNING: Different crc (%x != %x)", __func__,
          section_ptr->head.crc, crc);
    return -2;
  }

  if (section_ptr->device != device) {
    TRACE(3, "[%s] WARNING: Different device (%d != %d)", __func__,
          section_ptr->device, device);
    return -3;
  }

  if (section_ptr->cfg_len != len) {
    TRACE(3, "[%s] WARNING: Different length (%d != %d)", __func__,
          section_ptr->cfg_len, len);
    return -4;
  }

  memcpy(cfg, section_ptr, len);

  return 0;
}

int anccfg_loadfrom_audsec(const struct_anc_cfg *list[],
                           const struct_anc_cfg *list_44p1k[], uint32_t count) {
#ifdef PROGRAMMER

  return 1;

#else // !PROGRAMMER

#ifdef CHIP_BEST1000
  ASSERT(0, "[%s] Can not support anc load in this branch!!!", __func__);
#else
  unsigned int re_calc_crc, i;
  const pctool_aud_section *audsec_ptr;

  audsec_ptr = (pctool_aud_section *)__aud_start;
  TRACE(3, "0x%x,0x%x,0x%x", audsec_ptr->sec_head.magic,
        audsec_ptr->sec_head.version, audsec_ptr->sec_head.crc);
  if (audsec_ptr->sec_head.magic != aud_section_magic) {
    TRACE(0, "Invalid aud section - magic");
    return 1;
  }
  re_calc_crc = crc32(0, (unsigned char *)&(audsec_ptr->sec_body),
                      sizeof(audsec_body) - 4);
  if (re_calc_crc != audsec_ptr->sec_head.crc) {
    TRACE(0, "crc verify failure, invalid aud section.");
    return 1;
  }
  TRACE(0, "Valid aud section.");
  for (i = 0; i < ANC_COEF_LIST_NUM; i++)
    list[i] =
        (struct_anc_cfg *)&(audsec_ptr->sec_body.anc_config.anc_config_arr[i]
                                .anc_cfg[PCTOOL_SAMPLERATE_48X8K]);
#if (AUD_SECTION_STRUCT_VERSION == 3)
  for (i = 0; i < ANC_COEF_LIST_NUM; i++)
    list_44p1k[i] =
        (struct_anc_cfg *)&(audsec_ptr->sec_body.anc_config.anc_config_arr[i]
                                .anc_cfg[PCTOOL_SAMPLERATE_48X8K]);
#else
  for (i = 0; i < ANC_COEF_LIST_NUM; i++)
    list_44p1k[i] =
        (struct_anc_cfg *)&(audsec_ptr->sec_body.anc_config.anc_config_arr[i]
                                .anc_cfg[PCTOOL_SAMPLERATE_44_1X8K]);
#endif
#endif

  return 0;

#endif // !PROGRAMMER
}
