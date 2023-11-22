
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
add_executable( PhyRecorder
    WIN32
    ${PROJECT_SOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
)

target_compile_definitions( PhyRecorder
    PRIVATE
    PROJECT_ROOT_PATH="./data/")

    
add_subdirectory(camera)
add_subdirectory(signalprocess)
add_subdirectory(utils)
add_subdirectory(VideoExporter)

target_include_directories(PhyRecorder PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")


target_link_libraries(PhyRecorder PRIVATE Qt6::Widgets ${OpenCV_LIBS} realsense  cam_detect MediaWriter)

if(MSVC)
target_link_libraries(PhyRecorder PRIVATE "Setupapi.lib") # Serial
endif()

if(EIGEN3_FOUND)
    target_link_libraries(PhyRecorder PRIVATE Eigen3::Eigen)
    if(TARGET MKL::MKL)
        set_target_properties(PhyRecorder PROPERTIES ENVIRONMENT "${MKL_ENV}")
        target_include_directories(PhyRecorder PRIVATE  ${MKL_INCLUDE})
        target_link_libraries(PhyRecorder PRIVATE $<LINK_ONLY:MKL::MKL>)
        target_compile_definitions(PhyRecorder PUBLIC  EIGEN_USE_MKL_ALL)
    endif()
endif()
        

set_target_properties(PhyRecorder PROPERTIES WIN32_EXECUTABLE TRUE)

custom_target_install(PhyRecorder)