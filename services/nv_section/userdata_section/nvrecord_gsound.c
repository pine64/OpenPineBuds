/***************************************************************************
 *
 *Copyright 2015-2019 BES.
 *All rights reserved. All unpublished rights reserved.
 *
 *No part of this work may be used or reproduced in any form or by any
 *means, or stored in a database or retrieval system, without prior written
 *permission of BES.
 *
 *Use of this work is governed by a license granted by BES.
 *This work contains confidential and proprietary information of
 *BES. which is protected by copyright, trade secret,
 *trademark and other intellectual property rights.
 *
 ****************************************************************************/

/*****************************header include********************************/
#include "nvrecord_gsound.h"
#include "cmsis.h"
#include "factory_section.h"
#include "nv_section_dbg.h"

#ifdef BISTO_ENABLED
#include "gsound_target_ota.h"

#ifdef GSOUND_HOTWORD_ENABLED
#include "gsound_custom_hotword_common.h"
#endif
#endif

/************************private macro defination***************************/
#define HOTWORD_MODEL_MAX_SIZE (80 * 1024) //!< should adjust this value if
//!< hotword model size changed and should adjust the flash section size at
//!< the sametime., @see HOTWORD_SECTION_SIZE in common.mk

#ifndef OTA_FLASH_LOGIC_ADDR
#define OTA_FLASH_LOGIC_ADDR (FLASH_NC_BASE)
#endif

/************************private type defination****************************/

/************************extern function declearation***********************/
#ifdef GSOUND_HOTWORD_ENABLED
extern uint32_t __hotword_model_start[];
extern uint32_t __hotword_model_end[];
#endif

/**********************private function declearation************************/

/************************private variable defination************************/
#ifdef GSOUND_HOTWORD_ENABLED
#ifdef MODEL_FILE_EMBEDED
const uint8_t EMBEDED_MODEL_FILE[] = {
#include "res/gs_hw/en_all.txt" //!< must correspond to specific model ID, @see DEFAULT_HOTWORD_MODEL_ID
};

/// NOTE: Add more embeded model file info here

#define DEFAULT_MODEL_START_ADDR                                               \
  EMBEDED_MODEL_FILE //!< map to specific flash address within the bin file
#else                /// #ifndef MODEL_FILE_EMBEDED
#define DEFAULT_MODEL_START_ADDR                                               \
  __hotword_model_start //!< map to specific flash address out of the bin file
#endif                  /// #ifdef MODEL_FILE_EMBEDED

HOTWORD_MODEL_INFO_T defaultModel[DEFAULT_MODEL_NUM] = {
    {DEFAULT_HOTWORD_MODEL_ID, (uint32_t)DEFAULT_MODEL_START_ADDR,
     HOTWORD_MODEL_MAX_SIZE},
    /// NOTE: Add other embeded model info here
};

char supportedModels[GSOUND_MAX_SUPPORTED_HOTWORD_MODELS_BYTES] = {};
#endif /// #ifdef GSOUND_HOTWORD_ENABLED

static NV_GSOUND_INFO_T *nvrecord_gsound_p = NULL;

/****************************function defination****************************/
void nv_record_gsound_rec_init(void) {
  if (NULL == nvrecord_gsound_p) {
    nvrecord_gsound_p = &(nvrecord_extension_p->gsound_info);
  }

#ifdef GSOUND_HOTWORD_ENABLED
  /// init default supported model file(s)
  /// add to supported model file set(if it is not in this set)
  LOG_I("supportedModelCnt:%d", nvrecord_gsound_p->supportedModelCnt);
  if (0 == nvrecord_gsound_p
               ->supportedModelCnt) //!< load default info at first time bootup
  {
    for (uint8_t i = 0; i < DEFAULT_MODEL_NUM; i++) {
      LOG_I("hotword_model_start:0x%x", defaultModel[i].startAddr);

      nv_record_gsound_rec_add_new_model((void *)&defaultModel[i]);
    }
  }
#endif
}

void nv_record_gsound_rec_get_ptr(void **ptr) { *ptr = nvrecord_gsound_p; }

