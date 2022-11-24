#pragma once
#include <glm/glm.hpp>

namespace sand {

struct camera
{
    glm::vec2 top_left;
    std::uint32_t       screen_width;
    std::uint32_t       screen_height;
    int       zoom;
};

}