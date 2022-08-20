#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::ivec2 top_left;
    int        screen_width;
    int        screen_height;
    int        zoom;
};

}