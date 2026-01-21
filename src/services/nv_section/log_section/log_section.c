/***************************************************************************
 *
 * Copyright 2015-2020 BES.
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
#include "log_section.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "norflash_api.h"
#include "pmu.h"
#include <stdio.h>
#include <string.h>

// #define DUMP_NO_ROLLBACK
#define LOG_DUMP_PREFIX "__LOG_DUMP:"
#if 0
#define LOG_DUMP_TRACE(num, fmt, ...) TRACE(num, fmt, ##__VA_ARGS__)
#else
#define LOG_DUMP_TRACE(num, fmt, ...)
#endif
#define LOG_DUMP_TRACE_FORCE(num, fmt, ...) TRACE(num, fmt, ##__VA_ARGS__)
#define DUMP_LOG_ALIGN(x, a) (uint32_t)(((x + a - 1) / a) * a)

extern uint32_t __log_dump_start;
extern uint32_t __log_dump_end;

extern void watchdog_ping(void);

static const uint32_t log_dump_flash_start_addr = (uint32_t)&__log_dump_start;
static const uint32_t log_dump_flash_end_addr = (uint32_t)&__log_dump_end;
static uint32_t log_dump_flash_len;

static DATA_BUFFER data_buffer_list[LOG_DUMP_SECTOR_BUFFER_COUNT];
static uint32_t log_dump_w_seqnum = 0;
static uint32_t log_dump_f_seqnum = 0;
static uint32_t log_dump_flash_offs = 0;
static uint32_t log_dump_cur_dump_seqnum;
static bool log_dump_is_init = false;
static bool log_dump_is_immediately;
static bool log_dump_is_bootup = true;
static enum LOG_DUMP_FLUSH_STATE log_dump_flash_state =
    LOG_DUMP_FLASH_STATE_IDLE;

static enum NORFLASH_API_RET_T _flash_api_read(uint32_t addr, uint8_t *buffer,
                                               uint32_t len) {

  return norflash_sync_read(NORFLASH_API_MODULE_ID_LOG_DUMP, addr, buffer, len);
}

static void _flash_api_flush(void) {
  hal_trace_pause();
  do {
    norflash_api_flush();
  } while (!norflash_api_buffer_is_free(NORFLASH_API_MODULE_ID_LOG_DUMP));

  hal_trace_continue();
}

static enum NORFLASH_API_RET_T _flash_api_erase(uint32_t addr, bool is_async) {
  uint32_t lock;
  enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

  if (log_dump_is_immediately) {
    is_async = false;
  }
  do {

    lock = int_lock_global();
    hal_trace_pause();
    ret = norflash_api_erase(NORFLASH_API_MODULE_ID_LOG_DUMP, addr,
                             LOG_DUMP_SECTOR_SIZE, is_async);
    hal_trace_continue();
    int_unlock_global(lock);

    if (NORFLASH_API_OK == ret) {
      // LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s: norflash_api_erase ok!",__func__);
      break;
    } else if (NORFLASH_API_BUFFER_FULL == ret) {
      norflash_api_flush();
      if (is_async) {
        break;
      }
    } else {
      ASSERT(
          0,
          "_flash_api_erase: norflash_api_erase failed. ret = %d,addr = 0x%x.",
          ret, addr);
    }
  } while (1);

  return ret;
}

static enum NORFLASH_API_RET_T _flash_api_write(uint32_t addr, uint8_t *ptr,
                                                uint32_t len, bool is_async) {
  uint32_t lock;
  enum NORFLASH_API_RET_T ret = NORFLASH_API_OK;

  if (log_dump_is_immediately) {
    is_async = false;
  }

  do {
    lock = int_lock_global();
    hal_trace_pause();
    ret = norflash_api_write(NORFLASH_API_MODULE_ID_LOG_DUMP, addr, ptr, len,
                             is_async);

    hal_trace_continue();

    int_unlock_global(lock);

    if (NORFLASH_API_OK == ret) {
      // LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s: norflash_api_write ok!",__func__);
      break;
    } else if (NORFLASH_API_BUFFER_FULL == ret) {
      norflash_api_flush();
      if (is_async) {
        LOG_DUMP_TRACE(1, LOG_DUMP_PREFIX "%s: norflash api buffer full",
                       __func__);
        break;
      }
    } else {
      ASSERT(0, "_flash_api_write: norflash_api_write failed. ret = %d", ret);
    }
  } while (1);

  return ret;
}

static int32_t _get_seqnum(uint32_t *dump_seqnum, uint32_t *sector_seqnum) {
#ifndef DUMP_NO_ROLLBACK
  uint32_t i;
  uint32_t count;
  static enum NORFLASH_API_RET_T result;
  LOG_DUMP_HEADER log_dump_header;
  uint32_t max_dump_seqnum = 0;
  uint32_t max_sector_seqnum = 0;
  bool is_existed = false;

  count = (log_dump_flash_end_addr - log_dump_flash_start_addr) /
          LOG_DUMP_SECTOR_SIZE;
  for (i = 0; i < count; i++) {
    result =
        _flash_api_read(log_dump_flash_start_addr + i * LOG_DUMP_SECTOR_SIZE,
                        (uint8_t *)&log_dump_header, sizeof(LOG_DUMP_HEADER));
    if (result == NORFLASH_API_OK) {
      if (log_dump_header.magic == DUMP_LOG_MAGIC &&
          log_dump_header.version == DUMP_LOG_VERSION) {
        is_existed = true;
        if (log_dump_header.seqnum > max_dump_seqnum) {
          max_dump_seqnum = log_dump_header.seqnum;
          max_sector_seqnum = i;
        }
      }
    } else {
      ASSERT(0, "_get_cur_sector_seqnum: _flash_api_read failed!result = %d.",
             result);
    }
  }
  if (is_existed) {
    *dump_seqnum = max_dump_seqnum + 1;
    *sector_seqnum = max_sector_seqnum + 1 >= count ? 0 : max_sector_seqnum + 1;
  } else {
    *dump_seqnum = 0;
    *sector_seqnum = 0;
  }
#else
  {
    LOG_DUMP_HEADER log_dump_header;
    _flash_api_read(log_dump_flash_start_addr, (uint8_t *)&log_dump_header,
                    sizeof(LOG_DUMP_HEADER));
    *dump_seqnum = 0;
    *sector_seqnum = 0;
  }
#endif
  return 0;
}

static int _log_dump_flush(uint32_t buff_state, bool is_async) {
  enum NORFLASH_API_RET_T result;
  static uint32_t total_len = 0;
  DATA_BUFFER *data_buff;

  if (log_dump_flash_state == LOG_DUMP_FLASH_STATE_IDLE &&
      (log_dump_flash_offs % LOG_DUMP_SECTOR_SIZE) == 0) {
    LOG_DUMP_TRACE(3, LOG_DUMP_PREFIX "%s:%d,state = idle,addr = 0x%x.",
                   __func__, __LINE__,
                   log_dump_flash_start_addr + log_dump_flash_offs);
    result = _flash_api_erase(log_dump_flash_start_addr + log_dump_flash_offs,
                              is_async);
    if (result == NORFLASH_API_OK) {
      LOG_DUMP_TRACE(3, LOG_DUMP_PREFIX "%s:%d,erase ok,addr = 0x%x,", __func__,
                     __LINE__, log_dump_flash_start_addr + log_dump_flash_offs);
      log_dump_flash_state = LOG_DUMP_FLASH_STATE_ERASED;
    } else {
      LOG_DUMP_TRACE(
          3,
          LOG_DUMP_PREFIX "%s: _flash_api_erase failed!addr = 0x%x,ret = %d.",
          __func__, log_dump_flash_start_addr + log_dump_flash_offs, result);
    }
  } else if (log_dump_flash_state == LOG_DUMP_FLASH_STATE_ERASED ||
             log_dump_flash_state == LOG_DUMP_FLASH_STATE_WRITTING) {
    data_buff = &data_buffer_list[log_dump_f_seqnum];
    if ((data_buff->state & buff_state) != 0 && data_buff->offset > 0) {
      LOG_DUMP_TRACE(3, LOG_DUMP_PREFIX "%s:%d,state = ERASED,f_seqnum = %d.",
                     __func__, __LINE__, log_dump_f_seqnum);
      log_dump_flash_state = LOG_DUMP_FLASH_STATE_WRITTING;
      result = _flash_api_write(log_dump_flash_start_addr + log_dump_flash_offs,
                                data_buff->buffer, data_buff->offset, is_async);
      if (result == NORFLASH_API_OK) {
        LOG_DUMP_TRACE(
            5,
            LOG_DUMP_PREFIX
            "%s:%d,write ok,addr = 0x%x,f_seqnum = 0x%x,total_len = 0x%x.",
            __func__, __LINE__, log_dump_flash_start_addr + log_dump_flash_offs,
            log_dump_f_seqnum, total_len);

        data_buff->state = DATA_BUFFER_STATE_FREE;
        log_dump_f_seqnum =
            log_dump_f_seqnum + 1 == LOG_DUMP_SECTOR_BUFFER_COUNT
                ? 0
                : log_dump_f_seqnum + 1;
        total_len += data_buff->offset;

        log_dump_flash_offs =
            log_dump_flash_offs + data_buff->offset >= log_dump_flash_len
                ? 0
                : log_dump_flash_offs + data_buff->offset;
        log_dump_flash_state = LOG_DUMP_FLASH_STATE_IDLE;
      } else {
        LOG_DUMP_TRACE(2,
                       LOG_DUMP_PREFIX "%s: _flash_api_write failed!ret = %d.",
                       __func__, result);
        return 3;
      }
    }
  } else {
    LOG_DUMP_TRACE(3, LOG_DUMP_PREFIX "%s:%d state = %d.", __func__, __LINE__,
                   log_dump_flash_state);
  }
  return 0;
}

void _log_dump_flush_remain(void) {
  uint32_t i;
  uint32_t unfree_count = 0;

  do {
    unfree_count = 0;
    for (i = 0; i < LOG_DUMP_SECTOR_BUFFER_COUNT; i++) {
      if (data_buffer_list[i].state != DATA_BUFFER_STATE_FREE) {
        unfree_count++;
      }
    }
    if (unfree_count == 0) {
      break;
    }
    _log_dump_flush(DATA_BUFFER_STATE_WRITTING | DATA_BUFFER_STATE_WRITTEN,
                    false);
  } while (1);

  _flash_api_flush();
}

int log_dump_flush_all(void) {
  norflash_api_flush_enable_all();
  log_dump_is_immediately = true;
  _log_dump_flush_remain();
  return 0;
}
int log_dump_flush(void) {
  if (!log_dump_is_init) {
    return 0;
  }

  return _log_dump_flush(DATA_BUFFER_STATE_WRITTEN, true);
}

void log_dump_notify_handler(enum HAL_TRACE_STATE_T state) {
  // uint32_t lock;

  if (!log_dump_is_init) {
    return;
  }
  LOG_DUMP_TRACE(
      3, LOG_DUMP_PREFIX "%s: state = %d,start_addr = 0x%x,end_addr = 0x%x.",
      __func__, state, log_dump_flash_start_addr, log_dump_flash_end_addr);
  LOG_DUMP_TRACE(3,
                 LOG_DUMP_PREFIX "%s: dump_seqnum = 0x%x,flash_offset = 0x%x.",
                 __func__, log_dump_cur_dump_seqnum, log_dump_flash_offs);

  if (state == HAL_TRACE_STATE_CRASH_ASSERT_START ||
      state == HAL_TRACE_STATE_CRASH_FAULT_START) {
    norflash_api_flush_enable_all();
    log_dump_is_immediately = true;
  } else {
    LOG_DUMP_TRACE(2, LOG_DUMP_PREFIX " crash end.");
    // lock = int_lock_global();
    _log_dump_flush_remain();
    // int_unlock_global(lock);
  }
}

void log_dump_output_handler(const unsigned char *buf, unsigned int buf_len) {
  uint32_t write_len;
  uint32_t written_len;
  uint32_t remain_len;
  LOG_DUMP_HEADER log_header;
  DATA_BUFFER *data_buff;

  if (!log_dump_is_init) {
    return;
  }

  if (strstr((char *)buf, LOG_DUMP_PREFIX) != NULL) {
    return;
  }

  data_buff = &data_buffer_list[log_dump_w_seqnum];
  remain_len = buf_len;
  written_len = 0;
  do {
    if (data_buff->state == DATA_BUFFER_STATE_FREE ||
        data_buff->state == DATA_BUFFER_STATE_WRITTEN) {
      // LOG_DUMP_TRACE(3,LOG_DUMP_PREFIX"%s:%d data_buff->state is
      // free.w_seqnum = %d.",
      //     __func__,__LINE__,log_dump_w_seqnum);
      data_buff->state = DATA_BUFFER_STATE_WRITTING;
      data_buff->offset = 0;
      memset(data_buff->buffer, 0, LOG_DUMP_SECTOR_SIZE);
    }
    if (data_buff->offset == 0) {
#ifndef DUMP_NO_ROLLBACK
      // LOG_DUMP_TRACE(4,LOG_DUMP_PREFIX"%s:%d offset = 0.w_seqnum =
      // %d,dump_seqnum = %d.",
      //    __func__,__LINE__,log_dump_w_seqnum,log_dump_cur_dump_seqnum);
      memset((uint8_t *)&log_header, 0, sizeof(log_header));
      log_header.magic = DUMP_LOG_MAGIC;
      log_header.version = DUMP_LOG_VERSION;
      log_header.seqnum = log_dump_cur_dump_seqnum;
      if (log_dump_is_bootup) {
        log_header.is_bootup = 1;
        log_dump_is_bootup = false;
      } else {
        log_header.is_bootup = 0;
      }
      log_header.reseved[0] = '\0';
      log_header.reseved[1] = '\0';
      log_header.reseved[2] = '\n';

      memcpy(data_buff->buffer + data_buff->offset, (uint8_t *)&log_header,
             sizeof(log_header));

      data_buff->offset += sizeof(log_header);
#else
      if (log_dump_cur_dump_seqnum * LOG_DUMP_SECTOR_SIZE >=
          log_dump_flash_len) {
        log_header = log_header;
        log_dump_is_bootup = log_dump_is_bootup;
        break;
      }
#endif
      log_dump_cur_dump_seqnum++;
    }

    if (data_buff->offset + remain_len > LOG_DUMP_SECTOR_SIZE) {
      write_len = (LOG_DUMP_SECTOR_SIZE - data_buff->offset);
    } else {
      write_len = remain_len;
    }

    if (write_len > 0) {
      memcpy(data_buff->buffer + data_buff->offset, buf + written_len,
             write_len);
      data_buff->offset += write_len;
      written_len += write_len;
    }

    if (data_buff->offset == LOG_DUMP_SECTOR_SIZE) {
      data_buff->state = DATA_BUFFER_STATE_WRITTEN;
    }

    remain_len -= write_len;

    if (data_buff->offset == LOG_DUMP_SECTOR_SIZE) {
      log_dump_w_seqnum = log_dump_w_seqnum + 1 == LOG_DUMP_SECTOR_BUFFER_COUNT
                              ? 0
                              : log_dump_w_seqnum + 1;
      if (log_dump_w_seqnum == log_dump_f_seqnum) {
        log_dump_f_seqnum =
            log_dump_f_seqnum + 1 == LOG_DUMP_SECTOR_BUFFER_COUNT
                ? 0
                : log_dump_f_seqnum + 1;
      }
      // LOG_DUMP_TRACE(4,LOG_DUMP_PREFIX"%s:%d w_seqnum = %d,dump_seqnum =
      // %d.",
      //   __func__,__LINE__,log_dump_w_seqnum,log_dump_cur_dump_seqnum);

      data_buff = &data_buffer_list[log_dump_w_seqnum];

      ASSERT(data_buff->state == DATA_BUFFER_STATE_FREE ||
                 data_buff->state == DATA_BUFFER_STATE_WRITTEN,
             "log_dump_output_handler: data_buff state error! state = %d.",
             data_buff->state);
    }
  } while (remain_len > 0);
}

void log_dump_callback(void *param) {
  NORFLASH_API_OPERA_RESULT *opera_result;

  opera_result = (NORFLASH_API_OPERA_RESULT *)param;

  LOG_DUMP_TRACE_FORCE(
      6,
      LOG_DUMP_PREFIX
      "%s:type = %d, addr = 0x%x,len = 0x%x,remain = %d,result = %d.",
      __func__, opera_result->type, opera_result->addr, opera_result->len,
      opera_result->remain_num, opera_result->result);
  {
    static uint32_t is_fst = 1;
    if (is_fst) {
      LOG_DUMP_TRACE(
          3, LOG_DUMP_PREFIX "%s:log_dump_start = 0x%x, log_dump_end = 0x%x.",
          __func__, log_dump_flash_start_addr, log_dump_flash_end_addr);
      is_fst = 0;
    }
  }
}

void log_dump_init(void) {
  uint32_t block_size = 0;
  uint32_t sector_size = 0;
  uint32_t page_size = 0;
  uint32_t buffer_len = 0;
  uint32_t dump_seqnum = 0;
  uint32_t sector_seqnum = 0;
  enum NORFLASH_API_RET_T result;
  uint32_t i;

  log_dump_flash_len = log_dump_flash_end_addr - log_dump_flash_start_addr;
  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);
  buffer_len = LOG_DUMP_NORFALSH_BUFFER_LEN;
  /*
  LOG_DUMP_TRACE(4,LOG_DUMP_PREFIX"%s: log_dump_start = 0x%x, log_dump_len =
  0x%x, buff_len = 0x%x.",
                  __func__,
                  log_dump_flash_start_addr,
                  log_dump_flash_len,
                  buffer_len);
  */
  result = norflash_api_register(
      NORFLASH_API_MODULE_ID_LOG_DUMP, HAL_NORFLASH_ID_0,
      log_dump_flash_start_addr,
      log_dump_flash_end_addr - log_dump_flash_start_addr, block_size,
      sector_size, page_size, buffer_len, log_dump_callback);

  if (result == NORFLASH_API_OK) {
    /*
    LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s: norflash_api_register ok.", __func__);
    */
  } else {
    /*
    LOG_DUMP_TRACE(2,LOG_DUMP_PREFIX"%s: norflash_api_register failed,result =
    %d.", __func__, result);
    */
    return;
  }
  hal_trace_app_register(log_dump_notify_handler, log_dump_output_handler);
  hal_sleep_set_sleep_hook(HAL_SLEEP_HOOK_DUMP_LOG, log_dump_flush);
  _get_seqnum(&dump_seqnum, &sector_seqnum);
  log_dump_cur_dump_seqnum = dump_seqnum;
  log_dump_flash_offs = sector_seqnum * LOG_DUMP_SECTOR_SIZE;
  memset((uint8_t *)&data_buffer_list, 0, sizeof(data_buffer_list));
  for (i = 0; i < LOG_DUMP_SECTOR_BUFFER_COUNT; i++) {
    data_buffer_list[i].state = DATA_BUFFER_STATE_FREE;
    data_buffer_list[i].offset = 0;
  }
  log_dump_w_seqnum = 0;
  log_dump_f_seqnum = 0;
  log_dump_flash_state = LOG_DUMP_FLASH_STATE_IDLE;
  log_dump_is_immediately = false;
  log_dump_is_init = true;
}

