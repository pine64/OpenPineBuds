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
#include "CrashCatcherPriv.h"
#include <CrashCatcher.h>
#include <string.h>
//#include "cmsis_os.h"
#include "cmsis_nvic.h"

/* Test harness will define this value on 64-bit machine to provide upper
 * 32-bits of pointer addresses. */
CRASH_CATCHER_TEST_WRITEABLE uint64_t g_crashCatcherTestBaseAddress;

/* The unit tests can point the core to a fake location for the SCB->CPUID
 * register. */
CRASH_CATCHER_TEST_WRITEABLE uint32_t *g_pCrashCatcherCpuId =
    (uint32_t *)0xE000ED00;

/* The unit tests can point the core to a fake location for the fault status
 * registers. */
CRASH_CATCHER_TEST_WRITEABLE uint32_t *g_pCrashCatcherFaultStatusRegisters =
    (uint32_t *)0xE000ED28;

/* The unit tests can point the core to a fake location for the Coprocessor
 * Access Control Register. */
CRASH_CATCHER_TEST_WRITEABLE uint32_t
    *g_pCrashCatcherCoprocessorAccessControlRegister = (uint32_t *)0xE000ED88;

/* Fault handler will switch MSP to use this area as the stack while
   CrashCatcher code is running. NOTE: If you change the size of this buffer, it
   also needs to be changed in the HardFault_Handler (in FaultHandler_arm*.S)
   when initializing the stack pointer. */
uint32_t g_crashCatcherStack[CRASH_CATCHER_STACK_WORD_COUNT];

typedef struct {
  const CrashCatcherExceptionRegisters *pExceptionRegisters;
  CrashCatcherStackedRegisters *pSP;
  uint32_t flags;
  CrashCatcherInfo info;
} Object;

static Object
initStackPointers(const CrashCatcherExceptionRegisters *pExceptionRegisters);
static uint32_t getAddressOfExceptionStack(
    const CrashCatcherExceptionRegisters *pExceptionRegisters);
static void *uint32AddressToPointer(uint32_t address);
static void advanceStackPointerToValueBeforeException(Object *pObject);
static int areFloatingPointRegistersAutoStacked(const Object *pObject);
static void initFloatingPointFlag(Object *pObject);
static int areFloatingPointCoprocessorsEnabled(void);
static void initIsBKPT(Object *pObject);
static int isBKPT(uint16_t instruction);
static void setStackSentinel(void);
static void dumpSignature(void);
static void dumpFlags(const Object *pObject);
static void dumpR0toR3(const Object *pObject);
static void dumpR4toR11(const Object *pObject);
static void dumpR12(const Object *pObject);
static void dumpSP(const Object *pObject);
static void dumpLR_PC_PSR(const Object *pObject);
static void dumpMSPandPSPandExceptionPSR(const Object *pObject);
static void dumpFloatingPointRegisters(const Object *pObject);
static void dumpMemoryRegions(const CrashCatcherMemoryRegion *pRegion);
static void checkStackSentinelForStackOverflow(void);
static int isARMv6MDevice(void);
static void dumpFaultStatusRegisters(void);
static void advanceProgramCounterPastHardcodedBreakpoint(const Object *pObject);
static void dumpMSPandPSPandExceptionPSRinAssert(const Object *pObject);

struct TaskRegs {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t sp;
  uint32_t lr;
  uint32_t pc;
  uint32_t psr;
};
static void dumpTaskRegs(const struct TaskRegs *regs);

void AssertCatcher_Entry(void) {
  register uint32_t r0 asm("r0");
  register uint32_t r1 asm("r1");
  register uint32_t r2 asm("r2");
  register uint32_t r3 asm("r3");
  register uint32_t r4 asm("r4");
  register uint32_t r5 asm("r5");
  register uint32_t r6 asm("r6");
  register uint32_t r7 asm("r7");
  register uint32_t r8 asm("r8");
  register uint32_t r9 asm("r9");
  register uint32_t r10 asm("r10");
  register uint32_t r11 asm("r11");
  register uint32_t r12 asm("r12");
  register uint32_t sp asm("sp");
  register uint32_t lr asm("lr");
  uint32_t pc;
  struct TaskRegs regs;

#ifdef __ARMCC_VERSION
  asm volatile(""
               : "=r"(r0), "=r"(r1), "=r"(r2), "=r"(r3), "=r"(r4), "=r"(r5),
                 "=r"(r6), "=r"(r7), "=r"(r8), "=r"(r9), "=r"(r10), "=r"(r11),
                 "=r"(r12), "=r"(sp), "=r"(lr));
#endif

  regs.r0 = r0;
  regs.r1 = r1;
  regs.r2 = r2;
  regs.r3 = r3;
  regs.r4 = r4;
  regs.r5 = r5;
  regs.r6 = r6;
  regs.r7 = r7;
  regs.r8 = r8;
  regs.r9 = r9;
  regs.r10 = r10;
  regs.r11 = r11;
  regs.r12 = r12;
  regs.sp = sp;
  regs.lr = lr;
  __asm volatile("mov %0, r15" : "=r"(pc));
  regs.psr = __get_xPSR();
  regs.pc = pc;

  int ret;
  Object object;
  object.flags = 0;

  do {
    ret = CrashCatcher_DumpStart(NULL);
    if (ret)
      break;

    dumpSignature();
    dumpFlags(&object);

    dumpTaskRegs(&regs);

    dumpMSPandPSPandExceptionPSRinAssert(NULL);
    /*
    if (object.flags & CRASH_CATCHER_FLAGS_FLOATING_POINT)
        dumpFloatingPointRegisters(&object);
    */
    dumpMemoryRegions(CrashCatcher_GetMemoryRegions());
    if (!isARMv6MDevice())
      dumpFaultStatusRegisters();

  } while (CrashCatcher_DumpEnd() == CRASH_CATCHER_TRY_AGAIN);
}

