
set(KERNEL "rtx")

if(KERNEL STREQUAL "rtx5") #RTX5
    file (GLOB headers "rtx5/*.h" "rtx_config/*.h")
    file (GLOB sources "rtx5/*.c*" "rtx_config/*.c*")
else() #RTX
    file (GLOB headers "rtx/TARGET_CORTEX_M/*.h")
    file (GLOB sources "rtx/TARGET_CORTEX_M/*.c*" "rtx/TARGET_CORTEX_M/TARGET_M4/TOOLCHAIN_GCC/*.S")
endif()

add_library (rtos STATIC ${sources} ${headers})
target_link_libraries(rtos platform_cmsis platform_hal util_hwtimer_list)

if(KERNEL STREQUAL "rtx5") #RTX5
    target_include_directories(rtos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/rtx5/
        ${CMAKE_CURRENT_SOURCE_DIR}/rtx5/rtx_config/
    )
else() #RTX
    file (GLOB rtos_asm_sources "rtx/TARGET_CORTEX_M/TARGET_M4/TOOLCHAIN_GCC/*.S")
    if(rtos_asm_sources)
        set_property(SOURCE ${rtos_asm_sources} APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
    endif()
    target_include_directories(rtos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/rtx/TARGET_CORTEX_M/
    )

    # src/rtos/rtx/TARGET_CORTEX_M/Makefile
    target_compile_definitions(rtos PRIVATE OS_CLOCK_NOMINAL=32000) # CPU=m4 (common.mk)
    set_property(SOURCE rtx/TARGET_CORTEX_M/RTX_Conf_CM.c APPEND PROPERTY COMPILE_DEFINITIONS
        OS_TASKCNT=20         # common.mk default (TWS != 1)
        OS_SCHEDULERSTKSIZE=512
        OS_IDLESTKSIZE=256
        OS_FIFOSZ=24
    )
endif()

