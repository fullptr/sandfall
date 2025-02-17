#include "update.hpp"
#include "utility.hpp"
#include "config.hpp"
#include "explosion.hpp"
#include "world.hpp"
#include "update_rigid_bodies.hpp"

#include <array>
#include <utility>
#include <variant>
#include <print>
#include <algorithm>
#include <random>
#include <ranges>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

namespace sand {
namespace {

static constexpr auto neighbour_offsets = std::array{
    glm::ivec2{1, 0},
    glm::ivec2{-1, 0},
    glm::ivec2{0, 1},
    glm::ivec2{0, -1},
    glm::ivec2{1, 1},
    glm::ivec2{-1, -1},
    glm::ivec2{-1, 1},
    glm::ivec2{1, -1}
};

static constexpr auto adjacent_offsets = std::array{
    glm::ivec2{1, 0},
    glm::ivec2{-1, 0},
    glm::ivec2{0, 1},
    glm::ivec2{0, -1}
};

auto can_pixel_move_to(const world& w, glm::ivec2 src_pos, glm::ivec2 dst_pos) -> bool
{
    if (!w.pixels.valid(src_pos) || !w.pixels.valid(dst_pos)) { return false; }

    // If the destination is empty, we can always move there
    if (w.at(dst_pos).type == pixel_type::none) { return true; }

    const auto src = properties(w.at(src_pos)).phase;
    const auto dst = properties(w.at(dst_pos)).phase;

    using pm = pixel_phase;
    switch (src) {
        case pm::solid:
            return dst == pm::liquid
                || dst == pm::gas;

        case pm::liquid:
            return dst == pm::gas;

        default:
            return false;
    }
}

auto set_adjacent_free_falling(world& w, glm::ivec2 pos) -> void
{
    const auto l = pos + glm::ivec2{-1, 0};
    const auto r = pos + glm::ivec2{1, 0};

    for (const auto x : {l, r}) {
        if (w.pixels.valid(x)) {
            auto& px = w.at(x);
            const auto& props = properties(px);
            if (props.gravity_factor != 0.0f) {
                w.wake_chunk_with_pixel(l);
                if (random_unit() > props.inertial_resistance) px.flags[is_falling] = true;
            }
        }
    }
}

// Moves towards the given offset, updating pos to the new postion and returning
// true if the position has changed
auto move_offset(world& pixels, glm::ivec2& pos, glm::ivec2 offset) -> bool
{
    glm::ivec2 start_pos = pos;

    const auto a = pos;
    const auto b = pos + offset;
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        const auto next_pos = a + (b - a) * (i + 1)/steps;

        if (!can_pixel_move_to(pixels, pos, next_pos)) {
            break;
        }

        pos = pixels.swap(pos, next_pos);
        set_adjacent_free_falling(pixels, pos);
    }

    if (start_pos != pos) {
        pixels.at(pos).flags[is_falling] = true;
        pixels.wake_chunk_with_pixel(pos);
        return true;
    }

