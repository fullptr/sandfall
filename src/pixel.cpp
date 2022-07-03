#include "pixel.h"

#include <cstdlib>

namespace sand {

auto pixel::air() -> pixel
{
    return {
        .data = empty{},
        .colour = {
            44.0f / 256.0f,
            58.0f / 256.0f,
            71.0f / 256.0f,
            1.0
        }
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
        .colour = {
            (248.0f + (rand() % 20) - 10) / 256.0f,
            (239.0f + (rand() % 20) - 10) / 256.0f,
            (186.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
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
        .colour = {
            (30.0f + (rand() % 20) - 10) / 256.0f,
            (39.0f + (rand() % 20) - 10) / 256.0f,
            (46.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

auto pixel::rock() -> pixel
{
    return {
        .data = static_solid{},
        .colour = {
            (200.0f + (rand() % 20) - 10) / 256.0f,
            (200.0f + (rand() % 20) - 10) / 256.0f,
            (200.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

auto pixel::water() -> pixel
{
    return {
        .data = liquid{
            .velocity = {0.0, 0.0},
            .dispersion_rate = 3
        },
        .colour = {
            (27.0f  + (rand() % 20) - 10) / 256.0f,
            (156.0f + (rand() % 20) - 10) / 256.0f,
            (252.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

auto pixel::red_sand() -> pixel
{
    return {
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .inertial_resistance = 0.1f,
            .horizontal_transfer = 0.3f
        },
        .colour = {
            (254.0f + (rand() % 20) - 10) / 256.0f,
            (164.0f + (rand() % 20) - 10) / 256.0f,
            (127.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}


}