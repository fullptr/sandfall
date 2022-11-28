#include "renderer.hpp"
#include "utility.hpp"
#include "pixel.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace sand {
namespace {

constexpr auto vertex_shader = R"SHADER(
#version 410 core
layout (location = 0) in vec2 p_position;

uniform mat4  u_proj_matrix;
uniform vec2  u_tex_offset;
uniform vec2  u_tex_dimensions;
uniform float u_world_to_screen;

out vec2 pass_uv;

void main()
{
    vec2 position = (p_position * u_tex_dimensions - u_tex_offset)
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
    out_colour = texture(u_texture, pass_uv);
}
)SHADER";

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + sand::num_pixels * pos.y;
}

auto light_noise(glm::vec4 vec) -> glm::vec4
{
    return {
        std::clamp(vec.x + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.y + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.z + sand::random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        1.0f
    };
}

}

renderer::renderer()
    : d_vao{0}
    , d_vbo{0}
    , d_ebo{0}
    , d_texture{}
    , d_texture_data{}
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
    d_shader.load_sampler("u_texture", 0);

    resize(num_pixels, num_pixels);
}

renderer::~renderer()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

auto renderer::update(const world& world, bool show_chunks, const camera& camera) -> void
{
    static const auto fire_colours = std::array{
        from_hex(0xe55039), from_hex(0xf6b93b), from_hex(0xfad390)
    };

    static const auto electricity_colours = std::array{
        from_hex(0xf6e58d), from_hex(0xf9ca24)
    };

    const auto tex_top_left = glm::ivec2{0, 0};
    const auto tex_offset = camera.top_left;
    const auto tex_width = num_pixels;
    const auto tex_height = num_pixels;

    d_shader.load_float("u_world_to_screen", camera.world_to_screen);
    d_shader.load_vec2("u_tex_offset", tex_offset);
    d_shader.load_vec2("u_tex_dimensions", glm::vec2{tex_width, tex_height});

    const auto projection = glm::ortho(0.0f, camera.screen_width, camera.screen_height, 0.0f);
    d_shader.load_mat4("u_proj_matrix", projection);

    for (std::size_t x = 0; x != d_texture.width(); ++x) {
        for (std::size_t y = 0; y != d_texture.height(); ++y) {
            const auto world_coord = tex_top_left + glm::ivec2{x, y};
            if (!world.is_chunk_awake(world_coord)) continue;
            auto& colour = d_texture_data[x + d_texture.width() * y];

            if (!world.valid(world_coord)) {
                colour = glm::vec4{1.0, 1.0, 1.0, 1.0};
                continue;
            }
            const auto& pixel = world.at(world_coord);
            const auto& props = properties(pixel);

            if (pixel.flags[is_burning]) {
                colour = sand::random_element(fire_colours);
            }
            else if (props.power_type == pixel_power_type::source) {
                const auto a = from_hex(0x000000); // black
                const auto b = pixel.colour;
                const auto t = static_cast<float>(pixel.power) / props.power_max;
                colour = sand::lerp(a, b, t);
            }
            else if (props.power_type == pixel_power_type::conductor) {
                const auto a = pixel.colour;
                const auto b = sand::random_element(electricity_colours);
                const auto t = static_cast<float>(pixel.power) / props.power_max;
                colour = sand::lerp(a, b, t);
            }
            else {
                colour = pixel.colour;
            }

            if (show_chunks && world.is_chunk_awake(world_coord)) {
                colour += glm::vec4{0.05, 0.05, 0.05, 0};
            }
        }
    }

    d_texture.set_data(d_texture_data);
}

auto renderer::draw() const -> void
{
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

auto renderer::resize(std::uint32_t width, std::uint32_t height) -> void
{
    d_texture.resize(width, height);
    d_texture_data.resize(width * height);
}

}