if(Realsense_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverRS.cpp"
	)
	target_link_libraries(cam_driver PUBLIC Realsense)

	list(APPEND INSTALL_DEP_PATHS "${REALSENSE_DIR}/bin/x64/")
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_RS" PARENT_SCOPE) 

	message(STATUS "DRIVER: Realsense Enabled")
endif()