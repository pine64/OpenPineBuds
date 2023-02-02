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
#ifdef CHIP_HAS_DIG_RTC

#include "hal_rtc.h"
#include "plat_addr_map.h"
#include "reg_rtc.h"

static HAL_RTC_IRQ_HANDLER_T irq_handler = NULL;

static struct RTC_T *const rtc = (struct RTC_T *)RTC_BASE;

static const unsigned char rtc_days_in_month[] = {31, 28, 31, 30, 31, 30,
                                                  31, 31, 30, 31, 30, 31};

static const unsigned short rtc_ydays[2][13] = {
    /* Normal years */
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
    /* Leap years */
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}};

#define LEAPS_THRU_END_OF(y) ((y) / 4 - (y) / 100 + (y) / 400)

static inline int is_leap_year(unsigned int year) {
  return (!(year % 4) && (year % 100)) || !(year % 400);
}

/*
 * The number of days in the month.
 */
int rtc_month_days(unsigned int month, unsigned int year) {
  return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}

/*
 * The number of days since January 1. (0 to 365)
 */
int rtc_year_days(unsigned int day, unsigned int month, unsigned int year) {
  return rtc_ydays[is_leap_year(year)][month] + day - 1;
}

/*
 * Convert seconds since 01-01-1970 00:00:00 to Gregorian date.
 */
void rtc_time_to_tm(unsigned long time, struct rtc_time *tm) {
  unsigned int month, year;
  int days;

  days = time / 86400;
  time -= (unsigned int)days * 86400;

  /* day of the week, 1970-01-01 was a Thursday */
  // tm->tm_wday = (days + 4) % 7;

  year = 1970 + days / 365;
  days -= (year - 1970) * 365 + LEAPS_THRU_END_OF(year - 1) -
          LEAPS_THRU_END_OF(1970 - 1);
  if (days < 0) {
    year -= 1;
    days += 365 + is_leap_year(year);
  }
  tm->tm_year = year - 1900;
  // tm->tm_yday = days + 1;

  for (month = 0; month < 11; month++) {
    int newdays;

    newdays = days - rtc_month_days(month, year);
    if (newdays < 0)
      break;
    days = newdays;
  }
  tm->tm_mon = month;
  tm->tm_mday = days + 1;

  tm->tm_hour = time / 3600;
  time -= tm->tm_hour * 3600;
  tm->tm_min = time / 60;
  tm->tm_sec = time - tm->tm_min * 60;
}

/*
 * Does the rtc_time represent a valid date/time?
 */
int rtc_valid_tm(struct rtc_time *tm) {
  if (tm->tm_year < 70 || ((unsigned)tm->tm_mon) >= 12 || tm->tm_mday < 1 ||
      tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900) ||
      ((unsigned)tm->tm_hour) >= 24 || ((unsigned)tm->tm_min) >= 60 ||
      ((unsigned)tm->tm_sec) >= 60)
    return -1;

  return 0;
}

/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines where long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */
unsigned long mktime(const unsigned int year0, const unsigned int mon0,
                     const unsigned int day, const unsigned int hour,
                     const unsigned int min, const unsigned int sec) {
  unsigned int mon = mon0, year = year0;

  /* 1..12 -> 11,12,1..10 */
  if (0 >= (int)(mon -= 2)) {
    mon += 12; /* Puts Feb last since it has leap day */
    year -= 1;
  }

  return ((((unsigned long)(year / 4 - year / 100 + year / 400 +
                            367 * mon / 12 + day) +
            year * 365 - 719499) *
               24 +
           hour /* now have hours */
           ) * 60 +
          min /* now have minutes */
          ) * 60 +
         sec; /* finally seconds */
}

/*
 * Convert Gregorian date to seconds since 01-01-1970 00:00:00.
 */
int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time) {
  *time = mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
                 tm->tm_min, tm->tm_sec);
  return 0;
}

int hal_rtc_get(struct rtc_time *time) {
  unsigned long value;

  if ((rtc->RTCCR & RTC_CR_EN) == 0) {
    return 1;
  }

  value = rtc->RTCDR;
  rtc_tm_to_time(time, &value);
  return 0;
}

int hal_rtc_set(struct rtc_time *time) {
  unsigned long value;

  if (rtc_valid_tm(time) != 0) {
    return 1;
  }

  if ((rtc->RTCCR & RTC_CR_EN) == 0) {
    rtc->RTCCR = RTC_CR_EN;
  }

  rtc_tm_to_time(time, &value);
  rtc->RTCLR = value;
  return 0;
}

int hal_rtc_set_alarm(struct rtc_time *time) {
  unsigned long value;

  if (rtc_valid_tm(time) != 0) {
    return 1;
  }

  if ((rtc->RTCCR & RTC_CR_EN) == 0) {
    return 1;
  }

  rtc_tm_to_time(time, &value);
  rtc->RTCMR = value;
  rtc->RTCICR = RTC_BIT_AI;
  rtc->RTCIMSC = RTC_BIT_AI;
  return 0;
}

int hal_rtc_clear_alarm(void) {
  rtc->RTCIMSC = 0;
  rtc->RTCICR = RTC_BIT_AI;
  return 0;
}

HAL_RTC_IRQ_HANDLER_T hal_rtc_set_irq_handler(HAL_RTC_IRQ_HANDLER_T handler) {
  HAL_RTC_IRQ_HANDLER_T old_handler;

  old_handler = irq_handler;
  irq_handler = handler;
  return old_handler;
}

void RTC_IRQHandler(void) {
  uint32_t value;
  struct rtc_time time;

  if ((rtc->RTCMIS & RTC_BIT_AI) != 0) {
    rtc->RTCICR = RTC_BIT_AI;

    if (irq_handler != NULL) {
      value = rtc->RTCDR;
      rtc_time_to_tm(value, &time);
      irq_handler(&time);
    }
  }
}

#endif // CHIP_HAS_DIG_RTC
