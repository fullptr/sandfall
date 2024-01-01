#pragma once
#include "event.hpp"

#include <bitset>

namespace sand {

enum class mouse_button
{
    left,
    right,
};

class mouse
{
    std::bitset<8> d_down;
    std::bitset<8> d_down_this_frame;

public:
    auto on_event(const event& e) -> void;
    auto on_new_frame() -> void;

    auto is_down(mouse_button button) -> bool;
    auto is_down_this_frame(mouse_button button) -> bool;
};

enum class keyboard_key
{
    W = 87,
    A = 65,
    S = 83,
    D = 68,
};

class keyboard
{
    std::bitset<128> d_down;
    std::bitset<128> d_down_this_frame;

public:
    auto on_event(const event& e) -> void;
    auto on_new_frame() -> void;

    auto is_down(keyboard_key key) -> bool;
    auto is_down_this_frame(keyboard_key key) -> bool;
};
    
}