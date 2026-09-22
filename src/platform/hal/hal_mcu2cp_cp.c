/*
 * platform/hal/Makefile compiles hal_mcu2cp.c TWICE into two distinct
 * object files (hal_mcu2cp_mcu.o with -DMCU_API, hal_mcu2cp_cp.o with
 * -DCP_API) -- the MCU_API/CP_API guards inside hal_mcu2cp.c select
 * between the hal_mcu2cp_*_mcu()/hal_mcu2cp_*_cp() API variants and their
 * SRAM section placement (CP_TEXT_SRAM_LOC etc.) via the MCU2CP_API()
 * macro. Kbuild does this via an explicit two-target-from-one-source
 * pattern rule that CMake has no equivalent for, so replicate it as a
 * second translation unit instead: the normal hal_mcu2cp.c (matched by
 * CMakeLists.txt's source glob) provides the _mcu variant, this provides
 * the _cp one.
 */
#define CP_API
#include "hal_mcu2cp.c"
