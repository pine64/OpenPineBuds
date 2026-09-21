# Firmware image generation: linker-script preprocessing, build_info.c
# generation, and ELF -> BIN/HEX post-processing. Ports src/Makefile's
# cmd_cpp_lds_S / cmd_image_ver / cmd_gen-IMAGE_BIN / cmd_gen-IMAGE_HEX and
# src/config/common.mk's CPPFLAGS_${LDS_FILE} / LDS_SECTION_FLAGS.
#
# Usage:
#   opb_add_firmware_image(<name>
#       LDS scripts/link/best1000.lds.S
#       SOURCES ...
#       LINK_LIBRARIES ...
#   )
# produces <name>.elf, <name>.bin, <name>.hex next to the build directory.

# TODO(Phase 2): most of these are open_source-target defaults; move to
# cmake/targets/*.cmake once more than one target/section-size profile exists.
set(OPB_LDS_HOTWORD_SECTION_SIZE 0x0)
set(OPB_LDS_CORE_DUMP_SECTION_SIZE 0) # CORE_DUMP_TO_FLASH=0
set(OPB_LDS_OTA_UPGRADE_LOG_SIZE 0)   # OTA_ENABLE=0
set(OPB_LDS_LOG_DUMP_SECTION_SIZE 0)  # DUMP_NORMAL_LOG=0
set(OPB_LDS_CRASH_DUMP_SECTION_SIZE 0)
set(OPB_LDS_CUSTOM_PARAMETER_SECTION_SIZE 0x1000)
set(OPB_LDS_LHDC_LICENSE_SECTION_SIZE 0x1000)
set(OPB_LDS_USERDATA_SECTION_SIZE 0x1000)
set(OPB_LDS_AUD_SECTION_SIZE 0x10000) # ANC_APP=1, CHIP != best1000
set(OPB_LDS_RESERVED_SECTION_SIZE 0x1000)
set(OPB_LDS_FACTORY_SECTION_SIZE 0x1000)
set(OPB_LDS_OTA_CODE_OFFSET 0) # OTA_ENABLE=0

function(opb_preprocess_linker_script out_lds in_lds_s)
    get_filename_component(_lds_name ${in_lds_s} NAME_WLE)
    set(_out ${CMAKE_CURRENT_BINARY_DIR}/${_lds_name})
    add_custom_command(
        OUTPUT ${_out}
        COMMAND ${CMAKE_C_COMPILER}
            -DLINKER_SCRIPT
            -DFLASH_SIZE=${FLASH_SIZE}
            -DHOTWORD_SECTION_SIZE=${OPB_LDS_HOTWORD_SECTION_SIZE}
            -DCORE_DUMP_SECTION_SIZE=${OPB_LDS_CORE_DUMP_SECTION_SIZE}
            -DOTA_UPGRADE_LOG_SIZE=${OPB_LDS_OTA_UPGRADE_LOG_SIZE}
            -DLOG_DUMP_SECTION_SIZE=${OPB_LDS_LOG_DUMP_SECTION_SIZE}
            -DCRASH_DUMP_SECTION_SIZE=${OPB_LDS_CRASH_DUMP_SECTION_SIZE}
            -DCUSTOM_PARAMETER_SECTION_SIZE=${OPB_LDS_CUSTOM_PARAMETER_SECTION_SIZE}
            -DLHDC_LICENSE_SECTION_SIZE=${OPB_LDS_LHDC_LICENSE_SECTION_SIZE}
            -DUSERDATA_SECTION_SIZE=${OPB_LDS_USERDATA_SECTION_SIZE}
            -DAUD_SECTION_SIZE=${OPB_LDS_AUD_SECTION_SIZE}
            -DRESERVED_SECTION_SIZE=${OPB_LDS_RESERVED_SECTION_SIZE}
            -DFACTORY_SECTION_SIZE=${OPB_LDS_FACTORY_SECTION_SIZE}
            -DOTA_CODE_OFFSET=${OPB_LDS_OTA_CODE_OFFSET}
            -DCHIP_BEST2300P=${CHIP_BEST2300P} # TODO(Phase 2): parameterize per-chip
            -I${CMAKE_SOURCE_DIR}/src/platform/hal
            -P -C -E -x c
            -o ${_out}
            ${in_lds_s}
        DEPENDS ${in_lds_s}
        COMMENT "LDS     ${_lds_name}"
        VERBATIM
    )
    set(${out_lds} ${_out} PARENT_SCOPE)
endfunction()

function(opb_add_build_info out_var)
    # src/Makefile: BUILD_INFO_FLAGS / cmd_image_ver -> utils/build_info/build_info.c
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _git_rev
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _git_rev)
        set(_git_rev unknown)
    endif()
    set(_src ${CMAKE_SOURCE_DIR}/src/utils/build_info/build_info.c)
    if(NOT EXISTS ${_src})
        message(WARNING "opb_add_build_info: ${_src} not found, skipping build_info.o")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    add_library(opb_build_info OBJECT ${_src})
    target_compile_definitions(opb_build_info PRIVATE
        REVISION_INFO=${_git_rev}:${OPB_TARGET_CFG_DIR}
        FLASH_SIZE=${FLASH_SIZE}
        OTA_UPGRADE_CRC_LOG_SIZE=0
        NV_REC_DEV_VER=2
        CHIP=${CHIP}
    )
    target_include_directories(opb_build_info PRIVATE ${CMAKE_SOURCE_DIR}/src/platform/hal)
    set(${out_var} $<TARGET_OBJECTS:opb_build_info> PARENT_SCOPE)
endfunction()

function(opb_add_firmware_image name)
    cmake_parse_arguments(FW "" "LDS" "SOURCES;LINK_LIBRARIES" ${ARGN})

    opb_preprocess_linker_script(_lds ${FW_LDS})
    opb_add_build_info(_build_info_obj)

    add_executable(${name} ${FW_SOURCES} ${_build_info_obj})
    target_link_libraries(${name} ${FW_LINK_LIBRARIES})
    # LINK_DEPENDS alone only adds an order-only prerequisite; the custom
    # command producing ${_lds} is only wired into the ninja/make graph if
    # something treats it as a source. Add it as a non-compiled source so
    # its generating rule is actually emitted.
    target_sources(${name} PRIVATE ${_lds})
    set_source_files_properties(${_lds} PROPERTIES HEADER_FILE_ONLY TRUE)
    # TODO(Phase 3): src/Makefile's link recipe wraps IMAGE_INIT/IMAGE_MAIN in
    # --whole-archive/--no-whole-archive (weak-symbol overriding across many
    # archives) with system libs (LIB_LDFLAGS) outside that bracket. CMake's
    # target_link_libraries doesn't expose that ordering cleanly; revisit once
    # linking the real multi-hundred-archive firmware image, where it matters.
    target_link_options(${name} PRIVATE
        -T${_lds}
        -Wl,-Map=${name}.map
        -Wl,--gc-sections
    )
    set_target_properties(${name} PROPERTIES LINK_DEPENDS ${_lds})

    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -R .trc_str -O binary $<TARGET_FILE:${name}> ${name}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${name}> ${name}.hex
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${name}.bin ${CMAKE_CURRENT_BINARY_DIR}/${name}.hex
        COMMENT "GENBIN  ${name}.bin / ${name}.hex"
        VERBATIM
    )
endfunction()
