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

auto can_pixel_move_to(const pixel_world& w, pixel_pos src_pos, pixel_pos dst_pos) -> bool
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

auto set_adjacent_free_falling(pixel_world& w, pixel_pos pos) -> void
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
auto move_offset(pixel_world& w, pixel_pos& pos, glm::ivec2 offset) -> bool
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

auto is_surrounded(const pixel_world& w, pixel_pos pos) -> bool
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

inline auto update_pixel_position(pixel_world& w, pixel_pos& pos) -> void
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

// Rounds up or down at random, weighted so that the average is f. Lets sub-pixel
// velocities move pixels the right distance over time, and gives neighbouring pixels
// with the same velocity slightly different paths.
auto stochastic_round(float f) -> int
{
    return static_cast<int>(glm::floor(f + random_unit()));
}

// Moves a liquid up to steps pixels sideways, to the furthest free cell in range. It
// can pass through pixels of the same liquid on the way, so liquid in the middle of a
// body pushes outwards rather than jamming against its neighbours.
auto flow_sideways(pixel_world& w, pixel_pos& pos, int dx, int steps) -> bool
{
    const auto type = w[pos].type;
    auto target = pos;
    for (int i = 1; i <= steps; ++i) {
        const auto next = pos + glm::ivec2{dx * i, 0};
        if (can_pixel_move_to(w, pos, next)) {
            target = next;
        } else if (!w.is_valid_pixel(next) || w[next].type != type) {
            break;
        }
    }

    if (target == pos) return false;
    w.swap(pos, target);
    pos = target;
    set_adjacent_free_falling(w, pos);
    w.visit(pos, [&](pixel& p) { p.flags[is_falling] = true; });
    return true;
}

// Moves a liquid sideways through open space towards the nearest spot, within range,
// that it could fall from. This is how a resting surface flows to lower ground and
// levels out, while staying still on the flat.
auto flow_to_drop(pixel_world& w, pixel_pos& pos, glm::ivec2 down, int dx, int range) -> bool
{
    for (int i = 1; i <= range; ++i) {
        const auto next = pos + glm::ivec2{dx * i, 0};
        if (!can_pixel_move_to(w, pos, next)) return false;
        if (can_pixel_move_to(w, pos, next + down)) return move_offset(w, pos, {dx * i, 0});
    }
    return false;
}

// Liquids keep a 2D velocity. They fall under gravity, and on landing part of the fall
// becomes sideways speed, so they never speed up by landing. Along a surface they flow
// in a consistent direction, only gaining speed when flowing downhill (up to
// dispersion_rate pixels per tick) and otherwise slowing down. The weight of liquid on
// top pushes it outwards and a resting surface flows towards lower ground, so pools
// level out and then come to rest. Sideways speed is carried off ledges, so fast flows
// arc away while a trickle drops nearly straight down. Returns true if the pixel is
// still in motion, even if it didn't move this tick.
auto update_liquid_position(pixel_world& w, pixel_pos& pos) -> bool
{
    static constexpr auto air_drag     = 0.9f; // Fraction of sideways speed kept per tick in the air
    static constexpr auto flow_accel   = 0.1f; // Sideways speed gained per tick while flowing along a surface
    static constexpr auto splash       = 0.8f; // Fraction of fall speed turned into sideways speed on landing
    static constexpr auto surface_drag = 0.9f; // Fraction of above-normal sideways speed kept per tick on a surface
    static constexpr auto level_range  = 16;   // How far a resting surface looks for lower ground, in multiples of dispersion_rate

    const auto& props = properties(w[pos]);
    const auto down = glm::ivec2{0, sign(props.gravity_factor)};
    const auto speed = static_cast<float>(glm::max(props.dispersion_rate, 1));
    const auto fall_speed = w[pos].velocity.y * down.y;
    auto vel = w[pos].velocity + props.gravity_factor * config::gravity * config::time_step;

    const auto store_velocity = [&] {
        w.visit_no_wake(pos, [&](pixel& p) { p.velocity = vel; });
    };

    // Falling: move along the velocity, always at least a pixel downwards
    if (can_pixel_move_to(w, pos, pos + down)) {
        vel.x *= air_drag;
        const auto fall = glm::max(1, stochastic_round(glm::abs(vel.y)));
        move_offset(w, pos, {stochastic_round(vel.x), down.y * fall});
        store_velocity();
        return true;
    }

    // Resting on liquid that is still falling: wait for it to move rather than
    // spreading out mid-stream, and don't fall faster than it
    if (w.is_valid_pixel(pos + down)) {
        const auto& below = w[pos + down];
        if (properties(below).phase == pixel_phase::liquid && below.flags[is_falling] && below.velocity.y * down.y > 0.0f) {
            vel.y = glm::min(vel.y * down.y, below.velocity.y * down.y) * down.y;
            store_velocity();
            w.wake_chunk_with_pixel(pos);
            return true;
        }
    }

    // On a surface: keep flowing in the current direction (picking one on landing),
    // downhill if possible, otherwise along the surface, turning around at walls
    const auto dir = vel.x > 0.0f ? 1 : vel.x < 0.0f ? -1 : (coin_flip() ? 1 : -1);

    // Landing turns part of the fall into sideways speed, like a splash. Each pixel gets
    // a random share (weighted towards the top by the sqrt), so a splash throws water
    // out at a continuous spread of speeds rather than all at once. Since splash is
    // below 1 this can only lose speed, and a gentle landing gains almost nothing.
    const auto flow = glm::max(glm::abs(vel.x), fall_speed * splash * glm::sqrt(random_unit()));

    // The sideways speed after moving in direction dx. Only flowing downhill builds
    // speed (up to the liquid's normal speed); on the flat, or above normal speed after
    // a splash, it slows down. Turning around starts again from nothing.
    const auto flow_velocity = [&](int dx, bool downhill) {
        const auto current = dx == dir ? flow : 0.0f;
        const auto next = downhill && current < speed ? glm::min(current + flow_accel, speed) : current * surface_drag;
        return dx * next;
    };

    for (const auto dx : {dir, -dir}) {
        if (move_offset(w, pos, {dx, down.y})) {
            vel = {flow_velocity(dx, true), 0.0f};
            store_velocity();
            return true;
        }
    }

    // Move at the current sideways speed; turning around starts from nothing. Liquid with
    // more of itself on top is pushed outwards by the weight, so it can spread up to
    // dispersion_rate pixels through the body regardless of its speed. This is what
    // levels pools out, and it doesn't add any velocity. Liquid with neither speed nor
    // pressure only moves towards lower ground nearby, so on the flat, puddles and
    // droplets come to rest.
    const auto above = pos + -down;
    const auto pressure = w.is_valid_pixel(above) && w[above].type == w[pos].type ? props.dispersion_rate : 0;
    for (const auto dx : {dir, -dir}) {
        const auto steps = glm::max(pressure, stochastic_round(dx == dir ? flow : 0.0f));
        if (steps > 0 ? flow_sideways(w, pos, dx, steps) : flow_to_drop(w, pos, down, dx, props.dispersion_rate * level_range)) {
            vel = {flow_velocity(dx, steps == 0), 0.0f};
            store_velocity();
            return true;
        }
    }

    // Hemmed in, or resting on the flat with nowhere lower to go, so it has settled
    return false;
}

// Determines if the pixel at the given offset should power the current position.
// offset must be a unit vector.
auto should_get_powered(const pixel_world& w, pixel_pos pos, glm::ivec2 offset) -> bool
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
inline auto update_pixel_attributes(pixel_world& w, pixel_pos pos) -> void
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
            return;
        }

        // See if it explodes
        if (random_unit() < props.explosion_chance) {
            apply_explosion(w, pos, sand::explosion{
                .min_radius = 5.0f, .max_radius = 10.0f, .scorch = 5.0f
            });
            return;
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
                return;
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

    // Runs for every pixel, so skip the roll when it can never succeed
    if (props.spontaneous_destroy > 0.0f && random_unit() < props.spontaneous_destroy) {
        w.set(pos, pixel::air());
    }
}

inline auto update_pixel_neighbours(pixel_world& w, pixel_pos pos) -> void
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
                    return;
                }
            }
        }
        
        // Spread fire
        if (props.is_burn_source || px.flags[is_burning]) {
            const auto flammability = properties(neighbour).flammability;
            if (flammability > 0.0f && random_unit() < flammability) {
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

auto update_pixel(pixel_world& w, pixel_pos pos) -> pixel_pos
{
    if (w[pos].type == pixel_type::none || w[pos].flags[is_updated]) {
        return pos;
    }

    const auto start_pos = pos;
    auto in_motion = false;
    if (properties(w[pos]).phase == pixel_phase::liquid) {
        in_motion = update_liquid_position(w, pos);
    } else {
        update_pixel_position(w, pos);
    }

    // Pixels that don't move have their is_falling flag set to false. Liquids can
    // stay in motion for a tick without moving, e.g. waiting in a falling stream.
    if (pos == start_pos && !in_motion) {
        w.visit_no_wake(pos, [&](pixel& p) {
            p.flags[is_falling] = false;
            // Settled liquids keep no velocity, since any fall speed would turn into
            // sideways flow next tick. Liquids fall at least a pixel per tick anyway.
            if (properties(p).phase == pixel_phase::liquid) {
                p.velocity = glm::vec2{0.0f, 0.0f};
            } else if (properties(p).gravity_factor) {
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

auto player_handle_event(level& l, const context& ctx, entity e, const event& ev) -> void
{
    auto [body_comp, player_comp] = l.entities.get_all<body_component, player_component>(e);

    const bool on_ground = !player_comp.floors.empty();
    if (const auto inner = ev.get_if<keyboard_pressed_event>()) {
        if (inner->key == keyboard::space) {
            if (on_ground || player_comp.double_jump) {
                if (!on_ground) {
                    player_comp.double_jump = false;
                }
                const auto impulse = b2Body_GetMass(body_comp.body) * 7;
                b2Body_ApplyLinearImpulseToCenter(body_comp.body, b2Vec2(0, -impulse), true);
            }
        }
        else if (inner->key == keyboard::S) {
            if (player_comp.ground_pound) {
                player_comp.ground_pound = false;
                const auto impulse = b2Body_GetMass(body_comp.body) * 7;
                b2Body_ApplyLinearImpulseToCenter(body_comp.body, b2Vec2(0, impulse), true);
            }
        }
    }
    else if (const auto inner = ev.get_if<mouse_pressed_event>()) {
        if (inner->button == mouse::left) {
            const auto centre = ecs_entity_centre(l.entities, e);
            const auto direction = normalize_or_zero(mouse_pos_world_space(ctx.input, ctx.camera) - centre);
            if (direction == glm::vec2{0.0f, 0.0f}) return; // No aim direction, so don't fire
            const auto spawn_pot = centre + 10.0f * direction;
            
            auto grenade = l.entities.create();
            auto& body_comp = l.entities.emplace<body_component>(grenade);
            l.entities.emplace<grenade_component>(grenade);
            
            b2BodyDef def = b2DefaultBodyDef();
            def.type = b2_dynamicBody;
            def.enableSleep = false;
            def.gravityScale = 1.0f;
            def.fixedRotation = true;
            def.linearDamping = 1.0f;
            def.position = pixel_to_physics(spawn_pot);

            body_comp.body = b2CreateBody(l.physics.world, &def);
            b2Body_SetUserData(body_comp.body, to_user_data(grenade));
            b2Body_SetMassData(body_comp.body, b2MassData{.mass = 10});

            const auto impulse = 12.0f * b2Body_GetMass(body_comp.body) * direction;
            b2Body_ApplyLinearImpulseToCenter(body_comp.body, b2Vec2(impulse.x, impulse.y), true);

            // Set up main body fixture
            {
                b2Circle circle = {};
                circle.radius = pixel_to_physics(1.0f);

                b2ShapeDef def = b2DefaultShapeDef();
                def.density = 1.0f;
                def.enableContactEvents = true;

                body_comp.body_fixture = b2CreateCircleShape(body_comp.body, &def, &circle);
            }
        }
    }
}

auto update_player(registry& entities, entity e, const input& in) -> void
{
    auto [body_comp, player_comp] = entities.get_all<body_component, player_component>(e);
    
    const bool on_ground = !player_comp.floors.empty();
    const bool can_move_left = player_comp.num_left_contacts == 0;
    const bool can_move_right = player_comp.num_right_contacts == 0;

    const auto vel = b2Body_GetLinearVelocity(body_comp.body);
    
    auto direction = 0;
    if (can_move_left && in.is_down(keyboard::A)) {
        direction -= 1;
    }
    if (can_move_right && in.is_down(keyboard::D)) {
        direction += 1;
    }
    
    const auto max_vel = 5.0f;
    auto desired_vel = 0.0f;
    if (direction == -1) { // left
        if (vel.x > -max_vel) desired_vel = glm::max(vel.x - max_vel, -max_vel);
    } else if (direction == 1) { // right
        if (vel.x < max_vel) desired_vel = glm::min(vel.x + max_vel, max_vel);
    }

    // TODO: Is this actually doing anything?
    b2Shape_SetFriction(body_comp.body_fixture, (desired_vel != 0) ? 0.1f : 0.3f);

    float vel_change = desired_vel - vel.x;
    float impulse = b2Body_GetMass(body_comp.body) * vel_change;
    b2Body_ApplyLinearImpulseToCenter(body_comp.body, b2Vec2(impulse, 0), true);

    if (on_ground) {
        player_comp.double_jump = true;
        player_comp.ground_pound = true;
    }
}

auto update_enemy(registry& entities, entity e) -> void
{
    if (entities.has<enemy_component>(e)) {
        auto [body_comp, enemy_comp] = entities.get_all<body_component, enemy_component>(e);
        for (const auto curr : enemy_comp.nearby_entities) {
            if (entities.has<player_component>(curr)) {
                auto& curr_body_comp = entities.get<body_component>(curr);
                const auto pos = physics_to_pixel(b2Body_GetPosition(curr_body_comp.body));
                const auto self_pos = ecs_entity_centre(entities, e);
                const auto dir = normalize_or_zero(pos - self_pos);
                b2Body_ApplyLinearImpulseToCenter(body_comp.body, pixel_to_physics(200.0f * dir), true);
            }
        }
    }
}

}

auto get_chunk_top_left(chunk_pos pos) -> pixel_pos
{
    return {pos.x * config::chunk_size, pos.y * config::chunk_size};
}

auto pixel_world::wake_chunk(chunk_pos pos) -> void
{
    assert(is_valid_chunk(pos));
    at(pos).should_step_next = true;
}

auto pixel_world::at(pixel_pos pos) -> pixel&
{
    assert(is_valid_pixel(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto pixel_world::at(chunk_pos pos) -> chunk&
{
    assert(is_valid_chunk(pos));
    const auto index = pos.x + width_in_chunks() * pos.y;
    assert(index < d_chunks.size());
    return d_chunks[index];
}

auto pixel_world::is_valid_pixel(pixel_pos pos) const -> bool
{
    return 0 <= pos.x && pos.x < d_width && 0 <= pos.y && pos.y < d_height;
}

auto pixel_world::is_valid_chunk(chunk_pos pos) const -> bool
{
    return 0 <= pos.x && pos.x < width_in_chunks() && 0 <= pos.y && pos.y < height_in_chunks();
}

auto pixel_world::operator[](chunk_pos pos) const -> const chunk&
{
    assert(is_valid_chunk(pos));
    const auto width_chunks = d_width / config::chunk_size;
    const auto index = width_chunks * pos.y + pos.x;
    return d_chunks[index];
}

auto pixel_world::set(pixel_pos pos, const pixel& p) -> void
{
    assert(is_valid_pixel(pos));
    at(pos) = p;
    wake_chunk_with_pixel(pos);
}

auto pixel_world::swap(pixel_pos a, pixel_pos b) -> void
{
    std::swap(at(a), at(b));
    wake_chunk_with_pixel(a);
    wake_chunk_with_pixel(b);
}

auto pixel_world::operator[](pixel_pos pos) const -> const pixel&
{
    assert(is_valid_pixel(pos));
    return d_pixels[pos.x + d_width * pos.y];
}

auto pixel_world::wake_all() -> void
{
    for (auto& c : d_chunks) { c.should_step_next = true; }
}

auto pixel_world::wake_chunk_with_pixel(pixel_pos pos) -> void
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

auto pixel_world::step() -> void
{
    for (auto& chunk : d_chunks) {
        chunk.should_step = std::exchange(chunk.should_step_next, false);
    }

    // is_updated is only read for pixels in chunks stepping this tick, so only
    // those need resetting. Sleeping chunks get reset on the tick they wake up.
    // Walks row by row to keep memory access sequential.
    for (i32 y = 0; y != d_height; ++y) {
        for (i32 x = 0; x != d_width; x += config::chunk_size) {
            if (!at(get_chunk_from_pixel({x, y})).should_step) continue;
            for (i32 dx = 0; dx != config::chunk_size; ++dx) {
                at(pixel_pos{x + dx, y}).flags[is_updated] = false;
            }
        }
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
}

static auto make_world(glm::vec2 gravity) -> b2WorldId
{
    std::print("making world\n");
    auto def = b2DefaultWorldDef();
    def.gravity = {gravity.x, gravity.y};
    return b2CreateWorld(&def);
}

physics_world::physics_world(glm::vec2 gravity)
    : world{make_world(gravity)}
{
}

physics_world::~physics_world()
{
    b2DestroyWorld(world);
}

static void begin_contact(level& l, b2ShapeId curr, b2ShapeId other)
{
    const auto curr_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(curr)));
    const auto other_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(other)));
    
    if (!l.entities.valid(curr_entity)) return;

    // A grenade can have several contacts in one step; only explode on the first
    if (l.entities.has<grenade_component>(curr_entity)
        && !l.entities.marked_entities().contains(curr_entity)
        && !b2Shape_IsSensor(other)
        && (!l.entities.valid(other_entity) || !l.entities.has<player_component>(other_entity)))
    {
        l.entities.mark_for_death(curr_entity);
        const auto pos = ecs_entity_centre(l.entities, curr_entity);
        apply_explosion(l.pixels, pixel_pos::from_ivec2(pos), explosion{.min_radius=5, .max_radius=10, .scorch=15});   
    }
}

static void end_contact(level& l, b2ShapeId curr, b2ShapeId other)
{
}

static void begin_sensor_event(level& l, b2ShapeId sensor, b2ShapeId other)
{
    const auto sensor_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(sensor)));
    const auto other_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(other)));
    
    if (!l.entities.valid(sensor_entity)) return;
    
    if (l.entities.has<player_component>(sensor_entity) && !b2Shape_IsSensor(other))
    {
        auto& comp = l.entities.get<player_component>(sensor_entity);
        
        if (b2Shape_IsValid(comp.foot_sensor) && B2_ID_EQUALS(sensor, comp.foot_sensor)) {
            comp.floors.insert(other);
        }
        if (b2Shape_IsValid(comp.left_sensor) && B2_ID_EQUALS(sensor, comp.left_sensor)) {
            ++comp.num_left_contacts;
        }
        if (b2Shape_IsValid(comp.right_sensor) && B2_ID_EQUALS(sensor, comp.right_sensor)) {
            ++comp.num_right_contacts;
        }
    }
    
    if (l.entities.has<enemy_component>(sensor_entity) && l.entities.valid(other_entity))
    {
        auto& comp = l.entities.get<enemy_component>(sensor_entity);
        if (b2Shape_IsValid(comp.proximity_sensor) && B2_ID_EQUALS(sensor, comp.proximity_sensor) && l.entities.valid(other_entity)) {
            comp.nearby_entities.insert(other_entity);
        }
    }
}

static void end_sensor_event(level& l, b2ShapeId sensor, b2ShapeId other)
{
    if (!b2Shape_IsValid(sensor)) return;
    
    const auto sensor_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(sensor)));
    if (!l.entities.valid(sensor_entity)) return;
    
    if (l.entities.has<player_component>(sensor_entity) && !b2Shape_IsSensor(other))
    {
        auto& comp = l.entities.get<player_component>(sensor_entity);
        
        if (b2Shape_IsValid(comp.foot_sensor) && B2_ID_EQUALS(sensor, comp.foot_sensor)) {
            comp.floors.erase(other);
        }
        if (b2Shape_IsValid(comp.left_sensor) && B2_ID_EQUALS(sensor, comp.left_sensor)) {
            --comp.num_left_contacts;
        }
        if (b2Shape_IsValid(comp.right_sensor) && B2_ID_EQUALS(sensor, comp.right_sensor)) {
            --comp.num_right_contacts;
        }
    }
    
    // Both shapes in the end event can be invalid
    if (!b2Shape_IsValid(other)) return;

    const auto other_entity = from_user_data(b2Body_GetUserData(b2Shape_GetBody(other)));
    if (l.entities.has<enemy_component>(sensor_entity) && l.entities.valid(other_entity))
    {
        auto& comp = l.entities.get<enemy_component>(sensor_entity);
        if (b2Shape_IsValid(comp.proximity_sensor) && B2_ID_EQUALS(sensor, comp.proximity_sensor) && l.entities.valid(other_entity)) {
            comp.nearby_entities.erase(other_entity);
        }
    }
}

