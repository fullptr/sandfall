#ifndef INCLUDED_ALCHIMIA_TEXTURE
#define INCLUDED_ALCHIMIA_TEXTURE
#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace alc {

enum class pixel_type
{
    air,
    sand
};

struct pixel
{
    pixel_type type;
    bool updated_this_frame = false;
};

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
    bool valid(std::uint32_t x, std::uint32_t y) const;
    bool valid(std::size_t pos) const;

    void update_sand(std::uint32_t x, std::uint32_t y);

public:
    tile();

    void bind() const;

    void simulate();
    void update_if_needed();

    void set(std::uint32_t x, std::uint32_t y, const glm::vec4& value);
    void fill(const glm::vec4& value);
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW