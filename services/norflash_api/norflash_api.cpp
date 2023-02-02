#include "cmsis.h"
#include "stdio.h"
#include "string.h"
#ifdef RTOS
#include "cmsis_os.h"
#else
#include "hal_timer.h"
#endif
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_trace.h"
#include "norflash_api.h"
#include "pmu.h"

#if 0
#define NORFLASH_API_TRACE(str, ...) TRACE(str, ##__VA_ARGS__)
#else
#define NORFLASH_API_TRACE(str, ...)
#endif

#define API_IS_ALIGN(v, size) (((v / size) * size) == v)

static NORFLASH_API_INFO norflash_api_info = {
    false,
};
static OPERA_INFO_LIST opera_info_list[NORFLASH_API_OPRA_LIST_LEN];
static DATA_LIST data_list[NORFLASH_API_WRITE_BUFF_LEN];
static int suspend_number = 0;

static void *_norflash_api_malloc(uint32_t size) {
  uint32_t i;

  if (size == sizeof(OPRA_INFO)) {
    for (i = 0; i < NORFLASH_API_OPRA_LIST_LEN; i++) {
      if (opera_info_list[i].is_used == false) {
        opera_info_list[i].is_used = true;
        return (void *)&opera_info_list[i].opera_info;
      }
    }
    return NULL;
  } else if (size == NORFLASH_API_SECTOR_SIZE) {
    for (i = 0; i < NORFLASH_API_WRITE_BUFF_LEN; i++) {
      if (data_list[i].is_used == false) {
        data_list[i].is_used = true;
        return (void *)data_list[i].buffer;
      }
    }
    return NULL;
  } else {
    ASSERT(0, "%s: size(0x%x) error!", __func__, size);
  }
}

static void _norflash_api_free(void *p) {
  uint32_t i;

  for (i = 0; i < NORFLASH_API_OPRA_LIST_LEN; i++) {
    if ((uint8_t *)&opera_info_list[i].opera_info == p) {
      opera_info_list[i].is_used = false;
      return;
    }
  }

  for (i = 0; i < NORFLASH_API_WRITE_BUFF_LEN; i++) {
    if (data_list[i].buffer == p) {
      data_list[i].is_used = false;
      return;
    }
  }

  ASSERT(0, "%s: p(%p) error!", __func__, p);
}

static MODULE_INFO *_get_module_info(enum NORFLASH_API_MODULE_ID_T mod_id) {
  return &norflash_api_info.mod_info[mod_id];
}

static OPRA_INFO *_get_tail(MODULE_INFO *mod_info, bool is_remove) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *pre_node = NULL;
  OPRA_INFO *tmp;

  pre_node = mod_info->opera_info;
  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    tmp = opera_node->next;
    if (tmp) {
      pre_node = opera_node;
    }
  }
  if (is_remove) {
    if (pre_node) {
      pre_node->next = NULL;
    }
  }
  if (opera_node) {
    opera_node->lock = true;
  }
  return opera_node;
}

static void _opera_del(MODULE_INFO *mod_info, OPRA_INFO *node) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *pre_node = NULL;
  OPRA_INFO *tmp;

  pre_node = mod_info->opera_info;
  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    if (opera_node == node) {
      if (mod_info->opera_info == opera_node) {
        mod_info->opera_info = NULL;
      } else {
        pre_node->next = NULL;
      }
      if (node->buff) {
        _norflash_api_free(node->buff);
      }
      _norflash_api_free(node);
      break;
    }
    tmp = opera_node->next;
    if (tmp) {
      pre_node = opera_node;
    }
  }
}

static uint32_t _get_ew_count(MODULE_INFO *mod_info) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *tmp;
  uint32_t count = 0;

  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    count++;
    tmp = opera_node->next;
  }
  return count;
}

static uint32_t _get_w_count(MODULE_INFO *mod_info) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *tmp;
  uint32_t count = 0;

  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    if (opera_node->type == NORFLASH_API_WRITTING) {
      count++;
    }
    tmp = opera_node->next;
  }
  return count;
}

static uint32_t _get_e_count(MODULE_INFO *mod_info) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *tmp;
  uint32_t count = 0;

  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    if (opera_node->type == NORFLASH_API_ERASING) {
      count++;
    }
    tmp = opera_node->next;
  }
  return count;
}

static MODULE_INFO *_get_cur_mod(void) {
  uint32_t i;
  MODULE_INFO *mod_info = NULL;
  uint32_t tmp_mod_id = NORFLASH_API_MODULE_ID_COUNT;

  if (norflash_api_info.cur_mod) {
    return norflash_api_info.cur_mod;
  }

  tmp_mod_id = norflash_api_info.cur_mod_id;
  for (i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++) {
    tmp_mod_id =
        tmp_mod_id + 1 >= NORFLASH_API_MODULE_ID_COUNT ? 0 : tmp_mod_id + 1;
    mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)tmp_mod_id);
    if (mod_info->is_inited) {
      if (_get_ew_count(mod_info) > 0) {
        return mod_info;
      }
    }
  }
  return NULL;
}

static enum NORFLASH_API_MODULE_ID_T _get_mod_id(MODULE_INFO *mod_info) {
  uint32_t i;
  enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
  MODULE_INFO *tmp_mod_info = NULL;

  for (i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++) {
    tmp_mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
    if (tmp_mod_info == mod_info) {
      mod_id = (enum NORFLASH_API_MODULE_ID_T)i;
      break;
    }
  }
  return mod_id;
}

