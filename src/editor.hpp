#pragma once
#include "pixel.hpp"
#include "world.hpp"
#include "utility.hpp"
#include "player.hpp"
#include "graphics/window.hpp"

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
        
    bool show_chunks = false;
    bool show_demo = true;
    int zoom = 256;
    
    auto get_pixel() -> sand::pixel
    {
        return pixel_makers[current].second();
    }
};

// Returns true if the world has changed and should be rerendered
auto display_ui(
    editor& editor,
    world& world,
    const timer& timer,
    const window& window,
    const camera& camera,
    const player& player
) -> bool;

}