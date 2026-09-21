file (GLOB headers "*.h")
file (GLOB sources "*.cpp")

add_library (apps_key STATIC ${sources} ${headers})
target_include_directories(apps_key PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(apps_key platform_hal apps_common util_list)