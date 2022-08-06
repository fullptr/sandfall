#include "renderer.hpp"
#include "utility.hpp"

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

renderer::renderer()
    : d_texture{sand::tile_size, sand::tile_size}
    , d_texture_data{std::make_unique<texture_data>()}
{}

auto renderer::update(const tile& tile, bool show_chunks) -> void
{
    static const auto fire_colours = std::array{
        sand::from_hex(0xe55039),
        sand::from_hex(0xf6b93b),
        sand::from_hex(0xfad390)
    };

    for (std::size_t x = 0; x != sand::tile_size; ++x) {
        for (std::size_t y = 0; y != sand::tile_size; ++y) {
            const auto pos = get_pos({x, y});
            if (tile.at({x, y}).is_burning) {
                (*d_texture_data)[pos] = light_noise(sand::random_element(fire_colours));
            } else {
                (*d_texture_data)[pos] = tile.at({x, y}).colour;
            }

            if (show_chunks && tile.is_chunk_awake({x, y})) {
                (*d_texture_data)[pos] += glm::vec4{0.05, 0.05, 0.05, 0};
            }
        }
    }

    d_texture.set_data(*d_texture_data);
}

auto renderer::draw() const -> void
{
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

}