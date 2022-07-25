execute_process(
  COMMAND "@DEPLOYQT_EXECUTABLE@" @DEPLOY_OPTIONS@
  WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin
  RESULT_VARIABLE result
)
if(result)
  message(FATAL_ERROR "Executing @DEPLOYQT_EXECUTABLE@ failed: ${result}")
endif()
