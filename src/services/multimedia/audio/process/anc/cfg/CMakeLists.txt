file (GLOB sources "*.c*")
add_library (services_multimedia_anc_cfg STATIC ${sources})
target_link_libraries(services_multimedia_anc_cfg util_crc32 services_nv_section_aud)
target_include_directories(services_multimedia_anc_cfg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/filters/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/speech/inc
)
# services/multimedia/audio/process/anc/cfg/Makefile
target_compile_definitions(services_multimedia_anc_cfg PRIVATE USE_CMSIS_F32_FFT)
