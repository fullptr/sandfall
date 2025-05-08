#pragma once
#include "world.hpp"
#include "common.hpp"

#include <glm/glm.hpp>

namespace sand {

struct explosion
{
    // Radii from the centre to try and destroy
    float min_radius;
    float max_radius;

    // Explosions scorch the area around the destroyed pixels. This is the standard deviation in
    // pixels used for the normal distribution to select the scorch radius
    float scorch;
};

auto apply_explosion(pixel_world& w, pixel_pos pos, const explosion& info) -> void;

}