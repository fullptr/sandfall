#pragma once
#include "window.h"

namespace sand {

class ui
{
public:
    ui(sand::window& window);
    ~ui();

    void begin_frame();
    void end_frame();
};

}