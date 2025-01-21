#include "shape_renderer.hpp"

#include "render_context.hpp"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>

namespace sand {
namespace {

constexpr std::vector<quad_vertex> get_quad_vertices()
{
    return { {{-1, -1}}, {{-1, 1}}, {{1, 1}}, {{1, -1}} };
}

constexpr std::vector<std::uint32_t> get_quad_indices()
{
    return {0, 1, 2, 0, 2, 3};
}

constexpr auto line_vertex = R"SHADER(
#version 410 core
layout (location = 0) in vec2 position;

layout (location = 1) in vec2  line_begin;
layout (location = 2) in vec2  line_end;
layout (location = 3) in vec4  line_begin_colour;
layout (location = 4) in vec4  line_end_colour;
layout (location = 5) in float line_thickness;

out vec2  o_line_begin;
out vec2  o_line_end;
out vec4  o_line_begin_colour;
out vec4  o_line_end_colour;
out float o_line_thickness;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);

    o_line_begin = line_begin;
    o_line_end = line_end;
    o_line_begin_colour = line_begin_colour;
    o_line_end_colour = line_end_colour;
    o_line_thickness = line_thickness;
} 
)SHADER";

constexpr auto line_fragment = R"SHADER(
#version 410 core
layout (location = 0) out vec4 out_colour;

in vec2  o_line_begin;
in vec2  o_line_end;
in vec4  o_line_begin_colour;
in vec4  o_line_end_colour;
in float o_line_thickness;

uniform float u_width;
uniform float u_height;

uniform float u_camera_width;
uniform float u_camera_height;
uniform vec2 u_camera_top_left;
uniform float u_camera_world_to_screen;

float cross2d(vec2 a, vec2 b)
{
    return a.x * b.y - b.x * a.y;
}

void main()
{   
    vec2 pixel = vec2(gl_FragCoord.x, u_height - gl_FragCoord.y);

    if (o_line_begin == o_line_end && distance(pixel, o_line_begin) < o_line_thickness) {
        out_colour = o_line_begin_colour;
        return;
    }

    vec2 A = o_line_end - o_line_begin;
    vec2 B = pixel - o_line_begin;
    float lengthA = length(A);

    float distance_from_line = abs(cross2d(A, B)) / lengthA;

    float ratio_along = dot(A, B) / (lengthA * lengthA);

    if (distance_from_line <= o_line_thickness) {
        if (ratio_along > 0 && ratio_along < 1) {
            out_colour = mix(o_line_begin_colour, o_line_end_colour, ratio_along);
        } else if (ratio_along <= 0 && distance(o_line_begin, pixel) < o_line_thickness) {
            out_colour = o_line_begin_colour;
        } else if (ratio_along >= 1 && distance(o_line_end, pixel) < o_line_thickness) {
            out_colour = o_line_end_colour;
        } else {
            out_colour = vec4(0.0);
        }
    }
}
)SHADER";

constexpr auto circle_vertex = R"SHADER(
#version 410 core
layout (location = 0) in vec2 position;

layout (location = 1) in vec2  circle_centre;
layout (location = 2) in float circle_inner_radius;
layout (location = 3) in float circle_outer_radius;
layout (location = 4) in vec4  circle_begin_colour;
layout (location = 5) in vec4  circle_end_colour;
layout (location = 6) in float circle_angle;

out vec2  o_circle_centre;
out float o_circle_inner_radius;
out float o_circle_outer_radius;
out vec4  o_circle_begin_colour;
out vec4  o_circle_end_colour;
out float o_circle_angle;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    
    o_circle_centre = circle_centre;
    o_circle_inner_radius = circle_inner_radius;
    o_circle_outer_radius = circle_outer_radius;
    o_circle_begin_colour = circle_begin_colour;
    o_circle_end_colour = circle_end_colour;
    o_circle_angle = circle_angle;
} 
)SHADER";

constexpr auto circle_fragment = R"SHADER(
#version 410 core
layout (location = 0) out vec4 out_colour;

in vec2  o_circle_centre;
in float o_circle_inner_radius;
in float o_circle_outer_radius;
in vec4  o_circle_begin_colour;
in vec4  o_circle_end_colour;
in float o_circle_angle;

uniform float u_width;
uniform float u_height;

uniform float u_camera_width;
uniform float u_camera_height;
uniform vec2 u_camera_top_left;
uniform float u_camera_world_to_screen;

const float pi = 3.1415926535897932384626433832795;

