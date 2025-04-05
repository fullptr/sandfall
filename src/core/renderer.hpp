#pragma once
#include "common.hpp"
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
    u32 d_vao;
    u32 d_vbo;
    u32 d_ebo;

    texture                d_texture;
    std::vector<glm::vec4> d_texture_data;

    shader d_shader;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

public:
    renderer(u32 width, u32 height);
    ~renderer();

    auto bind() const -> void;

    auto update(const level& world, const camera& camera) -> void;

    auto draw() const -> void;

    auto resize(u32 width, u32 height) -> void;
};

}