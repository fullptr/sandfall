#include "update_functions.h"
#include "overloaded.hpp"
#include "random.hpp"

#include <array>
#include <utility>
#include <variant>

#include <glm/glm.hpp>

namespace sand {
namespace {

auto below(glm::ivec2 pos) -> glm::ivec2
{
    return pos + glm::ivec2{0, 1};
}

auto can_pixel_move_to(const tile& pixels, glm::ivec2 src, glm::ivec2 dst) -> bool
{
    if (!tile::valid(src) || !tile::valid(dst)) { return false; }

    const auto& from = pixels.at(src);
    const auto& to   = pixels.at(dst);

    if (from.is<movable_solid>() && to.is<empty, liquid>()) {
        return true;
    }
    else if (from.is<liquid>() && to.is<empty>()) {
        return true;
    }
    return false;
}

auto move_towards(tile& pixels, glm::ivec2 from, glm::ivec2 offset) -> glm::ivec2
{
    glm::ivec2 curr_pos = from;

    const auto a = from;
    const auto b = from + offset;
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        glm::ivec2 next_pos = a + (b - a) * (i + 1)/steps;

        if (!can_pixel_move_to(pixels, curr_pos, next_pos)) {
            break;
        }

        curr_pos = pixels.swap(curr_pos, next_pos);
    }

    if (curr_pos != from) {
        pixels.at(curr_pos).updated_this_frame = true;
    }

    return curr_pos;
}

}


void update_sand(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    // Apply gravity if can move down
    if (can_pixel_move_to(pixels, pos, below(pos))) {
        auto& vel = std::get<movable_solid>(pixels.at(pos).data).velocity;
        vel.y += 0.2f; // gravity
        vel.y = glm::max(1.0f, vel.y);
        
        pos = move_towards(pixels, pos, vel);
    }

    // Transfer to horizontal
    else {
        auto& vel = std::get<movable_solid>(pixels.at(pos).data).velocity;
        if (vel.y > 5.0 && vel.x == 0.0) {
            vel.x = 0.2 * vel.y * sign_flip();
            vel.y = 0.0;
        }
        vel.x *= 0.8;

        pos = move_towards(pixels, pos, vel);
        
    }

    if (pixels.at(pos).updated_this_frame) {
        return;
    }

    auto offsets = std::array{
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
    };

    if (rand() % 2) {
        std::swap(offsets[0], offsets[1]);
    }

    auto& data = std::get<movable_solid>(pixels.at(pos).data);
    for (auto offset : offsets) {
        if (move_towards(pixels, pos, offset) != pos) {
            return;
        }
    }
}

void update_water(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    auto& data = std::get<liquid>(pixels.at(pos).data);
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

void update_rock(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt)
{
    pixels.at(pos).updated_this_frame = true;
}

}