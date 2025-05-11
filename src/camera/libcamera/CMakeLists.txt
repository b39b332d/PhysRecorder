if(libcamera_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverLC.cpp"
	)
	target_link_libraries(cam_driver PUBLIC ${libcamera_LIBRARIES})
	target_include_directories(cam_driver PUBLIC ${libcamera_INCLUDE_DIRS})
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_LC" PARENT_SCOPE) 

	message(STATUS "DRIVER: libcamera Enabled")
endif()