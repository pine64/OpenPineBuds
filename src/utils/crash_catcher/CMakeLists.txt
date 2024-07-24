file (GLOB headers "*.h" "include/*.h")
file (GLOB sources "*.c*")

set_property(SOURCE CrashCatcher_armv7m.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
add_library (util_crash_catcher STATIC ${sources} ${headers} CrashCatcher_armv7m.S)
target_link_libraries(util_crash_catcher platform_cmsis utils_xyzmodem)
target_include_directories(util_crash_catcher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_include_directories(util_crash_catcher PRIVATE
    ${CMAKE_SOURCE_DIR}/utils/libc/inc
    ${CMAKE_SOURCE_DIR}/services/nv_section/log_section/
    ${CMAKE_SOURCE_DIR}/rtos/rtx/TARGET_CORTEX_M/
)