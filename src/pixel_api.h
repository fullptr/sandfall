#pragma once
#include "tile.h"
#include "world_settings.h"

#include <glm/glm.hpp>

namespace sand {

class pixel_api
// A light wrapper around pixel data to make the material update functions simpler.
{
    const world_settings& d_world_settings_ref;
    tile::pixels&         d_pixels_ref;
    glm::ivec2            d_pos;

    pixel_api(const pixel_api&) = delete;
    pixel_api& operator=(const pixel_api&) = delete;

public:
    pixel_api(const world_settings& settings, tile::pixels& pixels_ref, glm::ivec2 pos)
        : d_world_settings_ref(settings), d_pixels_ref(pixels_ref), d_pos(pos) {}

    glm::ivec2 move_to(glm::ivec2 offset);
    pixel& get(glm::ivec2 offset);
    bool valid(glm::ivec2 offset);

    const world_settings& world_settings() const { return d_world_settings_ref; }
};

}