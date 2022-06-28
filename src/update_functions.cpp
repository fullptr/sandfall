#include "update_functions.h"
#include "overloaded.hpp"

#include <array>
#include <utility>
#include <variant>

#include <glm/glm.hpp>

namespace sand {
namespace {

std::size_t get_pos(glm::ivec2 pos)
{
    return pos.x + tile_size * pos.y;
}

std::size_t get_below(glm::ivec2 pos)
{
    return pos.x + tile_size * (pos.y + 1);
}

auto below(glm::ivec2 pos) -> glm::ivec2
{
    return pos + glm::ivec2{0, 1};
}

auto can_pixel_move_to(tile::pixels& pixels, glm::ivec2 src, glm::ivec2 dst) -> bool
{
    if (!tile::valid(src) || !tile::valid(dst)) { return false; }

    const auto& from = pixels[get_pos(src)];
    const auto& to   = pixels[get_pos(dst)];

    if (from.is<movable_solid>() && to.is<empty, liquid>()) {
        return true;
    }
    else if (from.is<liquid>() && to.is<empty>()) {
        return true;
    }
    return false;
}

auto move_towards(tile::pixels& pixels, glm::ivec2 from, glm::ivec2 offset) -> glm::ivec2
{
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
        if (can_pixel_move_to(pixels, position, p)) {
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
    return position;
}

}


void update_sand(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    if (can_pixel_move_to(pixels, pos, below(pos))) {
        auto& data = std::get<movable_solid>(pixels[get_pos(pos)].data);
        auto& vel = data.velocity;
        vel.y += 0.2f;
        
        const auto offset = glm::ivec2{0, 1 + vel.y};
        pos = move_towards(pixels, pos, offset);

    } else {
        // Transfer velocity
    }

    if (pixels[get_pos(pos)].updated_this_frame) {
        return;
    }

    auto offsets = std::array{
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    auto& data = std::get<movable_solid>(pixels[get_pos(pos)].data);
    for (auto offset : offsets) {
        if (move_towards(pixels, pos, offset) != pos) {
            return;
        } else {
            data.velocity = {0.0, 0.0};
        }
    }
}

void update_water(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& data = std::get<liquid>(pixels[get_pos(pos)].data);
    auto& vel = data.velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (move_towards(pixels, pos, offset) != pos) {
        return;
    }

    auto offsets = std::array{
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
        glm::ivec2{-1 * data.dispersion_rate, 0},
        glm::ivec2{data.dispersion_rate, 0}
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
        std::swap(offsets[2], offsets[3]);
    }

    for (auto offset : offsets) {
        if (offset.y == 0) {
            data.velocity = {0.0, 0.0};
        }
        if (move_towards(pixels, pos, offset) != pos) {
            return;
        }
    }
}

void update_rock(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    pixels[get_pos(pos)].updated_this_frame = true;
}

}