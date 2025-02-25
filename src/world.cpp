#include "world.hpp"
#include "pixel.hpp"
#include "update.hpp"
#include "utility.hpp"

#include <cassert>
#include <algorithm>
#include <ranges>

namespace sand {
namespace {

static const auto default_pixel = pixel::air();

}

auto pixel_world::valid(glm::ivec2 pos) const -> bool
{
    return 0 <= pos.x && pos.x < d_width && 0 <= pos.y && pos.y < d_height;
}

auto pixel_world::operator[](glm::ivec2 pos) -> pixel&
{
    assert(valid(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto pixel_world::operator[](glm::ivec2 pos) const -> const pixel&
{
    assert(valid(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto get_chunk_index(const level& w, glm::ivec2 chunk) -> std::size_t
{
    const auto width_chunks = w.pixels.width() / config::chunk_size;
    return width_chunks * chunk.y + chunk.x;
}

auto get_chunk_pos(const level& w, std::size_t index) -> glm::ivec2
{
    const auto width_chunks = w.pixels.width() / config::chunk_size;
    return {index % width_chunks, index / width_chunks};
}

level::level(std::size_t width, std::size_t height)
    : physics{{sand::config::gravity.x, sand::config::gravity.y}}
    , pixels{width, height}
    , spawn_point{width / 2, height / 2}
    , player{physics, 5}
{
    assert(width % config::chunk_size == 0);
    assert(height % config::chunk_size == 0);
    const auto width_chunks = width / config::chunk_size;
    const auto height_chunks = height / config::chunk_size;
    chunks.resize(width_chunks * height_chunks);
}

auto level::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / sand::config::chunk_size;
    chunks[get_chunk_index(*this, chunk)].should_step_next = true;
    const auto width_pixels = static_cast<int>(pixels.width());

    // Wake right
    if (pixel.x != width_pixels - 1 && (pixel.x + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        if (pixels.valid(neighbour)) { chunks[get_chunk_index(*this, neighbour)].should_step_next = true; }
    }

    // Wake left
    if (pixel.x != 0 && (pixel.x - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        if (pixels.valid(neighbour)) { chunks[get_chunk_index(*this, neighbour)].should_step_next = true; }
    }

    // Wake down
    if (pixel.y != width_pixels - 1 && (pixel.y + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        if (pixels.valid(neighbour)) { chunks[get_chunk_index(*this, neighbour)].should_step_next = true; }
    }

    // Wake up
    if (pixel.y != 0 && (pixel.y - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        if (pixels.valid(neighbour)) { chunks[get_chunk_index(*this, neighbour)].should_step_next = true; }
    }
}

}