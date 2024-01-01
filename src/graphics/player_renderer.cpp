#include "player_renderer.hpp"
#include "utility.hpp"
#include "pixel.hpp"
#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace sand {
namespace {

constexpr auto vertex_shader = R"SHADER(
#version 410 core
layout (location = 0) in vec2 p_position;

uniform mat4  u_proj_matrix;
uniform vec2  u_world_offset;
uniform float u_world_to_screen;
uniform float u_width;
uniform float u_height;

uniform sampler2D u_texture;
uniform vec2 u_position;

out vec2 pass_uv;

void main()
{
    vec2 dimensions = vec2(u_width, u_height);
    vec2 position = (p_position * dimensions + u_position - u_world_offset)
                  * u_world_to_screen;

    pass_uv = p_position;
    gl_Position = u_proj_matrix * vec4(position, 0, 1);
}
)SHADER";

constexpr auto fragment_shader = R"SHADER(
#version 410 core
layout (location = 0) out vec4 out_colour;

in vec2 pass_uv;

uniform sampler2D u_texture;

void main()
{
    out_colour = vec4(1.0, 0.0, 0.0, 1.0);
}
)SHADER";

}

player_renderer::player_renderer()
    : d_vao{0}
    , d_vbo{0}
    , d_ebo{0}
    , d_shader{std::string{vertex_shader}, std::string{fragment_shader}}
{
    const float vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
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
}

player_renderer::~player_renderer()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

auto player_renderer::bind() const -> void
{
    glBindVertexArray(d_vao);
    d_shader.bind();
}

auto player_renderer::update(const world& world, glm::vec2 pos, float width, float height, const camera& camera) -> void
{
    d_shader.load_vec2("u_position", pos);
    d_shader.load_float("u_width", width);
    d_shader.load_float("u_height", height);
    
    d_shader.load_vec2("u_world_offset", camera.top_left);
    d_shader.load_float("u_world_to_screen", camera.world_to_screen);

    const auto projection = glm::ortho(0.0f, camera.screen_width, camera.screen_height, 0.0f);
    d_shader.load_mat4("u_proj_matrix", projection);
}

auto player_renderer::draw() const -> void
{
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

}