#include "ui.hpp"
#include "common.hpp"
#include "utility.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ranges>
#include <print>

namespace sand {
namespace {

auto load_pixel_font_atlas() -> font_atlas
{
    font_atlas atlas;
    atlas.texture = std::make_unique<texture_png>("pixel_font.png");
    atlas.missing_char = { .position{24, 52}, .size{5, 6}, .bearing{0, -6}, .advance=6 };

    atlas.chars['A'] = character{ .position{0, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['B'] = character{ .position{6, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['C'] = character{ .position{12, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['D'] = character{ .position{18, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['E'] = character{ .position{24, 0}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['F'] = character{ .position{29, 0}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['G'] = character{ .position{34, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['H'] = character{ .position{40, 0}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['I'] = character{ .position{46, 0}, .size={3, 7}, .bearing={0, -7}, .advance=4 };
    atlas.chars['J'] = character{ .position{50, 0}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['K'] = character{ .position{55, 0}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['L'] = character{ .position{60, 0}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['M'] = character{ .position{0, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['N'] = character{ .position{6, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['O'] = character{ .position{12, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['P'] = character{ .position{18, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['Q'] = character{ .position{24, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['R'] = character{ .position{30, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['S'] = character{ .position{36, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['T'] = character{ .position{42, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['U'] = character{ .position{48, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['V'] = character{ .position{54, 8}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['W'] = character{ .position{0, 16}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['X'] = character{ .position{6, 16}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['Y'] = character{ .position{12, 16}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['Z'] = character{ .position{18, 16}, .size={5, 7}, .bearing={0, -7}, .advance=6 };

    atlas.chars['a'] = character{ .position{24, 18}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['b'] = character{ .position{29, 16}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['c'] = character{ .position{34, 18}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['d'] = character{ .position{39, 16}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['e'] = character{ .position{44, 18}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['f'] = character{ .position{49, 16}, .size={3, 10}, .bearing={0, -7}, .advance=4 };
    atlas.chars['g'] = character{ .position{53, 18}, .size={4, 8}, .bearing={0, -5}, .advance=5 };
    atlas.chars['h'] = character{ .position{58, 16}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['i'] = character{ .position{63, 16}, .size={1, 7}, .bearing={0, -7}, .advance=2 };
    atlas.chars['j'] = character{ .position{1, 24}, .size={2, 10}, .bearing={0, -7}, .advance=3 };
    atlas.chars['k'] = character{ .position{4, 24}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['l'] = character{ .position{9, 24}, .size={1, 7}, .bearing={0, -7}, .advance=2 };
    atlas.chars['m'] = character{ .position{11, 26}, .size={5, 5}, .bearing={0, -5}, .advance=6 };
    atlas.chars['n'] = character{ .position{17, 26}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['o'] = character{ .position{22, 26}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['p'] = character{ .position{27, 26}, .size={4, 7}, .bearing={0, -5}, .advance=5 };
    atlas.chars['q'] = character{ .position{32, 26}, .size={5, 7}, .bearing={0, -5}, .advance=5 };
    atlas.chars['r'] = character{ .position{37, 26}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['s'] = character{ .position{42, 26}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['t'] = character{ .position{47, 24}, .size={2, 7}, .bearing={0, -7}, .advance=3 };
    atlas.chars['u'] = character{ .position{50, 26}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['v'] = character{ .position{55, 26}, .size={5, 5}, .bearing={0, -5}, .advance=6 };
    atlas.chars['w'] = character{ .position{0, 34}, .size={5, 5}, .bearing={0, -5}, .advance=6 };
    atlas.chars['x'] = character{ .position{6, 34}, .size={4, 5}, .bearing={0, -5}, .advance=5 };
    atlas.chars['y'] = character{ .position{11, 34}, .size={4, 8}, .bearing={0, -5}, .advance=5 };
    atlas.chars['z'] = character{ .position{16, 34}, .size={4, 5}, .bearing={0, -5}, .advance=5 };

    atlas.chars['1'] = character{ .position{0, 43}, .size={3, 7}, .bearing={0, -7}, .advance=4 };
    atlas.chars['2'] = character{ .position{4, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['3'] = character{ .position{10, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['4'] = character{ .position{16, 43}, .size={4, 7}, .bearing={0, -7}, .advance=5 };
    atlas.chars['5'] = character{ .position{21, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['6'] = character{ .position{27, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['7'] = character{ .position{33, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['8'] = character{ .position{39, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['9'] = character{ .position{45, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['0'] = character{ .position{51, 43}, .size={5, 7}, .bearing={0, -7}, .advance=6 };

    atlas.chars['!'] = character{ .position{60, 8}, .size={1, 7}, .bearing={0, -7}, .advance=2 };
    atlas.chars[';'] = character{ .position{62, 10}, .size={2, 7}, .bearing={0, -5}, .advance=3 };
    atlas.chars[':'] = character{ .position{63, 10}, .size={1, 4}, .bearing={0, -5}, .advance=2 };
    atlas.chars[','] = character{ .position{61, 29}, .size={2, 2}, .bearing={0, -2}, .advance=3 };
    atlas.chars['.'] = character{ .position{62, 29}, .size={1, 1}, .bearing={0, -1}, .advance=2 };
    atlas.chars[' '] = character{ .position{0, 0}, .size={1, 1}, .bearing={0, 0}, .advance=3 };
    atlas.chars['('] = character{ .position{38, 32}, .size={2, 9}, .bearing={0, -8}, .advance=3 };
    atlas.chars[')'] = character{ .position{41, 32}, .size={2, 9}, .bearing={0, -8}, .advance=3 };
    atlas.chars['{'] = character{ .position{44, 32}, .size={3, 9}, .bearing={0, -8}, .advance=4 };
    atlas.chars['}'] = character{ .position{48, 32}, .size={3, 9}, .bearing={0, -8}, .advance=4 };
    atlas.chars['['] = character{ .position{30, 51}, .size={2, 9}, .bearing={0, -8}, .advance=3 };
    atlas.chars[']'] = character{ .position{33, 51}, .size={2, 9}, .bearing={0, -8}, .advance=3 };
    atlas.chars['^'] = character{ .position{52, 32}, .size={5, 3}, .bearing={0, -7}, .advance=6 };
    atlas.chars['<'] = character{ .position{58, 32}, .size={3, 5}, .bearing={0, -6}, .advance=4 };
    atlas.chars['>'] = character{ .position{61, 34}, .size={3, 5}, .bearing={0, -6}, .advance=4 };
    atlas.chars['_'] = character{ .position{1, 41}, .size={4, 1}, .bearing={0, 0}, .advance=5 };
    atlas.chars['-'] = character{ .position{1, 41}, .size={3, 1}, .bearing={0, -4}, .advance=4 };
    atlas.chars['+'] = character{ .position{0, 51}, .size={3, 3}, .bearing={0, -5}, .advance=4 };
    atlas.chars['='] = character{ .position{4, 51}, .size={3, 3}, .bearing={0, -5}, .advance=4 };
    atlas.chars['/'] = character{ .position{21, 32}, .size={3, 7}, .bearing={0, -7}, .advance=4 };
    atlas.chars['@'] = character{ .position{12, 51}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['%'] = character{ .position{18, 51}, .size={5, 7}, .bearing={0, -7}, .advance=6 };
    atlas.chars['\\'] = character{ .position{24, 32}, .size={3, 7}, .bearing={0, -7}, .advance=4 };
    atlas.chars['\"'] = character{ .position{8, 51}, .size={3, 2}, .bearing={0, -7}, .advance=4 };
    return atlas;
}

constexpr auto quad_vertex = R"SHADER(
    #version 410 core
    layout (location = 0) in vec2 p_position;
    
    layout (location = 1) in vec2  quad_top_left;
    layout (location = 2) in float quad_width;
    layout (location = 3) in float quad_height;
    layout (location = 4) in float quad_angle;
    layout (location = 5) in vec4  quad_colour;
    layout (location = 6) in int   quad_use_texture;
    layout (location = 7) in ivec2 quad_uv_pos;
    layout (location = 8) in ivec2 quad_uv_size;
    
    uniform mat4      u_proj_matrix;
    uniform sampler2D u_texture;
    
    flat out int o_use_texture;
    out vec4     o_colour;
    out vec2     o_uv;
    
    mat2 rotate(float theta)
    {
        float c = cos(theta);
        float s = sin(theta);
        return mat2(c, s, -s, c);
    }
    
    void main()
    {
        vec2 dimensions = vec2(quad_width, quad_height) / 2;
        vec2 position = quad_top_left + dimensions;
    
        vec2 screen_position = rotate(quad_angle) * (p_position * dimensions) + position;
    
        gl_Position = u_proj_matrix * vec4(screen_position, 0, 1);
    
        o_use_texture = quad_use_texture;
        o_colour = quad_colour;

        o_uv = (p_position + vec2(1, 1)) / 2;
        o_uv = (o_uv * quad_uv_size + quad_uv_pos) / textureSize(u_texture, 0);
    }
)SHADER";
    
constexpr auto quad_fragment = R"SHADER(
    #version 410 core
    layout (location = 0) out vec4 out_colour;
    
    flat in int o_use_texture;
    in vec4     o_colour;
    in vec2     o_uv;

    uniform int       u_use_texture;
    uniform sampler2D u_texture;
    
    void main()
    {
        if (o_use_texture > 0) {
            float red = texture(u_texture, o_uv).r;
            out_colour = vec4(o_colour.xyz, red);
        } else {
            out_colour = o_colour;
        }
    }
)SHADER";

}

auto font_atlas::get_character(char c) const -> const character&
{
    if (auto it = chars.find(c); it != chars.end()) {
        return it->second;
    }
    return missing_char;
}

void ui_graphics_quad::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 9; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, top_left));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, width));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, height));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, angle));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, colour));
    glVertexAttribIPointer(6, 1, GL_INT, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, use_texture));
    glVertexAttribIPointer(7, 2, GL_INT, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, uv_pos));
    glVertexAttribIPointer(8, 2, GL_INT, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, uv_size));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ui_engine::ui_engine()
    : d_shader(quad_vertex, quad_fragment)
    , d_atlas{load_pixel_font_atlas()}
{
    const float vertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
    const std::uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &d_vao);
    glBindVertexArray(d_vao);

    glGenBuffers(1, &d_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &d_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    d_shader.bind();
    d_shader.load_sampler("u_texture", 0);
}

ui_engine::~ui_engine()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

static auto is_in_region(glm::vec2 pos, glm::vec2 top_left, f32 width, f32 height) -> bool
{
    return top_left.x <= pos.x && pos.x < top_left.x + width
        && top_left.y <= pos.y && pos.y < top_left.y + height;
}

void ui_engine::draw_frame(i32 screen_width, i32 screen_height, f64 dt)
{
    // Clean out any elements no longer around
    std::erase_if(d_data, [&](auto& elem) {
        return !elem.second.active;
    });

    d_time += dt;
    d_capture_mouse = false;
    
    for (auto& [hash, data] : d_data) {
        data.active = false; // if made next frame, it will activate again
        data.hovered_this_frame = false;
        data.clicked_this_frame = false;
        data.unhovered_this_frame = true;
        data.unclicked_this_frame = true;
        
        if (d_unclicked_this_frame && data.is_clicked()) {
            data.unclicked_this_frame = true;
            data.unclicked_time = d_time;
        }
        
        if (is_in_region(d_mouse_pos, data.top_left, data.width, data.height)) {
            d_capture_mouse = true;

            if (!data.is_hovered()) {
                data.hovered_this_frame = true;
                data.hovered_time = d_time;
            }
            if (d_clicked_this_frame) {
                data.clicked_this_frame = true;
                data.clicked_time = d_time;
            }
        } else if (data.is_hovered()) {
            data.unhovered_this_frame = true;
            data.unhovered_time = d_time;
        }
    }
    
    d_clicked_this_frame = false;
    d_unclicked_this_frame = false;
    
    glBindVertexArray(d_vao);
    d_atlas.texture->bind();
    d_shader.load_int("u_use_texture", 1);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto dimensions = glm::vec2{screen_width, screen_height};
    const auto projection = glm::ortho(0.0f, dimensions.x, dimensions.y, 0.0f);
    
    d_shader.bind();
    d_shader.load_mat4("u_proj_matrix", projection);
    d_instances.bind<ui_graphics_quad>(d_quads);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (int)d_quads.size());

    glDisable(GL_BLEND);

    d_quads.clear();
}

bool ui_engine::on_event(const event& event)
{
    if (const auto e = event.get_if<mouse_moved_event>()) {
        d_mouse_pos = e->pos;
    }
    else if (const auto e = event.get_if<mouse_pressed_event>()) {
        if (e->button == mouse::left && d_capture_mouse) {
            d_clicked_this_frame = true;
            return true;
        }
    }
    else if (const auto e = event.get_if<mouse_released_event>()) {
        d_unclicked_this_frame = true;
    }
    return false;
}

bool ui_engine::button(std::string_view name, glm::vec2 pos, f32 width, f32 height)
{
    const auto& data = get_data(name, pos, width, height);
    
    constexpr auto unhovered_colour = from_hex(0x17c0eb);
    constexpr auto hovered_colour = from_hex(0x18dcff);
    constexpr auto clicked_colour = from_hex(0xfffa65);

    const auto lerp_time = 0.1;
    
    auto colour = unhovered_colour;
    auto extra_width = 0.0f;
    if (data.is_clicked()) {
        colour = clicked_colour;
        extra_width = 10.0f;
    }
    else if (data.is_hovered()) {
        const auto t = std::clamp(data.time_hovered(d_time) / lerp_time, 0.0, 1.0);
        colour = sand::lerp(unhovered_colour, hovered_colour, t);
        extra_width = sand::lerp(0.0f, 10.0f, t);
    }
    else if (d_time > lerp_time) { // Don't start the game looking hovered
        const auto t = std::clamp(data.time_unhovered(d_time) / lerp_time, 0.0, 1.0);
        colour = sand::lerp(hovered_colour, unhovered_colour, t);
        extra_width = sand::lerp(10.0f, 0.0f, t);
    }
    
    const auto quad = ui_graphics_quad{pos, width + extra_width, height, 0.0f, colour, 0, {0, 0}, {0, 0}};
    d_quads.emplace_back(quad);
    return data.clicked_this_frame;
}

void ui_engine::text(std::string_view message, glm::vec2 pos)
{
    constexpr auto colour = from_hex(0xd2dae2);
    const auto scale = 3.0f;
    for (char c : message) {
        const auto ch = d_atlas.get_character(c);
        const auto sizef = glm::vec2{ch.size};
        const auto bearingf = glm::vec2{ch.bearing};
        const auto quad = ui_graphics_quad{
            pos + (scale * bearingf),
            scale * (sizef.x),
            scale * (sizef.y),
            0.0f,
            colour,
            1,
            ch.position,
            ch.size
        };
        d_quads.emplace_back(quad);
        pos.x += scale * ch.advance;
    }
}

}