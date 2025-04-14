cmake_minimum_required(VERSION 3.16)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)

add_library(core STATIC
    buffer.cpp
    renderer.cpp
    shape_renderer.cpp
    debug.cpp
    window.cpp
    entity.cpp
    shader.cpp
    texture.cpp
    utility.cpp
    mouse.cpp
    world.cpp
    pixel.cpp
    explosion.cpp
    update_rigid_bodies.cpp
    serialisation.cpp
)

target_include_directories(core PUBLIC .)

target_link_libraries(core PRIVATE
    glfw
    glad::glad
    glm::glm
    imgui::imgui
    cereal::cereal
    box2d::box2d
)