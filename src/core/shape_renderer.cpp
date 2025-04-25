#include "shape_renderer.hpp"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>

namespace sand {
namespace {

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

uniform int u_camera_width;
uniform int u_camera_height;
uniform vec2 u_camera_top_left;
uniform float u_camera_world_to_screen;

float cross2d(vec2 a, vec2 b)
{
    return a.x * b.y - b.x * a.y;
}

void main()
{   
    vec2 frag_coord = vec2(gl_FragCoord.x, u_camera_height - gl_FragCoord.y);

    float thickness = o_line_thickness / u_camera_world_to_screen;
    vec2 pixel = frag_coord / u_camera_world_to_screen + u_camera_top_left;

    vec2 line_begin = o_line_begin;
    vec2 line_end = o_line_end;

    if (line_begin == line_end && distance(pixel, line_begin) < thickness) {
        out_colour = o_line_begin_colour;
        return;
    }

    vec2 A = line_end - line_begin;
    vec2 B = pixel - line_begin;
    float lengthA = length(A);

    float distance_from_line = abs(cross2d(A, B)) / lengthA;

    float ratio_along = dot(A, B) / (lengthA * lengthA);

    if (distance_from_line <= thickness) {
        if (ratio_along > 0 && ratio_along < 1) {
            out_colour = mix(o_line_begin_colour, o_line_end_colour, ratio_along);
        } else if (ratio_along <= 0 && distance(line_begin, pixel) < thickness) {
            out_colour = o_line_begin_colour;
        } else if (ratio_along >= 1 && distance(line_end, pixel) < thickness) {
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

uniform int u_camera_width;
uniform int u_camera_height;
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
    vec2 frag_coord = vec2(gl_FragCoord.x, u_camera_height - gl_FragCoord.y);
    vec2 pixel = frag_coord / u_camera_world_to_screen + u_camera_top_left;

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

void quad_instance::set_buffer_attributes(std::uint32_t vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    for (int i = 1; i != 6; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(quad_instance), (void*)offsetof(quad_instance, centre));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(quad_instance), (void*)offsetof(quad_instance, width));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(quad_instance), (void*)offsetof(quad_instance, height));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(quad_instance), (void*)offsetof(quad_instance, angle));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(quad_instance), (void*)offsetof(quad_instance, colour));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

shape_renderer::shape_renderer()
    : d_line_shader(line_vertex, line_fragment)
    , d_circle_shader(circle_vertex, circle_fragment)
    , d_quad_shader(quad_vertex, quad_fragment)
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

shape_renderer::~shape_renderer()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

void shape_renderer::begin_frame(const camera& c)
{
    glBindVertexArray(d_vao);
    d_lines.clear();
    d_circles.clear();
    d_quads.clear();

    d_line_shader.bind();
    d_line_shader.load_int("u_camera_width", c.screen_width);
    d_line_shader.load_int("u_camera_height", c.screen_height);
    d_line_shader.load_vec2("u_camera_top_left", c.top_left);
    d_line_shader.load_float("u_camera_world_to_screen", c.world_to_screen);

    d_circle_shader.bind();
    d_circle_shader.load_int("u_camera_width", c.screen_width);
    d_circle_shader.load_int("u_camera_height", c.screen_height);
    d_circle_shader.load_vec2("u_camera_top_left", c.top_left);
    d_circle_shader.load_float("u_camera_world_to_screen", c.world_to_screen);

    const auto dimensions = glm::vec2{c.screen_width, c.screen_height} / c.world_to_screen;
    const auto projection = glm::ortho(0.0f, dimensions.x, dimensions.y, 0.0f);

    d_quad_shader.bind();
    d_quad_shader.load_mat4("u_proj_matrix", glm::translate(projection, glm::vec3{-c.top_left, 0.0f}));
}

void shape_renderer::end_frame()
{
    glBindVertexArray(d_vao);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    d_quad_shader.bind();
    d_instances.bind<quad_instance>(d_quads);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (int)d_quads.size());

    d_line_shader.bind();
    d_instances.bind<line_instance>(d_lines);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (int)d_lines.size());

    d_circle_shader.bind();
    d_instances.bind<circle_instance>(d_circles);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, (int)d_circles.size());

    glDisable(GL_BLEND);
}

void shape_renderer::draw_rect(glm::vec2 top_left, float width, float height, glm::vec4 colour)
{
    d_quads.emplace_back(top_left + glm::vec2{width/2, height/2}, width, height, 0.0f, colour);
}

void shape_renderer::draw_quad(glm::vec2 centre, float width, float height, float angle, glm::vec4 colour)
{
    d_quads.emplace_back(centre, width, height, angle, colour);
}

void shape_renderer::draw_line(glm::vec2 begin, glm::vec2 end, glm::vec4 begin_colour, glm::vec4 end_colour, float thickness)
{
    d_lines.emplace_back(begin, end, begin_colour, end_colour, thickness);
}

void shape_renderer::draw_line(glm::vec2 begin, glm::vec2 end, glm::vec4 colour, float thickness)
{
    d_lines.emplace_back(begin, end, colour, colour, thickness);
}

void shape_renderer::draw_circle(glm::vec2 centre, glm::vec4 colour, float radius)
{
    d_circles.emplace_back(centre, 0.0f, radius, colour, colour, 0.0f);
}

void shape_renderer::draw_annulus(glm::vec2 centre, glm::vec4 colour, float inner_radius, float outer_radius)
{
    d_circles.emplace_back(centre, inner_radius, outer_radius, colour, colour, 0.0f);
}
    
}