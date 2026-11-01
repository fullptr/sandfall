#pragma once
#include <glm/glm.hpp>

#include "common.hpp"
#include "world.hpp"

namespace sand {

// The collision shape of a chunk depends only on its static pixels, so
// comparing these tells you whether the chunk's body needs rebuilding
auto get_chunk_static_pixels(const pixel_world& w, pixel_pos top_left) -> chunk_static_pixels;

auto create_chunk_rigid_bodies(level& l, pixel_pos top_left, chunk_static_pixels chunk_pixels) -> b2BodyId;

}
