#include "world.hpp"
#include "pixel.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "update.hpp"
#include "explosion.hpp"
#include "mouse.hpp"
#include "player.hpp"

#include "graphics/renderer.hpp"
#include "graphics/shape_renderer.hpp"
#include "graphics/window.hpp"
#include "graphics/ui.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <imgui/imgui.h>
#include <box2d/box2d.h>
#include <cereal/archives/binary.hpp>

#include <memory>
#include <print>
#include <fstream>
#include <cmath>
#include <span>

static constexpr auto up = glm::ivec2{0, -1};
static constexpr auto right = glm::ivec2{1, 0};
static constexpr auto down = glm::ivec2{0, 1};
static constexpr auto left = glm::ivec2{-1, 0};
static constexpr auto offsets = {up, right, down, left};

using chunk_static_pixels = std::bitset<sand::config::chunk_size * sand::config::chunk_size>;

auto is_static_pixel_temp(
    const sand::world& w,
    glm::ivec2 pos) -> bool
{
    if (!w.valid(pos)) return false;
    const auto& pixel = w.at(pos);
    const auto& props = sand::properties(pixel);
    return pixel.type != sand::pixel_type::none
        && props.phase == sand::pixel_phase::solid
        && !pixel.flags.test(sand::pixel_flags::is_falling);
}

auto is_static_pixel(
    glm::ivec2 top_left,
    const sand::world& w,
    glm::ivec2 pos) -> bool
{
    if (!(top_left.x <= pos.x && pos.x < top_left.x + sand::config::chunk_size) || !(top_left.y <= pos.y && pos.y < top_left.y + sand::config::chunk_size)) return false;
    
    if (!w.valid(pos)) return false;
    const auto& pixel = w.at(pos);
    const auto& props = sand::properties(pixel);
    return pixel.type != sand::pixel_type::none
        && props.phase == sand::pixel_phase::solid
        && !pixel.flags.test(sand::pixel_flags::is_falling);
}

auto is_static_boundary(
    glm::ivec2 top_left,
    const sand::world& w,
    glm::ivec2 A, glm::ivec2 offset) -> bool
{
    assert(glm::abs(offset.x) + glm::abs(offset.y) == 1);
    const auto static_a = is_static_pixel(top_left, w, A);
    const auto static_b = is_static_pixel(top_left, w, A + offset);
    return (!static_a && static_b) || (!static_b && static_a);
}

