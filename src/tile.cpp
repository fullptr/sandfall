#include "tile.h"

#include <glad/glad.h>

#include <cassert>

namespace alc {

tile::tile()
{
    glGenTextures(1, &d_texture); 
    bind();

    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);
}

void tile::bind() const
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

void tile::update_texture()
{
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, d_buffer.data());
}

glm::vec4& tile::at(std::uint32_t x, std::uint32_t y)
{
    assert(0 <= x);
    assert(x < SIZE);
    assert(0 <= y);
    assert(y < SIZE);
    return d_buffer[x + alc::tile::SIZE * y];
}

void tile::fill(const glm::vec4& value)
{
    d_buffer.fill(value);
}

}