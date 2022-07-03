#pragma once
#include "event.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <cstdint>
#include <functional>

struct GLFWwindow;

namespace sand {

using callback_t = std::function<void(event&)>;

struct window_data
{
    std::string name;

    int width;
    int height;

    bool fullscreen;
    bool running;
    bool focused;

    GLFWwindow* native_window;

    callback_t callback = [](event&) {};
};

class window
{
    window_data d_data;

    window(const window&) = delete;
    window& operator=(const window&) = delete;

    window(window&&) = delete;
    window& operator=(window&&) = delete;

public:
    window(const std::string&, int width, int height);
    ~window();

    auto clear() const -> void;
    auto swap_buffers() -> void;
    auto poll_events() -> void;

    auto is_running() const -> bool;

    auto get_mouse_pos() const -> glm::vec2;

    auto set_name(const std::string& name) -> void;
    auto set_callback(const callback_t& callback) -> void;

    auto width() const -> float;
    auto height() const -> float;

    auto native_handle() -> GLFWwindow*;
};

}