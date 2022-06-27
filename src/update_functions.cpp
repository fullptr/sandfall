#include "update_functions.h"

#include <array>
#include <utility>

#include <glm/glm.hpp>

namespace sand {
namespace {

std::size_t get_pos(glm::vec2 pos)
{
    return pos.x + tile_size * pos.y;
}

auto move_towards(tile::pixels& pixels, glm::ivec2 from, glm::ivec2 offset) -> bool
{
    const auto can_displace = [](const pixel& src, const pixel& dst) {
        if (std::holds_alternative<movable_solid>(src.data) && (std::holds_alternative<std::monostate>(dst.data) || std::holds_alternative<liquid>(dst.data))) {
            return true;
        }
        else if (std::holds_alternative<liquid>(src.data) && std::holds_alternative<std::monostate>(dst.data)) {
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

// Only moves through air
auto move_towards_new(tile::pixels& pixels, glm::ivec2 from, glm::ivec2 offset) -> bool
{
    auto new_position = from;

    const auto src = from;
    const auto dst = from + offset;
    int steps = glm::max(glm::abs(src.x - dst.x), glm::abs(src.y - dst.y));

    for (int i = 0; i != steps; ++i) {
        int x = src.x + (float)(i + 1)/steps * (dst.x - src.x);
        int y = src.y + (float)(i + 1)/steps * (dst.y - src.y);
        glm::ivec2 p{x, y};
        if (!tile::valid(p)) { break; }

        auto curr_pos = get_pos(new_position);
        auto next_pos = get_pos(p);
        if (std::holds_alternative<std::monostate>(pixels[next_pos].data)) {
            std::swap(pixels[curr_pos], pixels[next_pos]);
            curr_pos = next_pos;
            new_position = p;
        } else {
            break;
        }
    }
    if (new_position != from) {
        pixels[get_pos(new_position)].updated_this_frame = true;
    }
    return new_position != from;
}

}


void update_sand(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& data = std::get<movable_solid>(pixels[get_pos(pos)].data);
    auto& vel = data.velocity;
    vel += settings.gravity * (float)dt;
    glm::ivec2 offset{0, glm::max(1, (int)vel.y)};

    if (move_towards(pixels, pos, offset)) {
        return;
    }

    auto offsets = std::array{
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    for (auto offset : offsets) {
        if (move_towards(pixels, pos, offset)) {
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
    
    if (move_towards_new(pixels, pos, offset)) {
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
        if (move_towards_new(pixels, pos, offset)) {
            return;
        }
    }
}

void update_rock(tile::pixels& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    pixels[get_pos(pos)].updated_this_frame = true;
}

}