set(MVis_FOUND TRUE)

if (NOT WIN32)
	set(MVis_FOUND FALSE)
	return()
endif()

if(MVis_ROOT)

	add_library(MVis SHARED IMPORTED )

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set_target_properties(MVis PROPERTIES
					 IMPORTED_LOCATION ${MVis_ROOT}/SDK/X64/MVCAMSDK_X64.dll
					 IMPORTED_IMPLIB ${MVis_ROOT}/SDK/X64/MVCAMSDK_X64.lib)
	else()
		set_target_properties(MVis PROPERTIES
					 IMPORTED_LOCATION ${MVis_ROOT}/SDK/MVCAMSDK.dll
					 IMPORTED_IMPLIB ${MVis_ROOT}/SDK/MVCAMSDK.lib)
	endif()
		target_include_directories(MVis INTERFACE ${MVis_ROOT}/Demo/VC++/Include)
else()
	set(MVis_FOUND FALSE)
endif()