auto is_along_boundary(
    glm::ivec2 top_left,
    const sand::world& w,
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
    const sand::world& w,
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
    const sand::world& w,
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
    const sand::world& w,
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
    const sand::world& w,
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
            std::print("degenerate polygon detected, could not triangulate\n");
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

auto create_chunk_triangles(sand::world& w, glm::ivec2 chunk_pos) -> void
{
    auto& chunk = w.chunks[sand::get_chunk_index(chunk_pos)];
 
    if (chunk.triangles) {
        w.physics.DestroyBody(chunk.triangles);
    }

    chunk.triangles = new_body(w.physics);
    
    const auto top_left = sand::config::chunk_size * chunk_pos;
    auto nodes = chunk_static_pixels{};
    
    // Fill up the bitset
    for (int x = 0; x != sand::config::chunk_size; ++x) {
        for (int y = 0; y != sand::config::chunk_size; ++y) {
            const auto index = y * sand::config::chunk_size + x;
            if (is_static_pixel_temp(w, top_left + glm::ivec2{x, y})) {
                nodes.set(index);
            }
        }
    }
    
    // While bitset still has elements, take one, apply algorithm to create
    // triangles, then flood remove the pixels
    while (nodes.any()) {
        const auto pos = get_starting_pixel(nodes) + top_left;
        const auto boundary = calc_boundary(top_left, w, pos, 1.5f);
        const auto triangles = triangulate(boundary);
        add_triangles_to_body(*chunk.triangles, triangles);
        const auto body = triangles_to_rigid_bodies(w.physics, triangles);
        flood_remove(nodes, pos - top_left);
    }
}

auto render_body_triangles(sand::shape_renderer& rend, const b2Body* body) -> void
{
    for (auto fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
        if (fixture->GetShape()->GetType() == b2Shape::Type::e_polygon) {
            const auto* shape = static_cast<const b2PolygonShape*>(fixture->GetShape());
            if (shape->m_count == 3) {
                const auto p1 = sand::physics_to_pixel(shape->m_vertices[0]);
                const auto p2 = sand::physics_to_pixel(shape->m_vertices[1]);
                const auto p3 = sand::physics_to_pixel(shape->m_vertices[2]);
                rend.draw_line(p1, p2, {1,0,0,1}, 1);
                rend.draw_line(p2, p3, {1,0,0,1}, 1);
                rend.draw_line(p3, p1, {1,0,0,1}, 1);
            }
        }
    }
}

auto main() -> int
{
    auto exe_path = sand::get_executable_filepath().parent_path();
    std::print("Executable directory: {}\n", exe_path.string());
    auto window = sand::window{"sandfall", 1280, 720};
    auto editor = sand::editor{};
    auto mouse = sand::mouse{};
    auto keyboard = sand::keyboard{};

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = static_cast<float>(window.width()),
        .screen_height = static_cast<float>(window.height()),
        .world_to_screen = 720.0f / 256.0f
    };

    window.set_callback([&](const sand::event& event) {
        auto& io = ImGui::GetIO();
        if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
            return;
        }
        if (event.is_mount_event() && io.WantCaptureMouse) {
            return;
        }

        mouse.on_event(event);
        keyboard.on_event(event);

        if (mouse.is_down(sand::mouse_button::right) && event.is<sand::mouse_moved_event>()) {
            const auto& e = event.as<sand::mouse_moved_event>();
            camera.top_left -= e.offset / camera.world_to_screen;
        }
        else if (event.is<sand::window_resize_event>()) {
            camera.screen_width = window.width();
            camera.screen_height = window.height();
        }
        else if (event.is<sand::mouse_scrolled_event>()) {
            const auto& e = event.as<sand::mouse_scrolled_event>();
            const auto old_centre = mouse_pos_world_space(window, camera);
            camera.world_to_screen += 0.1f * e.offset.y;
            camera.world_to_screen = std::clamp(camera.world_to_screen, 1.0f, 100.0f);
            const auto new_centre = mouse_pos_world_space(window, camera);
            camera.top_left -= new_centre - old_centre;
        }
    });

    auto world           = sand::world{};
    auto world_renderer  = sand::renderer{};
    auto ui              = sand::ui{window};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto player          = sand::player_controller(world.physics, 5);
    auto shape_renderer  = sand::shape_renderer{};

    auto file = std::ifstream{"save0.bin", std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};
    archive(world);
    world.wake_all_chunks();

    auto count = 0;
    auto show_triangles = false;
    auto show_vertices = false;

    const auto chunk_pos = glm::ivec2{3, 3};
    create_chunk_triangles(world, chunk_pos);
    b2Body* triangle_body = world.chunks[sand::get_chunk_index(chunk_pos)].triangles;

    while (window.is_running()) {
        const double dt = timer.on_update();

        mouse.on_new_frame();
        keyboard.on_new_frame();
        
        window.poll_events();
        window.clear();

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;

            sand::update(world);
            player.update(keyboard);
            count++;
            if (count % 5 == 0) {
                create_chunk_triangles(world, chunk_pos);
                triangle_body = world.chunks[sand::get_chunk_index(chunk_pos)].triangles;
            }
        }

        const auto mouse_pos = pixel_at_mouse(window, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (world.valid(coord)) {
                        world.set(coord, editor.get_pixel());
                        updated = true;
                    }
                }
            break; case 1:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (world.valid({x, y})) {
                                world.set({x, y}, editor.get_pixel());
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_down_this_frame(sand::mouse_button::left)) {
                    sand::apply_explosion(world, mouse_pos, sand::explosion{
                        .min_radius = 40.0f, .max_radius = 45.0f, .scorch = 10.0f
                    });
                    updated = true;
                }
        }
        
        // Renders the UI but doesn't yet draw on the screen
        ui.begin_frame();
        const auto mouse_actual = mouse_pos_world_space(window, camera);
        const auto mouse = pixel_at_mouse(window, camera);

        ImGui::ShowDemoWindow(&editor.show_demo);

        if (ImGui::Begin("Editor")) {
            ImGui::Text("Mouse");
            ImGui::Text("Position: {%.2f, %.2f}", mouse_actual.x, mouse_actual.y);
            ImGui::Text("Position: {%d, %d}", (int)std::round(mouse_actual.x), (int)std::round(mouse_actual.y));
            ImGui::Text("Pixel: {%d, %d}", mouse.x, mouse.y);
            ImGui::Separator();

            ImGui::Text("Camera");
            ImGui::Text("Top Left: {%.2f, %.2f}", camera.top_left.x, camera.top_left.y);
            ImGui::Text("Screen width: %.2f", camera.screen_width);
            ImGui::Text("Screen height: %.2f", camera.screen_height);
            ImGui::Text("Scale: %f", camera.world_to_screen);

            ImGui::Separator();
            ImGui::Checkbox("Show Triangles", &show_triangles);
            ImGui::Checkbox("Show Vertices", &show_vertices);
            ImGui::Separator();

            ImGui::Text("Info");
            ImGui::Text("FPS: %d", timer.frame_rate());
            ImGui::Text("Awake chunks: %d", world.num_awake_chunks());
            ImGui::Checkbox("Show chunks", &editor.show_chunks);
            if (ImGui::Button("Clear")) {
                world.wake_all_chunks();
                world.fill(sand::pixel::air());
            }
            ImGui::Separator();

            ImGui::Text("Brush");
            ImGui::SliderFloat("Size", &editor.brush_size, 0, 50);
            if (ImGui::RadioButton("Spray", editor.brush_type == 0)) editor.brush_type = 0;
            if (ImGui::RadioButton("Square", editor.brush_type == 1)) editor.brush_type = 1;
            if (ImGui::RadioButton("Explosion", editor.brush_type == 2)) editor.brush_type = 2;

            for (std::size_t i = 0; i != editor.pixel_makers.size(); ++i) {
                if (ImGui::Selectable(editor.pixel_makers[i].first.c_str(), editor.current == i)) {
                    editor.current = i;
                }
            }
            ImGui::Separator();
            ImGui::Text("Levels");
            for (int i = 0; i != 5; ++i) {
                ImGui::PushID(i);
                const auto filename = std::format("save{}.bin", i);
                if (ImGui::Button("Save")) {
                    auto file = std::ofstream{filename, std::ios::binary};
                    auto archive = cereal::BinaryOutputArchive{file};
                    archive(world);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    auto file = std::ifstream{filename, std::ios::binary};
                    auto archive = cereal::BinaryInputArchive{file};
                    archive(world);
                    world.wake_all_chunks();
                    updated = true;
                }
                ImGui::SameLine();
                ImGui::Text("Save %d", i);
                ImGui::PopID();
            }
        }
        ImGui::End();

        // Render and display the world
        world_renderer.bind();
        if (updated) {
            world_renderer.update(world, editor.show_chunks, camera);
        }
        world_renderer.draw();

        shape_renderer.begin_frame(camera);

        shape_renderer.draw_circle(player.centre(), {1.0, 1.0, 0.0, 1.0}, player.radius());

        //if (show_vertices && points.size() >= 2) {
        //    const auto red = glm::vec4{1,0,0,1};
        //    const auto blue =  glm::vec4{0, 0,1,1};
        //    for (size_t i = 0; i != points.size() - 1; i++) {
        //        const auto t = (float)i / points.size();
        //        const auto colour = sand::lerp(red, blue, t);
        //        shape_renderer.draw_line({points[i]}, {points[i+1]}, colour, colour, 1);
        //        shape_renderer.draw_circle({points[i]}, colour, 0.25);   
        //    }
        //    shape_renderer.draw_line({points.front()}, {points.back()}, {0,1,0,1}, 1);
        //}
        if (show_triangles) {
            render_body_triangles(shape_renderer, triangle_body);
        }
        shape_renderer.end_frame();
        
        // Display the UI
        ui.end_frame();

        window.swap_buffers();
    }
    
    return 0;
}