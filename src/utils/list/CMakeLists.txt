file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (util_list STATIC ${sources} ${headers})
target_include_directories(util_list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(util_list rtos)