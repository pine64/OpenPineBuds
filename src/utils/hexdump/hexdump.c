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

#ifndef __ARM_EABI__
#include "stdio.h"
#else
#include "hal_trace.h"
#endif

#define LINE_BUF_SIZE 80 // MIN: 5 + 16 * 3 + 3

static void dump_buffer_internal(const unsigned char *buf, size_t len,
                                 int now) {
  static const char hex_asc_upper[] = "0123456789ABCDEF";
  char line_buf[LINE_BUF_SIZE];
  size_t line_len, pos;
  size_t i, j;

  for (i = 0; i < len; i += 16) {
    if (i + 16 <= len) {
      line_len = 16;
    } else {
      line_len = len - i;
    }
    pos = 0;
    if (len > 0x1000) {
      line_buf[pos++] = hex_asc_upper[(i >> 12) & 0xF];
    }
    if (len > 0x100) {
      line_buf[pos++] = hex_asc_upper[(i >> 8) & 0xF];
    }
    if (len > 0x10) {
      line_buf[pos++] = hex_asc_upper[(i >> 4) & 0xF];
    }
    line_buf[pos++] = hex_asc_upper[(i & 0xF)];
    line_buf[pos++] = ':';
    for (j = 0; j < line_len && pos + 3 < sizeof(line_buf); j++) {
      line_buf[pos++] = ' ';
      line_buf[pos++] = hex_asc_upper[(buf[i + j] & 0xF0) >> 4];
      line_buf[pos++] = hex_asc_upper[(buf[i + j] & 0x0F)];
    }
#ifndef __ARM_EABI__
    if (pos < sizeof(line_buf)) {
      line_buf[pos++] = '\0';
    } else {
      line_buf[sizeof(line_buf) - 1] = '\0';
    }
    printf("%s\r\n", line_buf);
#else
    if (pos < sizeof(line_buf)) {
      line_buf[pos++] = '\0';
    } else {
      line_buf[sizeof(line_buf) - 1] = '\0';
    }
    LOG_INFO(LOG_ATTR_NO_ID | (now ? LOG_ATTR_IMM : 0), line_buf);
#endif
  }
}

void dump_buffer(const void *buf, size_t len) {
  dump_buffer_internal((const unsigned char *)buf, len, 0);
}

void dump_buffer_imm(const void *buf, size_t len) {
  dump_buffer_internal((const unsigned char *)buf, len, 1);
}
