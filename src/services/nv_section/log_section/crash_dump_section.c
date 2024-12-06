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
#if 0
#include "crash_dump_section.h"
#include "cmsis.h"

#if defined(CRASH_DUMP_SECTION_SIZE) && (CRASH_DUMP_SECTION_SIZE > 0)

#include "hal_bootmode.h"
#include "hal_norflash.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "heap_api.h"
#include "pmu.h"
#include "stdio.h"
#include "string.h"

extern uint32_t __crash_dump_start[];
extern uint32_t __crash_dump_end[];

extern const char sys_build_info[];

#define DUMPED_MSP_BYTES 32
#define DUMPED_PSP_BYTES 32

// #undef CRASH_DUMP_SECTION_SIZE

#define CRASH_DUMP_FRAME_NUM                                                   \
  (((uint32_t)__crash_dump_end - (uint32_t)__crash_dump_start) /               \
   CRASH_DUMP_FRAME_SIZE)

#ifdef TRACE_CRLF
#define NEW_LINE_STR "\r\n"
#else
#define NEW_LINE_STR "\n"
#endif

static struct CRASH_OUTPUT_BUF_T *crash_dump_section_p = NULL;
static struct CRASH_OUTPUT_BUF_T *crash_output_p = NULL;
static bool crash_dump_init = false;

static uint8_t isCrashHappened = false;
static uint8_t isCrashDumpExisting = false;

static void crashdump_dump_output(const unsigned char *buf, unsigned int buf_len)
{
    if (crash_dump_init){
        if ((crash_output_p->hdr.bufpos+buf_len) <= OFFSETOF(struct CRASH_OUTPUT_BUF_T, buf)){
            memcpy(crash_output_p->buf+crash_output_p->hdr.bufpos, buf, buf_len);
            crash_output_p->hdr.bufpos += buf_len;
        }else{
            memcpy(crash_output_p->buf+crash_output_p->hdr.bufpos, buf, OFFSETOF(struct CRASH_OUTPUT_BUF_T, buf)-crash_output_p->hdr.bufpos);
            crash_output_p->hdr.bufpos += OFFSETOF(struct CRASH_OUTPUT_BUF_T, buf)-crash_output_p->hdr.bufpos;
        }
    }
}

int crashdump_get_empty_section(uint32_t *seqnum, struct CRASH_OUTPUT_BUF_T **crash_dump_addr)
{
    struct CRASH_OUTPUT_BUF_T *p;
    int32_t lastWrittenIndex = -1, index = 0;
    int32_t max = -1;

    for (p = (struct CRASH_OUTPUT_BUF_T *)__crash_dump_start; p < (struct CRASH_OUTPUT_BUF_T *)__crash_dump_end; p++){
        // TRACE(2,"magic code is 0x%x seqnum is 0x%x", p->magicCode, (uint32_t)(p->seqnum));

        if ((CRASH_DUMP_MAGIC_CODE == p->hdr.magicCode) && (0xffffffff != (uint32_t)(p->hdr.seqnum)))
        {
            if (max < p->hdr.seqnum){
                max = p->hdr.seqnum;
                lastWrittenIndex = index;
            }
        }

        index++;
    }

    if (-1 != lastWrittenIndex)
    {
        crashdump_set_existing_flag(true);
        TRACE(1,"The last written dump log is at index %d", lastWrittenIndex);
        lastWrittenIndex++;
        if (lastWrittenIndex == (((uint32_t)__crash_dump_end-(uint32_t)__crash_dump_start)/sizeof(struct CRASH_OUTPUT_BUF_T)))
        {
            lastWrittenIndex = 0;
        }

        *crash_dump_addr = (struct CRASH_OUTPUT_BUF_T *)((uint32_t)__crash_dump_start +
            lastWrittenIndex*sizeof(struct CRASH_OUTPUT_BUF_T));
        *seqnum = max+1;
    }
    else
    {
        crashdump_set_existing_flag(false);
        TRACE(0,"No dump log saved yet!");
        *seqnum = 0;
        *crash_dump_addr = (struct CRASH_OUTPUT_BUF_T *)__crash_dump_start;
    }


    TRACE(1,"Next crash dump addr 0x%x", *crash_dump_addr);
    return 0;
}

