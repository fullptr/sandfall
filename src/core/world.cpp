#include "world.hpp"
#include "pixel.hpp"
#include "utility.hpp"

#include "update_rigid_bodies.hpp"
#include "explosion.hpp"

#include <cassert>
#include <algorithm>
#include <ranges>

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

auto can_pixel_move_to(const world& w, pixel_pos src_pos, pixel_pos dst_pos) -> bool
{
    if (!w.is_valid_pixel(src_pos) || !w.is_valid_pixel(dst_pos)) { return false; }

    // If the destination is empty, we can always move there
    if (w[dst_pos].type == pixel_type::none) { return true; }

    const auto src = properties(w[src_pos]).phase;
    const auto dst = properties(w[dst_pos]).phase;

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

auto set_adjacent_free_falling(world& w, pixel_pos pos) -> void
{
    const auto l = pos + glm::ivec2{-1, 0};
    const auto r = pos + glm::ivec2{1, 0};

    for (const auto x : {l, r}) {
        if (w.is_valid_pixel(x)) {
            const auto& px = w[x];
            const auto& props = properties(px);
            if (props.gravity_factor != 0.0f) {
                w.visit(x, [&](pixel& p) { p.flags[is_falling] = true; });
            }
        }
    }
}

// Moves towards the given offset, updating pos to the new postion and returning
// true if the position has changed
auto move_offset(world& w, pixel_pos& pos, glm::ivec2 offset) -> bool
{
    const auto start_pos = pos;

    const auto a = pos;
    const auto b = pos + offset;
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        const auto next_pos = a + (b - a) * (i + 1)/steps;

        if (!can_pixel_move_to(w, pos, next_pos)) {
            break;
        }

        w.swap(pos, next_pos);
        pos = next_pos;
        set_adjacent_free_falling(w, pos);
    }

    if (start_pos != pos) {
        w.visit(pos, [&](pixel& p) { p.flags[is_falling] = true; });
        return true;
    }

    return false;
}

