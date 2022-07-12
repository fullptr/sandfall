#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace sand {

enum class pixel_movement : std::uint8_t
{
    none,
    immovable_solid,
    movable_solid,
    liquid,
    gas,
};

enum class pixel_type : std::uint8_t
{
    none  = 0,
    sand  = 1,
    dirt  = 2,
    coal  = 3,
    water = 4,
    lava  = 5,
    rock  = 6,
};

struct pixel_properties
{
    pixel_movement movement            = pixel_movement::none;
    float          inertial_resistance = 0.0f;
    float          horizontal_transfer = 0.0f;
    int            dispersion_rate     = 0;
};

pixel_properties get_pixel_properties(pixel_type type);

struct pixel
{
    pixel_type type;

    glm::vec4 colour;
    glm::vec2 velocity           = {0.0, 0.0};
    bool      is_falling         = false;
    bool      updated_this_frame = false;

    static auto air() -> pixel;
    static auto sand() -> pixel;
    static auto coal() -> pixel;
    static auto dirt() -> pixel;
    static auto rock() -> pixel;
    static auto water() -> pixel;
    static auto lava() -> pixel;
};

}