void nv_record_gsound_rec_updata_enable_state(bool enable) {
  LOG_I("gsound enable state update:%d->%d", nvrecord_gsound_p->isGsoundEnabled,
        enable);

  if (nvrecord_gsound_p->isGsoundEnabled != enable) {
    /// disable the MPU protection for write operation
    uint32_t lock = nv_record_pre_write_operation();

    /// update the enable state
    nvrecord_gsound_p->isGsoundEnabled = enable;
    nv_record_extension_update();

    /// enable the MPU protection after the write operation
    nv_record_post_write_operation(lock);
  }
}

#ifdef GSOUND_HOTWORD_ENABLED
bool nv_record_gsound_rec_is_model_insert_allowed(const char *model) {
  bool ret = true;
  uint8_t i;

  if ((nvrecord_gsound_p->supportedModelCnt >= HOTWORD_MODLE_MAX_NUM) &&
      (0xFF != nvrecord_gsound_p->supportedModelCnt)) {
    for (i = 0; i < HOTWORD_MODLE_MAX_NUM; i++) {
      /// hotword model exist already
      if (!memcmp(&nvrecord_gsound_p->modelInfo[i].modelId, model,
                  strlen(model))) {
        break;
      }
    }

    if (HOTWORD_MODLE_MAX_NUM == i) {
      ret = false;
    }
  }

  return ret;
}

bool nv_record_gsound_rec_add_new_model(void *pInfo) {
  bool ret = true;
  bool found = false;
  uint32_t lock = 0;

  HOTWORD_MODEL_INFO_T *info = (HOTWORD_MODEL_INFO_T *)pInfo;

  if (INVALID_MODEL_NUM == nvrecord_gsound_p->supportedModelCnt) {
    lock = nv_record_pre_write_operation();
    nvrecord_gsound_p->supportedModelCnt = 0;
    nv_record_post_write_operation(lock);
  }

  if (info) {
    lock = nv_record_pre_write_operation();
    info->startAddr |= OTA_FLASH_LOGIC_ADDR;

    for (uint8_t i = 0; i < nvrecord_gsound_p->supportedModelCnt; i++) {
      /// hotword model exist already
      if (!memcmp(&nvrecord_gsound_p->modelInfo[i].modelId, info->modelId,
                  GSOUND_HOTWORD_MODEL_ID_BYTES)) {
        LOG_I("info->startAddr:%x", info->startAddr);

        ///.update the start address
        if (info->startAddr) {
          nvrecord_gsound_p->modelInfo[i].startAddr = info->startAddr;
          nvrecord_gsound_p->modelInfo[i].len = info->len;
        }

        LOG_I("model already exist in user section, model_id:%s, model_addr:%x",
              nvrecord_gsound_p->modelInfo[i].modelId,
              nvrecord_gsound_p->modelInfo[i].startAddr);
        found = true;
        break;
      }
    }

    LOG_I("current supportedModelCnt:%d", nvrecord_gsound_p->supportedModelCnt);

    /// brand new model insert
    if (!found) {
      memcpy(
          &nvrecord_gsound_p->modelInfo[nvrecord_gsound_p->supportedModelCnt],
          info, sizeof(HOTWORD_MODEL_INFO_T));

      LOG_I("new added modelId:%s, startAddr:%x, modelFileLen:%x",
            info->modelId, info->startAddr, info->len);
      nvrecord_gsound_p->supportedModelCnt++;
    }
    nv_record_post_write_operation(lock);

    /// new model file incoming, update flash info
    if (!found) {
      /// flush the hotword info to flash
      nv_record_update_runtime_userdata();
      nv_record_flash_flush();
    }

    LOG_I("current supportedModelCnt:%d", nvrecord_gsound_p->supportedModelCnt);
  } else {
    LOG_W("NULL pointer received in %s", __func__);
    ret = false;
  }

  return ret;
}

