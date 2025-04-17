add_executable(face_tracking_test EXCLUDE_FROM_ALL face_tracking_test.cpp)
target_link_libraries(face_tracking_test PRIVATE face_tracking)

add_library(face_tracking OBJECT face_tracking.cpp inference.cpp kalman.cpp inference_openvino.cpp inference_dlib.cpp)
set_target_properties(face_tracking PROPERTIES AUTOMOC OFF)
set_target_properties(face_tracking PROPERTIES AUTOUIC OFF)
target_include_directories(face_tracking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${utils_dir})
target_link_libraries(face_tracking PUBLIC ${OpenCV_LIBS})