auto level_on_update(level& l, const context& ctx) -> void
{
    l.pixels.step();

    b2World_Step(l.physics.world, sand::config::time_step, 4);

    {
        b2ContactEvents events = b2World_GetContactEvents(l.physics.world);
    
        for (std::size_t i = 0; i != events.beginCount; ++i) {
            begin_contact(l, events.beginEvents[i].shapeIdA, events.beginEvents[i].shapeIdB);
            begin_contact(l, events.beginEvents[i].shapeIdB, events.beginEvents[i].shapeIdA);
        }
    
        for (std::size_t i = 0; i != events.endCount; ++i) {
            end_contact(l, events.endEvents[i].shapeIdA, events.endEvents[i].shapeIdB);
            end_contact(l, events.endEvents[i].shapeIdB, events.endEvents[i].shapeIdA);
        }
    }

    {
        b2SensorEvents events = b2World_GetSensorEvents(l.physics.world);
    
        for (std::size_t i = 0; i != events.beginCount; ++i) {
            begin_sensor_event(l, events.beginEvents[i].sensorShapeId, events.beginEvents[i].visitorShapeId);
        }
    
        for (std::size_t i = 0; i != events.endCount; ++i) {
            end_sensor_event(l, events.endEvents[i].sensorShapeId, events.endEvents[i].visitorShapeId);
        }
    }

    const auto width_chunks = l.pixels.width_in_chunks();
    const auto height_chunks = l.pixels.height_in_chunks();
    for (i32 x = 0; x != width_chunks; ++x) {
        for (i32 y = 0; y != height_chunks; ++y) {
            const auto pos = chunk_pos{x, y};
            if (!l.pixels[pos].should_step) continue;
            
            const auto top_left = get_chunk_top_left(pos);
            const auto static_pixels = get_chunk_static_pixels(l.pixels, top_left);

            // Awake chunks often only contain moving liquids, gases or falling
            // pixels, so only rebuild if the static pixels have actually changed
            auto& map = l.physics.chunk_bodies;
            if (auto it = map.find(pos); it != map.end()) {
                if (it->second.static_pixels == static_pixels) continue;
                b2DestroyBody(it->second.body);
                map.erase(it);
            }
            map[pos] = {create_chunk_rigid_bodies(l, top_left, static_pixels), static_pixels};
        }
    }

    for (auto e : l.entities.view<player_component>()) {
        update_player(l.entities, e, ctx.input);
    }

    for (auto e : l.entities.view<enemy_component>()) {
        update_enemy(l.entities, e);
    }

    // Clean up all entities about to go.
    for (auto e : l.entities.marked_entities()) {
        if (l.entities.has<body_component>(e)) {
            const auto& comp = l.entities.get<body_component>(e);
            if (b2Body_IsValid(comp.body)) {
                b2DestroyBody(comp.body);
            }
        }

        // The sensor end events for this body will have invalid shape ids, so
        // they can't be mapped back to the entity; remove it from sensor sets here.
        for (auto enemy : l.entities.view<enemy_component>()) {
            l.entities.get<enemy_component>(enemy).nearby_entities.erase(e);
        }
    }
    l.entities.destroy_marked();
    
}

auto level_on_event(level& l, const context& ctx, const event& ev) -> void
{
    for (auto e : l.entities.view<player_component, body_component>()) {
        player_handle_event(l, ctx, e, ev);
    }
}


}