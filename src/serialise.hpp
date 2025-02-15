#pragma once
#include <glm/glm.hpp>

#define CEREAL_SERIALIZE_FUNCTION_NAME serialise
#include <cereal/types/array.hpp>
#include <cereal/types/bitset.hpp>
#include <cereal/types/vector.hpp>

namespace cereal {

auto serialise(auto& archive, glm::vec2& vec) -> void
{
    archive(vec.x, vec.y);
}

auto serialise(auto& archive, glm::ivec2& vec) -> void
{
    archive(vec.x, vec.y);
}

auto serialise(auto& archive, glm::vec4& vec) -> void
{
    archive(vec.x, vec.y, vec.z, vec.w);
}

}