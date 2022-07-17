#pragma once
#include <glm/glm.hpp>

namespace cereal {

auto serialise(auto& archive, glm::vec2& vec) -> void
{
    archive(vec.x, vec.y);
}

auto serialise(auto& archive, glm::vec4& vec) -> void
{
    archive(vec.x, vec.y, vec.z, vec.w);
}

#define CEREAL_SERIALIZE_FUNCTION_NAME serialise

}