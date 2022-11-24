#pragma once
#include "event.hpp"

#include <bitset>

namespace sand {

enum class mouse_button
{
    left,
    right
};

class mouse
{
    std::bitset<8> d_down;
    std::bitset<8> d_clicked;

public:
    auto on_event(const event& e) -> void;
    auto on_new_frame() -> void;

    auto is_button_down(mouse_button button) -> bool;
    auto is_button_clicked(mouse_button button) -> bool;
};
    
}