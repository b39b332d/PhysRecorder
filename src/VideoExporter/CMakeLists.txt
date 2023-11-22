add_subdirectory(Encoder)

add_library(MediaWriter OBJECT MediaWriter.cpp "avi-utils.cc" "fileio.cc" "gwavi.cc")
target_link_libraries(MediaWriter PUBLIC opencv_core Encoder)
target_include_directories(MediaWriter PUBLIC ./)

add_executable(container_test  container_test.cpp)
target_link_libraries(container_test PRIVATE ${OpenCV_LIBS} MediaWriter)
target_include_directories(container_test PRIVATE ./)