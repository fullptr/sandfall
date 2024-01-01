#pragma once
#include <glm/glm.hpp>

namespace sand {

struct player
{
    glm::vec2 position; // At the bottom in the middle
    float width;
    float height;

    // Returns a {x, y, width, height} rect to draw the player on the screen.
    inline auto get_rect() const -> glm::vec4
    {
        return glm::vec4{position.x - width / 2, position.y - height, width, height};
    }
};

}