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
    glm::ivec2         spawn_point;

    auto serialise(auto& archive) -> void
    {
        archive(pixels, width, height, spawn_point);
    }
};

}