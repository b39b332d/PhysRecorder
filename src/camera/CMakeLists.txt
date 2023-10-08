
set(RECORDER_SOURCES
        detect_cam.cpp
        qt_match_cam_msmf.cpp
)
 find_library(mfplat NAMES  mfplat REQUIRED)
 find_library(mf NAMES  mf REQUIRED)
 find_library(mfuuid NAMES  mfuuid REQUIRED)
 find_library(mrw NAMES  mfreadwrite REQUIRED)
add_library(cam_detect OBJECT
    ${RECORDER_SOURCES}
)

target_link_libraries(cam_detect PRIVATE Qt6::Multimedia ${OpenCV_LIBS} ${mfplat} ${mf} ${mfuuid} ${mrw})
target_include_directories(cam_detect PUBLIC ./)