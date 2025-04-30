# new features
cmake_minimum_required(VERSION 3.21)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
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
  list(APPEND CMAKE_LIBRARY_PATH $ENV{CONDA_PREFIX}/lib)
  list(APPEND CMAKE_INCLUDE_PATH $ENV{CONDA_PREFIX}/include)
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
set(FETCHCONTENT_QUIET FALSE)

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
set(LIBYUV_PATH CACHE PATH "libyuv.dll path")

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
  message(FATAL_ERROR "OpenCV does not exist! CMake will exit.")
endif()

string(TOUPPER ${Porject_BUILD_TYPE} Porject_BUILD_TYPE_UPPER)
get_target_property(cv_core opencv_core IMPORTED_LOCATION_${Porject_BUILD_TYPE_UPPER})
get_filename_component(INSTALL_OPENCV_BIN "${cv_core}" DIRECTORY)
list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENCV_BIN})

####### FIND OPENVINO
find_package(OpenVINO QUIET)
if(NOT OpenVINO_FOUND AND DOWNLOAD_URL)
  FetchContent_Declare(
    OpenVINO_Build
    URL "${DOWNLOAD_URL}/openvino2450/${PHY_RUNTIME}/${PHY_ARCH}.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(OpenVINO_Build)
  find_package(OpenVINO REQUIRED NO_SYSTEM_ENVIRONMENT_PATH HINTS ${openvino_build_SOURCE_DIR}/runtime)
endif()
if(NOT OpenVINO_FOUND)
  message(FATAL_ERROR "OpenVINO does not exist! CMake will exit.")
endif()
get_target_property(ov_core openvino::runtime IMPORTED_LOCATION_${Porject_BUILD_TYPE_UPPER})
get_filename_component(INSTALL_OPENVINO_BIN "${ov_core}" DIRECTORY)
list(APPEND INSTALL_DEP_PATHS ${INSTALL_OPENVINO_BIN})

set(OPENVINO_PATH_EXT "openvino-${OpenVINO_VERSION}")
string(SUBSTRING ${OpenVINO_VERSION_MAJOR} 2 -1 OpenVINO_VERSION_COMPACT)
set(OpenVINO_VERSION_COMPACT ${OpenVINO_VERSION_COMPACT}${OpenVINO_VERSION_MINOR}${OpenVINO_VERSION_PATCH})
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
      -DCMAKE_INSTALL_PREFIX=${LINUXDEPLOYQT_PREFIX}/install # Install prefix for the external project
      -DCMAKE_BUILD_TYPE=Release # Pass the build type (Debug, Release, etc.)
      INSTALL_BYPRODUCTS
      ${LINUXDEPLOYQT_PREFIX}/install/bin/linuxdeployqt # Adjust path if needed
      LOG_CONFIGURE TRUE # Log the configure step output
      LOG_BUILD TRUE # Log the build step output
      LOG_INSTALL TRUE # Log the install step output
    )
    set(DEPLOYQT_EXECUTABLE "${LINUXDEPLOYQT_PREFIX}/install/bin/linuxdeployqt")
  endif()
endif()
####### FIND MKL
set(USE_STATIC_MKL FALSE)
set(MKL_THREADING SEQ)
find_package(MKL)
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
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL
)
if(MKL_FOUND)
  set(EIGEN_BUILD_LAPACK TRUE)
  set(EIGEN_BUILD_BLAS TRUE)
  set(EIGEN_USE_MKL_ALL TRUE)
