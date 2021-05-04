#include "window.h"
#include "log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

int main()
{
    alc::window window("alchimia", 1280, 720);

    while (window.is_running()) {
        window.on_update(0.0);
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }
}