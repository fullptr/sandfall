#pragma once
#include <glm/glm.hpp>

namespace sand {

class level;
class chunk;
auto create_chunk_triangles(level& w, chunk& c, glm::ivec2 top_left) -> void;

}