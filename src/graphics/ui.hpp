#pragma once
#include "window.hpp"

namespace sand {

class ui
{
public:
    ui(sand::window& window);
    ~ui();

    auto begin_frame() -> void;
    auto end_frame() -> void;
};

}