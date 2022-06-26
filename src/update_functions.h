#pragma once
#include "tile.h"
#include "world_settings.h"

#include <glm/glm.hpp>

namespace sand {

void update_sand(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_water(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_rock(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
    
}