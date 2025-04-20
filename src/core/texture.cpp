#include "texture.hpp"

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <print>

namespace sand {

texture::texture(std::uint32_t width, std::uint32_t height)
    : d_texture{0}
{
    resize(width, height);
}

texture::texture()
{}

texture::texture(const unsigned char* data, i32 width, i32 height)
    : d_width(width)
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_FLOAT, data.data());
}

auto texture::set_data(std::span<const unsigned char> data, i32 width, i32 height) -> void
{
    assert(width == d_width);
    assert(height == d_height);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, data.data());
}

auto texture::set_data(const unsigned char* data, i32 width, i32 height) -> void
{
    if (d_width != width || d_height != height) {
        resize(width, height);
    }
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, data);
}

auto texture::set_subdata(std::span<const unsigned char> data, glm::ivec2 top_left, i32 width, i32 height) -> void
{
    bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, top_left.x, top_left.y, width, height, GL_RGBA, GL_FLOAT, data.data());
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
    glBindTexture(GL_TEXTURE_2D, d_texture);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_FLOAT, nullptr);
}

texture_png::texture_png(const char* filename)
{
    i32 channels = 0;
    unsigned char *data = stbi_load("pixel_font.png", &d_width, &d_height, &channels, 0);
    if (!data) {
        std::print("Failed to load image\n");
        std::exit(1);
    }
    
    glGenTextures(1, &d_texture);
    glBindTexture(GL_TEXTURE_2D, d_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d_width, d_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

texture_png::~texture_png()
{
    glDeleteTextures(1, &d_texture);
}

auto texture_png::bind() const -> void
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

}