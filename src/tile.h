#ifndef INCLUDED_ALCHIMIA_TEXTURE
#define INCLUDED_ALCHIMIA_TEXTURE
#include "pixel.h"

#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace alc {

class tile
{
public:
    static constexpr std::uint32_t SIZE = 128;

    using buffer = std::array<glm::vec4, SIZE * SIZE>;
    using pixels = std::array<pixel, SIZE * SIZE>;

private:
    std::uint32_t d_texture;
    buffer        d_buffer;
    pixels        d_pixels;


    void update_sand(double dt, glm::ivec2 pos);
    void update_rock(double dt, glm::ivec2 pos);
    void update_water(double dt, glm::ivec2 pos);

public:
    tile();

    void bind() const;

    // Returns true if the given position exists and false otherwise
    bool valid(glm::ivec2 pos) const;
    
    void simulate(double dt);
    void update_texture();

    void set(glm::ivec2 pos, const pixel& p);
    void fill(const pixel& p);

    const pixel& at(glm::ivec2 pos) const;
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW