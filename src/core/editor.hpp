#pragma once
#include "pixel.hpp"
#include "world.hpp"
#include "utility.hpp"
#include "entity.hpp"
#include "window.hpp"

#include <box2d/box2d.h>

#include <cstdint>
#include <vector>
#include <utility>
#include <string>

namespace sand {

struct editor
{
    sand::pixel_type current = sand::pixel_type::none;

    float brush_size = 5.0f;

    std::size_t brush_type = 1;
        // 0 == circular spray
        // 1 == square
        // 2 == explosion
        
    bool show_chunks  = false;
    bool show_demo    = true;
    bool show_physics = false;
    bool show_spawn   = false;

    int  zoom         = 256;

    int new_world_chunks_width  = 4;
    int new_world_chunks_height = 4;

    auto get_pixel() -> sand::pixel
    {
        return sand::make_pixel(current);
    }
};

}