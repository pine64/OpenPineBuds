# Cross-toolchain identification for the GNU Arm Embedded toolchain this
# firmware ships with (see Dockerfile: gcc-arm-none-eabi-9-2019-q4-major).
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

find_program(OPB_ARM_GCC arm-none-eabi-gcc)
if(NOT OPB_ARM_GCC)
    message(FATAL_ERROR "arm-none-eabi-gcc not found in PATH; install the GNU Arm Embedded toolchain (see Dockerfile)")
endif()

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE FILEPATH "")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE FILEPATH "")
set(CMAKE_SIZE arm-none-eabi-size CACHE FILEPATH "")
set(CMAKE_AR arm-none-eabi-ar CACHE FILEPATH "")

# Bare metal: no OS to run a test executable against, so CMake's compiler
# checks must link a static library instead of an executable (mirrored by
# CMAKE_TRY_COMPILE_TARGET_TYPE already being set in the top CMakeLists.txt).
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Host tools invoked by the build (cargo, cxxbridge, git, ffmpeg, ...) must
# resolve from the host PATH, not be constrained to a cross sysroot that
# doesn't exist for a bare-metal target.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ---------------------------------------------------------------------------
# CPU ABI, ported from src/config/common.mk's CPU_CFLAGS.
# Only best2300p (Cortex-M4F, hard-float) is wired up so far — this fork's
# src/CMakeLists.txt hardcodes CHIP=best2300p.
# TODO(Phase 2): derive from an OPB_CHIP cache variable via cmake/chips/*.cmake
# once other chips are ported, instead of hardcoding cortex-m4 here.
set(OPB_CPU_FLAGS "-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# --specs=nano.specs: ported from common.mk's SPECS_CFLAGS (NOSTD=0, non-armclang path).
# Only set on C/CXX_FLAGS, not EXE_LINKER_FLAGS: CMake's executable link rule
# already passes CMAKE_<LANG>_FLAGS to the compiler-as-linker-driver command,
# so adding --specs=nano.specs to both duplicates it on the link line, which
# gcc rejects ("attempt to rename spec 'link' to already defined spec
# 'nano_link'"). CPU_FLAGS duplicating is harmless, so it's kept on both for
# link-only sources (e.g. a target with only ASM/OBJECT sources).
set(CMAKE_C_FLAGS_INIT "${OPB_CPU_FLAGS} --specs=nano.specs")
set(CMAKE_CXX_FLAGS_INIT "${OPB_CPU_FLAGS} --specs=nano.specs")
set(CMAKE_ASM_FLAGS_INIT "${OPB_CPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${OPB_CPU_FLAGS}")
