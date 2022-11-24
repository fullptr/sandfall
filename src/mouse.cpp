#include "mouse.hpp"
#include "event.hpp"

#include <type_traits>

namespace sand {
namespace {

auto to_underlying(mouse_button button)
{
    return static_cast<std::underlying_type_t<mouse_button>>(button);
}

}

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
    return d_down.test(to_underlying(button));
}

auto mouse::is_button_clicked(mouse_button button) -> bool
{
    return d_clicked.test(to_underlying(button));
}

}