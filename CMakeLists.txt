# new features
cmake_minimum_required(VERSION 3.21)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 20)
if(WIN32)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
else()
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install/usr)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib)
list(APPEND CMAKE_BUILD_RPATH "$ORIGIN/../lib")
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
if(MSVC)
  SET(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
  SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()


set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD on)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
get_property(GENERATOR_IS_MULTI_CONFIG_VAR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT GENERATOR_IS_MULTI_CONFIG_VAR AND NOT DEFINED CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not defined, 'Release' will be used")
  # //Setting CMAKE_BUILD_TYPE as CACHE must go before project(). Otherwise project() sets its value and set() doesn't take an effect
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(DEFINED ENV{CONDA_PREFIX})
  list(PREPEND CMAKE_LIBRARY_PATH $ENV{CONDA_PREFIX}/lib)
  list(PREPEND CMAKE_INCLUDE_PATH $ENV{CONDA_PREFIX}/include)
endif()

project(rPPG LANGUAGES CXX C)
if(WIN32)
  set(CMAKE_INSTALL_LIBDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()
if("${CMAKE_CONFIGURATION_TYPES}" STREQUAL Debug OR "${CMAKE_BUILD_TYPE}" STREQUAL Debug)
  set(Porject_BUILD_TYPE debug)
else()
  set(Porject_BUILD_TYPE release)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

include(ExternalProject)

cmake_host_system_information(RESULT ProcessorCount_N QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "CMake ${CMAKE_VERSION} using ${ProcessorCount_N} threads")
if(ProcessorCount_N EQUAL 0)
  set(ProcessorCount_N 4)
endif()

#### DOWNLOAD OPENCV REALSENSE QT DEVELOPMENT RUNTIME LIBRARY

# CHECK THESE VARIABLES BEFORE CONFIGURE
set(OpenCV_ROOT CACHE PATH "OpenCV directory which contain release and debug build")
set(OpenVINO_ROOT CACHE PATH "OpenVINO directory point to runtime directory")
set(Qt6_ROOT CACHE PATH "Qt root directory")
set(Realsense_ROOT CACHE PATH "RealSense root directory")
set(MVis_ROOT CACHE PATH "MindVision SDK Root directory")
set(MKL_ROOT CACHE PATH "MKL cmake directory")
set(LIBYUV_PATH CACHE PATH "libyuv.dll and include path")

set(ENIGMAVBCONXOLE_EXE CACHE PATH "ENIGMA_BOX executable path")
set(UPX_EXE CACHE PATH "Ultimate Packer for eXecutables executable path")

include("cmake/env.cmake" OPTIONAL)

set(INSTALL_DEP_PATHS "")


if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
set(PHY_ARCH "x64")
elseif("${CMAKE_GENERATOR_PLATFORM}" MATCHES "ARM64")
set(PHY_ARCH "ARM64")
elseif("${CMAKE_GENERATOR}" MATCHES "ARM")
set(PHY_ARCH "ARM")
elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
set(PHY_ARCH "x64")
else()
set(PHY_ARCH "x86")
endif()
if(MSVC)
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
find_package(OpenCV QUIET PATH_SUFFIXES ${Porject_BUILD_TYPE}/build/cmake  ${Porject_BUILD_TYPE}/build build ${Porject_BUILD_TYPE})
if(NOT OpenCV_FOUND AND DOWNLOAD_URL)
  FetchContent_Declare(
    OpenCV_Build
    URL "${DOWNLOAD_URL}/opencv4/${PHY_RUNTIME}/${Porject_BUILD_TYPE}_${PHY_ARCH}.tar.gz"
  )
  FetchContent_MakeAvailable(OpenCV_Build)
  find_package(OpenCV REQUIRED NO_SYSTEM_ENVIRONMENT_PATH HINTS ${opencv_build_SOURCE_DIR})
endif()
if(NOT OpenCV_FOUND)
  message(FATAL_ERROR "OpenCV does not exist! CMake will exit.")
endif()

string(TOUPPER ${Porject_BUILD_TYPE} Porject_BUILD_TYPE_UPPER)
get_target_property(cv_core_config opencv_core IMPORTED_CONFIGURATIONS)
get_target_property(cv_core opencv_core IMPORTED_LOCATION_${cv_core_config})
get_filename_component(INSTALL_OPENCV_BIN "${cv_core}" DIRECTORY)
list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENCV_BIN})

