file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_drivers_ana STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_ana platform_hal)
target_include_directories(platform_drivers_ana PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
