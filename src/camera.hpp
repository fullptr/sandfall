#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::vec2 top_left;
    float     screen_width;
    float     screen_height;
    int       zoom;
};

}