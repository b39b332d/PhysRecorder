add_executable(phy_signal_test EXCLUDE_FROM_ALL signal_test.cpp)
target_link_libraries(phy_signal_test PRIVATE phy_signal)

add_library(phy_signal OBJECT filter.cpp RPPGExtractor.cpp)
set_target_properties(phy_signal PROPERTIES AUTOMOC OFF)
set_target_properties(phy_signal PROPERTIES AUTOUIC OFF)
target_include_directories(phy_signal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(phy_signal PUBLIC Eigen3::Eigen ${OpenCV_LIBS})

if(MKL_FOUND)
    target_link_libraries(phy_signal PUBLIC ${MKL_LIBRARIES} )
    target_include_directories(phy_signal PUBLIC ${MKL_INCLUDE_DIR})
    target_compile_definitions(phy_signal PUBLIC EIGEN_MKL_DEFAULT EIGEN_VECTORIZE_SSE4_2)
endif()