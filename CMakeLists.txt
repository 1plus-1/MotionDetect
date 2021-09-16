cmake_minimum_required(VERSION 2.8)

project( motion_detect )

find_package( OpenCV REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB SRC
	MotionDetect.cpp
)

add_executable( motion_detect ${SRC} )

target_link_libraries( motion_detect ${OpenCV_LIBS})
