#include "update_rigid_bodies.hpp"
#include "common.hpp"
#include "world.hpp"
#include "utility.hpp"

#include <bitset>
#include <vector>
#include <print>

#include <glm/glm.hpp>

namespace sand {

using chunk_static_pixels = std::bitset<sand::config::chunk_size * sand::config::chunk_size>;

auto is_static_pixel(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos pos) -> bool
{
    if (!(top_left.x <= pos.x && pos.x < top_left.x + sand::config::chunk_size) || !(top_left.y <= pos.y && pos.y < top_left.y + sand::config::chunk_size)) return false;
    
    if (!w.is_valid_pixel(pos)) return false;
    const auto& pixel = w[pos];
    const auto& props = sand::properties(pixel);
    return pixel.type != sand::pixel_type::none
        && props.phase == sand::pixel_phase::solid
        && !pixel.flags.test(sand::pixel_flags::is_falling);
}

auto is_static_boundary(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos A, glm::ivec2 offset) -> bool
{
    assert(glm::abs(offset.x) + glm::abs(offset.y) == 1);
    const auto static_a = is_static_pixel(top_left, w, A);
    const auto static_b = is_static_pixel(top_left, w, A + offset);
    return (!static_a && static_b) || (!static_b && static_a);
}

auto is_along_boundary(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos curr, pixel_pos next) -> bool
{
    const auto offset = next - curr;
    assert(glm::abs(offset.x) + glm::abs(offset.y) == 1);
    if (offset == up)    return is_static_boundary(top_left, w, next, left);
    if (offset == down)  return is_static_boundary(top_left, w, curr, left);
    if (offset == left)  return is_static_boundary(top_left, w, next, up);
    if (offset == right) return is_static_boundary(top_left, w, curr, up);
    std::unreachable();
}

auto is_boundary_cross(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos curr) -> bool
{
    const auto tl = is_static_pixel(top_left, w, curr + left + up);
    const auto tr = is_static_pixel(top_left, w, curr + up);
    const auto bl = is_static_pixel(top_left, w, curr + left);
    const auto br = is_static_pixel(top_left, w, curr);
    return (tl == br) && (bl == tr) && (tl != tr);
}

auto is_valid_step(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos prev,
    pixel_pos curr,
    pixel_pos next) -> bool
{
    if (!is_along_boundary(top_left, w, curr, next)) return false;
    if (prev.x == 0 && prev.y == 0) return true;
    if (prev == next) return false;

    if (!is_boundary_cross(top_left, w, curr)) return true;

    // in a straight line going over the cross
    if ((prev.x == curr.x && curr.x == next.x) || (prev.y == curr.y && curr.y == next.y)) {
        return false;
    }

    // curving through the cross must go round an actual pixel
    const auto pixel = pixel_pos{
        std::min({prev.x, curr.x, next.x}),
        std::min({prev.y, curr.y, next.y})
    };
    return is_static_pixel(top_left, w, pixel);
}

auto get_boundary(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos start) -> std::vector<pixel_pos>
{
    auto ret = std::vector<pixel_pos>{};
    auto current = start;
    while (is_static_pixel(top_left, w, current + up)) { current = current + up; }
    ret.push_back(current);
    
    // Find second point
    bool found_second = false;
    for (const auto offset : offsets) {
        const auto neigh = current + offset;
        if (is_valid_step(top_left, w, {0, 0}, current, neigh)) {
            current = neigh;
            ret.push_back(current);
            found_second = true;
            break;
        }
    }
    if (!found_second) {
        return {};
    }

    // continue until we get back to the start
    while (current != ret.front()) {
        bool found = false;
        for (const auto offset : offsets) {
            const auto neigh = current + offset;
            if (is_valid_step(top_left, w, ret.rbegin()[1], current, neigh)) {
                current = neigh;
                found = true;
                ret.push_back(current);
                break;
            }
        }
        if (!found) {
            return ret; // hmm
        }
    }

    return ret;
}

auto cross(glm::ivec2 a, glm::ivec2 b) -> float
{
    return a.x * b.y - a.y * b.x;
}

auto perpendicular_distance(pixel_pos p, pixel_pos a, pixel_pos b) -> float
{
    if (a == b) { a.x++; } // little hack to avoid dividing by zero

    const auto ab = glm::vec2{b - a};
    const auto ap = glm::vec2{p - a};
    return glm::abs(cross(ab, ap)) / glm::length(ab);
}

auto ramer_douglas_puecker(std::span<const pixel_pos> points, float epsilon, std::vector<pixel_pos>& out) -> void
{
    if (points.size() < 3) {
        out.insert(out.end(), points.begin(), points.end());
        return;
    }

    // Find the point with the maximum distance
    auto max_dist = 0.0f;
    auto pivot = points.begin() + 2;
    for (auto it = points.begin() + 2; it != points.end() - 1; ++it) {
        const auto dist = perpendicular_distance(*it, points.front(), points.back());
        if (dist > max_dist) {
            max_dist = dist;
            pivot = it;
        }
    }

    // Split and recurse if further than epsilon, otherwise just take the endpoint
    if (max_dist > epsilon) {
        ramer_douglas_puecker({ points.begin(), pivot + 1 }, epsilon, out);
        ramer_douglas_puecker({ pivot, points.end() },       epsilon, out);
    } else {
        out.push_back(points.back());
    }
}

auto calc_boundary(
    pixel_pos top_left,
    const pixel_world& w,
    pixel_pos start,
    float epsilon) -> std::vector<pixel_pos>
{
    const auto points = get_boundary(top_left, w, start);
    if (epsilon == 0.0f) {
        return points;
    }
    auto simplified = std::vector<pixel_pos>{};
    ramer_douglas_puecker(points, epsilon, simplified);
    return simplified;
}

auto flood_remove(chunk_static_pixels& pixels, glm::ivec2 offset) -> void
{
    const auto is_valid = [](const glm::ivec2 p) {
        return 0 <= p.x && p.x < sand::config::chunk_size && 0 <= p.y && p.y < sand::config::chunk_size;
    };

    const auto to_index = [](const glm::ivec2 p) {
        return p.y * sand::config::chunk_size + p.x;
    };

    std::vector<glm::ivec2> to_visit;
    to_visit.push_back(offset);
    while (!to_visit.empty()) {
        const auto curr = to_visit.back();
        to_visit.pop_back();
        pixels.reset(to_index(curr));
        for (const auto offset : offsets) {
            const auto neigh = curr + offset;
            if (is_valid(neigh) && pixels.test(to_index(neigh))) {
                to_visit.push_back(neigh);
            }
        }
    }
}

auto get_start_pixel_offset(const chunk_static_pixels& pixels) -> glm::ivec2
{
    assert(pixels.any());
    for (int x = 0; x != sand::config::chunk_size; ++x) {
        for (int y = 0; y != sand::config::chunk_size; ++y) {
            const auto index = y * sand::config::chunk_size + x;
            if (pixels.test(index)) {
                return {x, y};
            }
        }
    }
    std::unreachable();
}

auto create_chunk_rigid_bodies(level& l, pixel_pos top_left) -> b2BodyId
{
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_staticBody;
    body_def.position = {0.0f, 0.0f};
    auto body = b2CreateBody(l.physics.world, &body_def);
    b2Body_SetUserData(body, to_user_data(apx::null));
    
    auto chunk_pixels = chunk_static_pixels{};
    
    // Fill up the bitset
    for (int x = 0; x != sand::config::chunk_size; ++x) {
        for (int y = 0; y != sand::config::chunk_size; ++y) {
            const auto index = y * sand::config::chunk_size + x;
            if (is_static_pixel(top_left, l.pixels, top_left + glm::ivec2{x, y})) {
                chunk_pixels.set(index);
            }
        }
    }

    b2ShapeDef def = b2DefaultShapeDef();
    def.material.friction = 1;
    
    // While bitset still has elements, take one, apply algorithm to create
    // triangles, then flood remove the pixels
    while (chunk_pixels.any()) {
        const auto offset = get_start_pixel_offset(chunk_pixels);
        const auto boundary = calc_boundary(top_left, l.pixels, top_left + offset, 1.5f);

        if (boundary.size() > 3) { // If there's only a small group, dont bother
            std::vector<b2Vec2> points;
            points.reserve(boundary.size());
            for (const auto pos : boundary) {
                points.push_back(pixel_to_physics(pos));
            }
            b2ChainDef def = b2DefaultChainDef();
            def.enableSensorEvents = true;
            def.count = points.size();
            def.points = points.data();
            def.isLoop = true;
            b2CreateChain(body, &def);
        }
        flood_remove(chunk_pixels, offset);
    }

    return body;
}
    

}