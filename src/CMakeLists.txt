
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        imageviewer.cpp
        imageviewer.h

        converter.cpp
        converter.h
        capture.h
        capture.cpp

        ${CMAKE_SOURCE_DIR}/data/resources/darkstyle.qrc
)
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/data/resources/appicon.rc")
add_executable(rppg
    WIN32
    ${PROJECT_SOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
)
set(rppt_OUTPUT_NAME RealsenseViewer)
set_target_properties(rppg PROPERTIES
  OUTPUT_NAME ${rppt_OUTPUT_NAME}
)

target_compile_definitions( rppg
    PRIVATE
    PROJECT_ROOT_PATH="./data/")


add_subdirectory(signalprocess)
add_subdirectory(utils)

target_include_directories(rppg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")


target_link_libraries(rppg PRIVATE Qt6::SerialPort Qt6::Widgets ${OpenCV_LIBS} realsense "Setupapi.lib")

if(EIGEN3_FOUND)
    target_link_libraries(rppg PRIVATE Eigen3::Eigen)
    if(TARGET MKL::MKL)
        set_target_properties(rppg PROPERTIES ENVIRONMENT "${MKL_ENV}")
        target_include_directories(rppg PRIVATE  ${MKL_INCLUDE})
        target_link_libraries(rppg PRIVATE $<LINK_ONLY:MKL::MKL>)
        target_compile_definitions(rppg PUBLIC  EIGEN_USE_MKL_ALL)
    endif()
endif()
        

set_target_properties(rppg PROPERTIES WIN32_EXECUTABLE TRUE)


add_executable(camera camera.cpp )
target_link_libraries(camera realsense)


set(DEPLOY_OPTIONS "${rppt_OUTPUT_NAME}.exe -verbose=2 --no-translations")
configure_file(${CMAKE_SOURCE_DIR}/cmake/DeployQt.cmake DeployQt.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/DeployDep.cmake DeployDep.cmake @ONLY)


set(INSTALL_EXECUTABLE rppg)
install(TARGETS ${INSTALL_EXECUTABLE} RUNTIME_DEPENDENCY_SET set_name)
install(RUNTIME_DEPENDENCY_SET set_name 
    DIRECTORIES ${INSTALL_DEP_PATHS}
    PRE_EXCLUDE_REGEXES "^qt6.*\.dll" "^api-ms-win-crt-.*\.dll" "*\.exe"
    POST_INCLUDE_REGEXES "[/\/]msvc[^/\/]*$" "[/\/]vc[^/\/]*$" 
    POST_EXCLUDE_REGEXES [[.*[/\/]system32[/\/].*\.dll]] [[.*[/\/]qt[/\/].*\.dll]]
)
get_target_property(OUT rppg LINK_LIBRARIES)
if("opencv_dnn" IN_LIST OUT)
    unset(debug_postfix)
    if(Porject_BUILD_TYPE STREQUAL debug)
        set(debug_postfix "d")
    endif()
    set(dnn_dep ${INSTALL_OPENCV_BIN}/inference_engine_ir_reader${debug_postfix}.dll
        ${INSTALL_OPENCV_BIN}/inference_engine_legacy${debug_postfix}.dll
        ${INSTALL_OPENCV_BIN}/inference_engine_lp_transformations${debug_postfix}.dll
        ${INSTALL_OPENCV_BIN}/MKLDNNPlugin${debug_postfix}.dll
        ${INSTALL_OPENCV_BIN}/plugins.xml)
    install(FILES ${dnn_dep} TYPE BIN)
    install(CODE "list(APPEND _CMAKE_DEPS ${dnn_dep})")
endif()
install(SCRIPT  ${CMAKE_CURRENT_BINARY_DIR}/DeployDep.cmake)


set(DEPLOY_OPTIONS "rppg.exe -verbose 0 --no-translations")
install(SCRIPT  ${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data TYPE BIN)