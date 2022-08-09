# new features
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_INSTALL_PREFIX  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
SET(CMAKE_CXX_FLAGS_RELEASE "/O2")
set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD on)

get_property(GENERATOR_IS_MULTI_CONFIG_VAR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT GENERATOR_IS_MULTI_CONFIG_VAR AND NOT DEFINED CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not defined, 'Release' will be used")
    # //Setting CMAKE_BUILD_TYPE as CACHE must go before project(). Otherwise project() sets its value and set() doesn't take an effect
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...")
endif()

project(rPPG LANGUAGES CXX)

if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL Debug OR "${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    set(Porject_BUILD_TYPE debug)
else()
    set(Porject_BUILD_TYPE release)
endif()


#### DOWNLOAD OPENCV REALSENSE QT DEVELOPMENT RUNTIME LIBRARY
set(USE_EIGEN off)
# CHECK THESE VARIABLES BEFORE CONFIGURE
set(OpenCV_DIR "C:/Users/b39b3/Documents/SRC/opencv/opencv-${Porject_BUILD_TYPE}/build/cmake" CACHE PATH "OpenCV cmake directory" FORCE)
set(Qt6_ROOT "D:/Qt/6.3.1/msvc2019_64" CACHE PATH "Qt root directory" FORCE)
set(REALSENSE_DIR "C:/Program Files (x86)/Intel RealSense SDK 2.0" CACHE PATH "RealSense root directory" FORCE)

set(Eigen3_DIR "C:/Users/b39b3/Documents/SRC/SignalProcessor/lib/share/eigen3" CACHE PATH "Eigen root directory" FORCE)
set(MKL_DIR "C:/Program Files (x86)/Intel/oneAPI/mkl/2022.1.0" CACHE PATH "Qt root directory" FORCE)
set(MKL_LINK dynamic)
set(INSTALL_DEP_PATHS "")
####### FIND OPENCV
find_package(OpenCV REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
if(TARGET opencv_core) 
    string(TOUPPER ${Porject_BUILD_TYPE} Porject_BUILD_TYPE_UPPER)
    get_target_property(cv_core opencv_core IMPORTED_LOCATION_${Porject_BUILD_TYPE_UPPER})
    get_filename_component(INSTALL_OPENCV_BIN "${cv_core}" DIRECTORY)
    list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENCV_BIN})
    message(STATUS "OpenCV library status:")
    message(STATUS "    config: ${OpenCV_DIR}")
    message(STATUS "    version: ${OpenCV_VERSION}")
    message(STATUS "    libraries: ${OpenCV_LIBS}")
    message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
else()
    message( FATAL_ERROR "opencv_core does not exist! CMake will exit." )
endif()

if(NOT USE_EIGEN)
    ####### FIND MKL
    find_package(MKL REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
    get_target_property(mkl_type MKL::MKL TYPE)
    if (MKL_LINK STREQUAL dynamic)
        list(APPEND INSTALL_DEP_PATHS ${MKL_ENV_PATH})
    endif()
    message(STATUS "MKL library status:")
    message(STATUS "    config: ${MKL_ROOT}")
    message(STATUS "    version: ${PACKAGE_VERSION}")
    message(STATUS "    include path: ${MKL_INCLUDE}")

    ####### FIND Eigen3
    find_package(Eigen3 REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
endif()

####### FIND Qt
find_package(Qt6 COMPONENTS Widgets Gui Core REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
get_target_property(mocExe Qt6::moc IMPORTED_LOCATION)
get_filename_component(qtBinDir "${mocExe}" DIRECTORY)
find_program(DEPLOYQT_EXECUTABLE windeployqt PATHS "${qtBinDir}" NO_DEFAULT_PATH)

####### FIND REALSENSE
add_library(realsense SHARED IMPORTED )
set_target_properties(realsense PROPERTIES
             IMPORTED_LOCATION ${REALSENSE_DIR}/bin/x64/realsense2.dll
             IMPORTED_IMPLIB ${REALSENSE_DIR}/lib/x64/realsense2.lib)
target_include_directories(realsense INTERFACE "C:/Program Files (x86)/Intel RealSense SDK 2.0/include")
set(INSTALL_REALSENSE_BIN ${REALSENSE_DIR}/bin/x64/)
list(APPEND INSTALL_DEP_PATHS ${INSTALL_REALSENSE_BIN})

add_subdirectory(src)
add_subdirectory(data)