#ifdef FLASH_REMAP
static void _flash_remap_start(enum HAL_NORFLASH_ID_T id, uint32_t addr,
                               uint32_t len) {
  uint32_t remap_addr;
  uint32_t remap_len;
  uint32_t remap_offset;
  enum HAL_NORFLASH_RET_T ret;

  remap_addr = OTA_CODE_OFFSET;
  remap_len = OTA_REMAP_OFFSET - OTA_CODE_OFFSET;
  remap_offset = OTA_REMAP_OFFSET;

  // NORFLASH_API_TRACE(3,"%s: id = %d,addr = 0x%x,len = 0x%x.",
  // __func__,id,addr,len);
  if ((addr & 0x3ffffff) + len <= (remap_addr & 0x3ffffff) ||
      (addr & 0x3ffffff) >= (remap_addr & 0x3ffffff) + remap_len) {
    // NORFLASH_API_TRACE(3,"%s: Not in the remap area.",__func__);
    return;
  }

  if (((addr & 0x3ffffff) < (remap_addr & 0x3ffffff) &&
       (addr & 0x3ffffff) + len > (remap_addr & 0x3ffffff)) ||
      ((addr & 0x3ffffff) < (remap_addr & 0x3ffffff) + remap_len &&
       (addr & 0x3ffffff) + len > (remap_addr & 0x3ffffff) + remap_len)) {
    ASSERT(0,
           "%s: Address ranges bad!addr = 0x%x, "
           "len=0x%x,remap_addr=0x%x,remap_len=0x%x",
           __func__, addr, len, remap_addr, remap_len);
  }

  if (!hal_norflash_get_remap_status(id)) {
    // NORFLASH_API_TRACE(3,"%s: Unremap to enable remap.",__func__);
    ret = hal_norflash_enable_remap(id, remap_addr, remap_len, remap_offset);
    ASSERT(ret == HAL_NORFLASH_OK,
           "%s: Failed to enable remap(0x%x,0x%x,0x%x), ret = %d", __func__,
           remap_addr, remap_len, remap_offset, ret);
  } else {
    // NORFLASH_API_TRACE(3,"%s: Remaped to disable remap.",__func__);
    ret = hal_norflash_disable_remap(id);
    ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to disable remap, ret = %d",
           __func__, ret);
  }
}

static void _flash_remap_done(enum HAL_NORFLASH_ID_T id, uint32_t addr,
                              uint32_t len) {
  uint32_t remap_addr;
  uint32_t remap_len;
  uint32_t remap_offset;
  enum HAL_NORFLASH_RET_T ret;

  remap_addr = OTA_CODE_OFFSET;
  remap_len = OTA_REMAP_OFFSET - OTA_CODE_OFFSET;
  remap_offset = OTA_REMAP_OFFSET;

  // NORFLASH_API_TRACE(3, "%s: id = %d,addr = 0x%x,len = 0x%x.",
  // __func__,id,addr,len);
  if ((addr & 0x3ffffff) + len <= (remap_addr & 0x3ffffff) ||
      (addr & 0x3ffffff) >= (remap_addr & 0x3ffffff) + remap_len) {
    // NORFLASH_API_TRACE(3,"%s: Not in the remap area.",__func__);
    return;
  }

  if (((addr & 0x3ffffff) < (remap_addr & 0x3ffffff) &&
       (addr & 0x3ffffff) + len > (remap_addr & 0x3ffffff)) ||
      ((addr & 0x3ffffff) < (remap_addr & 0x3ffffff) + remap_len &&
       (addr & 0x3ffffff) + len > (remap_addr & 0x3ffffff) + remap_len)) {
    ASSERT(0,
           "%s: Address ranges bad!addr = 0x%x, "
           "len=0x%x,remap_addr=0x%x,remap_len=0x%x",
           __func__, addr, len, remap_addr, remap_len);
  }

  if (!hal_norflash_get_remap_status(id)) {
    // NORFLASH_API_TRACE(3, "%s: Unremap to enable remap.",__func__);
    ret = hal_norflash_enable_remap(id, remap_addr, remap_len, remap_offset);
    ASSERT(ret == HAL_NORFLASH_OK,
           "%s: Failed to enable remap(0x%x,0x%x,0x%x), ret = %d", __func__,
           remap_addr, remap_len, remap_offset, ret);
  } else {
    // NORFLASH_API_TRACE(3, "%s: Remaped to disable remap.",__func__);
    ret = hal_norflash_disable_remap(id);
    ASSERT(ret == HAL_NORFLASH_OK, "%s: Failed to disable remap, ret = %d",
           __func__, ret);
  }
}

#define FLASH_REMAP_START _flash_remap_start
#define FLASH_REMAP_DONE _flash_remap_done
#else
#define FLASH_REMAP_START(...)
#define FLASH_REMAP_DONE(...)
#endif

static int32_t _opera_read(MODULE_INFO *mod_info, uint32_t addr, uint8_t *buff,
                           uint32_t len) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *e_node = NULL;
  OPRA_INFO *w_node = NULL;
  OPRA_INFO *tmp;
  uint32_t r_offs;
  uint32_t sec_start;
  uint32_t sec_len;

  sec_len = mod_info->mod_sector_len;
  sec_start = (addr / sec_len) * sec_len;
  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    tmp = opera_node->next;
    if (opera_node->addr == sec_start) {
      if (opera_node->type == NORFLASH_API_WRITTING) {
        w_node = opera_node;
        break;
      } else {
        e_node = opera_node;
        break;
      }
    }
  }

  if (w_node) {
    r_offs = addr - sec_start;
    memcpy(buff, w_node->buff + r_offs, len);
  } else {
    if (e_node) {
      memset(buff, 0xff, len);
    } else {
      FLASH_REMAP_START(mod_info->dev_id, addr, len);
      memcpy(buff, (uint8_t *)addr, len);
      FLASH_REMAP_DONE(mod_info->dev_id, addr, len);
      /*
      HAL_NORFLASH_RET_T result;
      result = hal_norflash_read(mod_info->dev_id,addr,buff,len);
      if(result != HAL_NORFLASH_OK)
      {
          NORFLASH_API_TRACE(2,"%s: hal_norflash_read failed,result = %d.",
                  __func__,result);
          return result;
      }
      */
    }
  }
  return 0;
}

