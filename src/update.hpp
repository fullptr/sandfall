#pragma once
#include "world.hpp"

#include <glm/glm.hpp>

namespace sand {

auto update_pixel(world& pixels, glm::ivec2 pos) -> void;

auto apply_explosion(world& pixels, glm::ivec2 pos, int radius, float strenth) -> void;
    
}