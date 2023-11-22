
set(RECORDER_SOURCES
        detect_cam.cpp
        qt_match_cam_msmf.cpp
)
 find_library(mfplat_LIB NAMES  mfplat REQUIRED)
 find_library(mf_LIB NAMES  mf REQUIRED)
 find_library(mfuuid_LIB NAMES  mfuuid REQUIRED)
 find_library(mrw_LIB NAMES  mfreadwrite REQUIRED)
 set(MSMF_LIBS ${mfplat_LIB} ${mf_LIB} ${mfuuid_LIB} ${mrw_LIB})
add_library(cam_detect OBJECT
    ${RECORDER_SOURCES}
)

target_link_libraries(cam_detect PRIVATE Qt6::Multimedia ${OpenCV_LIBS} ${MSMF_LIBS})
target_include_directories(cam_detect PUBLIC ./)

add_executable(cam_test EXCLUDE_FROM_ALL camera_test.cpp )
target_link_libraries(cam_test PRIVATE ${OpenCV_LIBS} ${MSMF_LIBS})
target_include_directories(cam_test PRIVATE ./)


add_executable(cam_test_rs EXCLUDE_FROM_ALL camera_test_RS.cpp )
target_link_libraries(cam_test_rs PRIVATE ${OpenCV_LIBS} realsense)
target_include_directories(cam_test_rs PRIVATE ./)


add_executable(cam_driver_test EXCLUDE_FROM_ALL cam_driver_test.cpp CameraDriverMSMF.cpp)
target_link_libraries(cam_driver_test PRIVATE ${OpenCV_LIBS} ${MSMF_LIBS})
target_include_directories(cam_driver_test PRIVATE ./)