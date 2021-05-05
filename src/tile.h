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

    // Will be set to true if the buffer is changed, indicating that the buffer on the
    // GPU is stale and needs updating.
    bool d_stale = false;

    // Returns true if the given position exists and false otherwise
    bool valid(glm::ivec2 pos) const;

    void update_sand(glm::ivec2 pos);
    void update_rock(glm::ivec2 pos);

public:
    tile();

    void bind() const;

    void simulate();
    void update_if_needed();

    void set(glm::ivec2 pos, const pixel& p);
    void fill(const pixel& p);
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW