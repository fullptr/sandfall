#include "ui.hpp"
#include "common.hpp"
#include "utility.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <ranges>
#include <print>

namespace sand {
namespace {

auto load_font_atlas() -> font_atlas
{
    FT_Library library;
    if (FT_Init_FreeType(&library)) {
        std::print("failed to init FreeType\n");
        std::exit(1);
    }
    
    FT_Face face;
    if (FT_New_Face(library, "C:\\WINDOWS\\FONTS\\ARIAL.ttf", 0, &face))
    {
        std::print("ERROR::FREETYPE: Failed to load font\n");  
        std::exit(1);
    }
    FT_Set_Pixel_Sizes(face, 0, 96);

    if (FT_Load_Char(face, '%', FT_LOAD_RENDER))
    {
        std::print("ERROR::FREETYTPE: Failed to load Glyph\n");
        std::exit(1);
    }

    font_atlas atlas;
    atlas.texture = std::make_unique<texture>(texture_type::red);
    atlas.texture->resize(256, 256);
    std::span<const unsigned char> data{face->glyph->bitmap.buffer, face->glyph->bitmap.buffer + face->glyph->bitmap.width * face->glyph->bitmap.rows};
    atlas.texture->set_subdata(data, {50, 50}, face->glyph->bitmap.width, face->glyph->bitmap.rows);

    return atlas;
}

constexpr auto quad_vertex = R"SHADER(
    #version 410 core
    layout (location = 0) in vec2 p_position;
    
    layout (location = 1) in vec2  quad_centre;
    layout (location = 2) in float quad_width;
    layout (location = 3) in float quad_height;
    layout (location = 4) in float quad_angle;
    layout (location = 5) in vec4  quad_colour;
    layout (location = 6) in int   quad_use_texture;
    
    uniform mat4 u_proj_matrix;
    
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
        vec2 position = quad_centre;
        vec2 dimensions = vec2(quad_width, quad_height) / 2;
    
        vec2 screen_position = rotate(quad_angle) * (p_position * dimensions) + position;
    
        gl_Position = u_proj_matrix * vec4(screen_position, 0, 1);
    
        o_use_texture = quad_use_texture;
        o_colour = quad_colour;
        o_uv = vec2((p_position.x + 1), (p_position.y + 1)) / 2;
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
            out_colour = vec4(1, 1, 1, red);
        } else {
            out_colour = o_colour;
        }
    }
)SHADER";

}

void ui_graphics_quad::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 7; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, centre));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, width));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, height));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, angle));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, colour));
    glVertexAttribPointer(6, 1, GL_INT,   GL_FALSE, sizeof(ui_graphics_quad), (void*)offsetof(ui_graphics_quad, use_texture));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ui_engine::ui_engine()
    : d_shader(quad_vertex, quad_fragment)
    , d_atlas{load_font_atlas()}
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

    std::vector<glm::vec4> texture_data;
    texture_data.resize(256 * 256);
    for (int i = 0; i != 256; ++i) {
        for (int j = 0; j != 256; ++j) {
            const auto r = (float)i / 256.0f;
            const auto g = (float)j / 256.0f;
            texture_data[j * 256 + i] = {r, g, 0, 1};
        }
    }
    d_temp_textures[0].resize(256, 256);
    d_temp_textures[0].set_data(texture_data);
    for (int i = 0; i != 256; ++i) {
        for (int j = 0; j != 256; ++j) {
            const auto r = (float)i / 256.0f;
            const auto g = (float)j / 256.0f;
            texture_data[j * 256 + i] = {r, 0, g, 1};
        }
    }
    d_temp_textures[1].resize(256, 256);
    d_temp_textures[1].set_data(texture_data);

    d_shader.bind();
    d_shader.load_sampler("u_texture", 0);
}

ui_engine::~ui_engine()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

static auto is_in_region(glm::vec2 pos, glm::vec2 centre, f32 width, f32 height) -> bool
{
    return (centre.x - width / 2) <= pos.x && pos.x < (centre.x + width / 2)
    && (centre.y - height / 2) <= pos.y && pos.y < (centre.y + height / 2);
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
        
        if (is_in_region(d_mouse_pos, data.centre, data.width, data.height)) {
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
    //glBlendEquation(GL_FUNC_ADD);
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
    
    const auto quad = ui_graphics_quad{data.centre, width + extra_width, height, 0.0f, colour, 0};
    d_quads.emplace_back(quad);
    return data.clicked_this_frame;
}

void ui_engine::text(std::string_view message)
{
    const auto quad = ui_graphics_quad{{600, 300}, 256, 256, 0.0f, {0, 0, 0, 0}, 1};
    d_quads.emplace_back(quad);
}

}