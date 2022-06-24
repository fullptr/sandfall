#pragma once
#include "pixel.h"
#include "world_settings.h"

#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace sand {

class tile
{
public:
    static constexpr std::uint32_t SIZE = 256;

    using buffer = std::array<glm::vec4, SIZE * SIZE>;
    using pixels = std::array<pixel, SIZE * SIZE>;

private:
    std::uint32_t d_texture;
    buffer        d_buffer;
    pixels        d_pixels;

public:
    tile();

    void bind() const;

    // Returns true if the given position exists and false otherwise
    static bool valid(glm::ivec2 pos);
    
    void simulate(const world_settings& settings, double dt);
    void update_texture();

    void set(glm::ivec2 pos, const pixel& p);
    void fill(const pixel& p);

    const pixel& at(glm::ivec2 pos) const;
};

}