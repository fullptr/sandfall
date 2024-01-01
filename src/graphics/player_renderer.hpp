#pragma once
#include "graphics/texture.hpp"
#include "graphics/shader.hpp"
#include "world.hpp"
#include "camera.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <array>

namespace sand {

// Responsible for rendering the world to the screen.
class player_renderer
{
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    shader d_shader;

    player_renderer(const player_renderer&) = delete;
    player_renderer& operator=(const player_renderer&) = delete;

public:
    player_renderer();
    ~player_renderer();

    auto bind() const -> void;

    auto update(const world& world, glm::vec2 pos, const camera& camera) -> void;

    auto draw() const -> void;
};

}