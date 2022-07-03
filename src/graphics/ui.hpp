#pragma once
#include "window.h"

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