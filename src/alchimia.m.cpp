#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

int main()
{
    // Set up window
    alc::window window("alchimia", 1280, 720);

    // Set callbacks here

    // Main game loop
    while (true) {
        window.on_update(0.0);

        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }
}