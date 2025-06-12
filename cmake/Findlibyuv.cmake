set(libyuv_FOUND TRUE)


find_library(libyuv_build_lib yuv libyuv.dll libyuv.so libyuv.so.0 HINTS ${LIBYUV_PATH})
find_path(libyuv_include libyuv.h PATH_SUFFIXES include HINTS ${LIBYUV_PATH})
if(libyuv_build_lib AND libyuv_include)
    add_library(yuv SHARED IMPORTED)
    set_target_properties(yuv PROPERTIES
        IMPORTED_IMPLIB ${libyuv_build_lib}
        IMPORTED_LOCATION ${libyuv_build_lib}
        INTERFACE_INCLUDE_DIRECTORIES ${libyuv_include}
    )
    list(APPEND INSTALL_DEP_PATHS "${LIBYUV_PATH}")
    set(libyuv_status "local")
elseif(MSVC AND DOWNLOAD_URL)
    FetchContent_Declare(
        libyuv_build
        URL "${DOWNLOAD_URL}/libyuv.zip"
    )
    FetchContent_MakeAvailable(libyuv_build)
    find_library(libyuv_build libyuv.dll HINTS ${libyuv_build_SOURCE_DIR} REQUIRED)
    add_library(yuv SHARED IMPORTED)
    set_target_properties(yuv PROPERTIES
        IMPORTED_IMPLIB ${libyuv_build}
        IMPORTED_LOCATION ${libyuv_build}
        INTERFACE_INCLUDE_DIRECTORIES ${libyuv_build_SOURCE_DIR}/include
    )
    list(APPEND INSTALL_DEP_PATHS "${libyuv_build_SOURCE_DIR}")
    set(libyuv_status "download")
endif()
if(NOT TARGET yuv)
  FetchContent_Declare(
    libyuv
    GIT_REPOSITORY https://gitcode.com/gh_mirrors/li/libyuv.git
    GIT_TAG main
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
  )
  if(NOT libyuv_POPULATED)
    FetchContent_Populate(libyuv)
    add_subdirectory(${libyuv_SOURCE_DIR} ${libyuv_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  target_include_directories(yuv INTERFACE "${libyuv_SOURCE_DIR}/include/")
  set(libyuv_status ${CMAKE_CXX_COMPILER_ID})
endif()
