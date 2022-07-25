 if(CMAKE_INSTALL_CONFIG_NAME STREQUAL "Debug")
    foreach(_TMP_dep IN LISTS _CMAKE_DEPS)
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
    endforeach()
endif()