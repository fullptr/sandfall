#include "ui.hpp"
#include "common.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ranges>

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
    d_dt = dt;
    d_hovered = false;
    for (const auto& quad : d_quads) {
        d_times[quad.hash()].clicked_this_frame = false;
        if (is_in_region(d_mouse_pos, quad)) {
            d_hovered = true;
            if (d_clicked) {
                d_clicked_quad = quad.hash();
                d_times[quad.hash()].clicked_this_frame = true;
                d_times[quad.hash()].clicked_time = d_dt;
            }
        }
    }
    if (d_unclicked && d_clicked_quad != u64_max) {
        d_times[d_clicked_quad].unclicked_time = d_dt;
        d_clicked_quad = u64_max;
    }
    d_unclicked = false;
    d_clicked = false;
    
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
        if (e->button == mouse::left && d_hovered) {
            d_clicked = true;
            return true;
        }
    }
    else if (const auto e = event.get_if<mouse_released_event>()) {
        d_unclicked = true;
    }
    return false;
}

bool ui_engine::button(glm::vec2 pos, float width, float height)
{
    auto quad = ui_quad{pos + glm::vec2{width/2, height/2}, width, height, 0.0f, glm::vec4{1, 0, 0, 1}};
    if (is_in_region(d_mouse_pos, quad)) {
        quad.colour = {0, 1, 0, 1};
        d_times[quad.hash()].hovered_time = d_dt;
    } else {
        d_times[quad.hash()].unhovered_time = d_dt;
    }
    if (quad.hash() == d_clicked_quad) {
        quad.colour = {1, 1, 0, 1};
    }
    d_quads.emplace_back(quad);
    return d_times[quad.hash()].clicked_this_frame;
}

}