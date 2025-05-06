
unset(debug_postfix)
if(Porject_BUILD_TYPE STREQUAL debug)
    set(debug_postfix "d")
endif()
if(WIN32)
    set(shared_ext ${debug_postfix}.dll)
else()
    set(shared_ext .so)
endif()
if(MSVC)
    set(shared_prefix "")
else()
    set(shared_prefix lib)
endif()

function(find_all_library)
    unset(__deps_find_all_library)
    if(${ARGV1} STREQUAL PATH_SUFFIXES)
        set(arg_index "3")
    else()
        set(arg_index "0")
    endif()
    foreach(i RANGE ${arg_index} ${ARGC})
        unset(lib_file CACHE)
        if(arg_index EQUAL 3)
            find_file(lib_file NAMES ${ARGV${i}} PATH_SUFFIXES ${ARGV2} HINTS ${INSTALL_DEP_PATHS} ${CMAKE_LIBRARY_PATH})
        else()
            find_file(lib_file NAMES ${ARGV${i}} HINTS ${INSTALL_DEP_PATHS} ${CMAKE_LIBRARY_PATH})
        endif()
        if(lib_file)
            list(APPEND __deps_find_all_library ${lib_file})
        endif()
    endforeach()
    set(${ARGV0} ${__deps_find_all_library} PARENT_SCOPE)
    unset(lib_file CACHE) 
endfunction()

find_package(OpenVINO QUIET)
if(NOT OpenVINO_FOUND AND DOWNLOAD_URL)
  FetchContent_Declare(
    OpenVINO_Build
    URL "${DOWNLOAD_URL}/openvino2450/${PHY_RUNTIME}/${PHY_ARCH}.tar.gz"
  )
  FetchContent_MakeAvailable(OpenVINO_Build)
  find_package(OpenVINO REQUIRED NO_SYSTEM_ENVIRONMENT_PATH HINTS ${openvino_build_SOURCE_DIR}/runtime)
endif()

if(OpenVINO_FOUND)
    get_target_property(ov_core openvino::runtime IMPORTED_LOCATION_${Porject_BUILD_TYPE_UPPER})
    get_filename_component(INSTALL_OPENVINO_BIN "${ov_core}" DIRECTORY)
    list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENVINO_BIN})

    set(OPENVINO_PATH_EXT "openvino-${OpenVINO_VERSION}")
    string(SUBSTRING ${OpenVINO_VERSION_MAJOR} 2 -1 OpenVINO_VERSION_COMPACT)
    set(OpenVINO_VERSION_COMPACT ${OpenVINO_VERSION_COMPACT}${OpenVINO_VERSION_MINOR}${OpenVINO_VERSION_PATCH})
endif()

set(OpenVINOLIB_CVONLY FALSE)
if(${OpenCV_VERSION} VERSION_LESS 4.8.0)
    find_all_library(inference_engine_ir_reader_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
        ${shared_prefix}inference_engine_ir_reader${shared_ext}
        ${shared_prefix}inference_engine_ir_reader${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}inference_engine_ir_reader${shared_ext}.${OpenVINO_VERSION})
    list(APPEND OpenVINOLIB ${inference_engine_ir_reader_lib})
    find_all_library(inference_engine_legacy_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
        ${shared_prefix}inference_engine_legacy${shared_ext}
        ${shared_prefix}inference_engine_legacy${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}inference_engine_legacy${shared_ext}.${OpenVINO_VERSION})
    list(APPEND OpenVINOLIB ${inference_engine_legacy_lib})
    find_all_library(inference_engine_lp_transformations_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
        ${shared_prefix}inference_engine_lp_transformations${shared_ext}
        ${shared_prefix}inference_engine_lp_transformations${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}inference_engine_lp_transformations${shared_ext}.${OpenVINO_VERSION})
    list(APPEND OpenVINOLIB ${inference_engine_lp_transformations_lib})
    find_all_library(MKLDNNPlugin_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
        ${shared_prefix}MKLDNNPlugin${shared_ext}
        ${shared_prefix}MKLDNNPlugin${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}MKLDNNPlugin${shared_ext}.${OpenVINO_VERSION})
    list(APPEND OpenVINOLIB ${MKLDNNPlugin_lib})
    find_all_library(plugins_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} plugins.xml)
    list(APPEND OpenVINOLIB ${plugins_lib})
    if(NOT OpenVINO_FOUND)
        set(OpenVINOLIB_CVONLY TRUE)
    endif()
else()
    find_all_library(openvino_intel_cpu_plugin_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
        ${shared_prefix}openvino_intel_cpu_plugin${shared_ext}
        ${shared_prefix}openvino_intel_cpu_plugin${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}openvino_intel_cpu_plugin${shared_ext}.${OpenVINO_VERSION})
    list(APPEND OpenVINOLIB ${openvino_intel_cpu_plugin_lib})
    find_all_library(openvino_ir_frontend_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT}
        ${shared_prefix}openvino_ir_frontend${shared_ext}.${OpenVINO_VERSION_COMPACT}
        ${shared_prefix}openvino_ir_frontend${shared_ext}.${OpenVINO_VERSION}
        ${shared_prefix}openvino_ir_frontend${shared_ext})
    list(APPEND OpenVINOLIB ${openvino_ir_frontend_lib})
endif()

if(OpenVINOLIB)
    set(OpenVINOLIB_FOUND TRUE)
else()
    set(OpenVINOLIB_FOUND FALSE)
endif()