    return false;
}

auto is_surrounded(const world& w, glm::ivec2 pos) -> bool
{ 
    for (const auto& offset : neighbour_offsets) {
        if (w.pixels.valid(pos + offset)) {
            if (w.at(pos + offset).type == pixel_type::none) {
                return false;
            }
        }
    }
    return true;
}

auto sign(float f) -> int
{
    if (f < 0.0f) return -1;
    if (f > 0.0f) return 1;
    return 0;
}

inline auto update_pixel_position(world& pixels, glm::ivec2& pos) -> void
{
    const auto start_pos = pos;

    auto& data = pixels.at(pos);
    const auto& props = properties(data);

    // Pixels that don't move have their is_falling flag set to false at the end
    const auto after_position_update = scope_exit{[&] {
        pixels.at(pos).flags[is_falling] = pos != start_pos;
        if (pos == start_pos && properties(pixels.at(pos)).gravity_factor) {
            pixels.at(pos).velocity = glm::ivec2{0, 1}; // will always try to move at least one block
        }
    }};

    // Apply gravity
    if (props.gravity_factor) {
        const auto gravity_factor = props.gravity_factor;
        data.velocity += gravity_factor * config::gravity * config::time_step;
        if (move_offset(pixels, pos, data.velocity)) return;
    }

    // If we have resistance to moving and we are not, then we are not moving
    if (props.inertial_resistance && !pixels.at(pos).flags[is_falling]) {
        return;
    }

    // Attempts to move diagonally up/down
    if (props.can_move_diagonally) {
        const auto dir = sign(props.gravity_factor);
        auto offsets = std::array{glm::ivec2{-1, dir}, glm::ivec2{1, dir}};
        if (coin_flip()) std::swap(offsets[0], offsets[1]);

        for (auto offset : offsets) {
            if (move_offset(pixels, pos, offset)) return;
        }
    }

    // Attempts to disperse outwards according to the dispersion rate
    if (props.dispersion_rate) {

        const auto dr = props.dispersion_rate;
        auto offsets = std::array{glm::ivec2{-dr, 0}, glm::ivec2{dr, 0}};
        if (coin_flip()) std::swap(offsets[0], offsets[1]);

        for (auto offset : offsets) {
            if (move_offset(pixels, pos, offset)) return;
        }
    }
}

// Determines if the pixel at the given offset should power the current position.
// offset must be a unit vector.
auto should_get_powered(const world& w, glm::ivec2 pos, glm::ivec2 offset) -> bool
{
    const auto& src = w.at(pos + offset);
    const auto& dst = w.at(pos);

    // Prevents current from flowing from diode_out to diode_in
    if (dst.type == pixel_type::diode_in && src.type == pixel_type::diode_out) {
        return false;
    }

    // diode_out can *only* be powered by diode_in and itself
    if (dst.type == pixel_type::diode_out && src.type != pixel_type::diode_in
                                          && src.type != pixel_type::diode_out) {
        return false;
    }

    // If the neighbour is a relay, we need to jump over it and check the pixel on the
    // other side.
    if (src.type == pixel_type::relay) {
        auto new_pos = pos + 2 * offset;
        if (!w.pixels.valid(new_pos)) return false;
        const auto& new_src = w.at(new_pos);
        const auto& props = properties(new_src);
        return is_active_power_source(new_src)
            || ((props.power_max) / 2 < new_src.power && new_src.power < props.power_max);
    }

    // dst can get powered if src is either a power source or powered. Excludes the
    // maximum power level so electricity can only flow one block per tick.
    const auto& props = properties(src);
    return is_active_power_source(src)
        || ((props.power_max) / 2 < src.power && src.power < props.power_max);
}

// Update logic for single pixels depending on properties only
inline auto update_pixel_attributes(world& w, glm::ivec2 pos) -> void
{
    auto& pixel = w.at(pos);
    const auto& props = properties(pixel);

    if (pixel.flags[is_burning] || props.always_awake) {
        w.wake_chunk_with_pixel(pos);
    }

    // is_burning status
    if (pixel.flags[is_burning]) {

        // See if it can be put out
        const auto put_out = is_surrounded(w, pos) ? props.put_out_surrounded : props.put_out;
        if (random_unit() < put_out) {
            pixel.flags[is_burning] = false;
        }

        // See if it gets destroyed
        if (random_unit() < props.burn_out_chance) {
            pixel = pixel::air();
        }

        // See if it explodes
        if (random_unit() < props.explosion_chance) {
            apply_explosion(w, pos, sand::explosion{
                .min_radius = 5.0f, .max_radius = 10.0f, .scorch = 5.0f
            });
        }

    }

    // Electricity
    switch (props.power_type) {
        case pixel_power_type::conductor: {
            if (pixel.power > 0) {
                --pixel.power;
            }

            // Check to see if we should power up just before we hit zero in order to
            // maintain a current
            if (pixel.power <= 1) {
                for (const auto& offset : adjacent_offsets) {
                    if (!w.pixels.valid(pos + offset)) continue;
                    auto& neighbour = w.at(pos + offset);

                    if (should_get_powered(w, pos, offset)) {
                        pixel.power = props.power_max;
                        break;
                    }
                }
            }

            if (pixel.power > 0 && props.explodes_on_power) {
                apply_explosion(w, pos, sand::explosion{
                    .min_radius = 25.0f, .max_radius = 30.0f, .scorch = 10.0f
                });
            }
        } break;

        case pixel_power_type::source: {
            if (pixel.power < props.power_max) {
                ++pixel.power;
            }
            for (const auto& offset : adjacent_offsets) {
                if (!w.pixels.valid(pos + offset)) continue;
                auto& neighbour = w.at(pos + offset);

                // Powered diode_offs disable power sources
                if (neighbour.type == pixel_type::diode_out && neighbour.power > 0) {
                    pixel.power = 0;
                    break;
                }
            }
        } break;

        case pixel_power_type::none: {} break;
    }

    if (pixel.power > 0) {
        w.wake_chunk_with_pixel(pos);
    }

    if (random_unit() < props.spontaneous_destroy) {
        w.set(pos, pixel::air());
    }
}

inline auto update_pixel_neighbours(world& w, glm::ivec2 pos) -> void
{
    auto& pixel = w.at(pos);
    const auto& props = properties(pixel);

    // Affect adjacent neighbours as well as diagonals
    for (const auto& offset : neighbour_offsets) {
        if (!w.pixels.valid(pos + offset)) continue;             
        const auto neigh_pos = pos + offset;
        auto& neighbour = w.at(neigh_pos);

        // Boil water
        if (props.can_boil_water) {
            if (neighbour.type == pixel_type::water) {
                neighbour = pixel::steam();
            }
        }

        // Corrode neighbours
        if (props.is_corrosion_source) {
            if (random_unit() > properties(neighbour).corrosion_resist) {
                neighbour = pixel::air();
                if (random_unit() > 0.9f) {
                    pixel = pixel::air();
                }
            }
        }
        
        // Spread fire
        if (props.is_burn_source || pixel.flags[is_burning]) {
            if (random_unit() < properties(neighbour).flammability) {
                neighbour.flags[is_burning] = true;
                w.wake_chunk_with_pixel(neigh_pos);
            }
        }

        // Produce embers
        const bool can_produce_embers = props.is_ember_source || pixel.flags[is_burning];
        if (can_produce_embers && neighbour.type == pixel_type::none) {
            if (random_unit() < 0.01f) {
                w.set(neigh_pos, pixel::ember());
            }
        }
    }
}

auto update_pixel(world& pixels, glm::ivec2 pos) -> void
{
    if (pixels.at(pos).type == pixel_type::none || pixels.at(pos).flags[is_updated]) {
        return;
    }

    update_pixel_position(pixels, pos);
    update_pixel_neighbours(pixels, pos);
    update_pixel_attributes(pixels, pos);

    pixels.at(pos).flags[is_updated] = true;
}

}

auto update(world& w) -> void
{
    for (auto& pixel : w.pixels) {
        pixel.flags[is_updated] = false;
    }
    
    for (auto it = w.chunks.rbegin(); it != w.chunks.rend(); ++it) {
        auto& chunk = *it;
        chunk.should_step = std::exchange(chunk.should_step_next, false);
        if (!chunk.should_step) continue;
    
        const auto index = w.chunks.size() - std::distance(w.chunks.rbegin(), it) - 1;
        const auto top_left = sand::config::chunk_size * get_chunk_pos(index);
        for (int y = sand::config::chunk_size; y != 0; --y) {
            if (coin_flip()) {
                for (int x = 0; x != sand::config::chunk_size; ++x) {
                    const auto pos = top_left + glm::ivec2{x, y - 1};
                    update_pixel(w, pos);
                }
            }
            else {
                for (int x = sand::config::chunk_size; x != 0; --x) {
                    const auto pos = top_left + glm::ivec2{x - 1, y - 1};
                    update_pixel(w, pos);
                }
            }
        }
        create_chunk_triangles(w, chunk, top_left);
    }
    
    w.physics.Step(sand::config::time_step, 8, 3);
}

}