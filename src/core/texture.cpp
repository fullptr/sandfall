#include "texture.hpp"

#include <glad/glad.h>

#include <cassert>

namespace sand {

texture::texture(std::uint32_t width, std::uint32_t height, texture_type type)
    : d_type{type}
    , d_texture{0}
{
    resize(width, height);
}

texture::texture(texture_type type)
    : d_type{type}
{}

texture::~texture()
{
    glDeleteTextures(1, &d_texture);
}

auto texture::set_data(std::span<const glm::vec4> data) -> void
{
    assert(data.size() == d_width * d_height);
    bind();
    switch (d_type) {
        case texture_type::rgba: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_FLOAT, data.data());
        } break;
        case texture_type::red: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d_width, d_height, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
        } break;
    }
}

auto texture::set_data(std::span<const unsigned char> data, u64 width, u64 height) -> void
{
    assert(width == d_width);
    assert(height == d_height);
    bind();
    switch (d_type) {
        case texture_type::rgba: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, data.data());
        } break;
        case texture_type::red: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
        } break;
    }
}

auto texture::set_subdata(std::span<const unsigned char> data, glm::ivec2 top_left, u64 width, u64 height) -> void
{
    switch (d_type) {
        case texture_type::rgba: {
            glTexSubImage2D(GL_TEXTURE_2D, 0, top_left.x, top_left.y, width, height, GL_RGBA, GL_FLOAT, data.data());
        } break;
        case texture_type::red: {
            glTexSubImage2D(GL_TEXTURE_2D, 0, top_left.x, top_left.y, width, height, GL_RED, GL_UNSIGNED_BYTE, data.data());
        } break;
    }
}

auto texture::bind() const -> void
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

auto texture::resize(std::uint32_t width, std::uint32_t height) -> void
{
    if (d_texture) {
        glDeleteTextures(1, &d_texture);
    }

    d_width = width;
    d_height = height;

    glGenTextures(1, &d_texture);
    bind();
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    switch (d_type) {
        case texture_type::rgba: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_FLOAT, nullptr);
        } break;
        case texture_type::red: {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d_width, d_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        } break;
    }
}

}