uint32_t nv_record_gsound_rec_get_hotword_model_addr(const char *model_id,
                                                     bool add_new,
                                                     int32_t newModelLen) {
  uint32_t addr = 0;
  bool found = false;

  LOG_I("%s revceived model ID: %s", __func__, model_id);
  if (nvrecord_gsound_p) {
    for (uint8_t i = 0; i < ARRAY_SIZE(nvrecord_gsound_p->modelInfo); i++) {
      if (!memcmp(nvrecord_gsound_p->modelInfo[i].modelId, model_id, 4)) {
        addr = nvrecord_gsound_p->modelInfo[i].startAddr;
        found = true;
        LOG_I("found saved model info, index:%d", i);
        break;
      }
    }
  }

  do {
    if (add_new && !found) {
      addr = (uint32_t)__hotword_model_start;
      uint32_t tempAddr;
      uint8_t startIdx = 0;
#ifdef MODEL_FILE_EMBEDED
      startIdx =
          DEFAULT_MODEL_NUM; //!< set offset to skip the embeded model file info
#endif

      if (INVALID_MODEL_NUM == nvrecord_gsound_p->supportedModelCnt) {
        break;
      }

      /// get the largest flash address
      for (uint8_t i = startIdx; i < nvrecord_gsound_p->supportedModelCnt;
           i++) {
        tempAddr = nvrecord_gsound_p->modelInfo[i].startAddr +
                   nvrecord_gsound_p->modelInfo[i].len;
        if ((tempAddr & 0xFFFFFF) > (addr & 0xFFFFFF)) {
          addr = tempAddr;
        }
      }

      /// force 4K align the address
      addr = F_4K_ALIGN(addr);
      LOG_I("supportedModelCnt:%d, Largest address:0x%x",
            nvrecord_gsound_p->supportedModelCnt, addr);

      /// check is overwrite needed
      if ((newModelLen + addr) > (uint32_t)__hotword_model_end ||
          (nvrecord_gsound_p->supportedModelCnt >= HOTWORD_MODLE_MAX_NUM)) {
        uint8_t cleanIdx = 0; //!< model file index to clean
        addr = nvrecord_gsound_p->modelInfo[startIdx]
                   .startAddr; //!< init anchor point
        uint32_t cleanSize =
            F_4K_ALIGN(addr + nvrecord_gsound_p->modelInfo[startIdx].len) -
            addr; //!< flash size to clean

        uint32_t lock = nv_record_pre_write_operation(); //!< disable the MPU
                                                         //!< for info update
        uint32_t intLock = int_lock(); //!< lock the global interrupt

        /// remove the first record
        for (uint8_t i = startIdx;
             i < (nvrecord_gsound_p->supportedModelCnt - 1); i++) {
          nvrecord_gsound_p->modelInfo[i] = nvrecord_gsound_p->modelInfo[i + 1];
        }

        /// decrease the number of supported model
        nvrecord_gsound_p->supportedModelCnt--;

        while (cleanSize < newModelLen) {
          uint8_t headAdjacentIdx = 0xFF, tailAdjacentIdx = 0xFF;
          uint32_t headAdjacentOffset = 0xFFFFFFFF,
                   tailAdjacentOffset = 0xFFFFFFFF;

          /// find the adjacent chunck
          for (uint8_t i = startIdx; i < nvrecord_gsound_p->supportedModelCnt;
               i++) {
            if (nvrecord_gsound_p->modelInfo[i].startAddr > addr) {
              if (nvrecord_gsound_p->modelInfo[i].startAddr - addr <
                  tailAdjacentOffset) {
                tailAdjacentOffset =
                    nvrecord_gsound_p->modelInfo[i].startAddr - addr;
                tailAdjacentIdx = i;
              }
            } else if (nvrecord_gsound_p->modelInfo[i].startAddr < addr) {
              if (addr - nvrecord_gsound_p->modelInfo[i].startAddr <
                  headAdjacentOffset) {
                headAdjacentOffset =
                    nvrecord_gsound_p->modelInfo[i].startAddr - addr;
                headAdjacentIdx = i;
              }
            }
          }

          LOG_I("headAdjacentIdx:%d, tailAdjacentIdx:%d", headAdjacentIdx,
                tailAdjacentIdx);

          /// find the smaller index, smaller index means older record
          if (headAdjacentIdx > tailAdjacentIdx) {
            cleanSize +=
                F_4K_ALIGN(
                    nvrecord_gsound_p->modelInfo[tailAdjacentIdx].startAddr +
                    nvrecord_gsound_p->modelInfo[tailAdjacentIdx].len) -
                nvrecord_gsound_p->modelInfo[tailAdjacentIdx].startAddr;
            cleanIdx = tailAdjacentIdx;
          } else {
            addr = nvrecord_gsound_p->modelInfo[headAdjacentIdx].startAddr;
            cleanSize +=
                F_4K_ALIGN(addr +
                           nvrecord_gsound_p->modelInfo[headAdjacentIdx].len) -
                addr;
            cleanIdx = headAdjacentIdx;
          }

          /// remove the first record
          for (uint8_t i = cleanIdx;
               i < (nvrecord_gsound_p->supportedModelCnt - 1); i++) {
            nvrecord_gsound_p->modelInfo[i] =
                nvrecord_gsound_p->modelInfo[i + 1];
          }

          /// decrease the number of supported model
          nvrecord_gsound_p->supportedModelCnt--;
        }

        if ((addr < (uint32_t)__hotword_model_start) ||
            (addr > (uint32_t)__hotword_model_end)) {
          LOG_I("invalid addr found:0x%x, clean all model files", addr);
          nvrecord_gsound_p->supportedModelCnt = startIdx;
          addr = (uint32_t)__hotword_model_start;
        }

        if (0 == nvrecord_gsound_p->supportedModelCnt) {
          /// invalid number used to identify with fist time bootup
          nvrecord_gsound_p->supportedModelCnt = INVALID_MODEL_NUM;
        }

        int_unlock(intLock);                  //!< unlock the global interrupt
        nv_record_post_write_operation(lock); //!< enable the MPU

        /// flush the hotword info to flash
        nv_record_update_runtime_userdata();
        nv_record_flash_flush();
      }
    }
  } while (0);

  return addr;
}

