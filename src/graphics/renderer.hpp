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
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    texture                d_texture;
    std::vector<glm::vec4> d_texture_data;

    shader d_shader;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

public:
    renderer(std::uint32_t screen_width, std::uint32_t screen_height);
    ~renderer();

    auto update(
        const tile& tile,
        bool show_chunks,
        glm::ivec2 top_left,
        std::uint32_t screen_width,
        std::uint32_t screen_height
    ) -> void;

    auto draw() const -> void;

    auto resize(std::uint32_t width, std::uint32_t height) -> void;
};

}