void crashdump_flush_buffer(void)
{
    uint32_t sent_cnt;
    uint32_t send_frame_size;
    const uint32_t max_frame_size = 256;

    sent_cnt = 0;
    while (sent_cnt < crash_output_p->hdr.bufpos) {
        send_frame_size = crash_output_p->hdr.bufpos - sent_cnt;
        if (send_frame_size > max_frame_size) {
            send_frame_size = max_frame_size;
        }

        TRACE_FLUSH();
        TRACE_OUTPUT(crash_output_p->buf + sent_cnt, send_frame_size);
        TRACE_FLUSH();
        sent_cnt += send_frame_size;
    }
}

int crashdump_init(void)
{
    const char separate_line[] = NEW_LINE_STR "----------------------------------------" NEW_LINE_STR;
    uint32_t dump_section_idx;
    int len;
    char crash_buf[100];

    if (crash_dump_init)
        return 0;

    crash_dump_init = true;

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT_FLASH_FLUSH);

    crashdump_get_empty_section(&dump_section_idx, &crash_dump_section_p);

    syspool_init();
    syspool_get_buff((uint8_t **)&crash_output_p, sizeof(struct CRASH_OUTPUT_BUF_T));
    crash_output_p->hdr.bufpos = 0;

    crash_output_p->hdr.magicCode = CRASH_DUMP_MAGIC_CODE;
    crash_output_p->hdr.seqnum = dump_section_idx;
    crash_output_p->hdr.partnum = CRASH_DUMP_FRAME_NUM;
    crashdump_dump_output((unsigned char *)separate_line, sizeof(separate_line) - 1);
    len = snprintf(crash_buf, sizeof(crash_buf), "%08lX/%08lX" NEW_LINE_STR, crash_output_p->hdr.seqnum, crash_output_p->hdr.partnum);
    crashdump_dump_output((unsigned char *)crash_buf, len);
    crashdump_dump_output((unsigned char *)sys_build_info, strlen(sys_build_info));
    crashdump_dump_output((unsigned char *)separate_line, sizeof(separate_line) - 1);

#if defined(TRACE_DUMP2FLASH)
    {
        const unsigned char *e1 = NULL, *e2 = NULL;
        unsigned int len1 = 0, len2 = 0;

        hal_trace_get_history_buffer(&e1, &len1, &e2, &len2);
        if (e1 && len1) {
            crashdump_dump_output(e1, len1);
        }
        if (e2 && len2) {
            crashdump_dump_output(e2, len2);
        }
    }
#endif
    crashdump_dump_output((unsigned char *)separate_line, sizeof(separate_line) - 1);

    crashdump_dumptoflash();
    return 0;
}

#if defined(CRASH_DUMP_SECTION_SIZE) && (CRASH_DUMP_SECTION_SIZE > 0)
static void crashdump_state_handler(enum HAL_TRACE_STATE_T state)
{
    if (state == HAL_TRACE_STATE_CRASH_START) {
        crashdump_init();
    }
}
#endif

bool crashdump_is_crash_happened(void)
{
    return isCrashHappened;
}

bool crashdump_is_crash_dump_existing(void)
{
    return isCrashDumpExisting;
}

void crashdump_set_existing_flag(uint8_t isExisting)
{
    isCrashDumpExisting = isExisting;
}

int32_t crashdump_get_latest_flash_offset(void)
{
    if (crashdump_is_crash_dump_existing())
    {
        if ((uint32_t)__crash_dump_start == (uint32_t)crash_dump_section_p)
        {
            return ((uint32_t)__crash_dump_end - sizeof(struct CRASH_OUTPUT_BUF_T))&0x3FFFFF;
        }
        else
        {
            return ((uint32_t)crash_dump_section_p - sizeof(struct CRASH_OUTPUT_BUF_T))&0x3FFFFF;
        }
    }
    else
    {
        return -1;
    }
}