void log_dump_clear(void) {
  uint32_t i;
  uint32_t addr;
  uint32_t lock = int_lock_global();

  for (i = 0; i < log_dump_flash_len / LOG_DUMP_SECTOR_SIZE; i++) {
    addr = log_dump_flash_start_addr + i * LOG_DUMP_SECTOR_SIZE;
    _flash_api_erase(addr, false);
  }

  int_unlock_global(lock);
}

#if 0
uint8_t test_buff_r[LOG_DUMP_SECTOR_SIZE];

uint32_t test_log_dump_from_flash(uint32_t addr,uint32_t size)
{
	//uint32_t start_addr;
	uint32_t i;
	//uint8_t value = 0;
	int32_t ret = 0;
	enum NORFLASH_API_RET_T result = HAL_NORFLASH_OK;

	LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s enter!!!", __func__);

	for(i = 0; i < size/LOG_DUMP_SECTOR_SIZE; i++)
	{
		result = _flash_api_read((uint32_t)(addr)+ (i*LOG_DUMP_SECTOR_SIZE),
			test_buff_r,LOG_DUMP_SECTOR_SIZE);
		if(result != NORFLASH_API_OK)
		{
			ret = -1;
			//LOG_DUMP_TRACE(2,LOG_DUMP_PREFIX"%s ret=%d", __func__, ret);
			goto _func_end;
		}
		LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s", test_buff_r);
	}
    LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"%s end!!!", __func__);

_func_end:
	LOG_DUMP_TRACE(1,LOG_DUMP_PREFIX"_debug: flash checking end. ret = %d.",ret);
	return ret;
}
#endif
