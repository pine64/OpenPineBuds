file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_drivers_codec STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_codec platform_drivers_ana)
target_include_directories(platform_drivers_codec PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
