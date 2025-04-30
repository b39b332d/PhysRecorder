# get all dependent targets
MACRO(get_all_dep tgt var)
    if(TARGET ${tgt})
        get_target_property(target_type ${tgt} TYPE)
        if (NOT target_type STREQUAL "EXECUTABLE")
            list(APPEND ${var} ${tgt})
        endif ()
        get_target_property(tgt_imported ${tgt} IMPORTED)
        get_target_property(tgt_type ${tgt} TYPE)
        if(NOT tgt_imported AND NOT tgt_type STREQUAL "INTERFACE_LIBRARY")
            install(TARGETS ${tgt} RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET RESOURCE DESTINATION bin)
        endif()
        get_target_property(tgt_dep_lib ${tgt} LINK_LIBRARIES)
        if(NOT tgt_dep_lib STREQUAL "tgt_dep_lib-NOTFOUND")
            foreach(tgt_dep ${tgt_dep_lib})
                get_all_dep(${tgt_dep} ${var})
            endforeach()
        endif()
    endif()
endMACRO()

function(get_all_dep_recursive tgt var)
    set(__${tgt}_deps "")
    if(TARGET ${tgt})
        install(TARGETS ${tgt} RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET RESOURCE DESTINATION bin)
        get_all_dep(${tgt} __${tgt}_deps)
    endif()
    set(${var} "${__${tgt}_deps}" PARENT_SCOPE)

endfunction()

unset(debug_postfix)
if(Porject_BUILD_TYPE STREQUAL debug)
    set(debug_postfix "d")
endif()
if(WIN32)
    set(shared_prefix "")
    set(shared_ext ${debug_postfix}.dll)
else()
    set(shared_prefix lib)
    set(shared_ext .so)
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
            find_file(lib_file NAMES ${ARGV${i}} PATH_SUFFIXES ${ARGV2} HINTS ${INSTALL_DEP_PATHS})
        else()
            find_file(lib_file NAMES ${ARGV${i}} HINTS ${INSTALL_DEP_PATHS})
        endif()
        if(lib_file)
            list(APPEND __deps_find_all_library ${lib_file})
        endif()
    endforeach()
    set(${ARGV0} ${__deps_find_all_library} PARENT_SCOPE)
    unset(lib_file CACHE) 
endfunction()

function(custom_target_install tgt)
    if(TARGET ${tgt})
        get_all_dep_recursive( ${tgt} tgt_dep_lib)
        
        if(Qt6::Widgets IN_LIST tgt_dep_lib)
            if(WIN32)
                file(GENERATE
                    OUTPUT "DeployQt.cmake"
                    CONTENT "execute_process(COMMAND \"${DEPLOYQT_EXECUTABLE}\" \
                                --verbose 1 --no-compiler-runtime --no-translations --exclude-plugins qjpeg,qsvg,qwebp,qtiff,qtga,qicns,qsvgicon,qgif,qico,qwbmp,qnetworklistmanager,qtuiotouchplugin\
                                $<TARGET_NAME:${tgt}>.exe \
                                    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin \
                                RESULT_VARIABLE result)
                            if(result)
                                message(FATAL_ERROR \"Executing ${DEPLOYQT_EXECUTABLE} failed: \${result}\")
                            endif()"
                )
                install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake)

                install(RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET
                    DIRECTORIES ${INSTALL_DEP_PATHS} $ENV{Path}
                    PRE_EXCLUDE_REGEXES "^qt6.*\.dll" "^api-ms-win-.*\.dll" ".*\.exe" # "[/\/]vc[^/\/]*$"  "[/\/]msvc[^/\/]*$" 
                    POST_INCLUDE_REGEXES "[/\/]msvc[^/\/]*$" "[/\/]vcruntime[^/\/]*$"
                    POST_EXCLUDE_REGEXES [[.*[/\/]qt[/\/].*\.dll]] [[.*[/\/]system32[/\/].*\.dll]]
                )
                configure_file(${CMAKE_SOURCE_DIR}/cmake/DeployDep.cmake DeployDep.cmake @ONLY)
                install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployDep.cmake)
            elseif(UNIX)
                file(GENERATE
                    OUTPUT "DeployQt.cmake"
                    CONTENT "execute_process(COMMAND \"${DEPLOYQT_EXECUTABLE}\" \
                                $<TARGET_NAME:${tgt}> -verbose=1 -no-translations -qmake=${QMAKE_EXECUTABLE} \
                                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin \
                                RESULT_VARIABLE result)
                            if(result)
                                message(FATAL_ERROR \"Executing ${DEPLOYQT_EXECUTABLE} failed: \${result}\")
                            endif()"
                )
                install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake)
            endif()
        endif()

        if(MVis IN_LIST tgt_dep_lib)
            get_target_property(TGT_LINK_FILE MVis IMPORTED_LOCATION)
            install(FILES ${TGT_LINK_FILE} TYPE BIN)
        endif()

        if(opencv_core IN_LIST tgt_dep_lib)

            string(REPLACE "." "" OpenCV_VERSION_CP ${OpenCV_VERSION})

            if("opencv_dnn" IN_LIST tgt_dep_lib)
                if(${OpenCV_VERSION_CP} LESS 480)
                    set(dnn_dep ${INSTALL_OPENCV_BIN}/inference_engine_ir_reader
                        ${INSTALL_OPENCV_BIN}/inference_engine_legacy
                        ${INSTALL_OPENCV_BIN}/inference_engine_lp_transformations
                        ${INSTALL_OPENCV_BIN}/MKLDNNPlugin
                        ${INSTALL_OPENCV_BIN}/plugins.xml)
                    install(FILES ${dnn_dep} TYPE BIN)
                    install(CODE "list(APPEND _CMAKE_DEPS ${dnn_dep})")
                else()
                    find_all_library(openvino_intel_cpu_plugin_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT} 
                        openvino_intel_cpu_plugin${shared_ext}
                        ${shared_prefix}openvino_intel_cpu_plugin${shared_ext}.${OpenVINO_VERSION_COMPACT}
                        ${shared_prefix}openvino_intel_cpu_plugin${shared_ext}.${OpenVINO_VERSION})
                    list(APPEND dnn_dep_dst ${openvino_intel_cpu_plugin_lib})
                    find_all_library(openvino_ir_frontend_lib PATH_SUFFIXES ${OPENVINO_PATH_EXT}
                         ${shared_prefix}openvino_ir_frontend${shared_ext}.${OpenVINO_VERSION_COMPACT}
                        ${shared_prefix}openvino_ir_frontend${shared_ext}.${OpenVINO_VERSION}
                        openvino_ir_frontend${shared_ext})
                    list(APPEND dnn_dep_dst ${openvino_ir_frontend_lib})
                    install(FILES ${dnn_dep_dst} TYPE LIB)
                endif()
            endif()

            install(DIRECTORY ${CMAKE_SOURCE_DIR}/data TYPE BIN PATTERN "styles" EXCLUDE)
        endif()
    endif()
endfunction()
