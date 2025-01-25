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

// Converts a point in pixel space to world space


class static_physics_box
{
    int       d_width;
    int       d_height;
    glm::vec4 d_colour;
    b2Body*   d_body = nullptr;

public:
    static_physics_box(b2World& world, glm::vec2 pos, int width, int height, glm::vec4 colour, float angle = 0.0f)
        : d_width{width}
        , d_height{height}
        , d_colour{colour}
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_staticBody;
        const auto position = sand::pixel_to_physics(pos);
        bodyDef.position.Set(position.x, position.y);
        bodyDef.angle = angle;
        d_body = world.CreateBody(&bodyDef);

        b2PolygonShape box;
        const auto dimensions = sand::pixel_to_physics({width, height});
        box.SetAsBox(dimensions.x / 2, dimensions.y / 2);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &box;
        fixtureDef.friction = 1.0;
        d_body->CreateFixture(&fixtureDef);
    }

    auto centre() const -> glm::vec2 {
        return sand::physics_to_pixel(d_body->GetPosition());;
    }

    auto width() const {
        return d_width;
    }

    auto height() const {
        return d_height;
    }

    auto angle() const -> float {
        return d_body->GetAngle();
    }

    auto colour() const -> glm::vec4 {
        return d_colour;
    }
};

static constexpr auto offsets = {glm::ivec2{-1, 0}, glm::ivec2{0, -1}, glm::ivec2{1, 0}, glm::ivec2{0, 1}};

auto is_static_pixel(const sand::world& w, glm::ivec2 pos) -> bool
{
    if (!w.valid(pos)) return false;
    const auto& pixel = w.at(pos);
    const auto& props = sand::properties(pixel);
    return pixel.type != sand::pixel_type::none
        && props.phase == sand::pixel_phase::solid
        && !pixel.flags.test(sand::pixel_flags::is_falling);
}

auto is_boundary_point(const sand::world& w, glm::ivec2 pos) -> bool
{
    for (const auto offset : offsets) {
        const auto n = pos + offset;
        if (!is_static_pixel(w, n)) {
            return true;
        }
    }
    return false;
}

auto find_boundary(const sand::world& w, int x, int y) -> glm::ivec2
{
    auto current = glm::ivec2{x, y};
    while (!is_boundary_point(w, current)) { current.y -= 1; }
    return current;
}

auto is_air_boundary(const sand::world& w, glm::ivec2 A, glm::ivec2 B) -> bool
{
    const auto static_a = is_static_pixel(w, A);
    const auto static_b = is_static_pixel(w, B);
    return (!static_a && static_b) || (!static_b && static_a);
}

auto is_invalid_step(
    const sand::world& w,
    glm::ivec2 prev,
    glm::ivec2 curr,
    glm::ivec2 next) -> bool
{
    if (prev == next) return true;

    const auto tl = is_static_pixel(w, {curr.x - 1, curr.y - 1});
    const auto tr = is_static_pixel(w, {curr.x,     curr.y - 1});
    const auto bl = is_static_pixel(w, {curr.x - 1, curr.y    });
    const auto br = is_static_pixel(w, {curr.x,     curr.y    });

    using pt = sand::pixel_type;

    const auto cross1 = !tl && !br &&  tr &&  bl;
    const auto cross2 =  tl &&  br && !tr && !bl;

    if (!cross1 && !cross2) { return false; }

    // in a straight line
    if ((prev.x == curr.x && curr.x == next.x) || (prev.y == curr.y && curr.y == next.y)) {
        return true;
    }

    // 3 round a pixel
    const auto pixel = glm::ivec2{
        std::min({prev.x, curr.x, next.x}),
        std::min({prev.y, curr.y, next.y})
    };
    return !is_static_pixel(w, pixel);
}

auto is_valid_step(
    const sand::world& w,
    glm::ivec2 pos,
    glm::ivec2 offset) -> bool
{
    assert(glm::length2(offset) == 1);
    auto src = pos;
    auto dst = pos + offset;

    if (src.x == dst.x) { // vertical
        if (dst.y == src.y - 1) { // dst on top
            return is_air_boundary(w, dst, dst + glm::ivec2{-1, 0});
        } else { // src on top
            return is_air_boundary(w, src, src + glm::ivec2{-1, 0});
        }
    } else { // horizonal
        if (dst.x == src.x - 1) { // dst to left
            return is_air_boundary(w, dst, dst + glm::ivec2{0, -1});
        } else { // src to left
            return is_air_boundary(w, src, src + glm::ivec2{0, -1});
        }
    }
}

