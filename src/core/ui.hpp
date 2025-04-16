#pragma once
#include "buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "camera.hpp"

#include <glm/glm.hpp>

namespace sand {

// This is just a copy of quad_instance from the shape_renderer, should
// we combine these? I'm just making a copy now since I am assuming both will
// iterate in different directions and I don't necessarily want them tied
// together, but I still feel conflicted.
struct ui_quad
{
    glm::vec2 centre;
    float     width;
    float     height;
    float     angle;
    glm::vec4 colour;

    static void set_buffer_attributes(std::uint32_t vbo);
};

class ui
{
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    std::vector<ui_quad> d_quads;

    shader d_shader;

    vertex_buffer d_instances;

    ui(const ui&) = delete;
    ui& operator=(const ui&) = delete;

public:
    ui();
    ~ui();

    void start_frame(const camera& c);
    void end_frame();

    bool button(glm::vec2 pos, float width, float height);
};

}