void crashdump_test_flash_access(void)
{
    TRACE(1,"Did crash just happen: %d", crashdump_is_crash_happened());

    TRACE(1,"Did crash ever happen: %d", crashdump_is_crash_dump_existing());

    int32_t latestCrashDumpFlashOffset = crashdump_get_latest_flash_offset();
    if (latestCrashDumpFlashOffset > 0)
    {
        TRACE(2,"The latest crash dump flash offset 0x%x flash logic addr 0x%x",
        latestCrashDumpFlashOffset,
        latestCrashDumpFlashOffset + FLASH_NC_BASE);

        struct CRASH_OUTPUT_BUF_T* pBuf =
            (struct CRASH_OUTPUT_BUF_T *)(latestCrashDumpFlashOffset + FLASH_NC_BASE);
        TRACE(1,"Valid content size of the dump log is %d", pBuf->hdr.bufpos);
        TRACE(1,"Logic address of the trace buffer is 0x%x", pBuf->buf);
        TRACE(1,"Up to %d logs can be saved in the flash", pBuf->hdr.partnum);
        TRACE(1,"Log seq number is %d", pBuf->hdr.seqnum);

        for (int i = 0; i < pBuf->hdr.bufpos;i++)
        {
            TRACE_NOCRLF(1,"%c", pBuf->buf[i]);
        }
    }
    else
    {
        TRACE(0,"No crash dump log in flash.");
    }

    TRACE(0,"=========================================");
}

#endif // (CRASH_DUMP_SECTION_SIZE > 0)

void crashdump_set_crash_happened_flag(uint8_t isHappened)
{
#if defined(CRASH_DUMP_SECTION_SIZE) && (CRASH_DUMP_SECTION_SIZE > 0)
    isCrashHappened = isHappened;
#endif
}

void crashdump_dumptoflash(void)
{
#if defined(CRASH_DUMP_SECTION_SIZE) && (CRASH_DUMP_SECTION_SIZE > 0)
    uint32_t lock;
    uint32_t dump_section_idx;

    crashdump_get_empty_section(&dump_section_idx, &crash_dump_section_p);
    lock = int_lock_global();
    pmu_flash_write_config();
    hal_norflash_erase(HAL_NORFLASH_ID_0,(uint32_t)crash_dump_section_p,sizeof(struct CRASH_OUTPUT_BUF_T));
    hal_norflash_write(HAL_NORFLASH_ID_0,(uint32_t)crash_dump_section_p,(uint8_t *)crash_output_p, sizeof(struct CRASH_OUTPUT_BUF_T));
    pmu_flash_read_config();
    int_unlock_global(lock);
#endif
}

void crashdump_init_section_info(void)
{
#if defined(CRASH_DUMP_SECTION_SIZE) && (CRASH_DUMP_SECTION_SIZE > 0)
    uint32_t dump_section_idx;
    crashdump_get_empty_section(&dump_section_idx, &crash_dump_section_p);

    hal_trace_app_register(crashdump_state_handler, crashdump_dump_output);
#endif
}
#else
#include "cmsis.h"
#include "crash_dump_section.h"
#include "hal_cache.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "heap_api.h"
#include "norflash_api.h"
#include "pmu.h"
#include <stdio.h>
#include <string.h>

extern uint32_t __crash_dump_start;
extern uint32_t __crash_dump_end;
extern void syspool_init_specific_size(uint32_t size);

static const uint32_t crash_dump_flash_start_addr =
    (uint32_t)&__crash_dump_start;
static const uint32_t crash_dump_flash_end_addr = (uint32_t)&__crash_dump_end;
static uint32_t crash_dump_flash_len;
static CRASH_DATA_BUFFER crash_data_buffer;
static uint32_t crash_dump_w_seqnum = 0;
static uint32_t crash_dump_flash_offs = 0;
static uint32_t crash_dump_cur_dump_seqnum;
static uint32_t crash_dump_total_len = 0;
static bool crash_dump_is_init = false;
static uint8_t crash_dump_is_happend = 0;
static uint32_t crash_dump_type;
HAL_TRACE_APP_NOTIFY_T crash_dump_notify_cb = NULL;
HAL_TRACE_APP_OUTPUT_T crash_dump_output_cb = NULL;
HAL_TRACE_APP_OUTPUT_T crash_dump_fault_cb = NULL;
static enum NORFLASH_API_RET_T _flash_api_read(uint32_t addr, uint8_t *buffer,
                                               uint32_t len) {

  return norflash_sync_read(NORFLASH_API_MODULE_ID_CRASH_DUMP, addr, buffer,
                            len);
}