void CrashCatcher_Entry(
    const CrashCatcherExceptionRegisters *pExceptionRegisters) {
  int ret;
  Object object = initStackPointers(pExceptionRegisters);
  advanceStackPointerToValueBeforeException(&object);
  initFloatingPointFlag(&object);
  initIsBKPT(&object);

  do {
    setStackSentinel();
    ret = CrashCatcher_DumpStart(&object.info);
    if (ret)
      break;

    dumpSignature();
    dumpFlags(&object);
    dumpR0toR3(&object);
    dumpR4toR11(&object);
    dumpR12(&object);
    dumpSP(&object);
    dumpLR_PC_PSR(&object);
    dumpMSPandPSPandExceptionPSR(&object);
    if (object.flags & CRASH_CATCHER_FLAGS_FLOATING_POINT)
      dumpFloatingPointRegisters(&object);
    dumpMemoryRegions(CrashCatcher_GetMemoryRegions());
    if (!isARMv6MDevice())
      dumpFaultStatusRegisters();
    checkStackSentinelForStackOverflow();
  } while (CrashCatcher_DumpEnd() == CRASH_CATCHER_TRY_AGAIN);

  advanceProgramCounterPastHardcodedBreakpoint(&object);
}

static Object
initStackPointers(const CrashCatcherExceptionRegisters *pExceptionRegisters) {
  Object object;
  object.pExceptionRegisters = pExceptionRegisters;
  object.info.sp = getAddressOfExceptionStack(pExceptionRegisters);
  object.pSP = uint32AddressToPointer(object.info.sp);
  object.flags = 0;
  return object;
}

static uint32_t getAddressOfExceptionStack(
    const CrashCatcherExceptionRegisters *pExceptionRegisters) {
  /*always return psp */
  return pExceptionRegisters->psp;
  /*
  if (pExceptionRegisters->exceptionLR & LR_PSP)
      return pExceptionRegisters->psp;
  else
      return pExceptionRegisters->msp;
  */
}

static void *uint32AddressToPointer(uint32_t address) {
  if (sizeof(uint32_t *) == 8)
    return (void *)(unsigned long)((uint64_t)address |
                                   g_crashCatcherTestBaseAddress);
  else
    return (void *)(unsigned long)address;
}

static void advanceStackPointerToValueBeforeException(Object *pObject) {
  /* Cortex-M processor always push 8 integer registers on the stack. */
  pObject->info.sp += 8 * sizeof(uint32_t);
  /* ARMv7-M processors can also push 16 single-precision floating point
   * registers, FPSCR and a padding word. */
  if (areFloatingPointRegistersAutoStacked(pObject))
    pObject->info.sp += (16 + 1 + 1) * sizeof(uint32_t);
  /* Cortex-M processor may also have had to force 8-byte alignment before auto
   * stacking registers. */
  pObject->info.sp |= (pObject->pSP->psr & PSR_STACK_ALIGN) ? 4 : 0;
}

static int areFloatingPointRegistersAutoStacked(const Object *pObject) {
  // return 0 == (pObject->pExceptionRegisters->exceptionLR & LR_FLOAT);
  return 0;
}

static void initFloatingPointFlag(Object *pObject) {
  if (areFloatingPointCoprocessorsEnabled())
    pObject->flags |= CRASH_CATCHER_FLAGS_FLOATING_POINT;
}

static int areFloatingPointCoprocessorsEnabled(void) {
  static const uint32_t coProcessor10and11EnabledBits = 5 << 20;
  uint32_t coprocessorAccessControl =
      *g_pCrashCatcherCoprocessorAccessControlRegister;

  return (coprocessorAccessControl & (coProcessor10and11EnabledBits)) ==
         coProcessor10and11EnabledBits;
}

static void initIsBKPT(Object *pObject) {
  const uint16_t *pInstruction = uint32AddressToPointer(pObject->pSP->pc);

  pObject->info.isBKPT = isBKPT(*pInstruction);
}

static int isBKPT(uint16_t instruction) {
  return (instruction & 0xFF00) == 0xBE00;
}

static void setStackSentinel(void) {
  g_crashCatcherStack[0] = CRASH_CATCHER_STACK_SENTINEL;
}

