cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS}")

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(sandfall
               sandfall.m.cpp
               tile.cpp
               pixel.cpp
               update_functions.cpp
               utility.cpp
           
               graphics/window.cpp
               graphics/shader.cpp
               graphics/texture.cpp
               graphics/ui.cpp
)

target_include_directories(sandfall PUBLIC .)

target_link_libraries(sandfall PUBLIC
                      vendor
                      glfw
                      glad::glad
                      glm
                      fmt::fmt
                      imgui::imgui
)