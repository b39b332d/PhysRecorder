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

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules/intel)
foreach(net_name IN LISTS SUBDIRS)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/intel/${net_name}/FP32/${net_name}.xml AND 
        (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/intel/${net_name}/FP32/${net_name}.bin))
        file(DOWNLOAD "https://storage.openvinotoolkit.org/repositories/open_model_zoo/2021.4/models_bin/3/${net_name}/FP32/${net_name}.xml"
                ${CMAKE_CURRENT_SOURCE_DIR}/modules/intel/${net_name}/FP32/${net_name}.xml TIMEOUT 10 SHOW_PROGRESS)
        file(DOWNLOAD "https://storage.openvinotoolkit.org/repositories/open_model_zoo/2021.4/models_bin/3/${net_name}/FP32/${net_name}.bin"
                ${CMAKE_CURRENT_SOURCE_DIR}/modules/intel/${net_name}/FP32/${net_name}.bin TIMEOUT 10 SHOW_PROGRESS )
    endif()
    message(INFO ${net_name} download failed!)
endforeach()