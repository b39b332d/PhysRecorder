# get all dependent targets
macro(get_all_dep tgt var)
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
        get_target_property(_tgt_dep_lib ${tgt} LINK_LIBRARIES)
        if(_tgt_dep_lib)
            foreach(tgt_dep ${_tgt_dep_lib})
                get_all_dep(${tgt_dep} ${var})
            endforeach()
        endif()
        get_target_property(_tgt_dep_ext ${tgt} MANUALLY_ADDED_DEPENDENCIES )
        if(_tgt_dep_ext)
            foreach(tgt_dep ${_tgt_dep_ext})
                get_all_dep(${tgt_dep} ${var})
            endforeach()
        endif()
    endif()
endmacro()

function(get_all_dep_recursive tgt var)
    set(__${tgt}_deps "")
    if(TARGET ${tgt})
        install(TARGETS ${tgt} RUNTIME_DEPENDENCY_SET TGT_INSTALL_SET RESOURCE DESTINATION bin)
        get_all_dep(${tgt} __${tgt}_deps)
    endif()
    set(${var} "${__${tgt}_deps}" PARENT_SCOPE)

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
                                $<TARGET_NAME:${tgt}> -unsupported-allow-new-glibc -verbose=1 -no-translations -bundle-non-qt-libs -extra-plugins=platforms -qmake=${QMAKE_EXECUTABLE} \
                                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin \
                                RESULT_VARIABLE result)
                            if(result)
                                message(FATAL_ERROR \"Executing ${DEPLOYQT_EXECUTABLE} failed: \${result}\")
                            endif()"
                )
                install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake)
                install(DIRECTORY ${CMAKE_SOURCE_DIR}/share DESTINATION  ${CMAKE_INSTALL_PREFIX})
                install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink usr/share/applications/cn.edu.njust.nbslab.PhysRecorder.desktop cn.edu.njust.nbslab.PhysRecorder.desktop WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/../)")
                install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink usr/share/icons/hicolor/256x256/PhysRecorder.png PhysRecorder.png WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/../)")
            endif()
        endif()

        if(MVis IN_LIST tgt_dep_lib)
            get_target_property(TGT_LINK_FILE MVis IMPORTED_LOCATION)
            install(FILES ${TGT_LINK_FILE} TYPE BIN)
        endif()

        if(opencv_dnn IN_LIST tgt_dep_lib AND OpenVINOLIB_FOUND)
            install(FILES ${OpenVINOLIB} TYPE LIB)
        endif()

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/data TYPE BIN PATTERN "styles" EXCLUDE)
    endif()
endfunction()