// Rotates the given vec2 anti-clockwise by the given amount of radians.
vec2 rot(vec2 v, float rad)
{
    float s = sin(rad);
	float c = cos(rad);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

// Returns 0 for vectors on positive x axis, 0.5, for negative x axis, increasing
// rotating clockwise.
float rotation_value(vec2 v)
{
    if (v.y >= 0) { // Top right
        float cos_theta = normalize(-v).x;
        float theta = acos(cos_theta) + pi;
        return theta / (2 * pi);
    }
    else {
        float cos_theta = normalize(v).x;
        float theta = acos(cos_theta);
        return theta / (2 * pi);
    }
}

void main()
{   
    vec2 pixel = vec2(gl_FragCoord.x, u_height - gl_FragCoord.y);
    vec2 to_pixel = pixel - o_circle_centre;
    float from_centre = length(to_pixel);
    
    if (o_circle_inner_radius < from_centre && from_centre < o_circle_outer_radius) {
        float angle_offset = o_circle_angle / (2 * pi);
        out_colour = mix(
            o_circle_begin_colour,
            o_circle_end_colour,
            rotation_value(rot(to_pixel, -o_circle_angle))
        );
        return;
    }
}
)SHADER";

}

void quad_vertex::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(quad_vertex), (void*)offsetof(quad_vertex, position));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void line_instance::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 6; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(line_instance), (void*)offsetof(line_instance, begin));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(line_instance), (void*)offsetof(line_instance, end));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(line_instance), (void*)offsetof(line_instance, begin_colour));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(line_instance), (void*)offsetof(line_instance, end_colour));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(line_instance), (void*)offsetof(line_instance, thickness));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void circle_instance::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 7; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, centre));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, inner_radius));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, outer_radius));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, begin_colour));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, end_colour));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(circle_instance), (void*)offsetof(circle_instance, angle));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

shape_renderer::shape_renderer()
    : d_quad_vertices(get_quad_vertices())
    , d_quad_indices(get_quad_indices())
    , d_line_shader(line_vertex, line_fragment)
    , d_circle_shader(circle_vertex, circle_fragment)
{
    glGenVertexArrays(1, &d_vao);
}

shape_renderer::~shape_renderer()
{
    glDeleteVertexArrays(1, &d_vao);
}

void shape_renderer::begin_frame(const float width, const float height, const camera& c)
{
    glBindVertexArray(d_vao);

    d_line_shader.bind();
    d_line_shader.load_float("u_width", width);
    d_line_shader.load_float("u_height", height);
    d_line_shader.load_float("u_camera_width", c.screen_width);
    d_line_shader.load_float("u_camera_height", c.screen_height);
    d_line_shader.load_vec2("u_camera_top_left", c.top_left);
    d_line_shader.load_float("u_camera_world_to_screen", c.world_to_screen);
    d_lines.clear();

    d_circle_shader.bind();
    d_circle_shader.load_float("u_width", width);
    d_circle_shader.load_float("u_height", height);
    d_circle_shader.load_float("u_camera_width", c.screen_width);
    d_circle_shader.load_float("u_camera_height", c.screen_height);
    d_circle_shader.load_vec2("u_camera_top_left", c.top_left);
    d_circle_shader.load_float("u_camera_world_to_screen", c.world_to_screen);
    d_circles.clear();
}

void shape_renderer::end_frame()
{
    sand::render_context rc;
    rc.alpha_blending(true);
    rc.depth_testing(false);

    d_quad_vertices.bind();
    d_quad_indices.bind();

    const auto draw = [&](auto& instances, auto& shader, auto& data)
    {
        instances.set_data(data);
        shader.bind();
        instances.bind();
        glDrawElementsInstanced(
            GL_TRIANGLES, (int)d_quad_indices.size(),
            GL_UNSIGNED_INT, nullptr, (int)instances.size()
        );
        shader.unbind();
    };

    draw(d_line_instances, d_line_shader, d_lines);
    draw(d_circle_instances, d_circle_shader, d_circles);
}

void shape_renderer::draw_line(
    const glm::vec2& begin,
    const glm::vec2& end,
    const glm::vec4& begin_colour,
    const glm::vec4& end_colour,
    const float thickness)
{
    d_lines.emplace_back(begin, end, begin_colour, end_colour, thickness);
}

void shape_renderer::draw_circle_shape(
    const glm::vec2& centre,
    const float      inner_radius,
    const float      outer_radius,
    const glm::vec4& begin_colour,
    const glm::vec4& end_colour,
    const float      angle)
{
    d_circles.emplace_back(centre, inner_radius, outer_radius, begin_colour, end_colour, angle);
}

void shape_renderer::draw_circle(
    const glm::vec2& centre,
    const glm::vec4& colour,
    const float radius)
{
    d_circles.emplace_back(centre, 0.0f, radius, colour, colour, 0.0f);
}

void shape_renderer::draw_annulus(
    const glm::vec2& centre,
    const glm::vec4& colour,
    const float inner_radius,
    const float outer_radius)
{
    d_circles.emplace_back(centre, inner_radius, outer_radius, colour, colour, 0.0f);
}
    
}