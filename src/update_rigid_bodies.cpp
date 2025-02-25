#include "update_rigid_bodies.hpp"
#include "config.hpp"
#include "world.hpp"
#include "utility.hpp"

#include <bitset>
#include <vector>
#include <print>

#include <glm/glm.hpp>

namespace sand {

using chunk_static_pixels = std::bitset<sand::config::chunk_size * sand::config::chunk_size>;

auto is_static_pixel(
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 pos) -> bool
{
    if (!(top_left.x <= pos.x && pos.x < top_left.x + sand::config::chunk_size) || !(top_left.y <= pos.y && pos.y < top_left.y + sand::config::chunk_size)) return false;
    
    if (!w.valid(pos)) return false;
    const auto& pixel = w[pos];
    const auto& props = sand::properties(pixel);
    return pixel.type != sand::pixel_type::none
        && props.phase == sand::pixel_phase::solid
        && !pixel.flags.test(sand::pixel_flags::is_falling);
}

auto is_static_boundary(
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 A, glm::ivec2 offset) -> bool
{
    assert(glm::abs(offset.x) + glm::abs(offset.y) == 1);
    const auto static_a = is_static_pixel(top_left, w, A);
    const auto static_b = is_static_pixel(top_left, w, A + offset);
    return (!static_a && static_b) || (!static_b && static_a);
}

auto is_along_boundary(
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 curr, glm::ivec2 next) -> bool
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
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 curr) -> bool
{
    const auto tl = is_static_pixel(top_left, w, curr + left + up);
    const auto tr = is_static_pixel(top_left, w, curr + up);
    const auto bl = is_static_pixel(top_left, w, curr + left);
    const auto br = is_static_pixel(top_left, w, curr);
    return (tl == br) && (bl == tr) && (tl != tr);
}

auto is_valid_step(
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 prev,
    glm::ivec2 curr,
    glm::ivec2 next) -> bool
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
    const auto pixel = glm::ivec2{
        std::min({prev.x, curr.x, next.x}),
        std::min({prev.y, curr.y, next.y})
    };
    return is_static_pixel(top_left, w, pixel);
}

auto get_boundary(
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 start) -> std::vector<glm::ivec2>
{
    auto ret = std::vector<glm::ivec2>{};
    auto current = start;
    while (is_static_pixel(top_left, w, current + up)) { current += up; }
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

auto perpendicular_distance(glm::ivec2 p, glm::ivec2 a, glm::ivec2 b) -> float
{
    if (a == b) { a.x++; } // little hack to avoid dividing by zero

    const auto ab = glm::vec2{b - a};
    const auto ap = glm::vec2{p - a};
    return glm::abs(cross(ab, ap)) / glm::length(ab);
}

auto ramer_douglas_puecker(std::span<const glm::ivec2> points, float epsilon, std::vector<glm::ivec2>& out) -> void
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
    glm::ivec2 top_left,
    const world& w,
    glm::ivec2 start,
    float epsilon) -> std::vector<glm::ivec2>
{
    const auto points = get_boundary(top_left, w, start);
    if (epsilon == 0.0f) {
        return points;
    }
    auto simplified = std::vector<glm::ivec2>{};
    ramer_douglas_puecker(points, epsilon, simplified);
    return simplified;
}

struct triangle
{
    glm::ivec2 a;
    glm::ivec2 b;
    glm::ivec2 c;
};

auto new_body(b2World& world) -> b2Body*
{
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.position.Set(0.0f, 0.0f);
    return world.CreateBody(&bodyDef);
}

auto add_triangles_to_body(b2Body& body, const std::vector<triangle>& triangles) -> void
{
    b2PolygonShape polygonShape;
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &polygonShape;

    for (const triangle& t : triangles) {
        const auto vertices = {
            sand::pixel_to_physics(t.a),
            sand::pixel_to_physics(t.b),
            sand::pixel_to_physics(t.c)
        };

        polygonShape.Set(std::data(vertices), std::size(vertices)); 
        body.CreateFixture(&fixtureDef);
    }
}

auto triangles_to_rigid_bodies(b2World& world, const std::vector<triangle>& triangles) -> b2Body*
{
    b2Body* body = new_body(world);
    add_triangles_to_body(*body, triangles);
    return body;
}

inline auto are_collinear(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c) -> bool
{
    return cross(b - a, c - a) == 0;
}

inline auto is_convex(glm::ivec2 a, glm::ivec2 b, glm::ivec2 c) -> bool {
    return cross(b - a, c - a) > 0;  // Positive cross product means counter-clockwise turn (convex)
}

