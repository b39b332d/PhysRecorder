
add_library(phy_serial STATIC ppg.h ppg.cpp respi.cpp respi.h custom_serial.cpp custom_serial.cpp SerialReader.cpp)
target_link_libraries(phy_serial PUBLIC Qt6::Widgets serial )
target_include_directories(phy_serial PUBLIC ./ ${utils_dir})
set_target_properties(phy_serial PROPERTIES 
    AUTOMOC ON
)

if(MSVC)
	target_link_libraries(phy_serial PUBLIC "Setupapi.lib") # Serial
endif()