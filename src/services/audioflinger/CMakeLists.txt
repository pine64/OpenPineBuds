file (GLOB headers "*.h")
file (GLOB sources "*.c*")

add_library (services_audioflinger STATIC ${sources} ${headers})
target_include_directories(services_audioflinger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(services_audioflinger platform_hal platform_drivers_codec platform_drivers_ana util_cqueue)
target_include_directories(services_audioflinger PRIVATE
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat
    ${CMAKE_SOURCE_DIR}/src/services/fs/sd
    ${CMAKE_SOURCE_DIR}/src/services/fs/fat/ChaN
    ${CMAKE_SOURCE_DIR}/src/services/multimedia/audio/process/floatlimiter/include
)

# services/audioflinger/Makefile — open_source/target.mk + common.mk defaults:
# AF_DEVICE_INT_CODEC ?= 1, RTOS=1 -> AF_DEVICE_BT_PCM ?= 1,
# AUDIO_OUTPUT_DC_CALIB_ANA=1 (best2300p, see platform/drivers/ana), CHIP_HAS_I2S=2
set_property(SOURCE audioflinger.c APPEND PROPERTY COMPILE_DEFINITIONS
    AF_DEVICE_INT_CODEC
    AF_DEVICE_BT_PCM
    AUDIO_OUTPUT_DC_CALIB_ANA
    CHIP_HAS_I2S=2
)
