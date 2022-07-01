#include "pixel.h"

#include <cstdlib>

namespace sand {

pixel pixel::air()
{
    return {
        .data = std::monostate{},
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
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .intertial_resistance = 0.1f
        },
        .colour = {
            (248.0f + (rand() % 20) - 10) / 256.0f,
            (239.0f + (rand() % 20) - 10) / 256.0f,
            (186.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

pixel pixel::coal()
{
    return {
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .intertial_resistance = 0.95f
        },
        .colour = {
            (30.0f + (rand() % 20) - 10) / 256.0f,
            (39.0f + (rand() % 20) - 10) / 256.0f,
            (46.0f + (rand() % 20) - 10) / 256.0f,
            1.0
        }
    };
}

pixel pixel::rock()
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

pixel pixel::water()
{
    return {
        .data = liquid{},
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
        .data = movable_solid{
            .velocity = {0.0, 0.0},
            .is_falling = true,
            .intertial_resistance = 0.1f
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