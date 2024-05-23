file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_compile_definitions(
    TARGET_LPC1768=1
    WORDS_STACK_SIZE=1024
    OS_TIMERSTKSZ=1024
    __CORTEX_M4=1
)

add_library (util_kfifo STATIC ${sources} ${headers})
target_link_libraries(util_kfifo platform_hal)
target_include_directories(util_kfifo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(util_kfifo PRIVATE 
    ${CMAKE_SOURCE_DIR}/mbed/api/
    ${CMAKE_SOURCE_DIR}/mbed/common/
    ${CMAKE_SOURCE_DIR}/mbed/targets/hal/TARGET_BEST/TARGET_BEST100X/TARGET_MBED_BEST1000/
    ${CMAKE_SOURCE_DIR}/cmsis/inc/
    ${CMAKE_SOURCE_DIR}/mbed/targets/hal/TARGET_BEST/TARGET_BEST100X/
    ${CMAKE_SOURCE_DIR}/mbed/hal/
    ${CMAKE_SOURCE_DIR}/fs/fat/
    ${CMAKE_SOURCE_DIR}/fs/sd/
    ${CMAKE_SOURCE_DIR}/fs/fat/ChaN/
    ${CMAKE_SOURCE_DIR}/rtos/rtos/
    ${CMAKE_SOURCE_DIR}/iabt/inc/
)