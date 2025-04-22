add_library(cam_decoder OBJECT
"ImageDecoder.cpp"
)

target_include_directories(cam_decoder PUBLIC ./ ${utils_dir}/ ../)
target_link_libraries(cam_decoder PUBLIC ${OpenCV_LIBS} yuv)

add_executable(cam_decoder_test EXCLUDE_FROM_ALL ImageDecoder_test.cpp )
target_link_libraries(cam_decoder_test PRIVATE cam_decoder cam_driver)