static int32_t _e_opera_add(MODULE_INFO *mod_info, uint32_t addr,
                            uint32_t len) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *pre_node = NULL;
  OPRA_INFO *tmp;
  int32_t ret = 0;

  // delete opera nodes with the same address when add the erase opera node.
  pre_node = mod_info->opera_info;
  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    tmp = opera_node->next;

    if (opera_node->addr == addr) {
      if (opera_node->lock == false) {
        if (opera_node == mod_info->opera_info) {
          mod_info->opera_info = tmp;
        } else {
          pre_node->next = tmp;
        }
        if (opera_node->type == NORFLASH_API_WRITTING) {
          if (opera_node->buff) {
            _norflash_api_free(opera_node->buff);
          }
        }
        _norflash_api_free(opera_node);
      } else {
        if (opera_node->type == NORFLASH_API_ERASING) {
          NORFLASH_API_TRACE(3, "%s: erase is merged! addr = 0x%x,len = 0x%x.",
                             __func__, opera_node->addr, opera_node->len);
          ret = 0;
          goto _func_end;
        }
      }
    }

    if (tmp) {
      pre_node = opera_node;
    }
  }

  // add new node to header.
  opera_node = (OPRA_INFO *)_norflash_api_malloc(sizeof(OPRA_INFO));
  if (opera_node == NULL) {
    NORFLASH_API_TRACE(3, "%s:%d,_norflash_api_malloc failed! size = %d.",
                       __func__, __LINE__, sizeof(OPRA_INFO));
    ret = 1;
    goto _func_end;
  }
  opera_node->type = NORFLASH_API_ERASING;
  opera_node->addr = addr;
  opera_node->len = len;
  opera_node->w_offs = 0;
  opera_node->w_len = 0;
  opera_node->buff = NULL;
  opera_node->lock = false;
  opera_node->next = mod_info->opera_info;
  mod_info->opera_info = opera_node;
  ret = 0;
_func_end:

  return ret;
}

static int32_t _w_opera_add(MODULE_INFO *mod_info, uint32_t addr, uint32_t len,
                            uint8_t *buff) {
  OPRA_INFO *opera_node = NULL;
  OPRA_INFO *e_node = NULL;
  OPRA_INFO *w_node = NULL;
  OPRA_INFO *tmp;
  uint32_t w_offs;
  uint32_t w_len;
  uint32_t sec_start;
  uint32_t sec_len;
  uint32_t w_end1;
  uint32_t w_end2;
  uint32_t w_start;
  uint32_t w_end;
  uint32_t w_len_new;
  int32_t ret = 0;

  sec_len = mod_info->mod_sector_len;
  sec_start = (addr / sec_len) * sec_len;
  w_offs = addr - sec_start;
  w_len = len;
  tmp = mod_info->opera_info;
  while (tmp) {
    opera_node = tmp;
    tmp = opera_node->next;

    if (opera_node->addr == sec_start) {
      if (opera_node->type == NORFLASH_API_WRITTING) {
        if (!opera_node->lock) {
          // select the first w_node in the list.
          w_node = opera_node;
          break;
        }
      } else {
        e_node = opera_node;
        break;
      }
    }
  }

  if (w_node) {
    memcpy(w_node->buff + w_offs, buff, w_len);
    w_start = w_node->w_offs <= w_offs ? w_node->w_offs : w_offs;
    w_end1 = w_node->w_offs + w_node->w_len;
    w_end2 = w_offs + w_len;
    w_end = w_end1 >= w_end2 ? w_end1 : w_end2;
    w_len_new = w_end - w_start;
    w_node->w_offs = w_start;
    w_node->w_len = w_len_new;
    opera_node = w_node;
    ret = 0;
  } else {
    opera_node = (OPRA_INFO *)_norflash_api_malloc(sizeof(OPRA_INFO));
    if (opera_node == NULL) {
      NORFLASH_API_TRACE(3, "%s:%d,_norflash_api_malloc failed! size = %d.",
                         __func__, __LINE__, sizeof(OPRA_INFO));
      ret = 1;
      goto _func_end;
    }
    opera_node->type = NORFLASH_API_WRITTING;
    opera_node->addr = sec_start;
    opera_node->len = sec_len;
    opera_node->w_offs = w_offs;
    opera_node->w_len = w_len;
    opera_node->buff = (uint8_t *)_norflash_api_malloc(opera_node->len);
    if (opera_node->buff == NULL) {
      _norflash_api_free(opera_node);
      NORFLASH_API_TRACE(3, "%s:%d,_norflash_api_malloc failed! size = %d.",
                         __func__, __LINE__, opera_node->len);
      ret = 1;
      goto _func_end;
    }
    if (e_node) {
      memset(opera_node->buff, 0xff, opera_node->len);
    } else {
      memcpy(opera_node->buff, (uint8_t *)opera_node->addr, opera_node->len);
    }
    memcpy(opera_node->buff + w_offs, buff, w_len);
    opera_node->lock = false;
    opera_node->next = mod_info->opera_info;
    mod_info->opera_info = opera_node;
    ret = 0;
  }

_func_end:
  return ret;
}

