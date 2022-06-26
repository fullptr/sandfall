#pragma once
#include "pixel_api.h"

namespace sand {

void update_sand(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_water(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_rock(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
    
}