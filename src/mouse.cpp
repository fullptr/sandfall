#include "mouse.hpp"
#include "event.hpp"

#include <type_traits>
#include <utility>

namespace sand {

auto mouse::on_event(const event& e) -> void
{
    if (e.is<sand::mouse_pressed_event>()) {
        d_down[e.as<sand::mouse_pressed_event>().button] = true;
        d_down_this_frame[e.as<sand::mouse_pressed_event>().button] = true;
    }
    else if (e.is<sand::mouse_released_event>()) {
        d_down[e.as<sand::mouse_released_event>().button] = false;
    }
}

auto mouse::on_new_frame() -> void
{
    d_down_this_frame.reset();
}

auto mouse::is_down(mouse_button button) const -> bool
{
    return d_down.test(std::to_underlying(button));
}

auto mouse::is_down_this_frame(mouse_button button) const -> bool
{
    return d_down_this_frame.test(std::to_underlying(button));
}


auto keyboard::on_event(const event& e) -> void
{
    if (e.is<sand::keyboard_pressed_event>()) {
        d_down[e.as<sand::keyboard_pressed_event>().key] = true;
        d_down_this_frame[e.as<sand::keyboard_pressed_event>().key] = true;
    }
    else if (e.is<sand::keyboard_released_event>()) {
        d_down[e.as<sand::keyboard_released_event>().key] = false;
    }
}

auto keyboard::on_new_frame() -> void
{
    d_down_this_frame.reset();
}

auto keyboard::is_down(keyboard_key key) const -> bool
{
    return d_down.test(std::to_underlying(key));
}

auto keyboard::is_down_this_frame(keyboard_key key) const -> bool
{
    return d_down_this_frame.test(std::to_underlying(key));
}

}