bool _opera_flush(MODULE_INFO *mod_info, bool nosuspend) {
  OPRA_INFO *cur_opera_info = NULL;
  enum HAL_NORFLASH_RET_T result;
  bool opera_is_completed = false;
  NORFLASH_API_OPERA_RESULT opera_result;
  bool ret = false;
  bool suspend;

#if defined(FLASH_SUSPEND)
  suspend = true;
#else
  suspend = false;
#endif
  suspend = nosuspend == true ? false : suspend;

  if (!mod_info->cur_opera_info) {
    mod_info->cur_opera_info = _get_tail(mod_info, false);
  }

  if (!mod_info->cur_opera_info) {
    return false;
  }

  ret = true;
  cur_opera_info = mod_info->cur_opera_info;
  if (cur_opera_info->type == NORFLASH_API_WRITTING) {
    if (mod_info->state == NORFLASH_API_STATE_IDLE) {
      suspend_number = 0;
      if (cur_opera_info->w_len > 0) {
        NORFLASH_API_TRACE(5,
                           "%s: %d,hal_norflash_write_suspend,addr = 0x%x,len "
                           "= 0x%x,suspend = %d.",
                           __func__, __LINE__,
                           cur_opera_info->addr + cur_opera_info->w_offs,
                           cur_opera_info->w_len, suspend);

        FLASH_REMAP_START(mod_info->dev_id,
                          cur_opera_info->addr + cur_opera_info->w_offs,
                          cur_opera_info->w_len);
        pmu_flash_write_config();
        result = hal_norflash_write_suspend(
            mod_info->dev_id, cur_opera_info->addr + cur_opera_info->w_offs,
            cur_opera_info->buff + cur_opera_info->w_offs,
            cur_opera_info->w_len, suspend);
        pmu_flash_read_config();
        FLASH_REMAP_DONE(mod_info->dev_id,
                         cur_opera_info->addr + cur_opera_info->w_offs,
                         cur_opera_info->w_len);
      } else {
        result = HAL_NORFLASH_OK;
      }

      if (result == HAL_NORFLASH_OK) {
        opera_is_completed = true;
        goto __opera_is_completed;
      } else if (result == HAL_NORFLASH_SUSPENDED) {
        mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
      } else {
        ASSERT(0, "%s: %d, hal_norflash_write_suspend failed,result = %d",
               __func__, __LINE__, result);
      }
    } else if (mod_info->state == NORFLASH_API_STATE_WRITTING_SUSPEND) {
      suspend_number++;
      FLASH_REMAP_START(mod_info->dev_id,
                        cur_opera_info->addr + cur_opera_info->w_offs,
                        cur_opera_info->w_len);
      pmu_flash_write_config();
      result = hal_norflash_write_resume(mod_info->dev_id, suspend);
      pmu_flash_read_config();
      FLASH_REMAP_DONE(mod_info->dev_id,
                       cur_opera_info->addr + cur_opera_info->w_offs,
                       cur_opera_info->w_len);
      if (result == HAL_NORFLASH_OK) {
        opera_is_completed = true;
        goto __opera_is_completed;
      } else if (result == HAL_NORFLASH_SUSPENDED) {
        mod_info->state = NORFLASH_API_STATE_WRITTING_SUSPEND;
      } else {
        ASSERT(0, "%s: %d, hal_norflash_write_resume failed,result = %d",
               __func__, __LINE__, result);
      }
    } else {
      ASSERT(0, "%s: %d, mod_info->state error,state = %d", __func__, __LINE__,
             mod_info->state);
    }
  } else {
    if (mod_info->state == NORFLASH_API_STATE_IDLE) {
      suspend_number = 0;
      NORFLASH_API_TRACE(5,
                         "%s: %d,hal_norflash_erase_suspend,addr = 0x%x,len = "
                         "0x%x,suspend = %d.",
                         __func__, __LINE__, cur_opera_info->addr,
                         cur_opera_info->len, suspend);
      FLASH_REMAP_START(mod_info->dev_id, cur_opera_info->addr,
                        cur_opera_info->len);
      pmu_flash_write_config();
      result = hal_norflash_erase_suspend(
          mod_info->dev_id, cur_opera_info->addr, cur_opera_info->len, suspend);
      pmu_flash_read_config();
      FLASH_REMAP_DONE(mod_info->dev_id, cur_opera_info->addr,
                       cur_opera_info->len);
      if (result == HAL_NORFLASH_OK) {
        opera_is_completed = true;
        goto __opera_is_completed;
      } else if (result == HAL_NORFLASH_SUSPENDED) {
        mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
      } else {
        ASSERT(0, "%s: %d, hal_norflash_erase_suspend failed,result = %d",
               __func__, __LINE__, result);
      }
    } else if (mod_info->state == NORFLASH_API_STATE_ERASE_SUSPEND) {
      suspend_number++;
      FLASH_REMAP_START(mod_info->dev_id, cur_opera_info->addr,
                        cur_opera_info->len);
      pmu_flash_write_config();
      result = hal_norflash_erase_resume(mod_info->dev_id, suspend);
      pmu_flash_read_config();
      FLASH_REMAP_DONE(mod_info->dev_id, cur_opera_info->addr,
                       cur_opera_info->len);
      if (result == HAL_NORFLASH_OK) {
        opera_is_completed = true;
        goto __opera_is_completed;
      } else if (result == HAL_NORFLASH_SUSPENDED) {
        mod_info->state = NORFLASH_API_STATE_ERASE_SUSPEND;
      } else {
        ASSERT(0, "%s: %d, hal_norflash_write_resume failed,result = %d",
               __func__, __LINE__, result);
      }
    } else {
      ASSERT(0, "%s: %d, mod_info->state error,state = %d", __func__, __LINE__,
             mod_info->state);
    }
  }

__opera_is_completed:

  if (opera_is_completed) {
    mod_info->state = NORFLASH_API_STATE_IDLE;
    if (!nosuspend && mod_info->cb_func &&
        ((cur_opera_info->w_len > 0 &&
          cur_opera_info->type == NORFLASH_API_WRITTING) ||
         (cur_opera_info->len > 0 &&
          cur_opera_info->type == NORFLASH_API_ERASING))) {
      NORFLASH_API_TRACE(6,
                         "%s: w/e done.type = %d,addr = 0x%x,w_len = 0x%x,len "
                         "= 0x%x,suspend_num = %d.",
                         __func__, cur_opera_info->type,
                         cur_opera_info->addr + cur_opera_info->w_offs,
                         cur_opera_info->w_len, cur_opera_info->len,
                         suspend_number);
      if (cur_opera_info->type == NORFLASH_API_WRITTING) {
        opera_result.addr = cur_opera_info->addr + cur_opera_info->w_offs;
        opera_result.len = cur_opera_info->w_len;
      } else {
        opera_result.addr = cur_opera_info->addr;
        opera_result.len = cur_opera_info->len;
      }
      opera_result.type = cur_opera_info->type;
      opera_result.result = NORFLASH_API_OK;
      opera_result.remain_num = _get_ew_count(mod_info) - 1;
      mod_info->cb_func(&opera_result);
    }
    _opera_del(mod_info, cur_opera_info);
    mod_info->cur_opera_info = NULL;
  }

  return ret;
}

void _flush_disable(enum NORFLASH_API_USER user_id, uint32_t cb) {
  norflash_api_info.allowed_cb[user_id] = (NOFLASH_API_FLUSH_ALLOWED_CB)cb;
}

void _flush_enable(enum NORFLASH_API_USER user_id) {
  norflash_api_info.allowed_cb[user_id] = NULL;
}