int nv_record_gsound_rec_get_supported_model_id(char *models_out,
                                                uint8_t *length_out) {
  int ret = 0;
  const char *terminator = NULL;

  if (INVALID_MODEL_NUM == nvrecord_gsound_p->supportedModelCnt) {
    memcpy(models_out, STRING_TERMINATOR_NULL, STRING_TERMINATOR_SIZE);
    *length_out = STRING_TERMINATOR_SIZE;
  } else {
    for (uint8_t i = 0; i < nvrecord_gsound_p->supportedModelCnt; i++) {
      LOG_I("supported model_id:%s, model_addr:%x",
            nvrecord_gsound_p->modelInfo[i].modelId,
            nvrecord_gsound_p->modelInfo[i].startAddr);

      memcpy((supportedModels + i * GSOUND_HOTWORD_SUPPORTED_MODEL_ID_BYTES),
             nvrecord_gsound_p->modelInfo[i].modelId,
             GSOUND_HOTWORD_MODEL_ID_BYTES);

      if ((i + 1) == nvrecord_gsound_p->supportedModelCnt) {
        terminator = STRING_TERMINATOR_NULL;
      } else {
        terminator = SUPPORTED_HOTWORD_MODEL_DELIM;
      }
      memcpy(&supportedModels[(i * GSOUND_HOTWORD_SUPPORTED_MODEL_ID_BYTES) +
                              GSOUND_HOTWORD_MODEL_ID_BYTES],
             terminator, STRING_TERMINATOR_SIZE);
    }

    if (nvrecord_gsound_p->supportedModelCnt *
            GSOUND_HOTWORD_SUPPORTED_MODEL_ID_BYTES <=
        *length_out) {
      strcpy(models_out, supportedModels);
      *length_out = strlen(supportedModels) + STRING_TERMINATOR_SIZE;
      LOG_I("total supported model_id(length is %d):", *length_out);
      DUMP8("0x%02x ", models_out, *length_out);
    } else {
      ret = -1;
      LOG_W("supported model id length exceeded max length");
    }
  }

  return ret;
}
#endif
