file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_drivers_bt STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_bt platform_hal platform_drivers_ana util_cqueue util_heap util_crash_catcher)
target_include_directories(platform_drivers_bt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
target_include_directories(platform_drivers_bt PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/common
    ${CMAKE_SOURCE_DIR}/src/services/nvrecord
    ${CMAKE_SOURCE_DIR}/src/services/nv_section/userdata_section
    ${CMAKE_SOURCE_DIR}/src/services/bt_if_enhanced/inc # ENHANCED_STACK=1 (common.mk default)
)

if(CHIP STREQUAL "best2300p")
    # platform/drivers/bt/best2300p/Makefile
    set_property(SOURCE ${CHIP}/bt_drv_uart_bridge_intsys.c APPEND PROPERTY COMPILE_DEFINITIONS
        DEBUG_PORT=1 # open_source/target.mk default
    )
endif()
