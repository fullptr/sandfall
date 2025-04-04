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

auto can_pixel_move_to(const world& w, glm::ivec2 src_pos, glm::ivec2 dst_pos) -> bool
{
    if (!w.valid(src_pos) || !w.valid(dst_pos)) { return false; }

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

auto set_adjacent_free_falling(world& w, glm::ivec2 pos) -> void
{
    const auto l = pos + glm::ivec2{-1, 0};
    const auto r = pos + glm::ivec2{1, 0};

    for (const auto x : {l, r}) {
        if (w.valid(x)) {
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
auto move_offset(world& w, glm::ivec2& pos, glm::ivec2 offset) -> bool
{
    glm::ivec2 start_pos = pos;

    const auto a = pos;
    const auto b = pos + offset;
    const auto steps = glm::max(glm::abs(a.x - b.x), glm::abs(a.y - b.y));

    for (int i = 0; i != steps; ++i) {
        const auto next_pos = a + (b - a) * (i + 1)/steps;

        if (!can_pixel_move_to(w, pos, next_pos)) {
            break;
        }

        w.swap({pos.x, pos.y}, {next_pos.x, next_pos.y});
        pos = next_pos;
        set_adjacent_free_falling(w, pos);
    }

    if (start_pos != pos) {
        w.visit(pos, [&](pixel& p) { p.flags[is_falling] = true; });
        return true;
    }

    return false;
}

auto is_surrounded(const world& w, glm::ivec2 pos) -> bool
{ 
    for (const auto& offset : neighbour_offsets) {
        if (w.valid(pos + offset)) {
            if (w[pos + offset].type == pixel_type::none) {
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

inline auto update_pixel_position(world& w, glm::ivec2& pos) -> void
{
    const auto& props = properties(w[pos]);

    // Apply gravity
    if (props.gravity_factor) {
        const auto velocity = w[pos].velocity;
        const auto gravity_factor = props.gravity_factor;
        w.visit_no_wake({pos.x, pos.y}, [&](pixel& p) { p.velocity += gravity_factor * config::gravity * config::time_step; });
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
auto should_get_powered(const world& w, glm::ivec2 pos, glm::ivec2 offset) -> bool
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
        if (!w.valid(new_pos)) return false;
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
inline auto update_pixel_attributes(world& w, glm::ivec2 pos) -> void
{
    const auto& px = w[pos];
    const auto& props = properties(px);

    if (props.always_awake) {
        w.wake_chunk_with_pixel({pos.x, pos.y});
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
                    const auto neighbour = pos + offset;
                    if (w.valid(neighbour) && should_get_powered(w, pos, offset)) {
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
                if (!w.valid(pos + offset)) continue;
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
        w.wake_chunk_with_pixel({pos.x, pos.y});
    }

    if (random_unit() < props.spontaneous_destroy) {
        w.set(pos, pixel::air());
    }
}

inline auto update_pixel_neighbours(world& w, glm::ivec2 pos) -> void
{
    auto& px = w[pos];
    const auto& props = properties(px);

    // Affect adjacent neighbours as well as diagonals
    for (const auto& offset : neighbour_offsets) {
        const auto neigh_pos = pos + offset;
        if (!w.valid(neigh_pos)) continue;   

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

auto update_pixel(world& w, glm::ivec2 pos) -> pixel_pos
{
    if (w[pos].type == pixel_type::none || w[pos].flags[is_updated]) {
        return {pos.x, pos.y};
    }

    const auto start_pos = pos;
    update_pixel_position(w, pos);

    // Pixels that don't move have their is_falling flag set to false
    if (pos == start_pos) {
        w.visit_no_wake({pos.x, pos.y}, [&](pixel& p) {
            p.flags[is_falling] = false;
            if (properties(p).gravity_factor) {
                p.velocity = glm::ivec2{0, 1};
            }
        });
    }

    update_pixel_neighbours(w, pos);
    update_pixel_attributes(w, pos);
    return {pos.x, pos.y};
}

}

auto get_chunk_index(std::size_t width, chunk_pos chunk) -> std::size_t
{
    const auto width_chunks = width / config::chunk_size;
    return width_chunks * chunk.y + chunk.x;
}

auto get_chunk_pos(std::size_t width, std::size_t index) -> glm::ivec2
{
    const auto width_chunks = width / config::chunk_size;
    return {index % width_chunks, index / width_chunks};
}

auto get_chunk_from_pixel(pixel_pos pos) -> chunk_pos
{
    return {pos.x / config::chunk_size, pos.y / config::chunk_size};
}

auto wake_pixel_chunk(world& w, glm::ivec2 pos) -> void
{
    if (w.valid(pos)) {
        const auto chunk_pos = get_chunk_from_pixel({pos.x, pos.y});

    }
}

auto world::wake_chunk(chunk_pos pos) -> void
{
    assert(chunk_valid(chunk_pos));
    d_chunks[get_chunk_index(d_width, pos)].should_step_next = true;
}

auto world::at(pixel_pos pos) -> pixel&
{
    assert(valid(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto world::valid(glm::ivec2 pos) const -> bool
{
    return 0 <= pos.x && pos.x < d_width && 0 <= pos.y && pos.y < d_height;
}

auto world::chunk_valid(pixel_pos pos) const -> bool
{
    const auto chunk = chunk_pos{pos.x / sand::config::chunk_size, pos.y / sand::config::chunk_size};
    return get_chunk_index(d_width, chunk) < d_chunks.size();
}

auto world::chunk_valid(chunk_pos pos) const -> bool
{
    return get_chunk_index(d_width, pos) < d_chunks.size();
}

auto world::get_chunk(pixel_pos pos) -> chunk&
{
    assert(chunk_valid(pos));
    const auto chunk_pos = get_chunk_from_pixel(pos);
    const auto width_chunks = d_width / config::chunk_size;
    const auto index = width_chunks * chunk_pos.y + chunk_pos.x;
    return d_chunks[index];
}

auto world::set(glm::ivec2 pos, const pixel& p) -> void
{
    assert(valid(pos));
    at({pos.x, pos.y}) = p;
    wake_chunk_with_pixel({pos.x, pos.y});
}

auto world::swap(pixel_pos a, pixel_pos b) -> void
{
    std::swap(at(a), at(b));
    wake_chunk_with_pixel(a);
    wake_chunk_with_pixel(b);
}

auto world::operator[](glm::ivec2 pos) const -> const pixel&
{
    assert(valid(pos));
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
        const auto neighbour = pos + offset;
        const auto neighbour_chunk = get_chunk_from_pixel({neighbour.x, neighbour.y});
        if (chunk_valid(neighbour)) {
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

    for (int y = d_height - 1; y >= 0; --y) {
        if (coin_flip()) {
            for (int x = 0; x != d_width; x += config::chunk_size) {
                const auto chunk = get_chunk({x, y});
                if (chunk.should_step) {
                    for (int dx = 0; dx != config::chunk_size; ++dx) {
                        const auto pos = glm::ivec2{x + dx, y};
                        const auto new_pos = update_pixel(*this, pos);
                        at(new_pos).flags[is_updated] = true;
                    }
                }
            }
        }
        else {
            for (int x = d_width - 1; x >= 0; x -= config::chunk_size) {
                const auto chunk = get_chunk({x, y});
                if (chunk.should_step) {
                    for (int dx = 0; dx != config::chunk_size; ++dx) {
                        const auto pos = glm::ivec2{x - dx, y};
                        const auto new_pos = update_pixel(*this, pos);
                        at(new_pos).flags[is_updated] = true;
                    }
                }
            }
        }
    }
    
    const auto width_chunks = d_width / config::chunk_size;
    const auto height_chunks = d_height / config::chunk_size;

    for (int x = 0; x != width_chunks; ++x) {
        for (int y = 0; y != height_chunks; ++y) {
            auto& chunk = d_chunks[y * width_chunks + x];
            if (!chunk.should_step) continue;
            const auto top_left = config::chunk_size * glm::ivec2{x, y};
            create_chunk_triangles(*this, chunk, top_left);
        }
    }
    
    d_physics.Step(sand::config::time_step, 8, 3);
}

level::level(std::size_t width, std::size_t height, const std::vector<pixel>& data)
    : pixels{width, height, data}
    , spawn_point{width / 2, height / 2}
    , player{pixels.physics()}
{
}

}