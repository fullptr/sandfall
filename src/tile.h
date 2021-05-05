#ifndef INCLUDED_ALCHIMIA_TEXTURE
#define INCLUDED_ALCHIMIA_TEXTURE
#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace alc {

class tile
{
public:
    static constexpr std::uint32_t SIZE = 32;

    using buffer = std::array<glm::vec4, SIZE * SIZE>;

private:
    std::uint32_t d_texture;
    buffer        d_buffer;

public:
    tile();

    void bind() const;

    void update_texture();

    glm::vec4& at(std::uint32_t x, std::uint32_t y);
    void fill(const glm::vec4& value);
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW