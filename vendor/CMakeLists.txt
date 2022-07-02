cmake_minimum_required(VERSION 3.16)

project(vendor)

find_package(glfw3 CONFIG REQUIRED)

add_library(vendor STATIC
            imgui/imgui.cpp
            imgui/imgui_demo.cpp
            imgui/imgui_draw.cpp
            imgui/imgui_tables.cpp
            imgui/imgui_widgets.cpp

            imgui/backends/imgui_impl_glfw.cpp
            imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(vendor PUBLIC . ./imgui)
target_link_libraries(vendor PUBLIC glfw)