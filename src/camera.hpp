#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::vec2 top_left;
    int screen_width;
    int screen_height;
    float world_to_screen; // the number of screen pixels per world pixel, multiplying by it takes you from world space to screen space
};

}