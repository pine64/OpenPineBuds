file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_cp_accel STATIC ${sources} ${headers})
target_include_directories(services_cp_accel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_cp_accel util_cqueue)
target_include_directories(services_cp_accel PRIVATE ${CMAKE_SOURCE_DIR}/src/apps/common)
