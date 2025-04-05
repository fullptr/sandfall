#pragma once
#include <glm/glm.hpp>

#include "common.hpp"

namespace sand {

class world;
class chunk;
auto create_chunk_triangles(world& w, chunk& c, pixel_pos top_left) -> void;

}