####### FIND OPENVINO
find_package(OpenVINOLIB)
####### FIND Qt
find_package(Qt6 COMPONENTS Widgets Gui Core)
if(NOT Qt6_FOUND AND DOWNLOAD_URL)
  FetchContent_Declare(
    Qt6_BUILD
    URL "${DOWNLOAD_URL}/qt6/${PHY_RUNTIME}/${PHY_ARCH}.tar.gz"
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(Qt6_BUILD)
  set(Qt6_ROOT ${qt6_build_SOURCE_DIR})
  find_package(Qt6 COMPONENTS Widgets Gui Core REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
endif()
if(NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt does not exist! CMake will exit.")
endif()

get_target_property(mocExe Qt6::moc IMPORTED_LOCATION)
get_filename_component(qtBinDir "${mocExe}" DIRECTORY)
if(WIN32)
  find_program(DEPLOYQT_EXECUTABLE windeployqt PATHS "${qtBinDir}" PATH_SUFFIXES bin NO_DEFAULT_PATH)
elseif(UNIX)
  find_program(QMAKE_EXECUTABLE qmake PATHS "${qtBinDir}" PATH_SUFFIXES bin NO_DEFAULT_PATH REQUIRED)
  find_program(DEPLOYQT_EXECUTABLE linuxdeployqt)
  if(NOT ${DEPLOYQT_EXECUTABLE})
    set(LINUXDEPLOYQT_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/linuxdeployqt)
    ExternalProject_Add(
      linuxdeployqt_external
      GIT_REPOSITORY https://gitcode.com/gh_mirrors/li/linuxdeployqt.git
      GIT_TAG origin/master
      PREFIX ${LINUXDEPLOYQT_PREFIX}
      CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=Release
      -DQT_HOST_PATH=${QT_HOST_PATH}
      -DQT_REQUIRE_HOST_PATH_CHECK=${QT_REQUIRE_HOST_PATH_CHECK}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${LINUXDEPLOYQT_PREFIX}
      BUILD_BYPRODUCTS ${LINUXDEPLOYQT_PREFIX}/linuxdeployqt
      INSTALL_COMMAND ""
      LOG_CONFIGURE ON
      LOG_BUILD ON
      LOG_INSTALL ON
    )
    set(DEPLOYQT_EXECUTABLE "${LINUXDEPLOYQT_PREFIX}/linuxdeployqt")
  endif()
endif()
####### FIND MKL
if(NOT MKL_FOUND)
  set(USE_STATIC_MKL ON CACHE INTERNAL "")
endif()
set(MKL_THREADING SEQ CACHE INTERNAL "")
find_package(MKL)
if(NOT MKL_FOUND)
  set(USE_STATIC_MKL OFF CACHE INTERNAL "" FORCE)
  find_package(MKL)
endif()

if(MKL_FOUND)
  get_filename_component(MKL_BIN "${MKL_LIBRARIES_mkl_core_LIBRARY}" DIRECTORY)
  get_filename_component(MKL_BIN "${MKL_BIN}" DIRECTORY)
  list(APPEND INSTALL_DEP_PATHS ${MKL_BIN}/bin)
endif()

####### FIND Eigen3
FetchContent_Declare(
  Eigen3
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG master
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
)
if(MKL_FOUND)
  set(EIGEN_BUILD_LAPACK ON CACHE INTERNAL "")
  set(EIGEN_BUILD_BLAS ON CACHE INTERNAL "")
  set(EIGEN_USE_MKL_ALL ON CACHE INTERNAL "")
endif()
if(NOT Eigen3_POPULATED)
  FetchContent_Populate(Eigen3)
  add_subdirectory(${eigen3_SOURCE_DIR} ${eigen3_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


######### FIND PACKEXE

if(CMAKE_GENERATOR STREQUAL Ninja)
    set(dep_install DEPENDS install)
else()
    set(dep_install COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target install)
endif()
if(Porject_BUILD_TYPE STREQUAL "release")
  if(WIN32)
    if(NOT ENIGMAVBCONXOLE_EXE OR NOT UPX_EXE)
      set(BOX_TOOLS_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/box_tools)
      if(DOWNLOAD_URL)
        ExternalProject_Add(
          BOX_TOOLS
          URL "${DOWNLOAD_URL}/box_tools.zip"
          CONFIGURE_COMMAND ""
          BUILD_COMMAND  ""
          INSTALL_COMMAND ""
          PREFIX ${BOX_TOOLS_PREFIX}
          EXCLUDE_FROM_ALL
        )

        set(ENIGMAVBCONXOLE_EXE "${BOX_TOOLS_PREFIX}/src/BOX_TOOLS/enigmavbconsole.exe")
        set(UPX_EXE "${BOX_TOOLS_PREFIX}/src/BOX_TOOLS/upx.exe")
      endif()
    endif()
    if(ENIGMAVBCONXOLE_EXE AND UPX_EXE)
      add_custom_target(generate_boxed_exe
        ${dep_install}
        COMMAND ${CMAKE_COMMAND} -DEXEC_NAME=PhysRecorder -DUPX_EXE=${UPX_EXE} -DEVB_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bin -P ${CMAKE_HOME_DIRECTORY}/cmake/configure_evb_file.cmake
        COMMAND ${ENIGMAVBCONXOLE_EXE} ${CMAKE_INSTALL_PREFIX}/bin/PhysRecorder.evb
        USES_TERMINAL
      )
      set_target_properties(generate_boxed_exe PROPERTIES EXCLUDE_FROM_ALL ON)
      if(TARGET BOX_TOOLS)
        add_dependencies(generate_boxed_exe BOX_TOOLS)
      endif()
    endif()
  elseif(UNIX)
  if(PHY_ARCH STREQUAL x64)
  set(rt_file_type x86_64)
  elseif(PHY_ARCH STREQUAL ARM64)
  set(rt_file_type aarch64)
  elseif(PHY_ARCH STREQUAL ARM)
  set(rt_file_type armhf)
  elseif(PHY_ARCH STREQUAL x86)
  set(rt_file_type i686)
  endif()

    find_program(APPIMAGE_PACK_EXE appimagetool)
    set(APPIMAGE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/appimage)
    ExternalProject_Add(
      appimage_runtime
      URL https://master.dl.sourceforge.net/project/appimagekit.mirror/13/runtime-${rt_file_type}
      URL_MD5 37d6f0bc41f143c8c0376e874769e20a
      CONFIGURE_COMMAND ""
      BUILD_COMMAND  ""
      INSTALL_COMMAND ""
      DOWNLOAD_NAME runtime
      PREFIX ${APPIMAGE_PREFIX}
      EXCLUDE_FROM_ALL ON
      DOWNLOAD_NO_EXTRACT  ON
    )
    set(APPIMAGE_DEPS  appimage_runtime)
    if(NOT APPIMAGE_PACK_EXE)
      set(APPIMAGE_PACK_EXE "${APPIMAGE_PREFIX}/src/appimagetool")
      ExternalProject_Add(
        appimage_bin
        URL https://master.dl.sourceforge.net/project/appimagekit.mirror/13/appimagetool-${rt_file_type}.AppImage
        URL_MD5 8897f478bb7b701fcd107503a08f62c4
        CONFIGURE_COMMAND ""
        BUILD_COMMAND  ""
        INSTALL_COMMAND chmod +x ${APPIMAGE_PACK_EXE}
        DOWNLOAD_NAME appimagetool
        PREFIX ${APPIMAGE_PREFIX}
        EXCLUDE_FROM_ALL ON
        DOWNLOAD_NO_EXTRACT  ON
      )
      list(APPEND APPIMAGE_DEPS appimage_bin)
    endif()
    add_custom_target(generate_boxed_exe
        ${dep_install}
        COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target install
        COMMAND ${APPIMAGE_PACK_EXE} ./ ./cn.edu.njust.nbslab.PhysRecorder.appimage --runtime-file ${APPIMAGE_PREFIX}/src/runtime
        WORKING_DIRECTORY  ${CMAKE_INSTALL_PREFIX}/..
        USES_TERMINAL
    )
    add_dependencies(generate_boxed_exe ${APPIMAGE_DEPS})

  endif()
endif()
####### FIND MSMF
find_package(MSMF)
####### FIND MVis
find_package(MVis)
####### FIND Realsense
find_package(Realsense)
####### Find V4L2 and libcamera
if(UNIX)
    find_package(V4L2)
    find_package(libcamera)
endif()

####### FIND libyuv
find_package(libyuv)


####### Find serial
FetchContent_Declare(
  libserial
  GIT_REPOSITORY https://gitcode.com/gh_mirrors/se/serial.git
  GIT_SHALLOW ON
  GIT_TAG main
  EXCLUDE_FROM_ALL
)
FetchContent_GetProperties(libserial)
if(NOT libserial_POPULATED)
  FetchContent_Populate(libserial)
  add_subdirectory(${libserial_SOURCE_DIR} ${libserial_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
target_include_directories(serial PUBLIC "${libserial_SOURCE_DIR}/include/")
####### Find dlib
FetchContent_Declare(
  dlib
  URL https://pypi.tuna.tsinghua.edu.cn/packages/14/4c/8444567b0b82b0c146fd746f96751ffc7feb25e5ed628c35b88b9673c2ff/dlib-19.24.8.tar.gz
  URL_HASH SHA256=9135900983e279b5e5d0b1bb31b684a3054ea4f3ea1b1c69e8a675659271003d
  SOURCE_SUBDIR dlib
  EXCLUDE_FROM_ALL
)

set(DLIB_NO_GUI_SUPPORT ON CACHE INTERNAL "")
set(DLIB_PNG_SUPPORT OFF CACHE INTERNAL "")
set(DLIB_GIF_SUPPORT OFF CACHE INTERNAL "")
set(DLIB_JPEG_SUPPORT OFF CACHE INTERNAL "")
set(DLIB_USE_MKL_FFT OFF CACHE INTERNAL "")
set(DLIB_USE_FFMPEG OFF CACHE INTERNAL "")
if(MKL_FOUND)
  set(DLIB_USE_LAPACK ON CACHE INTERNAL "")
  set(DLIB_USE_BLAS ON CACHE INTERNAL "")
  set(DLIB_USE_MKL_SEQUENTIAL ON CACHE INTERNAL "")
else()
  set(DLIB_USE_LAPACK OFF CACHE INTERNAL "")
  set(DLIB_USE_BLAS OFF CACHE INTERNAL "")
  set(DLIB_USE_MKL_SEQUENTIAL CACHE INTERNAL "")
endif()

FetchContent_MakeAvailable(dlib)
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
get_target_property(yuv_include_dirs yuv INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "libyuv library status:")
message(STATUS "    build: ${libyuv_status}")
message(STATUS "    include path: ${yuv_include_dirs}")
message(STATUS "dlib library status:")
message(STATUS "    version: ${DLIB_VERSION}")
message(STATUS "    lapack: ${DLIB_USE_LAPACK}")
message(STATUS "Eigen library status:")
message(STATUS "    lapack: ${EIGEN_BUILD_LAPACK}")
message(STATUS "    version: ${EIGEN_VERSION_NUMBER}")
message(STATUS "    include path: ${INCLUDE_INSTALL_DIR}")
if(MKL_FOUND)
  message(STATUS "MKL library status:")
  message(STATUS "    threading: ${MKL_THREADING}")
  message(STATUS "    version: ${MKL_VERSION}")
  message(STATUS "    include path: ${MKL_INCLUDE_DIR}")
endif()
if(Realsense_FOUND)
  message(STATUS "Realsense library status:")
  message(STATUS "    library: ${realsense2_LIBRARY}")
  message(STATUS "    version: ${realsense2_VERSION}")
  message(STATUS "    include path: ${realsense2_INCLUDE_DIR}")
endif()
if(V4L2_FOUND)
  message(STATUS "V4L2 library status:")
  message(STATUS "    library: ${V4L2_LIBRARIES}")
  message(STATUS "    include path: ${V4L2_INCLUDE_DIRS}")
endif()
if(libcamera_FOUND)
  message(STATUS "libcamera library status:")
  message(STATUS "    library: ${libcamera_LIBRARIES}")
  message(STATUS "    include path: ${libcamera_INCLUDE_DIRS}")
endif()
if(OpenVINOLIB_FOUND)
  message(STATUS "OpenVINO library status:")
  message(STATUS "    library: ${OpenVINOLIB}")
  message(STATUS "    cv only: ${OpenVINOLIB_CVONLY}")
endif()
