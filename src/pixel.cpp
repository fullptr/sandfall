#include "pixel.h"

#include <cstdlib>

namespace sand {

pixel pixel::air()
{
    return {
        .data = empty{},
        .type = pixel_type::air,
        .colour = {
            44.0f / 256.0f,
            58.0f / 256.0f,
            71.0f / 256.0f,
            1.0
        }
    };
}

pixel pixel::sand()
{
    return {
        .data = movable_solid{},
        .type = pixel_type::sand,
        .colour = {
            (248.0f + (rand() % 20) - 10) / 256.0f,
            (239.0f + (rand() % 20) - 10) / 256.0f,
            (186.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

pixel pixel::rock()
{
    return {
        .data = static_solid{},
        .type = pixel_type::rock,
        .colour = {
            (200.0f + (rand() % 20) - 10) / 256.0f,
            (200.0f + (rand() % 20) - 10) / 256.0f,
            (200.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

pixel pixel::water()
{
    return {
        .data = liquid{},
        .type = pixel_type::water,
        .colour = {
            (27.0f  + (rand() % 20) - 10) / 256.0f,
            (156.0f + (rand() % 20) - 10) / 256.0f,
            (252.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

pixel pixel::red_sand()
{
    return {
        .data = movable_solid{},
        .type = pixel_type::sand,
        .colour = {
            (254.0f + (rand() % 20) - 10) / 256.0f,
            (164.0f + (rand() % 20) - 10) / 256.0f,
            (127.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}


}