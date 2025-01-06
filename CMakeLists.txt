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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(rPPG LANGUAGES CXX C)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL Debug OR "${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    set(Porject_BUILD_TYPE debug)
else()
    set(Porject_BUILD_TYPE release)
endif()

include(FetchContent )
set (FETCHCONTENT_QUIET FALSE)


cmake_host_system_information(RESULT ProcessorCount_N QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "CMake ${CMAKE_VERSION} using ${ProcessorCount_N} threads")
if(ProcessorCount_N EQUAL 0)
	set(ProcessorCount_N 4)
endif()

#### DOWNLOAD OPENCV REALSENSE QT DEVELOPMENT RUNTIME LIBRARY

# CHECK THESE VARIABLES BEFORE CONFIGURE
set(OpenCV_ROOT CACHE PATH "OpenCV directory which contain release and debug build")
set(Qt6_ROOT  CACHE PATH "Qt root directory")
set(Realsense_ROOT CACHE PATH "RealSense root directory")
set(MVis_ROOT  CACHE PATH "MindVision SDK Root directory")
set(MKL_DIR  CACHE PATH "MKL cmake directory")

set(ENIGMAVBCONXOLE_EXE CACHE PATH "ENIGMA_BOX executable path")
set(UPX_EXE CACHE PATH "Ultimate Packer for eXecutables executable path")

include("cmake/env.cmake" OPTIONAL)

set(INSTALL_DEP_PATHS "")


if(MSVC)
  # see Modules/CMakeGenericSystem.cmake
  if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
    set(PHY_ARCH "x64")
  elseif("${CMAKE_GENERATOR_PLATFORM}" MATCHES "ARM64")
    set(PHY_ARCH "ARM64")
  elseif("${CMAKE_GENERATOR}" MATCHES "ARM")
    set(PHY_ARCH "ARM")
  elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set(PHY_ARCH "x64")
  else()
    set(PHY_ARCH x86)
  endif()

  if(MSVC_VERSION EQUAL 1400)
    set(PHY_RUNTIME vc8)
  elseif(MSVC_VERSION EQUAL 1500)
    set(PHY_RUNTIME vc9)
  elseif(MSVC_VERSION EQUAL 1600)
    set(PHY_RUNTIME vc10)
  elseif(MSVC_VERSION EQUAL 1700)
    set(PHY_RUNTIME vc11)
  elseif(MSVC_VERSION EQUAL 1800)
    set(PHY_RUNTIME vc12)
  elseif(MSVC_VERSION EQUAL 1900)
    set(PHY_RUNTIME vc14)
  elseif(MSVC_VERSION MATCHES "^191[0-9]$")
    set(PHY_RUNTIME vc15)
  elseif(MSVC_VERSION MATCHES "^192[0-9]$")
    set(PHY_RUNTIME vc16)
  elseif(MSVC_VERSION MATCHES "^19[34][0-9]$")
    set(PHY_RUNTIME vc17)
  endif()
elseif(MINGW)
  set(PHY_RUNTIME mingw)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(PHY_ARCH x64)
  else()
    set(PHY_ARCH x86)
  endif()
endif()
####### FIND OPENCV
if(OpenCV_ROOT)
    set(OpenCV_DIR "${OpenCV_ROOT}/${Porject_BUILD_TYPE}")
endif()
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND AND DOWNLOAD_URL)
    FetchContent_Declare(
        OpenCV_Build
        URL "${DOWNLOAD_URL}/opencv4/${PHY_RUNTIME}/${Porject_BUILD_TYPE}_${PHY_ARCH}.tar.gz"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(OpenCV_Build)
    find_package(OpenCV REQUIRED NO_SYSTEM_ENVIRONMENT_PATH HINTS ${opencv_build_SOURCE_DIR})
endif()
if(NOT OpenCV_FOUND)
    message( FATAL_ERROR "OpenCV does not exist! CMake will exit." )
endif()

string(TOUPPER ${Porject_BUILD_TYPE} Porject_BUILD_TYPE_UPPER)
get_target_property(cv_core opencv_core IMPORTED_LOCATION_${Porject_BUILD_TYPE_UPPER})
get_filename_component(INSTALL_OPENCV_BIN "${cv_core}" DIRECTORY)
list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENCV_BIN})

