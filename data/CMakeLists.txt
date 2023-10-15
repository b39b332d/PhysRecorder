MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR}/models/intel)
foreach(net_name IN LISTS SUBDIRS)
    SUBDIRLIST(SUBDIRS_ACC ${CMAKE_CURRENT_SOURCE_DIR}/models/intel/${net_name})
    foreach(net_acc IN LISTS SUBDIRS_ACC)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models/intel/${net_name}/${net_acc}/${net_name}.xml AND 
        (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/models/intel/${net_name}/${net_acc}/${net_name}.bin))
        file(DOWNLOAD "https://storage.openvinotoolkit.org/repositories/open_model_zoo/2023.0/models_bin/1/${net_name}/${net_acc}/${net_name}.xml"
                ${CMAKE_CURRENT_SOURCE_DIR}/models/intel/${net_name}/${net_acc}/${net_name}.xml INACTIVITY_TIMEOUT 10 SHOW_PROGRESS)
        file(DOWNLOAD "https://storage.openvinotoolkit.org/repositories/open_model_zoo/2023.0/models_bin/1/${net_name}/${net_acc}/${net_name}.bin"
                ${CMAKE_CURRENT_SOURCE_DIR}/models/intel/${net_name}/${net_acc}/${net_name}.bin INACTIVITY_TIMEOUT 10 SHOW_PROGRESS)
    endif()
endforeach()
endforeach()