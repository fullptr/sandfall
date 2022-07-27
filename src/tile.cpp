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

auto get_chunk_pos(glm::vec2 chunk) -> std::size_t
{
    return chunk.x + num_chunks * chunk.y;
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
}

auto tile::simulate() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step = chunk.should_step_next;
        chunk.should_step_next = false;
    }
    
    for (std::uint32_t y = num_chunks; y != 0; ) {
        --y;
        if (coin_flip()) {
            for (std::uint32_t x = 0; x != num_chunks; ++x) {
                if (d_chunks[get_chunk_pos(glm::ivec2{x, y})].should_step) {
                    simulate_chunk({x, y});
                }
            }
        }
        else {
            for (std::uint32_t x = num_chunks; x != 0; ) {
                --x;
                if (d_chunks[get_chunk_pos(glm::ivec2{x, y})].should_step) {
                    simulate_chunk({x, y});
                }
            }
        }
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
    d_chunks[get_chunk_pos(chunk)].should_step_next = true;

    // Wake right
    if (pixel.x != tile_size - 1 && pixel.x + 1 % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake left
    if (pixel.x != 0 && pixel.x - 1 % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake down
    if (pixel.y != tile_size - 1 && pixel.y + 1 % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake up
    if (pixel.y != 0 && pixel.y - 1 % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }
}

auto tile::wake_all_chunks() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step_next = true;
    }
}

auto tile::num_awake_chunks() const -> std::size_t
{
    auto count = std::size_t{0};
    for (const auto& chunk : d_chunks) {
        count += static_cast<std::size_t>(chunk.should_step);
    }
    return count;
}

}