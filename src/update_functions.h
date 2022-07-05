#pragma once
#include "tile.h"
#include "world_settings.h"

#include <glm/glm.hpp>

namespace sand {

auto update_pixel(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt) -> void;
    
}