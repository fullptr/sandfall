#pragma once
#include "event.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <cstdint>
#include <functional>

struct GLFWwindow;

namespace sand {

using window_callback = std::function<void(const event&)>;

struct window_data
{
    std::string name;

    int width;
    int height;

    bool fullscreen;
    bool running;
    bool focused;

    glm::vec2       mouse_pos     = {0.0, 0.0};
    GLFWwindow*     native_window = nullptr;
    window_callback callback      = {};
};

class window
{
    window_data d_data;

    window(const window&) = delete;
    window& operator=(const window&) = delete;

    window(window&&) = delete;
    window& operator=(window&&) = delete;

public:
    window(const std::string& name, int width, int height);
    ~window();

    auto clear() const -> void;
    auto swap_buffers() -> void;
    auto poll_events() -> void;

    auto is_running() const -> bool;

    auto get_mouse_pos() const -> glm::vec2;

    auto set_name(const std::string& name) -> void;
    auto set_callback(const window_callback& callback) -> void;

    auto width() const -> int;
    auto height() const -> int;

    auto native_handle() -> GLFWwindow*;
};

}