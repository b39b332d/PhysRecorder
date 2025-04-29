if(Realsense_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverRS.cpp"
	)
	target_link_libraries(cam_driver PUBLIC realsense2::realsense2)
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_RS" PARENT_SCOPE) 

	message(STATUS "DRIVER: Realsense Enabled")
endif()