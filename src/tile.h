#ifndef INCLUDED_ALCHIMIA_TEXTURE
#define INCLUDED_ALCHIMIA_TEXTURE
#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace alc {

class tile
{
public:
    static constexpr std::uint32_t SIZE = 128;

    using buffer = std::array<glm::vec4, SIZE * SIZE>;

private:
    std::uint32_t d_texture;
    buffer        d_buffer;

public:
    tile();

    void bind() const;

    buffer& get_buffer() { return d_buffer; }
    void update_texture();
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW