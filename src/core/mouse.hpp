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
    glm::vec2      d_position_last_frame;
    glm::vec2      d_positiion_this_frame;

public:
    auto on_event(const event& e) -> void;
    auto on_new_frame() -> void;

    auto is_down(mouse_button button) const -> bool;
    auto is_down_this_frame(mouse_button button) const-> bool;
    auto offset() const -> glm::vec2;
    auto position() const -> glm::vec2;
};

enum keyboard_key // TODO: Make enum class
{
    A = 65,
    D = 68,
    E = 69,
    Q = 81,
    S = 83,
    W = 87,
};

class keyboard
{
    std::bitset<128> d_down;
    std::bitset<128> d_down_this_frame;

public:
    auto on_event(const event& e) -> void;
    auto on_new_frame() -> void;

    auto is_down(keyboard_key key) const -> bool;
    auto is_down_this_frame(keyboard_key key) const -> bool;
};
    
}