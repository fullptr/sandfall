#include "tile.h"
#include "log.h"
#include "generator.h"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>

namespace alc {
namespace {
    
alc::generator<glm::ivec2> pixel_path(glm::ivec2 a, glm::ivec2 b)
{
    // The number of steps taken will be the number of pixels in the longest
    // direction. This will ensure no missing pixels.
    int steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        int x = a.x + (float)(i + 1)/steps * (b.x - a.x);
        int y = a.y + (float)(i + 1)/steps * (b.y - a.y);
        co_yield {x, y};
    }
}

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + alc::tile::SIZE * pos.y;
}

glm::ivec2 move_down(alc::tile::pixels& pixels, int start_x, int start_y, int amount)
{
    if (amount < 1) { return {start_x, start_y}; }
    int new_y = start_y;

    const auto can_displace = [](pixel_type type) {
        return type == pixel_type::air || type == pixel_type::water;
    };

    auto current = get_pos({start_x, start_y});
    auto start = current;
    for (int y = start_y + 1; y != start_y + amount + 1; ++y) {
        auto next = get_pos({start_x, y});
        if (tile::valid({start_x, y}) && can_displace(pixels[next].type)) {
            std::swap(pixels[current], pixels[next]);
            current = next;
            new_y = y;
        }
        else {
            break;
        }
    }
    return {start_x, new_y};
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

bool tile::valid(glm::ivec2 pos)
{
    return 0 <= pos.x && pos.x < SIZE && 0 <= pos.y && pos.y < SIZE;
}

void tile::update_sand(const world_settings& settings, double dt, glm::ivec2 pos)
{
// Interface: TODO: Extract into an API object to pass into this function.
    const auto move_towards = [&pixels = d_pixels, &pos](glm::ivec2 offset) {

        const auto can_displace = [](const pixel& src, const pixel& dst) {
            if (src.type == pixel_type::sand && (dst.type == pixel_type::air || dst.type == pixel_type::water)) {
                return true;
            }
            else if (src.type == pixel_type::water && dst.type == pixel_type::air) {
                return true;
            }
            return false;
        };

        std::size_t curr_pos = get_pos(pos);
        auto start = curr_pos;
        for (auto p : pixel_path(pos, pos + offset)) {
            auto next_pos = get_pos(p);
            if (can_displace(pixels[curr_pos], pixels[next_pos])) {
                std::swap(pixels[curr_pos], pixels[next_pos]);
                curr_pos = next_pos;
            } else {
                break;
            }
        }
        if (curr_pos != start) {
            pixels[curr_pos].updated_this_frame = true;
        }
        return curr_pos != start;
    };

    const auto get_pixel = [&pixels = d_pixels, &pos](glm::ivec2 offset) -> pixel& {
        return pixels[get_pos(pos + offset)];
    };

    const auto is_valid = [&pixels = d_pixels, &pos](glm::ivec2 offset) {
        return tile::valid(pos + offset);
    };
    // End of interface

    auto& vel = get_pixel({0, 0}).velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (is_valid(offset) && move_towards(offset)) {
        return;
    }

    std::array<glm::ivec2, 2> offsets = {
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    for (auto offset : offsets) {
        if (is_valid(offset) && move_towards(offset)) {
            if (offset.y == 0) {
                get_pixel(offset).velocity = {0.0, 0.0};
            }
            return;
        }
    }
}

void tile::update_water(const world_settings& settings, double dt, glm::ivec2 pos)
{
    // Interface: TODO: Extract into an API object to pass into this function.
    const auto move_towards = [&pixels = d_pixels, &pos](glm::ivec2 offset) {

        const auto can_displace = [](const pixel& src, const pixel& dst) {
            if (src.type == pixel_type::sand && (dst.type == pixel_type::air || dst.type == pixel_type::water)) {
                return !dst.updated_this_frame;
            }
            else if (src.type == pixel_type::water && dst.type == pixel_type::air) {
                return !dst.updated_this_frame;
            }
            return false;
        };

        std::size_t curr_pos = get_pos(pos);
        auto start = curr_pos;
        for (auto p : pixel_path(pos, pos + offset)) {
            auto next_pos = get_pos(p);
            if (can_displace(pixels[curr_pos], pixels[next_pos])) {
                std::swap(pixels[curr_pos], pixels[next_pos]);
                curr_pos = next_pos;
            } else {
                break;
            }
        }
        if (curr_pos != start) {
            pixels[curr_pos].updated_this_frame = true;
        }
        return curr_pos != start;
    };

    const auto get_pixel = [&pixels = d_pixels, &pos](glm::ivec2 offset) -> pixel& {
        return pixels[get_pos(pos + offset)];
    };

    const auto is_valid = [&pixels = d_pixels, &pos](glm::ivec2 offset) {
        return tile::valid(pos + offset);
    };
    // End of interface

    auto& vel = get_pixel({0, 0}).velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (is_valid(offset) && move_towards(offset)) {
        return;
    }

    std::array<glm::ivec2, 4> offsets = {
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
        glm::ivec2{-1, 0},
        glm::ivec2{1, 0}
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
        std::swap(offsets[2], offsets[3]);
    }

    for (auto offset : offsets) {
        if (is_valid(offset) && move_towards(offset)) {
            if (offset.y == 0) {
                get_pixel(offset).velocity = {0.0, 0.0};
            }
            return;
        }
    }
}

void tile::update_rock(const world_settings&, double, glm::ivec2 pos)
{
    d_pixels[get_pos(pos)].updated_this_frame = true;
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
                update_sand(settings, dt, {x, y});
            } break;
            case pixel_type::rock: {
                update_rock(settings, dt, {x, y});
            } break;
            case pixel_type::water: {
                update_water(settings, dt, {x, y});
            }
            case pixel_type::air: return;
        }
    };

    if (rand() % 2) {
        for (std::uint32_t y = 0; y != SIZE; ++y) {
            if (rand() % 2) {
                for (std::uint32_t x = 0; x != SIZE; ++x) {
                    inner(x, y);
                }
            }
            else {
                for (std::uint32_t x = SIZE; x != 0; ) {
                    --x;
                    inner(x, y);
                }
            }
        }
    }
    else {
        for (std::uint32_t y = SIZE; y != 0; ) {
            --y;
            if (rand() % 2) {
                for (std::uint32_t x = 0; x != SIZE; ++x) {
                    inner(x, y);
                }
            }
            else {
                for (std::uint32_t x = SIZE; x != 0; ) {
                    --x;
                    inner(x, y);
                }
            }
        }
    }

    std::for_each(d_pixels.begin(), d_pixels.end(), [](auto& p) { p.updated_this_frame = false; });
    for (std::size_t pos = 0; pos != SIZE * SIZE; ++pos) {
        d_buffer[pos] = d_pixels[pos].colour;
    }
}

void tile::update_texture()
{
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIZE, SIZE, 0, GL_RGBA, GL_FLOAT, d_buffer.data());
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