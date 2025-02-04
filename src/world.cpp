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

auto get_pos(glm::vec2 pos) -> std::size_t
{
    return pos.x + sand::config::num_pixels * pos.y;
}

auto wake_chunk(chunk& c) -> void
{
    c.should_step_next = true;
}

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

auto get_chunk_index(glm::ivec2 chunk) -> std::size_t
{
    return num_chunks * chunk.y + chunk.x;
}

auto get_chunk_pos(std::size_t index) -> glm::ivec2
{
    return {index % num_chunks, index / num_chunks};
}

world::world()
    : d_physics{{sand::config::gravity.x, sand::config::gravity.y}}
    , d_pixels{sand::config::num_pixels, sand::config::num_pixels}
{
}

auto world::valid(glm::ivec2 pos) const -> bool
{
    return 0 <= pos.x && pos.x < sand::config::num_pixels && 0 <= pos.y && pos.y < sand::config::num_pixels;
}

auto world::set(glm::ivec2 pos, const pixel& pixel) -> void
{
    assert(d_pixels.valid(pos));
    wake_chunk_with_pixel(pos);
    d_pixels[pos] = pixel;
}

auto world::fill(const pixel& p) -> void
{
    std::fill(d_pixels.begin(), d_pixels.end(), p);
}

auto world::at(glm::ivec2 pos) const -> const pixel&
{
    return d_pixels[pos];
}

auto world::at(glm::ivec2 pos) -> pixel&
{
    return d_pixels[pos];
}

auto world::type(glm::ivec2 pos) const -> pixel_type
{
    return at(pos).type;
}

auto world::swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2
{
    wake_chunk_with_pixel(lhs);
    wake_chunk_with_pixel(rhs);
    std::swap(at(lhs), at(rhs));
    return rhs;
}

auto world::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / sand::config::chunk_size;
    wake_chunk(d_chunks[get_chunk_index(chunk)]);

    // Wake right
    if (pixel.x != sand::config::num_pixels - 1 && (pixel.x + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        if (valid(neighbour)) { wake_chunk(d_chunks[get_chunk_index(neighbour)]); }
    }

    // Wake left
    if (pixel.x != 0 && (pixel.x - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        if (valid(neighbour)) { wake_chunk(d_chunks[get_chunk_index(neighbour)]); }
    }

    // Wake down
    if (pixel.y != sand::config::num_pixels - 1 && (pixel.y + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        if (valid(neighbour)) { wake_chunk(d_chunks[get_chunk_index(neighbour)]); }
    }

    // Wake up
    if (pixel.y != 0 && (pixel.y - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        if (valid(neighbour)) { wake_chunk(d_chunks[get_chunk_index(neighbour)]); }
    }
}

auto world::wake_all_chunks() -> void
{
    for (auto& chunk : d_chunks) {
        wake_chunk(chunk);
    }
}

auto world::num_awake_chunks() const -> std::size_t
{  
    return std::count_if(d_chunks.begin(), d_chunks.end(), [](chunk c) {
        return c.should_step;
    });
}

auto world::new_frame() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step = std::exchange(chunk.should_step_next, false);
    }

    for (auto& pixel : d_pixels) {
        pixel.flags[is_updated] = false;
    }

    d_physics.Step(sand::config::time_step, 8, 3);
}

auto world::is_chunk_awake(glm::ivec2 pixel) const -> bool
{
    const auto chunk = pixel / sand::config::chunk_size;
    return d_chunks[get_chunk_index(chunk)].should_step;
}

}