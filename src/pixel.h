#pragma once
#include <glm/glm.hpp>

#include <variant>

namespace sand {

struct movable_solid
{
    glm::vec2 velocity = {0.0, 0.0};
};

struct static_solid
{
};

struct liquid
{
    glm::vec2 velocity = {0.0, 0.0};
};

struct gas
{
};

using pixel_data = std::variant<
    movable_solid,
    static_solid,
    liquid,
    gas,
    std::monostate
>;

struct pixel
{
    pixel_data data;
    glm::vec4  colour;
    bool       updated_this_frame = false;

    // TODO: Move out of struct
    static pixel air();
    static pixel sand();
    static pixel rock();
    static pixel water();
    static pixel red_sand();
};

}