#pragma once
#include "pixel.h"
#include "serialise.hpp"

#include <cstdint>
#include <array>

#include <glm/glm.hpp>
#include <cereal/types/array.hpp>

namespace sand {

static constexpr std::uint32_t tile_size = 256;
static constexpr float         tile_size_f = static_cast<double>(tile_size);

class tile
{
public:
    using buffer = std::array<glm::vec4, tile_size * tile_size>;
    using pixels = std::array<pixel, tile_size * tile_size>;

private:
    buffer        d_buffer;
    pixels        d_pixels;

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

    auto data() const -> const buffer& { return d_buffer; }

    auto serialise(auto& archive) -> void {
        archive(d_buffer, d_pixels);
    }
};

}