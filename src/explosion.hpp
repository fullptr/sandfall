#pragma once
#include "world.hpp"

#include <glm/glm.hpp>

namespace sand {

struct explosion
{
    // Radii from the centre to try and destroy
    float min_radius;
    float max_radius;

    // Extra offset to apply at the end of destruction to try and darken the surroundings
    float scorch_radius;
};

auto apply_explosion(world& pixels, glm::ivec2 pos, const explosion& info) -> void;

}