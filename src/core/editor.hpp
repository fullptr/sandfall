#pragma once
#include "pixel.hpp"
#include "world.hpp"
#include "utility.hpp"
#include "entity.hpp"
#include "window.hpp"

#include <box2d/box2d.h>

#include <cstdint>
#include <vector>
#include <utility>
#include <string>

namespace sand {

struct editor
{
    std::size_t current = 0;
    std::vector<std::pair<std::string, sand::pixel(*)()>> pixel_makers = {
        { "air",         sand::pixel::air         },
        { "sand",        sand::pixel::sand        },
        { "coal",        sand::pixel::coal        },
        { "dirt",        sand::pixel::dirt        },
        { "water",       sand::pixel::water       },
        { "lava",        sand::pixel::lava        },
        { "acid",        sand::pixel::acid        },
        { "rock",        sand::pixel::rock        },
        { "titanium",    sand::pixel::titanium    },
        { "steam",       sand::pixel::steam       },
        { "fuse",        sand::pixel::fuse        },
        { "ember",       sand::pixel::ember       },
        { "oil",         sand::pixel::oil         },
        { "gunpowder",   sand::pixel::gunpowder   },
        { "methane",     sand::pixel::methane     },
        { "battery",     sand::pixel::battery     },
        { "solder",      sand::pixel::solder      },
        { "diode_in",    sand::pixel::diode_in    },
        { "diode_out",   sand::pixel::diode_out   },
        { "spark",       sand::pixel::spark       },
        { "c4",          sand::pixel::c4          },
        { "relay",       sand::pixel::relay       }
    };

    float brush_size = 5.0f;

    std::size_t brush_type = 1;
        // 0 == circular spray
        // 1 == square
        // 2 == explosion
        
    bool show_chunks  = false;
    bool show_demo    = true;
    bool show_physics = false;
    bool show_spawn   = false;

    int  zoom         = 256;

    int new_world_chunks_width  = 4;
    int new_world_chunks_height = 4;

    auto get_pixel() -> sand::pixel
    {
        return pixel_makers[current].second();
    }
};

}