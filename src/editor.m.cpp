#include "world.hpp"
#include "pixel.hpp"
#include "common.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "update_rigid_bodies.hpp"
#include "explosion.hpp"
#include "mouse.hpp"
#include "player.hpp"
#include "world_save.hpp"
#include "renderer.hpp"
#include "shape_renderer.hpp"
#include "window.hpp"
#include "serialisation.hpp"

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

class physics_debug_draw : public b2Draw
{
    sand::shape_renderer* d_renderer;

public:
    physics_debug_draw(sand::shape_renderer* s) : d_renderer{s} {
        SetFlags(b2Draw::e_shapeBit);
    }

    void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
    {
        assert(vertexCount > 1);
        for (std::size_t i = 0; i < vertexCount - 1; ++i) {
            const auto p1 = sand::physics_to_pixel(vertices[i]);
            const auto p2 = sand::physics_to_pixel(vertices[i + 1]);
            d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
        }
        const auto p1 = sand::physics_to_pixel(vertices[vertexCount - 1]);
        const auto p2 = sand::physics_to_pixel(vertices[0]);
        d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
    }
    
    void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
    {
        DrawPolygon(vertices, vertexCount, color);
    }
    
    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override
    {
        d_renderer->draw_annulus(
            sand::physics_to_pixel(center),
            {color.r, color.g, color.b, 1.0},
            0.8f * sand::physics_to_pixel(radius),
            sand::physics_to_pixel(radius)
        );
    }

	void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override
    {
        DrawCircle(center, radius, color);
    }

	void DrawSegment(const b2Vec2& bp1, const b2Vec2& bp2, const b2Color& color) override
    {
        const auto p1 = sand::physics_to_pixel(bp1);
        const auto p2 = sand::physics_to_pixel(bp2);
        d_renderer->draw_line(p1, p2, {color.r, color.g, color.b, 1.0}, 1);
    }

	void DrawTransform(const b2Transform& xf) override
    {
    }

	void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override
    {
        d_renderer->draw_circle(
            sand::physics_to_pixel(p),
            {color.r, color.g, color.b, 1.0},
            2.0f
        );
    }
};

auto main() -> int
{
    auto window          = sand::window{"sandfall", 1280, 720};
    auto editor          = sand::editor{};
    auto mouse           = sand::mouse{};
    auto keyboard        = sand::keyboard{};
    auto level           = sand::new_level(4, 4);
    auto world_renderer  = sand::renderer{level->pixels.width(), level->pixels.height()};
    auto accumulator     = 0.0;
    auto timer           = sand::timer{};
    auto shape_renderer  = sand::shape_renderer{};
    auto debug_draw      = physics_debug_draw{&shape_renderer};

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
        mouse.on_new_frame();
        keyboard.on_new_frame();

        for (const auto event : window.events()) {
            auto& io = ImGui::GetIO();
            if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
                continue;
            }
            if (event.is_mount_event() && io.WantCaptureMouse) {
                continue;
            }

            mouse.on_event(event);
            keyboard.on_event(event);

            if (const auto e = event.get_if<sand::window_resize_event>()) {
                camera.screen_width = e->width;
                camera.screen_height = e->height;
            }
            else if (const auto e = event.get_if<sand::mouse_scrolled_event>()) {
                const auto old_centre = mouse_pos_world_space(mouse, camera);
                camera.world_to_screen += 0.1f * e->offset.y;
                camera.world_to_screen = std::clamp(camera.world_to_screen, 1.0f, 100.0f);
                const auto new_centre = mouse_pos_world_space(mouse, camera);
                camera.top_left -= new_centre - old_centre;
            }
        }

        if (mouse.is_down(sand::mouse_button::right)) {
            camera.top_left -= mouse.offset() / camera.world_to_screen;
        }

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            accumulator -= sand::config::time_step;
            updated = true;
            level->pixels.step();
        }
        level->player.update(keyboard);

        const auto mouse_pos = pixel_at_mouse(mouse, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (level->pixels.valid({coord.x, coord.y})) {
                        level->pixels.set(coord, editor.get_pixel());
                        updated = true;
                    }
                }
            break; case 1:
                if (mouse.is_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent + 1; ++y) {
                            if (level->pixels.valid({x, y})) {
                                level->pixels.set({x, y}, editor.get_pixel());
                                updated = true;
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_down_this_frame(sand::mouse_button::left)) {
                    sand::apply_explosion(level->pixels, mouse_pos, sand::explosion{
                        .min_radius = 40.0f, .max_radius = 45.0f, .scorch = 10.0f
                    });
                    updated = true;
                }
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto mouse_actual = mouse_pos_world_space(mouse, camera);
        const auto mouse_pixel = pixel_at_mouse(mouse, camera);

        ImGui::ShowDemoWindow(&editor.show_demo);

        if (ImGui::Begin("Editor")) {
            ImGui::Text("Mouse");
            ImGui::Text("Position: {%.2f, %.2f}", mouse_actual.x, mouse_actual.y);
            ImGui::Text("Pixel: {%d, %d}", mouse_pixel.x, mouse_pixel.y);
            if (level->pixels.valid({mouse_pixel.x, mouse_pixel.y})) {
                const auto px = level->pixels[mouse_pixel];
                ImGui::Text("  pixel power: %d", px.power);
                ImGui::Text("  is_falling: %s", px.flags[sand::pixel_flags::is_falling] ? "true" : "false");
            } else {
                ImGui::Text("  pixel power: n/a");
                ImGui::Text("  is_falling: n/a");
            }
            ImGui::Text("Number of Floors: %d", level->player.floors().size());
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
            ImGui::SliderInt("Spawn X", &level->spawn_point.x, 0, level->pixels.width());
            ImGui::SliderInt("Spawn Y", &level->spawn_point.y, 0, level->pixels.height());
            if (ImGui::Button("Respawn")) {
                level->player.set_position(level->spawn_point);
            }
            ImGui::Separator();

            ImGui::Text("Info");
            ImGui::Text("FPS: %d", timer.frame_rate());
            ImGui::Text("Awake chunks: %d", std::count_if(level->pixels.chunks().begin(), level->pixels.chunks().end(), [](const sand::chunk& c) {
                return c.should_step;
            }));
            ImGui::Checkbox("Show chunks", &editor.show_chunks);
            if (ImGui::Button("Clear")) {
                level->pixels.wake_all();
                std::fill(level->pixels.begin(), level->pixels.end(), sand::pixel::air());
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
                    save_level(filename, *level);
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

        // Render and display the world
        world_renderer.bind();
        if (updated) {
            world_renderer.update(*level, editor.show_chunks, camera);
        }
        world_renderer.draw();

        shape_renderer.begin_frame(camera);

        // TODO: Replace with actual sprite data
        shape_renderer.draw_circle(level->player.centre(), {1.0, 1.0, 0.0, 1.0}, 3);

        if (editor.show_physics) {
            level->pixels.physics().SetDebugDraw(&debug_draw);
            level->pixels.physics().DebugDraw();
        }

        if (editor.show_spawn) {
            shape_renderer.draw_circle(level->spawn_point, {0, 1, 0, 1}, 1.0);
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