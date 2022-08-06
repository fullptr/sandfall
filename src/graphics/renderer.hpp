#pragma once
#include "graphics/texture.hpp"
#include "tile.h"

#include <glm/glm.hpp>

#include <memory>
#include <array>

namespace sand {

// Responsible for rendering the world to the screen.
class renderer
{
public:
    using texture_data = std::array<glm::vec4, sand::tile_size * sand::tile_size>;

private:
    texture                       d_texture;
    std::unique_ptr<texture_data> d_texture_data;

public:
    renderer();

    auto update(const tile& tile, bool show_chunks) -> void;
    auto draw() const -> void;
};

}