

set(utils_dir ${CMAKE_CURRENT_SOURCE_DIR}/utils)

set(PROJECT_SOURCES
        mainwindow.cpp
        worker_handler.cpp
        video_ui.cpp
        mainwindow.ui
        video.ui
        imageviewer.cpp

        converter.cpp
        capture.cpp

        signalprocess/signalprocess.cpp
        imageprocess/color_extractor.hpp
        
        ${utils_dir}/cnpy.cpp
        ${utils_dir}/qcustomplot.cpp
        ${utils_dir}/MultiSelectComboBox.cpp
        ${utils_dir}/SilentLineEdit.cpp
)
add_library( PhysRecorder_main SHARED
    ${PROJECT_SOURCES}
)
if (MSVC)
    find_library(delayimp_LIB NAMES  delayimp REQUIRED)
endif()
set_target_properties(PhysRecorder_main PROPERTIES 
    AUTOUIC ON
    AUTOMOC ON
    AUTORCC ON
)

target_include_directories(PhysRecorder_main PRIVATE ./ ${utils_dir} )

add_subdirectory(camera)
add_subdirectory(signalprocess)
add_subdirectory(imageprocess)
add_subdirectory(serial)
add_subdirectory(recorder)
add_subdirectory(utils)
add_subdirectory(audio)

set(dep_target cam_driver phy_recorder phy_serial phy_signal face_tracking)
target_link_libraries(PhysRecorder_main PRIVATE  Qt6::Widgets  ${dep_target})
set_target_properties(PhysRecorder_main PROPERTIES 
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_SOURCE_DIR}/data/resources/appicon.rc")
add_executable( PhysRecorder
    WIN32
    main.cpp
    ${CMAKE_SOURCE_DIR}/data/styles/darkstyle.qrc
    ${APP_ICON_RESOURCE_WINDOWS}
)
set_target_properties(PhysRecorder PROPERTIES 
    RESOURCE "${CMAKE_SOURCE_DIR}/data/resources/icon.ico"
    WIN32_EXECUTABLE TRUE
    AUTOUIC ON
    AUTOMOC ON
    AUTORCC ON
)
if(OpenVINOLIB_FOUND)
    target_compile_definitions(PhysRecorder_main PRIVATE HAS_OPENVINO)
endif()
if(WIN32)
    target_link_libraries(PhysRecorder PRIVATE PhysRecorder_main Qt6::Widgets ${delayimp_LIB})
else()
    target_link_libraries(PhysRecorder PRIVATE Qt6::Widgets)
    add_dependencies(PhysRecorder PhysRecorder_main)
endif()

if (MSVC)
    target_link_options(PhysRecorder PRIVATE "/DELAYLOAD:PhysRecorder_main.dll")
endif()
custom_target_install(PhysRecorder)
