#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::vec2 top_left;
    float screen_width;
    float screen_height;
    float world_to_screen; // the number of screen pixels per world pixel, multiplying by it takes you from world space to screen space
};

}