bool _flush_is_allowed(void) {
  bool ret = true;
  uint32_t user_id;

  for (user_id = NORFLASH_API_USER_CP; user_id < NORFLASH_API_USER_COUNTS;
       user_id++) {
    if (norflash_api_info.allowed_cb[user_id]) {
      if (!norflash_api_info.allowed_cb[user_id]()) {
        ret = false;
      } else {
        norflash_api_info.allowed_cb[user_id] = NULL;
      }
    }
  }
  return ret;
}

//-------------------------------------------------------------------
// APIS Function.
//-------------------------------------------------------------------
enum NORFLASH_API_RET_T norflash_api_init(void) {
  uint32_t i;

  memset((void *)&norflash_api_info, 0, sizeof(NORFLASH_API_INFO));
  norflash_api_info.cur_mod_id = NORFLASH_API_MODULE_ID_COUNT;
  norflash_api_info.is_inited = true;
  norflash_api_info.cur_mod = NULL;
  for (i = 0; i < NORFLASH_API_MODULE_ID_COUNT; i++) {
    norflash_api_info.mod_info[i].state = NORFLASH_API_STATE_UNINITED;
  }

#if !defined(FLASH_API_SIMPLE)
#if defined(FLASH_SUSPEND)
  hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_NORFLASH_API, norflash_api_flush);
#else
  hal_sleep_set_deep_sleep_hook(HAL_DEEP_SLEEP_HOOK_NORFLASH_API,
                                norflash_api_flush);
#endif
#endif

  return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T
