#pragma once
#include <glm/glm.hpp>

#include "common.hpp"

class b2Body;

namespace sand {

class level;
auto create_chunk_triangles(level& l, pixel_pos top_left) -> b2Body*;

}