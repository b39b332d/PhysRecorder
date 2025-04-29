set(Realsense_FOUND TRUE)

if(Realsense_ROOT AND WIN32)
	add_library(realsense2::realsense2 SHARED IMPORTED )
	set_target_properties(realsense2::realsense2 PROPERTIES
				 IMPORTED_LOCATION ${Realsense_ROOT}/bin/x64/realsense2.dll
				 IMPORTED_IMPLIB ${Realsense_ROOT}/lib/x64/realsense2.lib)
	target_include_directories(realsense2::realsense2 INTERFACE ${Realsense_ROOT}/include)
	list(APPEND INSTALL_DEP_PATHS "${Realsense_ROOT}/bin/x64/")
else()
	find_package(realsense2 REQUIRED)
	if(TARGET realsense2::realsense2)
		set(Realsense_FOUND TRUE)
	else()
		set(Realsense_FOUND FALSE)
	endif()
endif()

