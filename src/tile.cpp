#include "tile.h"
#include "pixel.h"
#include "update_functions.h"
#include "utility.hpp"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>
#include <ranges>

namespace sand {
namespace {

static const auto default_pixel = pixel::air();

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + tile_size * pos.y;
}

auto get_chunk_pos(glm::vec2 chunk) -> std::size_t
{
    return chunk.x + num_chunks * chunk.y;
}

}

tile::tile()
{
    d_pixels.fill(pixel::air());
}

auto tile::valid(glm::ivec2 pos) const -> bool
{
    return 0 <= pos.x && pos.x < tile_size && 0 <= pos.y && pos.y < tile_size;
}

auto tile::simulate() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step = chunk.should_step_next;
        chunk.should_step_next = false;
    }
    
    const auto inner = [&] (glm::ivec2 pos) {
        if (is_chunk_awake(pos) && !at(pos).is_updated) {
            update_pixel(*this, pos);
        }
    };

    for (std::uint32_t y = tile_size; y != 0; --y) {
        if (coin_flip()) {
            for (std::uint32_t x = 0; x != tile_size; ++x) {
                inner({x, y - 1});
            }
        }
        else {
            for (std::uint32_t x = tile_size; x != 0; --x) {
                inner({x - 1, y - 1});
            }
        }
    }

    std::ranges::for_each(d_pixels, [](auto& p) { p.is_updated = false; });
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
    if (!valid(pos)) {
        return default_pixel;
    }
    return d_pixels[get_pos(pos)];
}

auto tile::at(glm::ivec2 pos) -> pixel&
{
    assert(valid(pos));
    return d_pixels[get_pos(pos)];
}

auto tile::swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2
{
    wake_chunk_with_pixel(lhs);
    wake_chunk_with_pixel(rhs);
    std::swap(at(lhs), at(rhs));
    return rhs;
}

auto tile::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / static_cast<int>(chunk_size);
    d_chunks[get_chunk_pos(chunk)].should_step_next = true;

    // Wake right
    if (pixel.x != tile_size - 1 && (pixel.x + 1) % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake left
    if (pixel.x != 0 && (pixel.x - 1) % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake down
    if (pixel.y != tile_size - 1 && (pixel.y + 1) % chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        if (valid(neighbour))
            d_chunks[get_chunk_pos(neighbour)].should_step_next = true;
    }

    // Wake up
    if (pixel.y != 0 && (pixel.y - 1) % chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        if (valid(neighbour))
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

auto tile::is_chunk_awake(glm::ivec2 pixel) const -> bool
{
    const auto chunk = pixel / static_cast<int>(chunk_size);
    return d_chunks[get_chunk_pos(chunk)].should_step;
}

}