# new features
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_INSTALL_PREFIX}/bin)
SET(CMAKE_CXX_FLAGS_RELEASE "/O2")

set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD on)

get_property(GENERATOR_IS_MULTI_CONFIG_VAR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT GENERATOR_IS_MULTI_CONFIG_VAR AND NOT DEFINED CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not defined, 'Release' will be used")
    # //Setting CMAKE_BUILD_TYPE as CACHE must go before project(). Otherwise project() sets its value and set() doesn't take an effect
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...")
endif()

project(rPPG LANGUAGES CXX C)

if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL Debug OR "${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    set(Porject_BUILD_TYPE debug)
else()
    set(Porject_BUILD_TYPE release)
endif()



cmake_host_system_information(RESULT ProcessorCount_N QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "CMake ${CMAKE_VERSION} using ${ProcessorCount_N} threads")
if(ProcessorCount_N EQUAL 0)
	set(ProcessorCount_N 4)
endif()

#### DOWNLOAD OPENCV REALSENSE QT DEVELOPMENT RUNTIME LIBRARY

# CHECK THESE VARIABLES BEFORE CONFIGURE
set(OpenCV_BUILD_Path_release CACHE PATH "OpenCV cmake release directory")
set(OpenCV_BUILD_Path_debug CACHE PATH "OpenCV cmake debug directory")
set(OpenCV_BUILD_Path CACHE PATH "OpenCV cmake directory")
set(Eigen3_DIR CACHE PATH "Eigen3 cmake directory")
set(Qt6_ROOT  CACHE PATH "Qt root directory")
set(REALSENSE_DIR CACHE PATH "RealSense root directory")
set(OpenVINO_DIR  CACHE PATH "OpenVINO Runtime directory")
set(MKL_DIR  CACHE PATH "MKL root directory")
set(ENIGMA_BOX_CONXOLE_EXE CACHE PATH "ENIGMA_BOX executable path")
set(UPX_PACK_EXE CACHE PATH "upx package executable path")

include("cmake/env.cmake" OPTIONAL)

set(MKL_LINK dynamic)
set(INSTALL_DEP_PATHS "")

####### FIND OPENCV
set(OpenCV_DIR "${OpenCV_BUILD_Path_${Porject_BUILD_TYPE}}")
if("${OpenCV_DIR}" STREQUAL "")
    set(OpenCV_DIR "${OpenCV_BUILD_Path}")
endif()
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
####### FIND Qt
find_package(Qt6 COMPONENTS Widgets Gui Core Multimedia REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
get_target_property(mocExe Qt6::moc IMPORTED_LOCATION)
get_filename_component(qtBinDir "${mocExe}" DIRECTORY)
find_program(DEPLOYQT_EXECUTABLE windeployqt PATHS "${qtBinDir}" NO_DEFAULT_PATH)

####### FIND REALSENSE
add_library(realsense SHARED IMPORTED )
set_target_properties(realsense PROPERTIES
             IMPORTED_LOCATION ${REALSENSE_DIR}/bin/x64/realsense2.dll
             IMPORTED_IMPLIB ${REALSENSE_DIR}/lib/x64/realsense2.lib)
target_include_directories(realsense INTERFACE ${REALSENSE_DIR}/include)
set(INSTALL_REALSENSE_BIN "${REALSENSE_DIR}/bin/x64/")
list(APPEND INSTALL_DEP_PATHS "${INSTALL_REALSENSE_BIN}")

list(APPEND INSTALL_DEP_PATHS "${OpenVINO_DIR}/bin/intel64/${Porject_BUILD_TYPE}")
list(APPEND INSTALL_DEP_PATHS "${OpenVINO_DIR}/3rdparty/tbb/bin")


find_package(MKL REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
get_target_property(mkl_type MKL::MKL TYPE)
if (MKL_LINK STREQUAL dynamic)
    list(APPEND INSTALL_DEP_PATHS ${MKL_ENV_PATH})
endif()
message(STATUS "MKL library status:")
message(STATUS "    config: ${MKL_ROOT}")
message(STATUS "    version: ${PACKAGE_VERSION}")
message(STATUS "    include path: ${MKL_INCLUDE}")
list(APPEND INSTALL_DEP_PATHS "${MKL_DIR}/lib/intel64")
list(APPEND INSTALL_DEP_PATHS "${MKL_DIR}/redist/intel64")

####### FIND Eigen3
find_package(Eigen3 REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)

add_subdirectory(libyuv)
include(cmake/install_target.cmake)
add_subdirectory(src)
add_subdirectory(data)

if(EXISTS ${ENIGMA_BOX_CONXOLE_EXE})
add_custom_target(generate_boxed_exe
    COMMAND ${CMAKE_COMMAND} -DUPX_EXE=${UPX_PACK_EXE} -DEVB_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bin -P ${CMAKE_HOME_DIRECTORY}/cmake/configure_evb_file.cmake 
    COMMAND ${ENIGMA_BOX_CONXOLE_EXE} ${CMAKE_INSTALL_PREFIX}/bin/PhyRecorder.evb
    DEPENDS install
    USES_TERMINAL
)
endif()