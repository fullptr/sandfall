#pragma once
#include "pixel.h"
#include "world_settings.h"

#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace sand {

static constexpr std::uint32_t tile_size = 256;

class tile
{
public:

    using buffer = std::array<glm::vec4, tile_size * tile_size>;
    using pixels = std::array<pixel, tile_size * tile_size>;

private:
    std::uint32_t d_texture;
    buffer        d_buffer;
    pixels        d_pixels;

public:
    tile();

    // Returns true if the given position exists and false otherwise
    static bool valid(glm::ivec2 pos);
    
    void simulate(const world_settings& settings, double dt);

    void set(glm::ivec2 pos, const pixel& p);
    void fill(const pixel& p);

    const pixel& at(glm::ivec2 pos) const;

    const buffer& data() const { return d_buffer; }
};

}