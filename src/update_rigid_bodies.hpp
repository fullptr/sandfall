#pragma once
#include <glm/glm.hpp>

namespace sand {

class world;
auto create_chunk_triangles(world& w, glm::ivec2 chunk_pos) -> void;

}