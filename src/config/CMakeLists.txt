# Per-target override, else default — ports the pattern used by
# config/Makefile and apps/main/Makefile's app_status_ind.c handling:
# config/<OPB_TARGET_CFG_DIR>/<file> if it exists, else
# config/_default_cfg_src_/<file>. OPB_TARGET_CFG_DIR is set by the parent
# src/CMakeLists.txt (TODO Phase 2: from an OPB_TARGET cache variable via
# cmake/targets/*.cmake, once more than one target is ported).
set(default_cfg_dir ${CMAKE_CURRENT_SOURCE_DIR}/_default_cfg_src_)

function(opb_config_override_source out_var filename)
    if(OPB_TARGET_CFG_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OPB_TARGET_CFG_DIR}/${filename}")
        set(${out_var} "${CMAKE_CURRENT_SOURCE_DIR}/${OPB_TARGET_CFG_DIR}/${filename}" PARENT_SCOPE)
    else()
        set(${out_var} "${default_cfg_dir}/${filename}" PARENT_SCOPE)
    endif()
endfunction()

opb_config_override_source(tgt_hardware_src tgt_hardware.c)

add_library(config STATIC ${tgt_hardware_src})
target_include_directories(config PUBLIC ${default_cfg_dir})
if(OPB_TARGET_CFG_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${OPB_TARGET_CFG_DIR}")
    target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/${OPB_TARGET_CFG_DIR})
endif()
target_link_libraries(config platform_hal services_nv_section_aud services_multimedia)
