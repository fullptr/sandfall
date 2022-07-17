#pragma once
#include <glm/glm.hpp>

namespace cereal {

template<class Archive>
void serialise(Archive& archive, glm::vec2& vec)
{
    archive(vec.x, vec.y);
}

template<class Archive>
void serialise(Archive& archive, glm::vec4& vec)
{
    archive(vec.x, vec.y, vec.z, vec.w);
}

#define CEREAL_SERIALIZE_FUNCTION_NAME serialise

}