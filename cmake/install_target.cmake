MACRO(get_all_dep tgt var)
    list( APPEND ${var} ${tgt})
    if(TARGET ${tgt})
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
        get_all_dep(${tgt} __${tgt}_deps)
    endif()
    set(__${tgt}_deps "${__${tgt}_deps}" PARENT_SCOPE)
    
endfunction()  

function(custom_target_install tgt)
    if(TARGET ${tgt})
        install(TARGETS ${tgt} RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET RESOURCE DESTINATION bin)
        get_all_dep_recursive(${tgt})
        set(tgt_dep_lib "${__${tgt}_deps}")
        #get_target_property(tgt_dep_lib ${tgt} LINK_LIBRARIES)
        if(Qt6::Widgets IN_LIST tgt_dep_lib)
            set(DEPLOY_OPTIONS "$<TARGET_NAME:${tgt}> -verbose 0 --no-translations")
            file (GENERATE
                  OUTPUT "DeployQt.cmake" 
                  CONTENT "execute_process(COMMAND \"${DEPLOYQT_EXECUTABLE}\" $<TARGET_NAME:${tgt}>.exe -verbose 0 --no-translations \
                             WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin \
                             RESULT_VARIABLE result)
                         if(result)
                             message(FATAL_ERROR \"Executing ${DEPLOYQT_EXECUTABLE} failed: \${result}\")
                         endif()"
            )           
            install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake)
        endif()

        if(MKL::MKL IN_LIST tgt_dep_lib)
                    set(mkl_dep mkl_avx2.2.dll
                        mkl_def.2.dll)
                    foreach(dep_item IN ITEMS  ${mkl_dep} )
                        unset(mkl_dep_dst_item CACHE)
                        find_file (mkl_dep_dst_item ${dep_item} PATHS  ${INSTALL_DEP_PATHS})
                        list(APPEND mkl_dep_dst ${mkl_dep_dst_item})
                    endforeach()
                    install(FILES ${mkl_dep_dst} TYPE BIN)
        endif()
        
        if(opencv_core IN_LIST tgt_dep_lib)
            configure_file(${CMAKE_SOURCE_DIR}/cmake/DeployDep.cmake DeployDep.cmake @ONLY)
            install(RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET 
                DIRECTORIES ${INSTALL_DEP_PATHS} $ENV{Path}
                PRE_EXCLUDE_REGEXES "[/\/]vc[^/\/]*$"  "[/\/]msvc[^/\/]*$"  "^qt6.*\.dll" "^api-ms-win-crt-.*\.dll" ".*\.exe"
                POST_INCLUDE_REGEXES  ""
                POST_EXCLUDE_REGEXES [[.*[/\/]system32[/\/].*\.dll]] [[.*[/\/]qt[/\/].*\.dll]]
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

             if("opencv_videoio" IN_LIST tgt_dep_lib)
                    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                    set(dnn_dep ${INSTALL_OPENCV_BIN}/opencv_videoio_ffmpeg${OpenCV_VERSION_CP}_64.dll
                        )
                    else()
                    set(dnn_dep ${INSTALL_OPENCV_BIN}/opencv_videoio_ffmpeg${OpenCV_VERSION_CP}${debug_postfix}.dll
                        )
                    endif()
                    install(FILES ${dnn_dep} TYPE BIN)
                    install(CODE "list(APPEND _CMAKE_DEPS ${dnn_dep})")
             endif()

            install(DIRECTORY ${CMAKE_SOURCE_DIR}/data TYPE BIN)
            install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployDep.cmake)
        endif()
    endif()
endfunction()
