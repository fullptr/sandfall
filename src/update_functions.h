#pragma once
#include "pixel_api.h"

namespace sand {

void update_sand(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_water(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
void update_rock(pixel_api&& api, tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt);
    
}