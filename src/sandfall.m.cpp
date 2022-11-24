#include "world.hpp"
#include "pixel.hpp"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"
#include "explosion.hpp"
#include "mouse.hpp"

#include "graphics/renderer.hpp"
#include "graphics/window.hpp"
#include "graphics/ui.hpp"

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include <memory>

auto pixel_at_mouse(const sand::window& w, const sand::camera& c) -> glm::ivec2
{
    const auto mouse = w.get_mouse_pos();
    const auto scale = (float)c.zoom / w.height();
    return glm::ivec2{mouse * scale + c.top_left};
}

auto main() -> int
{
    auto exe_path = sand::get_executable_filepath().parent_path();
    sand::print("Executable directory: {}\n", exe_path.string());
    auto window = sand::window{"sandfall", 1280, 720};
    auto editor = sand::editor{};
    auto mouse = sand::mouse{};

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = static_cast<float>(window.width()),
        .screen_height = static_cast<float>(window.height()),
        .zoom = 256
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

        if (mouse.is_button_down(sand::mouse_button::right) && event.is<sand::mouse_moved_event>()) {
            const auto& e = event.as<sand::mouse_moved_event>();
            const auto scale = (float)camera.zoom / window.height();
            camera.top_left -= glm::vec2{e.x_offset * scale, e.y_offset * scale};
        }
        else if (event.is<sand::window_resize_event>()) {
            camera.screen_width = window.width();
            camera.screen_height = window.height();
        }
        else if (event.is<sand::mouse_scrolled_event>()) {
            const auto& e = event.as<sand::mouse_scrolled_event>();
            const auto old_centre = pixel_at_mouse(window, camera);
            camera.zoom -= 5 * e.y_offset;
            camera.zoom = std::clamp(camera.zoom, 50, 500);
            const auto new_centre = pixel_at_mouse(window, camera);
            camera.top_left -= new_centre - old_centre;
        }
    });

    auto world       = std::make_unique<sand::world>();
    auto renderer    = sand::renderer{};
    auto ui          = sand::ui{window};
    auto accumulator = 0.0;
    auto timer       = sand::timer{};

    while (window.is_running()) {
        const double dt = timer.on_update();

        mouse.on_new_frame();
        
        window.poll_events();
        window.clear();

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            world->simulate();
            accumulator -= sand::config::time_step;
            updated = true;
        }

        // Draw the world
        if (updated) {
            renderer.update(*world, editor.show_chunks, camera);
        }
        renderer.draw();

        // Next, draw the editor UI
        ui.begin_frame();
        display_ui(editor, *world, timer, window, pixel_at_mouse(window, camera));
        ui.end_frame();
        
        const auto mouse_pos = pixel_at_mouse(window, camera);
        switch (editor.brush_type) {
            break; case 0:
                if (mouse.is_button_down(sand::mouse_button::left)) {
                    const auto coord = mouse_pos + sand::random_from_circle(editor.brush_size);
                    if (world->valid(coord)) {
                        world->set(coord, editor.get_pixel());
                    }
                }
            break; case 1:
                if (mouse.is_button_down(sand::mouse_button::left)) {
                    const auto half_extent = (int)(editor.brush_size / 2);
                    for (int x = mouse_pos.x - half_extent; x != mouse_pos.x + half_extent + 1; ++x) {
                        for (int y = mouse_pos.y - half_extent; y != mouse_pos.y + half_extent; ++y) {
                            if (world->valid({x, y})) {
                                world->set({x, y}, editor.get_pixel());
                            }
                        }
                    }
                }
            break; case 2:
                if (mouse.is_button_down(sand::mouse_button::left)) {
                    if (world->valid(mouse_pos)) {
                        world->set(mouse_pos, editor.get_pixel());
                    }
                }
            break; case 3:
                if (mouse.is_button_clicked(sand::mouse_button::left)) {
                    sand::apply_explosion(*world, mouse_pos, sand::explosion{
                        .min_radius = 40.0f, .max_radius = 45.0f, .scorch = 10.0f
                    });
                }
        }
        window.swap_buffers();
    }
    
    return 0;
}