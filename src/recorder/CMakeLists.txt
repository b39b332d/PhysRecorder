add_subdirectory(encoder)

add_library(phy_recorder OBJECT MediaWriter.cpp "avi-utils.cc" "fileio.cc" "gwavi.cc")
set_target_properties(phy_recorder PROPERTIES AUTOMOC OFF)
set_target_properties(phy_recorder PROPERTIES AUTOUIC OFF)
target_link_libraries(phy_recorder PUBLIC opencv_core phy_encoder "$<TARGET_OBJECTS:phy_encoder>")
target_include_directories(phy_recorder PUBLIC ./ ${utils_dir})

add_executable(container_test EXCLUDE_FROM_ALL container_test.cpp)
target_link_libraries(container_test PRIVATE ${OpenCV_LIBS} phy_recorder )
target_include_directories(container_test PRIVATE ./)