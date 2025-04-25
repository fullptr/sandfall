#pragma once
#include "buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "event.hpp"
#include "common.hpp"

#include <array>
#include <unordered_map>
#include <source_location>

#include <glm/glm.hpp>

namespace sand {

class widget_key
{
    std::string_view d_file;
    u64              d_line;
    u64              d_column;

public:
    widget_key(std::source_location loc = std::source_location::current())
        : d_file{loc.file_name()}
        , d_line{loc.line()}
        , d_column{loc.column()}
    {}
    auto operator<=>(const widget_key&) const = default;
    auto operator==(const widget_key&) const -> bool = default;
    auto hash() const -> std::size_t { return 1; }
};

}

template<>
struct std::hash<sand::widget_key>
{
    auto operator()(const sand::widget_key& wk) const -> std::size_t
    {
        return wk.hash();
    }
};

namespace sand {

struct character
{
    glm::ivec2 position;
    glm::ivec2 size;
    glm::ivec2 bearing;
    i32        advance;
};

struct font_atlas
{
    std::unique_ptr<texture_png>        texture;
    std::unordered_map<char, character> chars;
    character                           missing_char;
    i32                                 height; // height of an "a", used for centring

    auto get_character(char c) const -> const character&;
    auto length_of(std::string_view message) -> i32;
};

// This is just a copy of quad_instance from the shape_renderer, should
// we combine these? I'm just making a copy now since I am assuming both will
// iterate in different directions and I don't necessarily want them tied
// together, but I still feel conflicted.
struct ui_graphics_quad
{
    glm::ivec2 top_left;
    int        width;
    int        height;
    float      angle;
    glm::vec4  colour;
    int        use_texture;
    glm::ivec2 uv_pos;
    glm::ivec2 uv_size;

    static void set_buffer_attributes(std::uint32_t vbo);
};

struct ui_logic_quad
{
    glm::vec2 top_left = {0, 0};
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

    std::unordered_map<widget_key, ui_logic_quad> d_data;

    font_atlas d_atlas;

    const ui_logic_quad& get_data(const widget_key& key, glm::vec2 top_left, f32 width, f32 height) { 
        auto& data = d_data[key];
        data.active = true; // keep this alive
        data.top_left = top_left;
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
    bool button(std::string_view msg, glm::ivec2 pos, i32 width, i32 height, i32 scale, const widget_key& key = {});
    void text(std::string_view message, glm::ivec2 pos, i32 size);
    void text_box(std::string_view message, glm::ivec2 pos, i32 width, i32 height, i32 size);
    
    // Step 3: draw
    void draw_frame(i32 screen_width, i32 screen_height, f64 dt);
};

}