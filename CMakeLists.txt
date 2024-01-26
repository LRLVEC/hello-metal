cmake_minimum_required(VERSION 3.18)

project(hello-metal)

set(CMAKE_CXX_STANDARD 20)
include_directories(include/cpp)
add_subdirectory(dependencies/glfw)
include_directories(dependencies/glfw/include)

add_executable(hello-metal hello-metal.m)
target_link_libraries(hello-metal
    "-framework AppKit"
    "-framework QuartzCore"
    "-framework Metal"
)
target_link_libraries(hello-metal glfw)
set_target_properties(hello-metal PROPERTIES
    LINK_FLAGS "-ObjC"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")