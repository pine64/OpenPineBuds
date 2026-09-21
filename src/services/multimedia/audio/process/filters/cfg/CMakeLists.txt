file (GLOB sources "*.c*")
add_library (services_multimedia_filters_cfg STATIC ${sources})
target_link_libraries(services_multimedia_filters_cfg platform_hal)
target_include_directories(services_multimedia_filters_cfg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(services_multimedia_filters_cfg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
# open_source/target.mk: AUDIO_RESAMPLE ?= 0 -> no __AUDIO_RESAMPLE__ define
