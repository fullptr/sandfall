#include "texture.hpp"

#include <glad/glad.h>

#include <cassert>
#include <print>

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

texture::texture(const unsigned char* data, i32 width, i32 height)
    : d_type(texture_type::rgba)
    , d_width(width)
    , d_height(height)
{
    glGenTextures(1, &d_texture);
    glBindTexture(GL_TEXTURE_2D, d_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

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
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, d_width, d_height, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        } break;
    }
}

auto texture::set_data(std::span<const unsigned char> data, i32 width, i32 height) -> void
{
    assert(width == d_width);
    assert(height == d_height);
    bind();
    switch (d_type) {
        case texture_type::rgba: {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, data.data());
        } break;
        case texture_type::red: {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        } break;
    }
}

auto texture::set_data(const unsigned char* data, i32 width, i32 height) -> void
{
    if (d_width != width || d_height != height) {
        resize(width, height);
    }
    bind();
    std::print("about to set the image\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, data);
}

auto texture::set_subdata(std::span<const unsigned char> data, glm::ivec2 top_left, i32 width, i32 height) -> void
{
    bind();
    switch (d_type) {
        case texture_type::rgba: {
            glTexSubImage2D(GL_TEXTURE_2D, 0, top_left.x, top_left.y, width, height, GL_RGBA, GL_FLOAT, data.data());
        } break;
        case texture_type::red: {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, top_left.x, top_left.y, width, height, GL_RED, GL_UNSIGNED_BYTE, data.data());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        } break;
    }
}

auto texture::bind() const -> void
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

auto texture::resize(std::uint32_t width, std::uint32_t height) -> void
{
    std::print("resizing\n");
    if (d_texture) {
        glDeleteTextures(1, &d_texture);
    }

    d_width = width;
    d_height = height;

    glGenTextures(1, &d_texture);
    glBindTexture(GL_TEXTURE_2D, d_texture);
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