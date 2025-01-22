#pragma once
#include "graphics/buffer.hpp"
#include "graphics/shader.hpp"
#include "camera.hpp"

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

    static void set_buffer_attributes(std::uint32_t vbo);
};

struct circle_instance
{
    glm::vec2 centre;
    float     inner_radius;
    float     outer_radius;
    glm::vec4 begin_colour;
    glm::vec4 end_colour;
    float     angle;

    static void set_buffer_attributes(std::uint32_t vbo);
};

class shape_renderer
{
    std::uint32_t d_vao;
    std::uint32_t d_vbo;
    std::uint32_t d_ebo;

    sand::shader                       d_line_shader;
    std::vector<line_instance>         d_lines;
    sand::vertex_buffer<line_instance> d_line_instances;

    sand::shader                         d_circle_shader;
    std::vector<circle_instance>         d_circles;
    sand::vertex_buffer<circle_instance> d_circle_instances;

public:
    shape_renderer();
    ~shape_renderer();

    void begin_frame(const camera& c);
    void end_frame();

    void draw_line(
        const glm::vec2& begin,
        const glm::vec2& end,
        const glm::vec4& begin_colour,
        const glm::vec4& end_colour,
        const float      thickness
    );

    void draw_circle_shape(
        const glm::vec2& centre,
        const float      inner_radius,
        const float      outer_radius,
        const glm::vec4& begin_colour,
        const glm::vec4& end_colour,
        const float      angle
    );

    void draw_circle(
        const glm::vec2& centre,
        const glm::vec4& colour,
        const float      radius
    );

    void draw_annulus(
        const glm::vec2& centre,
        const glm::vec4& colour,
        const float      inner_radius,
        const float      outer_radius
    );
};

}