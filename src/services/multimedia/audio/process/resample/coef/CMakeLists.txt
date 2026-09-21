file (GLOB sources "*.c*")
add_library (services_multimedia_resample_coef STATIC ${sources})
target_link_libraries(services_multimedia_resample_coef platform_hal)
target_include_directories(services_multimedia_resample_coef PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
# open_source/target.mk: RESAMPLE_ANY_SAMPLE_RATE ?= 1 (common.mk always-on)
target_compile_definitions(services_multimedia_resample_coef PRIVATE RESAMPLE_ANY_SAMPLE_RATE)
