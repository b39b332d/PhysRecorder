# get all dependent targets
MACRO(get_all_dep tgt var)
    list( APPEND ${var} ${tgt})
    if(TARGET ${tgt})
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

function(get_all_dep_recursive tgt)
    set(__${tgt}_deps "")
    if(TARGET ${tgt})
        install(TARGETS ${tgt} RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET RESOURCE DESTINATION bin)
        get_all_dep(${tgt} __${tgt}_deps)
    endif()
    set(__${tgt}_deps "${__${tgt}_deps}" PARENT_SCOPE)
    
endfunction()  

function(custom_target_install tgt)
    if(TARGET ${tgt})
        get_all_dep_recursive(${tgt})
        set(tgt_dep_lib "${__${tgt}_deps}")
        #get_target_property(tgt_dep_lib ${tgt} LINK_LIBRARIES)
        if(Qt6::Core IN_LIST tgt_dep_lib)
            file (GENERATE
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

        endif()
        
        if(MVis IN_LIST tgt_dep_lib)
            get_target_property(TGT_LINK_FILE MVis IMPORTED_LOCATION)
            install(FILES ${TGT_LINK_FILE} TYPE BIN)
        endif()
        
        if(opencv_core IN_LIST tgt_dep_lib)
            configure_file(${CMAKE_SOURCE_DIR}/cmake/DeployDep.cmake DeployDep.cmake @ONLY)
            install(RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET 
                DIRECTORIES ${INSTALL_DEP_PATHS} $ENV{Path}
                PRE_EXCLUDE_REGEXES "^qt6.*\.dll" "^api-ms-win-.*\.dll" ".*\.exe" # "[/\/]vc[^/\/]*$"  "[/\/]msvc[^/\/]*$" 
                POST_INCLUDE_REGEXES  "[/\/]msvc[^/\/]*$" "[/\/]vcruntime[^/\/]*$"
                POST_EXCLUDE_REGEXES  [[.*[/\/]qt[/\/].*\.dll]] [[.*[/\/]system32[/\/].*\.dll]]
            )
            
            string(REPLACE "." "" OpenCV_VERSION_CP ${OpenCV_VERSION})
            unset(debug_postfix)
            if(Porject_BUILD_TYPE STREQUAL debug)
                set(debug_postfix "d")
            endif()
            
            if("opencv_dnn" IN_LIST tgt_dep_lib)
                if(${OpenCV_VERSION_CP} LESS 480)
                    set(dnn_dep ${INSTALL_OPENCV_BIN}/inference_engine_ir_reader${debug_postfix}.dll
                        ${INSTALL_OPENCV_BIN}/inference_engine_legacy${debug_postfix}.dll
                        ${INSTALL_OPENCV_BIN}/inference_engine_lp_transformations${debug_postfix}.dll
                        ${INSTALL_OPENCV_BIN}/MKLDNNPlugin${debug_postfix}.dll
                        ${INSTALL_OPENCV_BIN}/plugins.xml)
                    install(FILES ${dnn_dep} TYPE BIN)
                    install(CODE "list(APPEND _CMAKE_DEPS ${dnn_dep})")
                else()
                    set(dnn_dep openvino_auto_batch_plugin${debug_postfix}.dll
                        openvino_auto_plugin${debug_postfix}.dll
                        openvino_c${debug_postfix}.dll
                        openvino_intel_cpu_plugin${debug_postfix}.dll
                        openvino_ir_frontend${debug_postfix}.dll)
                    foreach(dep_item IN ITEMS  ${dnn_dep} )
                        unset(dnn_dep_dst_item CACHE)
                        find_file (dnn_dep_dst_item ${dep_item} PATHS  ${INSTALL_DEP_PATHS} REQUIRED)
                        list(APPEND dnn_dep_dst ${dnn_dep_dst_item})
                    endforeach()
                    install(FILES ${dnn_dep_dst} TYPE BIN)
                endif()
            endif()
            
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/data TYPE BIN PATTERN "styles" EXCLUDE)
            install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployDep.cmake)
        endif()
    endif()
endfunction()
