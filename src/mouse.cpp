#include "mouse.hpp"
#include "event.hpp"

#include <type_traits>
#include <utility>

namespace sand {

auto mouse::on_event(const event& e) -> void
{
    if (e.is<sand::mouse_pressed_event>()) {
        d_down[e.as<sand::mouse_pressed_event>().button] = true;
        d_clicked[e.as<sand::mouse_pressed_event>().button] = true;
    }
    else if (e.is<sand::mouse_released_event>()) {
        d_down[e.as<sand::mouse_released_event>().button] = false;
    }
}

auto mouse::on_new_frame() -> void
{
    d_clicked.reset();
}

auto mouse::is_button_down(mouse_button button) -> bool
{
    return d_down.test(std::to_underlying(button));
}

auto mouse::is_button_clicked(mouse_button button) -> bool
{
    return d_clicked.test(std::to_underlying(button));
}

}