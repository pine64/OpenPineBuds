file (GLOB headers "inc/*.h")
file (GLOB sources "src/*.c*")

add_library (services_hw_dsp STATIC ${sources} ${headers})
target_link_libraries(services_hw_dsp platform_hal)
target_include_directories(services_hw_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(services_hw_dsp PRIVATE ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/filters/include)
