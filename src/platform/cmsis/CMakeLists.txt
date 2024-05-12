file (GLOB headers "inc/*.h" "inc/ca/*.h" "*.h")
file (GLOB sources "ca/*.c" "ca/*.S" "*.c*")

add_compile_definitions(USAGE_FAULT=1 BUS_FAULT=1 MEM_FAULT=1)

if(DSP_LIB)
    add_subdirectory(DSP_Lib)
endif(DSP_LIB)

add_library (platform_cmsis STATIC ${sources} ${headers})
set_property(TARGET platform_cmsis APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
target_link_libraries(platform_cmsis platform_drivers_ana platform_drivers_codec platform_drivers_norflash)
target_include_directories(platform_cmsis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/ca/
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/
)
