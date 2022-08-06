#pragma once
#include "pixel.h"
#include "serialise.hpp"

#include <cstdint>
#include <unordered_set>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace sand {

static constexpr std::uint32_t tile_size  = 256;
static constexpr std::uint32_t chunk_size = 16;
static_assert(tile_size % chunk_size == 0);

static constexpr std::uint32_t num_chunks = tile_size / chunk_size;

struct chunk
{
    bool should_step      = true;
    bool should_step_next = true;
};

class tile
{
public:
    using buffer = std::array<glm::vec4, tile_size * tile_size>;
    using pixels = std::array<pixel, tile_size * tile_size>;
    using chunks = std::array<chunk, num_chunks * num_chunks>;

private:
    // TODO: Remove this from the class, this class should have
    // nothing to do with rendering
    buffer d_buffer;

    pixels d_pixels;
    chunks d_chunks;

public:
    tile();

    // Returns true if the given position exists and false otherwise
    static auto valid(glm::ivec2 pos) -> bool;
    
    // show_chunks is a bad arg here, because simulate should have nothing
    // to do with how the world is rendered, the rendering will be moved
    // out of this class eventually
    auto simulate() -> void;

    auto set(glm::ivec2 pos, const pixel& p) -> void;
    auto fill(const pixel& p) -> void;

    auto at(glm::ivec2 pos) const -> const pixel&;
    auto at(glm::ivec2 pos) -> pixel&;

    // Returns the rhs
    auto swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2;

    // Chunk API
    auto wake_chunk_with_pixel(glm::ivec2 pixel) -> void;
    auto wake_all_chunks() -> void;
    auto num_awake_chunks() const -> std::size_t;
    auto is_chunk_awake(glm::ivec2 pixel) const -> bool;

    auto data() const -> const buffer& { return d_buffer; }

    auto serialise(auto& archive) -> void
    {
        archive(d_buffer, d_pixels);
    }
};

}