norflash_api_register(enum NORFLASH_API_MODULE_ID_T mod_id,
                      enum HAL_NORFLASH_ID_T dev_id, uint32_t mod_base_addr,
                      uint32_t mod_len, uint32_t mod_block_len,
                      uint32_t mod_sector_len, uint32_t mod_page_len,
                      uint32_t buffer_len, NORFLASH_API_OPERA_CB cb_func) {
  MODULE_INFO *mod_info = NULL;

  NORFLASH_API_TRACE(
      5, "%s: mod_id = %d,dev_id = %d,mod_base_addr = 0x%x,mod_len = 0x%x,",
      __func__, mod_id, dev_id, mod_base_addr, mod_len);
  NORFLASH_API_TRACE(4,
                     "mod_block_len = 0x%x,mod_sector_len = 0x%x,mod_page_len "
                     "= 0x%x,buffer_len = 0x%x.",
                     mod_block_len, mod_sector_len, mod_page_len, buffer_len);
  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(2, "%s: %d, norflash_api uninit!", __func__, __LINE__);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    NORFLASH_API_TRACE(2, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
    return NORFLASH_API_BAD_MOD_ID;
  }

  if (dev_id >= HAL_NORFLASH_ID_NUM) {
    NORFLASH_API_TRACE(2, "%s : dev_id error! mod_id = %d.", __func__, dev_id);
    return NORFLASH_API_BAD_DEV_ID;
  }

  if (buffer_len < mod_sector_len ||
      !API_IS_ALIGN(buffer_len, mod_sector_len)) {
    NORFLASH_API_TRACE(2, "%s : buffer_len error buffer_len = %d.", __func__,
                       buffer_len);
    return NORFLASH_API_BAD_BUFF_LEN;
  }
  mod_info = _get_module_info(mod_id);
  if (mod_info->is_inited) {
    // NORFLASH_API_TRACE(3,"%s: %d, norflash_async[%d] has
    // registered!",__func__,__LINE__,mod_id);
    return NORFLASH_API_OK; // NORFLASH_API_ERR_HASINIT;
  }
  mod_info->dev_id = dev_id;
  mod_info->mod_id = mod_id;
  mod_info->mod_base_addr = mod_base_addr;
  mod_info->mod_len = mod_len;
  mod_info->mod_block_len = mod_block_len;
  mod_info->mod_sector_len = mod_sector_len;
  mod_info->mod_page_len = mod_page_len;
  mod_info->buff_len = buffer_len;
  mod_info->cb_func = cb_func;
  mod_info->opera_info = NULL;
  mod_info->cur_opera_info = NULL;
  mod_info->state = NORFLASH_API_STATE_IDLE;
  mod_info->is_inited = true;
  return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_read(enum NORFLASH_API_MODULE_ID_T mod_id,
                                          uint32_t start_addr, uint8_t *buffer,
                                          uint32_t len) {
  MODULE_INFO *mod_info = NULL;
  int32_t result;
  enum NORFLASH_API_RET_T ret;
  uint32_t lock;

  NORFLASH_API_TRACE(4, "%s:mod_id = %d,start_addr = 0x%x,len = 0x%x", __func__,
                     mod_id, start_addr, len);
  ASSERT(buffer, "%s:buffer is null! ", __func__);
  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(1, "%s: norflash_api uninit!", __func__);
    return NORFLASH_API_ERR_UNINIT;
  }
  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    NORFLASH_API_TRACE(2, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
    return NORFLASH_API_BAD_MOD_ID;
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    NORFLASH_API_TRACE(2, "%s : module unregistered! mod_id = %d.", __func__,
                       mod_id);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (start_addr < mod_info->mod_base_addr ||
      start_addr + len > mod_info->mod_base_addr + mod_info->mod_len) {
    NORFLASH_API_TRACE(
        3, "%s : reading out of range! start_address = 0x%x,len = 0x%x.",
        __func__, start_addr, len);
    return NORFLASH_API_BAD_ADDR;
  }

  if (len == 0) {
    NORFLASH_API_TRACE(2, "%s : len error! len = %d.", __func__, len);
    return NORFLASH_API_BAD_LEN;
  }
  lock = int_lock_global();
  result = _opera_read(mod_info, start_addr, (uint8_t *)buffer, len);

  if (result) {
    ret = NORFLASH_API_ERR;
  } else {
    ret = NORFLASH_API_OK;
  }
  int_unlock_global(lock);
  NORFLASH_API_TRACE(2, "%s: done. ret = %d.", __func__, ret);
  return ret;
}

enum NORFLASH_API_RET_T norflash_sync_read(enum NORFLASH_API_MODULE_ID_T mod_id,
                                           uint32_t start_addr, uint8_t *buffer,
                                           uint32_t len) {
  MODULE_INFO *mod_info = NULL;
  uint32_t lock;

  NORFLASH_API_TRACE(4, "%s:mod_id = %d,start_addr = 0x%x,len = 0x%x", __func__,
                     mod_id, start_addr, len);
  ASSERT(buffer, "%s:%d,buffer is null! ", __func__, __LINE__);
  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(1, "%s: norflash_api uninit!", __func__);
    return NORFLASH_API_ERR_UNINIT;
  }
  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    NORFLASH_API_TRACE(2, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
    return NORFLASH_API_BAD_MOD_ID;
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    NORFLASH_API_TRACE(2, "%s : module unregistered! mod_id = %d.", __func__,
                       mod_id);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (start_addr < mod_info->mod_base_addr ||
      start_addr + len > mod_info->mod_base_addr + mod_info->mod_len) {
    NORFLASH_API_TRACE(
        3, "%s : reading out of range! start_address = 0x%x,len = 0x%x.",
        __func__, start_addr, len);
    return NORFLASH_API_BAD_ADDR;
  }

  if (len == 0) {
    NORFLASH_API_TRACE(2, "%s : len error! len = %d.", __func__, len);
    return NORFLASH_API_BAD_LEN;
  }

  lock = int_lock_global();
  FLASH_REMAP_START(mod_info->dev_id, start_addr, len);
  memcpy(buffer, (uint8_t *)start_addr, len);
  FLASH_REMAP_DONE(mod_info->dev_id, start_addr, len);
  NORFLASH_API_TRACE(1, "%s: done.", __func__);
  int_unlock_global(lock);

  return NORFLASH_API_OK;
}

enum NORFLASH_API_RET_T norflash_api_erase(enum NORFLASH_API_MODULE_ID_T mod_id,
                                           uint32_t start_addr, uint32_t len,
                                           bool async) {
  MODULE_INFO *mod_info = NULL;
  MODULE_INFO *cur_mod_info = NULL;
  uint32_t lock;
  int32_t result;
  bool bresult = false;
  enum NORFLASH_API_RET_T ret;

  NORFLASH_API_TRACE(5,
                     "%s: mod_id = %d,start_addr = 0x%x,len = 0x%x,async = %d.",
                     __func__, mod_id, start_addr, len, async);
  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(1, "%s: norflash_api uninit!", __func__);
    return NORFLASH_API_ERR_UNINIT;
  }
  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    NORFLASH_API_TRACE(2, "%s : invalid mod_id! mod_id = %d.", __func__,
                       mod_id);
    return NORFLASH_API_BAD_MOD_ID;
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    NORFLASH_API_TRACE(2, "%s : module unregistered! mod_id = %d.", __func__,
                       mod_id);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (start_addr < mod_info->mod_base_addr ||
      start_addr + len > mod_info->mod_base_addr + mod_info->mod_len) {
    NORFLASH_API_TRACE(
        3, "%s : erase out of range! start_address = 0x%x,len = 0x%x.",
        __func__, start_addr, len);
    return NORFLASH_API_BAD_ADDR;
  }

  if (!API_IS_ALIGN(start_addr, mod_info->mod_sector_len)) {
    NORFLASH_API_TRACE(2,
                       "%s : start_address no alignment! start_address = %d.",
                       __func__, start_addr);
    return NORFLASH_API_BAD_ADDR;
  }

  if (
#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
      len != mod_info->mod_page_len &&
#endif
      len != mod_info->mod_sector_len && len != mod_info->mod_block_len) {
    NORFLASH_API_TRACE(2, "%s : len error. len = %d!", __func__, len);
    return NORFLASH_API_BAD_LEN;
  }

#if defined(FLASH_API_SIMPLE)
  async = false;
#endif
  if (async) {
    lock = int_lock_global();

    // add to opera_info chain header.
    result = _e_opera_add(mod_info, start_addr, len);
    if (result == 0) {
      ret = NORFLASH_API_OK;
    } else {
      ret = NORFLASH_API_BUFFER_FULL;
    }
    int_unlock_global(lock);
    NORFLASH_API_TRACE(
        4, "%s: _e_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, start_addr, len, ret);
  } else {
    lock = int_lock_global();
    // Handle the suspend operation.
    if (norflash_api_info.cur_mod != NULL &&
        mod_info != norflash_api_info.cur_mod) {
      cur_mod_info = norflash_api_info.cur_mod;
      while (cur_mod_info->state != NORFLASH_API_STATE_IDLE) {
        if (!_flush_is_allowed()) {
          continue;
        }
        bresult = _opera_flush(cur_mod_info, true);
        if (!bresult) {
          norflash_api_info.cur_mod = NULL;
        }
      }
    }

    // flush all of cur module opera.
    // norflash_api_info.cur_mod_id = mod_id;
    do {
      if (!_flush_is_allowed()) {
        continue;
      }
      bresult = _opera_flush(mod_info, true);
    } while (bresult);
    FLASH_REMAP_START(mod_info->dev_id, start_addr, len);
    pmu_flash_write_config();
    result = hal_norflash_erase(mod_info->dev_id, start_addr, len);
    pmu_flash_read_config();
    FLASH_REMAP_DONE(mod_info->dev_id, start_addr, len);
    if (result == HAL_NORFLASH_OK) {
      ret = NORFLASH_API_OK;
    } else if (result == HAL_NORFLASH_BAD_ADDR) {
      ret = NORFLASH_API_BAD_ADDR;
    } else if (result == HAL_NORFLASH_BAD_LEN) {
      ret = NORFLASH_API_BAD_LEN;
    } else {
      ret = NORFLASH_API_ERR;
    }
    int_unlock_global(lock);
    NORFLASH_API_TRACE(
        4,
        "%s: hal_norflash_erase done. start_addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, start_addr, len, ret);
  }
  // NORFLASH_API_TRACE(2,"%s: done.ret = %d.",__func__, ret);
  return ret;
}

enum NORFLASH_API_RET_T norflash_api_write(enum NORFLASH_API_MODULE_ID_T mod_id,
                                           uint32_t start_addr,
                                           const uint8_t *buffer, uint32_t len,
                                           bool async) {
  MODULE_INFO *mod_info = NULL;
  MODULE_INFO *cur_mod_info = NULL;
  uint32_t lock;
  int32_t result;
  bool bresult = false;
  enum NORFLASH_API_RET_T ret;

  NORFLASH_API_TRACE(4, "%s: mod_id = %d,start_addr = 0x%x,len = 0x%x.",
                     __func__, mod_id, start_addr, len);

  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(1, "%s: norflash_api uninit!", __func__);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (mod_id > NORFLASH_API_MODULE_ID_COUNT) {
    NORFLASH_API_TRACE(2, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
    return NORFLASH_API_BAD_MOD_ID;
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    NORFLASH_API_TRACE(2, "%s :module unregistered! mod_id = %d.", __func__,
                       mod_id);
    return NORFLASH_API_ERR_UNINIT;
  }

  if (start_addr < mod_info->mod_base_addr ||
      start_addr + len > mod_info->mod_base_addr + mod_info->mod_len) {
    NORFLASH_API_TRACE(
        3, "%s : writting out of range! start_address = 0x%x,len = 0x%x.",
        __func__, mod_info->mod_base_addr, mod_info->mod_len);
    return NORFLASH_API_BAD_ADDR;
  }

  if (len == 0) {
    NORFLASH_API_TRACE(2, "%s : len error! len = %d.", __func__, len);
    return NORFLASH_API_BAD_LEN;
  }
#if defined(FLASH_API_SIMPLE)
  async = false;
#endif
  if (async) {
    // add to opera_info chain header.
    lock = int_lock_global();
    result = _w_opera_add(mod_info, start_addr, len, (uint8_t *)buffer);
    if (result == 0) {
      ret = NORFLASH_API_OK;
    } else {
      ret = NORFLASH_API_BUFFER_FULL;
    }
    int_unlock_global(lock);
    NORFLASH_API_TRACE(
        4, "%s: _w_opera_add done. start_addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, start_addr, len, ret);
  } else {
    lock = int_lock_global();

    // flush the opera of currently being processed.
    if (norflash_api_info.cur_mod != NULL &&
        mod_info != norflash_api_info.cur_mod) {
      cur_mod_info = norflash_api_info.cur_mod;
      while (cur_mod_info->state != NORFLASH_API_STATE_IDLE) {
        if (!_flush_is_allowed()) {
          continue;
        }
        bresult = _opera_flush(cur_mod_info, true);
        if (!bresult) {
          norflash_api_info.cur_mod = NULL;
        }
      }
    }

    // flush all of cur module opera.
    do {
      if (!_flush_is_allowed()) {
        continue;
      }
      bresult = _opera_flush(mod_info, true);
    } while (bresult);
    FLASH_REMAP_START(mod_info->dev_id, start_addr, len);
    pmu_flash_write_config();
    result = hal_norflash_write(mod_info->dev_id, start_addr, buffer, len);
    pmu_flash_read_config();
    FLASH_REMAP_DONE(mod_info->dev_id, start_addr, len);
    if (result == HAL_NORFLASH_OK) {
      ret = NORFLASH_API_OK;
    } else if (result == HAL_NORFLASH_BAD_ADDR) {
      ret = NORFLASH_API_BAD_ADDR;
    } else if (result == HAL_NORFLASH_BAD_LEN) {
      ret = NORFLASH_API_BAD_LEN;
    } else {
      ret = NORFLASH_API_ERR;
    }
    int_unlock_global(lock);
    NORFLASH_API_TRACE(
        4,
        "%s: hal_norflash_write done. start_addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, start_addr, len, ret);
  }
  return ret;
}

// -1: error, 0:all pending flash op flushed, 1:still pending flash op to be
// flushed
int norflash_api_flush(void) {
  enum NORFLASH_API_MODULE_ID_T mod_id = NORFLASH_API_MODULE_ID_COUNT;
  MODULE_INFO *mod_info = NULL;
  uint32_t lock;
  bool bret = false;

  lock = int_lock_global();
  if (!norflash_api_info.is_inited) {
    NORFLASH_API_TRACE(1, "%s: norflash_api uninit!", __func__);
    int_unlock_global(lock);
    return -1;
  }
#if defined(FLASH_API_SIMPLE)
  int_unlock_global(lock);
  return 0;
#endif

  if (!_flush_is_allowed()) {
    int_unlock_global(lock);
    return 0;
  }

  mod_info = _get_cur_mod();
  if (!mod_info) {
    int_unlock_global(lock);
    return 0;
  }
  mod_id = _get_mod_id(mod_info);

  norflash_api_info.cur_mod_id = mod_id;
  norflash_api_info.cur_mod = mod_info;
  bret = _opera_flush(mod_info, false);
  if (!bret) {
    norflash_api_info.cur_mod = NULL;
  }
  int_unlock_global(lock);

  return 1;
}

bool norflash_api_buffer_is_free(enum NORFLASH_API_MODULE_ID_T mod_id) {
  MODULE_INFO *mod_info = NULL;
  uint32_t count;

  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    ASSERT(0, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    ASSERT(0, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
  }

  count = _get_ew_count(mod_info);
  if (count > 0) {
    return false;
  } else {
    return true;
  }
}

uint32_t
norflash_api_get_used_buffer_count(enum NORFLASH_API_MODULE_ID_T mod_id,
                                   enum NORFLASH_API_OPRATION_TYPE type) {
  MODULE_INFO *mod_info = NULL;
  uint32_t count = 0;

  if (mod_id >= NORFLASH_API_MODULE_ID_COUNT) {
    ASSERT(0, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
  }

  mod_info = _get_module_info(mod_id);
  if (!mod_info->is_inited) {
    ASSERT(0, "%s : mod_id error! mod_id = %d.", __func__, mod_id);
  }
  if (type & NORFLASH_API_WRITTING) {
    count = _get_w_count(mod_info);
  }

  if (type & NORFLASH_API_ERASING) {
    count += _get_e_count(mod_info);
  }
  return count;
}

uint32_t
norflash_api_get_free_buffer_count(enum NORFLASH_API_OPRATION_TYPE type) {
  MODULE_INFO *mod_info = NULL;
  uint32_t i;
  uint32_t used_count = 0;
  uint32_t free_count = 0;

  if (type & NORFLASH_API_WRITTING) {
    for (i = NORFLASH_API_MODULE_ID_LOG_DUMP; i < NORFLASH_API_MODULE_ID_COUNT;
         i++) {
      mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
      if (mod_info->is_inited) {
        used_count += _get_w_count(mod_info);
      }
    }
    ASSERT(used_count <= NORFLASH_API_OPRA_LIST_LEN,
           "writting opra count error!");
    free_count += (NORFLASH_API_OPRA_LIST_LEN - used_count);
  }

  if (type & NORFLASH_API_ERASING) {
    for (i = NORFLASH_API_MODULE_ID_LOG_DUMP; i < NORFLASH_API_MODULE_ID_COUNT;
         i++) {
      mod_info = _get_module_info((enum NORFLASH_API_MODULE_ID_T)i);
      if (mod_info->is_inited) {
        used_count += _get_e_count(mod_info);
      }
    }
    ASSERT(used_count <= NORFLASH_API_OPRA_LIST_LEN, "erase opra count error!");
    free_count += (NORFLASH_API_OPRA_LIST_LEN - used_count);
  }
  return free_count;
}

void norflash_api_flush_all(void) {
  int ret;
  int cnt = 0;

  norflash_api_flush_enable_all();
  do {
    ret = norflash_api_flush();
    if (ret == 1) {
      cnt++;
    }
  } while (1 == ret);

  NORFLASH_API_TRACE(2, "%s: done. cnt = %d.", __func__, cnt);
}

void norflash_api_flush_disable(enum NORFLASH_API_USER user_id, uint32_t cb) {
  if (!norflash_api_info.is_inited) {
    return;
  }
  ASSERT(user_id < NORFLASH_API_USER_COUNTS, "%s: user_id(%d) error!", __func__,
         user_id);
  _flush_disable(user_id, cb);
}

void norflash_api_flush_enable(enum NORFLASH_API_USER user_id) {
  if (!norflash_api_info.is_inited) {
    return;
  }
  ASSERT(user_id < NORFLASH_API_USER_COUNTS, "%s: user_id(%d) too large!",
         __func__, user_id);
  _flush_enable(user_id);
}

void norflash_api_flush_enable_all(void) {
  uint32_t user_id;

  if (!norflash_api_info.is_inited) {
    return;
  }
  for (user_id = NORFLASH_API_USER_CP; user_id < NORFLASH_API_USER_COUNTS;
       user_id++) {
    _flush_enable((enum NORFLASH_API_USER)user_id);
  }
}

enum NORFLASH_API_STATE
norflash_api_get_state(enum NORFLASH_API_MODULE_ID_T mod_id) {
  ASSERT(mod_id < NORFLASH_API_MODULE_ID_COUNT,
         "%s : mod_id error! mod_id = %d.", __func__, mod_id);
  return norflash_api_info.mod_info[mod_id].state;
}

void norflash_flush_all_pending_op(void) { norflash_api_flush_all(); }

void app_flush_pending_flash_op(enum NORFLASH_API_MODULE_ID_T module,
                                enum NORFLASH_API_OPRATION_TYPE type) {
  hal_trace_pause();
  do {
    norflash_api_flush();
    if (NORFLASH_API_ALL != type) {
      if (0 == norflash_api_get_used_buffer_count(module, type)) {
        break;
      }
    } else {
      if (norflash_api_buffer_is_free(module)) {
        break;
      }
    }
    osDelay(10);
  } while (1);

  hal_trace_continue();
}

void app_flash_page_erase(enum NORFLASH_API_MODULE_ID_T module,
                          uint32_t flashOffset) {
  // check whether the flash has been erased
  bool isEmptyPage = true;
  uint32_t *ptrStartFlashAddr = (uint32_t *)(FLASH_NC_BASE + flashOffset);
  for (uint32_t index = 0; index < FLASH_SECTOR_SIZE / sizeof(uint32_t);
       index++) {
    if (0xFFFFFFFF != ptrStartFlashAddr[index]) {
      isEmptyPage = false;
      break;
    }
  }

  if (isEmptyPage) {
    return;
  }

  uint32_t lock;
  enum NORFLASH_API_RET_T ret;

  flashOffset &= 0xFFFFFF;
  do {
    lock = int_lock_global();
    hal_trace_pause();
    ret = norflash_api_erase(module, (FLASH_NC_BASE + flashOffset),
                             FLASH_SECTOR_SIZE, true);
    hal_trace_continue();
    int_unlock_global(lock);

    if (NORFLASH_API_OK == ret) {
      TRACE(1, "%s: norflash_api_erase ok!", __func__);
      break;
    } else if (NORFLASH_API_BUFFER_FULL == ret) {
      TRACE(0, "Flash async cache overflow! To flush it.");
      app_flush_pending_flash_op(module, NORFLASH_API_ERASING);
    } else {
      ASSERT(0, "%s: norflash_api_erase failed. ret = %d", __FUNCTION__, ret);
    }
  } while (1);
}

void app_flash_page_program(enum NORFLASH_API_MODULE_ID_T module,
                            uint32_t flashOffset, uint8_t *ptr, uint32_t len,
                            bool synWrite) {
  uint32_t lock;
  enum NORFLASH_API_RET_T ret;
  bool is_async = false;

  flashOffset &= 0xFFFFFF;

  if (synWrite) {
    is_async = false;
  } else {
    is_async = true;
  }

  do {
    lock = int_lock_global();
    hal_trace_pause();

    ret = norflash_api_write(module, (FLASH_NC_BASE + flashOffset), ptr, len,
                             is_async);

    hal_trace_continue();

    int_unlock_global(lock);

    if (NORFLASH_API_OK == ret) {
      TRACE(1, "%s: norflash_api_write ok!", __func__);
      break;
    } else if (NORFLASH_API_BUFFER_FULL == ret) {
      TRACE(0, "Flash async cache overflow! To flush it.");
      app_flush_pending_flash_op(module, NORFLASH_API_WRITTING);
    } else {
      ASSERT(0, "%s: norflash_api_write failed. ret = %d", __FUNCTION__, ret);
    }
  } while (1);
}
