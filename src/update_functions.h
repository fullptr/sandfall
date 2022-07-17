#pragma once
#include "tile.h"

#include <glm/glm.hpp>

namespace sand {

auto update_pixel(tile& pixels, glm::ivec2 pos) -> void;
    
}