auto get_boundary(const sand::world& w, int x, int y) -> std::vector<glm::ivec2>
{
    auto ret = std::vector<glm::ivec2>{};
    auto current = find_boundary(w, x, y);
    ret.push_back(current);
    
    // Find second point
    bool found_second = false;
    for (const auto offset : offsets) {
        const auto neigh = current + offset;
        if (is_valid_step(w, current, offset)) {
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
            if (is_valid_step(w, current, offset) && !is_invalid_step(w, ret.rbegin()[1], current, neigh)) {
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

auto perpendicular_distance(glm::ivec2 p, glm::ivec2 a, glm::ivec2 b) -> float {
    if (a == b) { a.x++; } // little hack to avoid dividing by zero

    const auto ab = glm::vec2{b - a};
    const auto ap = glm::vec2{p - a};

    const auto cross_product = ab.x * ap.y - ab.y * ap.x;
    return glm::abs(cross_product) / glm::length(ab);
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

    // If the maximum distance is greater than epsilon, split the curve and recurse
    if (max_dist > epsilon) {
        ramer_douglas_puecker({ points.begin(), pivot + 1 }, epsilon, out);
        ramer_douglas_puecker({ pivot, points.end() },       epsilon, out);
    }

    // Other take the endpoints
    else {
        out.push_back(points.front());
        out.push_back(points.back());
    }
}

auto calc_boundary(const sand::world& w, int x, int y) -> std::vector<glm::ivec2>
{
    const auto points = get_boundary(w, x, y);
    auto simplified = std::vector<glm::ivec2>{};
    ramer_douglas_puecker(points, 1.5, simplified);
    return simplified;
}

auto main() -> int
{
    auto exe_path = sand::get_executable_filepath().parent_path();
    std::print("Executable directory: {}\n", exe_path.string());
    auto window = sand::window{"sandfall", 1280, 720};
    auto editor = sand::editor{};
    auto mouse = sand::mouse{};
    auto keyboard = sand::keyboard{};

    auto gravity = b2Vec2{0.0f, 10.0f};
    auto physics = b2World{gravity};

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

    auto world           = std::make_unique<sand::world>();
    auto world_renderer  = sand::renderer{};
    auto ui              = sand::ui{window};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto player          = sand::player_controller(physics, 5);
    auto shape_renderer  = sand::shape_renderer{};

    auto ground = std::vector<static_physics_box>{
        {physics, {128, 256 + 5}, 256, 10, {1.0, 1.0, 0.0, 1.0}},
    };

    auto file = std::ifstream{"save2.bin", std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};
    archive(*world);
    world->wake_all_chunks();

    auto points = calc_boundary(*world, 122, 233);
    auto count = 0;

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

            sand::update(*world);
            player.update(keyboard);
            physics.Step(sand::config::time_step, 8, 3);
            count++;
            if (count % 5 == 0) {
                if (world->at({122, 233}).type == sand::pixel_type::rock) {
                    points = calc_boundary(*world, 122, 233);
                } else {
                    points = {};
                }
            }
        }

        const auto mouse_pos = pixel_at_mouse(window, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (world->valid(coord)) {
                        world->set(coord, editor.get_pixel());
                        updated = true;
                    }
                }
            break; case 1:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (world->valid({x, y})) {
                                world->set({x, y}, editor.get_pixel());
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_down_this_frame(sand::mouse_button::left)) {
                    sand::apply_explosion(*world, mouse_pos, sand::explosion{
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

            ImGui::Text("Player");
            //ImGui::Text("On Ground: %d", is_on_ground(physics, player));
            //ImGui::Text("Num Contacts: %d", count);
            ImGui::Separator();

            ImGui::Text("Info");
            ImGui::Text("FPS: %d", timer.frame_rate());
            ImGui::Text("Awake chunks: %d", world->num_awake_chunks());
            ImGui::Checkbox("Show chunks", &editor.show_chunks);
            if (ImGui::Button("Clear")) {
                world->wake_all_chunks();
                world->fill(sand::pixel::air());
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
                    archive(*world);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    auto file = std::ifstream{filename, std::ios::binary};
                    auto archive = cereal::BinaryInputArchive{file};
                    archive(*world);
                    world->wake_all_chunks();
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
            world_renderer.update(*world, editor.show_chunks, camera);
        }
        world_renderer.draw();

        shape_renderer.begin_frame(camera);

        shape_renderer.draw_circle(player.centre(), {1.0, 1.0, 0.0, 1.0}, player.radius());
        
        for (const auto& obj : ground) {
            shape_renderer.draw_quad(obj.centre(), obj.width(), obj.height(), obj.angle(), obj.colour());
        }
        
        // Testing the line renderer
        for (const auto& obj : ground) {
            const auto& centre = obj.centre();

            const auto cos = glm::cos(obj.angle());
            const auto sin = glm::sin(obj.angle());
            const auto rotation = glm::mat2{cos, sin, -sin, cos};

            const auto rW = rotation * glm::vec2{obj.width() / 2.0, 0.0};
            const auto rH = rotation * glm::vec2{0.0, obj.height() / 2.0};

            const auto tl = centre - rW - rH;
            const auto tr = centre + rW - rH;
            const auto bl = centre - rW + rH;
            const auto br = centre + rW + rH;

            shape_renderer.draw_line(tl, tr, {1, 0, 0, 1}, {0, 0, 1, 1}, 1);
            shape_renderer.draw_line(tr, br, {1, 0, 0, 1}, {0, 0, 1, 1}, 1);
            shape_renderer.draw_line(br, bl, {1, 0, 0, 1}, {0, 0, 1, 1}, 1);
            shape_renderer.draw_line(bl, tl, {1, 0, 0, 1}, {0, 0, 1, 1}, 1);
        }
        if (points.size() >= 2) {
            for (size_t i = 0; i != points.size() - 1; i++) {
                shape_renderer.draw_line({points[i]}, {points[i+1]}, {1,0,0,1}, {1,0,0,1}, 1);
                shape_renderer.draw_circle({points[i]}, {0, 0, 1, 1}, 0.25);
                
            }
        }
        //for (const auto point : points) {
        //    shape_renderer.draw_circle(point, {1, 1, 0, 1}, 0.25);
        //}
        shape_renderer.end_frame();
        
        // Display the UI
        ui.end_frame();

        window.swap_buffers();
    }
    
    return 0;
}