static int32_t _crash_dump_get_seqnum(uint32_t *dump_seqnum,
                                      uint32_t *sector_seqnum) {
  uint32_t i;
  uint32_t count;
  static enum NORFLASH_API_RET_T result;
  CRASH_DUMP_HEADER_T crash_dump_header;
  uint32_t max_dump_seqnum = 0;
  uint32_t max_sector_seqnum = 0;
  bool is_existed = false;

  count = (crash_dump_flash_end_addr - crash_dump_flash_start_addr) /
          CRASH_DUMP_BUFFER_LEN;
  for (i = 0; i < count; i++) {
    result = _flash_api_read(
        crash_dump_flash_start_addr + i * CRASH_DUMP_BUFFER_LEN,
        (uint8_t *)&crash_dump_header, sizeof(CRASH_DUMP_HEADER_T));
    if (result == NORFLASH_API_OK) {
      if (crash_dump_header.magic == CRASH_DUMP_MAGIC_CODE &&
          crash_dump_header.version == CRASH_DUMP_VERSION) {
        is_existed = true;
        if (crash_dump_header.seqnum > max_dump_seqnum) {
          max_dump_seqnum = crash_dump_header.seqnum;
          max_sector_seqnum = i;
        }
      }
    } else {
      ASSERT(0, "_crash_dump_get_seqnum: _flash_api_read failed!result = %d.",
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
  return 0;
}
static void _crash_dump_notify(enum HAL_TRACE_STATE_T state) {
  uint32_t lock;
  CRASH_DATA_BUFFER *data_buff = NULL;

  if (!crash_dump_is_init) {
    return;
  }
  if (state == HAL_TRACE_STATE_CRASH_ASSERT_START) {
    crash_dump_type = CRASH_DUMP_ASSERT_CODE;
  } else if (state == HAL_TRACE_STATE_CRASH_FAULT_START) {
    crash_dump_type = CRASH_DUMP_EXCEPTION_CODE;
  }
  CRASH_DUMP_TRACE(2, "__CRASH_DUMP:%s: state = %d.", __func__, state);
  CRASH_DUMP_TRACE(3, "__CRASH_DUMP:%s: start_addr = 0x%x,end_addr = 0x%x.",
                   __func__, crash_dump_flash_start_addr,
                   crash_dump_flash_end_addr);
  CRASH_DUMP_TRACE(3,
                   "__CRASH_DUMP:%s: dump_seqnum = 0x%x,flash_offset = 0x%x.",
                   __func__, crash_dump_cur_dump_seqnum, crash_dump_flash_offs);

  if (HAL_TRACE_STATE_CRASH_END == state) {
    lock = int_lock_global();
    data_buff = &crash_data_buffer;
    crash_dump_write(crash_dump_flash_start_addr + crash_dump_flash_offs,
                     data_buff->buffer, data_buff->offset);
    int_unlock_global(lock);
  }
}

static void _crash_dump_output(const unsigned char *buf, unsigned int buf_len) {
  // uint32_t len,len1;
  uint32_t uint_len;
  uint32_t written_len;
  uint32_t remain_len;
  CRASH_DUMP_HEADER_T log_header;
  CRASH_DATA_BUFFER *data_buff = NULL;

  if (!crash_dump_is_init) {
    return;
  }

  if (strstr((char *)buf, CRASH_DUMP_PREFIX) != NULL) {
    return;
  }

  data_buff = &crash_data_buffer;
  remain_len = buf_len;
  written_len = 0;
  do {
    if (data_buff->offset == 0) {
      CRASH_DUMP_TRACE(
          4, "__CRASH_DUMP:%s:%d offset = 0.w_seqnum = %d,dump_seqnum = %d.",
          __func__, __LINE__, crash_dump_w_seqnum, crash_dump_cur_dump_seqnum);
      memset((uint8_t *)&log_header, 0, sizeof(log_header));
      log_header.magic = CRASH_DUMP_MAGIC_CODE;
      log_header.version = CRASH_DUMP_VERSION;
      log_header.seqnum = crash_dump_cur_dump_seqnum;
      log_header.reseved[0] = '\0';
      log_header.reseved[1] = '\0';
      log_header.reseved[2] = '\0';
      log_header.reseved[3] = '\n';
      memcpy(data_buff->buffer + data_buff->offset, (uint8_t *)&log_header,
             sizeof(log_header));

      data_buff->offset += sizeof(log_header);
      crash_dump_cur_dump_seqnum++;
    }

    if (data_buff->offset + remain_len > CRASH_DUMP_SECTOR_SIZE) {
      uint_len = (CRASH_DUMP_SECTOR_SIZE - data_buff->offset);
    } else {
      uint_len = remain_len;
    }
    if (uint_len > 0) {
      memcpy(data_buff->buffer + data_buff->offset, buf + written_len,
             uint_len);
      data_buff->offset += uint_len;
      written_len += uint_len;
      crash_dump_total_len += uint_len;
    }

    if (data_buff->offset == CRASH_DUMP_SECTOR_SIZE) {
      crash_dump_write(crash_dump_flash_start_addr + crash_dump_flash_offs,
                       data_buff->buffer, data_buff->offset);

      crash_dump_flash_offs =
          crash_dump_flash_offs + data_buff->offset >= crash_dump_flash_len
              ? 0
              : crash_dump_flash_offs + data_buff->offset;
      data_buff->offset = 0;
    }
    remain_len -= uint_len;
  } while (remain_len > 0);
}

static void _crash_dump_fault(const unsigned char *buf, unsigned int buf_len) {
  _crash_dump_output(buf, buf_len);
}

void crash_dump_init(void) {
  uint32_t block_size = 0;
  uint32_t sector_size = 0;
  uint32_t page_size = 0;
  uint32_t buffer_len = 0;
  uint32_t dump_seqnum = 0;
  uint32_t sector_seqnum = 0;
  enum NORFLASH_API_RET_T result;

  hal_norflash_get_size(HAL_NORFLASH_ID_0, NULL, &block_size, &sector_size,
                        &page_size);
  buffer_len = CRASH_DUMP_NORFALSH_BUFFER_LEN;
  CRASH_DUMP_TRACE(4,
                   "__CRASH_DUMP:%s: crash_dump_start = 0x%x, crash_dump_end = "
                   "0x%x, buff_len = 0x%x.",
                   __func__, crash_dump_flash_start_addr,
                   crash_dump_flash_end_addr, buffer_len);

  result = norflash_api_register(
      NORFLASH_API_MODULE_ID_CRASH_DUMP, HAL_NORFLASH_ID_0,
      crash_dump_flash_start_addr,
      crash_dump_flash_end_addr - crash_dump_flash_start_addr, block_size,
      sector_size, page_size, buffer_len, NULL);

  if (result == NORFLASH_API_OK) {
    CRASH_DUMP_TRACE(1, "__CRASH_DUMP:%s: norflash_api_register ok.", __func__);
  } else {
    CRASH_DUMP_TRACE(
        2, "__CRASH_DUMP:%s: norflash_api_register failed,result = %d",
        __func__, result);
    return;
  }

  crash_dump_notify_cb = _crash_dump_notify;
  crash_dump_output_cb = _crash_dump_output;
  crash_dump_fault_cb = _crash_dump_fault;

  hal_trace_app_custom_register(crash_dump_notify_handler,
                                crash_dump_output_handler,
                                crash_dump_fault_handler);
  _crash_dump_get_seqnum(&dump_seqnum, &sector_seqnum);
  crash_dump_cur_dump_seqnum = dump_seqnum;
  crash_dump_flash_len =
      crash_dump_flash_end_addr - crash_dump_flash_start_addr;
  crash_dump_flash_offs = sector_seqnum * CRASH_DUMP_BUFFER_LEN;
  crash_data_buffer.offset = 0;
  crash_data_buffer.buffer = NULL;
  crash_dump_w_seqnum = 0;
  crash_dump_total_len = 0;
  crash_dump_is_init = true;
}

void crash_dump_init_buffer(void) {
  crash_data_buffer.offset = 0;
  syspool_init_specific_size(CRASH_DUMP_BUFFER_LEN);
  syspool_get_buff(&(crash_data_buffer.buffer), CRASH_DUMP_BUFFER_LEN);
}

int32_t crash_dump_read(uint32_t addr, uint8_t *ptr, uint32_t len) {
  enum NORFLASH_API_RET_T ret;

  ret = norflash_sync_read(NORFLASH_API_MODULE_ID_CRASH_DUMP, addr, ptr, len);
  if (ret != NORFLASH_API_OK) {
    CRASH_DUMP_TRACE(
        4,
        "__CRASH_DUMP:%s: norflash_sync_read,addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, addr, len, ret);
    return ret;
  }
  return 0;
}

int32_t crash_dump_write(uint32_t addr, uint8_t *ptr, uint32_t len) {
  enum NORFLASH_API_RET_T ret;

  if (CRASH_LOG_ALIGN(addr, CRASH_DUMP_SECTOR_SIZE) != addr) {
    CRASH_DUMP_TRACE(2, "__CRASH_DUMP:%s: addr not aligned! addr = 0x%x.",
                     __func__, addr);
    return (int32_t)NORFLASH_API_BAD_ADDR;
  }
  ret = norflash_api_erase(NORFLASH_API_MODULE_ID_CRASH_DUMP, addr,
                           CRASH_DUMP_SECTOR_SIZE, false);
  if (ret != NORFLASH_API_OK) {
    CRASH_DUMP_TRACE(
        3, "__CRASH_DUMP:%s: norflash_api_erase failed! addr = 0x%x,ret = %d.",
        __func__, addr, ret);
    return (int32_t)ret;
  }
  ret = norflash_api_write(NORFLASH_API_MODULE_ID_CRASH_DUMP, addr, ptr, len,
                           false);
  if (ret != NORFLASH_API_OK) {
    CRASH_DUMP_TRACE(
        4,
        "__CRASH_DUMP:%s: norflash_api_write,addr = 0x%x,len = 0x%x,ret = %d.",
        __func__, addr, len, ret);
    return (int32_t)ret;
  }
  return 0;
}

void crash_dump_notify_handler(enum HAL_TRACE_STATE_T state) {
  if ((state == HAL_TRACE_STATE_CRASH_ASSERT_START) ||
      (state == HAL_TRACE_STATE_CRASH_FAULT_START)) {
    norflash_api_flush_enable_all();
    crash_dump_init_buffer();
  }

  if (crash_dump_notify_cb) {
    crash_dump_notify_cb(state);
  }
}

void crash_dump_output_handler(const unsigned char *buf, unsigned int buf_len) {
  if (crash_dump_output_cb) {
    crash_dump_output_cb(buf, buf_len);
  }
}

void crash_dump_fault_handler(const unsigned char *buf, unsigned int buf_len) {
  if (crash_dump_fault_cb) {
    crash_dump_fault_cb(buf, buf_len);
  }
}

void crash_dump_set_flag(uint8_t is_happend) {
  crash_dump_is_happend = is_happend;
}

void crash_dump_register(HAL_TRACE_APP_NOTIFY_T notify_cb,
                         HAL_TRACE_APP_OUTPUT_T crash_output_cb,
                         HAL_TRACE_APP_OUTPUT_T crash_fault_cb) {
  crash_dump_notify_cb = notify_cb;
  crash_dump_output_cb = crash_output_cb;
  crash_dump_fault_cb = crash_fault_cb;
}

CRASH_DATA_BUFFER *crash_dump_get_buffer(void) { return &crash_data_buffer; }

uint32_t crash_dump_get_type(void) { return crash_dump_type; }

#endif
