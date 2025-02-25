#include "world.hpp"
#include "pixel.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "update_rigid_bodies.hpp"
#include "explosion.hpp"
#include "mouse.hpp"
#include "player.hpp"
#include "world_save.hpp"

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

auto render_body_triangles(sand::shape_renderer& rend, const b2Body* body) -> void
{
    if (!body) return;
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

auto save_world(const std::string& file_path, const sand::level& w) -> void
{
    auto file = std::ofstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryOutputArchive{file};

    auto save = sand::world_save{
        .pixels = w.pixels.data(),
        .width = w.pixels.width(),
        .height = w.pixels.height(),
        .spawn_point = w.spawn_point
    };

    archive(save);
}

auto load_world(const std::string& file_path) -> std::unique_ptr<sand::level>
{
    auto file = std::ifstream{file_path, std::ios::binary};
    auto archive = cereal::BinaryInputArchive{file};

    auto save = sand::world_save{};
    archive(save);

    auto w = std::make_unique<sand::level>(save.width, save.height, save.pixels);
    w->spawn_point = save.spawn_point;
    w->player.set_position(save.spawn_point);
    return w;
}

auto new_world(int chunks_width, int chunks_height) -> std::unique_ptr<sand::level>
{
    const auto width = sand::config::chunk_size * chunks_width;
    const auto height = sand::config::chunk_size * chunks_height;
    return std::make_unique<sand::level>(
        width,
        height,
        std::vector<sand::pixel>(width * height, sand::pixel::air())
    );
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

    auto world           = new_world(4, 4);
    auto world_renderer  = sand::renderer{world->pixels.width(), world->pixels.height()};
    auto ui              = sand::ui{window};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};
    auto show_triangles = false;
    auto show_spawn     = false;

    auto new_world_chunks_width  = 4;
    auto new_world_chunks_height = 4;

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
            world->pixels.step();
            world->player.update(keyboard);
        }

        const auto mouse_pos = pixel_at_mouse(window, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (world->pixels.valid(coord)) {
                        world->pixels[coord] = editor.get_pixel();
                        world->pixels.wake_chunk_with_pixel(coord);
                        updated = true;
                    }
                }
            break; case 1:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (world->pixels.valid({x, y})) {
                                world->pixels[{x, y}] = editor.get_pixel();
                                world->pixels.wake_chunk_with_pixel({x, y});
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_down_this_frame(sand::mouse_button::left)) {
                    sand::apply_explosion(world->pixels, mouse_pos, sand::explosion{
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
            ImGui::Checkbox("Show Spawn", &show_spawn);
            ImGui::SliderInt("Spawn X", &world->spawn_point.x, 0, world->pixels.width());
            ImGui::SliderInt("Spawn Y", &world->spawn_point.y, 0, world->pixels.height());
            if (ImGui::Button("Respawn")) {
                world->player.set_position(world->spawn_point);
            }
            ImGui::Separator();

            ImGui::Text("Info");
            ImGui::Text("FPS: %d", timer.frame_rate());
            ImGui::Text("Awake chunks: %d", std::count_if(world->pixels.chunks().begin(), world->pixels.chunks().end(), [](const sand::chunk& c) {
                return c.should_step;
            }));
            ImGui::Checkbox("Show chunks", &editor.show_chunks);
            if (ImGui::Button("Clear")) {
                world->pixels.wake_all();
                std::fill(world->pixels.begin(), world->pixels.end(), sand::pixel::air());
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
            ImGui::InputInt("chunk width", &new_world_chunks_width);
            ImGui::InputInt("chunk height", &new_world_chunks_height);
            if (ImGui::Button("New World")) {
                world = new_world(new_world_chunks_width, new_world_chunks_height);
                updated = true;
            }
            ImGui::Text("Levels");
            for (int i = 0; i != 5; ++i) {
                ImGui::PushID(i);
                const auto filename = std::format("save{}.bin", i);
                if (ImGui::Button("Save")) {
                    save_world(filename, *world);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    world = load_world(filename);
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

        shape_renderer.draw_circle(world->player.centre(), {1.0, 1.0, 0.0, 1.0}, world->player.radius());

        if (show_triangles) {
            for (const auto& chunk : world->pixels.chunks()) {
                render_body_triangles(shape_renderer, chunk.triangles);
            }
        }

        if (show_spawn) {
            shape_renderer.draw_circle(world->spawn_point, {0, 1, 0, 1}, 1.0);
        }

        shape_renderer.end_frame();
        
        // Display the UI
        ui.end_frame();

        window.swap_buffers();
    }
    
    return 0;
}