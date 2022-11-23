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
layout (location = 0) in vec4 position_uv;

uniform mat4 u_proj_matrix;

out vec2 pass_uv;

void main()
{
    vec2 position = position_uv.xy;
    pass_uv = position_uv.zw;
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
    const float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };

    const std::uint32_t indices[] = {0, 1, 2, 0, 2, 3};

    glGenVertexArrays(1, &d_vao);
    glBindVertexArray(d_vao);

    glGenBuffers(1, &d_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &d_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    d_shader.bind();
    d_shader.load_sampler("u_texture", 0);
    d_shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, 1.0f, 1.0f, 0.0f));
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
        sand::from_hex(0xe55039),
        sand::from_hex(0xf6b93b),
        sand::from_hex(0xfad390)
    };

    static const auto electricity_colours = std::array{
        sand::from_hex(0xf6e58d),
        sand::from_hex(0xf9ca24)
    };

    const auto camera_width = camera.zoom * (camera.screen_width / camera.screen_height);
    const auto camera_height = camera.zoom;

    if (d_texture.width() != camera_width || d_texture.height() != camera_height) {
        resize(camera_width, camera_height);
    }

    for (std::size_t x = 0; x != d_texture.width(); ++x) {
        for (std::size_t y = 0; y != d_texture.height(); ++y) {
            const auto screen_coord = glm::ivec2{x, y};
            const auto world_coord = glm::ivec2{camera.top_left} + screen_coord;
            const auto pos = x + d_texture.width() * y;
            if (!world.valid(world_coord)) {
                d_texture_data[pos] = glm::vec4{1.0, 1.0, 1.0, 1.0};
                continue;
            }
            const auto& pixel = world.at(world_coord);
            const auto& props = properties(pixel);

            if (pixel.flags[is_burning]) {
                d_texture_data[pos] = light_noise(sand::random_element(fire_colours));
            }
            else if (props.power_type == pixel_power_type::source) {
                d_texture_data[pos] = ((float)pixel.power / 4) * pixel.colour;
            }
            else if (props.power_type == pixel_power_type::conductor) {
                const auto a = pixel.colour;
                const auto b = sand::random_element(electricity_colours);
                const auto t = (float)pixel.power / props.power_max;
                d_texture_data[pos] = sand::lerp(a, b, t);
            }
            else {
                d_texture_data[pos] = pixel.colour;
            }

            if (show_chunks && world.is_chunk_awake(world_coord)) {
                d_texture_data[pos] += glm::vec4{0.05, 0.05, 0.05, 0};
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