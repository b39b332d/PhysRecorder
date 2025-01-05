if(MSMF_FOUND)
	target_sources(cam_driver PRIVATE
	"./CameraDriverMSMF.cpp"
	"./CameraReaderMSMF.cpp"
	)
	target_link_libraries(cam_driver PUBLIC ${MSMF_LIBS})
	set( CAM_DRIVER_FUNC ${CAM_DRIVER_FUNC} "EnumerateCamera_MSMF" PARENT_SCOPE) 
	
	message(STATUS "DRIVER: MSMF Enabled")
endif()