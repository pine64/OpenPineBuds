file (GLOB headers "include/*.h")
file (GLOB sources "src/*.c*")

add_library (services_audio_dump STATIC ${sources} ${headers})
target_link_libraries(services_audio_dump platform_hal)
target_include_directories(services_audio_dump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(services_audio_dump PRIVATE ${CMAKE_SOURCE_DIR}/src/services/tota)
