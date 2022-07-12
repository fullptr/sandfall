#include "update_functions.h"
#include "utility.hpp"

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
    const auto from_props = get_pixel_properties(from.type);
    const auto& to   = pixels.at(dst);
    const auto to_props = get_pixel_properties(to.type);

    if (from_props.movement == pixel_movement::movable_solid) {
        return to_props.movement == pixel_movement::none || to_props.movement == pixel_movement::liquid;
    }
    else if (from_props.movement == pixel_movement::liquid) {
        return to_props.movement == pixel_movement::none;
    }
    return false;
}

auto set_adjacent_free_falling(tile& pixels, glm::ivec2 pos) -> void
{
    const auto l = pos + glm::ivec2{-1, 0};
    const auto r = pos + glm::ivec2{1, 0};

    if (pixels.valid(l)) {
        const auto props = get_pixel_properties(pixels.at(l).type);
        if (props.movement == pixel_movement::movable_solid) {
            auto& px = pixels.at(l);
            px.is_falling = random_from_range(0.0f, 1.0f) > props.inertial_resistance || px.is_falling;
        }
    }

    if (pixels.valid(r)) {
        const auto props = get_pixel_properties(pixels.at(r).type);
        if (props.movement == pixel_movement::movable_solid) {
            auto& px = pixels.at(r);
            px.is_falling = random_from_range(0.0f, 1.0f) > props.inertial_resistance || px.is_falling;
        }
    }
}

auto move_towards(tile& pixels, glm::ivec2 from, glm::ivec2 offset) -> glm::ivec2
{
    glm::ivec2 curr_pos = from;

    const auto a = from;
    const auto b = from + offset;
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        const auto next_pos = a + (b - a) * (i + 1)/steps;

        if (!can_pixel_move_to(pixels, curr_pos, next_pos)) {
            break;
        }

        curr_pos = pixels.swap(curr_pos, next_pos);
        set_adjacent_free_falling(pixels, curr_pos);
    }

    if (curr_pos != from) {
        pixels.at(curr_pos).updated_this_frame = true;
    }

    return curr_pos;
}

}


auto update_sand(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt) -> void
{
    const auto original_pos = pos;
    const auto scope = scope_exit{[&] {
        pixels.at(pos).is_falling = (pos != original_pos);
    }};

    // Apply gravity if can move down
    if (can_pixel_move_to(pixels, pos, below(pos))) {
        auto& vel = pixels.at(pos).velocity;
        vel += settings.gravity * (float)dt;
        vel.y = glm::max(1.0f, vel.y);
        
        pos = move_towards(pixels, pos, vel);
    }

    // Transfer to horizontal
    else {
        auto& data = pixels.at(pos);
        const auto props = get_pixel_properties(data.type);
        auto& vel = data.velocity;
        if (vel.y > 5.0 && vel.x == 0.0) {
            const auto ht = props.horizontal_transfer;
            vel.x = random_from_range(std::max(0.0f, ht - 0.1f), std::min(1.0f, ht + 0.1f)) * vel.y * sign_flip();
            vel.y = 0.0;
        }
        vel.x *= 0.8;

        pos = move_towards(pixels, pos, {vel.x, 0});
        
    }

    if (!pixels.at(pos).updated_this_frame && pixels.at(pos).is_falling) {
        auto offsets = std::array{
            glm::ivec2{-1, 1},
            glm::ivec2{1, 1},
        };

        if (rand() % 2) {
            std::swap(offsets[0], offsets[1]);
        }

        for (auto offset : offsets) {
            pos = move_towards(pixels, pos, offset);
            if (pos != original_pos) {
                break;
            }
        }
    }
}

auto update_water(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt) -> void
{
    auto& data = pixels.at(pos);
    const auto props = get_pixel_properties(data.type);
    auto& vel = data.velocity;
    vel += settings.gravity * (float)dt;
    auto offset = glm::ivec2{0, glm::max(1, (int)vel.y)};
    
    if (move_towards(pixels, pos, offset) != pos) {
        return;
    }

    auto offsets = std::array{
        glm::ivec2{-1, 1},
        glm::ivec2{1, 1},
        glm::ivec2{-1 * props.dispersion_rate, 0},
        glm::ivec2{props.dispersion_rate, 0}
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

auto update_pixel(tile& pixels, glm::ivec2 pos, const world_settings& settings, double dt) -> void
{
    const auto& pixel = pixels.at(pos);
    const auto props = get_pixel_properties(pixel.type);
    if (props.movement == pixel_movement::movable_solid) {
        update_sand(pixels, pos, settings, dt);
    }
    else if (props.movement == pixel_movement::liquid) {
        update_water(pixels, pos, settings, dt);
    }
}

}