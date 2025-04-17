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
struct ui_graphics_quad
{
    glm::vec2 centre;
    float     width;
    float     height;
    float     angle;
    glm::vec4 colour;

    static void set_buffer_attributes(std::uint32_t vbo);
};

struct ui_logic_quad
{
    glm::vec2 centre = {0, 0};
    f32       width = 0;
    f32       height = 0;
    bool      active = false;
    
    f64 hovered_time   = 0.0;
    f64 clicked_time   = 0.0;
    f64 unhovered_time = 0.0;
    f64 unclicked_time = 0.0;

    bool hovered_this_frame = false;
    bool clicked_this_frame = false;
    bool unhovered_this_frame = false;
    bool unclicked_this_frame = false;

    auto is_hovered() const -> bool { return hovered_time > unhovered_time; }
    auto is_clicked() const -> bool { return clicked_time > unclicked_time; }

    auto time_hovered(f64 now) const -> f64 { return glm::max(0.0, now - hovered_time); }
    auto time_clicked(f64 now) const -> f64 { return glm::max(0.0, now - clicked_time); }
    auto time_unhovered(f64 now) const -> f64 { return glm::max(0.0, now - unhovered_time); }
    auto time_unclicked(f64 now) const -> f64 { return glm::max(0.0, now - unclicked_time); }
};

class ui_engine
{
    u32 d_vao;
    u32 d_vbo;
    u32 d_ebo;

    std::vector<ui_graphics_quad> d_quads;

    shader d_shader;

    vertex_buffer d_instances;

    // Data from events
    glm::vec2 d_mouse_pos            = {0, 0};
    bool      d_clicked_this_frame   = false;
    bool      d_unclicked_this_frame = false;
    
    // Data from update
    f64       d_time                 = 0.0;
    bool      d_capture_mouse        = false;

    std::unordered_map<std::string_view, ui_logic_quad> d_data;

    ui_logic_quad& get_data(std::string_view name, glm::vec2 pos, f32 width, f32 height) { 
        auto& data = d_data[name];
        data.active = true; // keep this alive
        data.centre = pos + glm::vec2{width/2, height/2};
        data.width = width;
        data.height = height;
        return data;
    }

    ui_engine(const ui_engine&) = delete;
    ui_engine& operator=(const ui_engine&) = delete;

public:
    ui_engine();
    ~ui_engine();

    // Step 1: process events
    bool on_event(const event& e);

    // Step 2: setup ui elements    
    bool button(std::string_view name, glm::vec2 pos, float width, float height);
    
    // Step 3: draw
    void draw_frame(const camera& c, f64 dt);
};

}