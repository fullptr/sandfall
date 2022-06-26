#include "update_functions.h"

#include <array>
#include <utility>

#include <glm/glm.hpp>

namespace sand {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + tile::SIZE * pos.y;
}

auto move_towards(tile::pixels& pixels, glm::ivec2 from, glm::ivec2 offset) -> bool
{
    const auto can_displace = [](const pixel& src, const pixel& dst) {
        if (src.type == pixel_type::sand && (dst.type == pixel_type::air || dst.type == pixel_type::water)) {
            return true;
        }
        else if (src.type == pixel_type::water && dst.type == pixel_type::air) {
            return true;
        }
        return false;
    };

    glm::ivec2 position = from;

    auto a = from;
    auto b = from + offset;
    int steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        int x = a.x + (float)(i + 1)/steps * (b.x - a.x);
        int y = a.y + (float)(i + 1)/steps * (b.y - a.y);
        glm::ivec2 p{x, y};
        if (!tile::valid(p)) { break; }

        auto curr_pos = get_pos(position);
        auto next_pos = get_pos(p);
        if (can_displace(pixels[curr_pos], pixels[next_pos])) {
            std::swap(pixels[curr_pos], pixels[next_pos]);
            curr_pos = next_pos;
            position = p;
        } else {
            break;
        }
    }
    if (position != from) {
        pixels[get_pos(position)].updated_this_frame = true;
    }
    return position != from;
}

}


void update_sand(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& vel = pixels[get_pos(pos)].velocity;
    vel += settings.gravity * (float)dt;
    glm::ivec2 offset{0, glm::max(1, (int)vel.y)};

    if (move_towards(pixels, pos, offset)) {
        return;
    }

    std::array<glm::ivec2, 2> offsets = {
        glm::ivec2{-1, 1}, glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    for (auto offset : offsets) {
        if (move_towards(pixels, pos, offset)) {
            return;
        } else {
            pixels[get_pos(pos)].velocity = {0.0, 0.0};
        }
    }
}

void update_water(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& vel = pixels[get_pos(pos)].velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (move_towards(pixels, pos, offset)) {
        return;
    }

    std::array<glm::ivec2, 4> offsets = {
        glm::ivec2{-1, 1}, glm::ivec2{1, 1}, glm::ivec2{-1, 0}, glm::ivec2{1, 0}
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
        std::swap(offsets[2], offsets[3]);
    }

    for (auto offset : offsets) {
        if (move_towards(pixels, pos, offset)) {
            if (offset.y == 0) {
                pixels[get_pos(pos + offset)].velocity = {0.0, 0.0};
            }
            return;
        }
    }
}

void update_rock(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    pixels[get_pos(pos)].updated_this_frame = true;
}

}