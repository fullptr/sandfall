#pragma once
#include <glm/glm.hpp>

namespace sand {

class world;
class chunk;
auto create_chunk_triangles(world& w, chunk& c, glm::ivec2 top_left) -> void;

}