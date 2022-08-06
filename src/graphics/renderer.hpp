#pragma once
#include "graphics/texture.hpp"
#include "graphics/shader.h"
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
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    texture                       d_texture;
    std::unique_ptr<texture_data> d_texture_data;

    shader d_shader;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

public:
    renderer(float screen_width, float screen_height);
    ~renderer();

    auto update(const tile& tile, bool show_chunks) -> void;
    auto draw() const -> void;
};

}