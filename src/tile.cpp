#include "tile.h"
#include "update_functions.h"
#include "utility.hpp"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>
#include <ranges>

namespace sand {
namespace {

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + tile_size * pos.y;
}

auto light_noise(glm::vec4 vec) -> glm::vec4
{
    return {
        std::clamp(vec.x + random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.y + random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        std::clamp(vec.z + random_from_range(-0.04f, 0.04f), 0.0f, 1.0f),
        1.0f
    };
}

}

tile::tile()
{
    const auto default_pixel = pixel::air();
    d_pixels.fill(default_pixel);
    d_buffer.fill(default_pixel.colour);
}

auto tile::valid(glm::ivec2 pos) -> bool
{
    return 0 <= pos.x && pos.x < tile_size && 0 <= pos.y && pos.y < tile_size;
}

auto tile::simulate_chunk(glm::ivec2 chunk) -> void
{
    if (d_updated.contains(chunk)) {
        return;
    }

    const auto inner = [&] (std::uint32_t x, std::uint32_t y) {
        if (!at({x, y}).is_updated) {
            update_pixel(*this, {x, y});
        }
    };

    for (std::uint32_t y = chunk_size * (chunk.y + 1); y != chunk_size * chunk.y; ) {
        --y;
        if (coin_flip()) {
            for (std::uint32_t x = chunk_size * chunk.x; x != chunk_size * (chunk.x + 1); ++x) {
                inner(x, y);
            }
        }
        else {
            for (std::uint32_t x = chunk_size * (chunk.x + 1); x != chunk_size * chunk.x; ) {
                --x;
                inner(x, y);
            }
        }
    }

    d_updated.emplace(chunk);
}

auto tile::simulate() -> void
{
    while (!d_to_update.empty()) {
        const auto next = *d_to_update.begin();
        d_to_update.erase(d_to_update.begin());
        simulate_chunk(next);
    }

    static const auto fire_colours = std::array{
        from_hex(0xe55039),
        from_hex(0xf6b93b),
        from_hex(0xfad390)
    };

    std::ranges::for_each(d_pixels, [](auto& p) { p.is_updated = false; });
    for (std::size_t pos = 0; pos != tile_size * tile_size; ++pos) {
        if (d_pixels[pos].is_burning) {
            d_buffer[pos] = light_noise(random_element(fire_colours));
        } else {
            d_buffer[pos] = d_pixels[pos].colour;
        }
    }

    d_updated.clear();
}

auto tile::set(glm::ivec2 pos, const pixel& pixel) -> void
{
    assert(valid(pos));
    wake_chunk_with_pixel(pos);
    d_pixels[get_pos(pos)] = pixel;
}

auto tile::fill(const pixel& p) -> void
{
    d_pixels.fill(p);
}

auto tile::at(glm::ivec2 pos) const -> const pixel&
{
    return d_pixels[get_pos(pos)];
}

auto tile::at(glm::ivec2 pos) -> pixel&
{
    return d_pixels[get_pos(pos)];
}

auto tile::swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2
{
    std::swap(at(lhs), at(rhs));
    return rhs;
}

auto tile::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / static_cast<int>(chunk_size);
    d_to_update.emplace(chunk);

    // Wake right
    if (pixel.x != tile_size - 1 && pixel.x + 1 % chunk_size == 0)
    {
        const auto right = chunk + glm::ivec2{1, 0};
        d_to_update.emplace(right);
    }

    // Wake left
    if (pixel.x != 0 && pixel.x - 1 % chunk_size == 0)
    {
        const auto left = chunk - glm::ivec2{1, 0};
        d_to_update.emplace(left);
    }

    // Wake down
    if (pixel.y != tile_size - 1 && pixel.y + 1 % chunk_size == 0)
    {
        const auto down = chunk + glm::ivec2{0, 1};
        d_to_update.emplace(down);
    }

    // Wake up
    if (pixel.y != 0 && pixel.y - 1 % chunk_size == 0)
    {
        const auto up = chunk - glm::ivec2{0, 1};
        d_to_update.emplace(up);
    }
}

}