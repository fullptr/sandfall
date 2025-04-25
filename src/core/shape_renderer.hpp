#pragma once
#include "buffer.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "common.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace sand {

struct line_instance
{
    glm::vec2 begin;
    glm::vec2 end;
    glm::vec4 begin_colour;
    glm::vec4 end_colour;
    float     thickness;

    static void set_buffer_attributes(u32 vbo);
};

struct circle_instance
{
    glm::vec2 centre;
    float     inner_radius;
    float     outer_radius;
    glm::vec4 begin_colour;
    glm::vec4 end_colour;
    float     angle;

    static void set_buffer_attributes(u32 vbo);
};

struct quad_instance
{
    glm::vec2 centre;
    float     width;
    float     height;
    float     angle;
    glm::vec4 colour;

    static void set_buffer_attributes(u32 vbo);
};

class shape_renderer
{
    u32 d_vao;
    u32 d_vbo;
    u32 d_ebo;

    std::vector<quad_instance>   d_quads;
    std::vector<line_instance>   d_lines;
    std::vector<circle_instance> d_circles;

    shader d_quad_shader;
    shader d_line_shader;
    shader d_circle_shader;

    vertex_buffer d_instances;

public:
    shape_renderer();
    ~shape_renderer();

    void begin_frame(const camera& c);
    void end_frame();

    void draw_rect(glm::vec2 top_left, float width, float height, glm::vec4 colour);
    void draw_quad(glm::vec2 centre, float width, float height, float angle, glm::vec4 colour);
    void draw_line(glm::vec2 begin, glm::vec2 end, glm::vec4 begin_colour, glm::vec4 end_colour, float thickness);
    void draw_line(glm::vec2 begin, glm::vec2 end, glm::vec4 colour, float thickness);
    void draw_circle(glm::vec2 centre, glm::vec4 colour, float radius);
    void draw_annulus(glm::vec2 centre, glm::vec4 colour, float inner_radius, float outer_radius);
};

}