auto is_surrounded(const world& w, pixel_pos pos) -> bool
{ 
    for (const auto& offset : neighbour_offsets) {
        const auto n = pos + offset;
        if (w.is_valid_pixel(n)) {
            if (w[n].type == pixel_type::none) {
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

inline auto update_pixel_position(world& w, pixel_pos& pos) -> void
{
    const auto& props = properties(w[pos]);

    // Apply gravity
    if (props.gravity_factor) {
        const auto velocity = w[pos].velocity;
        const auto gravity_factor = props.gravity_factor;
        w.visit_no_wake(pos, [&](pixel& p) { p.velocity += gravity_factor * config::gravity * config::time_step; });
        if (move_offset(w, pos, velocity)) return;
    }

    // If we have resistance to moving and we are not, then we are not moving
    if (props.inertial_resistance && !w[pos].flags[is_falling]) {
        return;
    }

    // Attempts to move diagonally up/down
    if (props.can_move_diagonally) {
        const auto dir = sign(props.gravity_factor);
        auto offsets = std::array{glm::ivec2{-1, dir}, glm::ivec2{1, dir}};
        if (coin_flip()) std::swap(offsets[0], offsets[1]);

        for (auto offset : offsets) {
            if (move_offset(w, pos, offset)) return;
        }
    }

    // Attempts to disperse outwards according to the dispersion rate
    if (props.dispersion_rate) {
        const auto dr = props.dispersion_rate;
        auto offsets = std::array{glm::ivec2{-dr, 0}, glm::ivec2{dr, 0}};
        if (coin_flip()) std::swap(offsets[0], offsets[1]);

        for (auto offset : offsets) {
            if (move_offset(w, pos, offset)) return;
        }
    }
}

// Determines if the pixel at the given offset should power the current position.
// offset must be a unit vector.
auto should_get_powered(const world& w, pixel_pos pos, glm::ivec2 offset) -> bool
{
    const auto& dst = w[pos];
    const auto& src = w[pos + offset];

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
        if (!w.is_valid_pixel({new_pos.x, new_pos.y})) return false;
        const auto& new_src = w[new_pos];
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
inline auto update_pixel_attributes(world& w, pixel_pos pos) -> void
{
    const auto& px = w[pos];
    const auto& props = properties(px);

    if (props.always_awake) {
        w.wake_chunk_with_pixel(pos);
    }

    // is_burning status
    if (px.flags[is_burning]) {

        // See if it can be put out
        const auto put_out = is_surrounded(w, pos) ? props.put_out_surrounded : props.put_out;
        if (random_unit() < put_out) {
            w.visit(pos, [&](pixel& p) { p.flags[is_burning] = false; });
        }

        // See if it gets destroyed
        if (random_unit() < props.burn_out_chance) {
            w.set(pos, pixel::air());
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
            if (px.power > 0) {
                w.visit(pos, [&](pixel& p) { --p.power; });
            }

            // Check to see if we should power up just before we hit zero in order to
            // maintain a current
            if (px.power <= 1) {
                for (const auto& offset : adjacent_offsets) {
                    if (w.is_valid_pixel(pos + offset) && should_get_powered(w, pos, offset)) {
                        w.visit(pos, [&](pixel& p) { p.power = props.power_max; });
                        break;
                    }
                }
            }

            if (px.power > 0 && props.explodes_on_power) {
                apply_explosion(w, pos, sand::explosion{
                    .min_radius = 25.0f, .max_radius = 30.0f, .scorch = 10.0f
                });
            }
        } break;

        case pixel_power_type::source: {
            if (px.power < props.power_max) {
                w.visit(pos, [&](pixel& p) { ++p.power; });
            }
            for (const auto& offset : adjacent_offsets) {
                if (!w.is_valid_pixel(pos + offset)) continue;
                auto& neighbour = w[pos + offset];

                // Powered diode_offs disable power sources
                if (neighbour.type == pixel_type::diode_out && neighbour.power > 0) {
                    w.visit(pos, [&](pixel& p) { p.power = 0; });
                    break;
                }
            }
        } break;

        case pixel_power_type::none: {} break;
    }

    if (px.power > 0) {
        w.wake_chunk_with_pixel(pos);
    }

    if (random_unit() < props.spontaneous_destroy) {
        w.set(pos, pixel::air());
    }
}

inline auto update_pixel_neighbours(world& w, pixel_pos pos) -> void
{
    auto& px = w[pos];
    const auto& props = properties(px);

    // Affect adjacent neighbours as well as diagonals
    for (const auto& offset : neighbour_offsets) {
        const auto neigh_pos = pos + offset;
        if (!w.is_valid_pixel(neigh_pos)) continue;   

        const auto& neighbour = w[neigh_pos];

        // Boil water
        if (props.can_boil_water) {
            if (neighbour.type == pixel_type::water) {
                w.set(neigh_pos, pixel::steam());
            }
        }

        // Corrode neighbours
        if (props.is_corrosion_source) {
            if (random_unit() > properties(neighbour).corrosion_resist) {
                w.set(neigh_pos, pixel::air());
                if (random_unit() > 0.9f) {
                    w.set(pos, pixel::air());
                }
            }
        }
        
        // Spread fire
        if (props.is_burn_source || px.flags[is_burning]) {
            if (random_unit() < properties(neighbour).flammability) {
                w.visit(neigh_pos, [&](pixel& p) { p.flags[is_burning] = true; });
            }
        }

        // Produce embers
        const bool can_produce_embers = props.is_ember_source || px.flags[is_burning];
        if (can_produce_embers && neighbour.type == pixel_type::none) {
            if (random_unit() < 0.01f) {
                w.set(neigh_pos, pixel::ember());
            }
        }
    }
}

auto update_pixel(world& w, pixel_pos pos) -> pixel_pos
{
    if (w[pos].type == pixel_type::none || w[pos].flags[is_updated]) {
        return pos;
    }

    const auto start_pos = pos;
    update_pixel_position(w, pos);

    // Pixels that don't move have their is_falling flag set to false
    if (pos == start_pos) {
        w.visit_no_wake(pos, [&](pixel& p) {
            p.flags[is_falling] = false;
            if (properties(p).gravity_factor) {
                p.velocity = glm::ivec2{0, 1};
            }
        });
    }

    update_pixel_neighbours(w, pos);
    update_pixel_attributes(w, pos);
    return pos;
}

auto get_chunk_from_pixel(pixel_pos pos) -> chunk_pos
{
    return {pos.x / config::chunk_size, pos.y / config::chunk_size};
}

}

auto get_chunk_top_left(chunk_pos pos) -> pixel_pos
{
    return {pos.x * config::chunk_size, pos.y * config::chunk_size};
}

auto world::wake_chunk(chunk_pos pos) -> void
{
    assert(is_valid_chunk(pos));
    at(pos).should_step_next = true;
}

auto world::at(pixel_pos pos) -> pixel&
{
    assert(is_valid_pixel(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto world::at(chunk_pos pos) -> chunk&
{
    assert(is_valid_chunk(pos));
    return d_chunks[pos.x + width_in_chunks() * pos.y];
}

auto world::is_valid_pixel(pixel_pos pos) const -> bool
{
    return 0 <= pos.x && pos.x < d_width && 0 <= pos.y && pos.y < d_height;
}

auto world::is_valid_chunk(chunk_pos pos) const -> bool
{
    return 0 <= pos.x && pos.x < width_in_chunks() && 0 <= pos.y && pos.y < height_in_chunks();
}

auto world::operator[](chunk_pos pos) const -> const chunk&
{
    assert(is_valid_chunk(pos));
    const auto width_chunks = d_width / config::chunk_size;
    const auto index = width_chunks * pos.y + pos.x;
    return d_chunks[index];
}

auto world::set(pixel_pos pos, const pixel& p) -> void
{
    assert(is_valid_pixel(pos));
    at(pos) = p;
    wake_chunk_with_pixel(pos);
}

auto world::swap(pixel_pos a, pixel_pos b) -> void
{
    std::swap(at(a), at(b));
    wake_chunk_with_pixel(a);
    wake_chunk_with_pixel(b);
}

auto world::operator[](pixel_pos pos) const -> const pixel&
{
    assert(is_valid_pixel(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto world::wake_all() -> void
{
    for (auto& c : d_chunks) { c.should_step_next = true; }
}

auto world::wake_chunk_with_pixel(pixel_pos pos) -> void
{
    const auto chunk_pos = get_chunk_from_pixel(pos);
    wake_chunk(chunk_pos);

    for (const auto offset : adjacent_offsets) {
        const auto neighbour_chunk = get_chunk_from_pixel(pos + offset);
        if (is_valid_chunk(neighbour_chunk)) {
            wake_chunk(neighbour_chunk);
        }
    }
}

auto world::step() -> void
{
    for (auto& pixel : d_pixels) {
        pixel.flags[is_updated] = false;
    }

    for (auto& chunk : d_chunks) {
        chunk.should_step = std::exchange(chunk.should_step_next, false);
    }

    for (i32 y = d_height - 1; y >= 0; --y) {
        if (coin_flip()) {
            for (i32 x = 0; x != d_width; x += config::chunk_size) {
                const auto chunk = at(get_chunk_from_pixel({x, y}));
                if (chunk.should_step) {
                    for (i32 dx = 0; dx != config::chunk_size; ++dx) {
                        const auto new_pos = update_pixel(*this, {x + dx, y});
                        at(new_pos).flags[is_updated] = true;
                    }
                }
            }
        }
        else {
            for (i32 x = d_width - 1; x >= 0; x -= config::chunk_size) {
                const auto chunk = at(get_chunk_from_pixel({x, y}));
                if (chunk.should_step) {
                    for (i32 dx = 0; dx != config::chunk_size; ++dx) {
                        const auto new_pos = update_pixel(*this, {x - dx, y});
                        at(new_pos).flags[is_updated] = true;
                    }
                }
            }
        }
    }
    
    const auto width_chunks = d_width / config::chunk_size;
    const auto height_chunks = d_height / config::chunk_size;

    for (i32 x = 0; x != width_chunks; ++x) {
        for (i32 y = 0; y != height_chunks; ++y) {
            auto& chunk = d_chunks[y * width_chunks + x];
            if (!chunk.should_step) continue;
            const auto top_left = config::chunk_size * glm::ivec2{x, y};
            const auto tl = pixel_pos{top_left.x, top_left.y};
            create_chunk_triangles(*this, chunk, tl);
        }
    }
    
    d_physics.Step(sand::config::time_step, 8, 3);
}

level::level(i32 width, i32 height, const std::vector<pixel>& data)
    : pixels{width, height, data}
    , spawn_point{width / 2, height / 2}
    , player{pixels.physics()}
    , listener{this}
{
    pixels.physics().SetContactListener(&listener);
}

}