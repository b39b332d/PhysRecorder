if(MVis_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverMVis.cpp"
	)
	target_link_libraries(cam_driver PUBLIC MVis)
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_MVis" PARENT_SCOPE) 
	message(STATUS "DRIVER: MVis Enabled")
endif()