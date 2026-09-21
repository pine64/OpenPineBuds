file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_audio_process STATIC ${sources} ${headers})
target_include_directories(services_audio_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_audio_process util_heap services_nv_section_aud)
target_include_directories(services_audio_process PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/filters/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/common/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/drc/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/limiter/include
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/speech/inc
)

# open_source/target.mk: SW_IIR_EQ_PROCESS ?= 1
add_compile_definitions(__SW_IIR_EQ_PROCESS__)
# FULL_APP_PROJECT=1 (core-y includes apps/) -> SPEECH_LIB ?= 1 (common.mk)
set_property(SOURCE audio_spectrum.cpp APPEND PROPERTY COMPILE_DEFINITIONS SPEECH_LIB USE_MED_MALLOC)
