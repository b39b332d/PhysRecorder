
add_executable(utils_test EXCLUDE_FROM_ALL utils_test.cc )

target_link_libraries(utils_test PRIVATE  ${OpenCV_LIBS} ${delayimp_LIB})

    target_link_options(utils_test PRIVATE "/DELAYLOAD:opencv_imgproc4100.dll")
    target_link_options(utils_test PRIVATE "/DELAYLOAD:opencv_core4100.dll")