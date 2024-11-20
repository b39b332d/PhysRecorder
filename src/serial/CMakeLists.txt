add_library(serial OBJECT ppg.h ppg.cpp serialport.cpp serialport.h respi.cpp respi.h custom_serial.cpp custom_serial.cpp)
target_link_libraries(serial PUBLIC Qt6::Core)
target_include_directories(serial PUBLIC ./)
if(MSVC)
	target_link_libraries(serial PUBLIC "Setupapi.lib") # Serial
endif()