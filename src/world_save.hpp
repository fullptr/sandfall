#pragma once
#include "serialise.hpp"
#include "pixel.hpp"

#include <cstddef>

namespace sand {

struct world_save
{
    std::vector<pixel> pixels;
    std::size_t        width;
    std::size_t        height;
};

auto serialise(auto& archive, world_save& ws) -> void
{
    archive(ws.pixels, ws.width, ws.height);
}

}