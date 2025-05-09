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

set(DLIB_MODEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/models/dlib/shape_predictor_68_face_landmarks_GTX.dat)
if(NOT EXISTS "${DLIB_MODEL_PATH}")
  find_program(bz2_exe bzip2)
  if(NOT bz2_exe)
      if(WIN32)
        FetchContent_Declare(
          bz2_build
          URL "https://versaweb.dl.sourceforge.net/project/gnuwin32/bzip2/1.0.5/bzip2-1.0.5-bin.zip"
          URL_HASH MD5=a1155c41b1954a2f6da1014c7c1a1263
        )
        FetchContent_MakeAvailable(bz2_build)
        set(bz2_exe ${bz2_build_SOURCE_DIR}/bin/bzip2.exe)
      else()
        message(FATAL_ERROR "no bzip2 program found!")
      endif()
  endif()
  file(DOWNLOAD "https://raw.gitcode.com/gh_mirrors/dl/dlib-models/raw/master/shape_predictor_68_face_landmarks_GTX.dat.bz2"
    ${DLIB_MODEL_PATH}.bz2 INACTIVITY_TIMEOUT 30 SHOW_PROGRESS)
  execute_process(COMMAND ${bz2_exe} -d ${DLIB_MODEL_PATH}.bz2 OUTPUT_VARIABLE extract_out)
  file(REMOVE ${DLIB_MODEL_PATH}.bz2)
endif()
