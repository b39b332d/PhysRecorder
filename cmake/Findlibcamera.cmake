# rules for finding the V4L2 library

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_libcamera libcamera)

find_path(libcamera_INCLUDE_DIR libcamera/libcamera.h PATH_SUFFIXES libcamera HINTS ${PC_libcamera_INCLUDEDIR} ${PC_libcamera_INCLUDE_DIRS})
find_library(libcamera_LIBRARY NAMES camera libcamera.so REQUIRED HINTS ${PC_libcamera_LIBDIR} ${PC_libcamera_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libcamera DEFAULT_MSG libcamera_LIBRARY libcamera_INCLUDE_DIR)

mark_as_advanced(libcamera_INCLUDE_DIR libcamera_LIBRARY)

set(libcamera_INCLUDE_DIRS ${libcamera_INCLUDE_DIR})
set(libcamera_LIBRARIES ${libcamera_LIBRARY})