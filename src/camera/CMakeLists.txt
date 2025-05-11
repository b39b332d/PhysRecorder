

set(RECORDER_SOURCES
        detect_cam.cpp
        qt_match_cam_msmf.cpp
)
add_library(cam_driver STATIC
"CameraDriver.cpp"
"CameraCapture.cpp"
)
add_subdirectory(decoder)
target_link_libraries(cam_driver PUBLIC cam_decoder)

target_include_directories(cam_driver PUBLIC ./ ${utils_dir}/)
set( CAM_DRIVER_FUNC ) 
add_subdirectory(MSMF)
add_subdirectory(MVis)
add_subdirectory(RS)
add_subdirectory(V4L2)
add_subdirectory(libcamera)
set( CAM_DRIVER_DEF "#include <vector>\nnamespace capture {class CameraDevice;") 
set( CAM_DRIVER_REG "std::vector<CameraDevice*>(*enum_drivers[])()={") 
foreach ( func_name IN LISTS CAM_DRIVER_FUNC )
string(APPEND CAM_DRIVER_DEF "std::vector<CameraDevice*> ${func_name}();")
string(APPEND CAM_DRIVER_REG "${func_name},")
endforeach()
string(APPEND CAM_DRIVER_REG "};}")
string(APPEND CAM_DRIVER_DEF "${CAM_DRIVER_REG}")

file(WRITE "DriverEnum.tmp" 
"${CAM_DRIVER_DEF}"
)


add_executable(cam_test EXCLUDE_FROM_ALL camera_test.cpp )
target_link_libraries(cam_test PRIVATE ${OpenCV_LIBS})
target_include_directories(cam_test PRIVATE ./)


add_executable(cam_test_rs EXCLUDE_FROM_ALL camera_test_RS.cpp )
target_link_libraries(cam_test_rs PRIVATE ${OpenCV_LIBS} realsense)
target_include_directories(cam_test_rs PRIVATE ./)


add_executable(cam_driver_test EXCLUDE_FROM_ALL cam_driver_test.cpp )
target_link_libraries(cam_driver_test PRIVATE ${OpenCV_LIBS} ${MSMF_LIBS} cam_driver)

