#pragma once
#include <glm/glm.hpp>

#include <variant>

namespace sand {

enum class pixel_type
{
    air,
    sand,
    rock,
    water,
    red_sand
};

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

struct empty
{
};

using pixel_data = std::variant<
    movable_solid,
    static_solid,
    liquid,
    gas,
    empty
>;

struct pixel
{
    pixel_data data;
    pixel_type type;
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