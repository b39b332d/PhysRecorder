foreach(_TMP_dep IN LISTS _CMAKE_DEPS)
    if(CMAKE_INSTALL_CONFIG_NAME STREQUAL "Debug")
        foreach(_deppath @INSTALL_DEP_PATHS@)
            cmake_path(IS_PREFIX _deppath ${_TMP_dep} result)
            if(result)
                break()
            endif()
        endforeach()
        if(result)
            cmake_path(REPLACE_EXTENSION _TMP_dep LAST_ONLY pdb OUTPUT_VARIABLE _TMP_dep_ALT_EXT)
            if(EXISTS ${_TMP_dep_ALT_EXT})
                file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" 
                    TYPE SHARED_LIBRARY 
                    FILES ${_TMP_dep_ALT_EXT}
                    FOLLOW_SYMLINK_CHAIN)
            endif()
        endif()
    else()
        if(_TMP_dep MATCHES ".*[/\/]msvcp[0-9]*\.dll$")
            cmake_path(REMOVE_EXTENSION _TMP_dep OUTPUT_VARIABLE MSVCP_DLL_NAME)
            set(MSVCP_DLLS_NAME "${MSVCP_DLL_NAME}_1.dll;${MSVCP_DLL_NAME}_2.dll")
            foreach(_TMP_dep IN LISTS MSVCP_DLLS_NAME)
                if(NOT (_TMP_dep IN_LIST _CMAKE_DEPS) AND (EXISTS ${_TMP_dep}))
                    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" 
                                TYPE SHARED_LIBRARY 
                                FILES ${_TMP_dep}
                                FOLLOW_SYMLINK_CHAIN)
                endif()
            endforeach()
            break()
        endif()
    endif()
endforeach()
