#include "renderer.hpp"
#include "utility.hpp"
#include "pixel.hpp"
#include "camera.hpp"
#include "world.hpp"

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
uniform float u_world_to_screen;

uniform sampler2D u_texture;

out vec2 pass_uv;

void main()
{
    vec2 tex_size = vec2(textureSize(u_texture, 0));
    vec2 position = (p_position * tex_size - u_tex_offset)
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

constexpr auto fire_colours = std::array{
    from_hex(0xe55039), from_hex(0xf6b93b), from_hex(0xfad390)
};

constexpr auto electricity_colours = std::array{
    from_hex(0xf6e58d), from_hex(0xf9ca24)
};

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

renderer::renderer(i32 width, i32 height)
    : d_vao{0}
    , d_vbo{0}
    , d_ebo{0}
    , d_texture{width, height}
    , d_shader{vertex_shader, fragment_shader}
{
    const f32 vertices[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    const u32 indices[] = {0, 1, 2, 0, 2, 3};

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
    d_shader.load_sampler("u_texture", 0);

    resize(width, height);
}

renderer::~renderer()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

auto renderer::update(const level& world) -> void
{
    if (d_texture.width() != world.pixels.width_in_pixels() || d_texture.height() != world.pixels.height_in_pixels()) {
        resize(world.pixels.width_in_pixels(), world.pixels.height_in_pixels());
    }

    auto buffer = std::array<glm::vec4, config::chunk_size * config::chunk_size> {};

    for (i32 cx = 0; cx != world.pixels.width_in_chunks(); ++cx) {
        for (i32 cy = 0; cy != world.pixels.height_in_chunks(); ++cy) {
            const auto cpos = chunk_pos{cx, cy};
            const auto chunk = world.pixels[cpos];
            if (!chunk.should_step) continue;
            
            const auto top_left = get_chunk_top_left(cpos);
            for (i32 x = 0; x != config::chunk_size; ++x) {
                for (i32 y = 0; y != config::chunk_size; ++y) {
                    const auto world_coord = top_left + glm::ivec2{x, y};
                    const auto& pixel = world.pixels[world_coord];
                    const auto& props = properties(pixel);
                    
                    auto& colour = buffer[x + config::chunk_size * y];
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
                    else if (pixel.type == pixel_type::none) {
                        colour = {0.0, 0.0, 0.0, 0.0};
                    }
                    else {
                        colour = pixel.colour;
                    }
                }
            }
            d_texture.set_subdata(buffer, glm::ivec2{top_left}, config::chunk_size, config::chunk_size);
        }
    }    
}

auto renderer::draw(const camera& camera) const -> void
{
    glBindVertexArray(d_vao);
    d_shader.bind();
    d_shader.load_vec2("u_tex_offset", camera.top_left);
    d_shader.load_float("u_world_to_screen", camera.world_to_screen);
    
    const auto projection = glm::ortho(
        0.0f, static_cast<float>(camera.screen_width), static_cast<float>(camera.screen_height), 0.0f
    );
    d_shader.load_mat4("u_proj_matrix", projection);
    
    d_texture.bind();
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_BLEND);
}

auto renderer::resize(u32 width, u32 height) -> void
{
    d_texture.resize(width, height);
}

}