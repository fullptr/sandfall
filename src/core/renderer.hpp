#pragma once
#include "texture.hpp"
#include "shader.hpp"
#include "world.hpp"
#include "camera.hpp"

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
    renderer(std::size_t width, std::size_t height);
    ~renderer();

    auto bind() const -> void;

    auto update(const level& world, bool show_chunks, const camera& camera) -> void;

    auto draw() const -> void;

    auto resize(std::size_t width, std::size_t height) -> void;
};

}