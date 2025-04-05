#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::vec2 top_left;    // world space (screen space would just be {0, 0})
    int screen_width;      // screen space
    int screen_height;     // screen space
    float world_to_screen; // the number of screen pixels per world pixel, multiplying by it takes you from world space to screen space
};

// Returns the width and height as a glm::vec2
inline auto dimensions(const camera& c) -> glm::vec2
{
    return {c.screen_width, c.screen_height};
}

}