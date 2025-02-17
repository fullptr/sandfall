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
    : physics{{sand::config::gravity.x, sand::config::gravity.y}}
    , pixels{sand::config::num_pixels, sand::config::num_pixels}
    , spawn_point{128, 128}
{
    chunks.resize(num_chunks * num_chunks);
}

auto world::set(glm::ivec2 pos, const pixel& pixel) -> void
{
    assert(pixels.valid(pos));
    wake_chunk_with_pixel(pos);
    pixels[pos] = pixel;
}

auto world::type(glm::ivec2 pos) const -> pixel_type
{
    return pixels[pos].type;
}

auto world::swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2
{
    wake_chunk_with_pixel(lhs);
    wake_chunk_with_pixel(rhs);
    std::swap(pixels[lhs], pixels[rhs]);
    return rhs;
}

auto world::wake_chunk_with_pixel(glm::ivec2 pixel) -> void
{
    const auto chunk = pixel / sand::config::chunk_size;
    wake_chunk(chunks[get_chunk_index(chunk)]);

    // Wake right
    if (pixel.x != sand::config::num_pixels - 1 && (pixel.x + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{1, 0};
        if (pixels.valid(neighbour)) { wake_chunk(chunks[get_chunk_index(neighbour)]); }
    }

    // Wake left
    if (pixel.x != 0 && (pixel.x - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{1, 0};
        if (pixels.valid(neighbour)) { wake_chunk(chunks[get_chunk_index(neighbour)]); }
    }

    // Wake down
    if (pixel.y != sand::config::num_pixels - 1 && (pixel.y + 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk + glm::ivec2{0, 1};
        if (pixels.valid(neighbour)) { wake_chunk(chunks[get_chunk_index(neighbour)]); }
    }

    // Wake up
    if (pixel.y != 0 && (pixel.y - 1) % sand::config::chunk_size == 0)
    {
        const auto neighbour = chunk - glm::ivec2{0, 1};
        if (pixels.valid(neighbour)) { wake_chunk(chunks[get_chunk_index(neighbour)]); }
    }
}

auto world::wake_all_chunks() -> void
{
    for (auto& chunk : chunks) {
        wake_chunk(chunk);
    }
}

auto world::num_awake_chunks() const -> std::size_t
{  
    return std::count_if(chunks.begin(), chunks.end(), [](chunk c) {
        return c.should_step;
    });
}

auto world::is_chunk_awake(glm::ivec2 pixel) const -> bool
{
    const auto chunk = pixel / sand::config::chunk_size;
    return chunks[get_chunk_index(chunk)].should_step;
}

}