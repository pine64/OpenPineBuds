file (GLOB headers "inc/*.h" "inc/ca/*.h" "*.h")

# Explicit list, not file(GLOB *.c*): platform/cmsis/Makefile conditionally
# includes mpu_armv7m.o vs mpu_armv8m.o by CPU, and ca/ only for CPU=a*
# (Cortex-A). Globbing everything would compile both MPU variants (duplicate
# symbols) and the Cortex-A-only ca/ sources on this Cortex-M target.
set (sources
    cmsis_nvic.c
    patch.c
    patch_armv7m.c
    retarget_armclang_asm.S
    retarget_armclang.cpp
    retarget_gcc.cpp
    system_ARMCM.c
    system_cp.c
    system_utils.c
    stack_protector.c
)

if(CPU STREQUAL "m4")
    list(APPEND sources mpu_armv7m.c)
else()
    list(APPEND sources mpu_armv8m.c)
endif()

if(CPU MATCHES "^a")
    file (GLOB ca_sources "ca/*.c" "ca/*.S")
    list(APPEND sources ${ca_sources})
endif()

add_compile_definitions(USAGE_FAULT=1 BUS_FAULT=1 MEM_FAULT=1)

# common.mk: export DSP_LIB ?= 1 (unconditional default)
set(DSP_LIB 1)
if(DSP_LIB)
    add_subdirectory(DSP_Lib)
endif(DSP_LIB)

add_library (platform_cmsis STATIC ${sources} ${headers})
if(DSP_LIB)
    target_link_libraries(platform_cmsis platform_cmsis_dsp)
endif()
file (GLOB asm_sources "ca/*.S" "*.S")
if(asm_sources)
    set_property(SOURCE ${asm_sources} APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
endif()
target_link_libraries(platform_cmsis platform_drivers_ana platform_drivers_codec platform_drivers_norflash)
target_include_directories(platform_cmsis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/ca/
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/
)
