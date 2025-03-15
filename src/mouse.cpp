#include "mouse.hpp"
#include "event.hpp"

#include <type_traits>
#include <utility>

namespace sand {

auto mouse::on_event(const event& event) -> void
{
    if (const auto e = event.get_if<sand::mouse_pressed_event>()) {
        d_down[e->button] = true;
        d_down_this_frame[e->button] = true;
    }
    else if (const auto e = event.get_if<sand::mouse_released_event>()) {
        d_down[e->button] = false;
    }
    else if (const auto e = event.get_if<sand::mouse_moved_event>()) {
        d_positiion_this_frame = e->pos;
    }
}

auto mouse::on_new_frame() -> void
{
    d_position_last_frame = d_positiion_this_frame;
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

auto mouse::offset() const -> glm::vec2
{
    return d_positiion_this_frame - d_position_last_frame;
}


auto keyboard::on_event(const event& event) -> void
{
    if (const auto e = event.get_if<sand::keyboard_pressed_event>()) {
        d_down[e->key] = true;
        d_down_this_frame[e->key] = true;
    }
    else if (const auto e = event.get_if<sand::keyboard_released_event>()) {
        d_down[e->key] = false;
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