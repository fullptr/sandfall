#include "pixel.h"
#include "utility.hpp"

#include <cstdlib>

namespace sand {
namespace {

auto light_noise() -> glm::vec4
{
    return {
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        random_from_range(-0.04f, 0.04f),
        1.0f
    };
}

}

auto pixel::air() -> pixel
{
    return {
        .data = empty{},
        .colour = from_hex(0x2C3A47)
    };
}

auto pixel::sand() -> pixel
{
    return {
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .inertial_resistance = 0.1f,
            .horizontal_transfer = 0.3f
        },
        .colour = from_hex(0xF8EFBA) + light_noise()
    };
}

auto pixel::coal() -> pixel
{
    return {
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .inertial_resistance = 0.95f,
            .horizontal_transfer = 0.1f
        },
        .colour = from_hex(0x1E272E) + light_noise()
    };
}

auto pixel::dirt() -> pixel
{
    return {
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .inertial_resistance = 0.4f,
            .horizontal_transfer = 0.2f
        },
        .colour = from_hex(0x5C1D06) + light_noise()
    };
}

auto pixel::rock() -> pixel
{
    return {
        .data = static_solid{},
        .colour = from_hex(0xC8C8C8) + light_noise()
    };
}

auto pixel::water() -> pixel
{
    return {
        .data = liquid{
            .velocity = {0.0, 0.0},
            .dispersion_rate = 5
        },
        .colour = from_hex(0x1B9CFC) + light_noise()
    };
}

auto pixel::lava() -> pixel
{
    return {
        .data = liquid{
            .velocity = {0.0, 0.0},
            .dispersion_rate = 1
        },
        .colour = from_hex(0xF97F51) + light_noise()
    };
}

}