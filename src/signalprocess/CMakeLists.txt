add_executable(phy_signal_test EXCLUDE_FROM_ALL signal_test.cpp)
target_link_libraries(phy_signal_test PRIVATE phy_signal)

add_library(phy_signal OBJECT filter.cpp RPPGExtractor.cpp)
target_include_directories(phy_signal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(phy_signal PUBLIC Eigen3::Eigen MKL::MKL ${OpenCV_LIBS} )
target_compile_definitions(phy_signal PUBLIC EIGEN_MKL_DEFAULT)
