#pragma once
#include "buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "camera.hpp"
#include "event.hpp"
#include "common.hpp"

#include <unordered_map>

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

    auto hash() const -> u64 {
        static constexpr auto h = std::hash<f32>{};
        return h(centre.x) ^ h(centre.y) ^ h(width) ^ h(height);
    }
};

struct ui_quad_times
{
    f64 hovered_time   = 0.0;
    f64 clicked_time   = 0.0;
    
    f64 unhovered_time  = 0.0;
    f64 unclicked_time = 0.0;

    bool clicked_this_frame = false;
};

class ui_engine
{
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    std::vector<ui_quad> d_quads;

    shader d_shader;

    vertex_buffer d_instances;

    glm::vec2 d_mouse_pos = {0, 0};
    bool      d_hovered   = false;
    bool      d_clicked   = false;
    bool      d_unclicked = false;

    u64 d_clicked_quad = u64_max;
    f64 d_dt = 0.0;

    std::unordered_map<u64, ui_quad_times> d_times;

    ui_engine(const ui_engine&) = delete;
    ui_engine& operator=(const ui_engine&) = delete;

public:
    ui_engine();
    ~ui_engine();

    // Step 1: process events
    bool on_event(const event& e);

    // Step 2: setup ui elements    
    bool button(glm::vec2 pos, float width, float height);
    
    // Step 3: draw
    void draw_frame(const camera& c, f64 dt);
};

}