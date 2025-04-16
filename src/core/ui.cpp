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

constexpr auto quad_vertex = R"SHADER(
    #version 410 core
    layout (location = 0) in vec2 p_position;
    
    layout (location = 1) in vec2  quad_centre;
    layout (location = 2) in float quad_width;
    layout (location = 3) in float quad_height;
    layout (location = 4) in float quad_angle;
    layout (location = 5) in vec4  quad_colour;
    
    uniform mat4 u_proj_matrix;
    
    out vec4 o_colour;
    
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
    
        o_colour = quad_colour;
    }
    )SHADER";
    
constexpr auto quad_fragment = R"SHADER(
    #version 410 core
    layout (location = 0) out vec4 out_colour;
    
    in vec4 o_colour;
    
    void main()
    {
        out_colour = o_colour;
    }
    )SHADER";

}

void ui_quad::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 6; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ui_quad), (void*)offsetof(ui_quad, centre));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(ui_quad), (void*)offsetof(ui_quad, width));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ui_quad), (void*)offsetof(ui_quad, height));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ui_quad), (void*)offsetof(ui_quad, angle));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ui_quad), (void*)offsetof(ui_quad, colour));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ui_engine::ui_engine()
    : d_shader(quad_vertex, quad_fragment)
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
}

ui_engine::~ui_engine()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

static auto is_in_region(glm::vec2 pos, const ui_quad& quad) -> bool
{
    return (quad.centre.x - quad.width / 2) <= pos.x && pos.x < (quad.centre.x + quad.width / 2)
        && (quad.centre.y - quad.height / 2) <= pos.y && pos.y < (quad.centre.y + quad.height / 2);
}

void ui_engine::draw_frame(const camera& c, f64 dt)
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
        
        if (is_in_region(d_mouse_pos, data.quad)) {
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

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto dimensions = glm::vec2{c.screen_width, c.screen_height};
    const auto projection = glm::ortho(0.0f, dimensions.x, dimensions.y, 0.0f);
    
    d_shader.bind();
    d_shader.load_mat4("u_proj_matrix", projection);
    d_instances.bind<ui_quad>(d_quads);
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

bool ui_engine::button(std::string_view name, glm::vec2 pos, float width, float height)
{
    auto quad = ui_quad{pos + glm::vec2{width/2, height/2}, width, height, 0.0f, glm::vec4{1, 0, 0, 1}};
    auto& data = get_data(name);
    data.quad = quad;
    
    const auto hovered_colour = glm::vec4{1, 0, 1, 1};
    const auto unhovered_colour = glm::vec4{1, 0, 0, 1};

    if (data.is_clicked()) {
        quad.colour = {1, 1, 0, 1};
    }
    else if (data.is_hovered()) {
        const auto t = std::min(1.0, data.time_hovered(d_time) / 0.2);
        quad.colour = sand::lerp(unhovered_colour, hovered_colour, t);;
    }
    else {
        const auto t = std::min(1.0, data.time_unhovered(d_time) / 0.2);
        quad.colour = sand::lerp(hovered_colour, unhovered_colour, t);;
    }

    d_quads.emplace_back(quad);
    return data.clicked_this_frame;
}

}