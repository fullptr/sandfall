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

struct pixel2
{
    pixel_data data;
    glm::vec4  colour;
    bool       updated_this_frame = false;
};

auto make_sand() -> pixel2;
auto make_water() -> pixel2;
auto make_stone() -> pixel2;

}