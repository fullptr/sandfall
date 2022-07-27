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

static constexpr std::uint32_t tile_size   = 256;
static constexpr float         tile_size_f = static_cast<float>(tile_size);

static constexpr std::uint32_t chunk_size   = 16;
static constexpr float         chunk_size_f = static_cast<float>(chunk_size);

class tile
{
public:
    using buffer = std::array<glm::vec4, tile_size * tile_size>;
    using pixels = std::array<pixel, tile_size * tile_size>;

private:
    buffer        d_buffer;
    pixels        d_pixels;

    // Simulate Data Structures (stored here so we dont need to allocate each time)
    std::unordered_set<glm::ivec2> d_to_update;
    std::unordered_set<glm::ivec2> d_updated;
    auto simulate_chunk(glm::ivec2 chunk) -> void;

public:
    tile();

    // Returns true if the given position exists and false otherwise
    static auto valid(glm::ivec2 pos) -> bool;
    
    auto simulate() -> void;

    auto set(glm::ivec2 pos, const pixel& p) -> void;
    auto fill(const pixel& p) -> void;

    auto at(glm::ivec2 pos) const -> const pixel&;
    auto at(glm::ivec2 pos) -> pixel&;

    // Returns the rhs
    auto swap(glm::ivec2 lhs, glm::ivec2 rhs) -> glm::ivec2;

    // Coord of a pixel, not a chunk. If on a boundary, will wake adjacent chunk
    auto wake_chunk_with_pixel(glm::ivec2 pixel) -> void;

    auto data() const -> const buffer& { return d_buffer; }

    auto serialise(auto& archive) -> void {
        archive(d_buffer, d_pixels);
    }
};

}