
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(utils_dir ${CMAKE_CURRENT_SOURCE_DIR}/utils)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.ui
        imageviewer.cpp

        converter.cpp
        capture.cpp

        signalprocess/signalprocess.cpp
        
        ${utils_dir}/cnpy.cpp
        ${utils_dir}/qcustomplot.cpp
        ${utils_dir}/MultiSelectComboBox.cpp
        ${CMAKE_SOURCE_DIR}/data/resources/darkstyle.qrc
)
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/data/resources/appicon.rc")
add_executable( PhyRecorder
    WIN32
    ${PROJECT_SOURCES}
    ${APP_ICON_RESOURCE_WINDOWS}
)

set_target_properties(PhyRecorder PROPERTIES 
    WIN32_EXECUTABLE 1 
    RESOURCE "${CMAKE_SOURCE_DIR}/data/resources/icon.ico"
)
target_include_directories(PhyRecorder PRIVATE ./ ${utils_dir})

target_compile_definitions( PhyRecorder
    PRIVATE
    PROJECT_ROOT_PATH="./data/")

add_subdirectory(camera)
add_subdirectory(signalprocess)
add_subdirectory(serial)
add_subdirectory(recorder)

target_link_libraries(PhyRecorder PRIVATE Qt6::Widgets ${OpenCV_LIBS} 
realsense cam_driver phy_recorder serial phy_signal)

set_target_properties(PhyRecorder PROPERTIES WIN32_EXECUTABLE TRUE)

custom_target_install(PhyRecorder)