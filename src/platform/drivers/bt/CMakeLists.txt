file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_drivers_bt STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_bt apps_common platform_drivers_ana utils_cqueue util_heap)
target_include_directories(platform_drivers_bt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
target_include_directories(platform_drivers_bt PRIVATE
    ${CMAKE_SOURCE_DIR}/services/nvrecord
)