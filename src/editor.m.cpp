#include "world.hpp"
#include "pixel.hpp"
#include "common.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "update_rigid_bodies.hpp"
#include "explosion.hpp"
#include "input.hpp"
#include "entity.hpp"
#include "world_save.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"
#include "window.hpp"
#include "serialisation.hpp"
#include "debug.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <box2d/box2d.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include <memory>
#include <print>
#include <fstream>
#include <cmath>
#include <span>

auto num_awake_chunks(const sand::pixel_world& w) -> sand::u64
{
    auto count = 0;
    for (sand::i32 x = 0; x != w.width_in_chunks(); ++x) {
        for (sand::i32 y = 0; y != w.height_in_chunks(); ++y) {
            if (w[sand::chunk_pos{x, y}].should_step) {
                ++count;
            }
        }
    }
    return count;
}

auto clear_world(sand::pixel_world& w) -> void
{
    w.wake_all();
    for (sand::i32 x = 0; x != w.width_in_pixels(); ++x) {
        for (sand::i32 y = 0; y != w.height_in_pixels(); ++y) {
            w.visit_no_wake({x, y}, [](sand::pixel& p) {
                p = sand::pixel::air();
            });
        }
    }
}

auto main() -> int
{
    using namespace sand;
    auto window          = sand::window{"sandfall", 1280, 720};
    auto editor          = sand::editor{};
    auto input           = sand::input{};
    auto level           = sand::new_level(4, 4);
    auto world_renderer  = sand::renderer{level.pixels.width_in_pixels(), level.pixels.height_in_pixels()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};

    b2DebugDraw debug = b2DefaultDebugDraw();
    debug.context = static_cast<void*>(&shape_renderer);
    debug.drawShapes = true;

    debug.DrawPolygonFcn = draw_polygon;
    debug.DrawCircleFcn = draw_circle;

    auto update_window_half_width = 2 + sand::config::chunk_size;
    auto update_window_half_height = 2 + sand::config::chunk_size;

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = window.width(),
        .screen_height = window.height(),
        .world_to_screen = 720.0f / 256.0f
    };

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.native_handle(), true);
    ImGui_ImplOpenGL3_Init("#version 410");

    while (window.is_running()) {
        const double dt = timer.on_update();
        window.begin_frame();
        input.on_new_frame();

        for (const auto event : window.events()) {
            auto& io = ImGui::GetIO();
            if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
                continue;
            }
            if (event.is_mount_event() && io.WantCaptureMouse) {
                continue;
            }

            input.on_event(event);

            if (const auto e = event.get_if<sand::window_resize_event>()) {
                camera.screen_width = e->width;
                camera.screen_height = e->height;
            }
            else if (const auto e = event.get_if<sand::mouse_scrolled_event>()) {
                const auto old_centre = mouse_pos_world_space(input, camera);
                camera.world_to_screen += 0.1f * e->offset.y;
                camera.world_to_screen = std::clamp(camera.world_to_screen, 1.0f, 100.0f);
                const auto new_centre = mouse_pos_world_space(input, camera);
                camera.top_left -= new_centre - old_centre;
            }
        }

        if (input.is_down(mouse::right)) {
            camera.top_left -= input.offset() / camera.world_to_screen;
        }

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;
            level.pixels.step();
        }

        const auto mouse_pos = pixel_at_mouse(input, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (input.is_down(mouse::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (level.pixels.is_valid_pixel(coord)) {
                        level.pixels.set(coord, editor.get_pixel());
                        updated = true;
                    }
                }
            break; case 1:
                if (input.is_down(mouse::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (level.pixels.is_valid_pixel({x, y})) {
                                level.pixels.set({x, y}, editor.get_pixel());
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (input.is_down_this_frame(mouse::left)) {
                    sand::apply_explosion(level.pixels, mouse_pos, sand::explosion{
                        .min_radius = 40.0f, .max_radius = 45.0f, .scorch = 10.0f
                    });
                    updated = true;
                }
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto mouse_actual = mouse_pos_world_space(input, camera);
        const auto mouse_pixel = pixel_at_mouse(input, camera);

        ImGui::ShowDemoWindow(&editor.show_demo);

        if (ImGui::Begin("Editor")) {
            ImGui::Text("Mouse");
            ImGui::Text("Position: {%.2f, %.2f}", mouse_actual.x, mouse_actual.y);
            ImGui::Text("Pixel: {%d, %d}", mouse_pixel.x, mouse_pixel.y);
            if (level.pixels.is_valid_pixel(mouse_pixel)) {
                const auto px = level.pixels[mouse_pixel];
                ImGui::Text("  pixel power: %d", px.power);
                ImGui::Text("  is_falling: %s", px.flags[sand::pixel_flags::is_falling] ? "true" : "false");
            } else {
                ImGui::Text("  pixel power: n/a");
                ImGui::Text("  is_falling: n/a");
            }
            ImGui::Text("Events this frame: %zu", window.events().size());
            ImGui::Separator();

            ImGui::Text("Camera");
            ImGui::Text("Top Left: {%.2f, %.2f}", camera.top_left.x, camera.top_left.y);
            ImGui::Text("Screen width: %.2f", camera.screen_width);
            ImGui::Text("Screen height: %.2f", camera.screen_height);
            ImGui::Text("Scale: %f", camera.world_to_screen);

            ImGui::Separator();
            ImGui::Checkbox("Show Physics", &editor.show_physics);
            ImGui::Checkbox("Show Spawn", &editor.show_spawn);
            ImGui::SliderInt("Spawn X", &level.spawn_point.x, 0, level.pixels.width_in_pixels());
            ImGui::SliderInt("Spawn Y", &level.spawn_point.y, 0, level.pixels.height_in_pixels());
            ImGui::Separator();

            ImGui::Text("Info");
            ImGui::Text("FPS: %d", timer.frame_rate());
            ImGui::Text("Awake chunks: %d", num_awake_chunks(level.pixels));
            ImGui::Checkbox("Show chunks", &editor.show_chunks);
            if (ImGui::Button("Clear")) {
                clear_world(level.pixels);
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
            ImGui::InputInt("chunk width", &editor.new_world_chunks_width);
            ImGui::InputInt("chunk height", &editor.new_world_chunks_height);
            if (ImGui::Button("New World")) {
                level = sand::new_level(editor.new_world_chunks_width, editor.new_world_chunks_height);
                updated = true;
            }
            ImGui::Text("Levels");
            for (int i = 0; i != 5; ++i) {
                ImGui::PushID(i);
                const auto filename = std::format("save{}.bin", i);
                if (ImGui::Button("Save")) {
                    save_level(filename, level);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    level = sand::load_level(filename);
                    updated = true;
                }
                ImGui::SameLine();
                ImGui::Text("Save %d", i);
                ImGui::PopID();
            }
            static std::string filepath;
            ImGui::InputText("Load PNG", &filepath);
            if (ImGui::Button("Try Load")) {
                std::ifstream ifs{filepath, std::ios_base::in | std::ios_base::binary};
                std::vector<char> buffer(
                    (std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>()
                );
                std::print("loaded a file containing {} bytes\n", buffer.size());
            }
        }
        ImGui::End();

        // The shape renderer is used twice per frame, which is a bit of a hack,
        // but an easy way to draw a quad behind the level.
        shape_renderer.begin_frame(camera);
        shape_renderer.draw_rect(
            {0, 0},
            config::chunk_size * level.pixels.width_in_chunks(),
            config::chunk_size * level.pixels.height_in_chunks(),
            {0.1, 0.1, 0.1, 1.0}
        );
        shape_renderer.end_frame();

        // Render and display the world
        if (updated) {
            world_renderer.update(level);
        }
        world_renderer.draw(camera);

        shape_renderer.begin_frame(camera);

        if (editor.show_physics) {
            b2World_Draw(level.physics.world, &debug);
        }

        if (editor.show_spawn) {
            const auto p = glm::ivec2{level.spawn_point.x, level.spawn_point.y};
            shape_renderer.draw_circle(p, {0, 1, 0, 1}, 1.0);
        }

        if (editor.show_chunks) {
            for (i32 cx = 0; cx != level.pixels.width_in_chunks(); ++cx) {
                for (i32 cy = 0; cy != level.pixels.height_in_chunks(); ++cy) {
                    const auto cpos = chunk_pos{cx, cy};
                    const auto top_left = get_chunk_top_left(cpos);
                    const auto chunk = level.pixels[cpos];
                    if (chunk.should_step) {
                        shape_renderer.draw_rect(glm::ivec2{top_left}, config::chunk_size, config::chunk_size, {1, 1, 1, 0.1});
                    }
                }
            }
        }

        shape_renderer.end_frame();
        
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.end_frame();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    return 0;
}