endif()
if(NOT Eigen3_POPULATED)
  FetchContent_Populate(Eigen3)
  add_subdirectory(${eigen3_SOURCE_DIR} ${eigen3_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


######### FIND PACKEXE
if(Porject_BUILD_TYPE STREQUAL "release")
  if(WIN32)
    if(NOT ENIGMAVBCONXOLE_EXE OR NOT UPX_EXE)
      if(${DOWNLOAD_URL})
        FetchContent_Declare(
          BOX_TOOLS
          URL "${DOWNLOAD_URL}/box_tools.zip"
          DOWNLOAD_EXTRACT_TIMESTAMP TRUE
          OVERRIDE_FIND_PACKAGE
          EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(BOX_TOOLS)

        set(ENIGMAVBCONXOLE_EXE "${box_tools_SOURCE_DIR}/enigmavbconsole.exe")
        set(UPX_EXE "${box_tools_SOURCE_DIR}/upx.exe")
      endif()
    endif()
    if(ENIGMAVBCONXOLE_EXE AND UPX_EXE)
      add_custom_target(generate_boxed_exe
        COMMAND ${CMAKE_COMMAND} -DEXEC_NAME=PhysRecorder -DUPX_EXE=${UPX_EXE} -DEVB_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bin -P ${CMAKE_HOME_DIRECTORY}/cmake/configure_evb_file.cmake
        COMMAND ${ENIGMAVBCONXOLE_EXE} ${CMAKE_INSTALL_PREFIX}/bin/PhysRecorder.evb
        DEPENDS install
        USES_TERMINAL
      )
      set_target_properties(generate_boxed_exe PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
  elseif(UNIX)
    find_program(APPIMAGE_PACK_EXE appimagetool)
    if(NOT ${APPIMAGE_PACK_EXE})
      set(APPIMAGE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/appimage)
      ExternalProject_Add(
        appimage_external
        GIT_REPOSITORY https://gitcode.com/gh_mirrors/ap/appimagetool.git
        GIT_TAG origin/main
        PREFIX ${APPIMAGE_PREFIX}
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${APPIMAGE_PREFIX}/install # Install prefix for the external project
        -DCMAKE_BUILD_TYPE=Release # Pass the build type (Debug, Release, etc.)
        -DCMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}
        BUILD_BYPRODUCTS
        ${APPIMAGE_PREFIX}/bin/appimagetool # Adjust path if needed
        LOG_CONFIGURE TRUE # Log the configure step output
        LOG_BUILD TRUE # Log the build step output
        LOG_INSTALL TRUE # Log the install step output
        EXCLUDE_FROM_ALL TRUE
      )
      get_property(APPIMAGE_EXE_PATH TARGET appimage_external PROPERTY BINARY_DIR)
      set(APPIMAGE_EXECUTABLE "${APPIMAGE_EXE_PATH}/bin/appimagetool")
      set(ENV{PATH} "$ENV{PATH}:${APPIMAGE_EXE_PATH}/bin")
    endif()

    add_custom_target(generate_boxed_exe
      COMMAND ${CMAKE_COMMAND} -DEXEC_NAME=PhysRecorder -DUPX_EXE=${UPX_EXE} -DEVB_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/bin -P ${CMAKE_HOME_DIRECTORY}/cmake/configure_evb_file.cmake
      COMMAND ${ENIGMAVBCONXOLE_EXE} ${CMAKE_INSTALL_PREFIX}/bin/PhysRecorder.evb
      DEPENDS install
      USES_TERMINAL
    )
    set_target_properties(generate_boxed_exe PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_dependencies(generate_boxed_exe appimage_external)

  endif()
endif()
####### FIND MSMF
find_package(MSMF)
####### FIND MVis
find_package(MVis)
####### FIND Realsense
find_package(Realsense)

####### FIND libyuv
if(MSVC)
  find_library(libyuv_build_lib libyuv.dll HINT ${LIBYUV_PATH})
  if(libyuv_build_lib)
    set(libyuv_dll ${libyuv_build_lib})
    set(libyuv_include ${LIBYUV_PATH}/include)
    add_library(yuv SHARED IMPORTED)
    set_target_properties(yuv PROPERTIES
      IMPORTED_IMPLIB ${libyuv_build_lib}
      IMPORTED_LOCATION ${LIBYUV_PATH}/libyuv.dll
      INTERFACE_INCLUDE_DIRECTORIES ${LIBYUV_PATH}/include
    )
    list(APPEND INSTALL_DEP_PATHS "${LIBYUV_PATH}")
    set(libyuv_status "local")
  elseif(DOWNLOAD_URL)
    FetchContent_Declare(
      libyuv_build
      URL "${DOWNLOAD_URL}/libyuv.zip"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(libyuv_build)
    find_library(libyuv_build libyuv.dll HINT ${libyuv_build_SOURCE_DIR})
    add_library(yuv SHARED IMPORTED)
    set_target_properties(yuv PROPERTIES
      IMPORTED_IMPLIB ${libyuv_build}
      IMPORTED_LOCATION "${libyuv_build_SOURCE_DIR}/libyuv.dll"
      INTERFACE_INCLUDE_DIRECTORIES ${libyuv_build_SOURCE_DIR}/include
    )
    list(APPEND INSTALL_DEP_PATHS "${libyuv_build_SOURCE_DIR}")
    set(libyuv_status "download")
  endif()
endif()
if(NOT TARGET yuv)
  FetchContent_Declare(
    libyuv
    GIT_REPOSITORY https://gitcode.com/gh_mirrors/li/libyuv.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
  )
  if(NOT libyuv_POPULATED)
    FetchContent_Populate(libyuv)
    add_subdirectory(${libyuv_SOURCE_DIR} ${libyuv_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  target_include_directories(yuv INTERFACE "${libyuv_SOURCE_DIR}/include/")
  set(libyuv_status ${CMAKE_CXX_COMPILER_ID})
endif()


####### Find serial
FetchContent_Declare(
  libserial
  GIT_REPOSITORY https://gitcode.com/gh_mirrors/se/serial.git
  GIT_SHALLOW TRUE
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

set(DLIB_NO_GUI_SUPPORT TRUE)
set(DLIB_PNG_SUPPORT FALSE)
set(DLIB_GIF_SUPPORT FALSE)
set(DLIB_JPEG_SUPPORT FALSE)
set(DLIB_USE_MKL_FFT FALSE)
set(DLIB_USE_FFMPEG FALSE)
if(MKL_FOUND)
  set(DLIB_USE_LAPACK TRUE)
  set(DLIB_USE_BLAS TRUE)
  set(DLIB_USE_MKL_SEQUENTIAL TRUE)
else()
  set(DLIB_USE_LAPACK FALSE)
  set(DLIB_USE_BLAS FALSE)
  set(DLIB_USE_MKL_SEQUENTIAL)
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