static void dumpSignature(void) {
  static const uint8_t signature[4] = {
      CRASH_CATCHER_SIGNATURE_BYTE0, CRASH_CATCHER_SIGNATURE_BYTE1,
      CRASH_CATCHER_VERSION_MAJOR, CRASH_CATCHER_VERSION_MINOR};

  CrashCatcher_DumpMemory(signature, CRASH_CATCHER_BYTE, sizeof(signature));
}

static void dumpFlags(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->flags, CRASH_CATCHER_BYTE,
                          sizeof(pObject->flags));
}

static void dumpR0toR3(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->pSP->r0, CRASH_CATCHER_BYTE,
                          4 * sizeof(uint32_t));
}

static void dumpTaskRegs(const struct TaskRegs *regs) {
  CrashCatcher_DumpMemory(regs, CRASH_CATCHER_BYTE, sizeof(struct TaskRegs));
}

static void dumpR4toR11(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->pExceptionRegisters->r4, CRASH_CATCHER_BYTE,
                          (11 - 4 + 1) * sizeof(uint32_t));
}

static void dumpR12(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->pSP->r12, CRASH_CATCHER_BYTE,
                          sizeof(uint32_t));
}

static void dumpSP(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->info.sp, CRASH_CATCHER_BYTE,
                          sizeof(uint32_t));
}

static void dumpLR_PC_PSR(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->pSP->lr, CRASH_CATCHER_BYTE,
                          3 * sizeof(uint32_t));
}

static void dumpMSPandPSPandExceptionPSR(const Object *pObject) {
  CrashCatcher_DumpMemory(&pObject->pExceptionRegisters->msp,
                          CRASH_CATCHER_BYTE, 3 * sizeof(uint32_t));
}

static void dumpMSPandPSPandExceptionPSRinAssert(const Object *pObject) {
  uint32_t msp = __get_MSP();
  uint32_t psp = __get_PSP();
  uint32_t exceptionPSR = __get_xPSR();

  CrashCatcher_DumpMemory(&msp, CRASH_CATCHER_BYTE, sizeof(uint32_t));
  CrashCatcher_DumpMemory(&psp, CRASH_CATCHER_BYTE, sizeof(uint32_t));
  CrashCatcher_DumpMemory(&exceptionPSR, CRASH_CATCHER_BYTE, sizeof(uint32_t));
}

static void dumpFloatingPointRegisters(const Object *pObject) {
  uint32_t allFloatingPointRegisters[32 + 1];
  if (areFloatingPointRegistersAutoStacked(pObject)) {
    /* Copy the upper floats first as that will cause a lazy copy of the
     * auto-stacked registers. */
    CrashCatcher_CopyUpperFloatingPointRegisters(
        &allFloatingPointRegisters[16]);
    memcpy(&allFloatingPointRegisters[0], &pObject->pSP->floats,
           sizeof(pObject->pSP->floats));
    allFloatingPointRegisters[32] = pObject->pSP->fpscr;
  } else {
    CrashCatcher_CopyAllFloatingPointRegisters(allFloatingPointRegisters);
  }
  CrashCatcher_DumpMemory(allFloatingPointRegisters, CRASH_CATCHER_BYTE,
                          sizeof(allFloatingPointRegisters));
}

static void dumpMemoryRegions(const CrashCatcherMemoryRegion *pRegion) {
  while (pRegion && pRegion->startAddress != 0xFFFFFFFF) {
    /* Just dump the two addresses in pRegion.  The element size isn't required.
     */
    CrashCatcher_DumpMemory(pRegion, CRASH_CATCHER_BYTE, 2 * sizeof(uint32_t));
    CrashCatcher_DumpMemory(
        uint32AddressToPointer(pRegion->startAddress), pRegion->elementSize,
        (pRegion->endAddress - pRegion->startAddress) / pRegion->elementSize);
    pRegion++;
  }
}

static void checkStackSentinelForStackOverflow(void) {
  if (g_crashCatcherStack[0] != CRASH_CATCHER_STACK_SENTINEL) {
    uint8_t value[4] = {0xAC, 0xCE, 0x55, 0xED};
    CrashCatcher_DumpMemory(value, CRASH_CATCHER_BYTE, sizeof(value));
  }
}

static int isARMv6MDevice(void) {
  static const uint32_t armv6mArchitecture = 0xC << 16;
  uint32_t cpuId = *g_pCrashCatcherCpuId;
  uint32_t architecture = cpuId & (0xF << 16);

  return (architecture == armv6mArchitecture);
}

static void dumpFaultStatusRegisters(void) {
  uint32_t faultStatusRegistersAddress =
      (uint32_t)(unsigned long)g_pCrashCatcherFaultStatusRegisters;
  CrashCatcherMemoryRegion faultStatusRegion[] = {
      {faultStatusRegistersAddress,
       faultStatusRegistersAddress + 5 * sizeof(uint32_t), CRASH_CATCHER_WORD},
      {0xFFFFFFFF, 0xFFFFFFFF, CRASH_CATCHER_BYTE}};
  dumpMemoryRegions(faultStatusRegion);
}

static void
advanceProgramCounterPastHardcodedBreakpoint(const Object *pObject) {
  if (pObject->info.isBKPT)
    pObject->pSP->pc += 2;
}
