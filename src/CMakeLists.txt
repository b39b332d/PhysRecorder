

set(utils_dir ${CMAKE_CURRENT_SOURCE_DIR}/utils)

set(PROJECT_SOURCES
        mainwindow.cpp
        mainwindow.ui
        imageviewer.cpp

        converter.cpp
        capture.cpp

        signalprocess/signalprocess.cpp
        
        ${utils_dir}/cnpy.cpp
        ${utils_dir}/qcustomplot.cpp
        ${utils_dir}/MultiSelectComboBox.cpp
)
add_library( PhyRecorder_main SHARED
    ${PROJECT_SOURCES}
)

find_library(delayimp_LIB NAMES  delayimp REQUIRED)
set_target_properties(PhyRecorder_main PROPERTIES 
    AUTOUIC ON
    AUTOMOC ON
    AUTORCC ON
)

target_include_directories(PhyRecorder_main PRIVATE ./ ${utils_dir})

target_compile_definitions(PhyRecorder_main
    PRIVATE
    PROJECT_ROOT_PATH="./data/")

add_subdirectory(camera)
add_subdirectory(signalprocess)
add_subdirectory(serial)
add_subdirectory(recorder)
add_subdirectory(utils)
add_subdirectory(audio)

target_link_libraries(PhyRecorder_main PRIVATE  Qt6::Widgets  cam_driver phy_recorder serial phy_signal)




set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/data/resources/appicon.rc")
add_executable( PhyRecorder
    WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/data/resources/darkstyle.qrc
    ${APP_ICON_RESOURCE_WINDOWS}
)
set_target_properties(PhyRecorder PROPERTIES 
    RESOURCE "${CMAKE_SOURCE_DIR}/data/resources/icon.ico"
    WIN32_EXECUTABLE TRUE
    AUTOUIC ON
    AUTOMOC ON
    AUTORCC ON
)
target_link_libraries(PhyRecorder PRIVATE PhyRecorder_main Qt6::Widgets  ${delayimp_LIB})
target_compile_definitions( PhyRecorder
    PRIVATE
    PROJECT_ROOT_PATH="./data/")
target_link_options(PhyRecorder PRIVATE "/DELAYLOAD:PhyRecorder_main.dll")
custom_target_install(PhyRecorder)