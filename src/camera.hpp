#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::ivec2 top_left;
    int        width;
    int        height;
};

}