####### FIND Qt
find_package(Qt6 COMPONENTS Widgets Gui Core)
if(NOT Qt6_FOUND AND DOWNLOAD_URL)
    FetchContent_Declare(
        Qt6_BUILD
        URL "${DOWNLOAD_URL}/qt6/${PHY_RUNTIME}/${PHY_ARCH}.tar.gz"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        OVERRIDE_FIND_PACKAGE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(Qt6_BUILD)
    set(Qt6_ROOT ${qt6_build_SOURCE_DIR})
    find_package(Qt6 COMPONENTS Widgets Gui Core REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
endif()
if(NOT Qt6_FOUND)
    message( FATAL_ERROR "Qt does not exist! CMake will exit." )
endif()

get_target_property(mocExe Qt6::moc IMPORTED_LOCATION)
get_filename_component(qtBinDir "${mocExe}" DIRECTORY)
find_program(DEPLOYQT_EXECUTABLE windeployqt PATHS "${qtBinDir}" NO_DEFAULT_PATH)

####### FIND MKL
set(MKL_LINK static)
find_package(MKL)

####### FIND Eigen3
FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY  https://gitlab.com/libeigen/eigen.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Eigen3)

######### FIND PACKEXE
if(Porject_BUILD_TYPE STREQUAL "release" AND WIN32)
    if(NOT ENIGMAVBCONXOLE_EXE OR NOT UPX_EXE)
        FetchContent_Declare(
            BOX_TOOLS
            URL  "${DOWNLOAD_URL}/box_tools.zip"
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            OVERRIDE_FIND_PACKAGE
            EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(BOX_TOOLS)
    
        set(ENIGMAVBCONXOLE_EXE "${box_tools_SOURCE_DIR}/enigmavbconsole.exe")
        set(UPX_EXE "${box_tools_SOURCE_DIR}/upx.exe")
    endif()
    add_custom_target(generate_boxed_exe
        COMMAND ${CMAKE_COMMAND} -DEXEC_NAME=PhyRecorder -DUPX_EXE=${UPX_EXE} -DEVB_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bin -P ${CMAKE_HOME_DIRECTORY}/cmake/configure_evb_file.cmake 
        COMMAND ${ENIGMAVBCONXOLE_EXE} ${CMAKE_INSTALL_PREFIX}/bin/PhyRecorder.evb
        DEPENDS install
        USES_TERMINAL
    )
endif()

####### FIND MSMF
find_package(MSMF)
####### FIND MVis
find_package(MVis)
####### FIND Realsense
find_package(Realsense)
list(APPEND INSTALL_DEP_PATHS "${Realsense_ROOT}/bin/x64/")

####### FIND libyuv
add_subdirectory(libyuv EXCLUDE_FROM_ALL)

####### build
include(cmake/install_target.cmake)
add_subdirectory(src)
add_subdirectory(data)

####### summary

get_target_property(qt6_lib_inc Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Qt6 library status:")
message(STATUS "    config: ${Qt6_DIR}")
message(STATUS "    version: ${Qt6_VERSION}")
message(STATUS "    core include path: ${qt6_lib_inc}")
message(STATUS "    moc path: ${DEPLOYQT_EXECUTABLE}")
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
if(MKL_FOUND)
    get_target_property(mkl_type MKL::MKL TYPE)
    message(STATUS "MKL library status:")
    message(STATUS "    config: ${MKL_ROOT}")
    message(STATUS "    version: ${MKL_VERSION}")
    message(STATUS "    include path: ${MKL_INCLUDE}")
endif()