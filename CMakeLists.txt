cmake_minimum_required(VERSION 3.18)

project(hello-metal)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(dependencies/glfw)


add_executable(hello-metal 
    src/metal.mm
    src/hello-metal.cpp
)
target_link_libraries(hello-metal
    "-framework AppKit"
    "-framework QuartzCore"
    "-framework Metal"
)
target_include_directories(hello-metal PUBLIC
    "include"
	"dependencies/glfw/include"
	"dependencies/metal-cpp/include"
)
target_link_libraries(hello-metal glfw)
set_target_properties(hello-metal PROPERTIES
    LINK_FLAGS "-ObjC"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
