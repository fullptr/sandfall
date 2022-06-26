#include "tile.h"
#include "log.h"
#include "update_functions.h"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>

namespace sand {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + tile_size * pos.y;
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tile_size, tile_size, 0, GL_RGBA, GL_FLOAT, nullptr);

    pixel default_pixel{ pixel_type::air };
    d_pixels.fill(default_pixel);
}

bool tile::valid(glm::ivec2 pos)
{
    return 0 <= pos.x && pos.x < tile_size && 0 <= pos.y && pos.y < tile_size;
}

void tile::bind() const
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

void tile::simulate(const world_settings& settings, double dt)
{
    const auto inner = [&] (std::uint32_t x, std::uint32_t y) {
        auto& pixel = d_pixels[get_pos({x, y})];
        if (pixel.updated_this_frame) { return; }
        switch (pixel.type) {
            case pixel_type::sand: {
                update_sand(d_pixels, {x, y}, settings, dt);
            } break;
            case pixel_type::rock: {
                update_rock(d_pixels, {x, y}, settings, dt);
            } break;
            case pixel_type::water: {
                update_water(d_pixels, {x, y}, settings, dt);
            }
            case pixel_type::air: return;
        }
    };


    for (std::uint32_t y = tile_size; y != 0; ) {
        --y;
        if (rand() % 2) {
            for (std::uint32_t x = 0; x != tile_size; ++x) {
                inner(x, y);
            }
        }
        else {
            for (std::uint32_t x = tile_size; x != 0; ) {
                --x;
                inner(x, y);
            }
        }
    }

    std::for_each(d_pixels.begin(), d_pixels.end(), [](auto& p) { p.updated_this_frame = false; });
    for (std::size_t pos = 0; pos != tile_size * tile_size; ++pos) {
        d_buffer[pos] = d_pixels[pos].colour;
    }
}

void tile::update_texture()
{
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tile_size, tile_size, 0, GL_RGBA, GL_FLOAT, d_buffer.data());
}

void tile::set(glm::ivec2 pos, const pixel& pixel)
{
    assert(valid(pos));
    d_pixels[get_pos(pos)] = pixel;
}

void tile::fill(const pixel& p)
{
    d_pixels.fill(p);
}

}