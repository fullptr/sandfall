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

    texture_dyn d_texture;
    shader      d_shader;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;

public:
    renderer(i32 width, i32 height);
    ~renderer();

    auto update(const level& world) -> void;

    auto draw(const camera& camera) const -> void;

    auto resize(u32 width, u32 height) -> void;
};

}