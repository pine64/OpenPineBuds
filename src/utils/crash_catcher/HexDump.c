/* Copyright (C) 2018  Adam Green (https://github.com/adamgreen)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"
#include <CrashCatcher.h>
#include <assert.h>
//#include "cmsis_os.h"
#include "coredump_section.h"
#include "xyzmodem.h"

// CRASH_CATCHER_TEST_WRITEABLE CrashCatcherReturnCodes
// g_crashCatcherDumpEndReturn = CRASH_CATCHER_TRY_AGAIN;
static void printString(const char *pString);
// static void waitForUserInput(void);
static void dumpBytes(const uint8_t *pMemory, size_t elementCount);
static void dumpByteAsHex(uint8_t byte);
static void dumpHexDigit(uint8_t nibble);
static void dumpHalfwords(const uint16_t *pMemory, size_t elementCount);
static void dumpWords(const uint32_t *pMemory, size_t elementCount);

void CrashCatcher_putc(char c) {
  hal_trace_output((const unsigned char *)&c, 1);
}

extern uint32_t __data_start__, __data_end__;
extern uint32_t __bss_start__, __bss_end__;
extern uint32_t __StackLimit, __StackTop;
extern uint32_t __overlay_text_exec_start__;
extern uint32_t __fast_sram_end__;

const CrashCatcherMemoryRegion *CrashCatcher_GetMemoryRegions(void) {
  static CrashCatcherMemoryRegion regions[4];
  int j = 0;

  regions[j].startAddress = (uint32_t)&__data_start__;
  regions[j].endAddress = (uint32_t)&__data_end__;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  j++;

  regions[j].startAddress = (uint32_t)&__bss_start__;
  regions[j].endAddress = (uint32_t)&__bss_end__;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  j++;

  regions[j].startAddress = (uint32_t)&__StackLimit;
  regions[j].endAddress = (uint32_t)&__StackTop;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  j++;
  /*
  regions[j].startAddress = (uint32_t)&__overlay_text_exec_start__;
  regions[j].endAddress =   (uint32_t)&__fast_sram_end__;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  j++;

  regions[j].startAddress = 0x0c000000;
  regions[j].endAddress =   0x0c0c8000;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  j++;
  */

  regions[j].startAddress = 0xFFFFFFFF;
  regions[j].endAddress = 0xFFFFFFFF;
  regions[j].elementSize = CRASH_CATCHER_BYTE;
  return regions;
}

static enum { DUMP_TERM, DUMP_XMODEM, DUMP_FLASH } dump_direction = DUMP_TERM;

int CrashCatcher_DumpStart(const CrashCatcherInfo *pInfo) {
  int ret;

  printString("\r\n\r\n");
  /*
  if (pInfo->isBKPT)
      printString("BREAKPOINT");
  else
  */
  printString("CRASH");
  printString(" ENCOUNTERED\r\n"
              "Enable XMODEM and then press any key to start dump.\r\n");
  // waitForUserInput();
  hal_trace_flush_buffer();

#ifdef CORE_DUMP_TO_FLASH
  core_dump_erase_section();
  dump_direction = DUMP_FLASH;
  return 0;
#endif

  ret = xmodem_start_xfer(120);
  if (!ret) {
    dump_direction = DUMP_XMODEM;
    return 0;
  }

  return 0;
}

static void printString(const char *pString) {
  while (*pString)
    CrashCatcher_putc(*pString++);
}

#if 0
static void waitForUserInput(void)
{
    CrashCatcher_getc();
}
#endif

void CrashCatcher_DumpMemory(const void *pvMemory,
                             CrashCatcherElementSizes elementSize,
                             size_t elementCount) {
  switch (elementSize) {
  case CRASH_CATCHER_BYTE:
    dumpBytes(pvMemory, elementCount);
    break;
  case CRASH_CATCHER_HALFWORD:
    dumpHalfwords(pvMemory, elementCount);
    break;
  case CRASH_CATCHER_WORD:
    dumpWords(pvMemory, elementCount);
    break;
  }
  printString("\r\n");

#ifdef CORE_DUMP_TO_FLASH
  if (elementSize * elementCount >= COREDUMP_SECTOR_SIZE) {
    core_dump_write_large(pvMemory, elementSize * elementCount);
  } else {
    core_dump_write(pvMemory, elementSize * elementCount);
  }
#endif
}

static void dumpBytes(const uint8_t *pMemory, size_t elementCount) {
  size_t i;

  if (dump_direction == DUMP_TERM) {
    for (i = 0; i < elementCount; i++) {
      /* Only dump 16 bytes to a single line before introducing a line break. */
      if (i != 0 && (i & 0xF) == 0) {
        printString("\r\n");
        hal_trace_flush_buffer();
      }
      dumpByteAsHex(*pMemory++);
    }
  } else if (dump_direction == DUMP_XMODEM) {
    const uint8_t *buf = pMemory;
    int len;

    len = xmodem_send_stream(buf, elementCount, 1);
    if (len < 0) {
      // printString("#####error");
    }
  }
}

static void dumpByteAsHex(uint8_t byte) {
  dumpHexDigit(byte >> 4);
  dumpHexDigit(byte & 0xF);
}

static void dumpHexDigit(uint8_t nibble) {
  static const char hexToASCII[] = "0123456789ABCDEF";

  assert(nibble < 16);
  CrashCatcher_putc(hexToASCII[nibble]);
}

static void dumpHalfwords(const uint16_t *pMemory, size_t elementCount) {
  size_t i;
  for (i = 0; i < elementCount; i++) {
    uint16_t val = *pMemory++;
    /* Only dump 8 halfwords to a single line before introducing a line break.
     */
    if (i != 0 && (i & 0x7) == 0)
      printString("\r\n");
    dumpBytes((uint8_t *)&val, sizeof(val));
  }
}

static void dumpWords(const uint32_t *pMemory, size_t elementCount) {
  size_t i;
  for (i = 0; i < elementCount; i++) {
    uint32_t val = *pMemory++;
    /* Only dump 4 words to a single line before introducing a line break. */
    if (i != 0 && (i & 0x3) == 0) {
      printString("\r\n");
    }
    dumpBytes((uint8_t *)&val, sizeof(val));
  }
}

CrashCatcherReturnCodes CrashCatcher_DumpEnd(void) {
  char end_info[] = "\r\nEnd of dump\r\n";

  if (dump_direction == DUMP_XMODEM) {
    int len = (strlen(end_info) + 1) / 2 * 2;
    xmodem_send_stream((const uint8_t *)end_info, len, 0);
    xmodem_stop_xfer();
  } else {
    printString(end_info);
  }

  return CRASH_CATCHER_EXIT;
#if 0
    if (g_crashCatcherDumpEndReturn == CRASH_CATCHER_TRY_AGAIN && g_info.isBKPT)
        return CRASH_CATCHER_EXIT;
    else
        return g_crashCatcherDumpEndReturn;
#endif
}
