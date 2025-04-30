if(V4L2_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverV4L2.cpp"
	)
	target_link_libraries(cam_driver PUBLIC ${V4L2_LIBRARIES})
	target_include_directories(cam_driver PUBLIC ${V4L2_INCLUDE_DIRS})
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_V4L2" PARENT_SCOPE) 

	message(STATUS "DRIVER: V4L2 Enabled")
endif()