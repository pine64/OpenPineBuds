
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
    set_property(TARGET rtos APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
    target_include_directories(rtos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/rtx/TARGET_CORTEX_M/
    )
endif()

