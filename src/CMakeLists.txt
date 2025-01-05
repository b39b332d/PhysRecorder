
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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


 find_library(delayimp_LIB NAMES  delayimp REQUIRED)
set_target_properties(PhyRecorder PROPERTIES 
    RESOURCE "${CMAKE_SOURCE_DIR}/data/resources/icon.ico"
    WIN32_EXECUTABLE TRUE
    AUTOUIC ON
    AUTOMOC ON
    AUTORCC ON
)

target_include_directories(PhyRecorder PRIVATE ./ ${utils_dir})

target_compile_definitions( PhyRecorder
    PRIVATE
    PROJECT_ROOT_PATH="./data/")

add_subdirectory(camera)
add_subdirectory(signalprocess)
add_subdirectory(serial)
add_subdirectory(recorder)
add_subdirectory(utils)
add_subdirectory(audio)

target_link_libraries(PhyRecorder PRIVATE Qt6::Widgets  cam_driver phy_recorder serial phy_signal ${delayimp_LIB})

if(Porject_BUILD_TYPE STREQUAL "release")
    string(REPLACE "." "" OpenCV_VERSION_CP ${OpenCV_VERSION})
    #target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_core${OpenCV_VERSION_CP}.dll")
    #target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_imgproc${OpenCV_VERSION_CP}.dll")
    #target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:$<TARGET_LINKER_FILE:cam_driver>.dll")
endif()
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:cam_driver.dll")
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_imgproc4100.dll")
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_imgcodecs4100.dll")
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_imgproc4100.dll")
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:opencv_core4100.dll")
    target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:openvinod.dll")
        target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:realsense2.dll")

custom_target_install(PhyRecorder)