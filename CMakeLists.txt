cmake_minimum_required(VERSION 3.18)

project(hello-metal)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(dependencies/glfw)

# glcume

add_executable(hello-metal 
    "src/metal.mm"
    "src/hello-metal.cpp"
    # imgui
    "dependencies/imgui/imgui.cpp"
    "dependencies/imgui/backends/imgui_impl_glfw.cpp"
    "dependencies/imgui/backends/imgui_impl_metal.mm"
    "dependencies/imgui/imgui_draw.cpp"
    "dependencies/imgui/imgui_tables.cpp"
    "dependencies/imgui/imgui_widgets.cpp"
)
target_link_libraries(hello-metal
    "-framework AppKit"
    "-framework QuartzCore"
    "-framework Metal"
    "-framework GameController"
)
target_include_directories(hello-metal PUBLIC
    "include"
    "dependencies"
    "dependencies/imgui"
	"dependencies/glm"
	"dependencies/glfw/include"
	"dependencies/metal-cpp/include"
)
target_compile_definitions(hello-metal PUBLIC IMGUI_IMPL_METAL_CPP)
target_link_libraries(hello-metal glfw)
set_target_properties(hello-metal PROPERTIES
    LINK_FLAGS "-ObjC"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
