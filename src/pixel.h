#pragma once
#include <glm/glm.hpp>

namespace sand {

enum class pixel_type
{
    air,
    sand,
    rock,
    water,
    red_sand
};

struct pixel
{
    // Static Data
    pixel_type type;
    glm::vec4  colour;

    // Dynamic Data
    glm::vec2 velocity = {0.0, 0.0};
    bool updated_this_frame = false;

    static pixel air();
    static pixel sand();
    static pixel rock();
    static pixel water();
    static pixel red_sand();
};

}