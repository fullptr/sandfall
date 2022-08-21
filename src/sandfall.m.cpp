#include "tile.h"
#include "pixel.h"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"
#include "camera.hpp"

#include "graphics/renderer.hpp"
#include "graphics/window.h"
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
    auto window = sand::window{"sandfall", 1280, 720};
    auto editor = sand::editor{};
    auto mouse = std::array<bool, 5>{}; // TODO: Think of a better way

    auto camera = sand::camera{
        .top_left = {0, 0},
        .screen_width = static_cast<float>(window.width()),
        .screen_height = static_cast<float>(window.height()),
        .zoom = 256
    };

    window.set_callback([&](sand::event& event) {
        auto& io = ImGui::GetIO();
        if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
            return;
        }
        if (event.is_mount_event() && io.WantCaptureMouse) {
            return;
        }

        if (event.is<sand::mouse_pressed_event>()) {
            mouse[event.as<sand::mouse_pressed_event>().button] = true;
        }
        else if (event.is<sand::mouse_released_event>()) {
            mouse[event.as<sand::mouse_released_event>().button] = false;
        }
        else if (mouse[1] && event.is<sand::mouse_moved_event>()) {
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
            camera.zoom -= 5 * e.y_offset;
        }
    });

    auto tile = std::make_unique<sand::tile>();
    auto renderer = sand::renderer{window.width(), window.height()};
    auto ui = sand::ui{window};
    auto accumulator = 0.0;
    auto timer = sand::timer{};

    while (window.is_running()) {
        const double dt = timer.on_update();
        
        window.poll_events();
        window.clear();

        accumulator += dt;
        bool updated = false;
        while (accumulator > sand::config::time_step) {
            tile->simulate();
            accumulator -= sand::config::time_step;
            updated = true;
        }

        // Draw the world
        if (updated) {
            renderer.update(*tile, editor.show_chunks, camera);
        }
        renderer.draw();

        // Next, draw the editor UI
        ui.begin_frame();
        display_ui(editor, *tile, timer, window);
        ui.end_frame();
        
        if (mouse[0]) {
            const auto mouse = pixel_at_mouse(window, camera);
            if (editor.brush_type == 0) {
                const auto coord = mouse + sand::random_from_circle(editor.brush_size);
                if (tile->valid(coord)) {
                    tile->set(coord, editor.get_pixel());
                }
            }
            if (editor.brush_type == 1) {
                const auto half_extent = (int)(editor.brush_size / 2);
                for (int x = mouse.x - half_extent; x != mouse.x + half_extent; ++x) {
                    for (int y = mouse.y - half_extent; y != mouse.y + half_extent; ++y) {
                        if (tile->valid({x, y})) {
                            tile->set({x, y}, editor.get_pixel());
                        }
                    }
                }
            }
        }

        window.swap_buffers();
    }
    
    return 0;
}