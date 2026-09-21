/*
 * Minimal firmware image used to verify the CMake toolchain/link/image
 * pipeline (cmake/toolchain-arm-none-eabi.cmake, cmake/image.cmake) end to
 * end: cross-compile, link against the platform_hal/platform_cmsis/rtos/
 * utils chain with the real linker script, and post-process to .bin/.hex.
 *
 * This is not part of the firmware — it exercises only the subset of the
 * codebase already ported to CMake (see PLAN.md Phase 1/3). It reuses the
 * real platform/main/startup_main.S reset handler.
 */
#include "cmsis_os.h"

int main(void) {
  for (;;) {
    osDelay(1000);
  }
  return 0;
}
