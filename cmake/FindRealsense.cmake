set(Realsense_FOUND TRUE)

if(Realsense_ROOT)
	add_library(Realsense SHARED IMPORTED )
	set_target_properties(Realsense PROPERTIES
				 IMPORTED_LOCATION ${Realsense_ROOT}/bin/x64/realsense2.dll
				 IMPORTED_IMPLIB ${Realsense_ROOT}/lib/x64/realsense2.lib)
	target_include_directories(Realsense INTERFACE ${Realsense_ROOT}/include)
else()
	set(Realsense_FOUND FALSE)
endif()