// Check if a point p is inside the triangle
auto point_in_triangle(glm::ivec2 p, const triangle& t) -> bool
{
    const auto area_abc = (t.b.x - t.a.x) * (t.c.y - t.a.y) - (t.b.y - t.a.y) * (t.c.x - t.a.x);
    const auto area_pab = (t.a.x -   p.x) * (t.b.y -   p.y) - (t.a.y -   p.y) * (t.b.x -   p.x);
    const auto area_pbc = (t.b.x -   p.x) * (t.c.y -   p.y) - (t.b.y -   p.y) * (t.c.x -   p.x);
    const auto area_pca = (t.c.x -   p.x) * (t.a.y -   p.y) - (t.c.y -   p.y) * (t.a.x -   p.x);

    return (area_abc >= 0 && area_pab >= 0 && area_pbc >= 0 && area_pca >= 0) ||
            (area_abc <= 0 && area_pab <= 0 && area_pbc <= 0 && area_pca <= 0);
}

// Check if a triangle is valid (no points inside it and not degenerate)
auto is_valid_triangle(const triangle& t, std::span<const glm::ivec2> points) -> bool
{
    if (are_collinear(t.a, t.b, t.c)) {
        return false;  // Collinear points cannot form a valid triangle
    }

    for (const auto& point : points) {
        if (point != t.a && point != t.b && point != t.c) {
            if (point_in_triangle(point, t)) {
                return false;  // Point is inside the triangle, so it's not a valid triangle
            }
        }
    }
    return true;
}

auto remove_collinear_points(const std::vector<glm::ivec2>& points) -> std::vector<glm::ivec2>
{
    const auto n = points.size();
    auto filtered_points = std::vector<glm::ivec2>{};

    for (std::size_t curr = 0; curr != n; ++curr) {
        const auto prev = (curr == 0) ? n - 1 : curr - 1;
        const auto next = (curr == n - 1) ? 0 : curr + 1;
        if (!are_collinear(points[prev], points[curr], points[next])) {
            filtered_points.push_back(points[curr]);
        }
    }

    return filtered_points;
}

auto triangulate(std::vector<glm::ivec2> vertices) -> std::vector<triangle>
{
    auto triangles = std::vector<triangle>{};
    auto points = remove_collinear_points(vertices);  // Work on a copy of the vertices

    while (points.size() > 3) {
        bool earFound = false;

        for (size_t curr = 0; curr < points.size(); ++curr) {
            const auto prev = (curr == 0) ? points.size() - 1 : curr - 1;
            const auto next = (curr == points.size() - 1) ? 0 : curr + 1;
            const auto t = triangle{points[prev], points[curr], points[next]};

            if (!is_convex(t.a, t.b, t.c)) {
                continue;
            }

            if (is_valid_triangle(t, points)) {
                triangles.push_back(t);
                points.erase(points.begin() + curr);
                earFound = true;
                break;  // start over with the reduced polygon
            }
        }

        if (!earFound) {
            break;
        }
    }

    if (points.size() == 3) {
        triangles.push_back({points[0], points[1], points[2]});
    }

    return triangles;
}

auto flood_remove(chunk_static_pixels& pixels, glm::ivec2 pos) -> void
{
    const auto is_valid = [](const glm::ivec2 p) {
        return 0 <= p.x && p.x < sand::config::chunk_size && 0 <= p.y && p.y < sand::config::chunk_size;
    };

    const auto to_index = [](const glm::ivec2 p) {
        return p.y * sand::config::chunk_size + p.x;
    };

    std::vector<glm::ivec2> to_visit;
    to_visit.push_back(pos);
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

auto get_starting_pixel(const chunk_static_pixels& pixels) -> glm::ivec2
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

auto create_chunk_triangles(world& w, chunk& c, glm::ivec2 top_left) -> void
{
    if (c.triangles) {
        w.physics().DestroyBody(c.triangles);
    }
    
    c.triangles = new_body(w.physics());
    
    auto chunk_pixels = chunk_static_pixels{};
    
    // Fill up the bitset
    for (int x = 0; x != sand::config::chunk_size; ++x) {
        for (int y = 0; y != sand::config::chunk_size; ++y) {
            const auto index = y * sand::config::chunk_size + x;
            if (is_static_pixel(top_left, w, top_left + glm::ivec2{x, y})) {
                chunk_pixels.set(index);
            }
        }
    }
    
    // While bitset still has elements, take one, apply algorithm to create
    // triangles, then flood remove the pixels
    while (chunk_pixels.any()) {
        const auto pos = get_starting_pixel(chunk_pixels) + top_left;
        const auto boundary = calc_boundary(top_left, w, pos, 1.5f);
        const auto triangles = triangulate(boundary);
        add_triangles_to_body(*c.triangles, triangles);
        flood_remove(chunk_pixels, pos - top_left);
    }
}
    

}