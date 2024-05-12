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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "export_fn_rom.h"

char *strsep(char **stringp, const char *delim) {
  return __export_fn_rom.strsep(stringp, delim);
}

char *strtok(char *s, const char *delim) {
  static char *lasts;

  return strtok_r(s, delim, &lasts);
}

char *strtok_r(char *s, const char *delim, char **lasts) {
  return __export_fn_rom.strtok_r(s, delim, lasts);
}

char *strncat(s1, s2, n) char *s1;
const char *s2;
size_t n;
{ return __export_fn_rom.strncat(s1, s2, n); }

int strnicmp(s1, s2, n) const char *s1;
const char *s2;
size_t n;
{ return __export_fn_rom.strnicmp(s1, s2, n); }

char *strstr(s, find) const char *s, *find;
{ return __export_fn_rom.strstr(s, find); }

void *memset(dst0, c0, length) void *dst0;
int c0;
size_t length;
{ return __export_fn_rom.memset(dst0, c0, length); }

void *memcpy(void *dst, const void *src, size_t length) {
  return __export_fn_rom.memcpy(dst, src, length);
}

void *memmove(void *dst, const void *src, size_t length) {
  return __export_fn_rom.memmove(dst, src, length);
}

int atoi(const char *s) { return __export_fn_rom.atoi(s); }

#ifndef NO_STRTOL
long strtol(const char *nptr, char **endptr, int base) {
  return __export_fn_rom.strtol(nptr, endptr, base);
}
#endif

#ifndef NO_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, int base) {
  return __export_fn_rom.strtoul(nptr, endptr, base);
}
#endif

void qsort(void *baseP, size_t nElem, size_t width,
           int (*compar)(const void *, const void *)) {
  return __export_fn_rom.qsort(baseP, nElem, width, compar);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *)) {
  return __export_fn_rom.bsearch(key, base, nmemb, size, compar);
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list ap;
  int len;

  va_start(ap, fmt);
  len = __export_fn_rom.vsnprintf(buf, -(size_t)buf, fmt, ap);
  va_end(ap);
  return (len);
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  va_list ap;
  int len;

  va_start(ap, fmt);
  len = __export_fn_rom.vsnprintf(buf, size, fmt, ap);
  va_end(ap);
  return (len);
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
  return __export_fn_rom.vsprintf(buf, fmt, ap);
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
  return __export_fn_rom.vsnprintf(buf, size, fmt, ap);
}

#ifndef NO_VSSCANF
int sscanf(const char *buf, const char *fmt, ...) {
  va_list args;
  int i;

  va_start(args, fmt);
  i = __export_fn_rom.vsscanf(buf, fmt, args);
  va_end(args);
  return i;
}

int vsscanf(const char *fp, const char *fmt0, va_list ap) {
  return __export_fn_rom.vsscanf(fp, fmt0, ap);
}
#endif
