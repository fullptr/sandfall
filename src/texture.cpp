#include "texture.h"

#include <glad/glad.h>

namespace alc {
namespace {

void set_buffer_data(std::uint32_t texture, std::uint32_t size, void* data)
{
    glTextureSubImage2D(texture, 0, 0, 0, size, size, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

}

texture::texture(const std::array<glm::vec4, SIZE * SIZE>& data)
{
    glGenTextures(1, &d_texture); 
    glBindTexture(GL_TEXTURE_2D, d_texture); 
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, data.data());
    //glGenerateMipmap(GL_TEXTURE_2D);
}

void texture::bind() const
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

void texture::set_buffer(const std::array<glm::vec4, SIZE * SIZE>& data)
{
    set_buffer_data(d_texture, SIZE, (void*)data.data());
}

}