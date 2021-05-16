#include "tile.h"
#include "log.h"

#include <glad/glad.h>

#include <cassert>
#include <algorithm>

namespace alc {
namespace {

constexpr float gravity = 15.0f;
constexpr float terminal_velocity = 5.0f;

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

    const auto valid = [](glm::ivec2 pos) { // TODO: Replace 128 with SIZE
        return 0 <= pos.x && pos.x < 128 && 0 <= pos.y && pos.y < 128;
    };

    auto current = get_pos({start_x, start_y});
    auto start = current;
    for (int y = start_y + 1; y != start_y + amount + 1; ++y) {
        auto next = get_pos({start_x, y});
        if (valid({start_x, y}) && can_displace(pixels[next].type)) {
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

bool tile::valid(glm::ivec2 pos) const
{
    return 0 <= pos.x && pos.x < SIZE && 0 <= pos.y && pos.y < SIZE;
}

void tile::update_sand(double dt, glm::ivec2 pos)
{
    std::size_t curr_pos = get_pos(pos);
    const auto can_displace = [](pixel_type type) {
        return type == pixel_type::air || type == pixel_type::water;
    };

    auto next_pos = get_pos({pos.x, pos.y + 1});
    if (valid({pos.x, pos.y + 1}) && can_displace(d_pixels[next_pos].type) && !d_pixels[next_pos].updated_this_frame) {
        d_pixels[curr_pos].velocity += glm::vec2{0.0f, gravity * dt};
        if (d_pixels[curr_pos].velocity.y > terminal_velocity) {
            d_pixels[curr_pos].velocity.y = terminal_velocity;
        }
        int spaces_down = glm::floor(d_pixels[curr_pos].velocity.y);

        glm::ivec2 new_pos = move_down(d_pixels, pos.x, pos.y, glm::max(1, spaces_down));
        if (new_pos != glm::ivec2{pos.x, pos.y}) {
            d_pixels[get_pos(new_pos)].updated_this_frame = true;
            return;
        }
    } else {
        d_pixels[curr_pos].velocity = {0.0, 0.0};
    }

    std::array<int, 2> positions = {pos.x-1, pos.x+1};
    if (rand() % 2) {
         positions = {pos.x+1, pos.x-1};
    }

    for (auto new_x : positions) {
        auto next_pos = get_pos({new_x, pos.y + 1});
        if (valid({new_x, pos.y + 1}) && can_displace(d_pixels[next_pos].type) && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }
    }
}

void tile::update_water(double dt, glm::ivec2 pos)
{
    std::size_t curr_pos = get_pos(pos);
    
    auto next_pos = get_pos({pos.x, pos.y + 1});
    if (valid({pos.x, pos.y + 1}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
        d_pixels[curr_pos].velocity += glm::vec2{0.0f, gravity * dt};
        if (d_pixels[curr_pos].velocity.y > terminal_velocity) {
            d_pixels[curr_pos].velocity.y = terminal_velocity;
        }
        int spaces_down = glm::floor(d_pixels[curr_pos].velocity.y);

        glm::ivec2 new_pos = move_down(d_pixels, pos.x, pos.y, glm::max(1, spaces_down));
        if (new_pos != glm::ivec2{pos.x, pos.y}) {
            d_pixels[get_pos(new_pos)].updated_this_frame = true;
            return;
        }
    } else {
        d_pixels[curr_pos].velocity = {0.0, 0.0};
    }

    std::array<int, 2> positions = {pos.x-1, pos.x+1};
    if (rand() % 2) {
         positions = {pos.x+1, pos.x-1};
    }

    for (auto new_x : positions) {
        auto next_pos = get_pos({new_x, pos.y + 1});
        if (valid({new_x, pos.y + 1}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }
    }

    bool coin = rand() % 2;
    if (coin) {
        auto next_pos = get_pos({pos.x-1, pos.y});
        if (valid({pos.x-1, pos.y}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }

        next_pos = get_pos({pos.x+1, pos.y});
        if (valid({pos.x+1, pos.y}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }
    }
    else {
        auto next_pos = get_pos({pos.x+1, pos.y});
        if (valid({pos.x+1, pos.y}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }

        next_pos = get_pos({pos.x-1, pos.y});
        if (valid({pos.x-1, pos.y}) && d_pixels[next_pos].type == pixel_type::air && !d_pixels[next_pos].updated_this_frame) {
            std::swap(d_pixels[curr_pos], d_pixels[next_pos]);
            d_pixels[next_pos].updated_this_frame = true;
            return;
        }
    }
}

void tile::update_rock(double dt, glm::ivec2 pos)
{
    d_pixels[get_pos(pos)].updated_this_frame = true;
}

void tile::bind() const
{
    glBindTexture(GL_TEXTURE_2D, d_texture);
}

void tile::simulate(double dt)
{
    const auto inner = [&] (std::uint32_t x, std::uint32_t y) {
        auto& pixel = d_pixels[get_pos({x, y})];
        if (pixel.updated_this_frame) { return; }
        switch (pixel.type) {
            case pixel_type::sand: {
                update_sand(dt, {x, y});
            } break;
            case pixel_type::rock: {
                update_rock(dt, {x, y});
            } break;
            case pixel_type::water: {
                update_water(dt, {x, y});
            }
            case pixel_type::air: return;
        }
    };

    static bool coin = false;
    if (coin) {
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
    coin = !coin;

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