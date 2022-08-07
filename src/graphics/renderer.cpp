#include "renderer.hpp"
#include "utility.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glad/glad.h>

namespace sand {
namespace {

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + sand::tile_size * pos.y;
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

renderer::renderer(float screen_width, float screen_height)
    : d_vao{0}
    , d_vbo{0}
    , d_ebo{0}
    , d_texture{sand::tile_size, sand::tile_size}
    , d_texture_data{}
    , d_shader{"res\\vertex.glsl", "res\\fragment.glsl"}
{
    d_texture_data.resize(sand::tile_size * sand::tile_size);

    float vertices[] = {
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
    d_shader.load_mat4("u_proj_matrix", glm::ortho(0.0f, screen_width, screen_height, 0.0f));
}

renderer::~renderer()
{
    glDeleteBuffers(1, &d_ebo);
    glDeleteBuffers(1, &d_vbo);
    glDeleteVertexArrays(1, &d_vao);
}

auto renderer::update(const tile& tile, bool show_chunks) -> void
{
    d_shader.load_int("u_width", 1280);
    d_shader.load_int("u_height", 720);

    static const auto fire_colours = std::array{
        sand::from_hex(0xe55039),
        sand::from_hex(0xf6b93b),
        sand::from_hex(0xfad390)
    };

    for (std::size_t x = 0; x != d_texture.width(); ++x) {
        for (std::size_t y = 0; y != d_texture.height(); ++y) {
            const auto pos = x + d_texture.width() * y;
            if (!tile.valid({x, y})) {
                d_texture_data[pos] = glm::vec4{1.0, 1.0, 1.0, 1.0};
                continue;
            }
            if (tile.at({x, y}).is_burning) {
                d_texture_data[pos] = light_noise(sand::random_element(fire_colours));
            } else {
                d_texture_data[pos] = tile.at({x, y}).colour;
            }

            if (show_chunks && tile.is_chunk_awake({x, y})) {
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
}

}