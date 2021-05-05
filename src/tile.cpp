#include "tile.h"
#include "log.h"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>

namespace alc {
namespace {

std::size_t pos(std::uint32_t x, std::uint32_t y)
{
    return x + alc::tile::SIZE * y;
}

}

tile::tile()
{
    glGenTextures(1, &d_texture); 
    bind();

    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(d_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(d_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, nullptr);

    pixel default_pixel{ pixel_type::air };
    d_pixels.fill(default_pixel);
}

bool tile::valid(std::uint32_t x, std::uint32_t y) const
{
    return 0 <= x && x < SIZE && 0 <= y && y < SIZE;
}

void tile::update_sand(std::uint32_t x, std::uint32_t y)
{
    if (valid(x, y + 1) && d_pixels[pos(x, y + 1)].type == pixel_type::air) {
        d_pixels[pos(x, y + 1)].type = pixel_type::sand;
        d_pixels[pos(x, y + 1)].updated_this_frame = true;
        d_pixels[pos(x, y)].type = pixel_type::air;
        d_pixels[pos(x, y)].updated_this_frame = false;
    }
    else if (valid(x - 1, y + 1) && d_pixels[pos(x - 1, y + 1)].type == pixel_type::air) {
        d_pixels[pos(x - 1, y + 1)].type = pixel_type::sand;
        d_pixels[pos(x - 1, y + 1)].updated_this_frame = true;
        d_pixels[pos(x, y)].type = pixel_type::air;
        d_pixels[pos(x, y)].updated_this_frame = false;
    }
    else if (valid(x + 1, y + 1) && d_pixels[pos(x + 1, y + 1)].type == pixel_type::air) {
        d_pixels[pos(x + 1, y + 1)].type = pixel_type::sand;
        d_pixels[pos(x + 1, y + 1)].updated_this_frame = true;
        d_pixels[pos(x, y)].type = pixel_type::air;
        d_pixels[pos(x, y)].updated_this_frame = false;
    }
}

void tile::bind() const
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

void tile::simulate()
{
    for (std::uint32_t y = 0; y != SIZE; ++y) {
        for (std::uint32_t x = 0; x != SIZE; ++ x) {
            auto& pixel = d_pixels[pos(x, y)];
            if (pixel.updated_this_frame) { continue; }
            switch (pixel.type) {
                case pixel_type::sand: {
                    update_sand(x, y);
                } break;
                case pixel_type::air: continue;
            }
            d_stale = true;
        }
    }
    std::for_each(d_pixels.begin(), d_pixels.end(), [](auto& p) { p.updated_this_frame = false; });
    for (std::size_t pos = 0; pos != SIZE * SIZE; ++pos) {
        switch (d_pixels[pos].type) {
            case pixel_type::sand: {
                d_buffer[pos] = {1.0, 1.0, 1.0, 1.0};
            } break;
            case pixel_type::air: {
                d_buffer[pos] = { 44.0f / 256.0f, 58.0f / 256.0f, 71.0f / 256.0f, 1.0 };
            } break;
        }
    }
    update_if_needed();
}

void tile::update_if_needed()
{
    if (d_stale) {
        bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, d_buffer.data());
        d_stale = false;
    }
}

void tile::set(std::uint32_t x, std::uint32_t y, const glm::vec4& value)
{
    assert(valid(x, y));
    d_buffer[pos(x, y)] = value;
    d_pixels[pos(x, y)] = { pixel_type::sand };
    d_stale = true;
}

void tile::fill(const glm::vec4& value)
{
    d_buffer.fill(value);
    d_stale = true;
}

}