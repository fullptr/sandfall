#include "texture.hpp"

#include <glad/glad.h>

#include <cassert>

namespace sand {

texture::texture(std::uint32_t width, std::uint32_t height)
    : d_width(width)
    , d_height(height)
{
    glGenTextures(1, &d_texture); 
    bind();

    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
}

auto texture::set_data(std::span<const glm::vec4> data) -> void
{
    assert(data.size() == d_width * d_height);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_FLOAT, data.data());
}

auto texture::bind() const -> void
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

}