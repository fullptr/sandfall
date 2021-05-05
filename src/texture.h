#ifndef INCLUDED_ALCHIMIA_TEXTURE
#define INCLUDED_ALCHIMIA_TEXTURE
#include <cstdint>
#include <array>

#include <glm/glm.hpp>

namespace alc {

class texture
{
    static constexpr std::uint32_t SIZE = 64;

    std::uint32_t d_texture;

public:
    texture(const std::array<glm::vec4, SIZE * SIZE>& data);

    void bind() const;

    void set_buffer(const std::array<glm::vec4, SIZE * SIZE>& data);
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW