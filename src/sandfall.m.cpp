#include "tile.h"
#include "pixel.h"
#include "config.hpp"
#include "utility.hpp"
#include "editor.hpp"

#include "graphics/renderer.hpp"
#include "graphics/window.h"
#include "graphics/ui.hpp"

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include <memory>

auto pixel_at_mouse(const sand::window& w, std::uint32_t width, std::uint32_t height) -> glm::ivec2
{
    const auto mouse = w.get_mouse_pos();
    return {
        (mouse.x / w.width()) * width,
        (mouse.y / w.height()) * height
    };
}

auto main() -> int
{
    auto window = sand::window{"sandfall", 1280, 720};
    auto left_mouse_down = false; // TODO: Remove, do it in a better way

    window.set_callback([&](sand::event& event) {
        auto& io = ImGui::GetIO();
        if (event.is_keyboard_event() && io.WantCaptureKeyboard) {
            return;
        }
        if (event.is_mount_event() && io.WantCaptureMouse) {
            return;
        }

        if (event.is<sand::mouse_pressed_event>()) {
            if (event.as<sand::mouse_pressed_event>().button == 0) { left_mouse_down = true; }
        }
        else if (event.is<sand::mouse_released_event>()) {
            if (event.as<sand::mouse_released_event>().button == 0) { left_mouse_down = false; }
        }
    });

    auto tile = std::make_unique<sand::tile>();
    auto renderer = sand::renderer{window.width(), window.height()};
    auto editor = sand::editor{};
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

        const auto camera_width = window.width() / 3;
        const auto camera_height = window.height() / 3;

        // Draw the world
        if (updated) {
            renderer.update(*tile, editor.show_chunks, camera_width, camera_height);
        }
        renderer.draw();

        // Next, draw the editor UI
        ui.begin_frame();
        display_ui(editor, *tile, timer);
        ui.end_frame();
        
        if (left_mouse_down) {
            const auto mouse = pixel_at_mouse(window, camera_width, camera_height);
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